[العربية](../../ar/book/ch04-00-understanding-ownership.md) | **English**

# Understanding Ownership

Every program uses memory while it runs, and must give that memory back when it no longer needs it.
Languages handle this in one of two ways. Some leave it to the programmer, who must free each piece of
memory exactly once, at the right time; a mistake crashes the program or, worse, lets it read data
that is no longer valid. Others use a *garbage collector*, a part of the runtime that looks for unused
memory while the program runs and frees it, at a cost in speed and predictability.

SultanC takes a third way. A set of rules, called *ownership*, decides when each piece of memory is
freed. The compiler checks the rules before the program runs, and inserts the code that frees memory
at exactly the right places. You never write that code, nothing looks for unused memory while the
program runs, and a program that compiles cannot use memory after freeing it or free it twice.

The rules take some getting used to, and the compiler will refuse programs that seem fine at first. This
chapter explains the rules, the messages they produce, and the habits that make them easy to follow.

- [What Is Ownership?](ch04-01-what-is-ownership.md)
- [References and Borrowing](ch04-02-references-and-borrowing.md)
- [Copying and Cloning](ch04-03-copying-and-cloning.md)
