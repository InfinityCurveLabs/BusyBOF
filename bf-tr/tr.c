/*
 * tr.c — BOF: translate/delete characters from stdin-like input (file)
 * Usage: tr --set1 <chars> [--set2 <chars>] [--file <path>] [--options d]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *set1     = BeaconDataExtract(&parser, NULL);
    char *set2     = BeaconDataExtract(&parser, NULL);
    char *file_str = BeaconDataExtract(&parser, NULL);
    char *opts     = BeaconDataExtract(&parser, NULL);

    if (!set1 || !*set1)
        BOF_ERROR("Usage: tr --set1 <chars> [--set2 <chars>] [--file <path>] [--options d]");

    int delete_mode = 0;
    if (opts && *opts) {
        if (strchr(opts, 'd')) delete_mode = 1;
    }

    /*
     * Pipe support: the pipe executor appends the temp file path to the
     * first packed string (set1).  Scan for a space followed by a '/'
     * to detect and extract it.
     */
    char *pipe_file = NULL;
    char *scan = set1;
    while (*scan) {
        if (*scan == ' ' && *(scan + 1) == '/') {
            *scan = '\0';
            pipe_file = scan + 1;
            break;
        }
        scan++;
    }

    char *filepath = NULL;
    if (file_str && *file_str)
        filepath = file_str;
    else if (pipe_file)
        filepath = pipe_file;

    if (!filepath)
        BOF_ERROR("Usage: tr --set1 <chars> [--set2 <chars>] [--file <path>] [--options d]");

    if (!delete_mode && (!set2 || !*set2))
        BOF_ERROR("tr: set2 is required unless delete mode (-d)");

    FILE *fp = fopen(filepath, "r");
    if (!fp) BOF_ERROR("tr: %s: %s", filepath, strerror(errno));

    /* Build translation table */
    unsigned char map[256];
    for (int i = 0; i < 256; i++) map[i] = (unsigned char)i;

    size_t s1len = strlen(set1);
    size_t s2len = set2 ? strlen(set2) : 0;

    if (!delete_mode && set2) {
        for (size_t i = 0; i < s1len; i++) {
            unsigned char from = (unsigned char)set1[i];
            unsigned char to = (i < s2len) ? (unsigned char)set2[i] : (unsigned char)set2[s2len - 1];
            map[from] = to;
        }
    }

    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (delete_mode) {
            if (!memchr(set1, c, s1len))
                BeaconPrintf(CALLBACK_OUTPUT, "%c", c);
        } else {
            BeaconPrintf(CALLBACK_OUTPUT, "%c", map[(unsigned char)c]);
        }
    }
    fclose(fp);
}
