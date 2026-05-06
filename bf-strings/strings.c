/*
 * strings.c — BOF: find printable strings in binary files
 * Usage: strings --file <file> [--min_length <N>]
 */
#include "bofdefs.h"
#include <ctype.h>

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *filepath = BeaconDataExtract(&parser, NULL);
    char *min_str = BeaconDataExtract(&parser, NULL);

    if (!filepath || !*filepath)
        BOF_ERROR("Usage: strings --file <file> [--min_length <N>]");

    int minlen = 4;
    if (min_str && *min_str)
        minlen = atoi(min_str);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) BOF_ERROR("strings: %s: %s", filepath, strerror(errno));

    char buf[8192];
    int pos = 0;
    int c;

    while ((c = fgetc(fp)) != EOF) {
        if (isprint(c) || c == '\t') {
            if (pos < (int)sizeof(buf) - 1)
                buf[pos++] = (char)c;
        } else {
            if (pos >= minlen) {
                buf[pos] = '\0';
                BeaconPrintf(CALLBACK_OUTPUT, "%s\n", buf);
            }
            pos = 0;
        }
    }
    /* Flush remaining */
    if (pos >= minlen) {
        buf[pos] = '\0';
        BeaconPrintf(CALLBACK_OUTPUT, "%s\n", buf);
    }
    fclose(fp);
}
