/* Finalizes LLVM ARM64 Mach-O objects into bootstrap executables. */

#include "link/macho_executable.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Describes one input Mach-O section. */
typedef struct
{
    int exists;
    uint32_t index;
    uint64_t address;
    uint64_t size;
    uint32_t file_offset;
    uint32_t align_power;
    uint32_t relocation_offset;
    uint32_t relocation_count;
} __Bootstrap_MachO_Section__;

/* Describes one input Mach-O symbol. */
typedef struct
{
    const uint8_t *name;
    size_t name_size;
    uint8_t section;
    uint64_t value;
} __Bootstrap_MachO_Symbol__;

/* Stores one SHA-256 state. */
typedef struct
{
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t block[64];
    size_t block_size;
} __Bootstrap_SHA256__;

static int macho_fail(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size != 0U)
        snprintf(error, error_size, "%s", message);
    return 0;
}

static int macho_bounds(size_t total, uint64_t offset, uint64_t size)
{
    if (offset > (uint64_t)total)
        return 0;
    return size <= (uint64_t)total - offset;
}

static uint32_t macho_u32(const uint8_t *data, size_t offset)
{
    return (uint32_t)data[offset] | ((uint32_t)data[offset + 1U] << 8U) |
           ((uint32_t)data[offset + 2U] << 16U) | ((uint32_t)data[offset + 3U] << 24U);
}

static uint64_t macho_u64(const uint8_t *data, size_t offset)
{
    return (uint64_t)macho_u32(data, offset) | ((uint64_t)macho_u32(data, offset + 4U) << 32U);
}

static int macho_align(size_t value, size_t alignment, size_t *result)
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

static int macho_name_equals(const uint8_t *field, const char *name)
{
    size_t i;
    size_t length = strlen(name);
    if (length > 16U)
        return 0;
    for (i = 0U; i < 16U; ++i)
    {
        uint8_t expected = i < length ? (uint8_t)name[i] : 0U;
        if (field[i] != expected)
            return 0;
    }
    return 1;
}

static int macho_append_name(__Bootstrap_Byte_Buffer__ *buffer, const char *name)
{
    uint8_t field[16];
    size_t length = strlen(name);
    if (length > sizeof(field))
        return 0;
    memset(field, 0, sizeof(field));
    memcpy(field, name, length);
    return __Bootstrap_Byte_Buffer_Append__(buffer, field, sizeof(field));
}

static int macho_append_segment(__Bootstrap_Byte_Buffer__ *buffer,
                                const char *name,
                                uint64_t address,
                                uint64_t virtual_size,
                                uint64_t file_offset,
                                uint64_t file_size,
                                uint32_t max_protection,
                                uint32_t initial_protection,
                                uint32_t section_count)
{
    return __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0x19U) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 72U + section_count * 80U) &&
           macho_append_name(buffer, name) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, address) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, virtual_size) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, file_offset) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, file_size) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, max_protection) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, initial_protection) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, section_count) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0U);
}

static int macho_append_section(__Bootstrap_Byte_Buffer__ *buffer,
                                const char *name,
                                const char *segment,
                                uint64_t address,
                                uint64_t size,
                                uint32_t file_offset,
                                uint32_t align_power,
                                uint32_t flags)
{
    return macho_append_name(buffer, name) && macho_append_name(buffer, segment) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, address) &&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(buffer, size) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, file_offset) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, align_power) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0U) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0U) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, flags) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0U) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0U) &&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(buffer, 0U);
}

static int macho_append_path(__Bootstrap_Byte_Buffer__ *buffer, const char *path)
{
    size_t length = strlen(path) + 1U;
    return __Bootstrap_Byte_Buffer_Append__(buffer, path, length);
}

static int macho_append_uleb(__Bootstrap_Byte_Buffer__ *buffer, uint64_t value)
{
    do
    {
        uint8_t byte = (uint8_t)(value & 0x7FU);
        value >>= 7U;
        if (value != 0U)
            byte |= 0x80U;
        if (!__Bootstrap_Byte_Buffer_Append_U8__(buffer, byte))
            return 0;
    } while (value != 0U);
    return 1;
}

static int macho_page_delta(uint64_t source, uint64_t target, uint32_t *encoded)
{
    uint64_t from_page;
    uint64_t to_page;
    uint64_t difference;
    if (encoded == NULL)
        return 0;
    from_page = source / 4096U;
    to_page = target / 4096U;
    if (to_page >= from_page)
    {
        difference = to_page - from_page;
        if (difference > 0xFFFFFU)
            return 0;
        *encoded = (uint32_t)difference;
        return 1;
    }
    difference = from_page - to_page;
    if (difference > 0x100000U)
        return 0;
    *encoded = (uint32_t)(0x200000U - difference);
    return 1;
}

