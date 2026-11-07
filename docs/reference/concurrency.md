# Concurrency

SultanC follows Rust's model of threads, atomics and shared state (`std::thread`, `std::sync`,
`std::sync::atomic`, `std::sync::mpsc`) and takes Go's runtime as the reference for the OS layer
(thread start, futex and semaphore). The design is adapted to SultanC's ownership system, which
has no traits and runs no user destructors. This page describes each primitive, the OS layer
underneath, the compile-time check on what a thread may capture, and the current limits. It is
written for programmers who share data between threads and for those who work on `std.sync`.

## Modules

| Module | Rust / Go | Contents |
|---|---|---|
| `std.sync.atomic` | `std::sync::atomic`; `sync/atomic` | atomic integers and memory orderings |
| `std.thread.thread` | `std::thread` | `أطلق` / `spawn`, `انضم` / `join`, values from threads, sleep, yield |
| `std.os.wait_address` | `sys::futex`; `runtime.futex`, `sema` | sleep while a 32-bit cell holds a value; wake one or all |
| `std.sync.arc` | `Arc` | shared ownership with an atomic count |
| `std.sync.mutex` | `Mutex`; `sync.Mutex` | three-state futex mutex, closure-scoped |
| `std.sync.rwlock` | `RwLock`; `sync.RWMutex` | readers or one writer |
| `std.sync.condvar` | `Condvar`; `sync.Cond` | wait and notify on a sequence word |
| `std.sync.once` | `Once`, `OnceLock`; `sync.Once` | one-time initialization |
| `std.sync.channel` | `mpsc`; Go channels | unbounded and bounded channels |

## A first program

A counter behind a mutex, shared through an `Arc`, incremented by four threads. Each thread owns
a clone of the `Arc`; the lock is held only inside the closure given to `with_lock`.

```sultan
import std.sync.arc;
import std.sync.mutex;
import std.thread.thread;
import std.fmt.fmt;

def bump(v: &mutable i64): void { *v := *v + 1; }
def read(v: &mutable i64): i64 { return *v; }

def add_thousand(m: &arc<mutex<i64>>): void {
    let i: mutable i64 = 0;
    while (i < 1000) {
        with_lock(arc_get(m), fn(v: &mutable i64): void -> bump(v));
        i := i + 1;
    }
}

def main(): i32 {
    let counter: arc<mutex<i64>> = arc_new(mutex_new(0 as i64));
    let handles: mutable vector join_handle = [];
    let t: mutable i32 = 0;
    while (t < 4) {
        let mine: arc<mutex<i64>> = arc_clone(&counter);
        match spawn(fn(): void -> add_thousand(&mine)) with
        | result.ok(h) -> { append(handles, h); }
        | result.err(_) -> { return 1; };
        t := t + 1;
    }
    let j: mutable uint = 0;
    while (j < (length(handles) as uint)) {
        join(&mutable handles[j]);
        j := j + 1;
    }
    let total: i64 = with_lock(arc_get(&counter), fn(v: &mutable i64): i64 -> read(v));
    let r: result uint, fmt_error = println_fmt("total = {}", [fmt_arg.i64(total)]);
    return 0;
}
```

```sultan
استورد std.sync.arc;
استورد std.sync.mutex;
استورد std.thread.thread;
استورد المكتبة.تنسيق.fmt;

عرف زد(ق: &متغير ص64): فراغ { *ق := *ق + 1; }
عرف اقرأ(ق: &متغير ص64): ص64 { أرجع *ق; }

عرف أضف_ألفا(ق: &مشترك<قفل<ص64>>): فراغ {
    دع ف: متغير ص64 = 0;
    طالما (ف < 1000) {
        نفذ_مع_القفل(اقرأ_مشترك(ق), دالة(س: &متغير ص64): فراغ -> زد(س));
        ف := ف + 1;
    }
}

عرف الرئيسية(): ص32 {
    دع العداد: مشترك<قفل<ص64>> = أنشئ_مشترك(أنشئ_قفل(0 مثل ص64));
    دع المقابض: متغير متجه مقبض_خيط = [];
    دع خ: متغير ص32 = 0;
    طالما (خ < 4) {
        دع نسختي: مشترك<قفل<ص64>> = استنسخ_مشترك(&العداد);
        طابق أطلق(دالة(): فراغ -> أضف_ألفا(&نسختي)) مع
        | نتيجة.نجح(م) -> { ألحق(المقابض, م); }
        | نتيجة.خطأ(_) -> { أرجع 1; };
        خ := خ + 1;
    }
    دع ض: متغير طبيعي = 0;
    طالما (ض < (الطول(المقابض) مثل طبيعي)) {
        انضم(&متغير المقابض[ض]);
        ض := ض + 1;
    }
    دع المجموع: ص64 = نفذ_مع_القفل(اقرأ_مشترك(&العداد), دالة(س: &متغير ص64): ص64 -> اقرأ(س));
    دع ر: نتيجة طبيعي, خطأ_تنسيق = اطبع_سطرا_منسقا(«المجموع = {}», [قيمة_تنسيق.ص64(المجموع)]);
    أرجع 0;
}
```

