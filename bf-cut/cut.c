/*
 * cut.c — BOF: extract fields by delimiter
 * Usage: cut --field <N> [--delimiter <char>] [--file <path>]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *field_str = BeaconDataExtract(&parser, NULL);
    char *delim_str = BeaconDataExtract(&parser, NULL);
    char *file_str  = BeaconDataExtract(&parser, NULL);

    if (!field_str || !*field_str)
        BOF_ERROR("Usage: cut --field <N> [--delimiter <char>] [--file <path>]");

    int field = atoi(field_str);
    if (field < 1)
        BOF_ERROR("cut: field must be >= 1");

    char delim = '\t';
    if (delim_str && *delim_str)
        delim = delim_str[0];

    /* Pipe support: executor appends pipe file path to the first packed
       string, so field_str may be "1 /tmp/.bofpipe-XXXXX". */
    char *pipe_file = field_str;
    while (*pipe_file && *pipe_file != ' ') pipe_file++;
    while (*pipe_file == ' ') pipe_file++;
    if (!*pipe_file) pipe_file = NULL;

    FILE *fp = NULL;
    if (file_str && *file_str) {
        fp = fopen(file_str, "r");
        if (!fp) BOF_ERROR("cut: %s: %s", file_str, strerror(errno));
    } else if (pipe_file) {
        fp = fopen(pipe_file, "r");
        if (!fp) BOF_ERROR("cut: %s: %s", pipe_file, strerror(errno));
    } else {
        BOF_ERROR("Usage: cut --field <N> [--delimiter <char>] [--file <path>]");
    }

    char line[8192];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        char *cur = line;
        int f = 1;
        char *start = cur;

        while (*cur) {
            if (*cur == delim) {
                if (f == field) { *cur = '\0'; break; }
                f++;
                start = cur + 1;
            }
            cur++;
        }

        if (f >= field)
            BeaconPrintf(CALLBACK_OUTPUT, "%s\n", start);
    }
    fclose(fp);
}
