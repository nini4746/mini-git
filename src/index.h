#ifndef MINIGIT_INDEX_H
#define MINIGIT_INDEX_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t ctime_s, ctime_n;
    uint32_t mtime_s, mtime_n;
    uint32_t dev, ino, mode, uid, gid, size;
    unsigned char sha[20];
    char *path;
} index_entry;

typedef struct {
    index_entry *entries;
    size_t count;
    size_t cap;
} index_t;

/* Read .git/index (v2). If missing, returns an empty index. Returns 0 ok. */
int index_read(index_t *idx);

/* Write .git/index (v2) with trailing SHA-1 checksum. Returns 0 ok. */
int index_write(index_t *idx);

/* Insert or replace an entry by path, keeping entries sorted by path bytes. */
void index_upsert(index_t *idx, const index_entry *e);

void index_free(index_t *idx);

#endif
