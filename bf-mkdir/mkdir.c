/*
 * mkdir.c — BOF: create directories
 * Args: path (required), options (optional, flags: p)
 */
#include "bofdefs.h"

static int mkdir_p(const char *path, mode_t mode) {
    char tmp[PATH_MAX_BB];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    return mkdir(tmp, mode);
}

void go(char *args, int alen) {
    int parents = 0;
    char *path = NULL;

    /* Parse named arguments */
    if (args && alen > 0) {
        datap parser;
        BeaconDataParse(&parser, args, alen);
        path          = BeaconDataExtract(&parser, NULL);
        char *options = BeaconDataExtract(&parser, NULL);

        if (options && *options) {
            if (strchr(options, 'p')) parents = 1;
        }
    }

    /* Pipe input: create each directory from previous stage */
    FILE *pipe = bof_pipe_input();
    if (pipe) {
        char line[PATH_MAX_BB];
        while (fgets(line, sizeof(line), pipe)) {
            line[strcspn(line, "\n")] = '\0';
            if (line[0]) {
                int ret = parents ? mkdir_p(line, 0755) : mkdir(line, 0755);
                if (ret != 0 && !(parents && errno == EEXIST))
                    BeaconPrintf(CALLBACK_ERROR, "mkdir: '%s': %s\n", line, strerror(errno));
                else
                    BeaconPrintf(CALLBACK_OUTPUT, "Created: %s\n", line);
            }
        }
        fclose(pipe);
        return;
    }

    /* Direct argument */
    if (!path || !*path)
        BOF_ERROR("Usage: mkdir <path> [--options p]");

    int ret = parents ? mkdir_p(path, 0755) : mkdir(path, 0755);
    if (ret != 0 && !(parents && errno == EEXIST))
        BeaconPrintf(CALLBACK_ERROR, "mkdir: '%s': %s\n", path, strerror(errno));
    else
        BeaconPrintf(CALLBACK_OUTPUT, "Created: %s\n", path);
}
