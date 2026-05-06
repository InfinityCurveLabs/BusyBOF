/*
 * logger.c — BOF: write message to syslog
 * Usage: logger --message <msg> [--tag <tag>] [--priority <n>]
 */
#include "bofdefs.h"
#include <sys/socket.h>
#include <sys/un.h>

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *message      = BeaconDataExtract(&parser, NULL);
    char *tag_str      = BeaconDataExtract(&parser, NULL);
    char *priority_str = BeaconDataExtract(&parser, NULL);

    if (!message || !*message)
        BOF_ERROR("Usage: logger --message <msg> [--tag <tag>] [--priority <n>]");

    const char *tag = "user";
    if (tag_str && *tag_str)
        tag = tag_str;

    int priority = 14; /* user.info = (1<<3)|6 = 14 */
    if (priority_str && *priority_str)
        priority = atoi(priority_str);

    /* Format syslog message: <priority>tag: message */
    char syslog_msg[4200];
    snprintf(syslog_msg, sizeof(syslog_msg), "<%d>%s: %s", priority, tag, message);

    /* Send to /dev/log (Unix domain socket) */
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) BOF_ERROR("logger: socket: %s", strerror(errno));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/dev/log", sizeof(addr.sun_path) - 1);

    if (sendto(fd, syslog_msg, strlen(syslog_msg), 0,
               (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        BOF_ERROR("logger: sendto /dev/log: %s", strerror(errno));
    }
    close(fd);
    BeaconPrintf(CALLBACK_OUTPUT, "Logged: %s\n", message);
}
