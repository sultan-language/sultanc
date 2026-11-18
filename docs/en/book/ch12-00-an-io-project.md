[العربية](../../ar/book/ch12-00-an-io-project.md) | **English**

# An I/O Project: Building a Command Line Program

This chapter builds a real tool from start to end, using what the book has covered so far. It is a small
version of a classic search tool: given a word and a file, it prints the lines of the file that contain
the word.

```console
$ finder stone poem.txt
The river does not ask the stone
Ask the stone, it will say nothing.
```

On the way, the program will:

- read its command line arguments;
- read a file;
- split its work into a library and a small `main`, so that the library can be tested;
- grow its search function one test at a time;
- read an environment variable to change its behaviour;
- write its errors where errors belong, on standard error.

- [Accepting Command Line Arguments](ch12-01-accepting-arguments.md)
- [Reading a File](ch12-02-reading-a-file.md)
- [Refactoring for Modularity and Error Handling](ch12-03-refactoring.md)
- [Developing the Library with Tests](ch12-04-testing-the-library.md)
- [Working with Environment Variables](ch12-05-environment-variables.md)
- [Writing Errors to Standard Error](ch12-06-writing-to-stderr.md)
