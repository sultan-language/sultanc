[العربية](../../ar/book/ch11-02-running-tests.md) | **English**

# Controlling How Tests Are Run

## Each test runs alone

`sultanc test` runs every test in a process of its own. A test that crashes, loops forever or ends the
program early cannot affect the others, and the order in which tests run does not change their results.

## Running some of the tests

When you work on one part of a program, you may want to run only its tests. `--filter=TEXT` runs only
the tests whose names contain `TEXT`; the report counts the others as *filtered out*:

```console
$ sultanc test calc.sn --filter=zero
```

```text
PASS adding_zero_changes_nothing

test result: ok. passed: 1; failed: 0; filtered out: 1
```

`--list` prints the names of the tests without running them:

```console
$ sultanc test --list calc.sn
adding_works
adding_zero_changes_nothing
```

## Tests in a package

In a package, `supax test` runs all the package's tests: those of the library, of every program, and of
every file in the `tests` folder. It accepts the same choices:

| Command | Runs |
| --- | --- |
| `supax test` | every test of the package |
| `supax test TEXT` | the tests whose names contain `TEXT` |
| `supax test --lib` | only the library's tests |
| `supax test --bin NAME` | only the tests of the program `NAME` |
| `supax test --test NAME` | only the tests in `tests/NAME.sn` |
| `supax test --list` | lists the tests without running them |
| `supax test --no-fail-fast` | goes on after a file whose tests failed |

The next section shows a package with tests in all these places.
