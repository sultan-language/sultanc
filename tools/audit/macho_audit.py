#!/usr/bin/env python3
"""Structural audit for SultanC macOS ARM64 Mach-O executables.

Canonical ownership:
    This script owns verification of the executable container produced by the
    Mach-O backend. It does not define Mach-O emission policy, ABI semantics,
    external-symbol semantics, or code-generation behavior.

The audit derives command-region size from the commands that are actually in
an image. Foreign executables are validated by parsing LC_DYLD_INFO_ONLY,
LC_SYMTAB, LC_DYSYMTAB, the dyld bind program, undefined nlist_64 entries, and
loaded libraries. No symbol name or command count is special-cased.
"""
from __future__ import annotations

from dataclasses import dataclass
import pathlib
import struct
import sys

MH_MAGIC_64 = 0xFEEDFACF
CPU_TYPE_ARM64 = 0x0100000C
MH_EXECUTE = 2
MH_NOUNDEFS = 0x1

LC_SEGMENT_64 = 0x19
LC_LOAD_DYLINKER = 0x0E
LC_LOAD_DYLIB = 0x0C
LC_MAIN = 0x80000028
LC_UUID = 0x1B
LC_BUILD_VERSION = 0x32
LC_SYMTAB = 0x2
LC_DYSYMTAB = 0xB
LC_CODE_SIGNATURE = 0x1D
LC_CODE_SIGNATURE_COMMAND_SIZE = 16
LC_DYLD_INFO_ONLY = 0x80000022

S_ATTR_DEBUG = 0x02000000
N_EXT = 0x01
N_TYPE = 0x0E
N_UNDF = 0x00

BIND_OPCODE_MASK = 0xF0
BIND_IMMEDIATE_MASK = 0x0F
BIND_OPCODE_DONE = 0x00
BIND_OPCODE_SET_DYLIB_ORDINAL_IMM = 0x10
BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM = 0x40
BIND_OPCODE_SET_TYPE_IMM = 0x50
BIND_OPCODE_SET_ADDEND_SLEB = 0x60
BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB = 0x70
BIND_OPCODE_ADD_ADDR_ULEB = 0x80
BIND_OPCODE_DO_BIND = 0x90
BIND_TYPE_POINTER = 1

EXPECTED_DWARF = (
    b"__debug_info",
    b"__debug_abbrev",
    b"__debug_line",
    b"__debug_frame",
    b"__debug_aranges",
    b"__debug_pubnames",
)


@dataclass(frozen=True)
class Segment:
    name: bytes
    vmaddr: int
    vmsize: int
    fileoff: int
    filesize: int
    nsects: int


@dataclass(frozen=True)
class Symtab:
    symoff: int
    nsyms: int
    stroff: int
    strsize: int


@dataclass(frozen=True)
class Dysymtab:
    iundefsym: int
    nundefsym: int


@dataclass(frozen=True)
class DyldInfo:
    bind_off: int
    bind_size: int


@dataclass(frozen=True)
class Bind:
    symbol: bytes
    ordinal: int
    bind_type: int
    segment_index: int
    segment_offset: int


