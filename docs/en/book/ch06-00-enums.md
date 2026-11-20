[العربية](../../ar/book/ch06-00-enums.md) | **English**

# Enums and Pattern Matching

A record says "this value has all of these parts". An *enum* says "this value is one of these cases". A
traffic light is red, amber or green. A shape is a circle or a rectangle. A search found something, or
found nothing. Each of these is an enum.

Enums work together with `match`, which you have used since chapter 2. A `match` on an enum chooses the
case and takes out the data inside it in one step, and the compiler checks that no case is forgotten.
`result` and `option`, which you have also used since chapter 2, are enums too.

- [Defining an Enum](ch06-01-defining-an-enum.md)
- [The `match` Statement](ch06-02-match.md)
- [`option` and `result`](ch06-03-option-and-result.md)
