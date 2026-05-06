/*
 * grep.c — BOF: search file contents
 * Usage: grep --pattern <pat> [--file <file>] [--options <inv>]
 * Basic substring matching (i=case insensitive, n=line numbers, v=invert)
 */
#include "bofdefs.h"
#include <ctype.h>

static void strtolower(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *pattern = BeaconDataExtract(&parser, NULL);
    char *file    = BeaconDataExtract(&parser, NULL);
    char *options = BeaconDataExtract(&parser, NULL);

    if (!pattern || !*pattern)
        BOF_ERROR("Usage: grep --pattern <pat> [--file <file>] [--options <inv>]");

    int ignore_case = 0, show_num = 0, invert = 0;
    if (options && *options) {
        if (strchr(options, 'i')) ignore_case = 1;
        if (strchr(options, 'n')) show_num = 1;
        if (strchr(options, 'v')) invert = 1;
    }

    /* Determine the input file. If --file was given, use it.
       Otherwise check for a pipe path embedded in pattern
       (scan past first word for a space then path starting with /). */
    char *input_file = NULL;
    if (file && *file) {
        input_file = file;
    } else {
        /* Check if pattern contains an embedded pipe path */
        char *sp = pattern;
        while (*sp && *sp != ' ') sp++;
        while (*sp == ' ') sp++;
        if (*sp == '/') {
            input_file = sp;
            /* Null-terminate pattern at the space boundary */
            char *end = pattern;
            while (*end && *end != ' ') end++;
            *end = '\0';
        }
    }

    if (!input_file)
        BOF_ERROR("grep: no input file specified");

    char pat_lower[1024];
    strncpy(pat_lower, pattern, sizeof(pat_lower) - 1);
    pat_lower[sizeof(pat_lower) - 1] = '\0';
    if (ignore_case) strtolower(pat_lower);

    FILE *fp = fopen(input_file, "r");
    if (!fp)
        BOF_ERROR("grep: %s: %s", input_file, strerror(errno));

    char line[8192];
    int linenum = 0;
    while (fgets(line, sizeof(line), fp)) {
        linenum++;
        char *search_in = line;
        char lower_line[8192];
        if (ignore_case) {
            strncpy(lower_line, line, sizeof(lower_line) - 1);
            lower_line[sizeof(lower_line) - 1] = '\0';
            strtolower(lower_line);
            search_in = lower_line;
        }

        int match = (strstr(search_in, ignore_case ? pat_lower : pattern) != NULL);
        if (invert) match = !match;

        if (match) {
            /* Remove trailing newline for cleaner output */
            size_t len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

            if (show_num)
                BeaconPrintf(CALLBACK_OUTPUT, "%d:%s\n", linenum, line);
            else
                BeaconPrintf(CALLBACK_OUTPUT, "%s\n", line);
        }
    }
    fclose(fp);
}
