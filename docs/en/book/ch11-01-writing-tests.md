[العربية](../../ar/book/ch11-01-writing-tests.md) | **English**

# How to Write Tests

## The anatomy of a test

A test is a function marked with `@@test`. It takes no parameters and returns `void`. It checks what it
expects with `assert`: a test *passes* if it runs to its end, and *fails* if an assertion stops it.

<sub>Filename: calc.sn</sub>

```sultan test
import std.debug.assert;

def add(a: int, b: int): int {
    return a + b;
}

@@test
def adding_works(): void {
    assert(add(2, 3) == 5);
    assert(add(-1, 1) == 0);
}

@@test
def adding_zero_changes_nothing(): void {
    assert(add(7, 0) == 7);
}

def main(): int {
    return 0;
}
```

`sultanc test` compiles the tests of a file and runs them:

```console
$ sultanc test calc.sn
```

```text
PASS adding_works
PASS adding_zero_changes_nothing

test result: ok. passed: 2; failed: 0; filtered out: 0
```

The tests live in the same file as the code they check. An ordinary build, `sultanc calc.sn`, leaves
them out entirely: they cost nothing in the finished program.

`@@test` is an *annotation*: a word after `@@` that tells the compiler something about the declaration
that follows. Its Arabic spelling is `@@اختبار`.

## When a test fails

Here is a test with a wrong expectation:

```sultan test
import std.debug.assert;

def add(a: int, b: int): int {
    return a + b;
}

@@test
def adding_works(): void {
    assert(add(2, 3) == 5);
}

@@test
def a_wrong_expectation(): void {
    assert(add(2, 2) == 5);
}

def main(): int {
    return 0;
}
```

```text
PASS adding_works
FAIL a_wrong_expectation: exit status 1
    --> main.sn:14:5
    note: the test is declared at main.sn:13:5
    error[SULTANC-RUNTIME-PROGRAM-STOPPED]: the program stopped — An assertion failed.

test result: FAILED. passed: 1; failed: 1; filtered out: 0
```

The report names the failing test, the line of the assertion that stopped it, and where the test is
declared. When any test fails, `sultanc test` exits with status 1, so a script or a build server can
stop on it.

Write a test so that its failure tells you something. One test per behaviour, with a name that says
the behaviour, is easier to act on than one large test that checks everything.

## Testing that failure happens

Test the failure cases too; they are where bugs hide. A `match` with `assert(false)` in the branch that
must not happen checks that a function fails when it should:

```sultan test
import std.debug.assert;

def divide(a: int, b: int): result int, string {
    if (b == 0) {
        return result.err("division by zero");
    };
    return result.ok(a / b);
}

@@test
def dividing_works(): void {
    match divide(10, 2) with
    | result.ok(q) -> { assert(q == 5); }
    | result.err(_) -> { assert(false); };
}

@@test
def dividing_by_zero_fails(): void {
    match divide(1, 0) with
    | result.ok(_) -> { assert(false); }
    | result.err(_) -> { };
}

def main(): int {
    return 0;
}
```

```text
PASS dividing_works
PASS dividing_by_zero_fails

test result: ok. passed: 2; failed: 0; filtered out: 0
```

## Many cases in one test

When a function should give known answers for many inputs, put the cases in a vector of records and
check them in a loop. Adding a case is then one line:

```sultan test
import std.debug.assert;

def is_leap_year(year: int): bool {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

type Case = { year: int, leap: bool };

@@test
def leap_years(): void {
    let cases: vector Case = [
        { year: 2024, leap: true },
        { year: 2023, leap: false },
        { year: 1900, leap: false },
        { year: 2000, leap: true }
    ];
    for (let i: mutable int = 0; i < length(cases); i := i + 1) {
        assert(is_leap_year(cases[i].year) == cases[i].leap);
    }
}

def main(): int {
    return 0;
}
```

```text
PASS leap_years

test result: ok. passed: 1; failed: 0; filtered out: 0
```
