[العربية](../../ar/book/appendix-03-diagnostics.md) | **English**

# C. Diagnostics

Every message the compiler writes has an identity of the form `SULTANC-AREA-NAME`, which never changes
between versions and is the same in both languages. Chapter 1 showed how to read a message.

`sultanc explain IDENTITY` prints the explanation of one identity, in the language chosen with `--lang`:

```console
$ sultanc --lang=en explain SULTANC-OWNERSHIP-USE-AFTER-MOVE
SULTANC-OWNERSHIP-USE-AFTER-MOVE

Value used after move. Find the move in the related spans.
```

`--diagnostic-format=json` writes each message as a JSON object, for editors and other tools, and
`--diagnostic-format=short` as one line. The table below lists every identity with its explanation, as
`sultanc explain` gives it, grouped by the part of the compiler that reports it.

## Reading characters

| Identity | Explanation |
| --- | --- |
| `SULTANC-LEX-INVALID-TOKEN` | The source contains characters that do not form a valid token. Remove or correct them. |
| `SULTANC-LEX-INVALID-UTF8` | The source file is not valid UTF-8. Save it with UTF-8 encoding. |

## Grammar

| Identity | Explanation |
| --- | --- |
| `SULTANC-PARSE-EXPRESSION` | An expression is written in a form the parser cannot read. Check operators, brackets and operand order. |
| `SULTANC-PARSE-INCOMPLETE-STATEMENT` | This statement does nothing. A statement must call a function or assign a value. |
| `SULTANC-PARSE-ITEM` | A top-level declaration is malformed. Check the keyword, name and body of the declaration. |
| `SULTANC-PARSE-KEYWORD-TYPO` | This word looks like a misspelled keyword. Use the suggested keyword spelling. |
| `SULTANC-PARSE-MISPLACED-SEMICOLON` | A semicolon is placed where the grammar does not allow one. Move it to the end of the statement. |
| `SULTANC-PARSE-MISSING-RETURN-TYPE` | Every function must declare its return type. Add the type after the parameter list. |
| `SULTANC-PARSE-PATTERN` | A pattern is written in a form the parser cannot read. Check the constructor name and its bindings. |
| `SULTANC-PARSE-SYNTAX` | The source does not follow SultanC grammar here. Fix the first syntax error; later ones often follow from it. |
| `SULTANC-PARSE-TYPE` | A type is written in a form the parser cannot read. Check the type name and its parameters. |

## Grammar

| Identity | Explanation |
| --- | --- |
| `SULTANC-SYNTAX-DEPRECATED-SPELLING` | This old spelling is still accepted for one release and will then be removed. Replace it with the new spelling the message names. |

## Names

| Identity | Explanation |
| --- | --- |
| `SULTANC-NAME-DUPLICATE-DECLARATION` | The same name is declared twice in one scope. Rename or remove one declaration. |
| `SULTANC-NAME-PRIVATE` | This declaration is private here. Access it from an allowed scope. |
| `SULTANC-NAME-UNKNOWN` | Name is not visible here. Check spelling, scope and imports. |
| `SULTANC-NAME-UNRESOLVED` | The name could not be bound to a declaration. Check that it is declared and imported. |

## Imports

| Identity | Explanation |
| --- | --- |
| `SULTANC-IMPORT-AMBIGUOUS` | Two imports provide the same name. Import only one, or use a qualified name. |
| `SULTANC-IMPORT-CYCLE` | Modules import each other in a cycle. Move shared code to a module both can import. |
| `SULTANC-IMPORT-ESCAPES-ROOT` | The import path leaves its library root. Import the module through its own root. |
| `SULTANC-IMPORT-INVALID-REFERENCE` | The import is written in a form that does not name a module. Check the import path. |
| `SULTANC-IMPORT-SELECTIVE-EXPORT-NOT-FOUND` | The module does not export the name you import. Check the name or the module. |

## Modules

