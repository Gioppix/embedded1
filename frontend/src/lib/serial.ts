import { writable } from 'svelte/store';
import { BACKEND_TO_FRONTEND, BAUD, BITS_PER_COLOR, SCREENX, SCREENY } from './generated';

// Reactive stores for the Svelte component to subscribe to
export const is_connected = writable<boolean>(false);
export const status_message = writable<string>("Click 'Connect' to select the serial port.");
export const bytes_per_second = writable<number>(0);
export const ready_frame = writable<number[]>(Array(SCREENX * SCREENY).fill(0));
let bytes_queue: number[] = [];

const COLORS_PER_BYTE = Math.floor((8 - 1) / BITS_PER_COLOR);
const TOTAL_FRAME_TRANSFER_BYTES = SCREENY * Math.floor(SCREENX / COLORS_PER_BYTE);
let frame: number[] = [];
let frame_bytes_read = 0;
let byte_n = 0; // Assuming this is for debugging, retained.

// --- New Configuration ---
const ARDUINO_BOOT_DELAY_MS = 2000; // Wait 2 seconds for Arduino to boot. Adjust as needed.

function reset() {
	frame_bytes_read = 0;
	frame = [];
	// byte_n = 0; // Reset byte_n if it's per-connection/frame sequence
}

let port: SerialPort | null = null;
let reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
let writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
let keep_reading = false;
let _is_connected_internal = false;

is_connected.subscribe((value) => (_is_connected_internal = value));

setInterval(process, 10);

function process() {
	let byte_command = bytes_queue.shift();
	if (byte_command == undefined) {
		return;
	}
	byte_n++;

	let is_command = !!(byte_command & (1 << 7));
	let byte = byte_command & ~(1 << 7);

	if (is_command) {
		// Command
		switch (byte) {
			case BACKEND_TO_FRONTEND.BOOTED:
				reset();
				console.info('booted msg');
				break;
			case BACKEND_TO_FRONTEND.FRAME_START:
				console.info('FRAME_START');
				reset();
				break;
			case BACKEND_TO_FRONTEND.FRAME_END:
				console.log('FRAME_END');
				if (frame_bytes_read != TOTAL_FRAME_TRANSFER_BYTES) {
					// Consider a more graceful error recovery or logging here
					console.error(
						`Frame data mismatch. Expected ${TOTAL_FRAME_TRANSFER_BYTES}, got ${frame_bytes_read}. Resetting.`
					);
					// Optionally send an error status to the UI
					// status_message.set("Error: Incomplete frame received.");
					reset(); // Reset to try and recover
					// Do not throw an error that stops the read loop, unless it's unrecoverable
					break; // Break from switch, process() will be called again if queue has data
				}
				ready_frame.set(structuredClone(frame));
				reset();
				break;
			default:
				console.warn(`Unsupported command: ${byte}. Discarding.`);
				// To prevent errors from stopping the process, just log and continue
				// throw new Error('unsupported command');
				break; // Break from switch
		}
	} else {
		// Data
		frame_bytes_read++;
		for (let i = 0; i < COLORS_PER_BYTE; i++) {
			const mask = (1 << BITS_PER_COLOR) - 1;
			const extractedBits = (byte >> (i * BITS_PER_COLOR)) & mask;
			frame.push(extractedBits);
		}

		if (frame_bytes_read >= TOTAL_FRAME_TRANSFER_BYTES) {
			// Spontaneous transition to awaiting_command if a full frame is read
			// This can happen if FRAME_END is missed but enough bytes for a frame are received.
			// The next byte should ideally be FRAME_END or FRAME_START.
			// If it's FRAME_END, it will be handled in the next process() call.
			// If it's FRAME_START, it will also be handled.
			// If it's garbage, it will be caught by the default in awaiting_command.
		}
	}

	if (bytes_queue.length > 0) {
		process();
	}
}

let current_byte_count = 0;
setInterval(() => {
	bytes_per_second.set(current_byte_count);
	current_byte_count = 0;
}, 1000);

