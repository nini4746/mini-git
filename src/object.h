#ifndef MINIGIT_OBJECT_H
#define MINIGIT_OBJECT_H

#include <stddef.h>

/* Compute the object id for content of the given type. Writes the 40-char
 * lowercase hex id (NUL-terminated) into out_hex. If do_write is nonzero, the
 * object is stored under .git/objects (idempotent: existing objects are kept).
 * Returns 0 on success. */
int object_write(const char *type, const void *content, size_t len,
                 int do_write, char out_hex[41]);

/* Read and inflate an object by its 40-char hex id. On success returns a
 * malloc'd buffer holding the raw content (NUL-terminated for convenience),
 * sets *size_out to the content length and copies the type string into
 * type_out (buffer of at least 16 bytes). Returns NULL on error. */
unsigned char *object_read(const char *hex, char *type_out, size_t *size_out);

/* Return 1 if an object with this hex id exists in the store, else 0. */
int object_exists(const char *hex);

#endif
