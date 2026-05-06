/*
 * sysctl.c — BOF: read/write kernel parameters
 * Usage: sysctl [--key <key>] [--filter <pattern>]
 */
#include "bofdefs.h"

static void read_sysctl(const char *key) {
    char path[PATH_MAX_BB];
    /* Convert dot notation to path: net.ipv4.ip_forward -> /proc/sys/net/ipv4/ip_forward */
    snprintf(path, sizeof(path), "/proc/sys/%s", key);
    for (char *p = path + 10; *p; p++)
        if (*p == '.') *p = '/';

    FILE *fp = fopen(path, "r");
    if (!fp) {
        BeaconPrintf(CALLBACK_ERROR, "sysctl: %s: %s\n", key, strerror(errno));
        return;
    }
    char val[4096];
    if (fgets(val, sizeof(val), fp)) {
        val[strcspn(val, "\n")] = '\0';
        BeaconPrintf(CALLBACK_OUTPUT, "%s = %s\n", key, val);
    }
    fclose(fp);
}

static void walk_sysctl(const char *dirpath, const char *prefix, const char *filter) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char fullpath[PATH_MAX_BB], key[PATH_MAX_BB];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, ent->d_name);
        if (prefix[0])
            snprintf(key, sizeof(key), "%s.%s", prefix, ent->d_name);
        else
            snprintf(key, sizeof(key), "%s", ent->d_name);

        struct stat st;
        if (stat(fullpath, &st) < 0) continue;

        if (S_ISDIR(st.st_mode)) {
            walk_sysctl(fullpath, key, filter);
        } else if (S_ISREG(st.st_mode)) {
            if (filter && !strstr(key, filter)) continue;
            FILE *fp = fopen(fullpath, "r");
            if (fp) {
                char val[4096];
                if (fgets(val, sizeof(val), fp)) {
                    val[strcspn(val, "\n")] = '\0';
                    BeaconPrintf(CALLBACK_OUTPUT, "%s = %s\n", key, val);
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
}

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *key    = BeaconDataExtract(&parser, NULL);
    char *filter = BeaconDataExtract(&parser, NULL);

    if (key && *key) {
        /* Read specific key */
        read_sysctl(key);
    } else {
        /* List all, optionally filtered */
        walk_sysctl("/proc/sys", "", (filter && *filter) ? filter : NULL);
    }
}
