[العربية](../../ar/internals/msir-and-codegen.md) | **English**

# MSIR and Native Code Generation

MSIR is SultanC's target-neutral intermediate representation. The frontend decides source-language
semantics before lowering; the native backends, the LLVM provider and the interpreter consume
verified MSIR and never reinterpret the AST. This page covers the MSIR contract, how lowering and
the verifier bound it, and how each backend turns it into machine code and object files. It is
written for compiler contributors.

## Pipeline

```text
source
  |
  v
lexer / parser
  |
  v
module + name + type + pattern + safety analysis
  |
  v
semantic lowering
  |
  v
MSIR  -->  scalar promotion
  |
  v
MSIR verifier
  |
  +--------------------+
  |                    |
  v                    v
code generation      interpreter
  |
  v
instruction selection
  |
  v
register allocation / frame lowering
  |
  v
machine encoding
  |
  v
Mach-O, ELF or WebAssembly object
  |
  v
linker -> executable
```

The verifier is the boundary between semantic lowering and execution or code generation. An
optimization that changes MSIR must produce a program that verifies again before it reaches a
consumer.

## Verification boundary

The driver applies the boundary once (`compiler/driver/compiler_driver.sn`, `تحقق_برنامج_مسير`
right after lowering). `check`, the interpreter, and Native and LLVM code generation for objects,
executables, shared libraries, relocatable objects and target modules all consume only a program
that the canonical verifier accepted. There is no backend-specific verifier.

Several rules follow from sharing one verifier between consumers.

- A constructor pattern of a single-constructor type emits no tag test
  (`compiler/msir/lowering/patterns.sn`). The test would be always true, and its failure edge
  targets the arm body itself for an irrefutable arm, so payload bindings would not dominate
  their uses.
- A raw kernel call without an attested operation contract is valid but untrusted MSIR. Compiled
  code executes it as written and the interpreter refuses it at the call. Only a contracted call
  must carry exactly one proven memory descriptor per pointer operand
  (`compiler/msir/msir_verifier.sn`), so programs that build `execve` argv and envp arrays verify.
- No compiler primitive exposes facts of the compiling toolchain. The libLLVM that the running
  compiler uses is discovered by the LLVM provider (`compiler/backend/provider/llvm/toolchain.sn`)
  and recorded by the driver in the link plan together with the compiler's registered host
  target. The link applies it only to outputs for that host target
  (`compiler/link/dependency/llvm.sn`).
- Which functions an output defines is one MSIR policy shared by both backends
  (`compiler/msir/msir_consumer.sn`, `دوال_مسير_مطلوبة_لمخرج`). An executable keeps only what its
  entry and startup roots reach; objects, shared libraries, relocatable objects and target modules
  keep every function.

## What MSIR owns

MSIR owns the target-neutral execution model used after semantic analysis:

- typed SSA-style values;
- functions, blocks, block parameters and control flow;
- integer and floating-point operations;
- calls and external calls;
- places used for load and store semantics;
- function and global identities;
- constants and mutable module-level globals;
- runtime primitive identities;
- the operations the interpreter and native backends need.

MSIR carries canonical type IDs. It does not duplicate the frontend type system or target-specific
physical layout.

### Constants and globals

Immutable top-level scalar constants are substituted during lowering and need no writable runtime
storage. Mutable module-level values are MSIR globals with a canonical `GlobalID`, a type, an
initializer and a writability flag. Native output materializes their layout once using the target
data model, and backends refer to the same global identity. The object writers decide how that
identity is represented in Mach-O, ELF or WebAssembly.

### Pattern dispatch

A `match` whose arms are constructor patterns of the scrutinee's type, with or without payload
patterns and optionally ending in one irrefutable arm, lowers to one multiway branch on the
constructor tag (`compiler/msir/lowering/statements.sn`, as rustc switches on the discriminant
before testing sub-patterns). The branch enters the first arm of each tag. A failing payload test
continues with the next arm of the same tag and then with the final irrefutable arm or a trap.
Other matches keep ordered tests.

Each consumer handles the multiway branch its own way.

- Native x86-64 and ARM64 share one jump-table policy
  (`compiler/backend/codegen/selection_contract.sn`), taken from LLVM's switch lowering: at least
  4 cases, `cases * 100 >= range * 10`, an unsigned 32-bit selector, edges without block arguments,
  and a table indexed by `selector - lowest case` with an unsigned bounds check. Each architecture
  only encodes the plan; ARM64 compares through W17 when the last index exceeds the 12-bit CMP
  immediate. A switch the policy rejects keeps ordered comparisons, so dispatch is O(1) only for
  accepted switches. Tags need not be dense from zero.
