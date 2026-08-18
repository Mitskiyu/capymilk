#include <stdio.h>
#include <stdlib.h>

internal char *plat_file_read(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0L, SEEK_END);
    i32 size = (i32)ftell(f);
    rewind(f);

    char *buf = (char *)malloc(size + 1);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }

    i32 read = (i32)fread(buf, 1, size, f);
    fclose(f);
    if (read != size) {
        free(buf);
        return NULL; 
    }

    buf[size] = '\0';
    return buf;
}
