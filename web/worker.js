self.onmessage = async ({ data: job }) => {
	try {
		const { mode, speed, scale, raw, buffer } = job;

		if (!["encode", "decode"].includes(mode)) {
			throw new Error("Invalid operation.");
		}
		if (!["slow", "normal", "fast"].includes(speed)) {
			throw new Error("Invalid speed.");
		}
		if (!["chromatic", "pentatonic", "minor"].includes(scale)) {
			throw new Error("Invalid scale.");
		}

		const limit = mode === "encode" ? 1024 : 40 * 1024 * 1024;
		if (!(buffer instanceof ArrayBuffer) || buffer.byteLength > limit) {
			throw new Error("File exceeds the demo size limit.");
		}

		const { default: createModule } = await import(
			mode === "encode" ? "./encode.mjs" : "./decode.mjs"
		);

		const log = (text) => {
			self.postMessage({ type: "log", text: String(text) });
		};

		const module = await createModule({
			print: log,
			printErr: log,
		});

		module.FS.writeFile("/input", new Uint8Array(buffer));

		const result = module.ccall(
			"run_music",
			"number",
			["string", "string", "number"],
			[speed, scale, mode === "decode" && raw ? 1 : 0]
		);

		if (result !== 0) {
			throw new Error("Processing failed. See log for details.");
		}

		const output = module.FS.readFile("/output");
		self.postMessage(
			{ type: "done", data: output },
			[output.buffer]
		);
	} catch (error) {
		self.postMessage({
			type: "error",
			text: error?.message ?? String(error),
		});
	}
};