def fail(message: str) -> None:
    print(f"Mach-O audit: FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def name16(value: bytes) -> bytes:
    return value.split(b"\0", 1)[0]


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


def read_c_string(data: bytes, offset: int, end: int, label: str) -> tuple[bytes, int]:
    if offset < 0 or offset >= end or end > len(data):
        fail(f"{label} string starts outside its region")
    terminator = data.find(b"\0", offset, end)
    if terminator < 0:
        fail(f"{label} string is not NUL terminated")
    return data[offset:terminator], terminator + 1


def read_uleb(data: bytes, offset: int, end: int, label: str) -> tuple[int, int]:
    value = 0
    shift = 0
    while offset < end:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if (byte & 0x80) == 0:
            return value, offset
        shift += 7
        if shift >= 64:
            fail(f"{label} ULEB128 is too large")
    fail(f"{label} ULEB128 is truncated")


def read_sleb(data: bytes, offset: int, end: int, label: str) -> tuple[int, int]:
    value = 0
    shift = 0
    byte = 0
    while offset < end:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if (byte & 0x80) == 0:
            if shift < 64 and (byte & 0x40) != 0:
                value |= -(1 << shift)
            return value, offset
        if shift >= 64:
            fail(f"{label} SLEB128 is too large")
    fail(f"{label} SLEB128 is truncated")


def check_dwarf_unit(section: bytes, label: str) -> None:
    if len(section) < 4:
        fail(f"{label} too small")
    length = u32(section, 0)
    if length + 4 > len(section):
        fail(f"{label} unit length escapes section")


def parse_bind_stream(data: bytes,
                      info: DyldInfo,
                      segments: list[Segment]) -> list[Bind]:
    if info.bind_size == 0:
        return []
    start = info.bind_off
    end = start + info.bind_size
    if start < 0 or end > len(data) or start >= end:
        fail("dyld bind stream escapes file")

    offset = start
    ordinal = 0
    bind_type = 0
    symbol: bytes | None = None
    segment_index: int | None = None
    segment_offset = 0
    binds: list[Bind] = []
    saw_done = False

    while offset < end:
        opcode_byte = data[offset]
        offset += 1
        opcode = opcode_byte & BIND_OPCODE_MASK
        immediate = opcode_byte & BIND_IMMEDIATE_MASK

        if opcode == BIND_OPCODE_DONE:
            saw_done = True
            if any(data[offset:end]):
                fail("non-zero bytes follow BIND_OPCODE_DONE")
            break
        if opcode == BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
            ordinal = immediate
            continue
        if opcode == BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
            symbol, offset = read_c_string(data, offset, end, "bind symbol")
            if not symbol:
                fail("empty dyld bind symbol")
            continue
        if opcode == BIND_OPCODE_SET_TYPE_IMM:
            bind_type = immediate
            continue
        if opcode == BIND_OPCODE_SET_ADDEND_SLEB:
            _, offset = read_sleb(data, offset, end, "bind addend")
            continue
        if opcode == BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
            segment_index = immediate
            if segment_index >= len(segments):
                fail("dyld bind references an invalid segment index")
            segment_offset, offset = read_uleb(data, offset, end, "bind segment offset")
            continue
        if opcode == BIND_OPCODE_ADD_ADDR_ULEB:
            delta, offset = read_uleb(data, offset, end, "bind address delta")
            segment_offset += delta
            continue
        if opcode == BIND_OPCODE_DO_BIND:
            if symbol is None or segment_index is None:
                fail("BIND_OPCODE_DO_BIND is missing symbol or segment state")
            if ordinal <= 0:
                fail("foreign bind has no positive dylib ordinal")
            if bind_type != BIND_TYPE_POINTER:
                fail(f"unsupported foreign bind type {bind_type}")
            segment = segments[segment_index]
            if segment_offset + 8 > segment.vmsize:
                fail("foreign bind pointer escapes target segment")
            if segment.filesize != 0 and segment_offset + 8 > segment.filesize:
                fail("foreign bind pointer escapes file-backed segment data")
            binds.append(Bind(symbol, ordinal, bind_type, segment_index, segment_offset))
            segment_offset += 8
            continue
        fail(f"unsupported dyld bind opcode 0x{opcode_byte:02x}")

    if not saw_done:
        fail("dyld bind stream has no BIND_OPCODE_DONE")
    return binds


def main(path: str) -> None:
    data = pathlib.Path(path).read_bytes()
    if len(data) < 32:
        fail("truncated mach header")

    magic, cputype, _, filetype, ncmds, sizeofcmds, flags, _ = struct.unpack_from(
        "<8I", data, 0
    )
    if magic != MH_MAGIC_64:
        fail("not MH_MAGIC_64")
    if cputype != CPU_TYPE_ARM64:
        fail("not ARM64")
    if filetype != MH_EXECUTE:
        fail("not MH_EXECUTE")
    if ncmds == 0:
        fail("image has no load commands")
    command_end = 32 + sizeofcmds
    if command_end > len(data):
        fail("load-command region escapes file")

    offset = 32
    commands: list[int] = []
    segments: list[Segment] = []
    sections: dict[tuple[bytes, bytes], tuple[int, int, int, int, int]] = {}
    uuid: bytes | None = None
    entry: int | None = None
    dylinker_path: bytes | None = None
    dylibs: list[bytes] = []
    code_signature: tuple[int, int] | None = None
    code_signature_command_offset: int | None = None
    symtab: Symtab | None = None
    dysymtab: Dysymtab | None = None
    dyld_info: DyldInfo | None = None
    parsed_command_size = 0

    for index in range(ncmds):
        if offset + 8 > command_end:
            fail(f"truncated load command {index}")
        cmd, cmdsize = struct.unpack_from("<II", data, offset)
        if cmdsize < 8 or (cmdsize & 7) != 0:
            fail(f"invalid command size {cmdsize} at command {index}")
        if offset + cmdsize > command_end:
            fail(f"load command {index} escapes command region")
        commands.append(cmd)
        parsed_command_size += cmdsize

        if cmd == LC_SEGMENT_64:
            if cmdsize < 72:
                fail("short segment command")
            segname = name16(data[offset + 8:offset + 24])
            vmaddr, vmsize, fileoff, filesize = struct.unpack_from("<QQQQ", data, offset + 24)
            _, _, nsects, _ = struct.unpack_from("<IIII", data, offset + 56)
            if 72 + nsects * 80 != cmdsize:
                fail(f"segment {segname!r} command size does not match section count")
            if fileoff + filesize > len(data):
                fail(f"segment {segname!r} escapes file")
            segment = Segment(segname, vmaddr, vmsize, fileoff, filesize, nsects)
            segments.append(segment)
            section_offset = offset + 72
            for _ in range(nsects):
                sectname = name16(data[section_offset:section_offset + 16])
                section_segname = name16(data[section_offset + 16:section_offset + 32])
                addr, size = struct.unpack_from("<QQ", data, section_offset + 32)
                foff, align, _, _, section_flags = struct.unpack_from(
                    "<IIIII", data, section_offset + 48
                )
                if size != 0 and foff + size > len(data):
                    fail(f"section {sectname!r} escapes file")
                sections[(section_segname, sectname)] = (
                    addr, size, foff, align, section_flags
                )
                section_offset += 80
        elif cmd == LC_UUID:
            if cmdsize != 24:
                fail("bad LC_UUID size")
            uuid = data[offset + 8:offset + 24]
        elif cmd == LC_MAIN:
            if cmdsize != 24:
                fail("bad LC_MAIN size")
            entry = u64(data, offset + 8)
        elif cmd == LC_LOAD_DYLINKER:
            if cmdsize < 16:
                fail("short LC_LOAD_DYLINKER")
            nameoff = u32(data, offset + 8)
            if nameoff >= cmdsize:
                fail("LC_LOAD_DYLINKER name offset escapes command")
            dylinker_path, _ = read_c_string(
                data, offset + nameoff, offset + cmdsize, "dylinker path"
            )
        elif cmd == LC_LOAD_DYLIB:
            if cmdsize < 24:
                fail("short LC_LOAD_DYLIB")
            nameoff = u32(data, offset + 8)
            if nameoff >= cmdsize:
                fail("LC_LOAD_DYLIB name offset escapes command")
            dylib, _ = read_c_string(data, offset + nameoff, offset + cmdsize, "dylib path")
            dylibs.append(dylib)
        elif cmd == LC_CODE_SIGNATURE:
            if cmdsize != LC_CODE_SIGNATURE_COMMAND_SIZE:
                fail("bad LC_CODE_SIGNATURE size")
            if code_signature is not None:
                fail("multiple LC_CODE_SIGNATURE commands")
            code_signature_command_offset = offset
            code_signature = (u32(data, offset + 8), u32(data, offset + 12))
        elif cmd == LC_SYMTAB:
            if cmdsize != 24 or symtab is not None:
                fail("invalid or duplicate LC_SYMTAB")
            symtab = Symtab(
                u32(data, offset + 8),
                u32(data, offset + 12),
                u32(data, offset + 16),
                u32(data, offset + 20),
            )
        elif cmd == LC_DYSYMTAB:
            if cmdsize != 80 or dysymtab is not None:
                fail("invalid or duplicate LC_DYSYMTAB")
            dysymtab = Dysymtab(
                u32(data, offset + 8 + 4 * 4),
                u32(data, offset + 8 + 5 * 4),
            )
        elif cmd == LC_DYLD_INFO_ONLY:
            if cmdsize != 48 or dyld_info is not None:
                fail("invalid or duplicate LC_DYLD_INFO_ONLY")
            dyld_info = DyldInfo(u32(data, offset + 16), u32(data, offset + 20))

        offset += cmdsize

    if parsed_command_size != sizeofcmds or offset != command_end:
        fail(
            f"load commands total {parsed_command_size} bytes but header sizeofcmds is {sizeofcmds}"
        )

    for required in (
        LC_SYMTAB,
        LC_DYSYMTAB,
        LC_BUILD_VERSION,
        LC_UUID,
        LC_LOAD_DYLINKER,
        LC_LOAD_DYLIB,
        LC_MAIN,
    ):
        if commands.count(required) != 1:
            fail(f"required load command 0x{required:x} must appear exactly once")

    if uuid is None or uuid == b"\0" * 16:
        fail("UUID missing/zero")
    if entry is None:
        fail("LC_MAIN entryoff missing")
    if dylinker_path != b"/usr/lib/dyld":
        fail("canonical dyld path missing")
    if b"/usr/lib/libSystem.B.dylib" not in dylibs:
        fail("libSystem load missing")

    text = sections.get((b"__TEXT", b"__text"))
    if text is None:
        fail("__TEXT,__text missing")
    text_offset = text[2]
    text_end = text_offset + text[1]
    if command_end > text_offset:
        fail("load-command/header region overlaps __TEXT,__text")
    if (text_offset & 3) != 0 or text[3] != 2:
        fail("__text offset/alignment mismatch")
    if entry < text_offset or entry >= text_end:
        fail("LC_MAIN entryoff is outside __TEXT,__text")

    if code_signature is None:
        if text_offset - command_end < LC_CODE_SIGNATURE_COMMAND_SIZE:
            fail("unsigned image does not reserve LC_CODE_SIGNATURE command space before __text")
    else:
        if code_signature_command_offset is None:
            fail("LC_CODE_SIGNATURE command location missing")
        if code_signature_command_offset < 32 or (
            code_signature_command_offset + LC_CODE_SIGNATURE_COMMAND_SIZE > command_end
        ):
            fail("LC_CODE_SIGNATURE command escapes load-command region")
        sig_off, sig_size = code_signature
        if sig_size < 12 or sig_off + sig_size > len(data):
            fail("code signature escapes file")
        linkedit = next((segment for segment in segments if segment.name == b"__LINKEDIT"), None)
        if linkedit is None:
            fail("__LINKEDIT missing for code signature")
        if sig_off < linkedit.fileoff or sig_off + sig_size > linkedit.fileoff + linkedit.filesize:
            fail("code signature is outside __LINKEDIT")
        if not (sig_off + sig_size <= text_offset or sig_off >= text_end):
            fail("code-signature blob overlaps __TEXT,__text")
        if sig_off % 16 != 0:
            fail("code signature is not 16-byte aligned")
        if struct.unpack_from(">I", data, sig_off)[0] != 0xFADE0CC0:
            fail("code signature SuperBlob magic missing")
    note = sections.get((b"__DATA", b"__note.sultan"))
    if note is None or data[note[2]:note[2] + note[1]] != b"SULTANC\0":
        fail("__note.sultan payload mismatch")

    if symtab is None or dysymtab is None:
        fail("symbol-table metadata missing")
    if symtab.symoff + symtab.nsyms * 16 > len(data):
        fail("symbol table escapes file")
    if symtab.stroff + symtab.strsize > len(data):
        fail("string table escapes file")
    if symtab.strsize == 0 or data[symtab.stroff] != 0:
        fail("Mach-O string table must begin with NUL")
    if dysymtab.iundefsym + dysymtab.nundefsym > symtab.nsyms:
        fail("undefined-symbol range escapes symbol table")

    undefined_symbols: list[bytes] = []
    for symbol_index in range(dysymtab.iundefsym, dysymtab.iundefsym + dysymtab.nundefsym):
        symbol_offset = symtab.symoff + symbol_index * 16
        strx = u32(data, symbol_offset)
        n_type = data[symbol_offset + 4]
        n_sect = data[symbol_offset + 5]
        n_desc = u16(data, symbol_offset + 6)
        n_value = u64(data, symbol_offset + 8)
        if strx >= symtab.strsize:
            fail("undefined symbol string index escapes string table")
        symbol_name, _ = read_c_string(
            data,
            symtab.stroff + strx,
            symtab.stroff + symtab.strsize,
            "undefined symbol",
        )
        if (n_type & (N_TYPE | N_EXT)) != (N_UNDF | N_EXT) or n_sect != 0 or n_value != 0:
            fail(f"symbol {symbol_name!r} is not a canonical external undefined symbol")
        library_ordinal = (n_desc >> 8) & 0xFF
        if library_ordinal == 0 or library_ordinal > len(dylibs):
            fail(f"symbol {symbol_name!r} has invalid library ordinal {library_ordinal}")
        undefined_symbols.append(symbol_name)

    if undefined_symbols:
        if (flags & MH_NOUNDEFS) != 0:
            fail("foreign-symbol image incorrectly sets MH_NOUNDEFS")
        if dyld_info is None:
            fail("undefined symbols require LC_DYLD_INFO_ONLY")
        binds = parse_bind_stream(data, dyld_info, segments)
        bound_names = [bind.symbol for bind in binds]
        if sorted(bound_names) != sorted(undefined_symbols):
            fail(
                "dyld bound symbols do not exactly match undefined symbol table: "
                f"bind={bound_names!r}, undefined={undefined_symbols!r}"
            )
        for bind in binds:
            if bind.ordinal > len(dylibs):
                fail(f"bind for {bind.symbol!r} references missing dylib ordinal")
            segment = segments[bind.segment_index]
            if segment.name != b"__DATA":
                fail(f"foreign pointer for {bind.symbol!r} is not stored in __DATA")
            file_offset = segment.fileoff + bind.segment_offset
            if file_offset + 8 > len(data):
                fail(f"foreign pointer slot for {bind.symbol!r} escapes file")
            if u64(data, file_offset) != 0:
                fail(f"foreign pointer slot for {bind.symbol!r} is not zero before dyld binding")
    else:
        if (flags & MH_NOUNDEFS) == 0:
            fail("image without undefined symbols must set MH_NOUNDEFS")
        if dyld_info is not None and dyld_info.bind_size != 0:
            fail("image without undefined symbols unexpectedly contains bind program")

    dwarf_sections = []
    for name in EXPECTED_DWARF:
        section = sections.get((b"__DWARF", name))
        if section is None:
            fail(f"{name.decode()} missing")
        if section[1] == 0:
            fail(f"{name.decode()} is empty")
        if (section[4] & S_ATTR_DEBUG) == 0:
            fail(f"{name.decode()} missing DEBUG attribute")
        dwarf_sections.append((name, section, data[section[2]:section[2] + section[1]]))

    check_dwarf_unit(dwarf_sections[0][2], "__debug_info")
    if (u32(dwarf_sections[0][2], 4) & 0xFFFF) != 4:
        fail("__debug_info is not DWARF v4")
    if dwarf_sections[1][2][-1] != 0:
        fail("__debug_abbrev has no terminator")
    check_dwarf_unit(dwarf_sections[2][2], "__debug_line")
    check_dwarf_unit(dwarf_sections[3][2], "__debug_frame")
    check_dwarf_unit(dwarf_sections[4][2], "__debug_aranges")
    check_dwarf_unit(dwarf_sections[5][2], "__debug_pubnames")

    foreign_suffix = f" + {len(undefined_symbols)} foreign bind(s)" if undefined_symbols else ""
    print(f"Mach-O64 + DWARF audit: PASS{foreign_suffix}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: macho_audit.py FILE")
    main(sys.argv[1])
