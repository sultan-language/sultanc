[العربية](../../ar/book/ch09-00-error-handling.md) | **English**

# Error Handling

Things go wrong. A file is missing, a user types letters where a number belongs, a network connection
drops. A program that is to be relied on must decide, for every one of these, what to do.

SultanC sorts failures into two kinds, and handles them differently:

- **Recoverable errors** are part of normal life: the file might not be there, and the program should
  say so and go on. These are ordinary values of type `result`, returned like any other value. The type
  of a function says whether it can fail, and the compiler makes the caller deal with it.
- **Unrecoverable errors** are bugs: a condition that the program itself guarantees has turned out
  false. Going on would only spread the damage, so the program stops.

There are no exceptions, which jump from where a failure happens to some handler far away, past code
that never mentions them. In SultanC, a failure travels only along paths you can see in the code.

- [Unrecoverable Errors with `panic`](ch09-01-unrecoverable-errors.md)
- [Recoverable Errors with `result` and `?`](ch09-02-recoverable-errors.md)
- [To `panic` or Not to `panic`](ch09-03-to-panic-or-not.md)
