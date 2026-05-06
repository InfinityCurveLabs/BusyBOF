/*
 * tar.c — BOF: create, list, and extract POSIX tar archives
 * Usage: tar --mode <create|list|extract> --archive <file> [--path <dir>] [--filter <pat>]
 */
#include "bofdefs.h"

/* POSIX tar header (512 bytes) */
struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

static unsigned int oct_to_uint(const char *s, int len) {
    unsigned int val = 0;
    for (int i = 0; i < len && s[i] >= '0' && s[i] <= '7'; i++)
        val = val * 8 + (unsigned int)(s[i] - '0');
    return val;
}

static void uint_to_oct(unsigned int val, char *buf, int len) {
    buf[len - 1] = '\0';
    for (int i = len - 2; i >= 0; i--) {
        buf[i] = '0' + (char)(val & 7);
        val >>= 3;
    }
}

static unsigned int tar_checksum(struct tar_header *h) {
    unsigned char *p = (unsigned char *)h;
    unsigned int sum = 0;
    for (int i = 0; i < 512; i++) {
        /* Treat checksum field as spaces during computation */
        if (i >= 148 && i < 156)
            sum += ' ';
        else
            sum += p[i];
    }
    return sum;
}

/* ── Create ─────────────────────────────────────────── */

static int tar_add_file(FILE *out, const char *path, const char *arcname) {
    struct stat st;
    if (lstat(path, &st) < 0) return -1;

    struct tar_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    /* Handle long names with prefix */
    size_t namelen = strlen(arcname);
    if (namelen < 100) {
        strncpy(hdr.name, arcname, 100);
    } else if (namelen < 255) {
        /* Split at last / before position 155 */
        const char *slash = NULL;
        for (const char *p = arcname; p < arcname + 155 && *p; p++)
            if (*p == '/') slash = p;
        if (slash) {
            size_t preflen = (size_t)(slash - arcname);
            memcpy(hdr.prefix, arcname, preflen);
            strncpy(hdr.name, slash + 1, 100);
        } else {
            strncpy(hdr.name, arcname, 100);
        }
    } else {
        strncpy(hdr.name, arcname + namelen - 99, 99);
    }

    uint_to_oct(st.st_mode & 07777, hdr.mode, 8);
    uint_to_oct(st.st_uid, hdr.uid, 8);
    uint_to_oct(st.st_gid, hdr.gid, 8);
    uint_to_oct((unsigned int)st.st_mtime, hdr.mtime, 12);
    memcpy(hdr.magic, "ustar", 6);
    hdr.version[0] = '0';
    hdr.version[1] = '0';

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    if (pw) strncpy(hdr.uname, pw->pw_name, 31);
    if (gr) strncpy(hdr.gname, gr->gr_name, 31);

    if (S_ISDIR(st.st_mode)) {
        hdr.typeflag = '5';
        uint_to_oct(0, hdr.size, 12);
        /* Ensure directory names end with / */
        size_t nl = strlen(hdr.name);
        if (nl > 0 && nl < 99 && hdr.name[nl - 1] != '/')
            hdr.name[nl] = '/';
    } else if (S_ISLNK(st.st_mode)) {
        hdr.typeflag = '2';
        uint_to_oct(0, hdr.size, 12);
        char lnk[100];
        ssize_t r = readlink(path, lnk, sizeof(lnk) - 1);
        if (r > 0) { lnk[r] = '\0'; strncpy(hdr.linkname, lnk, 100); }
    } else {
        hdr.typeflag = '0';
        uint_to_oct((unsigned int)st.st_size, hdr.size, 12);
    }

    /* Compute checksum */
    unsigned int cksum = tar_checksum(&hdr);
    snprintf(hdr.chksum, 7, "%06o", cksum);
    hdr.chksum[6] = '\0';
    hdr.chksum[7] = ' ';

    fwrite(&hdr, 512, 1, out);

    /* Write file data in 512-byte blocks */
    if (S_ISREG(st.st_mode) && st.st_size > 0) {
        FILE *in = fopen(path, "rb");
        if (!in) return -1;
        char buf[512];
        size_t remaining = (size_t)st.st_size;
        while (remaining > 0) {
            memset(buf, 0, 512);
            size_t toread = remaining > 512 ? 512 : remaining;
            size_t got = fread(buf, 1, toread, in);
            fwrite(buf, 512, 1, out);
            remaining -= got;
            if (got < toread) break;
        }
        fclose(in);
    }
    return 0;
}

static void tar_add_recursive(FILE *out, const char *path, const char *arcname) {
    tar_add_file(out, path, arcname);

    struct stat st;
    if (lstat(path, &st) < 0 || !S_ISDIR(st.st_mode)) return;

    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        char child[PATH_MAX_BB], arcchild[PATH_MAX_BB];
        snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        snprintf(arcchild, sizeof(arcchild), "%s/%s", arcname, ent->d_name);
        tar_add_recursive(out, child, arcchild);
    }
    closedir(dir);
}

/* ── List / Extract ─────────────────────────────────── */

static void tar_fullname(struct tar_header *h, char *out, size_t outsz) {
    if (h->prefix[0]) {
        char prefix[156] = {0};
        memcpy(prefix, h->prefix, 155);
        snprintf(out, outsz, "%s/%.*s", prefix, 100, h->name);
    } else {
        snprintf(out, outsz, "%.*s", 100, h->name);
    }
}

