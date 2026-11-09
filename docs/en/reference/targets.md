[العربية](../../ar/reference/targets.md) | **English**

# Targets

`sultanc targets` lists the 34 registered compilation targets. This page records, for each one,
which backend generates its code, what standard library support it has, how its executable is
linked and how it is verified. It also describes where target-specific code lives in the
repository, for contributors who add or port a target.

## Backends

SultanC has two code-generation backends, selected with `--backend=native` or `--backend=llvm`.

The native backend is SultanC's own code generator and, with the built-in linkers, needs no
external toolchain. It is available for four targets: `x86_64-linux`, `arm64-darwin`,
`wasm32-wasi` and `wasm32-browser`. Requesting it for any other target reports
`SULTANC-BACKEND-UNAVAILABLE`.

The LLVM backend is available for every target and requires a compiler built with LLVM
(`./build.sh --enable-llvm`). It produces objects for every target. Targets without an object
format (NVPTX, XCore) are compiled with `--emit=asm`, which works for every target.

The `الدعم` (support) column of `sultanc targets` carries one of three values: `مضيف ذاتي`
(self-hosted, `arm64-darwin` and `x86_64-linux`), `ترجمة متقاطعة` (cross-compilation, every other
target) and `تجريبي` (experimental, none today).

## Target matrix

### Operating-system targets

These targets have the full standard library, including files, processes, sockets and threads
(except where noted). All of them use raw kernel calls or the platform's system interface and link
no libc; [Platform dependencies](platform-dependencies.md) lists every import.

| Target | Architecture | Backend | Executable | Verified run |
|---|---|---|---|---|
| `x86_64-linux` | x86_64 | native, LLVM | built-in ELF linker | native |
| `aarch64-linux-gnu` | aarch64 | LLVM | built-in ELF linker | qemu |
| `riscv64-linux-gnu` | riscv64 | LLVM | `ld.lld` | qemu: 12 examples identical to x86_64 |
| `loongarch64-linux-gnu` | loongarch64 | LLVM | `ld.lld` | qemu: the same 12 examples |
| `powerpc64le-linux-gnu` | powerpc64le | LLVM | `ld.lld` | qemu: the same 12 examples |
| `s390x-linux-gnu` | s390x | LLVM | `ld.lld` | qemu: the same 12 examples |
| `mips64el-linux-gnuabi64` | mips64el (n64) | LLVM | `ld.lld` | qemu: the same 12 examples |
| `sparcv9-linux-gnu` | sparcv9 | LLVM | `ld.lld` | qemu: the same 12 examples |
| `i686-linux-gnu` | x86 | LLVM | `ld.lld` | qemu-i386: every example identical to x86_64 |
| `armv7-linux-gnueabihf` | arm | LLVM | `ld.lld` | qemu-arm: every example identical to x86_64 |
| `x86_64-android` | x86_64 | LLVM | `ld.lld` | qemu-user |
| `aarch64-android` | aarch64 | LLVM | `ld.lld` | qemu-user |
| `armv7-android` | arm | LLVM | `ld.lld` | qemu-arm |
| `arm64-darwin` | aarch64 | native, LLVM | built-in Mach-O linker | natively on arm64 (native backend) |
| `x86_64-darwin` | x86_64 | LLVM | built-in Mach-O linker | built |
| `x86_64-windows-msvc` | x86_64 | LLVM | built-in COFF linker | Wine |
| `aarch64-windows-msvc` | aarch64 | LLVM | built-in COFF linker | built |
| `x86_64-freebsd` | x86_64 | LLVM | `ld.lld` | FreeBSD 14.2 amd64 VM |
| `aarch64-freebsd` | aarch64 | LLVM | `ld.lld` | FreeBSD 14.5 VM under qemu |
| `riscv64-freebsd` | riscv64 | LLVM | `ld.lld` | FreeBSD 14.5 VM under qemu |

