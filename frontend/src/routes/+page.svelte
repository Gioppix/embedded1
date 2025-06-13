<script lang="ts">
	import { onDestroy } from 'svelte';
	import {
		is_connected,
		status_message,
		bytes_per_second,
		connect_to_serial_port,
		disconnect_serial_port,
		ready_frame,
		fps,
		score,
		bullets
	} from '$lib/serial';
	import { SCREENX } from '$lib/generated';

	onDestroy(async () => {
		if ($is_connected) {
			await disconnect_serial_port();
		}
	});
</script>

<div class="min-h-[100dvh] bg-gray-100">
	<div class="mx-auto max-w-[60rem] p-6 font-mono text-gray-800 antialiased">
		<header class="mb-8 text-center">
			<h1 class="text-5xl font-bold tracking-wide text-indigo-700">Parachute 0.5</h1>
		</header>

		<section class="mb-8 rounded-md border-2 border-gray-400 bg-gray-200 p-6 shadow">
			<div class="grid grid-cols-3 items-center gap-4">
				<div class="col-span-1 text-left">
					<p class="mb-1 text-xs text-gray-600 uppercase">Status</p>
					<p
						class="truncate text-lg font-semibold"
						class:text-green-600={$is_connected}
						class:text-red-600={!$is_connected && $status_message.toLowerCase().includes('error')}
						class:text-orange-500={!$is_connected &&
							!$status_message.toLowerCase().includes('error') &&
							$status_message !== "Click 'Connect' to select the serial port." &&
							$status_message !== "Serial port disconnected. Click 'Connect' to reconnect."}
						class:text-gray-700={!$is_connected &&
							($status_message === "Click 'Connect' to select the serial port." ||
								$status_message === "Serial port disconnected. Click 'Connect' to reconnect.")}
						title={$status_message}
					>
						{$status_message}
					</p>
				</div>

				<div class="col-span-1 text-center">
					{#if !$is_connected}
						<button
							onclick={connect_to_serial_port}
							class="focus:ring-opacity-75 w-auto transform rounded border-b-4 border-green-700 bg-green-500 px-8 py-3 text-lg font-bold text-white transition-transform hover:scale-105 hover:border-green-600 hover:bg-green-600 focus:ring-2 focus:ring-green-400 focus:outline-none"
						>
							CONNECT
						</button>
					{:else}
						<button
							onclick={disconnect_serial_port}
							class="focus:ring-opacity-75 w-auto transform rounded border-b-4 border-red-700 bg-red-500 px-8 py-3 text-lg font-bold text-white transition-transform hover:scale-105 hover:border-red-600 hover:bg-red-600 focus:ring-2 focus:ring-red-400 focus:outline-none"
						>
							DISCONNECT
						</button>
					{/if}
				</div>

				<!-- FPS and Speed -->
				<div class="col-span-1 text-right">
					<div class="mb-0">
						<span class="text-xs text-gray-600 uppercase">FPS: </span>
						<span class="text-xl font-bold text-gray-700">{$fps}</span>
					</div>
					<div>
						<span class="text-xs text-gray-600 uppercase">SPEED: </span>
						<span class="text-xl font-bold text-gray-700">
							{$is_connected ? $bytes_per_second + ' B/s' : '--'}
						</span>
					</div>
				</div>
			</div>
		</section>

		<section class="mb-8 grid grid-cols-2 gap-6">
			<div
				class="rounded-lg border-4 border-black bg-gray-800 p-4 text-center text-yellow-400 shadow-lg"
			>
				<p class="mb-1 text-xl font-bold tracking-widest uppercase">SCORE</p>
				<p class="text-5xl font-black">{$score}</p>
			</div>
			<div
				class="rounded-lg border-4 border-black bg-gray-800 p-4 text-center text-yellow-400 shadow-lg"
			>
				<p class="mb-1 text-xl font-bold tracking-widest uppercase">BULLETS</p>
				<p class="text-5xl font-black">{$bullets}</p>
			</div>
		</section>

		<div class="mb-8 flex w-full justify-center">
			<div class="flex h-96 w-96 flex-col border-4 border-gray-600 bg-gray-300 shadow-inner">
				{#each Array.from( { length: Math.ceil($ready_frame.length / SCREENX) }, (_, i) => $ready_frame.slice(i * SCREENX, (i + 1) * SCREENX) ) as row}
					<div class="flex w-full grow">
						{#each row as cell}
							{@const color =
								cell === 0 ? '#FAFAFA' : cell === 1 ? '#BABABA' : cell === 2 ? 'black' : 'red'}
							<div class="grow" style="background-color: {color};"></div>
						{/each}
					</div>
				{/each}
			</div>
		</div>
	</div>
</div>
