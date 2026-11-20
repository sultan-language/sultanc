[العربية](../../ar/book/ch08-00-common-collections.md) | **English**

# Common Collections

A *collection* holds many values. Unlike a tuple or a record, whose size is fixed when the program is
written, a collection can grow and shrink while the program runs, so its contents live on the heap and
the collection owns them.

This chapter covers the three collections almost every program uses:

- a **vector** holds values in order, and finds them by position;
- a **string** holds text, which is a sequence of bytes that spell characters;
- a **map** finds a value by a key, and a **set** keeps distinct values.

The standard library's `std.collections` has more: queues, heaps, and maps and sets with different
trade-offs. The last section shows how to choose.

- [Vectors](ch08-01-vectors.md)
- [Text, Bytes and Characters](ch08-02-strings.md)
- [Maps and Sets](ch08-03-maps-and-sets.md)
