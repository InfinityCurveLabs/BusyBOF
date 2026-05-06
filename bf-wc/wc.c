/*
 * wc.c — BOF: word, line, byte count
 * Usage: wc [--file <file>] [--options lwc]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *file_str = BeaconDataExtract(&parser, NULL);
    char *opts     = BeaconDataExtract(&parser, NULL);

    int show_lines = 0, show_words = 0, show_bytes = 0;
    int explicit_flags = 0;

    if (opts && *opts) {
        if (strchr(opts, 'l')) { show_lines = 1; explicit_flags = 1; }
        if (strchr(opts, 'w')) { show_words = 1; explicit_flags = 1; }
        if (strchr(opts, 'c')) { show_bytes = 1; explicit_flags = 1; }
    }

    if (!explicit_flags) { show_lines = show_words = show_bytes = 1; }

    /* Pipe support: pipe path appended to first arg (file_str) */
    char *filepath = NULL;
    if (file_str && *file_str)
        filepath = file_str;

    if (!filepath)
        BOF_ERROR("Usage: wc --file <file> [--options lwc]");

    FILE *fp = fopen(filepath, "r");
    if (!fp) BOF_ERROR("wc: %s: %s", filepath, strerror(errno));

    long lines = 0, words = 0, bytes = 0;
    int c, in_word = 0;
    while ((c = fgetc(fp)) != EOF) {
        bytes++;
        if (c == '\n') lines++;
        if (c == ' ' || c == '\t' || c == '\n') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    fclose(fp);

    char out[256] = "";
    if (show_lines) { char tmp[32]; snprintf(tmp, sizeof(tmp), "%7ld ", lines); strcat(out, tmp); }
    if (show_words) { char tmp[32]; snprintf(tmp, sizeof(tmp), "%7ld ", words); strcat(out, tmp); }
    if (show_bytes) { char tmp[32]; snprintf(tmp, sizeof(tmp), "%7ld ", bytes); strcat(out, tmp); }
    BeaconPrintf(CALLBACK_OUTPUT, "%s%s\n", out, filepath);
}
