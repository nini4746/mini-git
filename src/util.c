#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

unsigned char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    unsigned char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return NULL; }
    buf[sz] = '\0';
    if (len)
        *len = (size_t)sz;
    return buf;
}

int write_file(const char *path, const void *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    size_t wrote = fwrite(data, 1, len, f);
    if (fclose(f) != 0)
        return -1;
    return wrote == len ? 0 : -1;
}

void hex_encode(const unsigned char *in, size_t n, char *out) {
    static const char *hx = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[i * 2]     = hx[in[i] >> 4];
        out[i * 2 + 1] = hx[in[i] & 0xf];
    }
    out[n * 2] = '\0';
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int hex_decode(const char *hex, unsigned char *out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int hi = hexval(hex[i * 2]);
        int lo = hexval(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0)
            return -1;
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return 0;
}

int mkdir_p(const char *path) {
    char tmp[4096];
    size_t len = strlen(path);
    if (len >= sizeof(tmp))
        return -1;
    memcpy(tmp, path, len + 1);
    /* strip trailing slash */
    if (len > 1 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return -1;
    return 0;
}

int mkdir_parents(const char *filepath) {
    char tmp[4096];
    size_t len = strlen(filepath);
    if (len >= sizeof(tmp))
        return -1;
    memcpy(tmp, filepath, len + 1);
    char *slash = strrchr(tmp, '/');
    if (!slash)
        return 0; /* no parent dir */
    *slash = '\0';
    if (tmp[0] == '\0')
        return 0;
    return mkdir_p(tmp);
}
