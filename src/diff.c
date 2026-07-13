#include "diff.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Split a buffer into lines (newline stripped). A trailing '\n' does not
 * create a final empty line. Returns a malloc'd array of malloc'd strings. */
static char **split_lines(const unsigned char *buf, size_t len, int *count) {
    char **lines = NULL;
    int n = 0, cap = 0;
    size_t i = 0;
    while (i < len) {
        size_t j = i;
        while (j < len && buf[j] != '\n')
            j++;
        if (n == cap) {
            cap = cap ? cap * 2 : 16;
            lines = realloc(lines, cap * sizeof(char *));
        }
        size_t l = j - i;
        char *s = malloc(l + 1);
        memcpy(s, buf + i, l);
        s[l] = '\0';
        lines[n++] = s;
        i = (j < len) ? j + 1 : j;
    }
    *count = n;
    return lines;
}

static void free_lines(char **lines, int n) {
    for (int i = 0; i < n; i++)
        free(lines[i]);
    free(lines);
}

/* An edit to emit: op '-' uses a[idx], op '+' uses b[idx]. */
typedef struct { char op; int idx; } edit;

int diff_files(const char *path_a, const char *path_b) {
    size_t la, lb;
    unsigned char *ba = read_file(path_a, &la);
    if (!ba) { fprintf(stderr, "diff: cannot read %s\n", path_a); return 1; }
    unsigned char *bb = read_file(path_b, &lb);
    if (!bb) { fprintf(stderr, "diff: cannot read %s\n", path_b); free(ba); return 1; }

    int n, m;
    char **a = split_lines(ba, la, &n);
    char **b = split_lines(bb, lb, &m);
    free(ba);
    free(bb);

    int max = n + m;
    int off = max;               /* index shift so k in [-max,max] maps to [0,2max] */
    int vsize = 2 * max + 1;
    int *v = malloc(sizeof(int) * vsize);
    /* trace[d] = snapshot of v after processing depth d */
    int **trace = malloc(sizeof(int *) * (max + 1));
    int found_d = -1;

    for (int i = 0; i < vsize; i++)
        v[i] = 0;

    for (int d = 0; d <= max; d++) {
        trace[d] = malloc(sizeof(int) * vsize);
        for (int k = -d; k <= d; k += 2) {
            int x;
            if (k == -d || (k != d && v[off + k - 1] < v[off + k + 1]))
                x = v[off + k + 1];       /* move down (insertion) */
            else
                x = v[off + k - 1] + 1;    /* move right (deletion) */
            int y = x - k;
            while (x < n && y < m && strcmp(a[x], b[y]) == 0) {
                x++;
                y++;
            }
            v[off + k] = x;
            if (x >= n && y >= m) {
                found_d = d;
                break;
            }
        }
        memcpy(trace[d], v, sizeof(int) * vsize);
        if (found_d >= 0)
            break;
    }

    /* Backtrack to recover the edit script (in reverse). */
    edit *edits = malloc(sizeof(edit) * (found_d > 0 ? found_d : 1));
    int ne = 0;
    int x = n, y = m;
    for (int d = found_d; d > 0; d--) {
        int *vd = trace[d];
        int k = x - y;
        int prev_k;
        if (k == -d || (k != d && vd[off + k - 1] < vd[off + k + 1]))
            prev_k = k + 1;
        else
            prev_k = k - 1;
        int prev_x = vd[off + prev_k];
        int prev_y = prev_x - prev_k;
        /* follow the diagonal (common lines) - not emitted */
        while (x > prev_x && y > prev_y) {
            x--;
            y--;
        }
        if (x == prev_x) {
            /* down move: insertion of b[prev_y] */
            edits[ne].op = '+';
            edits[ne].idx = prev_y;
            ne++;
        } else {
            /* right move: deletion of a[prev_x] */
            edits[ne].op = '-';
            edits[ne].idx = prev_x;
            ne++;
        }
        x = prev_x;
        y = prev_y;
    }

    /* Emit in forward order. */
    for (int i = ne - 1; i >= 0; i--) {
        if (edits[i].op == '-')
            printf("-%s\n", a[edits[i].idx]);
        else
            printf("+%s\n", b[edits[i].idx]);
    }

    free(edits);
    for (int d = 0; d <= found_d; d++)
        free(trace[d]);
    free(trace);
    free(v);
    free_lines(a, n);
    free_lines(b, m);
    return 0;
}
