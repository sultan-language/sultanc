/* Finalizes Stage0 LLVM objects into Stage1 executables without an external linker. */

#define _POSIX_C_SOURCE 200809L

#include "link/stage1_finalizer.h"
#include "link/byte_buffer.h"
#include "link/elf_executable.h"
#include "link/macho_executable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char stage1_finalizer_error[256];

static int finalizer_fail(const char *message)
{
    if (message == NULL)
        message = "unknown Stage1 finalization failure";
    snprintf(stage1_finalizer_error, sizeof(stage1_finalizer_error), "%s", message);
    return 0;
}

static int read_object(const char *path, uint8_t **data, size_t *size)
{
    FILE *file;
    long end;
    uint8_t *bytes;

    if (path == NULL || data == NULL || size == NULL)
        return finalizer_fail("invalid Stage1 object path");
    file = fopen(path, "rb");
    if (file == NULL)
        return finalizer_fail("failed to open Stage1 object");
    if (fseek(file, 0L, SEEK_END) != 0)
    {
        fclose(file);
        return finalizer_fail("failed to seek Stage1 object");
    }
    end = ftell(file);
    if (end < 0L || fseek(file, 0L, SEEK_SET) != 0)
    {
        fclose(file);
        return finalizer_fail("failed to size Stage1 object");
    }
    bytes = end == 0L ? NULL : (uint8_t *)malloc((size_t)end);
    if (end != 0L && bytes == NULL)
    {
        fclose(file);
        return finalizer_fail("out of memory reading Stage1 object");
    }
    if (end != 0L && fread(bytes, 1U, (size_t)end, file) != (size_t)end)
    {
        free(bytes);
        fclose(file);
        return finalizer_fail("failed to read Stage1 object");
    }
    if (fclose(file) != 0)
    {
        free(bytes);
        return finalizer_fail("failed to close Stage1 object");
    }
    *data = bytes;
    *size = (size_t)end;
    return 1;
}

static int write_executable(const char *path, const __Bootstrap_Byte_Buffer__ *image)
{
    FILE *file;

    if (path == NULL || image == NULL)
        return finalizer_fail("invalid Stage1 executable output");
    file = fopen(path, "wb");
    if (file == NULL)
        return finalizer_fail("failed to create Stage1 executable");
    if (image->size != 0U && fwrite(image->data, 1U, image->size, file) != image->size)
    {
        fclose(file);
        return finalizer_fail("failed to write Stage1 executable");
    }
    if (fchmod(fileno(file), 0755) != 0)
    {
        fclose(file);
        return finalizer_fail("failed to mark Stage1 executable as executable");
    }
    if (fclose(file) != 0)
        return finalizer_fail("failed to close Stage1 executable");
    return 1;
}

int __Bootstrap_Finalize_Stage1__(const char *object_path,
                                  const char *output_path,
                                  const char *target_name)
{
    uint8_t *object = NULL;
    size_t object_size = 0U;
    __Bootstrap_Byte_Buffer__ image;
    char format_error[256];
    int finalized = 0;
    int written = 0;

    stage1_finalizer_error[0] = '\0';
    format_error[0] = '\0';
    __Bootstrap_Byte_Buffer_Init__(&image);
    if (!read_object(object_path, &object, &object_size))
        return 0;

    if (target_name != NULL && strcmp(target_name, "arm64-darwin") == 0)
        finalized = __Bootstrap_Finalize_MachO_ARM64__(object, object_size, &image,
                                                       format_error, sizeof(format_error));
    else if (target_name != NULL && strcmp(target_name, "x86_64-linux") == 0)
        finalized = __Bootstrap_Finalize_ELF_X86_64__(object, object_size, &image,
                                                      format_error, sizeof(format_error));
    else
        finalizer_fail("unsupported Stage1 target");

    if (!finalized && stage1_finalizer_error[0] == '\0')
        finalizer_fail(format_error[0] == '\0' ? "Stage1 object finalization failed" : format_error);
    if (finalized)
        written = write_executable(output_path, &image);

    __Bootstrap_Byte_Buffer_Destroy__(&image);
    free(object);
    return finalized && written;
}

const char *__Bootstrap_Stage1_Finalizer_Error__(void)
{
    return stage1_finalizer_error;
}
