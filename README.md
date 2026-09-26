<div align="center">
<img src="https://raw.githubusercontent.com/sultan-language/sultanc-vscode/main/icons/sultanc-logo.svg" alt="SultanC" width="100">

# SultanC

**A self-hosted systems programming language, written in Arabic.**

<p>
<img src="https://img.shields.io/badge/self--hosted-yes-1a7f37?style=for-the-badge&labelColor=0d1117" alt="Self-hosted">
<img src="https://img.shields.io/badge/macOS-arm64-1a7f37?style=for-the-badge&labelColor=0d1117&logo=apple&logoColor=white" alt="macOS arm64">
<img src="https://img.shields.io/badge/Linux-x86__64-1a7f37?style=for-the-badge&labelColor=0d1117&logo=linux&logoColor=white" alt="Linux x86_64">
<img src="https://img.shields.io/badge/source-Arabic%20%2B%20English-8957e5?style=for-the-badge&labelColor=0d1117" alt="Language">
<img src="https://img.shields.io/badge/license-MIT-8b949e?style=for-the-badge&labelColor=0d1117" alt="License">
</p>

[Install](#install) &nbsp;·&nbsp; [Language](#language) &nbsp;·&nbsp; [Documentation](#documentation)

</div>

<br>

SultanC is a native systems language with ownership-aware safety, algebraic data types, and pattern matching. Its keywords exist in both Arabic and English as equal, interchangeable surfaces of the same syntax. The compiler is written in SultanC and compiles itself — every release passes a byte-for-byte self-host check on both macOS (arm64) and Linux (x86_64).

<br>

<table>
<tr><th width="50%">English</th><th width="50%">العربية</th></tr>
<tr>
<td valign="top">

```
def main (): int {
    let x: int = 40;
    let y: int = 2;
    return x + y;
}
```

</td>
<td valign="top" dir="rtl">

```
عرف القلعة(): عدد {
    دع س: عدد = 40;
    دع ص: عدد = 2;
    أرجع س + ص;
}
```

</td>
</tr>
</table>

Same AST, same semantics, same diagnostics — only the keywords and identifiers change.

## Install

```sh
git clone https://github.com/sultan-language/sultanc.git && cd sultanc
./build.sh
sudo cp build/sultanc /usr/local/bin/sultanc
```

> macOS with Homebrew LLVM: `export LLVM_CONFIG="$(brew --prefix llvm)/bin/llvm-config"` before building.

```sh
sultanc main.sn -o app && ./app
```

## Language

- **Ownership, borrowing, and move checking** — verified statically before codegen
- **Algebraic data types and pattern matching**
- **Arabic and English** as equal source surfaces, including diagnostics
- **A verified intermediate representation (MSIR)** that every optimization pass must pass through a checker before reaching a backend
- **Native output** — Mach-O and ELF, no interpreter in the compiled binary's path

```sh
sultanc check main.sn              # type-check only
sultanc interpret main.sn          # run without compiling
sultanc --lang ar main.sn          # Arabic diagnostics
sultanc --diagnostic-format json check main.sn
```

## Documentation

| | |
|---|---|
| [Architecture & MSIR](docs/MSIR_CODEGEN_ARCHITECTURE.md) | How source becomes native code |
| [Kernel architecture](docs/SULTANC_KERNEL_ARCHITECTURE.md) | Compiler internals |
| [Safety flow](docs/SULTAN_SAFETY_FLOW.md) | Ownership and borrow checking |
| [Type system](docs/TYPE_SYSTEM.md) | Types, inference, generics |
| [Arabic terminology](docs/ARABIC_TERMINOLOGY.md) | Canonical Arabic compiler vocabulary |

## Status

Self-hosting on `arm64-darwin` and `x86_64-linux`, both verified by byte-identical convergence across compiler generations. An LSP and the standard library are in progress.

## License

[MIT License](LICENSE)