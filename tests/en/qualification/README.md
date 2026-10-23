# SultanC compiler qualification

The qualification harness is implemented in SultanC and lives under `tests/runner/`.
Python, shell test scripts, and TSV manifests are not used.

From the repository root, build the runner with SultanC, then execute it:

```sh
./build/sultanc tests/runner/main.sn -o build/sultan-tests
./build/sultan-tests
```

Test cases are discovered recursively from `tests/ar/**` and `tests/en/**`.
Self-describing test directives are stored directly in `.sn` fixtures, including mode,
targets, expected exit status, diagnostic ID, Stage0 parity, differential execution, and
Arabic/English surface-pair information.

The runner also owns compiler architecture checks, CLI/diagnostic checks, malformed-source
robustness, ELF/Mach-O structural checks, determinism, Darwin code-sign verification,
network/TLS qualification, RSS regression checks, and optional self-host convergence.

Optional expensive qualification switches:

- `SULTANC_TEST_RSS=1` — measure check/object/executable peak RSS.
- `SULTANC_TEST_ALLOCATIONS=1` — capture macOS high-water allocation reports.
- `SULTANC_TEST_CONVERGENCE=1` — run Stage2 → Stage3 → Stage4 byte convergence.

Output distinguishes passed, failed, skipped, and foreign-target compile-only work.