| Identity | Explanation |
| --- | --- |
| `SULTANC-MODULE-INVALID-LOGICAL-PATH` | The logical module path is not valid. Use dot-separated module names. |
| `SULTANC-MODULE-LIBRARY-ROOT-UNREGISTERED` | The import uses a library root that is not registered. Add the root to the package or library path. |
| `SULTANC-MODULE-NOT-FOUND` | Module not found under registered roots. Check the import path. |
| `SULTANC-MODULE-OUTSIDE-ROOT-FILE` | Spellings of a folder's children are declared in the folder's root file, FOLDER/FOLDER.sn, or in a package's root file, SOURCE_ROOT/PACKAGE.sn. Move the declaration there. |
| `SULTANC-MODULE-PUBLIC-PATH-CONFLICT` | Two modules claim the same public path. Rename one of them. |
| `SULTANC-MODULE-REEXPORT-CYCLE` | Each re-export leads to another module path; following them reached a path already followed, so the import names no module. Make one re-export in the chain name the module's own library. |
| `SULTANC-MODULE-REEXPORT-SHADOWS` | A re-export adds a child to a folder; the folder already has its own module of that name. Remove the re-export or the folder's own child. |
| `SULTANC-MODULE-REEXPORT-UNAVAILABLE` | A re-export names a module of a visible library beneath the declaring one (std sees alloc and core; alloc sees core). This target is missing or not visible. |
| `SULTANC-MODULE-SOURCE-CONFLICT` | Two source files are given for the same module. Keep only one. |
| `SULTANC-MODULE-SOURCE-UNREADABLE` | The module file exists but could not be read. Check its permissions and encoding. |
| `SULTANC-MODULE-SPELLING-NO-CHILD` | A `module` declaration gives spellings to a child of its folder, named as it is on disk: a NAME.sn file or a NAME folder next to the root file. Correct the name or remove the declaration. |
| `SULTANC-MODULE-SPELLING-RESERVED` | A module spelling is written in import paths, where a reserved word of the language cannot stand. Choose a spelling that is not a reserved word. |
| `SULTANC-MODULE-SPELLING-SHARED` | An import names a child of a folder by its name or by one of its spellings, so a spelling names one child only, and no other child's name. Give each child its own spellings. |

## Packages

| Identity | Explanation |
| --- | --- |
| `SULTANC-PACKAGE-DEPENDENCY-MAP-INVALID` | No longer emitted by sultanc; Supax reports manifest and dependency errors. |
| `SULTANC-PACKAGE-GIT-DEPENDENCY-UNRESOLVED` | No longer emitted by sultanc; Supax reports manifest and dependency errors. |
| `SULTANC-PACKAGE-GRAPH-INVALID` | The package graph given with --package-graph cannot be used. Supax writes it: run `supax check` or `supax build`. |
| `SULTANC-PACKAGE-MANIFEST-INVALID` | No longer emitted by sultanc; Supax reports manifest and dependency errors. |

## Types