Both programs print the total, 4000.

## Atomics

`std.sync.atomic` provides atomic integers (`ذرة_ط64` / `atomic_u64`, `ذرة_ط32` / `atomic_u32`,
`ذرة_طبيعي` / `atomic_usize`) with load, store, swap, compare-exchange, fetch-add and a memory
fence. `atomic_u64` also has fetch-and and fetch-or, and has pointer forms that take a `*متغير`
cell. Orderings follow Rust's `Ordering`:

| English | Arabic |
|---|---|
| `memory_ordering.relaxed` | `ترتيب_ذاكرة.مسترخ` |
| `memory_ordering.release` | `ترتيب_ذاكرة.إطلاق` |
| `memory_ordering.acquire` | `ترتيب_ذاكرة.اكتساب` |
| `memory_ordering.acq_rel` | `ترتيب_ذاكرة.اكتساب_وإطلاق` |
| `memory_ordering.seq_cst` | `ترتيب_ذاكرة.تسلسلي` |

The operations are compiler intrinsics: `بدائي_ذري_حمل`, `بدائي_ذري_خزن`, `بدائي_ذري_تبديل`,
`بدائي_ذري_قارن_وبدل`, `بدائي_ذري_جلب_جمع`, the bitwise `بدائي_ذري_جلب_و`, `بدائي_ذري_جلب_أو` and
`بدائي_ذري_جلب_أو_حصري` (fetch-xor), and `بدائي_سياج_ذاكرة` (fence). Unlike every other operation,
they read and write the integer that a shared reference refers to. This is the one sound point of
interior mutability, as in Rust, where atomics take `&self`. The frontend accepts a shared `&` to a
32-bit, 64-bit or pointer-sized integer as the first operand.

Every backend lowers the intrinsics to the architecture's atomic instructions at
sequentially-consistent strength. The requested ordering is carried through, but the backends treat
every ordering as `seq_cst`, which is always sound.

| Backend | Lowering |
|---|---|
| x86_64 (native and LLVM) | `LOCK XADD`, `XCHG`, `LOCK CMPXCHG` (and/or/xor as a `CMPXCHG` loop), `MFENCE` |
| arm64 (LLVM; native Darwin) | the ARMv8.1 LSE forms `LDADDAL`, `SWPAL`, `LDCLRAL`, `LDSETAL`, `LDEORAL`, `CASAL`, `LDAR`, `STLR`, and `DMB ISH` |
| wasm32 | plain linear-memory loads and stores: a wasm32 module runs one thread (WASI threads are not used); `fence` is a no-op |
| LLVM | `atomicrmw`, `cmpxchg` and `fence` at `seq_cst` |
| interpreter | one OS thread, so an atomic is the plain read-modify-write of the cell its reference names; the write goes through even when the reference is shared (`compiler/interpreter/execution/primitives.sn`) |

## OS threads

`std.thread` provides:

| English | Arabic | Rust |
|---|---|---|
| `spawn` | `أطلق` | `thread::spawn`: move a closure to a new thread, return a join handle |
| `join` | `انضم` | `JoinHandle::join` |
| `spawn_value`, `join_value` | `أطلق_بقيمة`, `انضم_بقيمة` | a thread whose closure returns a value, which `join_value` answers as an `option` |
| `sleep` | `نم` | `thread::sleep` (seconds and nanoseconds) |
| `yield_now` | `تنازل` | `thread::yield_now` |
| `current_id`, `join_handle_id` | `معرف_الخيط_الحالي`, `معرف_المقبض` | the OS thread id |
| `available_parallelism` | `عدد_المعالجات_المتاحة` | `thread::available_parallelism` |

`spawn` returns a `result` whose error is the platform's failure to start a thread.

### Platform support