- LLVM receives an LLVM `switch` and applies its own target lowering.
- The interpreter derives, once per switch block, a table (same density rule, offset by the lowest
  tag) or a sorted tag array. Selection is O(1) or O(log n) independent of arm order
  (`compiler/interpreter/execution/execution_state.sn`).

### Member reads through places

A read of a member of storage that has a place is lowered through that place. Storage with a place
is an owner local or parameter (owners live in storage, `drops.sn`), a local whose address was
taken, a global, a dereference or a raw-pointer element. The read goes through any chain of record
field and vector element projections (`x.a.b`, `v[i].f`, `(*r).f`) as `مكان_حقل` or `مكان_فهرس`
followed by one `تحميل` of the member (`جذر_مكان_قراءة_للخفض`,
`compiler/msir/lowering/expressions.sn`), as rustc reads through a place projection. Loading the
whole root and extracting the member would make every backend copy the whole aggregate and the
interpreter copy its member list. A local that is still an SSA value keeps the extraction, which
addresses the member in the value's storage without a copy. A use of the root local that SSF
recorded as a move still gives up the owner's value.

### Scalar promotion

Lowering keeps every local that lives across control flow in a place
(`statements.sn`, `ثبّت_أماكن_محليات_قبل_تفرع_التحكم`), so each use of a loop counter is a load and
each assignment a store. One pass, `compiler/msir/msir_promotion.sn` (`رقّ_محليات_برنامج_مسير`),
runs on lowered MSIR right before verification and promotes those places to SSA values, as LLVM's
mem2reg does (Cytron et al., TOPLAS 1991).

A local place is promoted when its type is an integer, a float or a bool, every use is a whole
load or store, and its loads, its stored values and the place have one exact type. Block
parameters are added at the iterated dominance frontier (Cooper-Harvey-Kennedy frontiers over
`حلل_هيمنة_مسير`) of the blocks that store it. One walk of the dominator tree replaces every load
by the reaching value, and parameters that nothing reads are removed (pruned SSA). A path on which
the local holds no value passes the type's zero, as LLVM passes undef. The place's debug variables
are bound to the stored values and the added parameters, so both backends still describe the local.

A place whose address is taken, a member place, an aggregate, or a place in a function whose entry
block has predecessors keeps its storage. The verifier then checks the promoted program like any
other. Renumbering lookups answer a dense map (entry i maps value i) by index
(`msir_renumbering.sn`), so the pass stays linear per function.

### Float primitives

The IEEE operations of [Type System](../reference/type-system.md) ("Float operations") are one
primitive, `هوية_بدائي_مترجم.عملية_عائمة`, lowered as rustc lowers `f64::sqrt` and friends.