| Identity | Explanation |
| --- | --- |
| `SULTANC-TYPE-ADDRESS-OF-NON-LVALUE` | Taking an address requires addressable storage. |
| `SULTANC-TYPE-DEREFERENCE-NON-POINTER` | Dereference requires a pointer or reference value. |
| `SULTANC-TYPE-DUPLICATE-RECORD-FIELD` | A record initializer repeats a field. |
| `SULTANC-TYPE-INFINITE-RECURSION` | Recursive type has no finite layout. Introduce pointer indirection. |
| `SULTANC-TYPE-INVALID-DEFINITION` | This type definition breaks a rule for type declarations. Check its fields and constructors. |
| `SULTANC-TYPE-MISMATCH` | Types disagree. Compare expected and actual types in the diagnostic. |
| `SULTANC-TYPE-MISSING-RECORD-FIELD` | A record initializer is missing a required field. |
| `SULTANC-TYPE-OPAQUE-BY-VALUE` | An opaque type has no fields, size or alignment, so no value of it can be stored, passed, returned or held in a container. Only a raw pointer to it holds a value. |
| `SULTANC-TYPE-OPAQUE-DEREFERENCE` | The memory behind a pointer to an opaque type has no type the program can read or write, even inside unsafe. |
| `SULTANC-TYPE-OPAQUE-FIELD-ACCESS` | An opaque type declares no fields; its contents are known only to the code that defines it. |
| `SULTANC-TYPE-OPAQUE-POINTER-ARITHMETIC` | Indexing `p[i]` steps the pointer by the element size, and an opaque type has none. |
| `SULTANC-TYPE-OPAQUE-TYPE-ARGUMENT` | The instantiated generic uses its type parameter by value (a copy, a dereference, size_of or align_of, a local or a container), and an opaque type has no value, size or alignment. |
| `SULTANC-TYPE-RETURN-MISMATCH` | The returned value has a different type than the function declares. |
| `SULTANC-TYPE-UNKNOWN-RECORD-FIELD` | The record type has no field with this name. Check the field name. |
| `SULTANC-TYPE-UNRESOLVED` | This type name does not refer to a known type. Check its spelling and imports. |

## Calls

| Identity | Explanation |
| --- | --- |
| `SULTANC-CALL-ARGUMENT-COUNT-MISMATCH` | The call passes a different number of arguments than the function declares. |
| `SULTANC-CALL-ARGUMENT-TYPE-MISMATCH` | An argument has a different type than its parameter. Convert it or pass a value of the right type. |
| `SULTANC-CALL-NOT-CALLABLE` | Only functions can be called. This expression is not a function. |

## Patterns

| Identity | Explanation |
| --- | --- |
| `SULTANC-PATTERN-INVALID` | This pattern cannot match the value's type. Use a pattern of that type. |
| `SULTANC-PATTERN-NON-EXHAUSTIVE` | Pattern match misses possible values. Cover the missing cases. |
| `SULTANC-PATTERN-PAYLOAD-COUNT-MISMATCH` | The pattern binds a different number of payloads than the constructor has. |
| `SULTANC-PATTERN-UNREACHABLE` | An earlier pattern already covers this one, so it never runs. Remove or reorder it. |

## Control flow

| Identity | Explanation |
| --- | --- |
| `SULTANC-FLOW-LOOP-CONTROL-OUTSIDE-LOOP` | Leaving or continuing a loop is only possible inside a while or for loop of the same function. |
| `SULTANC-FLOW-MISSING-RETURN` | Some path through the function ends without returning a value. Add the missing return. |
| `SULTANC-FLOW-USE-BEFORE-INITIALIZATION` | The variable is read before it has a value. Assign it on every path before use. |

## Mutability

| Identity | Explanation |
| --- | --- |
| `SULTANC-MUTABILITY-WRITE-TO-IMMUTABLE` | Cannot assign to immutable storage. |

## Ownership

| Identity | Explanation |
| --- | --- |
| `SULTANC-OWNERSHIP-MOVE-OUT-OF-REFERENCE` | Owned value moved out of a reference. Borrow it with & or copy it with clone(&value). |
| `SULTANC-OWNERSHIP-PARTIAL-MOVE` | Moving one owned member out of an aggregate is not allowed. Move or copy the whole value. |
| `SULTANC-OWNERSHIP-RESOURCE-RELEASED` | The resource is used after it was released. Use it before the release, or do not release it yet. |
| `SULTANC-OWNERSHIP-USE-AFTER-MOVE` | Value used after move. Find the move in the related spans. |

## Borrowing

| Identity | Explanation |
| --- | --- |
| `SULTANC-BORROW-CONFLICT` | A borrow conflicts with another active borrow of the same value. End one borrow first. |
| `SULTANC-BORROW-REFERENCE-ESCAPE` | A reference outlives the value it points to. Return or store the value instead. |