`std.os.threads` has one guarded branch per OS. Each OS starts a thread with the mechanism its own
runtime uses.

| OS | Start | Join |
|---|---|---|
| Linux, Android (every architecture) | `clone` onto a stack std maps, on raw system calls with no libc | futex wait on the tid cell the kernel clears at exit |
| FreeBSD | `thr_new` onto a stack std maps | `_umtx_op` wait on `thr_exit`'s state word |
| macOS | `pthread_create` | `pthread_join` |
| Windows | `CreateThread` | `WaitForSingleObject`, `GetExitCodeThread`, `CloseHandle` |
| WASI, freestanding, the interpreter | not supported: `spawn` returns an error | |

### The Linux thread layer

The Linux layer (`library/std/os/platform/unix/linux/threads.sn`) starts a thread the way Go's
runtime and musl do. A thread gets a 2 MiB `mmap`'d stack with a `PROT_NONE` guard page, and a
`clone` with `CLONE_VM|FS|FILES|SIGHAND|THREAD|SYSVSEM|PARENT_SETTID|CHILD_CLEARTID`. The clone is
the compiler intrinsic `بدائي_استنسخ_خيط_نواة`: its child runs the closure on the new stack and
exits the thread. `join` waits on the tid cell that the kernel clears when the thread exits.

Two details differ between architectures. The tid cell goes into both of clone's last two argument
slots, because they are (child_tid, tls) on x86_64 and (tls, child_tid) on the asm-generic
architectures (aarch64, riscv64, loongarch64); the tls slot is ignored without `CLONE_SETTLS`.
`join` tests the whole word, because on big-endian machines (s390x, sparc64) the kernel's 32-bit
tid sits in the high half.

The stack mapping is shared with the FreeBSD layer: `mmap`, `mprotect` and `munmap` take the same
arguments there, and the flags come from the selected kernel's UAPI.

### Ownership of the closure

A thread owns its closure, as Rust's `FnOnce` is moved into a thread. The closure goes into a heap
cell that the thread's entry closure owns. The entry swaps the closure out, runs it and drops it
when it returns, so whatever the closure captured (an `Arc` clone, a channel's sender) is released
when the thread finishes, not when or whether it is joined. `join` returns the entry closure's last
reference, rebuilt from its address with `بدائي_دالة_من_عنوان`, which frees the entry and its cell.
A detached thread leaks only the entry (two small blocks).

### Results and panics

A closure given to `spawn` returns no value through `join`; `spawn_value` and `join_value` hand
a value back through shared state. A `إنهيار` / panic in any thread ends the whole process
(panic=abort), so `join` never observes a thread error and no lock can be poisoned.

## Waiting and waking

`std.os.wait_address` is the one blocking primitive that every `std.sync` type is built on, after
Rust's `sys/sync/futex` and Go's `runtime/sema`.

| Call | Meaning |
|---|---|
| `انتظر_عند_العنوان(&ذرة_ط32, المتوقعة, المهلة)` / `wait_on_address` | sleep while the cell holds `المتوقعة`; answers `خطأ` only when the relative timeout (nanoseconds, or none) passed |
| `أيقظ_واحدا_عند_العنوان` / `wake_by_address_single`, `أيقظ_الكل_عند_العنوان` / `wake_by_address_all` | wake one or every sleeper on the cell |
| `انتظر_عند_الخلية`, `أيقظ_واحدا_عند_الخلية` / `wait_on_cell`, `wake_one_on_cell` | the same on a bare 32-bit cell, such as a scalar module variable used as a lock word |
| `هل_الانتظار_عند_العنوان_مدعوم` / `wait_on_address_supported` | whether the target has a blocking wait |

The check of the value and the sleep are one kernel step, so a wake between a caller's load and
its wait is never lost. A wait may return spuriously, so callers loop on their condition.

| OS | Wait | Wake | Notes |
|---|---|---|---|
| Linux, Android | `futex(FUTEX_WAIT_PRIVATE)` | `futex(FUTEX_WAKE_PRIVATE)` | timed waits use `futex_time64` on i686 and armv7 (`مستقبل_بمهلة`), so the timespec is 64-bit everywhere; ETIMEDOUT is 110 (mips 145, sparc 60) |
| FreeBSD | `_umtx_op(UMTX_OP_WAIT_UINT_PRIVATE)` | `_umtx_op(UMTX_OP_WAKE_PRIVATE)` | the timeout is a relative `struct timespec` in `uaddr2` (`مستقبل_بمهلة_بي_إس_دي`) |
| macOS | `__ulock_wait2(UL_COMPARE_AND_WAIT \| ULF_NO_ERRNO)` | `__ulock_wake` (`ULF_WAKE_ALL`) | what Rust uses before macOS 14.4; `os_sync_wait_on_address` is not in the 11.3 SDK. Imported from libSystem and checked by `tools/audit/macos_imports_audit.py` |
| Windows | `WaitOnAddress` | `WakeByAddressSingle` / `WakeByAddressAll` | `api-ms-win-core-synch-l1-2-0.dll`, Rust's `synchronization` import; ERROR_TIMEOUT is the timeout |
| WASI, freestanding | returns at once (a timed wait reports the timeout) | nothing | one thread: nobody could change the cell |

The kernel calls carry memory contracts (`توقيع_عملية_نواة_لينكس`: the 4-byte cell, the 16-byte
timespec), as does every std kernel call that takes a pointer.

## Shared ownership: Arc

`مشترك<ق>` / `arc<T>` is Rust's `Arc<T>`: one heap value, any number of handles, an atomic
reference count, and the value dropped exactly once with the last handle.

SultanC runs no user destructors, so the count and the drop belong to a closure environment. A
handle holds one closure whose environment owns the boxed value (a move capture). The
environment's reference count is the strong count: closures count atomically (`LOCK XADD`,
`LDADDAL`, LLVM `atomicrmw`), copying a handle adds a reference, dropping one removes it, and the
environment's destructor drops the box with the last reference. The closure answers the boxed
value's address.

