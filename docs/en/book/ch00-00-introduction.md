[العربية](../../ar/book/ch00-00-introduction.md) | **English**

# Introduction

Welcome to *The SultanC Book*. SultanC is a language for writing programs that run fast and use memory
safely: operating system tools, servers, compilers, libraries for other languages to call. Its compiler
checks, before a program ever runs, that no value is used after it is freed, that no two threads write
the same data at once, and that every failure a function can report is handled. And you can write all
of it in Arabic, in English, or in both.

## Who this book is for

This book assumes you have written a program before, in any language, and that you can type commands
in a terminal. It does not assume you know how memory works, what a pointer is, or anything about
systems programming. Each idea is explained when it first appears.

## How to use this book

Read the chapters in order the first time; each one builds on the ones before it.

There are two kinds of chapters. *Concept chapters* teach one part of the language. *Project chapters*
build a program together, using what you have learned so far: chapter 2 is a guessing game, chapter 12
a command line search tool, and chapter 20 a web server. Chapter 2 comes before you have learned much,
on purpose: it shows how a real program fits together, and the chapters after it explain each piece in
depth.

After the first reading, every concept chapter also works as the place to look up its subject. The
appendixes collect what you will look up most: the keywords in both languages, the operators, the
compiler's diagnostics, and the standard library by module.

## How to read the code

Programs are shown in *listings*, with the file they go in:

<sub>Listing 2-1: reading a guess from the user and printing it. Filename: src/main.sn</sub>

Commands you type in a terminal start with `$`. Do not type the `$`; the lines without it are what the
command prints:

```console
$ supax run
Hello from SultanC
```

The book often shows code that does not compile, on purpose, followed by the compiler's error message.
Reading those messages is a large part of learning SultanC, and the text after each one explains what it
says and how to fix the code. A listing that does not compile always says so in its caption or in the
text before it.

## The editions

This English edition uses the English keywords: `def`, `let`, `if`, `match`. The
[Arabic edition](../../ar/book/ch00-00-introduction.md) uses the Arabic ones: `عرف`, `دع`, `إذا`,
`طابق`. Both describe one language: a program may mix the two, and the compiler treats `def` and `عرف`
as the same word. Appendix A lists every pair.

## Checked examples

Every program in this book is compiled and run by `tools/docs/check_guide.py` with the current compiler,
and its output is compared with what the book shows. When the language changes, the check fails until
the book is updated, so what you read here is what the compiler does today.

To report a mistake or suggest a change, open an issue in the
[SultanC repository](https://github.com/sultan-language/sultanc).