## Ownership tracking

| Identity | Explanation |
| --- | --- |
| `SULTANC-TRACK-CALLER-EXPLICIT-LOCATION` | A call of a `@@track_caller` function passes every argument but the last: the compiler passes the place of the call there (inside another marked function, the location that function received). |
| `SULTANC-TRACK-CALLER-SIGNATURE` | A `@@track_caller` function has a body, no type or compile-time parameters, and ends with a std.debug.location `source_location` parameter that each call fills with its place. |

## Memory safety

| Identity | Explanation |
| --- | --- |
| `SULTANC-SAFETY-INDEX-OUT-OF-BOUNDS` | The index is outside the bounds of the collection. Check the index against the length. |
| `SULTANC-SAFETY-UNSAFE-OPERATION` | This operation is only allowed inside an explicit unsafe scope. |

## Threads

| Identity | Explanation |
| --- | --- |
| `SULTANC-THREAD-CAPTURE-NON-ATOMIC-SHARED` | Every thread shares a writable module variable without atomicity. Use an atomic, a Mutex or an Arc instead. |
| `SULTANC-THREAD-CAPTURE-RAW-POINTER` | Nothing synchronizes what a raw pointer points to. Share the value through an Arc with a Mutex or an atomic, or spawn inside unsafe. |
| `SULTANC-THREAD-CAPTURE-REFERENCE` | The thread may outlive the borrowed storage. Move an owned value into the closure, or share it through an Arc. |

## Compile-time evaluation

| Identity | Explanation |
| --- | --- |
| `SULTANC-COMPTIME-ARGUMENT-COUNT-MISMATCH` | Pass exactly one value for each compile-time parameter of the function. |
| `SULTANC-COMPTIME-EVALUATION-FAILED` | Compile-time code failed or trapped while running. Fix the failing operation it reports. |
| `SULTANC-COMPTIME-FAILED` | Compile-time code rejected its input. Correct the issue described by the diagnostic. |
| `SULTANC-COMPTIME-LAYOUT-UNAVAILABLE` | Size and alignment queries need a type whose layout is already fixed for the target. |
| `SULTANC-COMPTIME-LIMIT-EXCEEDED` | Compile-time code ran too long, recursed too deeply or specialized too often. Simplify it. |
| `SULTANC-COMPTIME-NON-CONSTANT` | This expression requires runtime evaluation, not compile-time computation. |
| `SULTANC-COMPTIME-PROHIBITED-EFFECT` | Compile-time code may not do I/O or other outside effects. Move that work to runtime. |
| `SULTANC-COMPTIME-RESULT-NOT-CONSTANT` | Compile-time code must return a self-contained constant of the expected type. |
| `SULTANC-COMPTIME-UNSUPPORTED-OPERATION` | Only SultanC functions and supported operations can run at compile time. Move this to runtime. |

## Annotations

| Identity | Explanation |
| --- | --- |
| `SULTANC-ANNOTATION-UNSUPPORTED` | This declaration attribute is not supported by the current annotation processor. |

## The entry point

| Identity | Explanation |
| --- | --- |
| `SULTANC-ENTRYPOINT-INVALID-SIGNATURE` | Entry-point signature is invalid. Check parameters and return type. |
| `SULTANC-ENTRYPOINT-NOT-FOUND` | No valid entry point. Declare the required entry function. |

## Format strings

| Identity | Explanation |
| --- | --- |
| `SULTANC-FORMAT-STRING` | A literal format string is checked against its arguments at compile time. Correct the placeholder or the arguments. |

## Format strings

| Identity | Explanation |
| --- | --- |
| `SULTANC-FMT-UNFORMATTED` | `sultanc fmt --check` found a source file whose layout differs from the canonical one. Run `sultanc fmt` on it; formatting changes only whitespace. |

## External declarations

