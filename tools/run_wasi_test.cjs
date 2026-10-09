// Execute the same host C++ tests when Windows blocks generated PE binaries.
const fs = require('node:fs');
const path = require('node:path');
const { WASI } = require('node:wasi');
(async () => {
  const wasi = new WASI({version: 'preview1', args: [process.argv[2]],
    preopens: {'.': process.cwd()}, returnOnExit: true});
  const module = await WebAssembly.compile(fs.readFileSync(path.resolve(process.argv[2])));
  const instance = await WebAssembly.instantiate(module, {wasi_snapshot_preview1: wasi.wasiImport});
  process.exitCode = wasi.start(instance);
})().catch(error => { console.error(error); process.exitCode = 1; });
