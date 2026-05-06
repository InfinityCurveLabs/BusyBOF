/*
 * head.c — BOF: output first part of file
 * Usage: head --file <file> [--lines <N>]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *filepath = BeaconDataExtract(&parser, NULL);
    char *lines_str = BeaconDataExtract(&parser, NULL);

    if (!filepath || !*filepath)
        BOF_ERROR("Usage: head --file <file> [--lines <N>]");

    int nlines = 10;
    if (lines_str && *lines_str)
        nlines = atoi(lines_str);

    FILE *fp = fopen(filepath, "r");
    if (!fp) BOF_ERROR("head: %s: %s", filepath, strerror(errno));

    char line[4096];
    int count = 0;
    while (count < nlines && fgets(line, sizeof(line), fp)) {
        BeaconPrintf(CALLBACK_OUTPUT, "%s", line);
        count++;
    }
    fclose(fp);
}
