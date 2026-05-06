/*
 * uniq.c — BOF: remove duplicate adjacent lines
 * Usage: uniq [--file <file>] [--options c]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *file_str = BeaconDataExtract(&parser, NULL);
    char *opts     = BeaconDataExtract(&parser, NULL);

    int show_count = 0;
    if (opts && *opts) {
        if (strchr(opts, 'c')) show_count = 1;
    }

    /* Pipe support: pipe path appended to first arg (file_str) */
    char *filepath = NULL;
    if (file_str && *file_str)
        filepath = file_str;

    if (!filepath) BOF_ERROR("Usage: uniq --file <file> [--options c]");

    FILE *fp = fopen(filepath, "r");
    if (!fp) BOF_ERROR("uniq: %s: %s", filepath, strerror(errno));

    char prev[8192] = "", line[8192];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strcmp(line, prev) == 0) {
            count++;
        } else {
            if (count > 0) {
                if (show_count) BeaconPrintf(CALLBACK_OUTPUT, "%7d %s", count, prev);
                else BeaconPrintf(CALLBACK_OUTPUT, "%s", prev);
            }
            strncpy(prev, line, sizeof(prev) - 1);
            prev[sizeof(prev) - 1] = '\0';
            count = 1;
        }
    }
    if (count > 0) {
        if (show_count) BeaconPrintf(CALLBACK_OUTPUT, "%7d %s", count, prev);
        else BeaconPrintf(CALLBACK_OUTPUT, "%s", prev);
    }
    fclose(fp);
}
