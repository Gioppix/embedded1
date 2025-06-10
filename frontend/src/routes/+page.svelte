<script lang="ts">
	// Svelte 5: $state for reactive variables
	let port: SerialPort | null = $state(null);
	let reader: ReadableStreamDefaultReader<string> | null = $state(null);
	let writer = $state(null);
	let readableStreamClosed = $state(null); // Promise to await reader release
	let writableStreamClosed = $state(null); // Promise to await writer release

	let keepReading = $state(false);

	let isConnected = $state(false);
	let statusMessage = $state("Click 'Connect' to select the HC-06 serial port.");
	let messageToSend = $state('');
	let receivedMessages = $state([]); // An array to store logs of sent/received messages

	// Common baud rate for HC-06 is 9600. Adjust if yours is different.
	const BAUD_RATE = 9600;

	async function connectToSerialPort() {
		if (!('serial' in navigator)) {
			statusMessage = 'Web Serial API is not available in this browser.';
			console.error('Web Serial API not available.');
			alert('Web Serial API is not supported by your browser. Please use Chrome or Edge.');
			return;
		}

		try {
			statusMessage = 'Requesting serial port selection...';
			// Request a port and filter by USB vendor/product ID if known,
			// but for Bluetooth serial, it's usually just a generic selection.
			// The browser will show a list of available serial ports.
			const requestedPort = await navigator.serial.requestPort();
			port = requestedPort;

			statusMessage = `Opening port ${port.getInfo().usbProductId ? port.getInfo().usbVendorId + ':' + port.getInfo().usbProductId : ' (Bluetooth SPP)'}...`;
			console.log('Port selected:', port.getInfo());

			await port.open({
				baudRate: BAUD_RATE,
				dataBits: 8,
				parity: 'none',
				stopBits: 1
			});

			statusMessage = `Serial port opened successfully (Baud: ${BAUD_RATE}).`;
			isConnected = true;
			keepReading = true;

			// Setup reader
			const textDecoder = new TextDecoderStream();
			readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
			reader = textDecoder.readable.getReader();

			// Setup writer
			const textEncoder = new TextEncoderStream();
			writableStreamClosed = textEncoder.readable.pipeTo(port.writable);
			writer = textEncoder.writable.getWriter();

			readLoop(); // Start reading from the port

			receivedMessages = [
				...receivedMessages,
				{
					type: 'system',
					text: `Connected to port. Baud: ${BAUD_RATE}`,
					time: new Date().toLocaleTimeString()
				}
			];
		} catch (error) {
			statusMessage = `Error: ${error.message}`;
			console.error('Serial connection error:', error);
			if (port && port.readable) {
				// Basic cleanup if port was partially opened
				await disconnectSerialPort();
			}
		}
	}

	async function readLoop() {
		try {
			while (port && port.readable && keepReading) {
				const { value, done } = await reader.read();
				if (done) {
					// reader.releaseLock() happens automatically when the stream is cancelled or closes.
					console.log('Reader stream done.');
					break;
				}
				if (value) {
					console.log('Received:', value);
					receivedMessages = [
						...receivedMessages,
						{ type: 'received', text: value, time: new Date().toLocaleTimeString() }
					];
				}
			}
		} catch (error) {
			// This can happen if the port is closed while reading
			if (keepReading) {
				// Don't show error if we intentionally stopped reading
				statusMessage = `Read loop error: ${error.message}`;
				console.error('Read loop error:', error);
			}
			// Consider a gentle disconnect or UI update here
		} finally {
			// Ensure the reader is released if not already done by stream closure.
			// However, with pipeTo, this is usually handled.
			// if (reader) {
			//   reader.releaseLock();
			// }
			console.log('Exited read loop.');
		}
	}

	async function sendMessage() {
		if (!writer || !isConnected) {
			statusMessage = 'Not connected or writer not available.';
			return;
		}
		if (!messageToSend) {
			statusMessage = 'Message is empty.';
			return;
		}

		try {
			// Append newline if your device expects it (common for serial)
			// const messageWithNewline = messageToSend + '\r\n';
			const messageWithNewline = messageToSend; // Or send as is

			await writer.write(messageWithNewline);
			console.log('Sent:', messageWithNewline);
			receivedMessages = [
				...receivedMessages,
				{ type: 'sent', text: messageToSend, time: new Date().toLocaleTimeString() }
			];
			messageToSend = ''; // Clear input after sending
		} catch (error) {
			statusMessage = `Error sending message: ${error.message}`;
			console.error('Send error:', error);
		}
	}

	async function disconnectSerialPort() {
		keepReading = false; // Signal the read loop to stop

		if (writer) {
			try {
				await writer.close(); // Close the writer stream
				console.log('Writer closed');
			} catch (e) {
				console.error('Error closing writer:', e);
			}
		}

		if (reader) {
			try {
				// Cancel the reader to stop it and release the lock.
				// This will also cause the `readableStreamClosed` promise to resolve or reject.
				await reader.cancel();
				console.log('Reader cancelled');
			} catch (e) {
				console.error('Error cancelling reader:', e);
			}
		}

		// Wait for the streams to be fully closed
		// if (readableStreamClosed) {
		//   try { await readableStreamClosed.catch(() => {}); } catch (e) { console.warn("Error during readable stream close", e); }
		// }
		// if (writableStreamClosed) {
		//   try { await writableStreamClosed.catch(() => {}); } catch (e) { console.warn("Error during writable stream close", e); }
		// }

		if (port) {
			try {
				await port.close();
				console.log('Serial port closed.');
			} catch (error) {
				statusMessage = `Error closing port: ${error.message}`;
				console.error('Error closing port:', error);
			}
		}

		port = null;
		reader = null;
		writer = null;
		isConnected = false;
		statusMessage = "Serial port disconnected. Click 'Connect' to reconnect.";
		receivedMessages = [
			...receivedMessages,
			{ type: 'system', text: 'Disconnected.', time: new Date().toLocaleTimeString() }
		];
	}

	// Svelte's onDestroy for cleanup when the component is unmounted
	// import { onDestroy } from 'svelte'; // Not strictly needed with Svelte 5 for DOM events, but good for async ops
	// onDestroy(async () => {
	//   if (isConnected) {
	//     await disconnectSerialPort();
	//   }
	// });
