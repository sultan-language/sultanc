# Language Tour

This tour introduces SultanC through short programs, each shown in Arabic and in English. It is written for
programmers who already know another systems language and want to read and write SultanC. Every example
compiles with the current compiler; the sections point to the reference pages for the full rules.

## Two surfaces, one language

Arabic and English are equal source surfaces. A keyword, a type name or a standard-library function has one
meaning and two spellings, and the compiler resolves both to the same canonical form before any later stage sees
them. A file can mix the two. Names you declare may use either script.

The entry function is `main` in English and `الرئيسية` in Arabic, and the `int` it returns is the process exit
status. Examples below show the Arabic form first. The complete vocabulary is in
[Arabic terminology](../reference/arabic-terminology.md).

## Values and types

`let` binds a name. A binding is immutable unless its type is wrapped in `mutable`, and `:=` assigns. Types are
written after the name. A top-level `let` is a module constant evaluated at compile time.

The builtin types are `int` (`عدد`), `uint` (`طبيعي`), `bool` (`منطقي`), `char` (`حرف`), `string` (`نص`),
`void` (`فراغ`), the fixed-width integers `i8`…`i64` (`ص8`…`ص64`) and `u8`…`u64` (`ط8`…`ط64`), and the floats
`f32` and `f64` (`ع32`, `ع64`). Conversions between scalar types are explicit and use `as` (`مثل`).

```sultan
دع الحد: عدد = 100;

عرف الرئيسية(): عدد {
    دع البايت: ط8 = 255;
    دع النسبة: ع64 = 0.75;
    دع الحرف: حرف = 'س';
    دع الاسم: نص = "سلطان";
    دع العداد: متغير عدد = 0;
    العداد := العداد + 1;
    دع موسع: ص64 = (البايت مثل ص64) - 250;
    أرجع العداد + الحد + (موسع مثل عدد);
}
```

```sultan
let limit: int = 100;

def main(): int {
    let byte: u8 = 255;
    let ratio: f64 = 0.75;
    let letter: char = 'S';
    let name: string = "Sultan";
    let counter: mutable int = 0;
    counter := counter + 1;
    let widened: i64 = (byte as i64) - 250;
    return counter + limit + (widened as int);
}
```

See [Type system](../reference/type-system.md) for the type model and the exact conversion rules.

## Functions

A function declares its parameter types and its result type. Anonymous functions capture the values around them,
and function types are written `fn(int): int` (`دالة(عدد): عدد`). A function takes type parameters in angle brackets.

```sultan
عرف أنشئ_جامعا(المقدار: عدد): دالة(عدد): عدد {
    أرجع دالة(القيمة: عدد): عدد -> القيمة + المقدار;
}

عرف هوية<T>(القيمة: T): T {
    أرجع القيمة;
}

عرف الرئيسية(): عدد {
    دع أضف_عشرة: دالة(عدد): عدد = أنشئ_جامعا(10);
    أرجع أضف_عشرة(هوية(5));
}
```

```sultan
def make_adder(amount: int): fn(int): int {
    return fn(value: int): int -> value + amount;
}

def identity<T>(value: T): T {
    return value;
}

def main(): int {
    let add_ten: fn(int): int = make_adder(10);
    return add_ten(identity(5));
}
```

## Control flow

`if` takes a parenthesized condition and braced blocks, and the statement ends with a semicolon. `while`
(`طالما`) tests a condition before each pass. `for` (`لكل`) has the C form with an initializer, a condition and a
step. `continue` (`تابع_حلقة`) and `break` (`اخرج_من_حلقة`) work in both loops.

```sultan
استورد std.io.println;
استورد std.text.number;

عرف الرئيسية(): عدد {
    دع المجموع: متغير عدد = 0;
    دع ف: متغير عدد = 1;
    طالما (ف <= 5) {
        المجموع := المجموع + ف;
        ف := ف + 1;
    }
    لكل (دع ك: متغير عدد = 0; ك < 10; ك := ك + 1) {
        لو (ك % 2 == 0) { تابع_حلقة; };
        لو (ك > 6) { اخرج_من_حلقة; };
        المجموع := المجموع + ك;
    }
    لو (المجموع > 20) {
        اطبع_سطر("كبير");
    } وإلا {
        اطبع_سطر("صغير");
    };
    اطبع_سطر(عدد_إلى_نص(المجموع));
    أرجع 0;
}
```

