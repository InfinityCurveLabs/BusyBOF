/*
 * find.c — BOF: search for files
 * Usage: find <path> [-name pattern] [-iname pattern] [-type f|d|l]
 * Basic glob name matching and type filter
 */
#define _GNU_SOURCE
#include "bofdefs.h"
#include <fnmatch.h>

static const char *g_name_pattern = NULL;
static int g_name_icase = 0;
static int g_type_filter = 0; /* 0=all, 'f'=file, 'd'=dir, 'l'=link */

#define FIND_MAX_DEPTH 64

static void find_recurse(const char *path, int depth) {
    if (depth > FIND_MAX_DEPTH) return; /* prevent symlink loops / stack overflow */

    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *ent;
    char child[PATH_MAX_BB];

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);

        struct stat st;
        if (lstat(child, &st) < 0) continue;

        /* Type filter */
        int type_ok = 1;
        if (g_type_filter == 'f' && !S_ISREG(st.st_mode)) type_ok = 0;
        if (g_type_filter == 'd' && !S_ISDIR(st.st_mode)) type_ok = 0;
        if (g_type_filter == 'l' && !S_ISLNK(st.st_mode)) type_ok = 0;

        /* Name filter */
        int name_ok = 1;
        if (g_name_pattern && fnmatch(g_name_pattern, ent->d_name, g_name_icase ? FNM_CASEFOLD : 0) != 0)
            name_ok = 0;

        if (type_ok && name_ok)
            BeaconPrintf(CALLBACK_OUTPUT, "%s\n", child);

        /* Only recurse into real directories, skip symlinks to dirs */
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode))
            find_recurse(child, depth + 1);
    }
    closedir(dir);
}

void go(char *args, int alen) {
    char *search_path = ".";
    g_name_pattern = NULL;
    g_name_icase = 0;
    g_type_filter = 0;

    if (args && alen > 0) {
        datap parser;
        BeaconDataParse(&parser, args, alen);
        char *path_arg = BeaconDataExtract(&parser, NULL);
        char *options = BeaconDataExtract(&parser, NULL);

        if (path_arg && *path_arg)
            search_path = path_arg;

        /* Parse options string (e.g. "-name *.txt -type f") */
        if (options && *options) {
            char *saveptr;
            char *tok = strtok_r(options, " ", &saveptr);
            while (tok) {
                if (strcmp(tok, "-name") == 0) {
                    tok = strtok_r(NULL, " ", &saveptr);
                    if (tok) g_name_pattern = tok;
                } else if (strcmp(tok, "-iname") == 0) {
                    tok = strtok_r(NULL, " ", &saveptr);
                    if (tok) { g_name_pattern = tok; g_name_icase = 1; }
                } else if (strcmp(tok, "-type") == 0) {
                    tok = strtok_r(NULL, " ", &saveptr);
                    if (tok) g_type_filter = tok[0];
                }
                tok = strtok_r(NULL, " ", &saveptr);
            }
        }
    }

    find_recurse(search_path, 0);
}
