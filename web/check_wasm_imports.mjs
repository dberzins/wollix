// check_wasm_imports.mjs - the packaged bare-WASM modules import exactly what
// the JS host provides.
//
// Usage: node check_wasm_imports.mjs <host.js> <module.wasm>...
//
// For every module: the `env` imports are exactly abort and puts (puts
// carries WLX_ERROR_PRINT to the browser console; a module that defines
// puts itself would be mute), every `wlx` and `env` import is a member of
// the host's wlxImports / envImports table, and no other import module
// appears. The host tables are read by their layout in wollix_wasm.js: a
// table opens at `    const <name> = {` and closes at the first line that
// is exactly `    };`, and its members start at eight spaces. A table with
// no members found fails the check, so a layout change cannot pass it
// vacuously. Exits non-zero on any mismatch.

import { readFileSync } from "node:fs";

const ENV_EXPECTED = ["abort", "puts"];

function hostTable(lines, name) {
    const open = lines.indexOf(`    const ${name} = {`);
    if (open < 0) throw new Error(`host table ${name} not found`);
    const members = new Set();
    for (let i = open + 1; i < lines.length; i++) {
        if (lines[i] === "    };") return members;
        const m = /^ {8}([A-Za-z_$][\w$]*)\s*[(:]/.exec(lines[i]);
        if (m) members.add(m[1]);
    }
    throw new Error(`host table ${name} has no closing line`);
}

const [hostPath, ...wasmPaths] = process.argv.slice(2);
if (!hostPath || wasmPaths.length === 0) {
    console.error("usage: node check_wasm_imports.mjs <host.js> <module.wasm>...");
    process.exit(2);
}

const hostLines = readFileSync(hostPath, "utf8").split(/\r?\n/);
const host = { wlx: hostTable(hostLines, "wlxImports"), env: hostTable(hostLines, "envImports") };
let failures = 0;
for (const [mod, members] of Object.entries(host)) {
    if (members.size === 0) {
        console.error(`check_wasm_imports: no members found in the host's ${mod} table`);
        failures++;
    }
}

for (const path of wasmPaths) {
    const imports = WebAssembly.Module.imports(new WebAssembly.Module(readFileSync(path)));
    const env = imports.filter((i) => i.module === "env").map((i) => i.name).sort();
    const wlx = imports.filter((i) => i.module === "wlx").map((i) => i.name);
    const name = path.split("/").pop();
    const problems = [];
    if (env.join(",") !== ENV_EXPECTED.join(",")) {
        problems.push(`env imports [${env.join(", ")}], expected [${ENV_EXPECTED.join(", ")}]`);
    }
    for (const i of imports) {
        if (!(i.module in host)) problems.push(`import from unknown module ${i.module}.${i.name}`);
        else if (!host[i.module].has(i.name)) problems.push(`${i.module}.${i.name} missing from the host table`);
    }
    if (problems.length > 0) {
        failures += problems.length;
        for (const p of problems) console.error(`check_wasm_imports: ${name}: ${p}`);
    } else {
        console.log(`check_wasm_imports: ${name}: env [${env.join(", ")}], wlx ${wlx.length} (host ${host.wlx.size})`);
    }
}
process.exit(failures === 0 ? 0 : 1);
