#ifndef MINIGIT_COMMIT_H
#define MINIGIT_COMMIT_H

#include <stddef.h>

/* Parsed view of a commit object (points into the caller-provided content). */
typedef struct {
    char tree[41];
    char parent[41];   /* first parent; empty string if none */
    int has_parent;
    char author_name[256];
    char author_email[256];
    char author_date[64];   /* "<epoch> <tz>" */
    const char *message;    /* points inside content */
} commit_info;

/* Parse commit object content. Returns 0 on success. */
int commit_parse(const unsigned char *content, size_t len, commit_info *out);

/* Build a commit object from a tree hex, optional parent hex (NULL/empty for
 * none) and a message, using the GIT_AUTHOR / GIT_COMMITTER env vars (with
 * a hardcoded fallback).
 * Writes it to the store and returns its hex in out_hex. Returns 0 ok. */
int commit_create(const char *tree_hex, const char *parent_hex,
                  const char *message, char out_hex[41]);

#endif
