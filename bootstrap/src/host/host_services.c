#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *sultanc_bootstrap_host_path(const unsigned char *data, uint64_t length)
{
    char *path;
    if (length > (uint64_t)SIZE_MAX - 1U)
        return NULL;
    path = (char *)malloc((size_t)length + 1U);
    if (path == NULL)
        return NULL;
    if (length != 0U)
        memcpy(path, data, (size_t)length);
    path[length] = '\0';
    return path;
}

static int64_t sultanc_bootstrap_host_errno(void)
{
    int value = errno;
    return value != 0 ? -(int64_t)value : -1;
}

int64_t sultanc_bootstrap_host_read_file(const unsigned char *path_data,
                                         uint64_t path_length,
                                         unsigned char **out_data,
                                         uint64_t *out_length,
                                         uint64_t *out_capacity)
{
    char *path = NULL;
    FILE *file = NULL;
    unsigned char *data = NULL;
    long end;
    size_t length;
    size_t read_count;
    int64_t result = -1;

    if (out_data == NULL || out_length == NULL || out_capacity == NULL)
        return -1;
    *out_data = NULL;
    *out_length = 0U;
    *out_capacity = 0U;
    path = sultanc_bootstrap_host_path(path_data, path_length);
    if (path == NULL)
        return sultanc_bootstrap_host_errno();
    file = fopen(path, "rb");
    free(path);
    if (file == NULL)
        return sultanc_bootstrap_host_errno();
    if (fseek(file, 0L, SEEK_END) != 0)
        goto done;
    end = ftell(file);
    if (end < 0)
        goto done;
    if (fseek(file, 0L, SEEK_SET) != 0)
        goto done;
    length = (size_t)end;
    if ((long)length != end)
        goto done;
    if (length != 0U)
    {
        data = (unsigned char *)malloc(length);
        if (data == NULL)
            goto done;
        read_count = fread(data, 1U, length, file);
        if (read_count != length)
        {
            if (ferror(file))
                goto done;
            length = read_count;
        }
    }
    *out_data = data;
    *out_length = (uint64_t)length;
    *out_capacity = (uint64_t)length;
    data = NULL;
    result = (int64_t)length;

done:
    if (result < 0)
        result = sultanc_bootstrap_host_errno();
    free(data);
    if (file != NULL)
        fclose(file);
    return result;
}

int64_t sultanc_bootstrap_host_read_line(unsigned char **out_data,
                                         uint64_t *out_length,
                                         uint64_t *out_capacity)
{
    unsigned char *data = NULL;
    size_t length = 0U;
    size_t capacity = 0U;
    int ch;
    if (out_data == NULL || out_length == NULL || out_capacity == NULL)
        return -1;
    for (;;)
    {
        ch = fgetc(stdin);
        if (ch == EOF)
            break;
        if (length == capacity)
        {
            size_t next = capacity == 0U ? 128U : capacity * 2U;
            unsigned char *grown;
            if (next < capacity)
            {
                free(data);
                return -1;
            }
            grown = (unsigned char *)realloc(data, next);
            if (grown == NULL)
            {
                free(data);
                return sultanc_bootstrap_host_errno();
            }
            data = grown;
            capacity = next;
        }
        data[length++] = (unsigned char)ch;
        if (ch == '\n')
            break;
    }
    if (ferror(stdin))
    {
        free(data);
        return sultanc_bootstrap_host_errno();
    }
    *out_data = data;
    *out_length = (uint64_t)length;
    *out_capacity = (uint64_t)capacity;
    return (int64_t)length;
}

int64_t sultanc_bootstrap_host_write_file(const unsigned char *path_data,
                                          uint64_t path_length,
                                          const unsigned char *data,
                                          uint64_t length)
{
    char *path;
    FILE *file;
    size_t written;
    int close_result;
    if (length > (uint64_t)SIZE_MAX)
        return -1;
    path = sultanc_bootstrap_host_path(path_data, path_length);
    if (path == NULL)
        return sultanc_bootstrap_host_errno();
    file = fopen(path, "wb");
    free(path);
    if (file == NULL)
        return sultanc_bootstrap_host_errno();
    written = length == 0U ? 0U : fwrite(data, 1U, (size_t)length, file);
    close_result = fclose(file);
    if (written != (size_t)length || close_result != 0)
        return sultanc_bootstrap_host_errno();
    return (int64_t)written;
}

int64_t
sultanc_bootstrap_host_write_stream(int32_t descriptor, const unsigned char *data, uint64_t length)
{
    FILE *stream = descriptor == 2 ? stderr : stdout;
    size_t written;
    if (descriptor != 1 && descriptor != 2)
        return -1;
    if (length > (uint64_t)SIZE_MAX)
        return -1;
    written = length == 0U ? 0U : fwrite(data, 1U, (size_t)length, stream);
    if (written != (size_t)length || fflush(stream) != 0)
        return sultanc_bootstrap_host_errno();
    return (int64_t)written;
}

void sultanc_bootstrap_host_exit(int64_t status)
{
    exit((int)status);
}
