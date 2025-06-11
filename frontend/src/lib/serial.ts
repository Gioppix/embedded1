import { writable } from 'svelte/store';

// Configuration
export const BAUD_RATE = 1_000_000;

// Reactive stores for the Svelte component to subscribe to
export const is_connected = writable<boolean>(false);
export const status_message = writable<string>("Click 'Connect' to select the serial port.");
export const bytes_per_second = writable<number>(0);
export const incoming_byte_chunk = writable<Uint8Array | null>(null); // Store for raw incoming data chunks

let port: SerialPort | null = null;
let reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
let writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
let keep_reading = false;
let _is_connected_internal = false; // Internal flag to avoid direct store reads in service logic

// Keep internal flag synced with the store
is_connected.subscribe((value) => (_is_connected_internal = value));

// Speed calculation
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
				break;
			}
			if (value && value.length > 0) {
				current_byte_count += value.length;
				incoming_byte_chunk.set(value); // Notify subscribers about the new data chunk
			}
		}
	} catch (error: any) {
		if (keep_reading) {
			// Avoid error message if intentionally stopped
			status_message.set(`Read loop error: ${error.message}`);
			console.error('Read loop error:', error);
		}
	} finally {
		console.log('Exited read loop.');
	}
}

export async function connect_to_serial_port() {
	if (!('serial' in navigator)) {
		status_message.set('Web Serial API is not available in this browser.');
		console.error('Web Serial API not available.');
		alert('Web Serial API is not supported by your browser. Please use Chrome or Edge.');
		return;
	}

	try {
		status_message.set('Requesting serial port selection...');
		const requested_port = await navigator.serial.requestPort();
		port = requested_port;

		const port_info = port.getInfo();
		const port_identifier = port_info.usbProductId
			? `${port_info.usbVendorId}:${port_info.usbProductId}`
			: '(Bluetooth SPP)'; // Common for HC-06
		status_message.set(`Opening port ${port_identifier}...`);
		console.log('Port selected:', port_info);

		await port.open({
			baudRate: BAUD_RATE,
			dataBits: 8,
			parity: 'none',
			stopBits: 1
		});

		status_message.set(`Serial port opened successfully (Baud: ${BAUD_RATE}).`);
		is_connected.set(true);
		keep_reading = true;

		if (port.readable) {
			reader = port.readable.getReader();
		}
		if (port.writable) {
			writer = port.writable.getWriter();
		}

		if (reader) {
			read_loop(); // Do not await, let it run in the background
		} else {
			is_connected.set(false); // Rollback
			throw new Error('Serial port is not readable.');
		}
		if (!writer) {
			console.warn('Serial port is not writable. Sending data will not be possible.');
			// Potentially update status_message here if critical
		}
	} catch (error: any) {
		status_message.set(`Error: ${error.message}`);
		console.error('Serial connection error:', error);
		if (port) {
			await disconnect_serial_port_internal(); // Attempt cleanup
		} else {
			is_connected.set(false); // Ensure disconnected state if port was never acquired
		}
	}
}

export async function send_data(data: Uint8Array): Promise<boolean> {
	if (!writer || !_is_connected_internal) {
		status_message.set('Not connected or writer not available.');
		console.warn('Attempted to send data while not connected or writer is unavailable.');
		return false;
	}
	if (!data || data.length === 0) {
		status_message.set('Data to send is empty.');
		return false;
	}

	try {
		await writer.write(data);
		console.log('Sent bytes:', data);
		return true;
	} catch (error: any) {
		status_message.set(`Error sending data: ${error.message}`);
		console.error('Send error:', error);
		return false;
	}
}

// Internal disconnect function to be used by connect_to_serial_port on error
async function disconnect_serial_port_internal() {
	keep_reading = false;

	if (reader) {
		try {
			await reader.cancel();
			console.log('Reader cancelled');
		} catch (e: any) {
			console.warn('Error cancelling reader:', e.message);
		}
		reader = null;
	}

	if (writer) {
		try {
			// Check if port and writable stream still exist before trying to close/abort writer
			if (port && port.writable) {
				await writer.close(); // or writer.abort(); for immediate termination
				console.log('Writer closed');
			}
		} catch (e: any) {
			console.warn('Error closing writer:', e.message);
		}
		writer = null;
	}

	if (port) {
		try {
			await port.close();
			console.log('Serial port closed.');
		} catch (error: any) {
			console.error('Error closing port:', error);
			// Avoid overwriting a more specific error message with "Error closing port"
		}
		port = null;
	}
	// Note: is_connected and status_message are set by the calling function (connect or disconnect)
}

export async function disconnect_serial_port() {
	await disconnect_serial_port_internal(); // Call the internal version
	is_connected.set(false);
	status_message.set("Serial port disconnected. Click 'Connect' to reconnect.");
}
