#ifndef MINIGIT_UTIL_H
#define MINIGIT_UTIL_H

#include <stddef.h>
#include <sys/types.h>

/* Read whole file into a malloc'd buffer. Sets *len. Returns NULL on error. */
unsigned char *read_file(const char *path, size_t *len);

/* Write buffer to file (truncate/create). Returns 0 on success. */
int write_file(const char *path, const void *data, size_t len);

/* Hex-encode n raw bytes into out (must hold 2*n+1 bytes, NUL-terminated). */
void hex_encode(const unsigned char *in, size_t n, char *out);

/* Decode 2*n hex chars into out (n raw bytes). Returns 0 on success. */
int hex_decode(const char *hex, unsigned char *out, size_t n);

/* mkdir -p for a directory path. Returns 0 on success. */
int mkdir_p(const char *path);

/* Create all parent directories for a file path. Returns 0 on success. */
int mkdir_parents(const char *filepath);

/* Fatal error: print to stderr and exit(1). */
void die(const char *fmt, ...);

#endif
