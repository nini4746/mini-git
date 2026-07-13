#include "tree.h"
#include "object.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One entry to appear in a tree object. */
typedef struct {
    char *name;
    int is_dir;
    unsigned int mode;       /* e.g. 0100644, 0100755, 040000 (octal value) */
    unsigned char sha[20];
} tent;

/* git tree sort: compare names, treating directory names as if they had a
 * trailing '/'. */
static int tent_cmp(const void *a, const void *b) {
    const tent *x = a, *y = b;
    size_t lx = strlen(x->name), ly = strlen(y->name);
    size_t m = lx < ly ? lx : ly;
    int c = memcmp(x->name, y->name, m);
    if (c != 0)
        return c;
    /* one is a prefix of the other: append virtual '/' for dirs */
    unsigned char cx = lx > m ? (unsigned char)x->name[m] : (x->is_dir ? '/' : 0);
    unsigned char cy = ly > m ? (unsigned char)y->name[m] : (y->is_dir ? '/' : 0);
    return (int)cx - (int)cy;
}

/* Build a tree for entries idx->entries[start..end) that all live under the
 * directory whose full-path prefix has length prefixlen (prefix includes a
 * trailing '/', or is empty for the root). */
static int build(index_t *idx, size_t start, size_t end, size_t prefixlen,
                 char out_hex[41]) {
    tent *tents = NULL;
    size_t tn = 0, tcap = 0;
    int rc = -1;

    size_t i = start;
    while (i < end) {
        const char *path = idx->entries[i].path;
        const char *rest = path + prefixlen;
        const char *slash = strchr(rest, '/');
        if (tn == tcap) {
            tcap = tcap ? tcap * 2 : 8;
            tents = realloc(tents, tcap * sizeof(tent));
        }
        if (!slash) {
            /* plain file directly in this directory */
            tent *t = &tents[tn++];
            t->name = strdup(rest);
            t->is_dir = 0;
            t->mode = idx->entries[i].mode;
            memcpy(t->sha, idx->entries[i].sha, 20);
            i++;
        } else {
            /* subdirectory: gather the contiguous run sharing this prefix */
            size_t complen = (size_t)(slash - rest);
            size_t new_prefixlen = prefixlen + complen + 1;
            size_t j = i;
            while (j < end &&
                   strncmp(idx->entries[j].path, path, new_prefixlen) == 0)
                j++;
            char subhex[41];
            if (build(idx, i, j, new_prefixlen, subhex) != 0)
                goto done;
            tent *t = &tents[tn++];
            t->name = strndup(rest, complen);
            t->is_dir = 1;
            t->mode = 040000;
            if (hex_decode(subhex, t->sha, 20) != 0)
                goto done;
            i = j;
        }
    }

    qsort(tents, tn, sizeof(tent), tent_cmp);

    /* Assemble tree content: "<mode-octal-no-leading-zero> <name>\0<20 raw>". */
    size_t cap = 64, len = 0;
    unsigned char *content = malloc(cap);
    for (size_t k = 0; k < tn; k++) {
        char modebuf[16];
        int ml = snprintf(modebuf, sizeof(modebuf), "%o", tents[k].mode);
        size_t nl = strlen(tents[k].name);
        size_t need = len + ml + 1 + nl + 1 + 20;
        if (need > cap) {
            while (cap < need)
                cap *= 2;
            content = realloc(content, cap);
        }
        memcpy(content + len, modebuf, ml); len += ml;
        content[len++] = ' ';
        memcpy(content + len, tents[k].name, nl); len += nl;
        content[len++] = '\0';
        memcpy(content + len, tents[k].sha, 20); len += 20;
    }

    rc = object_write("tree", content, len, 1, out_hex);
    free(content);

done:
    for (size_t k = 0; k < tn; k++)
        free(tents[k].name);
    free(tents);
    return rc;
}

int tree_write(index_t *idx, char out_hex[41]) {
    return build(idx, 0, idx->count, 0, out_hex);
}
