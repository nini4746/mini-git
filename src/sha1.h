#ifndef MINIGIT_SHA1_H
#define MINIGIT_SHA1_H

#include <stddef.h>
#include <stdint.h>

/* SHA-1 per RFC 3174, implemented by hand. */

typedef struct {
    uint32_t h[5];
    uint64_t len;      /* total message length in bytes */
    unsigned char buf[64];
    size_t buflen;     /* bytes currently in buf */
} sha1_ctx;

void sha1_init(sha1_ctx *c);
void sha1_update(sha1_ctx *c, const void *data, size_t len);
void sha1_final(sha1_ctx *c, unsigned char out[20]);

/* Convenience one-shot. */
void sha1(const void *data, size_t len, unsigned char out[20]);

#endif
