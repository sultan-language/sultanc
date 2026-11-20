[العربية](../../ar/book/ch09-03-to-panic-or-not.md) | **English**

# To `panic` or Not to `panic`

When should code return a `result`, and when should it stop the program? The question to ask is: *whose
mistake is this, and who can do something about it?*

## Return a `result` when the caller can act

Bad input, a missing file, a refused connection, a full disk: none of these is a bug, and the right
response depends on the program. An interactive program asks again; a server answers with an error page;
a script stops with a clear message. The function where the failure is noticed cannot know which, so it
returns a `result` and lets its caller decide.

This is especially true for a library. A library that stops the program takes the decision away from
every program that uses it. Libraries should almost always return a `result`.

## Return an `option` when absence is normal

A search that finds nothing, a key not in a map, the first element of an empty vector: nothing has gone
wrong, there is simply no value. `option` says that, without inventing an error.

## Stop when the program itself is wrong

A function documents that its parameter is never negative, and receives a negative one. A value that
was checked a moment ago has changed. Two parts of the program disagree about what a record holds. These
are bugs: the code is wrong, and no caller can fix it at run time. Stop, with `assert` or `panic`,
and say which assumption failed. Continuing would act on a state the program was never written for.

## Examples, prototypes and tests

In a quick prototype, or an example that should stay short, stopping on every error is reasonable: a
`match` that panics on `err` marks clearly where real handling will go later. In tests, a failed
assertion is exactly how a test reports failure, as [chapter 11](ch11-00-testing.md) shows.

## Use types to make errors impossible

The best error is one that cannot happen. If a function needs a number that is never negative, take a
`uint`, and the question disappears. If a value may be absent, an `option` in the type makes every
caller handle that. If a record must keep two fields in agreement, as `Temperature` in chapter 5 did, a
function that builds it is the one place where the rule lives. Each of these moves a check from run time
to compile time, where the compiler does it for you on every path.

## Summary

| Situation | Use |
| --- | --- |
| The caller can do something about it: bad input, a missing file, a refused connection | return a `result` |
| A value may simply not exist: a search with no match | return an `option` |
| The program is wrong if this happens: a broken assumption | `assert` or `panic` |

- `result T, E` is `ok(value)` or `err(error)`; `?` passes an error up to the caller.
- `Error`, from `std.error.failure`, is the standard error type; `main` may return
  `result int, Error`.
- `panic` and `assert` stop the program, with a message on standard error and status 1.
