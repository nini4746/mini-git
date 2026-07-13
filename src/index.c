#include "index.h"
#include "sha1.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_PATH ".git/index"

static uint32_t rd32(const unsigned char *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static void wr32(unsigned char *p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

void index_free(index_t *idx) {
    for (size_t i = 0; i < idx->count; i++)
        free(idx->entries[i].path);
    free(idx->entries);
    idx->entries = NULL;
    idx->count = idx->cap = 0;
}

static void ensure_cap(index_t *idx, size_t need) {
    if (need <= idx->cap)
        return;
    size_t nc = idx->cap ? idx->cap * 2 : 8;
    while (nc < need)
        nc *= 2;
    idx->entries = realloc(idx->entries, nc * sizeof(index_entry));
    idx->cap = nc;
}

int index_read(index_t *idx) {
    memset(idx, 0, sizeof(*idx));
    size_t len;
    unsigned char *buf = read_file(INDEX_PATH, &len);
    if (!buf)
        return 0; /* no index yet = empty */
    if (len < 12 || memcmp(buf, "DIRC", 4) != 0) { free(buf); return -1; }
    uint32_t ver = rd32(buf + 4);
    uint32_t n = rd32(buf + 8);
    if (ver != 2) { free(buf); return -1; }

    size_t off = 12;
    for (uint32_t i = 0; i < n; i++) {
        size_t start = off;
        if (off + 62 > len) { free(buf); index_free(idx); return -1; }
        index_entry e;
        memset(&e, 0, sizeof(e));
        e.ctime_s = rd32(buf + off);      e.ctime_n = rd32(buf + off + 4);
        e.mtime_s = rd32(buf + off + 8);  e.mtime_n = rd32(buf + off + 12);
        e.dev     = rd32(buf + off + 16); e.ino     = rd32(buf + off + 20);
        e.mode    = rd32(buf + off + 24); e.uid     = rd32(buf + off + 28);
        e.gid     = rd32(buf + off + 32); e.size    = rd32(buf + off + 36);
        memcpy(e.sha, buf + off + 40, 20);
        /* flags at off+60 (2 bytes); low 12 bits = name length */
        off += 62;
        /* path is NUL-terminated */
        size_t plen = 0;
        while (off + plen < len && buf[off + plen] != '\0')
            plen++;
        e.path = malloc(plen + 1);
        memcpy(e.path, buf + off, plen);
        e.path[plen] = '\0';
        off += plen;
        /* advance to 8-byte alignment relative to entry start */
        size_t entry_len = 62 + plen;
        size_t padded = (entry_len + 8) & ~((size_t)7);
        off = start + padded;
        ensure_cap(idx, idx->count + 1);
        idx->entries[idx->count++] = e;
    }
    free(buf);
    return 0;
}

int index_write(index_t *idx) {
    /* Build the full byte image, then append SHA-1 checksum. */
    size_t cap = 12 + 64;
    for (size_t i = 0; i < idx->count; i++)
        cap += 62 + strlen(idx->entries[i].path) + 8;
    cap += 20;
    unsigned char *buf = malloc(cap);
    if (!buf)
        return -1;
    memcpy(buf, "DIRC", 4);
    wr32(buf + 4, 2);
    wr32(buf + 8, (uint32_t)idx->count);
    size_t off = 12;
    for (size_t i = 0; i < idx->count; i++) {
        index_entry *e = &idx->entries[i];
        size_t start = off;
        wr32(buf + off, e->ctime_s);      wr32(buf + off + 4, e->ctime_n);
        wr32(buf + off + 8, e->mtime_s);  wr32(buf + off + 12, e->mtime_n);
        wr32(buf + off + 16, e->dev);     wr32(buf + off + 20, e->ino);
        wr32(buf + off + 24, e->mode);    wr32(buf + off + 28, e->uid);
        wr32(buf + off + 32, e->gid);     wr32(buf + off + 36, e->size);
        memcpy(buf + off + 40, e->sha, 20);
        size_t plen = strlen(e->path);
        uint16_t flags = plen < 0xFFF ? (uint16_t)plen : 0xFFF;
        buf[off + 60] = (flags >> 8) & 0xff;
        buf[off + 61] = flags & 0xff;
        off += 62;
        memcpy(buf + off, e->path, plen);
        off += plen;
        size_t entry_len = 62 + plen;
        size_t padded = (entry_len + 8) & ~((size_t)7);
        size_t padlen = padded - entry_len;
        memset(buf + off, 0, padlen);
        off += padlen;
        (void)start;
    }
    unsigned char digest[20];
    sha1(buf, off, digest);
    memcpy(buf + off, digest, 20);
    off += 20;
    int rc = write_file(INDEX_PATH, buf, off);
    free(buf);
    return rc;
}

void index_upsert(index_t *idx, const index_entry *e) {
    /* Find by path; replace if present. */
    for (size_t i = 0; i < idx->count; i++) {
        if (strcmp(idx->entries[i].path, e->path) == 0) {
            free(idx->entries[i].path);
            idx->entries[i] = *e;
            idx->entries[i].path = strdup(e->path);
            return;
        }
    }
    /* Insert keeping sorted order by path bytes. */
    ensure_cap(idx, idx->count + 1);
    size_t pos = idx->count;
    for (size_t i = 0; i < idx->count; i++) {
        if (strcmp(e->path, idx->entries[i].path) < 0) {
            pos = i;
            break;
        }
    }
    for (size_t i = idx->count; i > pos; i--)
        idx->entries[i] = idx->entries[i - 1];
    idx->entries[pos] = *e;
    idx->entries[pos].path = strdup(e->path);
    idx->count++;
}
