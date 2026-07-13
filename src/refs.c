#include "refs.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int head_symbolic_ref(char out[256]) {
    size_t len;
    unsigned char *buf = read_file(".git/HEAD", &len);
    if (!buf)
        return -1;
    /* trim trailing whitespace/newline */
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' ||
                       buf[len - 1] == ' '))
        buf[--len] = '\0';
    int rc;
    if (strncmp((char *)buf, "ref: ", 5) == 0) {
        snprintf(out, 256, "%s", (char *)buf + 5);
        rc = 1;
    } else {
        rc = 0; /* detached: raw hash */
    }
    free(buf);
    return rc;
}

int head_resolve(char hex[41]) {
    char ref[256];
    int sym = head_symbolic_ref(ref);
    if (sym < 0)
        return -1;
    if (sym == 0) {
        /* detached: HEAD itself holds the hash */
        size_t len;
        unsigned char *buf = read_file(".git/HEAD", &len);
        if (!buf)
            return -1;
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == ' '))
            buf[--len] = '\0';
        if (len < 40) { free(buf); return -1; }
        memcpy(hex, buf, 40);
        hex[40] = '\0';
        free(buf);
        return 0;
    }
    char path[300];
    snprintf(path, sizeof(path), ".git/%s", ref);
    size_t len;
    unsigned char *buf = read_file(path, &len);
    if (!buf)
        return 1; /* unborn branch */
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == ' '))
        buf[--len] = '\0';
    if (len < 40) { free(buf); return -1; }
    memcpy(hex, buf, 40);
    hex[40] = '\0';
    free(buf);
    return 0;
}

int ref_update_head(const char *hex) {
    char ref[256];
    int sym = head_symbolic_ref(ref);
    if (sym < 0)
        return -1;
    char line[64];
    snprintf(line, sizeof(line), "%s\n", hex);
    if (sym == 1) {
        char path[300];
        snprintf(path, sizeof(path), ".git/%s", ref);
        if (mkdir_parents(path) != 0)
            return -1;
        return write_file(path, line, strlen(line));
    }
    return write_file(".git/HEAD", line, strlen(line));
}

int head_set_detached(const char *hex) {
    char line[64];
    snprintf(line, sizeof(line), "%s\n", hex);
    return write_file(".git/HEAD", line, strlen(line));
}