static int macho_patch_branch26(__Bootstrap_Byte_Buffer__ *buffer,
                                size_t offset,
                                uint64_t source,
                                uint64_t target)
{
    uint32_t original;
    uint64_t words;
    uint32_t encoded;
    if (buffer == NULL || offset > buffer->size || 4U > buffer->size - offset)
        return 0;
    if (target >= source)
    {
        uint64_t difference = target - source;
        if ((difference % 4U) != 0U || difference / 4U > 0x1FFFFFFU)
            return 0;
        words = difference / 4U;
    }
    else
    {
        uint64_t difference = source - target;
        if ((difference % 4U) != 0U || difference / 4U > 0x2000000U)
            return 0;
        words = (0x4000000U - (difference / 4U)) % 0x4000000U;
    }
    original = macho_u32(buffer->data, offset);
    encoded = (original & 0xFC000000U) | ((uint32_t)words & 0x03FFFFFFU);
    return __Bootstrap_Byte_Buffer_Write_U32_LE__(buffer, offset, encoded);
}

static int macho_patch_page21(__Bootstrap_Byte_Buffer__ *buffer,
                              size_t offset,
                              uint64_t source,
                              uint64_t target)
{
    uint32_t delta;
    uint32_t original;
    uint32_t encoded;
    if (buffer == NULL || offset > buffer->size || 4U > buffer->size - offset ||
        !macho_page_delta(source, target, &delta))
        return 0;
    original = macho_u32(buffer->data, offset);
    encoded = (original & 0x9F00001FU) | ((delta & 3U) << 29U) |
              (((delta >> 2U) & 0x7FFFFU) << 5U);
    return __Bootstrap_Byte_Buffer_Write_U32_LE__(buffer, offset, encoded);
}

static int macho_patch_pageoff12(__Bootstrap_Byte_Buffer__ *buffer,
                                 size_t offset,
                                 uint64_t target)
{
    uint32_t original;
    uint32_t immediate;
    uint32_t category;
    uint32_t encoded;
    uint64_t low = target % 4096U;
    if (buffer == NULL || offset > buffer->size || 4U > buffer->size - offset)
        return 0;
    original = macho_u32(buffer->data, offset);
    immediate = (uint32_t)low;
    category = original & 0x3B000000U;
    if (category == 0x39000000U)
    {
        uint32_t scale_power = (original >> 30U) & 3U;
        uint32_t scale = 1U << scale_power;
        if ((low % scale) != 0U)
            return 0;
        immediate = (uint32_t)(low / scale);
    }
    encoded = (original & 0xFFC003FFU) | ((immediate & 0xFFFU) << 10U);
    return __Bootstrap_Byte_Buffer_Write_U32_LE__(buffer, offset, encoded);
}

static uint32_t sha_rotr(uint32_t value, unsigned amount)
{
    return (value >> amount) | (value << (32U - amount));
}

static void sha_transform(__Bootstrap_SHA256__ *context, const uint8_t block[64])
{
    static const uint32_t constants[64] = {
        0x428A2F98U,0x71374491U,0xB5C0FBCFU,0xE9B5DBA5U,0x3956C25BU,0x59F111F1U,0x923F82A4U,0xAB1C5ED5U,
        0xD807AA98U,0x12835B01U,0x243185BEU,0x550C7DC3U,0x72BE5D74U,0x80DEB1FEU,0x9BDC06A7U,0xC19BF174U,
        0xE49B69C1U,0xEFBE4786U,0x0FC19DC6U,0x240CA1CCU,0x2DE92C6FU,0x4A7484AAU,0x5CB0A9DCU,0x76F988DAU,
        0x983E5152U,0xA831C66DU,0xB00327C8U,0xBF597FC7U,0xC6E00BF3U,0xD5A79147U,0x06CA6351U,0x14292967U,
        0x27B70A85U,0x2E1B2138U,0x4D2C6DFCU,0x53380D13U,0x650A7354U,0x766A0ABBU,0x81C2C92EU,0x92722C85U,
        0xA2BFE8A1U,0xA81A664BU,0xC24B8B70U,0xC76C51A3U,0xD192E819U,0xD6990624U,0xF40E3585U,0x106AA070U,
        0x19A4C116U,0x1E376C08U,0x2748774CU,0x34B0BCB5U,0x391C0CB3U,0x4ED8AA4AU,0x5B9CCA4FU,0x682E6FF3U,
        0x748F82EEU,0x78A5636FU,0x84C87814U,0x8CC70208U,0x90BEFFFAU,0xA4506CEBU,0xBEF9A3F7U,0xC67178F2U
    };
    uint32_t words[64];
    uint32_t a,b,c,d,e,f,g,h;
    size_t i;
    for (i = 0U; i < 16U; ++i)
    {
        size_t p = i * 4U;
        words[i] = ((uint32_t)block[p] << 24U) | ((uint32_t)block[p + 1U] << 16U) |
                   ((uint32_t)block[p + 2U] << 8U) | (uint32_t)block[p + 3U];
    }
    for (i = 16U; i < 64U; ++i)
    {
        uint32_t s0 = sha_rotr(words[i - 15U],7U) ^ sha_rotr(words[i - 15U],18U) ^ (words[i - 15U] >> 3U);
        uint32_t s1 = sha_rotr(words[i - 2U],17U) ^ sha_rotr(words[i - 2U],19U) ^ (words[i - 2U] >> 10U);
        words[i] = words[i - 16U] + s0 + words[i - 7U] + s1;
    }
    a=context->state[0]; b=context->state[1]; c=context->state[2]; d=context->state[3];
    e=context->state[4]; f=context->state[5]; g=context->state[6]; h=context->state[7];
    for (i = 0U; i < 64U; ++i)
    {
        uint32_t s1=sha_rotr(e,6U)^sha_rotr(e,11U)^sha_rotr(e,25U);
        uint32_t choose=(e&f)^((~e)&g);
        uint32_t temp1=h+s1+choose+constants[i]+words[i];
        uint32_t s0=sha_rotr(a,2U)^sha_rotr(a,13U)^sha_rotr(a,22U);
        uint32_t majority=(a&b)^(a&c)^(b&c);
        uint32_t temp2=s0+majority;
        h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
    }
    context->state[0]+=a; context->state[1]+=b; context->state[2]+=c; context->state[3]+=d;
    context->state[4]+=e; context->state[5]+=f; context->state[6]+=g; context->state[7]+=h;
}

