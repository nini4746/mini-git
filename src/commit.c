#include "commit.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Compute "<epoch> <tz>" for the current local time. */
static void now_date(char out[64]) {
    time_t t = time(NULL);
    struct tm lt;
    localtime_r(&t, &lt);
    long off = lt.tm_gmtoff; /* seconds east of UTC */
    char sign = off < 0 ? '-' : '+';
    long a = off < 0 ? -off : off;
    int hh = (int)(a / 3600), mm = (int)((a % 3600) / 60);
    snprintf(out, 64, "%lld %c%02d%02d", (long long)t, sign, hh, mm);
}

static const char *env_or(const char *name, const char *fallback) {
    const char *v = getenv(name);
    return (v && *v) ? v : fallback;
}

int commit_create(const char *tree_hex, const char *parent_hex,
                  const char *message, char out_hex[41]) {
    const char *an = env_or("GIT_AUTHOR_NAME", "minigit");
    const char *ae = env_or("GIT_AUTHOR_EMAIL", "minigit@example.com");
    const char *cn = env_or("GIT_COMMITTER_NAME", an);
    const char *ce = env_or("GIT_COMMITTER_EMAIL", ae);

    char nowbuf[64];
    now_date(nowbuf);
    const char *ad = env_or("GIT_AUTHOR_DATE", nowbuf);
    const char *cd = env_or("GIT_COMMITTER_DATE", ad);

    /* Ensure the message ends with exactly one trailing newline. */
    size_t mlen = strlen(message);
    int add_nl = (mlen == 0 || message[mlen - 1] != '\n');

    /* Size the buffer from the actual field lengths so untrusted env vars
       (GIT_AUTHOR_NAME, ...) cannot make snprintf truncate and push `len`
       past `cap`, which would corrupt the heap in the memcpy below. */
    size_t cap = 128 + mlen
               + strlen(tree_hex)
               + (parent_hex ? strlen(parent_hex) : 0)
               + strlen(an) + strlen(ae) + strlen(ad)
               + strlen(cn) + strlen(ce) + strlen(cd);
    char *buf = malloc(cap);
    if (!buf)
        return -1;
    size_t len = 0;
    len += snprintf(buf + len, cap - len, "tree %s\n", tree_hex);
    if (parent_hex && *parent_hex)
        len += snprintf(buf + len, cap - len, "parent %s\n", parent_hex);
    len += snprintf(buf + len, cap - len, "author %s <%s> %s\n", an, ae, ad);
    len += snprintf(buf + len, cap - len, "committer %s <%s> %s\n", cn, ce, cd);
    len += snprintf(buf + len, cap - len, "\n");
    memcpy(buf + len, message, mlen);
    len += mlen;
    if (add_nl)
        buf[len++] = '\n';

    int rc = object_write("commit", buf, len, 1, out_hex);
    free(buf);
    return rc;
}

/* Copy one whitespace-delimited token from *p into out, advance *p. */
static void take_token(const char **p, char *out, size_t outsz) {
    const char *s = *p;
    while (*s == ' ')
        s++;
    size_t i = 0;
    while (*s && *s != ' ' && *s != '\n' && i + 1 < outsz)
        out[i++] = *s++;
    out[i] = '\0';
    *p = s;
}

int commit_parse(const unsigned char *content, size_t len, commit_info *out) {
    memset(out, 0, sizeof(*out));
    const char *p = (const char *)content;
    const char *end = p + len;
    while (p < end) {
        if (*p == '\n') {
            /* blank line: message follows */
            p++;
            out->message = p;
            return 0;
        }
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl)
            break;
        size_t linelen = (size_t)(nl - p);
        if (linelen >= 5 && strncmp(p, "tree ", 5) == 0) {
            size_t n = linelen - 5;
            if (n > 40) n = 40;
            memcpy(out->tree, p + 5, n);
            out->tree[n] = '\0';
        } else if (linelen >= 7 && strncmp(p, "parent ", 7) == 0) {
            size_t n = linelen - 7;
            if (n > 40) n = 40;
            memcpy(out->parent, p + 7, n);
            out->parent[n] = '\0';
            out->has_parent = 1;
        } else if (linelen >= 7 && strncmp(p, "author ", 7) == 0) {
            /* "author NAME <EMAIL> <epoch> <tz>" */
            const char *lt = memchr(p, '<', linelen);
            const char *gt = lt ? memchr(lt, '>', (size_t)(nl - lt)) : NULL;
            if (lt && gt) {
                size_t nn = (size_t)(lt - (p + 7));
                while (nn > 0 && p[7 + nn - 1] == ' ') nn--;
                if (nn >= sizeof(out->author_name)) nn = sizeof(out->author_name) - 1;
                memcpy(out->author_name, p + 7, nn);
                out->author_name[nn] = '\0';
                size_t el = (size_t)(gt - (lt + 1));
                if (el >= sizeof(out->author_email)) el = sizeof(out->author_email) - 1;
                memcpy(out->author_email, lt + 1, el);
                out->author_email[el] = '\0';
                const char *rest = gt + 1;
                char tok[64];
                take_token(&rest, tok, sizeof(tok)); /* epoch */
                char tz[64];
                take_token(&rest, tz, sizeof(tz));   /* tz */
                snprintf(out->author_date, sizeof(out->author_date), "%s %s", tok, tz);
            }
        }
        p = nl + 1;
    }
    return 0;
}