| Identity | Explanation |
| --- | --- |
| `SULTANC-EXTERN-SYMBOL-CONFLICT` | Every external declaration of one linker symbol must declare the same parameter and result types. Keep one extern for the symbol and reach other shapes through ordinary functions. |

## Module variables

| Identity | Explanation |
| --- | --- |
| `SULTANC-GLOBAL-ALLOCATOR-INVALID` | Mark all three or none: `@@global_alloc` (size: W): W, `@@global_realloc` (address: W, size: W): W, `@@global_dealloc` (address: W): void, W the pointer-width unsigned integer. |

## C imports

| Identity | Explanation |
| --- | --- |
| `SULTANC-C-ABI-MISUSE` | A `@@c_abi` function has a body, no type or compile-time parameters, and only scalar or raw-pointer parameters and result. C calls it at `c_function_address(f)`; it is no function value. |
| `SULTANC-C-ARGUMENT-COUNT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-COMPILER-FAILED` | With SULTANC_C_COMPILER=cc, the system C compiler compiles imported .c files, and it reported an error for this one (its own messages come first). Fix the C source, or unset SULTANC_C_COMPILER so Jisr compiles it. |
| `SULTANC-C-COMPILER-HOST-ONLY` | With SULTANC_C_COMPILER=cc, imported .c files are compiled by the system C compiler, which builds for the host only. Unset SULTANC_C_COMPILER so Jisr compiles the C for any target, or build for the host. |
| `SULTANC-C-CONFLICTING-TYPES` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-DUPLICATE-CASE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-DUPLICATE-MACRO-PARAMETER` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-ERROR-DIRECTIVE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-HOST-HEADERS` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INCLUDE-DEPTH` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INCLUDE-NOT-FOUND` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INCOMPATIBLE-TYPES` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INCOMPLETE-MEMBER` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-BIT-FIELD` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-CONDITION` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-DIRECTIVE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-FLEXIBLE-ARRAY` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-IDENTIFIER-CHARACTER` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-INCLUDE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-INITIALIZER` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-JUMP` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-MACRO-NAME` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-MACRO-PARAMETERS` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-NUMBER` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-OPERANDS` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-PASTE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-PRAGMA-OPERATOR` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-PREPROCESSOR` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-STRINGIZE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-INVALID-UTF8` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-MACRO-ARGUMENT-COUNT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-MACRO-NOT-CONSTANT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-MACRO-UNTERMINATED-CALL` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-MISSING-TYPE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NAME-CONFLICT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NAME-CONFLICT-SKIPPED` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NEGATIVE-ARRAY-SIZE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NO-MEMBER` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NO-SYSTEM-HEADERS` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NOT-ASSIGNABLE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NOT-CALLABLE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-NOT-CONSTANT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-READ-HEADER-UNTRANSLATED` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-REDEFINITION` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-STATIC-ASSERT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-SYNTAX` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-TAG-KIND-MISMATCH` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNAVAILABLE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNDECLARED` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNDEFINED-LABEL` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNKNOWN-TYPE-NAME` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNMATCHED-CONDITIONAL` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNPROTOTYPED` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNSUPPORTED` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNSUPPORTED-LABEL` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNSUPPORTED-TYPE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNSUPPORTED-VARIABLE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNSUPPORTED-VARIADIC` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNTERMINATED-COMMENT` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-UNTERMINATED-CONDITIONAL` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-VARIADIC-CALL` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |
| `SULTANC-C-WARNING-DIRECTIVE` | Jisr, the C frontend, found a problem in the imported C file or a header it includes, at the C line and column. Fix the C source, or the include directories ($SULTANC_C_INCLUDE). |

## Tests