| English | Arabic | Rust |
|---|---|---|
| `arc_new` | `أنشئ_مشترك` | `Arc::new` |
| `arc_clone` | `استنسخ_مشترك` | `Arc::clone`; copying a handle is the same clone, as closures are copyable values |
| `arc_get` | `اقرأ_مشترك` | `Deref`: a shared reference |
| `arc_strong_count` | `عدد_مراجع_مشترك` | `Arc::strong_count`; reads the environment's count slot |
| `arc_ptr_eq` | `هل_المشترك_نفسه` | `Arc::ptr_eq` |
| `arc_as_ptr` | `مؤشر_مشترك` | `Arc::as_ptr` |

The value is reached only through a shared reference. Mutation across threads goes through a
mutex, an rwlock or an atomic inside the `Arc`. The interpreter counts no closure references, so
an `Arc` there is never freed and `arc_strong_count` is unavailable (the closure-address
intrinsics are refused).

## Mutex

`قفل<ق>` / `mutex<T>` is Rust's futex mutex (`sys/sync/mutex/futex.rs`, after Ulrich Drepper's
"Futexes Are Tricky"). The state is 0 unlocked, 1 locked, or 2 locked with waiters. An uncontended
lock is one compare-exchange and an uncontended unlock one swap, with no system call. A contended
lock spins 100 rounds, marks the state 2 and sleeps; an unlock that finds 2 wakes one sleeper.

The value is reached only inside a closure that runs with the lock held:
`نفذ_مع_القفل(&م, دالة(&متغير ق): ن): ن` / `with_lock`, or `حاول_القفل` / `try_lock`, which answers
`none` when another thread holds the lock. This is Rust's `MutexGuard` scoped to a call, because
SultanC runs nothing at scope exit: the unlock cannot be forgotten and no reference to the value
outlives the lock. `هل_القفل_مأخوذ` / `mutex_is_locked` reports the state.

There is no poisoning, because a panic ends the process. A mutex moves but is never copied (a
never-allocated vector marker makes it an owned value), and it is shared through an `Arc`
(`مشترك<قفل<ق>>`), as in Rust.

## RwLock

`قفل_قراءة_كتابة<ق>` / `rwlock<T>` is one futex word: 0 when free, n for n readers, or `0xFFFFFFFF`
for a writer. Access is through `نفذ_مع_قراءة` / `with_read` and `نفذ_مع_كتابة` / `with_write`. It
is simpler than Rust's futex RwLock: there is no writer preference, so a steady stream of readers
can delay a writer.

## Condvar

`شرط_انتظار` / `condvar` is Rust's futex condvar: a 32-bit sequence number. `انتظر_الشرط(&ش, &م)`
/ `condvar_wait` reads the number, releases the mutex, sleeps while the number is unchanged and
takes the mutex again. A notify increments the number and wakes one sleeper (`نبه_واحدا`) or all
(`نبه_الكل`).

