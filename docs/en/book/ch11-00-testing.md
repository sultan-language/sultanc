[العربية](../../ar/book/ch11-00-testing.md) | **English**

# Writing Automated Tests

The compiler checks a great deal: types, ownership, every path through every function. It cannot check
that a program does what you meant. If `add` returns `a - b`, the compiler is satisfied; you are not.

A *test* is a small function that runs part of your program with known inputs and checks the result.
Tests let you change code with confidence: when they still pass after a change, the behaviour they
check still holds. SultanC has testing built into the language and the compiler, with nothing to
install.

- [How to Write Tests](ch11-01-writing-tests.md)
- [Controlling How Tests Are Run](ch11-02-running-tests.md)
- [Test Organization](ch11-03-test-organization.md)
