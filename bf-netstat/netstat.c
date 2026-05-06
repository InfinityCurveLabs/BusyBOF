/*
 * netstat.c — BOF: show network connections from /proc/net
 * Usage: netstat [-tlnp]
 */
#include "bofdefs.h"
#include <dirent.h>
#include <unistd.h>

static const char *tcp_state(unsigned int st) {
    const char *states[] = {
        "?", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1", "FIN_WAIT2",
        "TIME_WAIT", "CLOSE", "CLOSE_WAIT", "LAST_ACK", "LISTEN", "CLOSING"
    };
    if (st <= 11) return states[st];
    return "UNKNOWN";
}

static void parse_addr(const char *hex, char *out, size_t outsz) {
    unsigned int ip = 0, port = 0;
    if (sscanf(hex, "%X:%X", &ip, &port) < 2) { snprintf(out, outsz, "?:?"); return; }
    snprintf(out, outsz, "%d.%d.%d.%d:%d",
        ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF, port);
}

static void find_pid_by_inode(unsigned long inode, char *out, size_t outsz) {
    out[0] = '-'; out[1] = '\0';
    if (inode == 0) return;

    char target[64];
    snprintf(target, sizeof(target), "socket:[%lu]", inode);

    DIR *proc = opendir("/proc");
    if (!proc) return;

    struct dirent *pde;
    while ((pde = readdir(proc)) != NULL) {
        if (pde->d_name[0] < '0' || pde->d_name[0] > '9') continue;

        char fddir[280];
        snprintf(fddir, sizeof(fddir), "/proc/%s/fd", pde->d_name);
        DIR *fds = opendir(fddir);
        if (!fds) continue;

        struct dirent *fde;
        while ((fde = readdir(fds)) != NULL) {
            char linkpath[544], linkval[128];
            snprintf(linkpath, sizeof(linkpath), "%s/%s", fddir, fde->d_name);
            ssize_t r = readlink(linkpath, linkval, sizeof(linkval) - 1);
            if (r <= 0) continue;
            linkval[r] = '\0';
            if (strcmp(linkval, target) == 0) {
                /* Found the PID, read comm */
                char commpath[280], comm[64] = "";
                snprintf(commpath, sizeof(commpath), "/proc/%s/comm", pde->d_name);
                FILE *cf = fopen(commpath, "r");
                if (cf) {
                    if (fgets(comm, sizeof(comm), cf)) {
                        comm[strcspn(comm, "\n")] = '\0';
                    }
                    fclose(cf);
                }
                snprintf(out, outsz, "%.32s/%s", pde->d_name, comm[0] ? comm : "?");
                closedir(fds);
                closedir(proc);
                return;
            }
        }
        closedir(fds);
    }
    closedir(proc);
}

static void show_connections(const char *path, const char *proto, int listen_only, int show_proc, int is_udp) {
    FILE *fp = fopen(path, "r");
    if (!fp) return;

    char line[512];
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; } /* skip header */

    while (fgets(line, sizeof(line), fp)) {
        unsigned int idx, state, uid;
        unsigned long inode;
        char local_hex[64], remote_hex[64];

        if (sscanf(line, " %u: %63s %63s %X %*s %*s %*s %u %*d %lu",
                   &idx, local_hex, remote_hex, &state, &uid, &inode) < 5)
            continue;

        if (listen_only) {
            if (is_udp && state != 7) continue;  /* 7 = UDP unconnected (listening) */
            if (!is_udp && state != 10) continue; /* 10 = TCP LISTEN */
        }

        char local[64], remote[64];
        parse_addr(local_hex, local, sizeof(local));
        parse_addr(remote_hex, remote, sizeof(remote));

        const char *ststr = is_udp ? (state == 1 ? "ESTABLISHED" : "UNCONN") : tcp_state(state);

        if (show_proc) {
            char proc_info[128];
            find_pid_by_inode(inode, proc_info, sizeof(proc_info));
            BeaconPrintf(CALLBACK_OUTPUT, "%-6s %-22s %-22s %-12s %s\n",
                proto, local, remote, ststr, proc_info);
        } else {
            BeaconPrintf(CALLBACK_OUTPUT, "%-6s %-22s %-22s %-12s %lu\n",
                proto, local, remote, ststr, inode);
        }
    }
    fclose(fp);
}

void go(char *args, int alen) {
    int listen_only = 0, show_tcp = 1, show_udp = 1, show_proc = 0;

    if (args && alen > 0) {
        datap parser;
        BeaconDataParse(&parser, args, alen);
        char *argv_str = BeaconDataExtract(&parser, NULL);
        if (argv_str && *argv_str) {
            /* Parse flags from -tlnpu style argument */
            char *p = argv_str;
            if (*p == '-') p++;
            int has_t = 0, has_u = 0;
            for (; *p && *p != ' '; p++) {
                if (*p == 'l') listen_only = 1;
                if (*p == 't') has_t = 1;
                if (*p == 'u') has_u = 1;
                if (*p == 'p') show_proc = 1;
            }
            if (has_t && !has_u) show_udp = 0;
            if (has_u && !has_t) show_tcp = 0;
        }
    }

    BeaconPrintf(CALLBACK_OUTPUT, "%-6s %-22s %-22s %-12s %s\n",
        "Proto", "Local Address", "Foreign Address", "State", show_proc ? "PID/Program" : "Inode");

    if (show_tcp) show_connections("/proc/net/tcp", "tcp", listen_only, show_proc, 0);
    if (show_udp) show_connections("/proc/net/udp", "udp", listen_only, show_proc, 1);
    if (show_tcp) show_connections("/proc/net/tcp6", "tcp6", listen_only, show_proc, 0);
    if (show_udp) show_connections("/proc/net/udp6", "udp6", listen_only, show_proc, 1);
}
