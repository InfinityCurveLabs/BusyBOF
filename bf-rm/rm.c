/*
 * rm.c — BOF: remove files/directories
 * Args: path (required), options (optional, flags: r f)
 */
#include "bofdefs.h"

static int rm_recursive(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) return -1;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;

        struct dirent *ent;
        char child[PATH_MAX_BB];
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
            rm_recursive(child);
        }
        closedir(dir);
        return rmdir(path);
    }
    return unlink(path);
}

static void rm_one(const char *path, int recursive, int force, int *count) {
    int ret = recursive ? rm_recursive(path) : unlink(path);
    if (ret != 0 && !force)
        BeaconPrintf(CALLBACK_ERROR, "rm: cannot remove '%s': %s\n", path, strerror(errno));
    else
        (*count)++;
}

void go(char *args, int alen) {
    int recursive = 0, force = 0, count = 0;
    char *path = NULL;

    /* Parse named arguments */
    if (args && alen > 0) {
        datap parser;
        BeaconDataParse(&parser, args, alen);
        path          = BeaconDataExtract(&parser, NULL);
        char *options = BeaconDataExtract(&parser, NULL);

        if (options && *options) {
            if (strchr(options, 'r')) recursive = 1;
            if (strchr(options, 'f')) force     = 1;
        }
    }

    /* Pipe input: remove each path from previous stage */
    FILE *pipe = bof_pipe_input();
    if (pipe) {
        char line[PATH_MAX_BB];
        while (fgets(line, sizeof(line), pipe)) {
            line[strcspn(line, "\n")] = '\0';
            if (line[0]) rm_one(line, recursive, force, &count);
        }
        fclose(pipe);
    }

    /* Direct argument */
    if (path && *path)
        rm_one(path, recursive, force, &count);

    if (count > 0)
        BeaconPrintf(CALLBACK_OUTPUT, "Removed %d item(s)\n", count);
    else if (!pipe)
        BOF_ERROR("Usage: rm <path> [--options rf]");
}
