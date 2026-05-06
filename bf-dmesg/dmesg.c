/*
 * dmesg.c — BOF: print kernel ring buffer
 * Usage: dmesg [--lines <N>]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    int max_lines = 0; /* 0 = all */

    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *lines_str = BeaconDataExtract(&parser, NULL);

    if (lines_str && *lines_str)
        max_lines = atoi(lines_str);

    /*
     * /dev/kmsg is a streaming device — fgets blocks forever waiting for
     * new messages unless O_NONBLOCK is set.  Open non-blocking and use
     * SEEK_SET (offset 0) to start from the oldest buffered message.
     */
    int kmsg_fd = open("/dev/kmsg", O_RDONLY | O_NONBLOCK);
    FILE *fp = NULL;
    if (kmsg_fd >= 0) {
        lseek(kmsg_fd, 0, SEEK_SET);
        fp = fdopen(kmsg_fd, "r");
        if (!fp) close(kmsg_fd);
    }
    if (!fp) {
        fp = fopen("/var/log/kern.log", "r");
        if (!fp) fp = fopen("/var/log/dmesg", "r");
        if (!fp) BOF_ERROR("dmesg: cannot access kernel log (need root or /var/log/kern.log)");
    }

    char line[1024];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        /* /dev/kmsg format: "priority,seq,timestamp,-;message" */
        char *msg = strchr(line, ';');
        if (msg) {
            msg++; /* skip ';' */
            BeaconPrintf(CALLBACK_OUTPUT, "%s", msg);
        } else {
            /* /proc/kmsg or kern.log: raw lines */
            BeaconPrintf(CALLBACK_OUTPUT, "%s", line);
        }
        count++;
        if (max_lines > 0 && count >= max_lines) break;
    }
    fclose(fp);
}
