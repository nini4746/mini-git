#include "sha1.h"
#include "util.h"
#include "object.h"
#include "index.h"
#include "tree.h"
#include "refs.h"
#include "commit.h"
#include "diff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* ---- init -------------------------------------------------------------- */

static int cmd_init(void) {
    struct stat st;
    if (stat(".git", &st) == 0)
        die("fatal: .git already exists");
    if (mkdir_p(".git/objects") != 0)
        die("fatal: cannot create .git/objects");
    if (mkdir_p(".git/refs/heads") != 0)
        die("fatal: cannot create .git/refs/heads");
    const char *head = "ref: refs/heads/main\n";
    if (write_file(".git/HEAD", head, strlen(head)) != 0)
        die("fatal: cannot write .git/HEAD");
    printf("Initialized empty minigit repository in .git/\n");
    return 0;
}

/* ---- hash-object ------------------------------------------------------- */

static int cmd_hash_object(int argc, char **argv) {
    int do_write = 0;
    const char *file = NULL;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0)
            do_write = 1;
        else
            file = argv[i];
    }
    if (!file)
        die("usage: minigit hash-object [-w] <file>");
    size_t len;
    unsigned char *data = read_file(file, &len);
    if (!data)
        die("fatal: cannot open '%s'", file);
    char hex[41];
    if (object_write("blob", data, len, do_write, hex) != 0)
        die("fatal: hash-object failed");
    free(data);
    printf("%s\n", hex);
    return 0;
}

/* ---- cat-file ---------------------------------------------------------- */

static void tree_pretty_print(const unsigned char *content, size_t len) {
    size_t i = 0;
    while (i < len) {
        /* mode (ascii) up to space */
        size_t ms = i;
        while (i < len && content[i] != ' ')
            i++;
        char modebuf[16];
        size_t ml = i - ms;
        if (ml >= sizeof(modebuf)) ml = sizeof(modebuf) - 1;
        memcpy(modebuf, content + ms, ml);
        modebuf[ml] = '\0';
        i++; /* skip space */
        /* name up to NUL */
        size_t ns = i;
        while (i < len && content[i] != '\0')
            i++;
        size_t nl = i - ns;
        char name[4096];
        if (nl >= sizeof(name)) nl = sizeof(name) - 1;
        memcpy(name, content + ns, nl);
        name[nl] = '\0';
        i++; /* skip NUL */
        /* 20 raw sha bytes */
        if (i + 20 > len)
            break;
        char hex[41];
        hex_encode(content + i, 20, hex);
        i += 20;
        unsigned int mode = (unsigned int)strtol(modebuf, NULL, 8);
        const char *type = (mode == 040000) ? "tree" : "blob";
        printf("%06o %s %s\t%s\n", mode, type, hex, name);
    }
}

static int cmd_cat_file(int argc, char **argv) {
    if (argc < 2)
        die("usage: minigit cat-file -t|-s|-p <sha>");
    const char *opt = argv[0];
    const char *hex = argv[1];
    char type[16];
    size_t size;
    unsigned char *content = object_read(hex, type, &size);
    if (!content)
        die("fatal: not a valid object name %s", hex);
    if (strcmp(opt, "-t") == 0) {
        printf("%s\n", type);
    } else if (strcmp(opt, "-s") == 0) {
        printf("%zu\n", size);
    } else if (strcmp(opt, "-p") == 0) {
        if (strcmp(type, "tree") == 0)
            tree_pretty_print(content, size);
        else
            fwrite(content, 1, size, stdout);
    } else {
        free(content);
        die("usage: minigit cat-file -t|-s|-p <sha>");
    }
    free(content);
    return 0;
}

/* ---- add --------------------------------------------------------------- */

static int path_in_git(const char *p) {
    return strcmp(p, ".git") == 0 || strncmp(p, ".git/", 5) == 0;
}