async function read_loop() {
	try {
		while (port?.readable && keep_reading && reader) {
			const { value, done } = await reader.read();
			if (done) {
				// reader.cancel() has been called.
				break;
			}
			if (value && value.length > 0) {
				current_byte_count += value.length;
				bytes_queue.push(...value);
			}
		}
	} catch (error: any) {
		if (keep_reading) {
			status_message.set(`Read loop error: ${error.message}. Connection may be lost.`);
			console.error('Read loop error:', error);
			// Consider triggering a disconnect or attempting a reconnect here
			await disconnect_serial_port_internal();
			is_connected.set(false);
			status_message.set(`Connection lost due to read error: ${error.message}. Please reconnect.`);
		}
	} finally {
		console.log('Exited read loop.');
		// Ensure reader is released if loop exits unexpectedly while still "connected"
		if (reader && keep_reading) {
			try {
				await reader.cancel(); // This should lead to reader.releaseLock() being safe
				reader.releaseLock();
			} catch (e) {
				/* ignore */
			}
			reader = null;
		}
	}
}

export async function connect_to_serial_port() {
	if (!('serial' in navigator)) {
		status_message.set('Web Serial API is not available in this browser.');
		console.error('Web Serial API not available.');
		alert('Web Serial API is not supported by your browser. Please use Chrome or Edge.');
		return;
	}

	// Reset any previous connection state fully before attempting a new one
	if (_is_connected_internal || port) {
		await disconnect_serial_port();
	}
	bytes_queue = []; // Clear any residual data in the queue
	reset(); // Reset framing logic

	try {
		status_message.set('Requesting serial port selection...');
		const requested_port = await navigator.serial.requestPort();
		port = requested_port;

		const port_info = port.getInfo();
		const port_identifier = port_info.usbProductId
			? `USB VID:PID ${port_info.usbVendorId}:${port_info.usbProductId}`
			: 'Bluetooth SPP Device';
		status_message.set(`Opening port ${port_identifier}...`);
		console.log('Port selected:', port_info);

		await port.open({
			baudRate: BAUD,
			dataBits: 8,
			parity: 'none',
			stopBits: 1,
			bufferSize: 16000
		});

		status_message.set(
			`Port ${port_identifier} opened. Waiting for device to boot (approx. ${
				ARDUINO_BOOT_DELAY_MS / 1000
			}s)...`
		);

		// --- Stale Data Purge and Boot Wait Logic ---
		let temp_reader_for_boot: ReadableStreamDefaultReader<Uint8Array> | null = null;
		if (port.readable) {
			temp_reader_for_boot = port.readable.getReader();
			let total_bytes_discarded = 0;
			const boot_wait_end_time = Date.now() + ARDUINO_BOOT_DELAY_MS;

			try {
				while (Date.now() < boot_wait_end_time) {
					const time_left = boot_wait_end_time - Date.now();
					if (time_left <= 0) break;

					// Read with a timeout shorter than the remaining boot wait time
					const { value, done } = await Promise.race([
						temp_reader_for_boot.read(),
						new Promise(
							(resolve) =>
								setTimeout(
									() => resolve({ value: undefined, done: false }),
									Math.min(time_left, 200)
								) // Race with a timeout
						)
					]);

					if (done) {
						// Stream closed during boot wait
						throw new Error('Serial port closed unexpectedly during device boot wait.');
					}
					if (value && value.length > 0) {
						total_bytes_discarded += value.length;
						// console.log(`Discarded ${value.length} byte(s) during boot wait.`);
					}
				}
				console.log(
					`Finished boot wait. Discarded approximately ${total_bytes_discarded} stale/boot bytes.`
				);
			} catch (e: any) {
				// If temp_reader_for_boot.read() itself throws (e.g. device unplugged)
				if (temp_reader_for_boot) {
					try {
						await temp_reader_for_boot.cancel(); // Attempt to cancel before releasing
						temp_reader_for_boot.releaseLock();
					} catch (releaseError) {
						console.warn(
							'Error releasing temporary reader lock during boot wait error handling:',
							releaseError
						);
					}
				}
				temp_reader_for_boot = null;
				throw e; // Re-throw the error to be caught by the main catch block
			} finally {
				if (temp_reader_for_boot) {
					temp_reader_for_boot.releaseLock();
				}
			}
		}
		// --- End Stale Data Purge and Boot Wait Logic ---

		// Ensure main reader and writer are set up *after* boot wait
		if (port.readable) {
			reader = port.readable.getReader();
		} else {
			throw new Error('Serial port is not readable after boot wait.');
		}
		if (port.writable) {
			writer = port.writable.getWriter();
		} else {
			console.warn('Serial port is not writable. Sending data will not be possible.');
			// Not throwing an error, as some applications might be read-only
		}

		bytes_queue = []; // Final clear of the queue before real operations start
		reset(); // Reset state machine again to be absolutely sure

		status_message.set(`Device ready (Baud: ${BAUD}). Listening for data...`);
		is_connected.set(true);
		keep_reading = true;

		if (reader) {
			read_loop(); // Do not await, let it run in the background
		} else {
			// This case should ideally be caught by 'Serial port is not readable after boot wait.'
			is_connected.set(false);
			throw new Error('Reader not available to start read_loop.');
		}
	} catch (error: any) {
		status_message.set(`Error: ${error.message}`);
		console.error('Serial connection error:', error);
		if (port) {
			// Ensure cleanup if port was partially opened or an error occurred after opening
			await disconnect_serial_port_internal();
		}
		is_connected.set(false); // Ensure disconnected state
		port = null; // Ensure port is nullified
		reader = null;
		writer = null;
	}
}

