/* Finalizes LLVM x86_64 ELF objects into bootstrap executables. */

#include "link/elf_executable.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ELF_SHT_PROGBITS 1U
#define ELF_SHT_SYMTAB 2U
#define ELF_SHT_STRTAB 3U
#define ELF_SHT_RELA 4U
#define ELF_SHT_NOBITS 8U
#define ELF_SHF_WRITE 1ULL
#define ELF_SHF_ALLOC 2ULL
#define ELF_SHN_UNDEF 0U
#define ELF_SHN_ABS 0xFFF1U
#define ELF_R_X86_64_64 1U
#define ELF_R_X86_64_PC32 2U
#define ELF_R_X86_64_PLT32 4U
#define ELF_R_X86_64_32 10U
#define ELF_R_X86_64_32S 11U
#define ELF_R_X86_64_REX_GOTPCRELX 42U

/* Describes one ELF64 section header. */
typedef struct
{
    uint32_t type;
    uint64_t flags;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t align;
    uint64_t entry_size;
} __Bootstrap_ELF_Section__;

/* Describes one ELF64 symbol. */
typedef struct
{
    uint32_t name;
    uint8_t info;
    uint16_t section;
    uint64_t value;
    uint64_t size;
} __Bootstrap_ELF_Symbol__;

/* Reports one ELF finalizer failure. */
static int elf_fail(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size != 0U)
        snprintf(error, error_size, "%s", message);
    return 0;
}

/* Checks a byte range. */
static int elf_bounds(size_t total, uint64_t offset, uint64_t size)
{
    if (offset > (uint64_t)total)
        return 0;
    return size <= (uint64_t)total - offset;
}

/* Reads little-endian integer values. */
static uint16_t elf_u16(const uint8_t *data, size_t offset)
{
    return (uint16_t)((uint16_t)data[offset] | ((uint16_t)data[offset + 1U] << 8U));
}

static uint32_t elf_u32(const uint8_t *data, size_t offset)
{
    return (uint32_t)data[offset] | ((uint32_t)data[offset + 1U] << 8U) |
           ((uint32_t)data[offset + 2U] << 16U) | ((uint32_t)data[offset + 3U] << 24U);
}

static uint64_t elf_u64(const uint8_t *data, size_t offset)
{
    return (uint64_t)elf_u32(data, offset) | ((uint64_t)elf_u32(data, offset + 4U) << 32U);
}

/* Aligns a size upward. */
static int elf_align(size_t value, size_t alignment, size_t *result)
{
    size_t remainder;
    size_t addition;
    if (result == NULL || alignment == 0U)
        return 0;
    remainder = value % alignment;
    if (remainder == 0U)
    {
        *result = value;
        return 1;
    }
    addition = alignment - remainder;
    if (value > SIZE_MAX - addition)
        return 0;
    *result = value + addition;
    return 1;
}

/* Reads one section header. */
static int elf_read_section(const uint8_t *object,
                            size_t object_size,
                            uint64_t table,
                            size_t index,
                            __Bootstrap_ELF_Section__ *section)
{
    size_t offset;
    if (section == NULL || index > (SIZE_MAX - (size_t)table) / 64U)
        return 0;
    offset = (size_t)table + index * 64U;
    if (!elf_bounds(object_size, offset, 64U))
        return 0;
    section->type = elf_u32(object, offset + 4U);
    section->flags = elf_u64(object, offset + 8U);
    section->offset = elf_u64(object, offset + 24U);
    section->size = elf_u64(object, offset + 32U);
    section->link = elf_u32(object, offset + 40U);
    section->info = elf_u32(object, offset + 44U);
    section->align = elf_u64(object, offset + 48U);
    section->entry_size = elf_u64(object, offset + 56U);
    if (section->type != ELF_SHT_NOBITS && !elf_bounds(object_size, section->offset, section->size))
        return 0;
    return 1;
}

/* Reads one symbol-table entry. */
static int elf_read_symbol(const uint8_t *object,
                           size_t object_size,
                           const __Bootstrap_ELF_Section__ *symtab,
                           size_t index,
                           __Bootstrap_ELF_Symbol__ *symbol)
{
    size_t offset;
    if (symtab == NULL || symbol == NULL || symtab->entry_size != 24U ||
        index > (SIZE_MAX - (size_t)symtab->offset) / 24U)
        return 0;
    offset = (size_t)symtab->offset + index * 24U;
    if (!elf_bounds(object_size, offset, 24U))
        return 0;
    symbol->name = elf_u32(object, offset);
    symbol->info = object[offset + 4U];
    symbol->section = elf_u16(object, offset + 6U);
    symbol->value = elf_u64(object, offset + 8U);
    symbol->size = elf_u64(object, offset + 16U);
    return 1;
}

