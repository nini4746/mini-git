#include "sha1.h"
#include <string.h>

/* Left-rotate a 32-bit word. */
static uint32_t rol(uint32_t v, int n) {
    return (v << n) | (v >> (32 - n));
}

static void sha1_block(sha1_ctx *c, const unsigned char *p) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)p[i * 4] << 24) |
               ((uint32_t)p[i * 4 + 1] << 16) |
               ((uint32_t)p[i * 4 + 2] << 8) |
               ((uint32_t)p[i * 4 + 3]);
    }
    for (int i = 16; i < 80; i++)
        w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    uint32_t a = c->h[0], b = c->h[1], d = c->h[2], e = c->h[3], f = c->h[4];
    /* note: using d,e,f as c,d,e to avoid shadowing ctx name */
    for (int i = 0; i < 80; i++) {
        uint32_t k, tmp;
        if (i < 20)       { tmp = (b & d) | ((~b) & e);        k = 0x5A827999; }
        else if (i < 40)  { tmp = b ^ d ^ e;                   k = 0x6ED9EBA1; }
        else if (i < 60)  { tmp = (b & d) | (b & e) | (d & e); k = 0x8F1BBCDC; }
        else              { tmp = b ^ d ^ e;                   k = 0xCA62C1D6; }
        tmp += rol(a, 5) + f + k + w[i];
        f = e;
        e = d;
        d = rol(b, 30);
        b = a;
        a = tmp;
    }
    c->h[0] += a;
    c->h[1] += b;
    c->h[2] += d;
    c->h[3] += e;
    c->h[4] += f;
}

void sha1_init(sha1_ctx *c) {
    c->h[0] = 0x67452301;
    c->h[1] = 0xEFCDAB89;
    c->h[2] = 0x98BADCFE;
    c->h[3] = 0x10325476;
    c->h[4] = 0xC3D2E1F0;
    c->len = 0;
    c->buflen = 0;
}

void sha1_update(sha1_ctx *c, const void *data, size_t len) {
    const unsigned char *p = data;
    c->len += len;
    while (len > 0) {
        size_t space = 64 - c->buflen;
        size_t take = len < space ? len : space;
        memcpy(c->buf + c->buflen, p, take);
        c->buflen += take;
        p += take;
        len -= take;
        if (c->buflen == 64) {
            sha1_block(c, c->buf);
            c->buflen = 0;
        }
    }
}

void sha1_final(sha1_ctx *c, unsigned char out[20]) {
    uint64_t bitlen = c->len * 8;
    unsigned char pad = 0x80;
    sha1_update(c, &pad, 1);
    unsigned char zero = 0x00;
    while (c->buflen != 56)
        sha1_update(c, &zero, 1);
    /* append length as big-endian 64-bit */
    unsigned char lenbuf[8];
    for (int i = 0; i < 8; i++)
        lenbuf[i] = (unsigned char)(bitlen >> (56 - i * 8));
    sha1_update(c, lenbuf, 8);
    /* c->buflen should now be 0 (a block was processed) */
    for (int i = 0; i < 5; i++) {
        out[i * 4]     = (unsigned char)(c->h[i] >> 24);
        out[i * 4 + 1] = (unsigned char)(c->h[i] >> 16);
        out[i * 4 + 2] = (unsigned char)(c->h[i] >> 8);
        out[i * 4 + 3] = (unsigned char)(c->h[i]);
    }
}

void sha1(const void *data, size_t len, unsigned char out[20]) {
    sha1_ctx c;
    sha1_init(&c);
    sha1_update(&c, data, len);
    sha1_final(&c, out);
}