static int mkdir_p(const char *path, mode_t mode) {
    char tmp[PATH_MAX_BB];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, mode);
            *p = '/';
        }
    }
    return mkdir(tmp, mode);
}

static void tar_extract_or_list(const char *archive, const char *destdir,
                                int list_only, const char *filter) {
    FILE *fp = fopen(archive, "rb");
    if (!fp) BOF_ERROR("tar: %s: %s", archive, strerror(errno));

    struct tar_header hdr;
    int count = 0;

    while (fread(&hdr, 512, 1, fp) == 1) {
        /* Two consecutive zero blocks = end of archive */
        if (hdr.name[0] == '\0') break;

        char name[PATH_MAX_BB];
        tar_fullname(&hdr, name, sizeof(name));
        unsigned int size = oct_to_uint(hdr.size, 12);
        unsigned int mode = oct_to_uint(hdr.mode, 8);

        /* Apply filter if provided */
        if (filter && *filter && !strstr(name, filter)) {
            /* Skip data blocks */
            if (size > 0) {
                unsigned int blocks = (size + 511) / 512;
                fseek(fp, (long)blocks * 512, SEEK_CUR);
            }
            continue;
        }

        if (list_only) {
            BeaconPrintf(CALLBACK_OUTPUT, "%s\n", name);
        } else {
            char fullpath[PATH_MAX_BB + 256];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", destdir, name);

            if (hdr.typeflag == '5') {
                /* Directory */
                mkdir_p(fullpath, (mode_t)(mode ? mode : 0755));
            } else if (hdr.typeflag == '2') {
                /* Symlink */
                char target[101] = {0};
                memcpy(target, hdr.linkname, 100);
                /* Ensure parent exists */
                char *sl = strrchr(fullpath, '/');
                if (sl) { *sl = '\0'; mkdir_p(fullpath, 0755); *sl = '/'; }
                symlink(target, fullpath);
            } else {
                /* Regular file */
                char *sl = strrchr(fullpath, '/');
                if (sl) { *sl = '\0'; mkdir_p(fullpath, 0755); *sl = '/'; }

                FILE *out = fopen(fullpath, "wb");
                if (out) {
                    unsigned int remaining = size;
                    char buf[512];
                    while (remaining > 0) {
                        if (fread(buf, 512, 1, fp) != 1) break;
                        unsigned int towrite = remaining > 512 ? 512 : remaining;
                        fwrite(buf, 1, towrite, out);
                        remaining -= towrite;
                    }
                    fclose(out);
                    chmod(fullpath, (mode_t)(mode ? mode : 0644));
                    count++;
                    size = 0; /* already consumed data blocks */
                } else {
                    BeaconPrintf(CALLBACK_ERROR, "tar: %s: %s\n", fullpath, strerror(errno));
                }
            }
            if (!list_only && hdr.typeflag != '0' && hdr.typeflag != '\0')
                count++;
        }

        /* Skip data blocks (if not already consumed) */
        if (size > 0) {
            unsigned int blocks = (size + 511) / 512;
            fseek(fp, (long)blocks * 512, SEEK_CUR);
        }
    }
    fclose(fp);

    if (list_only)
        BeaconPrintf(CALLBACK_OUTPUT, "\n%d entries\n", count);
    else
        BeaconPrintf(CALLBACK_OUTPUT, "Extracted %d items to %s\n", count, destdir);
}

/* ── Entry point ────────────────────────────────────── */

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *mode    = BeaconDataExtract(&parser, NULL);
    char *archive = BeaconDataExtract(&parser, NULL);
    char *path    = BeaconDataExtract(&parser, NULL);
    char *filter  = BeaconDataExtract(&parser, NULL);

    if (!mode || !*mode || !archive || !*archive)
        BOF_ERROR("Usage: tar --mode <create|list|extract> --archive <file> [--path <dir>] [--filter <pat>]");

    if (strcmp(mode, "create") == 0) {
        if (!path || !*path)
            BOF_ERROR("tar: create requires --path <source>");

        FILE *out = fopen(archive, "wb");
        if (!out) BOF_ERROR("tar: %s: %s", archive, strerror(errno));

        /* Use basename of path as archive root name */
        const char *arcname = strrchr(path, '/');
        arcname = arcname ? arcname + 1 : path;

        tar_add_recursive(out, path, arcname);

        /* Two zero blocks = end of archive */
        char zeros[1024];
        memset(zeros, 0, sizeof(zeros));
        fwrite(zeros, 1024, 1, out);
        fclose(out);

        struct stat st;
        stat(archive, &st);
        BeaconPrintf(CALLBACK_OUTPUT, "Created %s (%ld bytes)\n", archive, (long)st.st_size);

    } else if (strcmp(mode, "list") == 0) {
        tar_extract_or_list(archive, NULL, 1,
                            (filter && *filter) ? filter : NULL);

    } else if (strcmp(mode, "extract") == 0) {
        const char *destdir = (path && *path) ? path : ".";
        tar_extract_or_list(archive, destdir, 0,
                            (filter && *filter) ? filter : NULL);

    } else {
        BOF_ERROR("tar: unknown mode '%s' (use create, list, or extract)", mode);
    }
}
