/*
 * od.c — BOF: octal dump
 * Usage: od --file <file> [--address_format x|o|d|n] [--type_format x|o|c] [--max_bytes N]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *filepath = BeaconDataExtract(&parser, NULL);
    char *addr_fmt_str = BeaconDataExtract(&parser, NULL);
    char *type_fmt_str = BeaconDataExtract(&parser, NULL);
    char *max_bytes_str = BeaconDataExtract(&parser, NULL);

    if (!filepath || !*filepath)
        BOF_ERROR("Usage: od --file <file> [--address_format x|o|d|n] [--type_format x|o|c] [--max_bytes N]");

    char addr_fmt = 'o';  /* o=octal, x=hex, d=decimal, n=none */
    char type = 'o';      /* o=octal bytes, x=hex bytes, c=chars */
    int max_bytes = 0;    /* 0 = all */

    if (addr_fmt_str && *addr_fmt_str)
        addr_fmt = addr_fmt_str[0];
    if (type_fmt_str && *type_fmt_str)
        type = type_fmt_str[0];
    if (max_bytes_str && *max_bytes_str)
        max_bytes = atoi(max_bytes_str);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) BOF_ERROR("od: %s: %s", filepath, strerror(errno));

    unsigned char buf[16];
    size_t offset = 0;
    size_t n;
    int total = 0;

    while ((n = fread(buf, 1, 16, fp)) > 0) {
        if (max_bytes > 0 && total + (int)n > max_bytes)
            n = (size_t)(max_bytes - total);

        /* Address */
        switch (addr_fmt) {
        case 'x': BeaconPrintf(CALLBACK_OUTPUT, "%07zx", offset); break;
        case 'd': BeaconPrintf(CALLBACK_OUTPUT, "%07zd", offset); break;
        case 'n': break;
        default:  BeaconPrintf(CALLBACK_OUTPUT, "%07zo", offset); break;
        }

        /* Data */
        for (size_t i = 0; i < n; i++) {
            switch (type) {
            case 'x':
                BeaconPrintf(CALLBACK_OUTPUT, " %02x", buf[i]);
                break;
            case 'c':
                if (buf[i] >= 0x20 && buf[i] < 0x7F)
                    BeaconPrintf(CALLBACK_OUTPUT, " %3c", buf[i]);
                else if (buf[i] == '\n') BeaconPrintf(CALLBACK_OUTPUT, "  \\n");
                else if (buf[i] == '\t') BeaconPrintf(CALLBACK_OUTPUT, "  \\t");
                else if (buf[i] == '\0') BeaconPrintf(CALLBACK_OUTPUT, "  \\0");
                else BeaconPrintf(CALLBACK_OUTPUT, " %03o", buf[i]);
                break;
            default:
                BeaconPrintf(CALLBACK_OUTPUT, " %03o", buf[i]);
                break;
            }
        }
        BeaconPrintf(CALLBACK_OUTPUT, "\n");

        offset += n;
        total += (int)n;
        if (max_bytes > 0 && total >= max_bytes) break;
    }

    /* Final address */
    if (addr_fmt != 'n') {
        switch (addr_fmt) {
        case 'x': BeaconPrintf(CALLBACK_OUTPUT, "%07zx\n", offset); break;
        case 'd': BeaconPrintf(CALLBACK_OUTPUT, "%07zd\n", offset); break;
        default:  BeaconPrintf(CALLBACK_OUTPUT, "%07zo\n", offset); break;
        }
    }
    fclose(fp);
}
