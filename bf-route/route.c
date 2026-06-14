/*
 * route.c — BOF: show routing table
 * Usage: route
 */
#include "bofdefs.h"

static void hex_to_ip(const char *hex, char *out, size_t outsz) {
    unsigned int ip = 0;
    sscanf(hex, "%X", &ip);
    snprintf(out, outsz, "%d.%d.%d.%d",
        ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
}

void go(char *args, int alen) {
    FILE *fp = fopen("/proc/net/route", "r");
    if (!fp) BOF_ERROR("route: cannot read /proc/net/route");

    BeaconPrintf(CALLBACK_OUTPUT, "%-16s %-16s %-16s %-6s %-6s %-6s %s\n",
        "Destination", "Gateway", "Genmask", "Flags", "Metric", "Ref", "Iface");

    char line[512];
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; } /* skip header */

    while (fgets(line, sizeof(line), fp)) {
        char iface[32], dest_hex[16], gw_hex[16], mask_hex[16];
        unsigned int flags, refcnt, use, metric, mtu, window, irtt;

        if (sscanf(line, "%31s %15s %15s %X %u %u %u %15s %u %u %u",
                   iface, dest_hex, gw_hex, &flags, &refcnt, &use,
                   &metric, mask_hex, &mtu, &window, &irtt) < 8)
            continue;

        char dest[20], gw[20], mask[20];
        hex_to_ip(dest_hex, dest, sizeof(dest));
        hex_to_ip(gw_hex, gw, sizeof(gw));
        hex_to_ip(mask_hex, mask, sizeof(mask));

        char flagstr[8] = "";
        int fi = 0;
        if (flags & 0x0001) flagstr[fi++] = 'U';
        if (flags & 0x0002) flagstr[fi++] = 'G';
        if (flags & 0x0004) flagstr[fi++] = 'H';
        flagstr[fi] = '\0';

        BeaconPrintf(CALLBACK_OUTPUT, "%-16s %-16s %-16s %-6s %-6u %-6u %s\n",
            dest, gw, mask, flagstr, metric, refcnt, iface);
    }
    fclose(fp);
}
