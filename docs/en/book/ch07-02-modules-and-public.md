[العربية](../../ar/book/ch07-02-modules-and-public.md) | **English**

# Modules and `public`

Every `.sn` file is a module. A module decides which of its declarations other modules can use: those
marked `public`. Everything else is *private*: usable inside the module, and invisible outside it.

Here is a module, in the file `geometry.sn`:

<sub>Filename: geometry.sn</sub>

```sultan file=geometry.sn
public type Rectangle = { width: int, height: int };

public def area(r: &Rectangle): int {
    return (*r).width * (*r).height;
}

public def perimeter(r: &Rectangle): int {
    return double((*r).width + (*r).height);
}

def double(value: int): int {
    return value * 2;
}
```

And a program beside it that imports it by its file name:

<sub>Filename: main.sn</sub>

```sultan
import "geometry.sn";
import std.io.println;
import std.text.number;

def main(): int {
    let room: Rectangle = { width: 5, height: 4 };
    println(int_to_text(area(&room)));
    println(int_to_text(perimeter(&room)));
    return 0;
}
```

```text
20
18
```

Compile the program as usual, naming only the file with `main`; the compiler follows the imports and
finds `geometry.sn` itself:

```console
$ sultanc main.sn -o rooms
```

`main.sn` uses `Rectangle`, `area` and `perimeter`, which are `public`. `double` is not: it is a helper
that `perimeter` needs, and no other module can call it:

```sultan fail
import "geometry.sn";

def main(): int {
    return double(4);
}
```

```text
error[SULTANC-NAME-PRIVATE]: declaration is private in this context — The inaccessible private declaration is `double`. Name resolution cannot access a private declaration.
  --> main.sn:4:12..4:18
   |
 4 |     return double(4);
   |            ^^^^^^
   |
  = help: declare the name `public` in its module to use it from another module
```

## Why keep things private

Everything a module makes public, other modules may come to depend on. Changing a public function's name
or parameters then means changing every module that uses it. A private function can change any time,
because the compiler guarantees nothing outside its module uses it.

So make public only what other modules need: the types and functions that are the module's purpose.
Keep the helpers that implement them private. The public declarations of a module are its *interface*:
what the rest of the program can rely on.

## Public types and their fields

A `public` record type is visible together with its fields: a module that can see `Rectangle` can make
one with `{ width: 5, height: 4 }` and read `room.width`. When a type has rules that its fields must
follow, offer a public function that builds it correctly, like `from_celsius` in chapter 5, and let
other modules use that.