export async function send_data(data: Uint8Array): Promise<boolean> {
	if (!writer || !_is_connected_internal) {
		status_message.set('Not connected or writer not available.');
		console.warn('Attempted to send data while not connected or writer is unavailable.');
		return false;
	}
	if (!data || data.length === 0) {
		// status_message.set('Data to send is empty.'); // Probably too noisy for UI
		console.warn('Data to send is empty.');
		return false;
	}

	try {
		await writer.write(data);
		// console.log('Sent bytes:', data); // Can be noisy
		return true;
	} catch (error: any) {
		status_message.set(`Error sending data: ${error.message}`);
		console.error('Send error:', error);
		// Consider if a send error should lead to a disconnect
		// await disconnect_serial_port_internal();
		// is_connected.set(false);
		return false;
	}
}

async function disconnect_serial_port_internal() {
	console.log('disconnect_serial_port_internal called');
	keep_reading = false; // Signal read_loop to stop

	if (reader) {
		try {
			await reader.cancel(); // This should make the read() in read_loop resolve with { done: true }
			reader.releaseLock();
			console.log('Reader cancelled and lock released.');
		} catch (e: any) {
			console.warn('Error cancelling or releasing reader:', e.message);
		}
		reader = null;
	}

	if (writer) {
		try {
			if (port && port.writable) {
				// Check if port is still valid and writable
				await writer.close(); // or writer.abort();
				console.log('Writer closed.');
			} else {
				// If port or port.writable is null, writer.close() might throw.
				// We can try to release the lock directly if abort/close isn't appropriate.
				writer.releaseLock();
				console.log('Writer lock released (port not available for close).');
			}
		} catch (e: any) {
			console.warn('Error closing or releasing writer:', e.message);
		}
		writer = null;
	}

	if (port) {
		try {
			// Additional check for readable/writable to ensure they are not locked
			// This is mostly covered by reader/writer cancellation above.
			if (port.readable && port.readable.locked) {
				console.warn('Port readable stream still locked before closing. This might be an issue.');
			}
			if (port.writable && port.writable.locked) {
				console.warn('Port writable stream still locked before closing. This might be an issue.');
			}
			await port.close();
			console.log('Serial port closed.');
		} catch (error: any) {
			console.error('Error closing port:', error);
		}
		port = null;
	}
}

export async function disconnect_serial_port() {
	const was_connected = _is_connected_internal;
	await disconnect_serial_port_internal();
	is_connected.set(false);
	if (was_connected) {
		// Only update status if it was truly connected
		status_message.set("Serial port disconnected. Click 'Connect' to reconnect.");
	} else {
		// If it wasn't fully connected (e.g., error during connect), message might already be set
		// Or set a generic "Ready to connect" message
		status_message.set("Click 'Connect' to select the serial port.");
	}
	bytes_queue = []; // Clear queue on disconnect
	reset(); // Reset state machine
}

// Small improvement to process() to make it non-recursive and avoid stack overflows on large data bursts
// The original `process()` is now called by `process_async()` which handles the iteration.
// The `if (bytes_queue.length > 0) { process(); }` at the end of `process()` is removed.
