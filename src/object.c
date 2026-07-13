#include "object.h"
#include "sha1.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

/* Build the ".git/objects/xx/yy..38" path for a hex id. */
static void object_path(const char *hex, char *out, size_t outsz) {
    snprintf(out, outsz, ".git/objects/%c%c/%s", hex[0], hex[1], hex + 2);
}

int object_exists(const char *hex) {
    char path[256];
    object_path(hex, path, sizeof(path));
    struct stat st;
    return stat(path, &st) == 0;
}

int object_write(const char *type, const void *content, size_t len,
                 int do_write, char out_hex[41]) {
    /* Assemble the object: "<type> <size>\0<content>". */
    char header[64];
    int hlen = snprintf(header, sizeof(header), "%s %zu", type, len);
    size_t store_len = (size_t)hlen + 1 + len;
    unsigned char *store = malloc(store_len);
    if (!store)
        return -1;
    memcpy(store, header, hlen);
    store[hlen] = '\0';
    memcpy(store + hlen + 1, content, len);

    unsigned char digest[20];
    sha1(store, store_len, digest);
    hex_encode(digest, 20, out_hex);

    if (!do_write) {
        free(store);
        return 0;
    }

    char path[256];
    object_path(out_hex, path, sizeof(path));
    struct stat st;
    if (stat(path, &st) == 0) {
        /* Already stored; keep it (idempotent). */
        free(store);
        return 0;
    }

    /* zlib-compress the store buffer. */
    uLongf clen = compressBound(store_len);
    unsigned char *comp = malloc(clen);
    if (!comp) { free(store); return -1; }
    if (compress2(comp, &clen, store, store_len, Z_DEFAULT_COMPRESSION) != Z_OK) {
        free(store);
        free(comp);
        return -1;
    }
    free(store);

    if (mkdir_parents(path) != 0) { free(comp); return -1; }
    int rc = write_file(path, comp, clen);
    free(comp);
    return rc;
}

unsigned char *object_read(const char *hex, char *type_out, size_t *size_out) {
    char path[256];
    object_path(hex, path, sizeof(path));
    size_t clen;
    unsigned char *comp = read_file(path, &clen);
    if (!comp)
        return NULL;

    /* Inflate, growing the output buffer as needed. */
    size_t cap = clen * 4 + 64;
    unsigned char *out = malloc(cap);
    if (!out) { free(comp); return NULL; }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK) { free(comp); free(out); return NULL; }
    zs.next_in = comp;
    zs.avail_in = (uInt)clen;
    zs.next_out = out;
    zs.avail_out = (uInt)cap;
    int ret;
    for (;;) {
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret == Z_STREAM_END)
            break;
        if (ret != Z_OK) {
            inflateEnd(&zs);
            free(comp);
            free(out);
            return NULL;
        }
        if (zs.avail_out == 0) {
            size_t used = cap;
            cap *= 2;
            unsigned char *n = realloc(out, cap);
            if (!n) { inflateEnd(&zs); free(comp); free(out); return NULL; }
            out = n;
            zs.next_out = out + used;
            zs.avail_out = (uInt)(cap - used);
        }
    }
    size_t total = zs.total_out;
    inflateEnd(&zs);
    free(comp);

    /* Parse "<type> <size>\0". */
    unsigned char *nul = memchr(out, '\0', total);
    if (!nul) { free(out); return NULL; }
    size_t hdrlen = (size_t)(nul - out);
    char hdr[64];
    if (hdrlen >= sizeof(hdr)) { free(out); return NULL; }
    memcpy(hdr, out, hdrlen);
    hdr[hdrlen] = '\0';
    char type[16];
    unsigned long long size;
    if (sscanf(hdr, "%15s %llu", type, &size) != 2) { free(out); return NULL; }

    size_t content_len = total - hdrlen - 1;
    unsigned char *content = malloc(content_len + 1);
    if (!content) { free(out); return NULL; }
    memcpy(content, nul + 1, content_len);
    content[content_len] = '\0';
    free(out);

    if (type_out)
        strcpy(type_out, type);
    if (size_out)
        *size_out = content_len;
    return content;
}
