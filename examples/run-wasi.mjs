// Runs a SultanC program built for wasm32-wasi under Node's WASI support.
//   ./build/sultanc examples/en/basics/hello.sn --target=wasm32-wasi -o build/hello.wasm
//   node --no-warnings examples/run-wasi.mjs build/hello.wasm
import { readFile } from "node:fs/promises";
import { WASI } from "node:wasi";

const [file, ...args] = process.argv.slice(2);
if (!file) {
  console.error("usage: node run-wasi.mjs <program.wasm> [arguments...]");
  process.exit(2);
}
const wasi = new WASI({ version: "preview1", args: [file, ...args], env: process.env, preopens: { ".": "." } });
const { instance } = await WebAssembly.instantiate(await readFile(file), wasi.getImportObject());
process.exitCode = wasi.start(instance);
