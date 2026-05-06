/*
 * ping.c — BOF: test host reachability via raw ICMP
 * Usage: ping --host <host> [--count <n>]
 * Note: requires CAP_NET_RAW or root
 */
#include "bofdefs.h"
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

static uint16_t checksum(void *data, int len) {
    uint32_t sum = 0;
    uint16_t *p = (uint16_t *)data;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len == 1) sum += *(uint8_t *)p;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)~sum;
}

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *host      = BeaconDataExtract(&parser, NULL);
    char *count_str = BeaconDataExtract(&parser, NULL);

    if (!host || !*host)
        BOF_ERROR("Usage: ping --host <host> [--count <n>]");

    int count = 3;
    if (count_str && *count_str)
        count = atoi(count_str);
    if (count < 1) count = 1;

    /* Resolve hostname */
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    if (getaddrinfo(host, NULL, &hints, &res) != 0)
        BOF_ERROR("ping: cannot resolve '%s'", host);

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        /* Fallback: try DGRAM ICMP (unprivileged) */
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
        if (sock < 0) {
            freeaddrinfo(res);
            BOF_ERROR("ping: socket: %s (need CAP_NET_RAW)", strerror(errno));
        }
    }

    struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    BeaconPrintf(CALLBACK_OUTPUT, "PING %s (%s): %d packets\n", host, ip_str, count);

    int sent = 0, recv_ok = 0;
    for (int i = 0; i < count; i++) {
        struct icmphdr icmp = {0};
        icmp.type = ICMP_ECHO;
        icmp.un.echo.id = (uint16_t)(getpid() & 0xFFFF);
        icmp.un.echo.sequence = (uint16_t)i;
        icmp.checksum = checksum(&icmp, (int)sizeof(icmp));

        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        if (sendto(sock, &icmp, sizeof(icmp), 0,
                   (struct sockaddr *)addr, sizeof(*addr)) > 0) {
            sent++;
            char buf[1024];
            if (recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL) > 0) {
                gettimeofday(&t2, NULL);
                double ms = (double)(t2.tv_sec - t1.tv_sec) * 1000.0 +
                            (double)(t2.tv_usec - t1.tv_usec) / 1000.0;
                recv_ok++;
                BeaconPrintf(CALLBACK_OUTPUT, "Reply from %s: seq=%d time=%.1fms\n",
                    ip_str, i, ms);
            } else {
                BeaconPrintf(CALLBACK_OUTPUT, "Request timeout: seq=%d\n", i);
            }
        }
    }

    close(sock);
    freeaddrinfo(res);

    int loss = sent > 0 ? ((sent - recv_ok) * 100 / sent) : 100;
    BeaconPrintf(CALLBACK_OUTPUT, "\n--- %s ping statistics ---\n"
        "%d packets transmitted, %d received, %d%% loss\n",
        host, sent, recv_ok, loss);
}