/* Returns a validated symbol string. */
static const char *elf_string(const uint8_t *object,
                              size_t object_size,
                              const __Bootstrap_ELF_Section__ *strtab,
                              uint32_t index)
{
    size_t start;
    size_t end;
    if (strtab == NULL || index >= strtab->size || strtab->offset > SIZE_MAX - index)
        return NULL;
    start = (size_t)strtab->offset + index;
    end = (size_t)strtab->offset + (size_t)strtab->size;
    if (end > object_size || start >= end)
        return NULL;
    while (start < end)
    {
        if (object[start] == 0U)
            return (const char *)(object + (size_t)strtab->offset + index);
        ++start;
    }
    return NULL;
}

/* Computes one signed PC-relative relocation. */
static int elf_relative32(uint64_t place,
                          uint64_t target,
                          int64_t addend,
                          uint32_t *result)
{
    int64_t value;
    if (result == NULL || target > INT64_MAX || place > INT64_MAX)
        return 0;
    value = (int64_t)target + addend - (int64_t)place;
    if (value < INT32_MIN || value > INT32_MAX)
        return 0;
    *result = (uint32_t)(int32_t)value;
    return 1;
}

/* Writes one absolute relocation. */
static int elf_write_absolute(__Bootstrap_Byte_Buffer__ *output,
                              size_t offset,
                              uint32_t type,
                              uint64_t target,
                              int64_t addend)
{
    int64_t signed_value;
    uint64_t value;
    if (target > INT64_MAX)
        return 0;
    signed_value = (int64_t)target + addend;
    value = (uint64_t)signed_value;
    if (type == ELF_R_X86_64_64)
        return __Bootstrap_Byte_Buffer_Write_U64_LE__(output, offset, value);
    if (type == ELF_R_X86_64_32)
    {
        if (signed_value < 0 || (uint64_t)signed_value > UINT32_MAX)
            return 0;
        return __Bootstrap_Byte_Buffer_Write_U32_LE__(output, offset, (uint32_t)signed_value);
    }
    if (type == ELF_R_X86_64_32S)
    {
        if (signed_value < INT32_MIN || signed_value > INT32_MAX)
            return 0;
        return __Bootstrap_Byte_Buffer_Write_U32_LE__(output, offset, (uint32_t)(int32_t)signed_value);
    }
    return 0;
}

/* Appends one ELF program header. */
static int elf_append_program_header(__Bootstrap_Byte_Buffer__ *buffer,
                                     uint32_t type,
                                     uint32_t flags,
                                     uint64_t offset,
                                     uint64_t virtual_address,
                                     uint64_t file_size,
                                     uint64_t memory_size,
                                     uint64_t align)
{
    return __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, type) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, flags) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, offset) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, virtual_address) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, virtual_address) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, file_size) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, memory_size) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, align);
}

/* Appends one ELF dynamic table entry. */
static int elf_append_dynamic(__Bootstrap_Byte_Buffer__ *buffer, uint64_t tag, uint64_t value)
{
    return __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, tag) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, value);
}