Linux and Android targets use the Linux kernel ABI directly. The 32-bit targets (`i686-linux-gnu`,
`armv7-linux-gnueabihf`, `armv7-android`) use the time64 and large-file system calls. On them, and
on every other target that lacks a divide instruction or a wide divide, the backend expands
division in IR so no compiler-rt is linked (see [What lives where](#what-lives-where)).

The FreeBSD targets use raw system calls with no libc. They require FreeBSD 13 or later (for
`close_range`) and are built against the 14.2 headers. The executable is static, with
`ELFOSABI_FREEBSD` and the `.note.tag` ABI note.

### WebAssembly

| Target | Backend | Executable | Notes |
|---|---|---|---|
| `wasm32-wasi` | native, LLVM | built-in wasm linker | the full std over WASI preview 1; sockets are limited to preopened ones; no processes, threads or file locks. Runs under a wasmtime-compatible runner. |
| `wasm32-browser` | native, LLVM | built-in wasm linker | freestanding: language core only |
| `wasm64-unknown-unknown` | LLVM | objects | freestanding: language core only |

### Freestanding and accelerator targets

These targets have no OS layer; programs use the language core and `std` modules that do not
depend on an operating system.

| Target | Architecture | Output |
|---|---|---|
| `riscv32-unknown-elf` | riscv32 | objects, assembly |
| `avr-unknown-unknown` | avr | objects, assembly |
| `msp430-unknown-elf` | msp430 | objects, assembly |
| `hexagon-unknown-elf` | hexagon | objects, assembly |
| `lanai-unknown-unknown` | lanai | objects, assembly |
| `bpfel-unknown-none` | bpf | objects, assembly |
| `amdgcn-amd-amdhsa` | amdgpu | objects, assembly |
| `spirv64-unknown-unknown` | spirv64 | SPIR-V objects, assembly |
| `nvptx64-nvidia-cuda` | nvptx64 | assembly only |
| `xcore-unknown-unknown` | xcore | assembly only |

These targets cannot be linked into an executable by SultanC: requesting `exe` reports
`SULTANC-LINK-OUTPUT-UNSUPPORTED`. Use `-c` or `--emit=obj` for objects and `--emit=asm` for
assembly.

### Not yet supported: ve-linux-gnu

`ve-linux-gnu` (NEC Vector Engine) compiles to objects and assembly but has no std OS layer. VE has
no kernel trap: a system call is a request to the VEOS daemon. The number and arguments are stored
in a per-thread shared-memory area whose address the VEOS and glibc start-up places in the thread
control block (`tcbhead_t.shm_offset`, glibc-ve `sysdeps/unix/sysv/linux/ve/sysdep.h`), and then
`monc` is issued. Porting needs that start-up protocol and VE hardware or an emulator to verify it.

## What lives where

| Layer | Owner | Notes |
|---|---|---|
| Kernel ABI (syscall numbers, flags, record layouts, byte order) | std: `library/std/os/platform/**` | Linux tables are generated from each architecture's kernel UAPI headers (`tools/generate/generate_linux_syscalls.py`, `generate_linux_uapi.py`). The 32-bit ABIs take the `ilp32:` rows of `tools/generate/registry/linux_syscalls.def` (clock_gettime64, ppoll_time64, mmap2, _llseek, fcntl64). FreeBSD's are generated from its headers (releng/14.2) into `platform/unix/freebsd`: one system-call table for every architecture (`generate_freebsd_syscalls.py`, `registry/freebsd_syscalls.def`, each row with its `syscalls.master` prototype), and the flag values, `struct stat` offsets and errno classification (`generate_freebsd_uapi.py`, read with `clang -E -dM` and `offsetof`). |
| OS providers (files, processes, sockets, memory, clocks) | std | One guarded branch per OS in `library/std/os/*.sn`. The direct-kernel provider in `platform/unix/linux` also serves FreeBSD: it takes its numbers from the selected table (`syscalls/selection.sn`) and its constants from the selected kernel's values (`linux/uapi.sn` or `freebsd/uapi.sn`, same function names). Where FreeBSD's call differs, a FreeBSD block handles it: `fstatat` for `statx`, `getdirentries` records, `__getcwd`, `wait4` for `waitid`, `fork` for `clone`, `getrlimit`/`setrlimit` for `prlimit64`, and the `__sysctl` `hw.pagesize` for the procfs auxv. |
| Kernel entry instruction, `_start`, error-flag normalization | backend (`compiler/backend/provider/llvm/{lowering,entry}.sn`) | The only per-architecture code outside std. i386 passes the number and sixth argument through a stack block so EBP can be saved. FreeBSD flags errors with the carry flag (amd64, arm64) or t0 (riscv) and a positive errno, which the sequence turns into -errno. Its `_start` takes the argument block from rdi, x0 or a0, and the executable carries the `.note.tag` ABI note (osrel 1402000) besides `ELFOSABI_FREEBSD`. |
| Arithmetic the target lacks | backend (`compiler/backend/provider/llvm/wide_division.sn`; checked multiply in `lowering.sn`) | Expanded in IR so no compiler-rt is linked. 64-bit division on 32- and 16-bit targets is expanded, and so is division of every width on targets with no divide instruction (ARMv7-A, which does not guarantee SDIV/UDIV; AVR; MSP430; Hexagon; Lanai), by one restoring shift-and-subtract function per width (`sultanc.udivmodN`). SPARC V9 divides 8-, 16- and 32-bit operands in 64 bits (`SDIVX`/`UDIVX`) instead of the deprecated Y-register `SDIV`/`UDIV`, because QEMU 8.2's `sdiv` also divides by the zero-extended divisor and returns wrong narrow signed quotients. |
| Linking | built-in linkers (ELF for x86_64 and aarch64 Linux, Mach-O, COFF, wasm); `ld.lld` for the other Linux ELF targets and FreeBSD (`compiler/driver/external_linker.sn`, `$SULTANC_LINKER`) | Static executables, no libc. |

A native backend for a new architecture needs only the kernel entry and `_start`; the std layer is
shared with the LLVM backend.

## Verification

Each target is verified at the strongest level its environment allows.

| Level | Targets | Meaning |
|---|---|---|
| Native run | `x86_64-linux`; `arm64-darwin` on an arm64 Mac | the built executable runs on the host |
| qemu-user | aarch64, riscv64, loongarch64, powerpc64le, s390x, mips64el, sparcv9, i686 and armv7 Linux; the Android targets | examples run under `qemu-<arch>` and their output is compared with x86_64 |
| Wine | `x86_64-windows-msvc` | the PE executable runs under Wine |
| FreeBSD VM | `x86_64-freebsd`; `aarch64-freebsd` and `riscv64-freebsd` under qemu system emulation | examples run in a FreeBSD guest |
| WASI runner | `wasm32-wasi` | a wasmtime-compatible runner |
| Built | `x86_64-darwin`, `aarch64-windows-msvc` | the executable links; `tools/audit/macos_imports_audit.py` checks Mach-O imports against the SDK |
| Objects and assembly | freestanding and accelerator targets | the compiler produces objects or assembly |

On the 12-example qemu comparison for riscv64, loongarch64, powerpc64le, s390x, mips64el and
sparcv9, output is identical to x86_64. i686 and armv7 pass every example. On FreeBSD x86_64,
every buildable example is identical to x86_64 Linux and its system calls are checked with
`truss`. On the aarch64 and riscv64 VMs, every buildable example (87, built with the LLVM backend)
gives the same output and status as on x86_64 Linux. `io/logging` interleaves stdout and stderr
differently over ssh, with each stream identical. `tools/tests/freebsd_examples.sh` runs the
x86_64 examples on a Linux host through a ptrace system-call translator
(`tools/tests/freebsd_abi_emulator.c`).

### Cross-checking a Linux target

```sh
./build/sultanc --backend=llvm --target=riscv64-linux-gnu examples/en/fs/files.sn -o files
qemu-riscv64 ./files
```

### Regenerating the FreeBSD tables

The generators read only the `sys/` and `include/` headers of a FreeBSD source tree.

```sh
git clone --depth 1 -b releng/14.2 https://github.com/freebsd/freebsd-src
python3 tools/generate/generate_freebsd_syscalls.py --src freebsd-src
python3 tools/generate/generate_freebsd_uapi.py --src freebsd-src
```

### Running FreeBSD aarch64 and riscv64 under qemu

The official VM images run under qemu's system emulation (TCG; no KVM needed). On Ubuntu 24.04 the
packages are `qemu-system-arm qemu-system-misc qemu-efi-aarch64 u-boot-qemu opensbi`. The images
are in `https://download.freebsd.org/releases/VM-IMAGES/14.5-RELEASE/{aarch64,riscv64}/Latest/`:
`FreeBSD-14.5-RELEASE-arm64-aarch64-ufs.qcow2.xz` and
`FreeBSD-14.5-RELEASE-riscv-riscv64-ufs.qcow2.xz`. Check them against `CHECKSUM.SHA256`. The 14.2
release is no longer on the mirror; 14.5 runs binaries built against the 14.2 headers. Converting
an image with `qemu-img convert -c -O qcow2` halves its size.

```sh
# aarch64: UEFI firmware, virtio PCI devices, SSH on host port 2223
qemu-system-aarch64 -M virt -cpu cortex-a72 -smp 4 -m 2048 -display none \
  -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
  -drive file=arm64-aarch64.qcow2,if=none,id=hd0,format=qcow2 -device virtio-blk-pci,drive=hd0 \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2223-:22 -device virtio-net-pci,netdev=n0 \
  -serial unix:ser-aarch64.sock,server,nowait -monitor none

# riscv64: OpenSBI, then U-Boot (S-mode), which boots the image's EFI loader; SSH on 2224
qemu-system-riscv64 -M virt -smp 4 -m 2048 -display none \
  -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin \
  -kernel /usr/lib/u-boot/qemu-riscv64_smode/u-boot.bin \
  -drive file=riscv-riscv64.qcow2,if=none,id=hd0,format=qcow2 -device virtio-blk-device,drive=hd0 \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2224-:22 -device virtio-net-device,netdev=n0 \
  -serial unix:ser-riscv64.sock,server,nowait -monitor none
```

The images log `root` in on the serial console without a password. Over the console socket,
install a key in `/root/.ssh/authorized_keys`, add `PermitRootLogin prohibit-password` to
`/etc/ssh/sshd_config`, then run `sysrc sshd_enable=YES; service sshd keygen; service sshd start`
(sshd is off in the images, and generating the host keys takes several minutes on riscv64 under
TCG). Copy a program with `scp -P 2223` (or 2224) and run it over `ssh`. Under TCG on a 4-core host
the aarch64 VM boots in about 3 minutes and the riscv64 VM in about 10, and riscv64 programs run
roughly ten times slower than aarch64 ones.

## Limitations

- The native backend covers four targets; every other target needs the LLVM backend.
- `ve-linux-gnu` has no std OS layer.
- Freestanding and accelerator targets produce objects or assembly only.
- `x86_64-darwin` and `aarch64-windows-msvc` are built but not run.
- `wasm32-wasi` has no processes, threads or file locks, and its sockets are limited to
  preopened ones.

## See also

- [Platform dependencies](platform-dependencies.md): every system interface std imports, per platform.
- [Concurrency](concurrency.md#os-threads): thread support per OS.
- [MSIR and code generation](../internals/msir-and-codegen.md): how backends and linkers fit together.
- [Bootstrap](../internals/bootstrap.md): building the compiler with or without LLVM.