static void sha_init(__Bootstrap_SHA256__ *context)
{
    static const uint32_t initial[8] = {
        0x6A09E667U,0xBB67AE85U,0x3C6EF372U,0xA54FF53AU,
        0x510E527FU,0x9B05688CU,0x1F83D9ABU,0x5BE0CD19U
    };
    memcpy(context->state, initial, sizeof(initial));
    context->bit_count = 0U;
    context->block_size = 0U;
}

static void sha_update(__Bootstrap_SHA256__ *context, const uint8_t *data, size_t size)
{
    size_t i;
    for (i = 0U; i < size; ++i)
    {
        context->block[context->block_size++] = data[i];
        if (context->block_size == 64U)
        {
            sha_transform(context, context->block);
            context->bit_count += 512U;
            context->block_size = 0U;
        }
    }
}

static void sha_finish(__Bootstrap_SHA256__ *context, uint8_t output[32])
{
    uint64_t bits = context->bit_count + (uint64_t)context->block_size * 8U;
    size_t i;
    context->block[context->block_size++] = 0x80U;
    if (context->block_size > 56U)
    {
        while (context->block_size < 64U)
            context->block[context->block_size++] = 0U;
        sha_transform(context, context->block);
        context->block_size = 0U;
    }
    while (context->block_size < 56U)
        context->block[context->block_size++] = 0U;
    for (i = 0U; i < 8U; ++i)
        context->block[56U + i] = (uint8_t)(bits >> ((7U - i) * 8U));
    sha_transform(context, context->block);
    for (i = 0U; i < 8U; ++i)
    {
        output[i * 4U] = (uint8_t)(context->state[i] >> 24U);
        output[i * 4U + 1U] = (uint8_t)(context->state[i] >> 16U);
        output[i * 4U + 2U] = (uint8_t)(context->state[i] >> 8U);
        output[i * 4U + 3U] = (uint8_t)context->state[i];
    }
}

static void macho_sha256(const uint8_t *data, size_t size, uint8_t output[32])
{
    __Bootstrap_SHA256__ context;
    sha_init(&context);
    sha_update(&context, data, size);
    sha_finish(&context, output);
}

static int macho_build_signature(const __Bootstrap_Byte_Buffer__ *image,
                                 __Bootstrap_Byte_Buffer__ *signature)
{
    static const uint8_t identifier[] = {'s','u','l','t','a','n','c',0};
    const size_t page_size = 4096U;
    size_t code_limit;
    size_t page_count;
    size_t hash_offset = 44U + sizeof(identifier);
    size_t code_directory_size;
    size_t total_size;
    size_t page;

    if (image == NULL || signature == NULL || image->size > UINT32_MAX)
        return 0;
    code_limit = image->size;
    page_count = (code_limit + page_size - 1U) / page_size;
    if (page_count > (SIZE_MAX - hash_offset) / 32U)
        return 0;
    code_directory_size = hash_offset + page_count * 32U;
    if (code_directory_size > UINT32_MAX || code_directory_size > SIZE_MAX - 20U)
        return 0;
    total_size = 20U + code_directory_size;
    if (total_size > UINT32_MAX)
        return 0;

    __Bootstrap_Byte_Buffer_Init__(signature);
    if (!__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0xFADE0CC0U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,(uint32_t)total_size) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,1U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,20U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0xFADE0C02U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,(uint32_t)code_directory_size) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0x00020001U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0x00000002U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,(uint32_t)hash_offset) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,44U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,(uint32_t)page_count) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,(uint32_t)code_limit) ||
        !__Bootstrap_Byte_Buffer_Append_U8__(signature,32U) ||
        !__Bootstrap_Byte_Buffer_Append_U8__(signature,2U) ||
        !__Bootstrap_Byte_Buffer_Append_U8__(signature,0U) ||
        !__Bootstrap_Byte_Buffer_Append_U8__(signature,12U) ||
        !__Bootstrap_Byte_Buffer_Append_U32_BE__(signature,0U) ||
        !__Bootstrap_Byte_Buffer_Append__(signature,identifier,sizeof(identifier)))
    {
        __Bootstrap_Byte_Buffer_Destroy__(signature);
        return 0;
    }
    for (page = 0U; page < page_count; ++page)
    {
        size_t begin = page * page_size;
        size_t length = code_limit - begin;
        uint8_t hash[32];
        if (length > page_size)
            length = page_size;
        macho_sha256(image->data + begin, length, hash);
        if (!__Bootstrap_Byte_Buffer_Append__(signature,hash,sizeof(hash)))
        {
            __Bootstrap_Byte_Buffer_Destroy__(signature);
            return 0;
        }
    }
    return signature->size == total_size;
}