/* Finalizes one LLVM ELF object. */
int __Bootstrap_Finalize_ELF_X86_64__(const uint8_t *object,
                                      size_t object_size,
                                      __Bootstrap_Byte_Buffer__ *output,
                                      char *error,
                                      size_t error_size)
{
    const uint64_t base = 0x400000ULL;
    const size_t page = 0x1000U;
    const size_t interpreter_offset = 0x200U;
    static const char interpreter[] = "/lib64/ld-linux-x86-64.so.2";
    static const char library[] = "libc.so.6";
    __Bootstrap_ELF_Section__ *sections = NULL;
    __Bootstrap_ELF_Symbol__ *symbols = NULL;
    size_t *section_offsets = NULL;
    size_t *external_ordinals = NULL;
    uint32_t *external_name_offsets = NULL;
    const char **external_names = NULL;
    __Bootstrap_ELF_Section__ symtab;
    __Bootstrap_ELF_Section__ strtab;
    uint64_t section_headers;
    size_t section_count;
    size_t symtab_index = SIZE_MAX;
    size_t symbol_count = 0U;
    size_t external_count = 0U;
    size_t main_symbol = SIZE_MAX;
    size_t i;
    size_t cursor;
    size_t start_offset = page;
    size_t plt_offset;
    size_t dynstr_offset;
    size_t dynsym_offset;
    size_t hash_offset;
    size_t rela_plt_offset;
    size_t read_end;
    size_t write_offset;
    size_t got_offset;
    size_t dynamic_offset;
    size_t file_end;
    size_t dynstr_size;
    size_t dynsym_size;
    size_t hash_size;
    size_t rela_plt_size;
    size_t plt_size;
    size_t got_size;
    size_t dynamic_size = 12U * 16U;
    uint64_t main_address = 0U;
    int ok = 1;

    memset(&symtab, 0, sizeof(symtab));
    memset(&strtab, 0, sizeof(strtab));
    if (object == NULL || output == NULL || object_size < 64U)
        return elf_fail(error, error_size, "invalid ELF object");
    if (object[0] != 0x7FU || object[1] != 'E' || object[2] != 'L' || object[3] != 'F' ||
        object[4] != 2U || object[5] != 1U || object[6] != 1U ||
        elf_u16(object, 16U) != 1U || elf_u16(object, 18U) != 62U ||
        elf_u32(object, 20U) != 1U || elf_u16(object, 52U) != 64U ||
        elf_u16(object, 58U) != 64U)
        return elf_fail(error, error_size, "unsupported ELF object header");

    section_headers = elf_u64(object, 40U);
    section_count = (size_t)elf_u16(object, 60U);
    if (section_count == 0U || !elf_bounds(object_size, section_headers, (uint64_t)section_count * 64ULL))
        return elf_fail(error, error_size, "invalid ELF section table");

    sections = (__Bootstrap_ELF_Section__ *)calloc(section_count, sizeof(*sections));
    section_offsets = (size_t *)malloc(section_count * sizeof(*section_offsets));
    if (sections == NULL || section_offsets == NULL)
        ok = 0;
    if (ok)
    {
        for (i = 0U; i < section_count; ++i)
        {
            section_offsets[i] = SIZE_MAX;
            if (!elf_read_section(object, object_size, section_headers, i, &sections[i]))
            {
                ok = 0;
                break;
            }
            if (sections[i].type == ELF_SHT_SYMTAB)
            {
                if (symtab_index != SIZE_MAX)
                {
                    ok = 0;
                    break;
                }
                symtab_index = i;
                symtab = sections[i];
            }
        }
    }
    if (ok && (symtab_index == SIZE_MAX || symtab.entry_size != 24U || symtab.size % 24U != 0U ||
               symtab.link >= section_count || sections[symtab.link].type != ELF_SHT_STRTAB))
        ok = 0;
    if (ok)
    {
        strtab = sections[symtab.link];
        symbol_count = (size_t)(symtab.size / 24U);
        symbols = (__Bootstrap_ELF_Symbol__ *)calloc(symbol_count, sizeof(*symbols));
        external_ordinals = (size_t *)malloc(symbol_count * sizeof(*external_ordinals));
        external_names = (const char **)calloc(symbol_count == 0U ? 1U : symbol_count,
                                                sizeof(*external_names));
        if (symbols == NULL || external_ordinals == NULL || external_names == NULL)
            ok = 0;
    }
    if (ok)
    {
        for (i = 0U; i < symbol_count; ++i)
        {
            const char *name;
            external_ordinals[i] = SIZE_MAX;
            if (!elf_read_symbol(object, object_size, &symtab, i, &symbols[i]))
            {
                ok = 0;
                break;
            }
            name = elf_string(object, object_size, &strtab, symbols[i].name);
            if (name == NULL)
            {
                ok = 0;
                break;
            }
            if (symbols[i].section == ELF_SHN_UNDEF && name[0] != '\0')
            {
                external_ordinals[i] = external_count;
                external_names[external_count] = name;
                ++external_count;
            }
            if (symbols[i].section != ELF_SHN_UNDEF && strcmp(name, "main") == 0)
            {
                if (main_symbol != SIZE_MAX)
                {
                    ok = 0;
                    break;
                }
                main_symbol = i;
            }
        }
    }
    if (ok && main_symbol == SIZE_MAX)
        ok = 0;

    cursor = start_offset + 32U;
    if (ok)
    {
        for (i = 1U; i < section_count; ++i)
        {
            size_t aligned;
            size_t alignment;
            if ((sections[i].flags & ELF_SHF_ALLOC) == 0U || (sections[i].flags & ELF_SHF_WRITE) != 0U)
                continue;
            alignment = sections[i].align == 0U ? 1U : (size_t)sections[i].align;
            if (!elf_align(cursor, alignment, &aligned) || sections[i].size > SIZE_MAX - aligned)
            {
                ok = 0;
                break;
            }
            section_offsets[i] = aligned;
            cursor = aligned + (size_t)sections[i].size;
        }
    }
    if (ok)
    {
        if (!elf_align(cursor, 16U, &plt_offset))
            ok = 0;
        else
        {
            plt_size = (external_count + 1U) * 16U;
            cursor = plt_offset + plt_size;
        }
    }
    if (ok && !elf_align(cursor, 8U, &dynstr_offset))
        ok = 0;

    dynstr_size = 1U + sizeof(library);
    if (ok)
    {
        for (i = 0U; i < external_count; ++i)
        {
            size_t name_size = strlen(external_names[i]) + 1U;
            if (name_size > SIZE_MAX - dynstr_size)
            {
                ok = 0;
                break;
            }
            dynstr_size += name_size;
        }
    }
    if (ok)
        cursor = dynstr_offset + dynstr_size;
    if (ok && !elf_align(cursor, 8U, &dynsym_offset))
        ok = 0;
    if (ok)
    {
        dynsym_size = (external_count + 1U) * 24U;
        cursor = dynsym_offset + dynsym_size;
    }
    if (ok && !elf_align(cursor, 8U, &hash_offset))
        ok = 0;
    if (ok)
    {
        hash_size = (2U + 1U + external_count + 1U) * 4U;
        cursor = hash_offset + hash_size;
    }
    if (ok && !elf_align(cursor, 8U, &rela_plt_offset))
        ok = 0;
    if (ok)
    {
        rela_plt_size = external_count * 24U;
        cursor = rela_plt_offset + rela_plt_size;
    }
    if (ok && !elf_align(cursor, page, &read_end))
        ok = 0;
    if (ok)
    {
        write_offset = read_end;
        cursor = write_offset;
    }

    if (ok)
    {
        for (i = 1U; i < section_count; ++i)
        {
            size_t aligned;
            size_t alignment;
            if ((sections[i].flags & ELF_SHF_ALLOC) == 0U || (sections[i].flags & ELF_SHF_WRITE) == 0U)
                continue;
            alignment = sections[i].align == 0U ? 1U : (size_t)sections[i].align;
            if (!elf_align(cursor, alignment, &aligned) || sections[i].size > SIZE_MAX - aligned)
            {
                ok = 0;
                break;
            }
            section_offsets[i] = aligned;
            cursor = aligned + (size_t)sections[i].size;
        }
    }
    if (ok && !elf_align(cursor, 8U, &got_offset))
        ok = 0;
    if (ok)
    {
        got_size = (external_count + 3U) * 8U;
        cursor = got_offset + got_size;
    }
    if (ok && !elf_align(cursor, 8U, &dynamic_offset))
        ok = 0;
    if (ok)
        file_end = dynamic_offset + dynamic_size;

    if (ok)
    {
        uint16_t main_section = symbols[main_symbol].section;
        if (main_section >= section_count || section_offsets[main_section] == SIZE_MAX ||
            symbols[main_symbol].value > SIZE_MAX - section_offsets[main_section])
            ok = 0;
        else
            main_address = base + section_offsets[main_section] + (size_t)symbols[main_symbol].value;
    }

    if (ok)
    {
        static const uint8_t startup_template[] = {
            0x31,0xED,
            0x48,0x8B,0x3C,0x24,
            0x48,0x8D,0x74,0x24,0x08,
            0x48,0x83,0xE4,0xF0,
            0xE8,0x00,0x00,0x00,0x00,
            0x89,0xC7,
            0xB8,0x3C,0x00,0x00,0x00,
            0x0F,0x05,
            0x0F,0x0B
        };
        uint32_t call_disp;
        uint64_t call_place = base + start_offset + 16U;
        if (!__Bootstrap_Byte_Buffer_Resize_Zero__(output, file_end) ||
            !elf_relative32(call_place, main_address, -4, &call_disp))
            ok = 0;
        if (ok)
        {
            memcpy(output->data + start_offset, startup_template, sizeof(startup_template));
            if (!__Bootstrap_Byte_Buffer_Write_U32_LE__(output, start_offset + 16U, call_disp))
                ok = 0;
        }
    }

    if (ok)
    {
        for (i = 1U; i < section_count; ++i)
        {
            size_t destination = section_offsets[i];
            if (destination == SIZE_MAX || sections[i].size == 0U)
                continue;
            if (sections[i].type != ELF_SHT_NOBITS)
                memcpy(output->data + destination, object + (size_t)sections[i].offset,
                       (size_t)sections[i].size);
        }
    }

    external_name_offsets = ok ? (uint32_t *)calloc(external_count == 0U ? 1U : external_count,
                                                     sizeof(*external_name_offsets)) : NULL;
    if (ok && external_name_offsets == NULL)
        ok = 0;
    if (ok)
    {
        size_t position = dynstr_offset;
        output->data[position++] = 0U;
        memcpy(output->data + position, library, sizeof(library));
        position += sizeof(library);
        for (i = 0U; i < external_count; ++i)
        {
            size_t name_size = strlen(external_names[i]) + 1U;
            external_name_offsets[i] = (uint32_t)(position - dynstr_offset);
            memcpy(output->data + position, external_names[i], name_size);
            position += name_size;
        }
        if (position != dynstr_offset + dynstr_size)
            ok = 0;
    }

    if (ok)
    {
        size_t position = dynsym_offset + 24U;
        for (i = 0U; i < external_count; ++i)
        {
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, external_name_offsets[i]);
            output->data[position + 4U] = 0x12U;
            output->data[position + 5U] = 0U;
            output->data[position + 6U] = 0U;
            output->data[position + 7U] = 0U;
            position += 24U;
        }
    }

    if (ok)
    {
        size_t position = hash_offset;
        size_t chain;
        __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, 1U);
        position += 4U;
        __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, (uint32_t)(external_count + 1U));
        position += 4U;
        __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, external_count == 0U ? 0U : 1U);
        position += 4U;
        __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, 0U);
        position += 4U;
        for (chain = 1U; chain <= external_count; ++chain)
        {
            uint32_t next = chain < external_count ? (uint32_t)(chain + 1U) : 0U;
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, next);
            position += 4U;
        }
    }

    if (ok)
    {
        size_t position = plt_offset;
        uint32_t disp;
        uint64_t plt_address = base + plt_offset;
        uint64_t got_address = base + got_offset;
        output->data[position++] = 0xFFU;
        output->data[position++] = 0x35U;
        if (!elf_relative32(plt_address + 2U, got_address + 8U, -4, &disp))
            ok = 0;
        if (ok)
        {
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, disp);
            position += 4U;
            output->data[position++] = 0xFFU;
            output->data[position++] = 0x25U;
        }
        if (ok && !elf_relative32(plt_address + 8U, got_address + 16U, -4, &disp))
            ok = 0;
        if (ok)
        {
            static const uint8_t padding[4] = {0x0FU,0x1FU,0x40U,0x00U};
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, disp);
            position += 4U;
            memcpy(output->data + position, padding, sizeof(padding));
            position += sizeof(padding);
        }
        for (i = 0U; ok && i < external_count; ++i)
        {
            uint64_t entry_address = base + position;
            uint64_t got_entry = got_address + (i + 3U) * 8U;
            output->data[position++] = 0xFFU;
            output->data[position++] = 0x25U;
            if (!elf_relative32(entry_address + 2U, got_entry, -4, &disp))
            {
                ok = 0;
                break;
            }
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, disp);
            position += 4U;
            output->data[position++] = 0x68U;
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, (uint32_t)i);
            position += 4U;
            output->data[position++] = 0xE9U;
            if (!elf_relative32(entry_address + 12U, plt_address, -4, &disp))
            {
                ok = 0;
                break;
            }
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, position, disp);
            position += 4U;
        }
        if (ok && position != plt_offset + plt_size)
            ok = 0;
    }

    if (ok)
    {
        __Bootstrap_Byte_Buffer_Write_U64_LE__(output, got_offset, base + dynamic_offset);
        __Bootstrap_Byte_Buffer_Write_U64_LE__(output, got_offset + 8U, 0U);
        __Bootstrap_Byte_Buffer_Write_U64_LE__(output, got_offset + 16U, 0U);
        for (i = 0U; i < external_count; ++i)
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, got_offset + (i + 3U) * 8U,
                                                   base + plt_offset + (i + 1U) * 16U + 6U);
    }

    if (ok)
    {
        for (i = 0U; i < external_count; ++i)
        {
            size_t offset = rela_plt_offset + i * 24U;
            uint64_t info = ((uint64_t)(i + 1U) << 32U) | 7U;
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, offset,
                                                   base + got_offset + (i + 3U) * 8U);
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, offset + 8U, info);
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, offset + 16U, 0U);
        }
    }

    if (ok)
    {
        __Bootstrap_Byte_Buffer__ dynamic;
        __Bootstrap_Byte_Buffer_Init__(&dynamic);
        ok = elf_append_dynamic(&dynamic, 1U, 1U) &&
             elf_append_dynamic(&dynamic, 4U, base + hash_offset) &&
             elf_append_dynamic(&dynamic, 5U, base + dynstr_offset) &&
             elf_append_dynamic(&dynamic, 6U, base + dynsym_offset) &&
             elf_append_dynamic(&dynamic, 10U, dynstr_size) &&
             elf_append_dynamic(&dynamic, 11U, 24U) &&
             elf_append_dynamic(&dynamic, 3U, base + got_offset) &&
             elf_append_dynamic(&dynamic, 2U, rela_plt_size) &&
             elf_append_dynamic(&dynamic, 20U, 7U) &&
             elf_append_dynamic(&dynamic, 23U, base + rela_plt_offset) &&
             elf_append_dynamic(&dynamic, 9U, 24U) &&
             elf_append_dynamic(&dynamic, 0U, 0U);
        if (ok && dynamic.size == dynamic_size)
            memcpy(output->data + dynamic_offset, dynamic.data, dynamic.size);
        else
            ok = 0;
        __Bootstrap_Byte_Buffer_Destroy__(&dynamic);
    }

    if (ok)
    {
        for (i = 1U; i < section_count; ++i)
        {
            const __Bootstrap_ELF_Section__ *relocations = &sections[i];
            size_t count;
            size_t r;
            size_t target_section;
            if (relocations->type != ELF_SHT_RELA)
                continue;
            target_section = relocations->info;
            if (target_section >= section_count || section_offsets[target_section] == SIZE_MAX)
                continue;
            if (relocations->entry_size != 24U || relocations->size % 24U != 0U ||
                relocations->link != symtab_index)
            {
                ok = 0;
                break;
            }
            count = (size_t)(relocations->size / 24U);
            for (r = 0U; r < count; ++r)
            {
                size_t relocation_position = (size_t)relocations->offset + r * 24U;
                uint64_t relocation_offset = elf_u64(object, relocation_position);
                uint64_t info = elf_u64(object, relocation_position + 8U);
                int64_t addend = (int64_t)elf_u64(object, relocation_position + 16U);
                size_t symbol_index = (size_t)(info >> 32U);
                uint32_t type = (uint32_t)info;
                uint64_t target_address;
                size_t patch_offset;
                uint32_t relative;
                __Bootstrap_ELF_Symbol__ *symbol;

                if (symbol_index >= symbol_count || relocation_offset > sections[target_section].size ||
                    4U > sections[target_section].size - relocation_offset)
                {
                    ok = 0;
                    break;
                }
                patch_offset = section_offsets[target_section] + (size_t)relocation_offset;
                symbol = &symbols[symbol_index];
                if (symbol->section == ELF_SHN_UNDEF)
                {
                    size_t ordinal = external_ordinals[symbol_index];
                    if (ordinal == SIZE_MAX)
                    {
                        ok = 0;
                        break;
                    }
                    if (type == ELF_R_X86_64_REX_GOTPCRELX)
                        target_address = base + got_offset + (ordinal + 3U) * 8U;
                    else
                        target_address = base + plt_offset + (ordinal + 1U) * 16U;
                }
                else if (symbol->section == ELF_SHN_ABS)
                {
                    target_address = symbol->value;
                }
                else if (symbol->section < section_count && section_offsets[symbol->section] != SIZE_MAX)
                {
                    target_address = base + section_offsets[symbol->section] + symbol->value;
                }
                else
                {
                    ok = 0;
                    break;
                }

                if (type == ELF_R_X86_64_PC32 || type == ELF_R_X86_64_PLT32 ||
                    type == ELF_R_X86_64_REX_GOTPCRELX)
                {
                    if (type == ELF_R_X86_64_REX_GOTPCRELX && symbol->section != ELF_SHN_UNDEF)
                    {
                        if (patch_offset < 2U || output->data[patch_offset - 2U] != 0x8BU)
                        {
                            ok = 0;
                            break;
                        }
                        output->data[patch_offset - 2U] = 0x8DU;
                    }
                    if (!elf_relative32(base + patch_offset, target_address, addend, &relative) ||
                        !__Bootstrap_Byte_Buffer_Write_U32_LE__(output, patch_offset, relative))
                    {
                        ok = 0;
                        break;
                    }
                }
                else if (type == ELF_R_X86_64_64 || type == ELF_R_X86_64_32 ||
                         type == ELF_R_X86_64_32S)
                {
                    if (!elf_write_absolute(output, patch_offset, type, target_address, addend))
                    {
                        ok = 0;
                        break;
                    }
                }
                else
                {
                    ok = 0;
                    break;
                }
            }
            if (!ok)
                break;
        }
    }

    if (ok)
    {
        size_t ph_size = 4U * 56U;
        size_t interp_size = sizeof(interpreter);
        size_t header_end = 64U + ph_size;
        if (header_end > interpreter_offset || interpreter_offset + interp_size > start_offset)
            ok = 0;
        if (ok)
        {
            size_t p = 0U;
            __Bootstrap_Byte_Buffer__ headers;
            output->data[p++] = 0x7FU;
            output->data[p++] = 'E';
            output->data[p++] = 'L';
            output->data[p++] = 'F';
            output->data[p++] = 2U;
            output->data[p++] = 1U;
            output->data[p++] = 1U;
            output->data[p++] = 0U;
            memset(output->data + p, 0, 8U);
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, 16U,
                                                   (uint32_t)2U | ((uint32_t)62U << 16U));
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, 20U, 1U);
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, 24U, base + start_offset);
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, 32U, 64U);
            __Bootstrap_Byte_Buffer_Write_U64_LE__(output, 40U, 0U);
            __Bootstrap_Byte_Buffer_Write_U32_LE__(output, 48U, 0U);
            output->data[52U] = 64U;
            output->data[53U] = 0U;
            output->data[54U] = 56U;
            output->data[55U] = 0U;
            output->data[56U] = 4U;
            output->data[57U] = 0U;
            output->data[58U] = 0U;
            output->data[59U] = 0U;
            output->data[60U] = 0U;
            output->data[61U] = 0U;
            output->data[62U] = 0U;
            output->data[63U] = 0U;

            __Bootstrap_Byte_Buffer_Init__(&headers);
            ok = elf_append_program_header(&headers, 1U, 5U, 0U, base, read_end, read_end, page) &&
                 elf_append_program_header(&headers, 1U, 6U, write_offset, base + write_offset,
                                           file_end - write_offset, file_end - write_offset, page) &&
                 elf_append_program_header(&headers, 3U, 4U, interpreter_offset,
                                           base + interpreter_offset, interp_size, interp_size, 1U) &&
                 elf_append_program_header(&headers, 2U, 6U, dynamic_offset,
                                           base + dynamic_offset, dynamic_size, dynamic_size, 8U);
            if (ok && headers.size == ph_size)
                memcpy(output->data + 64U, headers.data, headers.size);
            else
                ok = 0;
            __Bootstrap_Byte_Buffer_Destroy__(&headers);
            if (ok)
                memcpy(output->data + interpreter_offset, interpreter, interp_size);
        }
    }

    free(external_name_offsets);
    free(external_names);
    free(external_ordinals);
    free(symbols);
    free(section_offsets);
    free(sections);

    if (!ok)
    {
        __Bootstrap_Byte_Buffer_Destroy__(output);
        return elf_fail(error, error_size, "unable to finalize LLVM ELF object");
    }
    return 1;
}
