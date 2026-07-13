#ifndef MINIGIT_TREE_H
#define MINIGIT_TREE_H

#include "index.h"

/* Build tree objects from the index (recursively, deepest first) and write
 * them to the object store. Returns the root tree's 40-char hex id in
 * out_hex. Returns 0 on success. */
int tree_write(index_t *idx, char out_hex[41]);

#endif
