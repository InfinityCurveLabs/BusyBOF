/*
 * xxd.c — BOF: hex dump
 * Usage: xxd --file <file> [--length <N>]
 */
#include "bofdefs.h"
#include <ctype.h>

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *filepath = BeaconDataExtract(&parser, NULL);
    char *len_str = BeaconDataExtract(&parser, NULL);

    if (!filepath || !*filepath)
        BOF_ERROR("Usage: xxd --file <file> [--length <N>]");

    int limit = 0; /* 0 = no limit */
    if (len_str && *len_str)
        limit = atoi(len_str);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) BOF_ERROR("xxd: %s: %s", filepath, strerror(errno));

    unsigned char buf[16];
    size_t offset = 0;
    size_t n;

    while ((n = fread(buf, 1, (limit > 0 && (size_t)limit - offset < 16) ? (size_t)limit - offset : 16, fp)) > 0) {
        if (limit > 0 && offset >= (size_t)limit) break;

        char line[128];
        size_t pos = (size_t)snprintf(line, sizeof(line), "%08zx: ", offset);

        for (size_t i = 0; i < 16; i++) {
            if (i < n) pos += (size_t)snprintf(line + pos, sizeof(line) - pos, "%02x", buf[i]);
            else pos += (size_t)snprintf(line + pos, sizeof(line) - pos, "  ");
            if (i % 2 == 1) pos += (size_t)snprintf(line + pos, sizeof(line) - pos, " ");
        }

        pos += (size_t)snprintf(line + pos, sizeof(line) - pos, " ");
        for (size_t i = 0; i < n; i++)
            line[pos++] = isprint(buf[i]) ? (char)buf[i] : '.';
        line[pos] = '\0';

        BeaconPrintf(CALLBACK_OUTPUT, "%s\n", line);
        offset += n;
    }
    fclose(fp);
}
