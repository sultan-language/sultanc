[العربية](../../ar/book/ch10-00-generics-and-functions.md) | **English**

# Generic Types and Function Values

Every program has duplicated code that differs only in a type, or only in one step. Finding the largest
number in a vector and the longest word in another are the same loop with a different comparison.
Keeping a stack of integers and a stack of strings are the same code with a different element type.

This chapter shows two tools that let you write such code once:

- *generics*, which let a function or type work with any type, written as a *type parameter*;
- *function values*, which let the caller hand over the step that differs, as a function.

The two work together. SultanC has no interfaces or traits: generic code cannot ask whether a type has
`<`. When it needs to compare, order or combine values of a type parameter, it takes a function that
knows how.

- [Generic Data Types](ch10-01-generic-data-types.md)
- [Functions as Values](ch10-02-functions-as-values.md)
- [Closures](ch10-03-closures.md)
