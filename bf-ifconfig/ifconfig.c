/*
 * ifconfig.c — BOF: show network interfaces (read-only)
 * Usage: ifconfig [interface]
 * Reads from /sys/class/net and /proc/net/if_inet6
 */
#include "bofdefs.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *argv_str = BeaconDataExtract(&parser, NULL);
    char *filter_iface = (argv_str && *argv_str) ? argv_str : NULL;

    DIR *dir = opendir("/sys/class/net");
    if (!dir) BOF_ERROR("ifconfig: cannot list interfaces");

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (filter_iface && strcmp(ent->d_name, filter_iface) != 0) continue;

        BeaconPrintf(CALLBACK_OUTPUT, "%s:", ent->d_name);

        /* Flags */
        char flagpath[512];
        snprintf(flagpath, sizeof(flagpath), "/sys/class/net/%s/flags", ent->d_name);
        FILE *fp = fopen(flagpath, "r");
        if (fp) {
            unsigned int flags;
            if (fscanf(fp, "%x", &flags) == 1) {
                BeaconPrintf(CALLBACK_OUTPUT, " flags=%x<", flags);
                if (flags & IFF_UP) BeaconPrintf(CALLBACK_OUTPUT, "UP,");
                if (flags & IFF_BROADCAST) BeaconPrintf(CALLBACK_OUTPUT, "BROADCAST,");
                if (flags & IFF_LOOPBACK) BeaconPrintf(CALLBACK_OUTPUT, "LOOPBACK,");
                if (flags & IFF_RUNNING) BeaconPrintf(CALLBACK_OUTPUT, "RUNNING,");
                if (flags & IFF_MULTICAST) BeaconPrintf(CALLBACK_OUTPUT, "MULTICAST,");
                BeaconPrintf(CALLBACK_OUTPUT, ">");
            }
            fclose(fp);
        }

        /* MTU */
        snprintf(flagpath, sizeof(flagpath), "/sys/class/net/%s/mtu", ent->d_name);
        fp = fopen(flagpath, "r");
        if (fp) {
            int mtu;
            if (fscanf(fp, "%d", &mtu) == 1)
                BeaconPrintf(CALLBACK_OUTPUT, "  mtu %d", mtu);
            fclose(fp);
        }
        BeaconPrintf(CALLBACK_OUTPUT, "\n");

        /* IPv4 via ioctl */
        if (sock >= 0) {
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            strncpy(ifr.ifr_name, ent->d_name, IFNAMSIZ - 1);
            if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
                struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

                char mask[INET_ADDRSTRLEN] = "?";
                if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0) {
                    struct sockaddr_in *m = (struct sockaddr_in *)&ifr.ifr_netmask;
                    inet_ntop(AF_INET, &m->sin_addr, mask, sizeof(mask));
                }
                BeaconPrintf(CALLBACK_OUTPUT, "        inet %s  netmask %s\n", ip, mask);
            }
        }

        /* IPv6 via /proc/net/if_inet6 */
        FILE *f6 = fopen("/proc/net/if_inet6", "r");
        if (f6) {
            char line6[256];
            while (fgets(line6, sizeof(line6), f6)) {
                char hex6[33], iface6[IFNAMSIZ];
                int idx, plen, scope, flags;
                if (sscanf(line6, "%32s %x %x %x %x %s", hex6, &idx, &plen, &scope, &flags, iface6) == 6
                    && strcmp(iface6, ent->d_name) == 0) {
                    unsigned char addr6[16];
                    for (int i = 0; i < 16; i++) {
                        unsigned int b;
                        sscanf(hex6 + i * 2, "%2x", &b);
                        addr6[i] = (unsigned char)b;
                    }
                    char ip6[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, addr6, ip6, sizeof(ip6));
                    const char *scopestr = "Global";
                    if (scope == 0x20) scopestr = "Link";
                    else if (scope == 0x10) scopestr = "Host";
                    else if (scope == 0x40) scopestr = "Admin";
                    else if (scope == 0x80) scopestr = "Site";
                    BeaconPrintf(CALLBACK_OUTPUT, "        inet6 %s  prefixlen %d  scopeid 0x%x<%s>\n",
                                 ip6, plen, scope, scopestr);
                }
            }
            fclose(f6);
        }

        /* HW address */
        snprintf(flagpath, sizeof(flagpath), "/sys/class/net/%s/address", ent->d_name);
        fp = fopen(flagpath, "r");
        if (fp) {
            char mac[32];
            if (fgets(mac, sizeof(mac), fp)) {
                mac[strcspn(mac, "\n")] = '\0';
                if (strcmp(mac, "00:00:00:00:00:00") != 0)
                    BeaconPrintf(CALLBACK_OUTPUT, "        ether %s\n", mac);
            }
            fclose(fp);
        }

        BeaconPrintf(CALLBACK_OUTPUT, "\n");
    }
    closedir(dir);
    if (sock >= 0) close(sock);
}