| Identity | Explanation |
| --- | --- |
| `SULTANC-TEST-FILE-NOT-IN-PROGRAM` | `--tests-in` keeps the tests declared in the given source files; this file is not a source of the program. Name the entry file or a file it imports. |
| `SULTANC-TEST-NOT-A-FUNCTION` | `@@test` marks an ordinary function with a body: not a type, a module value, a generic or compile-time function, or an external one. Move it to a function the test can call. |
| `SULTANC-TEST-NOT-FOUND` | `--test` runs exactly the tests named, in any spelling of their names; this name is no test of the root package. Run `sultanc test --list` for the names. |
| `SULTANC-TEST-PARAMETERS` | A test runs by itself, with nothing to pass it: `@@test` marks a function without parameters. Make the inputs local values of the test, or call a helper that takes them. |
| `SULTANC-TEST-RESULT-TYPE` | A test returns nothing, or `result int, Error` (std.error.failure), whose error the run reports as the test's failure. Change the result type to one of these. |

## Targets

| Identity | Explanation |
| --- | --- |
| `SULTANC-TARGET-ABI-UNAVAILABLE` | The target has no calling convention for this code. Pick a target that defines one. |
| `SULTANC-TARGET-FEATURE-UNSUPPORTED` | The chosen target lacks a feature this program needs. Pick another target or avoid the feature. |
| `SULTANC-TARGET-HOST-UNAVAILABLE` | The compiler cannot target the machine it runs on. Pass a target explicitly with --target. |
| `SULTANC-TARGET-RUNTIME-PROVIDER-MISSING` | The target runtime lacks a service this program uses. Pick a target that provides it. |
| `SULTANC-TARGET-UNKNOWN` | The requested target is not registered. Run sultanc targets to list supported targets. |

## Backends

| Identity | Explanation |
| --- | --- |
| `SULTANC-BACKEND-RANGE-EXCEEDED` | A jump, offset or constant is too far or too large for the machine instruction. Split large functions. |
| `SULTANC-BACKEND-UNAVAILABLE` | Selected backend cannot emit this target. Check LLVM target support or use a supported backend. |
| `SULTANC-BACKEND-UNSUPPORTED-CONSTRUCT` | This backend cannot compile this construct yet. Try the other backend or rewrite the code. |

## Object files

| Identity | Explanation |
| --- | --- |
| `SULTANC-OBJECT-FORMAT-LIMIT-EXCEEDED` | The output is larger than its file format allows. Reduce code or data size, or split the program. |
| `SULTANC-OBJECT-FORMAT-UNSUPPORTED` | Object files of this format cannot be written for this target. Pick a supported target or backend. |
| `SULTANC-OBJECT-RELOCATION-UNSUPPORTED` | The output needs a relocation its object format cannot express. Try another backend or target. |

## Linking

| Identity | Explanation |
| --- | --- |
| `SULTANC-LINK-DUPLICATE-SYMBOL` | Two inputs define the same symbol. Remove one definition or make one private. |
| `SULTANC-LINK-ENTRY-SYMBOL-MISSING` | The output lacks the start symbol the target needs (such as `_start`); usually a startup object or runtime library is missing. |
| `SULTANC-LINK-EXTERNAL-SYMBOL-UNSUPPORTED` | The linker cannot bind this external symbol for the target. Provide it from a supported library. |
| `SULTANC-LINK-INPUT-UNSUPPORTED` | A link input is no object, archive, library or linker script the target's linker reads; a .tbd stub is read only for macOS and iOS. Remove it, or pass a library built for the target. |
| `SULTANC-LINK-LIBC-NOT-FOUND` | A C import links the C library on Linux, FreeBSD and Android. No library directory holds it, so the link goes on without it; C code that needs it then fails as an undefined symbol. Add its directory with -L. |
| `SULTANC-LINK-LIBRARY-NOT-FOUND` | A -l library was not found in the -L directories or the target's standard library directories. Add its directory with -L, or pass the library file itself. |
| `SULTANC-LINK-LLVM-LIBRARY-MISSING` | The output target needs its own libLLVM shared library. Supply a compatible target library. |
| `SULTANC-LINK-LLVM-SYMBOL-UNRESOLVED` | The selected libLLVM does not provide the required LLVM C API symbol. |
| `SULTANC-LINK-OUTPUT-UNSUPPORTED` | The linker cannot produce this output for the target. Choose another output kind or target. |
| `SULTANC-LINK-SYMBOL-TYPE-MISMATCH` | A symbol is defined as one kind, such as data, and used as another, such as a function. Make both agree. |
| `SULTANC-LINK-UNDEFINED-SYMBOL` | A used symbol is defined in no linked input. Link the library that defines it. |