static int add_one(index_t *idx, const char *path) {
    if (path_in_git(path))
        die("fatal: cannot add .git path '%s'", path);
    struct stat st;
    if (stat(path, &st) != 0)
        die("fatal: cannot stat '%s'", path);
    if (!S_ISREG(st.st_mode))
        die("fatal: '%s' is not a regular file", path);
    size_t len;
    unsigned char *data = read_file(path, &len);
    if (!data)
        die("fatal: cannot read '%s'", path);
    char hex[41];
    if (object_write("blob", data, len, 1, hex) != 0)
        die("fatal: cannot store blob for '%s'", path);
    free(data);

    index_entry e;
    memset(&e, 0, sizeof(e));
    e.ctime_s = (uint32_t)st.st_ctime;
    e.mtime_s = (uint32_t)st.st_mtime;
    e.dev = (uint32_t)st.st_dev;
    e.ino = (uint32_t)st.st_ino;
    e.mode = (st.st_mode & 0111) ? 0100755 : 0100644;
    e.uid = (uint32_t)st.st_uid;
    e.gid = (uint32_t)st.st_gid;
    e.size = (uint32_t)st.st_size;
    if (hex_decode(hex, e.sha, 20) != 0)
        die("fatal: bad hash");
    e.path = (char *)path;
    index_upsert(idx, &e);
    return 0;
}

static int cmd_add(int argc, char **argv) {
    if (argc < 1)
        die("usage: minigit add <file>...");
    index_t idx;
    if (index_read(&idx) != 0)
        die("fatal: corrupt index");
    for (int i = 0; i < argc; i++)
        add_one(&idx, argv[i]);
    if (index_write(&idx) != 0)
        die("fatal: cannot write index");
    index_free(&idx);
    return 0;
}

/* ---- write-tree (helper for tests) ------------------------------------- */

static int cmd_write_tree(void) {
    index_t idx;
    if (index_read(&idx) != 0)
        die("fatal: corrupt index");
    if (idx.count == 0)
        die("fatal: nothing to write-tree");
    char hex[41];
    if (tree_write(&idx, hex) != 0)
        die("fatal: write-tree failed");
    index_free(&idx);
    printf("%s\n", hex);
    return 0;
}

/* ---- commit ------------------------------------------------------------ */

static int cmd_commit(int argc, char **argv) {
    const char *msg = NULL;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
            msg = argv[++i];
    }
    if (!msg)
        die("usage: minigit commit -m <message>");

    index_t idx;
    if (index_read(&idx) != 0)
        die("fatal: corrupt index");
    if (idx.count == 0)
        die("fatal: nothing to commit (empty index)");

    char tree_hex[41];
    if (tree_write(&idx, tree_hex) != 0)
        die("fatal: cannot build tree");
    index_free(&idx);

    char parent_hex[41];
    int r = head_resolve(parent_hex);
    const char *parent = (r == 0) ? parent_hex : NULL;

    char commit_hex[41];
    if (commit_create(tree_hex, parent, msg, commit_hex) != 0)
        die("fatal: cannot create commit");
    if (ref_update_head(commit_hex) != 0)
        die("fatal: cannot update ref");
    printf("%s\n", commit_hex);
    return 0;
}

/* ---- log --------------------------------------------------------------- */

static int cmd_log(void) {
    char hex[41];
    int r = head_resolve(hex);
    if (r != 0)
        die("fatal: no commits yet");
    while (1) {
        char type[16];
        size_t size;
        unsigned char *content = object_read(hex, type, &size);
        if (!content || strcmp(type, "commit") != 0) {
            free(content);
            die("fatal: bad commit %s", hex);
        }
        commit_info ci;
        commit_parse(content, size, &ci);
        printf("commit %s\n", hex);
        printf("Author: %s <%s>\n", ci.author_name, ci.author_email);
        printf("Date:   %s\n\n", ci.author_date);
        if (ci.message)
            printf("    %s\n", ci.message);
        free(content);
        if (!ci.has_parent)
            break;
        strcpy(hex, ci.parent);
    }
    return 0;
}

/* ---- checkout ---------------------------------------------------------- */