A wait is called with the mutex held, inside the closure of `نفذ_مع_القفل` on the same mutex. The
closure's reference to the value stays valid, because the lock is held again when the wait
returns. `انتظر_الشرط_بمهلة` / `condvar_wait_timeout` answers whether the wait timed out;
`انتظر_طالما` / `condvar_wait_while` loops on a condition over the guarded value.

## Once and OnceLock

`مرة` / `once` is Rust's futex `Once`: 0 incomplete, 1 running, 2 running with sleepers, 3
complete. `نفذ_مرة` / `call_once` runs its closure exactly once however many threads race, and the
others sleep until it completes. A completed `Once` costs one acquire load.

`خلية_مرة<ق>` / `once_lock<T>` stores its value in a vector of at most one element behind an
`Arc`. An element is a place, so `اقرأ_خلية_مرة` / `once_lock_get` and `اقرأ_أو_هيئ_خلية_مرة` /
`once_lock_get_or_init` lend it in place, and the vector never grows after the one append, so its
address stays put. `اضبط_خلية_مرة` / `once_lock_set` refuses a second value.

## Channels

`قناة<ق>()` / `channel()` and `قناة_متزامنة<ق>(الحد)` / `sync_channel(bound)` return both ends
(`طرفا_قناة` / `channel_ends`), which a pattern takes apart:
`| { المرسل: م, المستلم: س } -> …`. `مستلم_قناة` / `channel_receiver` and `مرسل_لمستلم` /
`sender_for` make the ends one at a time. A channel has many senders (`استنسخ_مرسل` /
`sender_clone`) and one receiver, and delivers in FIFO order per sender.

| Operation | English | Arabic |
|---|---|---|
| Send, waiting for room in a bounded channel | `send` | `أرسل` |
| Send without waiting | `try_send` | `حاول_الإرسال` |
| Receive, waiting | `recv` | `استقبل` |
| Receive without waiting | `try_recv` | `حاول_الاستقبال` |
| Receive with a timeout | `recv_timeout` | `استقبل_بمهلة` |
| Drop an end explicitly | `drop_sender`, `drop_receiver` | `أسقط_مرسل`, `أسقط_مستلم` |

`send` hands the value back as the error once the receiver is gone. `try_send` reports `ممتلئة` /
full or `منقطعة` / disconnected, with the value. `recv` fails once every sender is gone and nothing
is queued. `try_recv` and `recv_timeout` report empty, timeout or disconnected, using the error
types of Rust's `mpsc` (`خطأ_محاولة_استقبال`, `خطأ_استقبال_بمهلة`, `خطأ_استقبال`,
`خطأ_محاولة_إرسال`).

### Implementation

The shared state sits behind an `Arc`. It is a queue of two vectors: values are appended to an
inbox and popped from an outbox that is refilled from the inbox, so every value is moved and never
copied. The queue is guarded by the three-state lock and accompanied by two futex sequence words,
"a value is available" and "a slot is free".

### Disconnection without destructors

The channel counts its ends with two shared tokens (`Arc`s). Each token is held by the shared
state and by every sender (or the receiver), so the number of live senders is the sender token's
strong count minus the state's own. An end passed to `أسقط_مرسل` or `أسقط_مستلم` is dropped there
and the other side is woken at once. An end dropped implicitly, by a thread that finishes or a
scope that ends, is noticed by a waiting thread within a few milliseconds, because the waiter
re-checks the counts between bounded sleeps (1 ms doubling to 32 ms). Ends are not copyable, so a
spawned closure moves the end it captures.

A bound of 0 holds one value: Rust's rendezvous channel is not implemented.

## Data-race freedom: the spawn capture check

A closure handed to another thread may capture only what may cross threads, which is Rust's
`F: Send + 'static`. The compiler rejects a closure that captures:

- a raw pointer, unless the spawn sits inside `غير_آمن`;
- a borrowed reference or slice;
- a writable module variable that its body uses outside an unsafe block.