## The command line

| Identity | Explanation |
| --- | --- |
| `SULTANC-CLI-CONFLICTING-MODES` | Two options request different compiler modes. Keep only one of them. |
| `SULTANC-CLI-ENV-VALUE-INVALID` | The compiler reads some settings from environment variables: SULTANC_COMPTIME_BUDGET is a positive decimal count of compile-time instructions, `_` separators allowed. Fix or unset the variable. |
| `SULTANC-CLI-INVALID-DIAGNOSTIC-COLOR` | The color policy must be auto, always or never. |
| `SULTANC-CLI-INVALID-DIAGNOSTIC-FORMAT` | The diagnostic format must be human, short or json. |
| `SULTANC-CLI-INVALID-DIAGNOSTIC-LANGUAGE` | The diagnostic language must be auto, ar or en. |
| `SULTANC-CLI-INVALID-INVOCATION` | The command line is not a valid compiler invocation. Run sultanc help to see the accepted form. |
| `SULTANC-CLI-MISSING-OPTION-VALUE` | This option needs a value. Write it after the option or after an equals sign. |
| `SULTANC-CLI-MISSING-SOURCE` | No source file was given. Pass the path of the main source file. |
| `SULTANC-CLI-MULTIPLE-PRIMARY-SOURCES` | Only one main source file is allowed. Remove the extra source path. |
| `SULTANC-CLI-UNEXPECTED-ARGUMENT` | This command does not take another argument here. Remove it or pass it with its option. |
| `SULTANC-CLI-UNKNOWN-DIAGNOSTIC-CODE` | No diagnostic has this code. Check the code printed in the error header. |
| `SULTANC-CLI-UNKNOWN-INPUT-TYPE` | The compiler does not know this input file type. Check the file extension. |
| `SULTANC-CLI-UNKNOWN-OPTION` | The compiler does not recognize this option. Check its spelling or run sultanc help. |

## Files the compiler reads

| Identity | Explanation |
| --- | --- |
| `SULTANC-IO-INPUT-NOT-FOUND` | An input file does not exist at the given path. Check the path. |
| `SULTANC-IO-OUTPUT-WRITE-FAILED` | The output file could not be written. Check the path, permissions and free disk space. |

## Machine-readable output

| Identity | Explanation |
| --- | --- |
| `SULTANC-JSON-DERIVE-UNSUPPORTED-FIELD-TYPE` | A `@@json` field must be an integer, a float, a boolean, a character, a string, a named type without type arguments, or a vector or option of these. |

## The standard library

| Identity | Explanation |
| --- | --- |
| `SULTANC-STD-NOT-FOUND` | The compiler looks for a library directory holding std/ in $SULTANC_STD, in ./library and next to itself, and found none. |

## The host system

| Identity | Explanation |
| --- | --- |
| `SULTANC-HOST-OUT-OF-MEMORY` | The compiler ran out of memory. Free memory or compile a smaller program. |

## Compiler memory

| Identity | Explanation |
| --- | --- |
| `SULTANC-MEMORY-INVALID-TYPE-CLASSIFICATION` | The compiler cannot decide how this type is stored and released. Check how the type owns its parts. |

## `sultanc run`

