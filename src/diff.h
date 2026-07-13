#ifndef MINIGIT_DIFF_H
#define MINIGIT_DIFF_H

/* Print a line-level Myers diff of two files to stdout. Deleted lines are
 * prefixed with '-', added lines with '+', common lines with ' '.
 * Returns 0 on success, nonzero on error. */
int diff_files(const char *path_a, const char *path_b);

#endif
