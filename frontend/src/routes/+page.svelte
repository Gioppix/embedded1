<script lang="ts">
	import { onDestroy } from 'svelte';
	import {
		is_connected,
		status_message,
		bytes_per_second,
		connect_to_serial_port,
		disconnect_serial_port,
		send_data,
		ready_frame
	} from '$lib/serial'; // Adjust path if you place it elsewhere
	import { BAUD, SCREENX } from '$lib/generated';

	let message_to_send = $state('');
	const text_encoder = new TextEncoder(); // For converting string input to Uint8Array

	// Example: Log incoming data chunks to console or update a display
	// let communication_log_content = $state('');
	// $incoming_byte_chunk

	async function handle_send_message() {
		if (!message_to_send) {
			// The service's send_data function will set a status message for empty data.
			// You could also set a local, temporary message here if preferred.
		}
		const data_bytes = text_encoder.encode(message_to_send);
		const success = await send_data(data_bytes);
		if (success) {
			message_to_send = ''; // Clear input after successful send
		}
	}

	// Ensure disconnection when the component is destroyed (e.g., page navigation)
	onDestroy(async () => {
		if ($is_connected) {
			await disconnect_serial_port();
		}
	});
</script>

<div class="container mx-auto p-4 font-sans sm:p-6 lg:p-8">
	<header class="mb-6">
		<h1 class="text-3xl font-bold text-indigo-700">HC-06 Web Serial Communicator</h1>
		<p class="text-sm text-gray-600">
			Using Svelte 5, Tailwind CSS, and Web Serial API (Refactored)
		</p>
	</header>

	<section class="mb-6 rounded-lg bg-gray-100 p-4 shadow">
		<p class="text-sm text-gray-800">
			Status:
			<span
				class="font-semibold"
				class:text-green-600={$is_connected}
				class:text-red-600={!$is_connected && $status_message.toLowerCase().includes('error')}
				class:text-yellow-600={!$is_connected &&
					!$status_message.toLowerCase().includes('error') &&
					$status_message !== "Click 'Connect' to select the serial port." &&
					$status_message !== "Serial port disconnected. Click 'Connect' to reconnect."}
			>
				{$status_message}
			</span>
		</p>
	</section>

	<section class="mb-6">
		{#if !$is_connected}
			<button
				onclick={connect_to_serial_port}
				class="focus:ring-opacity-50 w-full transform rounded-lg bg-indigo-600 px-6 py-3 font-bold text-white shadow-md transition-transform hover:scale-105 hover:bg-indigo-700 focus:ring-2 focus:ring-indigo-500 focus:outline-none sm:w-auto"
			>
				Connect to HC-06 (Serial)
			</button>
		{:else}
			<button
				onclick={disconnect_serial_port}
				class="focus:ring-opacity-50 w-full transform rounded-lg bg-red-600 px-6 py-3 font-bold text-white shadow-md transition-transform hover:scale-105 hover:bg-red-700 focus:ring-2 focus:ring-red-500 focus:outline-none sm:w-auto"
			>
				Disconnect Speed: {$bytes_per_second}
			</button>
		{/if}
	</section>

	{#if $is_connected}
		<section class="mb-6 rounded-lg bg-white p-4 shadow-lg">
			<h2 class="mb-3 text-xl font-semibold text-gray-700">Send Data</h2>
			<div class="flex flex-col gap-3 sm:flex-row">
				<input
					type="text"
					bind:value={message_to_send}
					placeholder="Type message to send as bytes..."
					class="flex-grow rounded-lg border border-gray-300 p-3 focus:border-transparent focus:ring-2 focus:ring-indigo-500"
					onkeypress={(e) => e.key === 'Enter' && handle_send_message()}
					disabled={!$is_connected}
				/>
				<button
					onclick={handle_send_message}
					disabled={!message_to_send || !$is_connected}
					class="focus:ring-opacity-50 rounded-lg bg-green-500 px-6 py-3 font-bold text-white shadow-md transition-colors hover:bg-green-600 focus:ring-2 focus:ring-green-500 focus:outline-none disabled:cursor-not-allowed disabled:bg-gray-400"
				>
					Send
				</button>
			</div>
		</section>
	{/if}

	<div class="flex w-full justify-center">
		<div class="flex h-96 w-96 flex-col bg-red-300">
			{#each Array.from( { length: Math.ceil($ready_frame.length / SCREENX) }, (_, i) => $ready_frame.slice(i * SCREENX, (i + 1) * SCREENX) ) as row}
				<div class="flex w-full grow">
					{#each row as cell}
						{@const color =
							cell === 0 ? '#FAFAFA' : cell === 1 ? 'gray' : cell === 2 ? 'black' : 'red'}
						<div class="grow" style="background-color: {color};"></div>
					{/each}
				</div>
			{/each}
		</div>
	</div>

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
				The default baud rate is set to {BAUD}. If your HC-06 uses a different rate, you'll need to
				modify the `BAUD_RATE` constant in the `serial_service.ts` file.
			</li>
		</ul>
	</footer>
</div>