| Operation | x86-64 native | ARM64 native | WebAssembly | LLVM |
|---|---|---|---|---|
| sqrt | SQRTSD / SQRTSS | FSQRT | f64.sqrt | llvm.sqrt |
| abs | ANDPD with ~sign | FABS | f64.abs | llvm.fabs |
| copysign | ANDPD/ANDNPD/ORPD | BIT with the sign mask | f64.copysign | llvm.copysign |
| minimum, maximum | MINSD/MAXSD, CMPSD masks and a+b for NaN (LLVM's FMINIMUM lowering) | FMIN / FMAX | f64.min / f64.max | llvm.minimum on AArch64, wasm32 and x86-64; compare/select elsewhere |
| floor, ceil, trunc, round-even | not provided | FRINTM/P/Z/N | f64.floor/ceil/trunc/nearest | llvm.floor/ceil/trunc/roundeven |
| fma | not provided | FMADD | not provided | llvm.fma |

Floats live in general registers in the native backends, so each operation moves its operands into
scratch vector registers (x86-64 XMM0..XMM5, ARM64 D0..D2, all caller-saved) and moves the result
back. Integral rounding is supported where the baseline instruction set has it (AArch64 and
WebAssembly), and fused multiply-add on AArch64, RISC-V, POWER, z/Architecture and LoongArch
(`يدعم_عملية_عائمة_للهدف`, `runtime_primitive_query_context.sn`). On the other targets std.math
uses exact bit-level forms, so LLVM never emits a libm call such as `floor` or `fma` that SultanC
executables do not link. The interpreter computes every operation on the raw bits
(`compiler/interpreter/operations/float_software.sn`): fdlibm's bit-by-bit square root, integral
rounding by masking the fraction, IEEE minimum and maximum, and SoftFloat's mulAdd for fma, which
give the hardware's bits. The verifier checks a float primitive's result against its first
argument's type.

### Division and remainder by a constant

When the divisor of a division or remainder is an MSIR integer constant (`ثابت_صحيح_لقيمة_مسير`,
`msir_consumer.sn`), the x86-64 and ARM64 selectors emit a multiply instead of DIV, UDIV or SDIV.
The plans follow LLVM's `UnsignedDivisionByConstantInfo` and `SignedDivisionByConstantInfo`
(Hacker's Delight, 2nd ed., chapter 10; Granlund and Montgomery, PLDI 1994) and are computed in
`compiler/backend/codegen/division_by_constant.sn`:

- unsigned values use `mulhu(x >> pre, magic)`, with the add-and-shift form when the magic needs
  65 bits;
- signed values use `mulhs(x, magic)` with the plus-or-minus x correction, an arithmetic shift and
  a sign-bit add;
- powers of two use shifts and masks.

Every width is computed on the 64-bit register after zero- or sign-extending the operand, because
the 64-bit quotient of the extended value is the N-bit quotient. A remainder is `x - q * d`. A zero
divisor, and -1 or 1 for signed values, keep the trapping divide, since MIN / -1 must still trap.
LLVM receives the constant and does the same itself at O2. The tests are
`tests/*/language/numeric/division_by_constant_*.sn`: every u8 and i8 divisor over every dividend,
u16 and i16 divisors over every dividend, and 32- and 64-bit edges plus pseudo-random dividends.

### Register allocation and vector length reads

When no register is free, the linear-scan allocator (`compiler/backend/codegen/register_allocator.sn`,
`خصص_السجلات_مع_انسكاب`) spills whichever of the new interval and the active, unfixed, unbound
intervals it could take over ends last (Poletto and Sarkar, TOPLAS 1999). A short-lived value
therefore does not go to the stack while a long-lived one holds the register.

A bounds check loads the whole vector to take its length. When that load has no other use and the
length call follows it, the native selectors read the length through the place
(`تحميلات_للطول_فقط_مسير`, `compiler/msir/msir_analysis.sn`) instead of copying the whole header to
the stack first.

## Target boundary

Target selection is separate from source-language semantics. A resolved target describes the
architecture and platform contracts that native code generation needs:

- architecture;
- operating system and environment;
- data model;
- ABI;
- object format;
- runtime provider;
- register rules;
- executable finalization.

The Native backend is registered for these target families.

| Target | Architecture | ABI | Object format |
| --- | --- | --- | --- |
| `arm64-darwin` | AArch64 | Darwin ARM64 | Mach-O |
| `x86_64-linux` | x86-64 | System V AMD64 | ELF |
| `wasm32-wasi`, `wasm32-browser` | wasm32 | portable SultanC ABI | WebAssembly relocatable object |

The target registry records support maturity; no backend infers it. The LLVM provider covers every
other registered target, and [Targets](../reference/targets.md) lists what each one supports.

## Backend responsibilities

Architecture backends own machine-specific code generation:

1. Instruction selection maps MSIR operations to architecture-level instructions.
2. Allocation constraints describe register classes, clobbers, fixed registers and operand
   requirements.
3. Register allocation and frame lowering assign physical locations and construct the final frame.
4. Encoding produces machine bytes and symbolic relocation requests.

Backends do not decide operating-system policy, object format, runtime selection or target
maturity. Those decisions belong to the target layer.

## ABI and layout

Physical layout and calling convention are distinct concerns. Layout answers questions about size
and alignment, field offsets and the representation of aggregate values. The ABI answers questions
about argument and return placement, call-preserved and call-clobbered registers, stack-call
requirements and architecture or platform calling-convention details. Code generation queries these
owners instead of keeping backend-local copies of their policy.

SultanC keeps its own internal ABI for calls between SultanC functions, std and runtime functions
(`حل_خطة_واجهة_دالة_للهدف`: the target's registered ABI identity, or the portable SultanC ABI on
delegated targets). On the native targets that identity is the platform's register convention
(AAPCS64-Darwin, SysV AMD64), the machine-level realization of SultanC's calling contract on that
architecture. Only declared external functions cross a foreign boundary. They are resolved by
`حل_خطة_واجهة_دالة_خارجية_للهدف` in `compiler/backend/target/target_abi_resolution.sn` and used by
Native and LLVM alike for external declarations, calls and WebAssembly imports.

Each foreign classifier file documents the specification it implements and the mature
implementation it was checked against.

| Targets | Classifier |
|---|---|
| arm64-darwin | `architectures/arm64/systems/macos/abi_rules.sn` (registered ABI) |
| x86_64-linux and delegated x86-64 Unix | `architectures/x86_64/systems/linux/abi_rules.sn` (SysV AMD64) |
| x86_64-windows-msvc | `architectures/x86_64/systems/windows/abi_rules.sn` (Microsoft x64) |
| aarch64 Linux, Android, FreeBSD, Windows | `architectures/arm64/systems/aapcs64/abi_rules.sn` (AAPCS64) |
| riscv64 (lp64d), riscv32 (ilp32) | `architectures/riscv/systems/elf/abi_rules.sn` |
| loongarch64 (lp64d) | `architectures/loongarch/systems/linux/abi_rules.sn` (RISC-V rules, LoongArch parameters) |
| powerpc64le | `architectures/powerpc64/systems/linux/abi_rules.sn` (ELFv2) |
| s390x | `architectures/systemz/systems/linux/abi_rules.sn` |
| mips64el | `architectures/mips64/systems/linux/abi_rules.sn` (n64) |
| sparcv9 | `architectures/sparc64/systems/linux/abi_rules.sn` (float members as Clang's CoerceBuilder; packed `inreg` parts with single floats) |
| ve | `architectures/ve/systems/linux/abi_rules.sn` |
| every other target | scalars and pointers only; structures are rejected explicitly |

A plan entry carries the register positions, the register part shapes of a value passed in
registers (`شكل_جزء_واجهة`: offset, size, integer or floating class), the extension of narrow
integers, whether the parts form one consecutive block, and whether an indirect argument points to
a caller-owned copy (always true at the foreign boundary). The LLVM provider spells these in IR in
one place (`compiler/backend/provider/llvm/abi_parts.sn`): float, double and iN parts, `[N x T]`
blocks, `zeroext` and `signext`, and copies for indirect arguments. The native backends read the
register positions and keep narrow integers normalized to their type in full registers, which meets
the extension requirement. The target owner also supplies the code-generation baseline that a
platform C ABI depends on (`أساس_توليد_شفرة_للهدف`: RV64GC/lp64d, RV32IMAC/ilp32, LoongArch64
lp64d), which the LLVM target machine applies.

SPARC V9 structures with float members split as Clang's `SparcV9ABIInfo` does. Each naturally
aligned float or double is a floating part at its offset and the bytes around them are integer
parts. With a single-precision member the plan marks the parts packed
(`تصنيف_قيمة_واجهة.أجزاء_مرصوصة`), which the LLVM provider spells as `inreg` on each part and on a
multi-part result (checked against Clang's sparcv9 code for the same C declarations). The native
backends pass every indirect aggregate argument as the address of a per-call copy in the outgoing
call area (the AArch64 encoder, the only native target whose C ABI passes structures by address), so
a foreign callee may write it. x86-64 SysV passes large structures by value on the stack.

### Release and drop glue

Release (`release`) of an affine box lowers in every backend to an ordinary call of the std release
provider bound by MSIR (`مزودات_تخصيص_الذاكرة.التحرير`), shared through `نداء_تحرير_مكيف_للهدف` in
`compiler/backend/target/runtime_primitive_query_context.sn`. Releases of other owned values, that
is the drops MSIR lowering schedules and explicit `release`, never reach the backends as such. The
ownership-glue pass (`compiler/msir/lowering/ownership_glue.sn`) rewrites each into the type's drop
glue, whose storage releases are calls of the same provider. See [Safety Flow](safety-flow.md).

### LLVM aggregate copies

The LLVM provider emits fixed-size aggregate copies as `llvm.memcpy.inline`
(`ابن_نسخ_ذاكرة_ثابتة_إل_إل_في_إم`, `compiler/backend/provider/llvm/values.sn`), because plain
`llvm.memcpy` becomes a call of libc `memcpy` above LLVM's inline threshold and SultanC executables
link no C library.

## The LLVM provider

`compiler/backend/provider/llvm` lowers verified MSIR to LLVM IR through the LLVM C API and runs
LLVM's optimization and code-generation pipelines.

The driver passes an optimization level, so the provider runs both the `default<O2>` pipeline and
the O2 code generator. At O2 LLVM promotes the allocas, hoists the vector data pointers and lengths
out of loops, removes the bounds checks it can prove, and turns divisions by constants into
multiplies. `--emit=llvm-ir` prints the optimized module before code generation, as
`rustc --emit=llvm-ir` does.

With the inliner running, every instruction inside a function that has a subprogram needs a debug
location, because LLVM keeps the callee's locations when a call site has none. Compiler-generated
instructions therefore take a line-0 location in the function's subprogram, as Clang's artificial
locations do (`debuginfo.sn`, `ابدأ_دالة_تصحيح_إل_إل_في_إم`).

At O2 the loop idiom recognizer turns copy and fill loops into `llvm.memcpy` and `llvm.memset`,
which become calls of libc `memcpy` and `memset`, and SultanC executables link no C library. Before
the passes, every defined function gets the `"no-builtins"` attribute (`optimization.sn`,
`علّم_دوال_مستقلة_إل_إل_في_إم`), as Clang does for `-ffreestanding`. After them, each remaining
constant-length `llvm.memcpy` or `llvm.memset` is switched to its `.inline` form
(`ضمن_نسخ_مضمن_إل_إل_في_إم`), which the code generator always expands inline.

The provider reports the kind of output it emitted, object or assembly. An object request for a
target whose declared machine output is assembly (NVPTX, XCore) fails with
`llvm.object-output-unsupported`; assembly is never reported or written as an object.

## Compiler working memory

The compiler releases its working data as it goes (see "Storage reclamation" in
[Safety Flow](safety-flow.md)), but allocation volume is still CPU time and peak memory, so
per-function working data is sized and reused deliberately.

- MSIR verification (`compiler/msir/msir_verifier.sn`) owns one value-definition index for the whole
  program, rebuilt in place per function with a logical length (`فهرس_تعريفات_قيم_مسير.العدد`).
- The x86-64 allocation problem (`compiler/backend/architectures/x86_64/allocation_constraints.sn`)
  builds the target's caller-saved register list once per function. Every call fact shares that
  read-only list, and liveness copies clobbers into its own interval lists. The spill
  scratch-register list is likewise built once per function in `allocated_code.sn`. CFG edge
  de-duplication scans only the current block's edges.
- The linear-scan worklists (`compiler/backend/codegen/register_allocator.sn`, `جدول_تخصيص_خطي`) are
  compressed arrays (offset plus flat) filled by a stable counting sort, the layout LLVM's
  linear-scan allocators use for start-ordered interval lists. Iteration order is the order of the
  start positions.
- MSIR lowering scope exits (`اقصر_محليات_حالة_خفض`, `compiler/msir/lowering/msir_context.sn`) make
  one private prefix copy of the locals table, and none when the table is already private and no
  longer than the prefix.

## Object emission

The object layer serializes already-selected machine code and data. It owns sections, symbols,
relocation encoding, object-format metadata, and Mach-O, ELF or WebAssembly serialization.
Architecture encoders emit relocation intent and the object writer maps that intent to the
format-specific relocation. For mutable globals, the shared native-output model provides
initialized bytes, alignment and the canonical global identity, and the Mach-O and ELF writers place
the data and create the required symbols without a second global model.

### ARM64 and Mach-O

The ARM64 path lowers symbolic addresses through ARM64 address materialization and Mach-O
relocations. Writable global data is emitted through the Mach-O data-section path and stays separate
from executable text.

### x86-64 and ELF

The x86-64 path distinguishes call relocations from ordinary PC-relative address materialization.
Calls use the PLT-oriented relocation path when required, and data and function addresses use
PC-relative address relocations. Writable global data is emitted in ELF data sections and resolved
into writable executable mappings by finalization.

LLVM at O2 folds a GOT load into its use (`sub rcx, [rip + foo@GOTPCREL]`) for symbols without
`dso_local`. The internal linker accepts `R_X86_64_REX_GOTPCRELX` and `R_X86_64_GOTPCRELX` on MOV and
on the ALU and TEST forms (`object_ingest.sn`). An executable link relaxes MOV to LEA and the ALU
forms to `binop $foo, %reg`, as GNU ld's `elf_x86_64_convert_load_reloc` does (`final_image.sn`). The
ALU rewrite uses opcode 81 /n or F7 /0, moves the register from ModRM.reg to ModRM.rm and REX.R to
REX.B, and takes the symbol's address as the immediate when it fits. A shared library rejects the ALU
forms.

### wasm32 and WebAssembly

WebAssembly has typed locals and structured control instead of registers and jumps, so
`compiler/backend/architectures/wasm32/` replaces the selection, allocation and encoding stages with
one lowering. Every MSIR value owns a local: a scalar, or the i32 address of aggregate storage in a
shadow-stack frame. The CFG is structured over the dominator tree (`control_flow.sn`): loop headers
become `loop` and merge nodes `block`, after Ramsey's "Beyond Relooper", and irreducible CFGs are
rejected. Calls follow the portable SultanC ABI plan from the target ABI owner.

Relocatable fields are 5-byte LEB128 slots recorded as relocation intent (`encoded_code.sn`).
`compiler/backend/object/wasm/wasm_object.sn` maps them to the tool-conventions relocations, symbols
and segments of a relocatable WebAssembly object, which SultanC's WebAssembly linker
(`compiler/link/wasm/linker.sn`) links into the final module. Storage services are the std
allocation providers bound in MSIR, and the WASI `_start` passes the entry's status to the std
process-exit provider. No C runtime is involved, and the LLVM provider follows the same contracts.

Debug information follows the same owners as the other native paths. The selector records the source
origin of every MSIR instruction's byte range and the local of every MSIR value (`encoded_code.sn`).
`native_output.sn` turns them into the machine debug context: line ranges, and value locations as the
`محلي_ويب_أسمبلي` kind of `backend/codegen/debug/model.sn`, serialized as `DW_OP_WASM_location`. The
shared DWARF producer builds the sections, and `wasm_object.sn` writes them as `.debug_*` custom
sections with section symbols and `reloc..debug_*` relocations: `R_WASM_FUNCTION_OFFSET_I32` for code
addresses (CODE-section offsets), `R_WASM_MEMORY_ADDR_I32` for data and `R_WASM_SECTION_OFFSET_I32`
between sections. No `.debug_frame` is written.

Shared modules (`--emit=shared`, WebAssembly DynamicLinking.md) need position-independent code. The
driver passes that fact to both backends (`compiler_driver.sn`, `مخرج_مستقل_الموضع_لربط`, and the
LLVM PIC relocation model). The native selector then addresses data and functions as
`global.get <base>; i32.const <offset>; i32.add` (`encoded_code.sn`, `قاعدة_موضعية`), and
`wasm_object.sn` turns each pair into `R_WASM_GLOBAL_INDEX_LEB` against `__memory_base`,
`__table_base` or the symbol's `GOT.mem` import, plus `R_WASM_MEMORY_ADDR_REL_SLEB` or
`R_WASM_TABLE_INDEX_REL_SLEB`. The WebAssembly linker (`link/wasm/linker.sn`) owns everything else:
the `dylink.0` section, GOT entries, the imported memory, table and bases, runtime data fixups in
`__wasm_apply_data_relocs`, and exports. Shared-module inputs are read by
`link/wasm/dynamic_ingest.sn`.

## Runtime calls

Runtime primitives are identified independently of their platform implementation. The target runtime
provider maps those identities to the implementation available for the selected target. This keeps
system services, memory operations, networking, synchronization and other runtime mechanisms out of
frontend semantics and out of bootstrap C.

## Interpreter

The interpreter executes verified MSIR. It shares the semantic program model of native code
generation and does not become an alternate parser, type or safety implementation. Host interaction
goes through the interpreter's runtime and host adapters, so platform-specific operating-system
behavior stays behind them and out of the MSIR operation dispatcher. See [Interpreter](interpreter.md).

## Extension rule

A new backend or execution engine begins at verified MSIR. It may provide its own instruction
selection or execution mechanism, but it consumes the same semantic decisions, type identities,
runtime contracts and verified control-flow model as the existing consumers.

## Not implemented

Variadic foreign calls and C function pointers called from SultanC have no syntax and are not
implemented.

## See also

- [Compiler Architecture](compiler-architecture.md)
- [Safety Flow](safety-flow.md)
- [Interpreter](interpreter.md)
- [Targets](../reference/targets.md)
- [Type System](../reference/type-system.md)