```sultan
import std.io.println;
import std.text.number;

def main(): int {
    let total: mutable int = 0;
    let i: mutable int = 1;
    while (i <= 5) {
        total := total + i;
        i := i + 1;
    }
    for (let k: mutable int = 0; k < 10; k := k + 1) {
        if (k % 2 == 0) { continue; };
        if (k > 6) { break; };
        total := total + k;
    }
    if (total > 20) {
        println("large");
    } else {
        println("small");
    };
    println(int_to_text(total));
    return 0;
}
```

## Records, enums and pattern matching

A record type names a set of fields. A value is written as a literal in braces and a field is read with a dot.
`type` also names an existing type, as in `type Celsius = int;`. An enum lists its constructors, each optionally carrying a payload with `of` (`بنوع`). `match` (`طابق`) selects an
arm by pattern. The compiler checks that the arms are exhaustive. Patterns destructure payloads and records, and
match integer, character and string literals, alternatives separated by `|`, and ranges.

```sultan
نوع نقطة = { س: عدد, ص: عدد };

نوع شكل =
    | دائرة بنوع عدد
    | مستطيل بنوع نقطة
    | نقطة_فقط
;

عرف المساحة(الشكل: شكل): عدد {
    طابق الشكل مع
    | شكل.دائرة(نق) -> { أرجع 3 * نق * نق; }
    | شكل.مستطيل({ س: العرض, ص: الارتفاع }) -> { أرجع العرض * الارتفاع; }
    | شكل.نقطة_فقط -> { أرجع 0; };
}

عرف الرئيسية(): عدد {
    دع الحجم: نقطة = { س: 3, ص: 4 };
    أرجع المساحة(شكل.دائرة(2)) + المساحة(شكل.مستطيل(الحجم)) + الحجم.س;
}
```

```sultan
type Point = { x: int, y: int };

type Shape =
    | Circle of int
    | Rectangle of Point
    | Dot
;

def area(shape: Shape): int {
    match shape with
    | Shape.Circle(r) -> { return 3 * r * r; }
    | Shape.Rectangle({ x: width, y: height }) -> { return width * height; }
    | Shape.Dot -> { return 0; };
}

def main(): int {
    let size: Point = { x: 3, y: 4 };
    return area(Shape.Circle(2)) + area(Shape.Rectangle(size)) + size.x;
}
```

A constructor without a payload is written without parentheses in a pattern and with them when it is built, as in
`Shape.Dot()` below the type.

## Vectors

A `vector T` (`متجه T`) is a growable sequence that owns its elements. A literal is written in brackets. Indexing
uses `[ ]`, `length` (`الطول`) gives the element count, and `append` (`ألحق`) adds an element to a `mutable`
vector. Passing `&values` lends the vector without giving it away.

```sultan
عرف الرئيسية(): عدد {
    دع القيم: متغير متجه عدد = [10, 20];
    ألحق(القيم, 12);
    أرجع القيم[0] + القيم[2] + الطول(القيم);
}
```

```sultan
def main(): int {
    let values: mutable vector int = [10, 20];
    append(values, 12);
    return values[0] + values[2] + length(values);
}
```

## Ownership and borrowing

Every value has one owner. Using an owning value, such as a `string`, a `vector` or a record that contains one,
moves it: the old name can no longer be used. Plain values such as numbers are copied. A reference `&T`
borrows a value for reading, and `&mutable T` (`&متغير T`) borrows it for writing. A reference is dereferenced with
`*`. `clone` (`استنسخ`) makes an independent deep copy. The compiler checks all of this before generating code,
and frees an owned value when its owner goes out of scope.

