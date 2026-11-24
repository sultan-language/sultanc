[العربية](../../ar/book/ch07-04-separating-modules.md) | **English**

# Separating Modules into Files

This section builds a small package with a library, the way larger projects are organized.

## The layout

Start from the `shop` package of section 7.1, with `[lib] path = "src"` in its manifest. Prices need a
tax calculation, so it goes in a module of its own, in a folder for everything about prices:

```text
shop/
├── Supax.toml
└── src/
    ├── main.sn
    └── prices/
        └── tax.sn
```

<sub>Filename: src/prices/tax.sn</sub>

```sultan ignore
public def with_tax(amount: int): int {
    return amount + amount * rate_percent() / 100;
}

def rate_percent(): int {
    return 15;
}
```

The file `src/prices/tax.sn` is the module `shop.prices.tax`: the package name, then the folders and the
file name under `src`. The main program imports it by that name:

<sub>Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.text.number;
import shop.prices.tax;

def main(): int {
    println(int_to_text(with_tax(200)));
    return 0;
}
```

```console
$ supax run
   Compiling shop v0.1.0 (/projects/shop)
    Finished in 0.13s → build/bin/shop
     Running build/bin/shop
230
```

`rate_percent` is private to `shop.prices.tax`, so `main.sn` cannot call it:

```text
error[SULTANC-NAME-PRIVATE]: declaration is private in this context — The inaccessible private declaration is `rate_percent`. Name resolution cannot access a private declaration.
  --> /projects/shop/src/main.sn:6:25..6:37
   |
 6 |     println(int_to_text(rate_percent()));
   |                         ^^^^^^^^^^^^
   |
  = help: declare the name `public` in its module to use it from another module
```

## How to divide a program

There is no single right layout, but a few habits work well:

- One module per subject: everything about prices in `prices`, everything about customers in
  `customers`. A module whose name you cannot say in one word probably does two things.
- Put a type and the functions that work on it in the same module.
- Keep `main.sn` short: reading input, calling the library, printing the result. The real work lives in
  the library, where tests and other programs can use it too.
- Start with one file, and split when a part grows big enough to deserve its own name.

## Names in other languages

A package can give its modules names in other languages, as the standard library does. The names are
declared in *root files*: `src/NAME.sn` for the package's top-level modules, and `FOLDER/FOLDER.sn` for
the modules in a folder. Each line names a module and its other spellings:

<sub>Filename: src/shop.sn</sub>

```sultan ignore
module prices { ar: الأسعار };
```

<sub>Filename: src/prices/prices.sn</sub>

```sultan ignore
module tax { ar: الضريبة };
```

With these, `import shop.الأسعار.الضريبة;` imports the same module as `import shop.prices.tax;`. The
standard library's own root file, `library/std/std.sn`, declares the Arabic names of its modules this
way.

## Summary

- A package is a `Supax.toml` and a `src` folder; `[lib] path = "src"` makes `src` a library.
- `src/main.sn` and `src/bin/NAME.sn` are programs.
- Every file is a module; only `public` declarations are visible outside it.
- `import a.b.c;`, `import a.b.c as p;` and `from a.b import c;` import modules by name; `import
  "file.sn";` imports by path.
- `src/a/b.sn` in package `NAME` is the module `NAME.a.b`; root files give modules other spellings.