Each case has its own diagnostic, in Arabic and English: `SULTANC-THREAD-CAPTURE-RAW-POINTER`,
`SULTANC-THREAD-CAPTURE-REFERENCE` and `SULTANC-THREAD-CAPTURE-NON-ATOMIC-SHARED`. The calls that
send a closure are derived from the closure-address intrinsics, not listed by name. The rules, the
derivation and their limits are in [Type system](type-system.md#thread-capture-rules).

The thread intrinsics themselves stay behind `غير_آمن`: `بدائي_استنسخ_خيط_نواة` runs code on a
caller-supplied stack and `بدائي_دالة_من_عنوان` rebuilds a closure from an address. Leaking a box
(`بدائي_سرب_قيمة`) is also an unsafe intrinsic.

## Process-wide state

Two process-wide tables are made safe for threads.

- The allocator (`library/std/os/allocation.sn`) serves every thread. A flag set by the first
  spawn turns on an uncontended compare-exchange spinlock around allocate, reallocate and free; a
  program that never spawns pays one branch. `هل_بدأت_خيوط` exposes the flag.
- The environment table (`std.env.table`; Rust's `ENV_LOCK`, Go's `envLock`) takes one
  process-wide lock for every read and write once a thread exists, using the three-state protocol
  on a scalar module variable (`انتظر_عند_الخلية`). Before the first thread, no lock is taken.

## The interpreter

`sultanc run` executes on one OS thread. Atomics run as plain read-modify-writes, and mutex,
rwlock, condvar (notify), once, once-lock and `Arc` reads work and agree with native code;
`tests/*/stdlib/behavior/sync_single_thread.sn` is a differential pair. `بدائي_استنسخ_خيط_نواة`
and the closure-address intrinsics are reported as unsupported machine primitives, so spawning,
`arc_strong_count` and channels (which count their ends with it) are not available there.

## Verification

| Where | What runs |
|---|---|
| x86_64 Linux, native backend | every test below, Arabic and English |
| aarch64, riscv64, s390x Linux (LLVM, qemu-user) | the same tests |
| i686 and armv7 Linux (LLVM, qemu-user) | spawn, join and a mutex counter across threads |
| Windows x86_64 (Wine) | `wait_address_single`, `sync_single_thread` (`WaitOnAddress`, including the timed wait), and a spawned mutex counter |
| macOS arm64 and x86_64 | built; `macos_imports_audit.py` reports no missing import (`__ulock_wait2`, `__ulock_wake`) |
| FreeBSD x86_64, aarch64, riscv64 | built; not run |

The tests are in `tests/{ar,en}/stdlib/behavior/`:

| Test | What it checks |
|---|---|
| `sync_mutex_counter` | 8 threads, 10 000 increments each, exact total |
| `sync_arc_threads_rss_flat` | 10^4 rounds with the allocator's live bytes flat |
| `sync_condvar_ping_pong` | 2 000 turns and a timed wait |
| `sync_once_contention` | 8 threads, one run, `once_lock` |
| `sync_channel_producers` | 4 producers, exact sum, per-producer order |
| `sync_channel_bounded` | backpressure and a full `try_send` |
| `sync_channel_disconnect` | empty, timeout and disconnected both ways, a sender dropped by a finishing thread |
| `sync_atomic_register_pressure` | atomics reached through closures and three-deep call chains with many live locals on 4 threads, exact totals |
| `sync_env_threads` | the environment table under threads |
| `sync_single_thread` | the interpreter differential pair |
| `wait_address_single` | wait and wake on one thread |
| `thread_spawn_join`, `thread_join_value`, `thread_current`, `thread_nested`, `thread_many_short`, `thread_slots` | the thread API |

The reject tests are `tests/*/diagnostics/safety/thread_capture_{raw_pointer,reference,global}.sn`,
and the accept test is `tests/*/compiler/semantic/thread_capture_allowed.sn`.

## Limitations

- The spawn capture check does not see through a closure passed in a variable, a captured
  closure, or a call that touches a writable module variable. Rust tracks these with auto traits
  on closure types.
- Scoped threads (`thread::scope`) need region and lifetime support that the ownership checker
  does not express cheaply. Thread-locals, barriers and `WaitGroup` do not exist.
- Atomics ignore weaker orderings and always use `seq_cst`.
- The rwlock has no writer preference.
- Channels notice an implicitly dropped end by polling, within 32 ms.
- The HTTP/1 and HTTP/2 servers do not serve connections concurrently.
- Threads are not available on WASI, on freestanding targets or in the interpreter.

## See also

- [Type system](type-system.md#thread-capture-rules): the capture rules.
- [Ownership](ownership.md): moves, borrows and why closures own what they capture.
- [Standard library coverage](std-coverage.md): where `std.sync` and `std.thread` sit beside Rust and Go.
- [Platform dependencies](platform-dependencies.md): the system calls under the wait/wake and thread layers.
