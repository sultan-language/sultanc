# Embedded machine-code runtime retirement — static source audit

This records the **checkpoint-owned** runtime image that was removed from `compiler/**`. It does not assert that unresolved services have been implemented. No builds or tests were run.

## Previous behavior

- `compiler/backend/architectures/arm64/systems/macos/runtime_rules.sn`: AArch64 Darwin function images decoded from long hex text, with linker offset/relocation tables and exported STD symbols.
- `compiler/backend/architectures/x86_64/systems/linux/runtime_rules.sn`: x86-64 Linux function images decoded from hex, direct syscalls, in-image heap allocator, vector/text helpers and `_start`.
- `compiler/backend/target/native_output_capability.sn` patched calls into those runtime images. `compiler/backend/object/{elf,macho}` synthesized host runtime objects. `compiler/link/dependency/runtime.sn` injected them into the final link.

## Current compiler ownership

- Native and LLVM generate ordinary external relocation requests; ELF/Mach-O writers and the linker resolve normal definitions, with no runtime image injection.
- Linux `_start` is assembled as actual x86 instructions via `compiler/backend/architectures/x86_64/encoder.sn`. It calls the canonical selected language entry and then the STD process-exit provider, preserving return value and stack alignment. No OS syscall body or dedicated allocator state is emitted.
- The exit provider is a program fact, not a name-based link: MSIR lowering (`compiler/msir/semantic_lowering.sn`, `مزود_خروج_عملية_مسير`) records the unique public definition spelled `اسم_خروج_عملية_نظام_أساسي()` (std.os.process) in `برنامج_مسير.دالة_خروج_العملية`; the entry calls it by function identity. If the provider is not part of the program the native encoder fails with `مزود_خروج_عملية_مفقود` and the LLVM provider with `واجهة_ثنائية_غير_متاحة` (fail closed). Previously the entry referenced the name as an unresolved external symbol, which no object defined (functions are emitted as `sultanc_fn_*`).
- Programs do not import the provider themselves. `compiler/driver/compiler_driver.sn` (`مفاتيح_جذور_مطلوبة_لطلب`) adds the provider module as an additional frontend root whenever SultanC emits the process start itself (`بدء_عملية_ذاتي_لربط`: executables for targets linked by the internal ELF linker, and native objects for them). The module reference (`os/process.sn`) is owned next to the ABI spelling in `compiler/runtime/primitive_runtime_abi.sn` (`مسار_وحدة_خروج_عملية_نظام_أساسي`) and is resolved by `compiler/frontend/module_loading/import_resolution.sn` (`حل_وحدة_مكتبة_قياسية_موثوقة`) exactly like an import inside the trusted std root, so it shares the canonical ModuleKey. Without a trusted std root nothing is added and the backends fail closed as above.
- Executable reachability (`دوال_مسير_تنفيذية_مطلوبة`, entry + exit provider + function-valued globals) is a target-neutral MSIR query in `compiler/msir/msir_consumer.sn`, shared by the native provider and the LLVM provider (`compiler/backend/provider/llvm/llvm_codegen.sn`). Previously it was private to `compiler/backend/native_output.sn` and LLVM executables carried every function of every loaded module, including unreachable references to services the static image cannot resolve.
- LLVM executables for the internal ELF linker (no CRT) get their own `_start` (`compiler/backend/provider/llvm/entry.sn`, `أضف_بدء_عملية_لينكس_إل_إل_في_إم`, x86-64 and AArch64 module assembly). It runs `.init_array`, calls the CRT-style `main`, and passes the i32 status to the hidden LLVM function `sultanc.process_exit`, which calls the exit provider through its LLVM function record (hidden closure-environment parameter, integer width). WASI command modules are the same case: the LLVM provider renames the language entry to `sultanc.entry` and defines `void _start()` that calls it and passes the status to `sultanc.process_exit` (WASI `proc_exit`); a WASI `_start` that simply returned would always report status 0. Mach-O (dyld calls `main`) and PE (CRT entry) keep the CRT contract.
- wasm32 storage services are STD-owned: `library/std/os/allocation.sn` implements allocate/reallocate/free over linear memory using the compiler intrinsics `بدائي_نمو_الذاكرة_الخطية`, `بدائي_اقرأ_الذاكرة_الخطية`, `بدائي_اكتب_الذاكرة_الخطية`, `بدائي_انسخ_الذاكرة_الخطية` and `بدائي_قاعدة_الكومة_الخطية` (`compiler/runtime/primitive_identity.sn`; lowered only where `يدعم_ذاكرة_خطية_للهدف` holds). MSIR lowering binds the three public functions as `مزودات_تخصيص_الذاكرة` (names owned by `compiler/runtime/primitive_runtime_abi.sn`), `مزود_خدمات_ذاكرة_للهدف` reports them as std-provided so no C symbol is requested, and the driver loads the module for wasm32 objects and links. Previously wasm32 programs that allocated required wasi-libc's `libc.a`.
- wasm32 has a native machine path: target composition `ويب_أسمبلي_اثنان_وثلاثون_وحدة` enables the native provider (now the implicit default for wasm32-wasi and wasm32-browser). `compiler/backend/architectures/wasm32/` lowers verified MSIR to structured WebAssembly (dominator-tree structuring after Ramsey's "Beyond Relooper"; shadow-stack frames; the portable SultanC ABI), and `compiler/backend/object/wasm/wasm_object.sn` writes the relocatable object wasm-ld consumes. The WASI `_start` calls the language entry and passes its status to the std process-exit provider. The native wasm32 path emits no DWARF yet.
- Wasm executables, modules and shared modules are linked by `compiler/link/wasm/linker.sn` with `--stack-first -z stack-size=8388608`: wasm-ld's default is a 64 KiB shadow stack placed after the data segments, so deep call chains (the compiler built for WASI) silently overwrote static data. The stack now precedes data and an overflow traps. Relocatable outputs keep wasm-ld's defaults.
- A compiler built for a WASI host has no libLLVM: `compiler/backend/provider/llvm/llvm_codegen.sn` and `toolchain.sn` select, by target condition, an entry that reports `خلفية_هدف_غير_متاحة` and a toolchain fact of major version 0, which the driver records as no LLVM link dependency. The native backend and internal ELF/Mach-O linkers are unchanged and produce byte-identical output there.
- Compiler memory (SultanC has no automatic release, so peak memory is cumulative allocation; the WASI-hosted compiler trapped with `unreachable` on allocation failure at the 4 GiB wasm32 limit while compiling itself). `compiler/backend/architectures/arm64/allocated_code.sn` no longer copies the whole register-assignment table per instruction (`خصص_تعليمة_ذراع_بخطة_انسكاب`: instructions without a spill use the block plan directly; spilled ones use a per-instruction overlay of only the spill assignments) and appends into the block's output instead of returning per-instruction vectors. `compiler/msir/debug_contract.sn` stores the symbol→debug-variable table per function from the lowest registered symbol (`جدول_رموز_متغيرات_تصحيح`) instead of indexing from symbol 0. `compiler/link/macho/code_signature.sn` reuses one SHA-256 schedule per page and `compiler/backend/object/macho/macho_format.sn` sorts with two swapped buffers. Generated objects are byte-identical; the native self-compile peak fell from 11.7 GB to 6.6 GB RSS and the WASI-hosted self-compile to an arm64-darwin object fits in 3.2 GiB.
- x86-64 native vector append is expanded inline as the composite the link plan already declares (`إلحاق` = realloc service; see `runtime_primitive_query_context.sn`), matching the LLVM provider expansion. Previously it called the retired helper `تشغيل_إلحاق`, which had no definition anywhere. The AArch64 encoder (`compiler/backend/architectures/arm64/encoder.sn`) uses the same inline realloc expansion, so neither native encoder references the helper.
- Removed the uninstantiated `خدمة_نظام` allocation plan and its x86-64 inline syscall sequence; both native encoders now consume only ordinary symbolic allocation calls.
- `compiler/backend/target/runtime_primitive_query_context.sn` no longer considers Linux append/release storage dependencies locally provided by an embedded native runtime.
- Interpreter error classification now calls the existing `تشغيل_تصنيف_خطأ_نظام` standard-library interface rather than carrying another platform errno translation table. The interpreter still owns its semantic values, frames and verified-MSir evaluation, not an independent OS runtime.
- Real OS, filesystem, TLS, networking, resource management, vector backing allocation and text construction belong to `library/std/**`. The deleted function payloads are **not** replaced by any new compiler-private source implementation.
- `compiler/runtime/primitive_runtime_abi.sn` remains a shared **symbol identity** mapping, not an implementation. The interpreter already consumes these identities; its host-service execution implementation (`compiler/interpreter/execution/primitives.sn`) still needs unification after STD definitions exist.

## External dependencies that block full equivalence

- The checkpoint `library/std/os/runtime.sn` contains `عام خارجي عرف تشغيل_*` declarations without source definitions. The missing implementations must be created under the STD owner, which is read-only to this assignment.
- Target runtime service mapping (`compiler/backend/target/runtime_symbol_query_context.sn`) continues to name `malloc`, `realloc`, `free` and `exit` for allocation/process ABI. Replacing these with new symbols without an established STD ABI would invent a missing interface. This dependency remains unresolved.
- Existing primitive external calls such as `تشغيل_إلحاق` and `تشغيل_تحرير` need canonical storage/ownership implementations; they are not supplied by deleting their former runtime machine code. `library/std/text/from_bytes.sn` still declares `تشغيل_نص_من_بايتات` as external in this checkpoint, so that standard-library dependency also requires source-level replacement without recreating a compiler-private implementation.
- `compiler/backend/target/runtime_symbol_query_context.sn` still resolves allocation ABI services to platform-specific `malloc`, `realloc` and `free`. Their replacement requires a source-defined allocation ABI and a consistently linked STD provider; no such implementation or interface is present in the read-only STD checkpoint.
- The ELF delegated linker no longer automatically selects the configured default system library when it encounters unresolved symbols (`compiler/link/elf/external_linker.sn`). Explicitly configured dynamic libraries and toolchain search paths are preserved, but missing standard-library definitions must fail normal symbol resolution rather than implicitly linking libc.
- The internal ELF final-image writer also no longer adds a default libc `DT_NEEDED`, and the target/toolchain link metadata no longer carries an implicit system C library. Explicit dynamic libraries and per-target loader paths remain. Executables with unresolved symbols and no dynamic dependency now fail instead of emitting an unresolvable image. These source changes are statically inspected only.

## Original Darwin exported machine-code symbols

Each entry was an exported function at a fixed **byte offset** in the deleted ARM64 image. The symbol ABI is defined by `compiler/runtime/primitive_runtime_abi.sn` and, where present, the STD declaration. The callers are ordinary compiled `extern`/primitive invocations via object relocations. The replacement is a source-defined function in the listed owning subsystem, **not** an ABI-offset table.

| Symbol | Old image offset | Canonical area | Baseline STD status |
|---|---:|---|---|
| `تشغيل_فتح_ملف_للقراءة` | 1800 | Filesystem / IO | extern only |
| `تشغيل_قراءة_بايت_ملف` | 1920 | Filesystem / IO | No STD declaration |
| `تشغيل_قراءة_مقطع_ملف` | 2308 | Filesystem / IO | extern only |
| `تشغيل_إنشاء_ملف_للكتابة` | 1992 | Filesystem / IO | extern only |
| `تشغيل_كتابة_مقطع_ملف` | 2128 | Filesystem / IO | extern only |
| `تشغيل_إغلاق_ملف` | 2208 | Filesystem / IO | extern only |
| `تشغيل_فتح_دليل` | 2408 | Filesystem / IO | extern only |
| `تشغيل_قراءة_مدخل_دليل` | 2524 | Filesystem / IO | extern only |
| `تشغيل_إغلاق_دليل` | 2732 | Filesystem / IO | extern only |
| `تشغيل_قراءة_بايت_الدخل_القياسي` | 2240 | OS / std | No STD declaration |
| `تشغيل_قراءة_مقطع_الدخل_القياسي` | 2388 | Filesystem / IO | extern only |
| `تشغيل_كتابة_بايتات_تنفيذية` | 1192 | Filesystem / IO | extern only |
| `تشغيل_كتابة_الخرج_القياسي` | 1452 | Filesystem / IO | extern only |
| `تشغيل_كتابة_الخطأ_القياسي` | 1504 | Filesystem / IO | extern only |
| `تشغيل_عدد_الوسائط` | 1556 | OS / process | extern only |
| `تشغيل_الوصول_إلى_وسيط` | 1584 | OS / process | extern only |
| `تشغيل_خروج_العملية` | 1704 | OS / process | extern only |
| `تشغيل_تصنيف_خطأ_نظام` | 2868 | OS / std | extern only |
| `تشغيل_إنشاء_دليل` | 3056 | Filesystem / IO | extern only |
| `تشغيل_إزالة_ملف` | 3132 | Filesystem / IO | extern only |
| `تشغيل_إزالة_دليل` | 3200 | Filesystem / IO | extern only |
| `تشغيل_إعادة_تسمية` | 3268 | OS / std | extern only |
| `تشغيل_نوع_مسار` | 3604 | Filesystem / IO | extern only |
| `تشغيل_حجم_مسار` | 3612 | Filesystem / IO | extern only |
| `تشغيل_وقت_تعديل_مسار` | 3620 | Filesystem / IO | extern only |
| `تشغيل_فتح_مقبض_ملف_للقراءة` | 3628 | Filesystem / IO | extern only |
| `تشغيل_إنشاء_مقبض_ملف_للكتابة` | 3748 | Filesystem / IO | extern only |
| `تشغيل_قراءة_مقطع_مقبض_ملف` | 3884 | Filesystem / IO | extern only |
| `تشغيل_كتابة_مقطع_مقبض_ملف` | 3964 | Filesystem / IO | extern only |
| `تشغيل_إغلاق_مقبض_ملف` | 4044 | Filesystem / IO | extern only |
| `تشغيل_تحريك_مقبض_ملف` | 4116 | Filesystem / IO | extern only |
| `تشغيل_موضع_مقبض_ملف` | 4152 | Filesystem / IO | No STD declaration |
| `تشغيل_مزامنة_مقبض_ملف` | 4196 | Filesystem / IO | extern only |
| `تشغيل_وقت_رتيب_نانو` | 4332 | OS / process | extern only |
| `تشغيل_وقت_نظام_نانو` | 4340 | OS / process | extern only |
| `تشغيل_نوم_نانو` | 4348 | OS / process | extern only |
| `تشغيل_طول_متغير_بيئة` | 4436 | OS / process | extern only |
| `تشغيل_قراءة_متغير_بيئة` | 4508 | OS / process | extern only |
| `تشغيل_دليل_العمل` | 4676 | Filesystem / IO | extern only |
| `تشغيل_الدليل_المؤقت` | 4776 | Filesystem / IO | extern only |
| `تشغيل_تشغيل_عملية` | 4940 | OS / process | extern only |
| `تشغيل_انتظار_عملية` | 5296 | OS / process | extern only |
| `تشغيل_حجم_صفحة_النظام` | 5420 | OS / std | extern only |
| `تشغيل_خريطة_ذاكرة_النظام` | 5440 | Memory / OS | extern only |
| `تشغيل_إلغاء_خريطة_ذاكرة_النظام` | 5484 | Memory / OS | extern only |
| `تشغيل_حماية_ذاكرة_النظام` | 5504 | Memory / OS | extern only |
| `تشغيل_قفل_ذاكرة_النظام` | 5528 | Memory / OS | extern only |
| `تشغيل_إلغاء_قفل_ذاكرة_النظام` | 5548 | Memory / OS | extern only |
| `تشغيل_نصيحة_ذاكرة_النظام` | 5568 | Memory / OS | extern only |
| `تشغيل_عشوائية_آمنة` | 5608 | OS / process | No STD declaration |
| `تشغيل_إنشاء_مقبس` | 5688 | Socket / OS | extern only |
| `تشغيل_اتصال_مقبس` | 5708 | Socket / OS | extern only |
| `تشغيل_ربط_مقبس` | 5760 | Socket / OS | extern only |
| `تشغيل_استماع_مقبس` | 5812 | Socket / OS | extern only |
| `تشغيل_قبول_مقبس` | 5832 | Socket / OS | extern only |
| `تشغيل_إرسال_مقبس` | 5860 | Socket / OS | extern only |
| `تشغيل_استقبال_مقبس` | 5940 | Socket / OS | extern only |
| `تشغيل_إيقاف_مقبس` | 6020 | Socket / OS | extern only |
| `تشغيل_إغلاق_مقبس` | 6040 | Socket / OS | extern only |
| `تشغيل_ترميز_عنوان_مقبس` | 7524 | Socket / OS | extern only |
| `تشغيل_ضبط_مقبس_غير_حاصر` | 7740 | Socket / OS | extern only |
| `تشغيل_ضبط_خيار_مقبس` | 7828 | Socket / OS | extern only |
| `تشغيل_إنشاء_جلسة_تي_إل_إس` | 8012 | TLS / network / concurrency | No STD declaration |
| `تشغيل_ضبط_اسم_نظير_تي_إل_إس` | 10148 | TLS / network / concurrency | No STD declaration |
| `تشغيل_ضبط_تحقق_تي_إل_إس` | 10484 | TLS / network / concurrency | No STD declaration |
| `تشغيل_ضبط_هوية_خادم_تي_إل_إس` | 12468 | TLS / network / concurrency | No STD declaration |
| `تشغيل_ضبط_جذور_ثقة_تي_إل_إس` | 15712 | TLS / network / concurrency | No STD declaration |
| `تشغيل_مصافحة_تي_إل_إس` | 10804 | TLS / network / concurrency | No STD declaration |
| `تشغيل_قراءة_تي_إل_إس` | 11160 | TLS / network / concurrency | No STD declaration |
| `تشغيل_كتابة_تي_إل_إس` | 11640 | TLS / network / concurrency | No STD declaration |
| `تشغيل_إغلاق_جلسة_تي_إل_إس` | 12128 | TLS / network / concurrency | No STD declaration |
| `تشغيل_إنشاء_حلقة_أحداث` | 6060 | TLS / network / concurrency | No STD declaration |
| `تشغيل_تحكم_حلقة_أحداث` | 6080 | TLS / network / concurrency | No STD declaration |
| `تشغيل_انتظار_حلقة_أحداث` | 6320 | TLS / network / concurrency | No STD declaration |
| `تشغيل_إغلاق_حلقة_أحداث` | 6508 | TLS / network / concurrency | No STD declaration |
| `تشغيل_حل_اسم_مضيف` | 6548 | TLS / network / concurrency | extern only |
| `تشغيل_إنشاء_خيط` | 6960 | TLS / network / concurrency | No STD declaration |
| `تشغيل_انتظار_خيط` | 7020 | TLS / network / concurrency | No STD declaration |
| `تشغيل_إنشاء_قفل_تبادلي` | 7068 | TLS / network / concurrency | No STD declaration |
| `تشغيل_تدمير_قفل_تبادلي` | 7160 | TLS / network / concurrency | No STD declaration |
| `تشغيل_قفل_تبادلي` | 7216 | TLS / network / concurrency | No STD declaration |
| `تشغيل_فتح_قفل_تبادلي` | 7248 | TLS / network / concurrency | No STD declaration |
| `تشغيل_إنشاء_شرط` | 7280 | TLS / network / concurrency | No STD declaration |
| `تشغيل_تدمير_شرط` | 7372 | TLS / network / concurrency | No STD declaration |
| `تشغيل_انتظار_شرط` | 7428 | TLS / network / concurrency | No STD declaration |
| `تشغيل_إشارة_شرط` | 7460 | TLS / network / concurrency | No STD declaration |
| `تشغيل_بث_شرط` | 7492 | TLS / network / concurrency | No STD declaration |
| `تشغيل_نص_من_بايتات` | 17512 | Text / bytes | No STD declaration |

## Classification and ABI boundaries

- **Instruction encoding / entry ABI:** compiler-owned and emitted from semantic operations and machine encoders, not hex image blobs. Only Linux process entry was formerly embedded in these sources; Darwin delegates entry to the existing platform linking/startup policy.
- **Memory allocation / vector growth / text materialization:** actual runtime storage ownership. Required STD definitions and generic lowering are not yet proven; callers and their old ABI identity are retained instead of inventing replacements.
- **OS, networking, DNS, TLS, threads, clocks, environment and filesystem:** all belong to target-aware standard library source. Original image called Linux syscalls or Darwin host facilities and applied platform ABI conversions; those implementation details are dependencies of the STD platform owners, not machine instruction templates for the compiler.
- **Relocations, symbol names, object writing, linkage:** compiler-owned; canonical ELF and Mach-O paths remain active. The numeric encoding constants used by these formats are legitimate and retained.

Original Darwin exports indexed: 88; without a confirmed source-defined implementation in this checkpoint: 88.

This is a **partial migration checkpoint** until the external STD implementations and primitive semantics are in the supplied baseline. Do not claim linked executables work from this static audit.

## Source dependencies outside compiler ownership

- `library/std/os/runtime.sn`: the exported `تشغيل_*` host-ABI functions (including file, directory, process, exit, socket, clock, mapping and error classification) remain external declarations rather than source definitions. They need OS-specific source providers with a canonical, resolved ABI.
- `library/std/text/from_bytes.sn`: replace its remaining obsolete `تشغيل_نص_من_بايتات` extern with the STD-owned text constructor, preserving `من_بايتات`.
- Resolved: append is expanded inline (realloc provider) and release of a box calls the std release provider (`حرر_ذاكرة_التشغيل`) in every backend (`نداء_تحرير_مكيف_للهدف`, `compiler/backend/target/runtime_primitive_query_context.sn`); neither `تشغيل_إلحاق` nor `تشغيل_تحرير` is referenced by generated code. No compiler-private allocator or libc shim has been added.
- `compiler/interpreter/execution/primitives.sn` still has unsupported exports for services not implemented in STD. They remain explicit interpreter failures, never silent fake successes.

Only static inspection was authorized. Builds, tests and convergence were not run.