| Identity | Explanation |
| --- | --- |
| `SULTANC-RUN-LIBRARY-LOAD-FAILED` | `run` loads each -l library with the system's dynamic loader before the program starts; this one failed for the reason shown. Check that it is a shared library for this platform and that its dependencies are found. |
| `SULTANC-RUN-LIBRARY-NO-INSTALL-NAME` | On macOS a `.tbd` stub names the install name a dynamic library is loaded from. This stub names none, so there is nothing to load. Pass the `.dylib` itself. |
| `SULTANC-RUN-LIBRARY-STATIC-ONLY` | An interpreter cannot load a static archive: `run` and `interpret` load -l libraries into the running program. Build an executable, which links the archive, or provide the shared library. |
| `SULTANC-RUN-NEEDS-PROCESS` | This compiler runs under WASI, where it can neither build nor start a process. `run` interprets the program in place, std's WASI imports included, but imported C files, -l libraries and C functions the interpreter cannot call run only in an executable. Run the program with a native sultanc, or build it and run the executable yourself. |

## The interpreter

| Identity | Explanation |
| --- | --- |
| `SULTANC-INTERPRETER-INDEX-OUT-OF-BOUNDS` | The running program used an index outside the bounds of a collection. |
| `SULTANC-INTERPRETER-INVALID-RUNTIME-STATE` | The running program read or wrote storage in a state that does not allow it. |
| `SULTANC-INTERPRETER-INVALID-UTF8` | The running program produced text that is not valid UTF-8. |
| `SULTANC-INTERPRETER-RUNTIME-FAILURE` | The program failed while being interpreted. The reported reason names the failed operation. |
| `SULTANC-INTERPRETER-TRAP` | The program reached a trap and stopped. Check the condition that leads to it. |
| `SULTANC-INTERPRETER-UNSUPPORTED-EXTERNAL-CALL` | The interpreter cannot call this external function. Build a native executable to run it. |
| `SULTANC-INTERPRETER-UNSUPPORTED-MACHINE-PRIMITIVE` | The interpreter cannot run this machine-level operation. Build a native executable to run it. |

## A running program

| Identity | Explanation |
| --- | --- |
| `SULTANC-RUNTIME-PROGRAM-ERROR` | The program entry returned an error. Its description follows: the message, its notes and its causes. The exit status is 1. |
| `SULTANC-RUNTIME-PROGRAM-STOPPED` | The program called the stop primitive (panic) or an assertion failed. Owned values are not dropped. The exit status is 1. |

## A running program

| Identity | Explanation |
| --- | --- |
| `SULTANC-PANIC-HANDLER-INVALID` | A program has at most one `@@panic_handler` besides std's. It takes the message (`text`) and never returns: every path ends in `exit`, `panic` or an endless loop. |

## Internal compiler errors

An identity starting with `SULTANC-ICE-` means the compiler itself failed. It is never your mistake: please report it, with the program that caused it.

| Identity | Explanation |
| --- | --- |
| `SULTANC-ICE-COMPILER-FAILURE` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-CONTEXT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-INTERPRETER-CORRUPT-MEMORY` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-INTERPRETER-PRIMITIVE-ARGUMENT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-INTERPRETER-PRIMITIVE-ARGUMENT-COUNT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-INTERPRETER-UNEXPECTED-VALUE-KIND` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LEXER-FAILURE` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LOWERING-MISSING-CONSTRUCTOR` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LOWERING-MISSING-FUNCTION-FACT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LOWERING-MISSING-NAME-BINDING` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LOWERING-MISSING-TYPE-FACT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LOWERING-UNSUPPORTED-EXPRESSION` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-LOWERING-UNSUPPORTED-STATEMENT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-MISSING-SEMANTIC-FACT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-MODULE-GRAPH-REJECTED` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-MODULE-IMPORT-NOT-LOADED` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-MSIR-INVALID` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-PARSER-FAILURE` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-PARSER-NO-RESULT` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
| `SULTANC-ICE-TYPE-UNSUPPORTED-EXPRESSION` | Internal compiler failure. This is a compiler bug; please report it with the diagnostic code. |
