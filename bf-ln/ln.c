/*
 * ln.c — BOF: create links
 * Args: target (required), linkname (required), options (optional, flags: s)
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    if (!args || alen <= 0)
        BOF_ERROR("Usage: ln <target> <linkname> [--options s]");

    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *target   = BeaconDataExtract(&parser, NULL);
    char *linkname = BeaconDataExtract(&parser, NULL);
    char *options  = BeaconDataExtract(&parser, NULL);

    if (!target || !*target || !linkname || !*linkname)
        BOF_ERROR("Usage: ln <target> <linkname> [--options s]");

    int symbolic = 0;
    if (options && *options) {
        if (strchr(options, 's')) symbolic = 1;
    }

    int ret;
    if (symbolic)
        ret = symlink(target, linkname);
    else
        ret = link(target, linkname);

    if (ret != 0)
        BOF_ERROR("ln: '%s' -> '%s': %s", linkname, target, strerror(errno));

    BeaconPrintf(CALLBACK_OUTPUT, "'%s' -> '%s'%s\n", linkname, target,
                 symbolic ? " [symbolic]" : "");
}
