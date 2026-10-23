# SultanC RSS qualification

RSS and allocation profiling are implemented by the SultanC test runner in
`tests/runner/suites/system.sn`.

Enable coarse peak-RSS measurement:

```sh
SULTANC_TEST_RSS=1 ./build/sultan-tests
```

This measures the same compiler source through:

1. `check` — frontend + MSIR verification
2. object emission — native backend + object output
3. executable emission — executable finalization

The deterministic regression-gate samples previously stored as JSON fixtures are now
encoded directly in the SultanC suite so no external data manifest is required.

On macOS, exact high-water allocation capture can be enabled with:

```sh
SULTANC_TEST_ALLOCATIONS=1 ./build/sultan-tests
```

The runner uses macOS `malloc_history` and `vmmap` as system inspection tools, while all
test orchestration, thresholds, process control, parsing, validation, and reporting remain
SultanC code.
