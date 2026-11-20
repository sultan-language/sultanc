[العربية](../../ar/book/ch11-03-test-organization.md) | **English**

# Test Organization

Tests come in two kinds, and a package keeps them in two places.

## Unit tests, beside the code

A *unit test* checks one module on its own, including its private functions. It lives in the module's
own file, next to the code it tests. Here is the `shop.prices.tax` module of chapter 7, with a test of
its private `rate_percent`:

<sub>Filename: src/prices/tax.sn</sub>

```sultan ignore
import std.debug.assert;

public def with_tax(amount: int): int {
    return amount + amount * rate_percent() / 100;
}

def rate_percent(): int {
    return 15;
}

@@test
def the_rate_is_fifteen_percent(): void {
    assert(rate_percent() == 15);
}
```

`supax test --lib` runs the library's tests. It starts from the library's *root file*, `src/NAME.sn`,
and runs the tests of every module the root file imports:

<sub>Filename: src/shop.sn</sub>

```sultan ignore
import shop.prices.tax;
```

## Integration tests, in `tests`

An *integration test* uses the library the way another program would: through its public names only. It
lives in the folder `tests`, at the top of the package, and imports the library by name:

<sub>Filename: tests/prices.sn</sub>

```sultan ignore
import std.debug.assert;
import shop.prices.tax;

@@test
def tax_is_added(): void {
    assert(with_tax(200) == 230);
}

@@test
def zero_stays_zero(): void {
    assert(with_tax(0) == 0);
}
```

Integration tests catch what unit tests miss: parts that work alone and fail together, and public
interfaces that are awkward to use.

## Running them all

```text
shop/
├── Supax.toml
├── src/
│   ├── shop.sn
│   ├── main.sn
│   ├── bin/
│   │   └── report.sn
│   └── prices/
│       └── tax.sn
└── tests/
    └── prices.sn
```

```console
$ supax test
     Running lib src/shop.sn
PASS the_rate_is_fifteen_percent

test result: ok. passed: 1; failed: 0; filtered out: 0
     Running bin src/main.sn

test result: ok. passed: 0; failed: 0; filtered out: 1
     Running bin src/bin/report.sn

test result: ok. passed: 0; failed: 0; filtered out: 1
     Running test tests/prices.sn
PASS tax_is_added
PASS zero_stays_zero

test result: ok. passed: 2; failed: 0; filtered out: 0
    Finished in 0.80s
```

Each target runs its own tests. The programs `main.sn` and `report.sn` import `shop.prices.tax` too,
but its test belongs to the library, so their runs count it as filtered out instead of running it a
second time.

## Summary

- A test is a `@@test` function with no parameters returning `void`; `assert` checks inside it.
- `sultanc test file.sn` runs a file's tests; an ordinary build leaves them out.
- A failure reports the assertion's line; any failure makes the status 1. Each test runs alone.
- `--filter` runs some tests, `--list` lists them; `supax test` runs a package's tests.
- Unit tests live beside the code; integration tests live in `tests` and use public names only.
