/*
 * tail.c — BOF: output last part of file
 * Usage: tail --file <file> [--lines <N>]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *filepath = BeaconDataExtract(&parser, NULL);
    char *lines_str = BeaconDataExtract(&parser, NULL);

    if (!filepath || !*filepath)
        BOF_ERROR("Usage: tail --file <file> [--lines <N>]");

    int nlines = 10;
    if (lines_str && *lines_str)
        nlines = atoi(lines_str);

    FILE *fp = fopen(filepath, "r");
    if (!fp) BOF_ERROR("tail: %s: %s", filepath, strerror(errno));

    /* Count total lines */
    char line[4096];
    int total = 0;
    while (fgets(line, sizeof(line), fp)) total++;

    int skip = total - nlines;
    if (skip < 0) skip = 0;

    rewind(fp);
    int cur = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (cur >= skip)
            BeaconPrintf(CALLBACK_OUTPUT, "%s", line);
        cur++;
    }
    fclose(fp);
}
