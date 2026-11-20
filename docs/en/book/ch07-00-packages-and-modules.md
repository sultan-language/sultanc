[العربية](../../ar/book/ch07-00-packages-and-modules.md) | **English**

# Managing Growing Projects with Packages and Modules

The programs so far have fit in one file. Real programs do not: they grow to dozens or hundreds of
files, and they use code that other people wrote. Two ideas organize them:

- A *module* is one `.sn` file. It groups related code, and decides which of its names other modules may
  use and which stay private.
- A *package* is a project that Supax builds: a `Supax.toml`, a `src` folder of modules, and possibly
  programs, tests and dependencies on other packages.

Splitting code into modules lets you read one part without the rest, hide details that other parts
should not depend on, and change those details freely later.

- [Packages](ch07-01-packages.md)
- [Modules and `public`](ch07-02-modules-and-public.md)
- [The Three Import Forms](ch07-03-import-forms.md)
- [Separating Modules into Files](ch07-04-separating-modules.md)
