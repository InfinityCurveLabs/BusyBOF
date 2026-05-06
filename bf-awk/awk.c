/*
 * awk.c — BOF: minimal awk (print columns only)
 * Usage: awk --expression '{print $N}' --file <file> [--delimiter <sep>]
 * Supports: {print $1}, {print $1,$3}, {print $0}, {print $NF}
 */
#include "bofdefs.h"
#include <ctype.h>

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *expression = BeaconDataExtract(&parser, NULL);
    char *filepath = BeaconDataExtract(&parser, NULL);
    char *delim = BeaconDataExtract(&parser, NULL);
    if (!expression || !*expression)
        BOF_ERROR("Usage: awk --expression '{print $N}' --file <file> [--delimiter <sep>]");

    char sep = ' '; /* default: whitespace */
    if (delim && *delim)
        sep = delim[0];

    /* Strip surrounding quotes from expression if present */
    char *prog = expression;
    size_t elen = strlen(prog);
    if (elen >= 2 && prog[0] == '\'' && prog[elen - 1] == '\'') {
        prog[elen - 1] = '\0';
        prog++;
    }

    /* Extract embedded file path after closing '}' (injected by pipe executor) */
    char *pipe_file = NULL;
    char *brace = strchr(prog, '}');
    if (brace) {
        char *after = brace + 1;
        while (*after == ' ') after++;
        if (*after) {
            pipe_file = after;
            *brace = '\0';
        }
    }

    /* Parse field numbers from {print $1,$2,...} */
    int fields[32], nfields = 0;
    int print_nf = 0; /* print last field */
    char *p = strstr(prog, "print");
    if (!p) BOF_ERROR("awk: only {print ...} supported");
    p += 5;
    while (*p && nfields < 32) {
        if (*p == '$') {
            p++;
            if (strncmp(p, "NF", 2) == 0) { print_nf = 1; p += 2; }
            else { fields[nfields++] = atoi(p); while (isdigit(*p)) p++; }
        } else p++;
    }

    FILE *fp = NULL;
    if (filepath && *filepath) {
        fp = fopen(filepath, "r");
        if (!fp) BOF_ERROR("awk: %s: %s", filepath, strerror(errno));
    } else if (pipe_file) {
        fp = fopen(pipe_file, "r");
        if (!fp) BOF_ERROR("awk: %s: %s", pipe_file, strerror(errno));
    } else {
        BOF_ERROR("Usage: awk --expression '{print $N}' --file <file> [--delimiter <sep>]");
    }

    char line[8192];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';

        /* Keep a pristine copy for $0 (field splitting inserts NULs) */
        char line0[8192];
        strncpy(line0, line, sizeof(line0) - 1);
        line0[sizeof(line0) - 1] = '\0';

        /* Split into fields */
        char *flds[256];
        int nf = 0;
        char *s = line;
        while (*s && nf < 256) {
            if (sep == ' ') { while (*s == ' ' || *s == '\t') s++; if (!*s) break; }
            flds[nf] = s;
            if (sep == ' ') { while (*s && *s != ' ' && *s != '\t') s++; }
            else { while (*s && *s != sep) s++; }
            if (*s) { *s = '\0'; s++; }
            nf++;
        }

        for (int i = 0; i < nfields; i++) {
            if (i > 0) BeaconPrintf(CALLBACK_OUTPUT, " ");
            int idx = fields[i];
            if (idx == 0) BeaconPrintf(CALLBACK_OUTPUT, "%s", line0);
            else if (idx > 0 && idx <= nf) BeaconPrintf(CALLBACK_OUTPUT, "%s", flds[idx - 1]);
        }
        if (print_nf && nf > 0) {
            if (nfields > 0) BeaconPrintf(CALLBACK_OUTPUT, " ");
            BeaconPrintf(CALLBACK_OUTPUT, "%s", flds[nf - 1]);
        }
        BeaconPrintf(CALLBACK_OUTPUT, "\n");
    }
    fclose(fp);
}