</script>

<div class="container mx-auto p-4 font-sans sm:p-6 lg:p-8">
	<header class="mb-6">
		<h1 class="text-3xl font-bold text-indigo-700">HC-06 Web Serial Communicator</h1>
		<p class="text-sm text-gray-600">Using Svelte 5, Tailwind CSS, and Web Serial API</p>
	</header>

	<section class="mb-6 rounded-lg bg-gray-100 p-4 shadow">
		<p class="text-sm text-gray-800">
			Status:
			<span
				class="font-semibold"
				class:text-green-600={isConnected}
				class:text-red-600={!isConnected && statusMessage.toLowerCase().includes('error')}
				class:text-yellow-600={!isConnected &&
					!statusMessage.toLowerCase().includes('error') &&
					statusMessage !== "Click 'Connect' to select the HC-06 serial port." &&
					statusMessage !== "Serial port disconnected. Click 'Connect' to reconnect."}
			>
				{statusMessage}
			</span>
		</p>
	</section>

	<section class="mb-6">
		{#if !isConnected}
			<button
				onclick={connectToSerialPort}
				class="focus:ring-opacity-50 w-full transform rounded-lg bg-indigo-600 px-6 py-3 font-bold text-white shadow-md transition-transform hover:scale-105 hover:bg-indigo-700 focus:ring-2 focus:ring-indigo-500 focus:outline-none sm:w-auto"
			>
				Connect to HC-06 (Serial)
			</button>
		{:else}
			<button
				onclick={disconnectSerialPort}
				class="focus:ring-opacity-50 w-full transform rounded-lg bg-red-600 px-6 py-3 font-bold text-white shadow-md transition-transform hover:scale-105 hover:bg-red-700 focus:ring-2 focus:ring-red-500 focus:outline-none sm:w-auto"
			>
				Disconnect
			</button>
		{/if}
	</section>

	{#if isConnected}
		<section class="mb-6 rounded-lg bg-white p-4 shadow-lg">
			<h2 class="mb-3 text-xl font-semibold text-gray-700">Send Data</h2>
			<div class="flex flex-col gap-3 sm:flex-row">
				<input
					type="text"
					bind:value={messageToSend}
					placeholder="Type message..."
					class="flex-grow rounded-lg border border-gray-300 p-3 focus:border-transparent focus:ring-2 focus:ring-indigo-500"
					onkeypress={(e) => e.key === 'Enter' && sendMessage()}
				/>
				<button
					onclick={sendMessage}
					disabled={!messageToSend || !writer}
					class="focus:ring-opacity-50 rounded-lg bg-green-500 px-6 py-3 font-bold text-white shadow-md transition-colors hover:bg-green-600 focus:ring-2 focus:ring-green-500 focus:outline-none disabled:cursor-not-allowed disabled:bg-gray-400"
				>
					Send
				</button>
			</div>
		</section>

		<section class="rounded-lg bg-white p-4 shadow-lg">
			<h2 class="mb-3 text-xl font-semibold text-gray-700">Communication Log</h2>
			{#if receivedMessages.length === 0}
				<p class="text-gray-500 italic">No messages yet. Waiting for data...</p>
			{:else}
				<div
					class="max-h-80 space-y-2 overflow-y-auto rounded-md border border-gray-200 bg-gray-50 p-3"
				>
					{#each receivedMessages as msg (msg.time + msg.text + Math.random())}
						<!-- Added Math.random for better key uniqueness if messages are identical -->
						<div
							class={`rounded-md p-2 text-sm ${msg.type === 'sent' ? 'self-end bg-blue-100 text-right text-blue-800' : msg.type === 'received' ? 'self-start bg-green-100 text-green-800' : 'self-center bg-gray-200 text-center text-gray-700'}`}
						>
							<span class="block font-mono text-xs text-gray-500">{msg.time}</span>
							<p class="break-words whitespace-pre-wrap">{msg.text}</p>
						</div>
					{/each}
				</div>
			{/if}
		</section>
	{/if}

	<footer class="mt-8 rounded-lg border border-yellow-200 bg-yellow-50 p-4 text-sm text-yellow-700">
		<h3 class="mb-2 font-bold text-yellow-800">Important Notes for Web Serial:</h3>
		<ul class="list-inside list-disc space-y-1">
			<li>
				Ensure your HC-06 is <strong class="text-yellow-800">paired with your macOS system</strong> via
				Bluetooth settings.
			</li>
			<li>
				macOS should create a serial port for the HC-06 (e.g., <code>/dev/tty.HC-06-SPP</code>).
			</li>
			<li>
				This page must be served over <strong class="text-yellow-800">HTTPS</strong> or accessed via
				<strong class="text-yellow-800">localhost</strong>.
			</li>
			<li>Your browser must support the Web Serial API (e.g., Chrome, Edge).</li>
			<li>
				When you click "Connect", your browser will prompt you to select the serial port
				corresponding to your HC-06.
			</li>
			<li>
				The default baud rate is set to 9600. If your HC-06 uses a different rate, you'll need to
				modify the `BAUD_RATE` constant in the script.
			</li>
		</ul>
	</footer>
</div>
