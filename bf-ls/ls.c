/*
 * ls.c — BOF: list directory contents
 * Args: path (optional, default "."), options (optional, flags: l a F)
 */
#include "bofdefs.h"
#include <sys/ioctl.h>

static void mode_str(mode_t m, char *buf) {
    buf[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISCHR(m) ? 'c' :
             S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
    buf[1] = (m & S_IRUSR) ? 'r' : '-';
    buf[2] = (m & S_IWUSR) ? 'w' : '-';
    buf[3] = (m & S_IXUSR) ? 'x' : '-';
    buf[4] = (m & S_IRGRP) ? 'r' : '-';
    buf[5] = (m & S_IWGRP) ? 'w' : '-';
    buf[6] = (m & S_IXGRP) ? 'x' : '-';
    buf[7] = (m & S_IROTH) ? 'r' : '-';
    buf[8] = (m & S_IWOTH) ? 'w' : '-';
    buf[9] = (m & S_IXOTH) ? 'x' : '-';
    buf[10] = '\0';
}

void go(char *args, int alen) {
    char path[PATH_MAX_BB] = ".";
    int long_fmt = 0, show_all = 0, full_path = 0;

    if (args && alen > 0) {
        datap parser;
        BeaconDataParse(&parser, args, alen);
        char *path_arg = BeaconDataExtract(&parser, NULL);
        char *options  = BeaconDataExtract(&parser, NULL);

        if (path_arg && *path_arg) {
            strncpy(path, path_arg, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        }
        if (options && *options) {
            if (strchr(options, 'l')) long_fmt  = 1;
            if (strchr(options, 'a')) show_all  = 1;
            if (strchr(options, 'F')) full_path = 1;
        }
    }

    DIR *dir = opendir(path);
    if (!dir) {
        BOF_ERROR("ls: %s: %s", path, strerror(errno));
    }

    struct dirent *ent;
    char fullpath[PATH_MAX_BB + 256]; /* path + d_name */
    struct stat st;

    while ((ent = readdir(dir)) != NULL) {
        if (!show_all && ent->d_name[0] == '.') continue;

        if (long_fmt) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->d_name);
            if (lstat(fullpath, &st) < 0) {
                BeaconPrintf(CALLBACK_OUTPUT, "?????????? ? ? ? ? %s\n", ent->d_name);
                continue;
            }

            char modestr[12];
            mode_str(st.st_mode, modestr);

            struct passwd *pw = getpwuid(st.st_uid);
            struct group  *gr = getgrgid(st.st_gid);
            char timebuf[64];
            struct tm *tm = localtime(&st.st_mtime);
            strftime(timebuf, sizeof(timebuf), "%b %2d %H:%M", tm);

            BeaconPrintf(CALLBACK_OUTPUT, "%s %3ld %-8s %-8s %8ld %s %s\n",
                modestr,
                (long)st.st_nlink,
                pw ? pw->pw_name : "?",
                gr ? gr->gr_name : "?",
                (long)st.st_size,
                timebuf,
                full_path ? fullpath : ent->d_name);
        } else {
            if (full_path) {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->d_name);
                BeaconPrintf(CALLBACK_OUTPUT, "%s\n", fullpath);
            } else {
                BeaconPrintf(CALLBACK_OUTPUT, "%s\n", ent->d_name);
            }
        }
    }
    closedir(dir);
}