static int macho_symbol_name(const uint8_t *object,
                             size_t object_size,
                             uint32_t string_offset,
                             uint32_t string_size,
                             uint32_t name_index,
                             const uint8_t **name,
                             size_t *name_size)
{
    size_t begin;
    size_t end;
    size_t position;
    if (name == NULL || name_size == NULL || name_index >= string_size)
        return 0;
    if ((uint64_t)string_offset + (uint64_t)name_index > (uint64_t)SIZE_MAX)
        return 0;
    begin = (size_t)string_offset + name_index;
    end = (size_t)string_offset + string_size;
    if (end > object_size || begin >= end)
        return 0;
    position = begin;
    while (position < end && object[position] != 0U)
        ++position;
    if (position == end)
        return 0;
    *name = object + begin;
    *name_size = position - begin;
    return 1;
}

static int macho_symbol_equals(const __Bootstrap_MachO_Symbol__ *symbol, const char *name)
{
    size_t length = strlen(name);
    return symbol != NULL && symbol->name_size == length &&
           memcmp(symbol->name,name,length) == 0;
}

static int macho_external_ordinal(const size_t *externals,
                                  size_t external_count,
                                  size_t symbol_index,
                                  size_t *ordinal)
{
    size_t i;
    for (i = 0U; i < external_count; ++i)
    {
        if (externals[i] == symbol_index)
        {
            if (ordinal != NULL)
                *ordinal = i;
            return 1;
        }
    }
    return 0;
}

static uint64_t macho_symbol_target(const __Bootstrap_MachO_Symbol__ *symbol,
                                    const __Bootstrap_MachO_Section__ *text,
                                    size_t text_start,
                                    const __Bootstrap_MachO_Section__ *constants,
                                    size_t const_start,
                                    const __Bootstrap_MachO_Section__ *strings,
                                    size_t strings_start,
                                    const __Bootstrap_MachO_Section__ *data,
                                    size_t data_start,
                                    const __Bootstrap_MachO_Section__ *common,
                                    size_t common_start,
                                    const __Bootstrap_MachO_Section__ *bss,
                                    size_t bss_start,
                                    uint64_t base)
{
    const __Bootstrap_MachO_Section__ *sections[6] = {text,constants,strings,data,common,bss};
    size_t starts[6] = {text_start,const_start,strings_start,data_start,common_start,bss_start};
    size_t i;
    for (i = 0U; i < 6U; ++i)
    {
        if (sections[i]->exists && symbol->section == sections[i]->index &&
            symbol->value >= sections[i]->address &&
            symbol->value < sections[i]->address + sections[i]->size)
            return base + starts[i] + (symbol->value - sections[i]->address);
    }
    return 0U;
}