```sultan
استورد std.io.println;
استورد std.text.number;

نوع حساب = { المالك: نص, الرصيد: عدد };

عرف أودع(الحساب: &متغير حساب, المقدار: عدد): فراغ {
    (*الحساب).الرصيد := (*الحساب).الرصيد + المقدار;
}

عرف أغلق(الحساب: حساب): نص {
    أرجع استنسخ(&الحساب.المالك);
}

عرف الرئيسية(): عدد {
    دع الحساب: متغير حساب = { المالك: "ليلى", الرصيد: 100 };
    أودع(&متغير الحساب, 50);
    اطبع_سطر(عدد_إلى_نص(الحساب.الرصيد));
    اطبع_سطر(أغلق(الحساب));
    أرجع 0;
}
```

```sultan
import std.io.println;
import std.text.number;

type Account = { owner: string, balance: int };

def deposit(account: &mutable Account, amount: int): void {
    (*account).balance := (*account).balance + amount;
}

def close(account: Account): string {
    return clone(&account.owner);
}

def main(): int {
    let account: mutable Account = { owner: "Layla", balance: 100 };
    deposit(&mutable account, 50);
    println(int_to_text(account.balance));
    println(close(account));
    return 0;
}
```

`close` takes its `Account` by value, so `account` is moved at the call. Using `account` afterwards is rejected
with `SULTANC-OWNERSHIP-USE-AFTER-MOVE`, which points at both the original owner and the move. A mutable borrow
cannot coexist with another borrow of the same value (`SULTANC-BORROW-CONFLICT`), and an owned
value cannot be moved out of borrowed storage (`SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE`). The rules, the drop
model and `clone` are in [Ownership and destruction](../reference/ownership.md).

## Errors

Failure is a value. `result T, E` (`نتيجة T, E`) holds `result.ok(value)` (`نتيجة.نجح`) or `result.err(error)`
(`نتيجة.خطأ`). `option T` (`اختياري T`) holds `option.some(value)` (`اختياري.موجود`) or `option.none`
(`اختياري.لا_شيء`). `match` takes either apart. The postfix `?` (`؟`) returns the error or the absence to the
caller and otherwise yields the value, as in `let q: int = divide(a, b)?;`. The error type must be the same on both
sides of the call.

```sultan
استورد std.io.println;
استورد std.text.number;

عرف اقسم(أ: عدد, ب: عدد): نتيجة عدد, نص {
    لو (ب == 0) { أرجع نتيجة.خطأ("القسمة على صفر"); };
    أرجع نتيجة.نجح(أ / ب);
}

عرف نصف_الناتج(أ: عدد, ب: عدد): نتيجة عدد, نص {
    دع ق: عدد = اقسم(أ, ب)؟;
    أرجع نتيجة.نجح(ق / 2);
}

عرف اطبع_النتيجة(ن: نتيجة عدد, نص): فراغ {
    طابق ن مع
    | نتيجة.نجح(ق) -> { اطبع_سطر(عدد_إلى_نص(ق)); }
    | نتيجة.خطأ(رسالة) -> { اطبع_سطر(رسالة); };
}

عرف الرئيسية(): عدد {
    اطبع_النتيجة(نصف_الناتج(84, 2));
    اطبع_النتيجة(نصف_الناتج(1, 0));
    أرجع 0;
}
```

```sultan
import std.io.println;
import std.text.number;

def divide(a: int, b: int): result int, string {
    if (b == 0) { return result.err("division by zero"); };
    return result.ok(a / b);
}

def half_quotient(a: int, b: int): result int, string {
    let q: int = divide(a, b)?;
    return result.ok(q / 2);
}

def show(outcome: result int, string): void {
    match outcome with
    | result.ok(q) -> { println(int_to_text(q)); }
    | result.err(message) -> { println(message); };
}

def main(): int {
    show(half_quotient(84, 2));
    show(half_quotient(1, 0));
    return 0;
}
```