static void checkout_tree(const char *tree_hex, const char *prefix) {
    char type[16];
    size_t size;
    unsigned char *content = object_read(tree_hex, type, &size);
    if (!content || strcmp(type, "tree") != 0) {
        free(content);
        die("fatal: bad tree %s", tree_hex);
    }
    size_t i = 0;
    while (i < size) {
        size_t ms = i;
        while (i < size && content[i] != ' ')
            i++;
        char modebuf[16];
        size_t ml = i - ms;
        if (ml >= sizeof(modebuf)) ml = sizeof(modebuf) - 1;
        memcpy(modebuf, content + ms, ml);
        modebuf[ml] = '\0';
        i++;
        size_t ns = i;
        while (i < size && content[i] != '\0')
            i++;
        size_t nl = i - ns;
        char name[4096];
        if (nl >= sizeof(name)) nl = sizeof(name) - 1;
        memcpy(name, content + ns, nl);
        name[nl] = '\0';
        i++;
        if (i + 20 > size)
            break;
        char entry_hex[41];
        hex_encode(content + i, 20, entry_hex);
        i += 20;

        char path[8192];
        if (prefix && *prefix)
            snprintf(path, sizeof(path), "%s/%s", prefix, name);
        else
            snprintf(path, sizeof(path), "%s", name);

        unsigned int mode = (unsigned int)strtol(modebuf, NULL, 8);
        if (mode == 040000) {
            if (mkdir_p(path) != 0 && errno != EEXIST)
                die("fatal: cannot create dir '%s'", path);
            checkout_tree(entry_hex, path);
        } else {
            char btype[16];
            size_t bsize;
            unsigned char *bc = object_read(entry_hex, btype, &bsize);
            if (!bc)
                die("fatal: missing blob %s", entry_hex);
            if (mkdir_parents(path) != 0)
                die("fatal: cannot create parent of '%s'", path);
            if (write_file(path, bc, bsize) != 0)
                die("fatal: cannot write '%s'", path);
            mode_t fmode = (mode == 0100755) ? 0755 : 0644;
            chmod(path, fmode);
            free(bc);
        }
    }
    free(content);
}

static int cmd_checkout(int argc, char **argv) {
    if (argc < 1)
        die("usage: minigit checkout <commit-sha>");
    const char *hex = argv[0];
    if (strlen(hex) != 40)
        die("fatal: not a valid commit hash '%s'", hex);
    char type[16];
    size_t size;
    unsigned char *content = object_read(hex, type, &size);
    if (!content)
        die("fatal: object '%s' not found", hex);
    if (strcmp(type, "commit") != 0) {
        free(content);
        die("fatal: '%s' is not a commit", hex);
    }
    commit_info ci;
    commit_parse(content, size, &ci);
    free(content);
    /* restore working tree from the commit's tree */
    checkout_tree(ci.tree, "");
    if (head_set_detached(hex) != 0)
        die("fatal: cannot update HEAD");
    printf("HEAD is now at %s\n", hex);
    return 0;
}

/* ---- sha1 self-test (RFC 3174 vectors) --------------------------------- */

static int check_vec(const char *msg, const char *expect) {
    unsigned char d[20];
    char hex[41];
    sha1(msg, strlen(msg), d);
    hex_encode(d, 20, hex);
    if (strcmp(hex, expect) != 0) {
        fprintf(stderr, "sha1 FAIL: \"%.20s\" got %s want %s\n", msg, hex, expect);
        return 1;
    }
    return 0;
}

static int cmd_sha1test(void) {
    int bad = 0;
    bad |= check_vec("", "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    bad |= check_vec("abc", "a9993e364706816aba3e25717850c26c9cd0d89d");
    bad |= check_vec("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                     "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    if (!bad)
        printf("sha1 self-test OK\n");
    return bad ? 1 : 0;
}

/* ---- dispatch ---------------------------------------------------------- */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "usage: minigit <command> [args]\n"
            "commands: init hash-object cat-file add commit log checkout diff\n");
        return 1;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "sha1-test") == 0)
        return cmd_sha1test();
    if (strcmp(cmd, "init") == 0)
        return cmd_init();
    if (strcmp(cmd, "hash-object") == 0)
        return cmd_hash_object(argc - 2, argv + 2);
    if (strcmp(cmd, "cat-file") == 0)
        return cmd_cat_file(argc - 2, argv + 2);
    if (strcmp(cmd, "add") == 0)
        return cmd_add(argc - 2, argv + 2);
    if (strcmp(cmd, "write-tree") == 0)
        return cmd_write_tree();
    if (strcmp(cmd, "commit") == 0)
        return cmd_commit(argc - 2, argv + 2);
    if (strcmp(cmd, "log") == 0)
        return cmd_log();
    if (strcmp(cmd, "checkout") == 0)
        return cmd_checkout(argc - 2, argv + 2);
    if (strcmp(cmd, "diff") == 0) {
        if (argc < 4)
            die("usage: minigit diff <fileA> <fileB>");
        return diff_files(argv[2], argv[3]);
    }
    fprintf(stderr, "minigit: unknown command '%s'\n", cmd);
    return 1;
}
