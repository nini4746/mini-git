#ifndef MINIGIT_REFS_H
#define MINIGIT_REFS_H

/* If HEAD is symbolic ("ref: <path>"), copy <path> into out (size >= 256) and
 * return 1. If HEAD holds a raw hash (detached), return 0. Returns -1 on
 * error (e.g. no HEAD). */
int head_symbolic_ref(char out[256]);

/* Resolve HEAD to a commit hex id. Returns 0 and fills hex[41] on success;
 * returns 1 if the branch is unborn (no commit yet); returns -1 on error. */
int head_resolve(char hex[41]);

/* Update the branch that HEAD points to (or HEAD itself if detached) to the
 * given commit hex. Returns 0 on success. */
int ref_update_head(const char *hex);

/* Write a raw hash into .git/HEAD (detached HEAD). Returns 0 on success. */
int head_set_detached(const char *hex);

#endif