`panic` (`إنهيار`) stops the program with a message for conditions that should never happen. `main` may itself
return a `result`, in which case an error is printed and the process exits with status 1. The std error value, its
helpers and the exact semantics of `?` are in [Errors](../reference/errors.md).

## Modules and imports

Each `.sn` file is a module. `public` (`عام`) exposes a declaration to importers, and everything else stays private.
There are three import forms: a quoted path relative to the importing file, a dotted module path, and a selective
import of names from a directory. `as` (`مثل`) gives a module a local name used as a qualifier.

```sultan
استورد "geometry_ar.sn";
استورد std.text.number مثل الأعداد;
من std.io استورد اطبع_سطر;

عرف الرئيسية(): عدد {
    دع الغرفة: مستطيل = { العرض: 5, الارتفاع: 4 };
    اطبع_سطر(الأعداد.عدد_إلى_نص(المساحة(&الغرفة)));
    أرجع 0;
}
```

```sultan
import "geometry_en.sn";
import std.text.number as numbers;
from std.io import println;

def main(): int {
    let room: Rectangle = { width: 5, height: 4 };
    println(numbers.int_to_text(area(&room)));
    return 0;
}
```

The module they import:

```sultan
عام نوع مستطيل = { العرض: عدد, الارتفاع: عدد };

عام عرف المساحة(الشكل: &مستطيل): عدد {
    أرجع (*الشكل).العرض * (*الشكل).الارتفاع;
}
```

```sultan
public type Rectangle = { width: int, height: int };

public def area(shape: &Rectangle): int {
    return (*shape).width * (*shape).height;
}
```

`import std.text.number;` brings every public name of that std module into scope, and `import std.text.number
as numbers;` makes them reachable only as `numbers.name`. `std` has the Arabic spelling `المكتبة`, and its
subdirectories have Arabic names as well (`المكتبة.نصوص.number`). [Modules and imports](../reference/modules-and-imports.md)
specifies resolution.

## Using the standard library

`std.io.println` writes a line to standard output, `std.text.number` converts numbers to text and back, and `std.fmt`
formats text with Rust-style placeholders. Each argument to a format function is wrapped in a value that carries
its type, and a literal format string is checked at compile time.

```sultan
استورد المكتبة.تنسيق.fmt;

عرف الرئيسية(): عدد {
    دع الاسم: نص = "سلطان";
    دع ر1: نتيجة طبيعي, خطأ_تنسيق = اطبع_سطرا_منسقا("أهلا يا {}! لديك {} رسائل.", [قيمة_تنسيق.نص(&الاسم), قيمة_تنسيق.عدد(3)]);
    دع ر2: نتيجة طبيعي, خطأ_تنسيق = اطبع_سطرا_منسقا("[{0:>8}] {1:#x} {2:.2}", [قيمة_تنسيق.نص(&الاسم), قيمة_تنسيق.عدد(255), قيمة_تنسيق.ع64(3.14159)]);
    أرجع 0;
}
```

```sultan
import std.fmt.fmt;

def main(): int {
    let name: string = "Sultan";
    let r1: result uint, fmt_error = println_fmt("Hello, {}! You have {} messages.", [fmt_arg.text(&name), fmt_arg.int(3)]);
    let r2: result uint, fmt_error = println_fmt("[{0:>8}] {1:#x} {2:.2}", [fmt_arg.text(&name), fmt_arg.int(255), fmt_arg.f64(3.14159)]);
    return 0;
}
```

```text
Hello, Sultan! You have 3 messages.
[  Sultan] 0xff 3.14
```

The format-spec grammar, custom formatters and alignment rules are in [Formatting](../reference/formatting.md).
[Std coverage](../reference/std-coverage.md) lists what the library provides.

## See also

- [Getting started](getting-started.md)
- [Type system](../reference/type-system.md)
- [Ownership and destruction](../reference/ownership.md)
- [Errors](../reference/errors.md)
- [Compile-time evaluation](../reference/comptime.md)
- [Concurrency](../reference/concurrency.md)
- The `examples/` directory, which holds many more runnable programs in both languages