int __Bootstrap_Finalize_MachO_ARM64__(const uint8_t *object,
                                       size_t object_size,
                                       __Bootstrap_Byte_Buffer__ *output,
                                       char *error,
                                       size_t error_size)
{
    const uint64_t base = 0x100000000ULL;
    __Bootstrap_MachO_Section__ text={0}, constants={0}, strings={0}, data={0}, common={0}, bss={0};
    __Bootstrap_MachO_Symbol__ *symbols = NULL;
    size_t *externals = NULL;
    uint32_t *external_name_offsets = NULL;
    uint32_t symbol_offset=0U, symbol_count32=0U, string_offset=0U, string_size=0U;
    uint32_t command_count, command_size;
    uint32_t section_index=0U;
    size_t cursor=32U;
    size_t command;
    size_t symbol_count;
    size_t external_count=0U;
    size_t entry_symbol=SIZE_MAX;
    size_t i;
    size_t load_commands_size;
    size_t text_start=0U,const_start=0U,strings_start=0U,stub_start=0U,text_end=0U;
    size_t data_segment_start=0U,data_source_start=0U,common_start=0U,bss_start=0U,got_start=0U,note_start=0U,data_end=0U;
    size_t const_size,string_size_out,data_size,common_size,bss_size,stub_size,got_size;
    size_t const_align,string_align,data_align,common_align,bss_align;
    size_t link_start,bind_start,external_symbols_start,external_strings_start,signature_start;
    size_t link_command_offset,uuid_offset,signature_size_offset;
    __Bootstrap_Byte_Buffer__ bind, external_strings, preliminary_signature, signature;
    int ok=1;

    __Bootstrap_Byte_Buffer_Init__(&bind);
    __Bootstrap_Byte_Buffer_Init__(&external_strings);
    __Bootstrap_Byte_Buffer_Init__(&preliminary_signature);
    __Bootstrap_Byte_Buffer_Init__(&signature);
    if (object == NULL || output == NULL || object_size < 32U ||
        macho_u32(object,0U) != 0xFEEDFACFU || macho_u32(object,4U) != 0x0100000CU ||
        macho_u32(object,12U) != 1U)
        return macho_fail(error,error_size,"unsupported Mach-O object header");
    command_count=macho_u32(object,16U);
    command_size=macho_u32(object,20U);
    if (!macho_bounds(object_size,32U,command_size))
        return macho_fail(error,error_size,"invalid Mach-O load commands");

    for (command=0U; ok && command<command_count; ++command)
    {
        uint32_t type,size;
        if (!macho_bounds(object_size,cursor,8U)) { ok=0; break; }
        type=macho_u32(object,cursor); size=macho_u32(object,cursor+4U);
        if (size<8U || !macho_bounds(object_size,cursor,size)) { ok=0; break; }
        if (type==0x19U)
        {
            uint32_t count;
            size_t s;
            if (size<72U) { ok=0; break; }
            count=macho_u32(object,cursor+64U);
            if ((uint64_t)72U+(uint64_t)count*80U != size) { ok=0; break; }
            for (s=0U; s<count; ++s)
            {
                size_t p=cursor+72U+s*80U;
                uint32_t flags=macho_u32(object,p+64U);
                uint32_t kind=flags&0xFFU;
                int zero=kind==1U || kind==0xCU || kind==0x12U;
                __Bootstrap_MachO_Section__ section;
                ++section_index;
                memset(&section,0,sizeof(section));
                section.exists=1; section.index=section_index;
                section.address=macho_u64(object,p+32U); section.size=macho_u64(object,p+40U);
                section.file_offset=macho_u32(object,p+48U); section.align_power=macho_u32(object,p+52U);
                section.relocation_offset=macho_u32(object,p+56U); section.relocation_count=macho_u32(object,p+60U);
                if (section.align_power>20U || (!zero && !macho_bounds(object_size,section.file_offset,section.size)) ||
                    !macho_bounds(object_size,section.relocation_offset,(uint64_t)section.relocation_count*8U)) { ok=0; break; }
#define ASSIGN_SEC(NAME, FIELD) if (macho_name_equals(object+p,NAME)) { if ((FIELD).exists) { ok=0; break; } (FIELD)=section; }
                ASSIGN_SEC("__text",text)
                ASSIGN_SEC("__const",constants)
                ASSIGN_SEC("__cstring",strings)
                ASSIGN_SEC("__data",data)
                ASSIGN_SEC("__common",common)
                ASSIGN_SEC("__bss",bss)
#undef ASSIGN_SEC
            }
        }
        else if (type==2U)
        {
            if (size!=24U || symbol_count32!=0U) { ok=0; break; }
            symbol_offset=macho_u32(object,cursor+8U); symbol_count32=macho_u32(object,cursor+12U);
            string_offset=macho_u32(object,cursor+16U); string_size=macho_u32(object,cursor+20U);
        }
        cursor+=size;
    }
    if (!ok || cursor!=32U+command_size || !text.exists || symbol_count32==0U ||
        !macho_bounds(object_size,symbol_offset,(uint64_t)symbol_count32*16U) ||
        !macho_bounds(object_size,string_offset,string_size))
        return macho_fail(error,error_size,"invalid Mach-O object structure");

    symbol_count=symbol_count32;
    symbols=(__Bootstrap_MachO_Symbol__*)calloc(symbol_count,sizeof(*symbols));
    externals=(size_t*)malloc(symbol_count*sizeof(*externals));
    if (symbols==NULL || externals==NULL) ok=0;
    for (i=0U; ok && i<symbol_count; ++i)
    {
        size_t p=(size_t)symbol_offset+i*16U;
        uint32_t name_index=macho_u32(object,p);
        uint8_t type=object[p+4U], section=object[p+5U];
        uint8_t type_class=type&0x0EU;
        if (section>section_index || (section==0U && type_class!=0U) || (section!=0U && type_class!=0x0EU) ||
            !macho_symbol_name(object,object_size,string_offset,string_size,name_index,&symbols[i].name,&symbols[i].name_size)) { ok=0; break; }
        symbols[i].section=section; symbols[i].value=macho_u64(object,p+8U);
        if (section==0U && symbols[i].name_size!=0U) externals[external_count++]=i;
        if (section==text.index && macho_symbol_equals(&symbols[i],"_main"))
        {
            if (entry_symbol!=SIZE_MAX || symbols[i].value<text.address || symbols[i].value>=text.address+text.size) { ok=0; break; }
            entry_symbol=i;
        }
    }
    if (!ok || entry_symbol==SIZE_MAX)
    {
        free(externals); free(symbols);
        return macho_fail(error,error_size,"Mach-O object has no valid _main entry");
    }

    load_commands_size=external_count>0U?1016U:968U;
    if (data.exists) load_commands_size+=80U;
    if (!macho_align(32U+load_commands_size,16U,&text_start)) ok=0;
    const_align=8U;
    if (constants.exists && ((size_t)1U<<constants.align_power)>const_align) const_align=(size_t)1U<<constants.align_power;
    if (ok && !macho_align(text_start+(size_t)text.size,const_align,&const_start)) ok=0;
    const_size=constants.exists?(size_t)constants.size:0U;
    string_align=strings.exists?((size_t)1U<<strings.align_power):1U;
    if (ok && !macho_align(const_start+const_size,string_align,&strings_start)) ok=0;
    string_size_out=strings.exists?(size_t)strings.size:0U;
    if (ok && !macho_align(strings_start+string_size_out,4U,&stub_start)) ok=0;
    stub_size=external_count*12U;
    if (ok && !macho_align(stub_start+stub_size,0x4000U,&text_end)) ok=0;
    data_segment_start=text_end;
    data_align=data.exists?((size_t)1U<<data.align_power):1U;
    if (ok && !macho_align(data_segment_start,data_align,&data_source_start)) ok=0;
    data_size=data.exists?(size_t)data.size:0U;
    common_align=common.exists?((size_t)1U<<common.align_power):1U;
    if (ok && !macho_align(data_source_start+data_size,common_align,&common_start)) ok=0;
    common_size=common.exists?(size_t)common.size:0U;
    bss_align=bss.exists?((size_t)1U<<bss.align_power):1U;
    if (ok && !macho_align(common_start+common_size,bss_align,&bss_start)) ok=0;
    bss_size=bss.exists?(size_t)bss.size:0U;
    if (ok && !macho_align(bss_start+bss_size,8U,&got_start)) ok=0;
    got_size=external_count*8U;
    if (ok && !macho_align(got_start+got_size,8U,&note_start)) ok=0;
    if (ok && !macho_align(note_start+8U,0x4000U,&data_end)) ok=0;

    external_name_offsets=ok?(uint32_t*)calloc(external_count==0U?1U:external_count,sizeof(*external_name_offsets)):NULL;
    if (ok && external_name_offsets==NULL) ok=0;
    if (ok && !__Bootstrap_Byte_Buffer_Append_U8__(&external_strings,0U)) ok=0;
    if (ok && external_count>0U)
        ok=__Bootstrap_Byte_Buffer_Append_U8__(&bind,0x11U)&&__Bootstrap_Byte_Buffer_Append_U8__(&bind,0x51U);
    for (i=0U; ok && i<external_count; ++i)
    {
        __Bootstrap_MachO_Symbol__ *symbol=&symbols[externals[i]];
        uint64_t got_inside_data=(uint64_t)(got_start-data_segment_start+i*8U);
        if (external_strings.size>UINT32_MAX) { ok=0; break; }
        external_name_offsets[i]=(uint32_t)external_strings.size;
        ok=__Bootstrap_Byte_Buffer_Append_U8__(&bind,0x40U)&&
           __Bootstrap_Byte_Buffer_Append__(&bind,symbol->name,symbol->name_size)&&
           __Bootstrap_Byte_Buffer_Append_U8__(&bind,0U)&&
           __Bootstrap_Byte_Buffer_Append_U8__(&bind,0x72U)&&macho_append_uleb(&bind,got_inside_data)&&
           __Bootstrap_Byte_Buffer_Append_U8__(&bind,0x90U)&&
           __Bootstrap_Byte_Buffer_Append__(&external_strings,symbol->name,symbol->name_size)&&
           __Bootstrap_Byte_Buffer_Append_U8__(&external_strings,0U);
    }
    if (ok && external_count>0U) ok=__Bootstrap_Byte_Buffer_Append_U8__(&bind,0U);
    link_start=data_end; bind_start=link_start;
    if (ok && !macho_align(bind_start+bind.size,8U,&external_symbols_start)) ok=0;
    if (ok) external_strings_start=external_symbols_start+external_count*16U;
    if (ok && !macho_align(external_strings_start+external_strings.size,16U,&signature_start)) ok=0;

    __Bootstrap_Byte_Buffer_Init__(output);
    if (ok)
    {
        uint32_t flags=0x200004U | (external_count==0U?1U:0U);
        uint32_t commands=external_count>0U?13U:12U;
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0xFEEDFACFU)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x0100000CU)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,2U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,commands)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)load_commands_size)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,flags)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U);
    }
    if (ok) ok=macho_append_segment(output,"__PAGEZERO",0U,base,0U,0U,0U,0U,0U);
    if (ok) ok=macho_append_segment(output,"__TEXT",base,text_end,0U,text_end,5U,5U,3U)&&
        macho_append_section(output,"__text","__TEXT",base+text_start,text.size,(uint32_t)text_start,2U,0x80000400U)&&
        macho_append_section(output,"__const","__TEXT",base+const_start,
                             (strings_start+string_size_out)-const_start,(uint32_t)const_start,
                             constants.exists?constants.align_power:3U,0U)&&
        macho_append_section(output,"__stubcode","__TEXT",base+stub_start,stub_size,(uint32_t)stub_start,2U,0x80000400U);
    if (ok)
    {
        uint32_t data_sections=data.exists?3U:2U;
        ok=macho_append_segment(output,"__DATA",base+data_segment_start,data_end-data_segment_start,
                                data_segment_start,data_end-data_segment_start,3U,3U,data_sections);
        if (ok && data.exists)
            ok=macho_append_section(output,"__data","__DATA",base+data_source_start,data_size,
                                    (uint32_t)data_source_start,data.align_power,0U);
        if (ok) ok=macho_append_section(output,"__got","__DATA",base+got_start,got_size,(uint32_t)got_start,3U,0U)&&
                  macho_append_section(output,"__note.sultan","__DATA",base+note_start,8U,(uint32_t)note_start,3U,0U);
    }
    link_command_offset=output->size;
    if (ok) ok=macho_append_segment(output,"__LINKEDIT",base+link_start,0U,link_start,0U,1U,1U,0U);
    if (ok && external_count>0U)
    {
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x80000022U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,48U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)bind_start)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)bind.size)&&
           __Bootstrap_Byte_Buffer_Resize_Zero__(output,output->size+24U);
    }
    if (ok)
    {
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,2U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,24U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)external_symbols_start)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)external_count)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)external_strings_start)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)external_strings.size)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0xBU)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,80U)&&
           __Bootstrap_Byte_Buffer_Resize_Zero__(output,output->size+20U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)external_count)&&
           __Bootstrap_Byte_Buffer_Resize_Zero__(output,output->size+48U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x32U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,24U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,1U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x000B0000U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x1BU)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,24U);
        uuid_offset=output->size;
        ok=ok&&__Bootstrap_Byte_Buffer_Resize_Zero__(output,output->size+16U);
    }
    if (ok)
    {
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0xEU)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,32U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,12U)&&macho_append_path(output,"/usr/lib/dyld")&&
           __Bootstrap_Byte_Buffer_Pad_To__(output,(output->size+7U)/8U*8U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0xCU)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,56U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,24U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U)&&
           macho_append_path(output,"/usr/lib/libSystem.B.dylib")&&
           __Bootstrap_Byte_Buffer_Pad_To__(output,(output->size+7U)/8U*8U);
    }
    if (ok)
    {
        size_t entry_offset=(size_t)(symbols[entry_symbol].value-text.address);
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x80000028U)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,24U)&&
           __Bootstrap_Byte_Buffer_Append_U64_LE__(output,text_start+entry_offset)&&__Bootstrap_Byte_Buffer_Append_U64_LE__(output,0U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0x1DU)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,16U)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,(uint32_t)signature_start);
        signature_size_offset=output->size;
        ok=ok&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,0U);
    }
    if (ok && output->size!=32U+load_commands_size) ok=0;

    if (ok) ok=__Bootstrap_Byte_Buffer_Pad_To__(output,text_start)&&
              __Bootstrap_Byte_Buffer_Append__(output,object+text.file_offset,(size_t)text.size)&&
              __Bootstrap_Byte_Buffer_Pad_To__(output,const_start);
    if (ok && constants.exists) ok=__Bootstrap_Byte_Buffer_Append__(output,object+constants.file_offset,(size_t)constants.size);
    if (ok) ok=__Bootstrap_Byte_Buffer_Pad_To__(output,strings_start);
    if (ok && strings.exists) ok=__Bootstrap_Byte_Buffer_Append__(output,object+strings.file_offset,(size_t)strings.size);
    if (ok) ok=__Bootstrap_Byte_Buffer_Pad_To__(output,stub_start);
    for (i=0U; ok && i<external_count; ++i)
    {
        uint64_t stub_address=base+stub_start+i*12U;
        uint64_t got_address=base+got_start+i*8U;
        uint32_t page_code;
        uint32_t adrp,ldr;
        if (!macho_page_delta(stub_address,got_address,&page_code)) { ok=0; break; }
        adrp=0x90000010U|((page_code&3U)<<29U)|(((page_code>>2U)&0x7FFFFU)<<5U);
        ldr=0xF9400210U|((uint32_t)(((got_address%4096U)/8U)&0xFFFU)<<10U);
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,adrp)&&__Bootstrap_Byte_Buffer_Append_U32_LE__(output,ldr)&&
           __Bootstrap_Byte_Buffer_Append_U32_LE__(output,0xD61F0200U);
    }

    for (i=0U; ok && i<text.relocation_count; ++i)
    {
        size_t p=(size_t)text.relocation_offset+i*8U;
        uint32_t relocation=macho_u32(object,p), info=macho_u32(object,p+4U);
        uint32_t symbol_index=info&0xFFFFFFU, pcrel=(info>>24U)&1U, length=(info>>25U)&3U;
        uint32_t external=(info>>27U)&1U, type=(info>>28U)&0xFU;
        size_t field_size=type==0U?8U:4U;
        uint64_t target;
        __Bootstrap_MachO_Symbol__ *symbol;
        size_t patch;
        if (symbol_index>=symbol_count || external!=1U || (type!=0U&&type!=2U&&type!=3U&&type!=4U) ||
            (type==0U&&(pcrel!=0U||length!=3U)) || ((type==2U||type==3U)&&(pcrel!=1U||length!=2U)) ||
            (type==4U&&(pcrel!=0U||length!=2U)) || relocation>text.size || field_size>text.size-relocation ||
            relocation%field_size!=0U) { ok=0; break; }
        symbol=&symbols[symbol_index];
        target=macho_symbol_target(symbol,&text,text_start,&constants,const_start,&strings,strings_start,
                                   &data,data_source_start,&common,common_start,&bss,bss_start,base);
        if (target==0U && symbol->section==0U)
        {
            size_t ordinal;
            if (!macho_external_ordinal(externals,external_count,symbol_index,&ordinal)) { ok=0; break; }
            target=base+stub_start+ordinal*12U;
        }
        if (target==0U) { ok=0; break; }
        patch=text_start+(size_t)relocation;
        if (type==2U) ok=macho_patch_branch26(output,patch,base+patch,target);
        else if (type==3U) ok=symbol->section!=0U&&macho_patch_page21(output,patch,base+patch,target);
        else if (type==4U) ok=symbol->section!=0U&&macho_patch_pageoff12(output,patch,target);
        else ok=symbol->section!=0U&&__Bootstrap_Byte_Buffer_Write_U64_LE__(output,patch,target);
    }

    if (ok) ok=__Bootstrap_Byte_Buffer_Pad_To__(output,data_source_start);
    if (ok && data.exists) ok=__Bootstrap_Byte_Buffer_Append__(output,object+data.file_offset,(size_t)data.size);
    if (ok) ok=__Bootstrap_Byte_Buffer_Pad_To__(output,common_start)&&
              __Bootstrap_Byte_Buffer_Resize_Zero__(output,common_start+common_size)&&
              __Bootstrap_Byte_Buffer_Pad_To__(output,bss_start)&&
              __Bootstrap_Byte_Buffer_Resize_Zero__(output,bss_start+bss_size)&&
              __Bootstrap_Byte_Buffer_Pad_To__(output,got_start)&&
              __Bootstrap_Byte_Buffer_Resize_Zero__(output,got_start+got_size)&&
              __Bootstrap_Byte_Buffer_Pad_To__(output,note_start);
    if (ok)
    {
        static const uint8_t note[8]={'S','U','L','T','A','N','C',0};
        ok=__Bootstrap_Byte_Buffer_Append__(output,note,sizeof(note))&&
           __Bootstrap_Byte_Buffer_Pad_To__(output,bind_start)&&
           __Bootstrap_Byte_Buffer_Append__(output,bind.data,bind.size)&&
           __Bootstrap_Byte_Buffer_Pad_To__(output,external_symbols_start);
    }
    for (i=0U; ok && i<external_count; ++i)
    {
        ok=__Bootstrap_Byte_Buffer_Append_U32_LE__(output,external_name_offsets[i])&&
           __Bootstrap_Byte_Buffer_Append_U8__(output,1U)&&__Bootstrap_Byte_Buffer_Append_U8__(output,0U)&&
           __Bootstrap_Byte_Buffer_Append_U16_LE__(output,0x0100U)&&__Bootstrap_Byte_Buffer_Append_U64_LE__(output,0U);
    }
    if (ok) ok=__Bootstrap_Byte_Buffer_Pad_To__(output,external_strings_start)&&
              __Bootstrap_Byte_Buffer_Append__(output,external_strings.data,external_strings.size)&&
              __Bootstrap_Byte_Buffer_Pad_To__(output,signature_start);
    if (ok)
    {
        uint8_t hash[32];
        macho_sha256(object,object_size,hash);
        if (uuid_offset+16U>output->size) ok=0;
        else memcpy(output->data+uuid_offset,hash,16U);
    }
    if (ok) ok=macho_build_signature(output,&preliminary_signature);
    if (ok) ok=__Bootstrap_Byte_Buffer_Write_U32_LE__(output,signature_size_offset,(uint32_t)preliminary_signature.size);
    if (ok)
    {
        size_t link_end=signature_start+preliminary_signature.size;
        size_t virtual_size;
        if (!macho_align(link_end-link_start,0x4000U,&virtual_size)) ok=0;
        else ok=__Bootstrap_Byte_Buffer_Write_U64_LE__(output,link_command_offset+32U,virtual_size)&&
                __Bootstrap_Byte_Buffer_Write_U64_LE__(output,link_command_offset+48U,link_end-link_start);
    }
    if (ok) ok=macho_build_signature(output,&signature);
    if (ok && signature.size!=preliminary_signature.size) ok=0;
    if (ok) ok=__Bootstrap_Byte_Buffer_Append__(output,signature.data,signature.size);
    if (ok && output->size!=signature_start+signature.size) ok=0;

    free(external_name_offsets); free(externals); free(symbols);
    __Bootstrap_Byte_Buffer_Destroy__(&bind); __Bootstrap_Byte_Buffer_Destroy__(&external_strings);
    __Bootstrap_Byte_Buffer_Destroy__(&preliminary_signature); __Bootstrap_Byte_Buffer_Destroy__(&signature);
    if (!ok)
    {
        __Bootstrap_Byte_Buffer_Destroy__(output);
        return macho_fail(error,error_size,"unable to finalize LLVM Mach-O object");
    }
    return 1;
}
