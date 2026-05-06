/*
 * kill.c — BOF: send signal to process
 * Usage: kill --pid <pid> [--signal <sig>]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *pid_str    = BeaconDataExtract(&parser, NULL);
    char *signal_str = BeaconDataExtract(&parser, NULL);

    if (!pid_str || !*pid_str)
        BOF_ERROR("Usage: kill --pid <pid> [--signal <sig>]");

    int sig = SIGTERM;
    if (signal_str && *signal_str) {
        sig = atoi(signal_str);
        if (sig == 0 && strcmp(signal_str, "0") != 0) {
            /* Named signal */
            if (strcasecmp(signal_str, "HUP") == 0)       sig = SIGHUP;
            else if (strcasecmp(signal_str, "INT") == 0)   sig = SIGINT;
            else if (strcasecmp(signal_str, "KILL") == 0)  sig = SIGKILL;
            else if (strcasecmp(signal_str, "TERM") == 0)  sig = SIGTERM;
            else if (strcasecmp(signal_str, "STOP") == 0)  sig = SIGSTOP;
            else if (strcasecmp(signal_str, "CONT") == 0)  sig = SIGCONT;
            else if (strcasecmp(signal_str, "USR1") == 0)  sig = SIGUSR1;
            else if (strcasecmp(signal_str, "USR2") == 0)  sig = SIGUSR2;
            else BOF_ERROR("kill: unknown signal '%s'", signal_str);
        }
    }

    pid_t pid = (pid_t)atoi(pid_str);
    if (kill(pid, sig) != 0)
        BeaconPrintf(CALLBACK_ERROR, "kill: pid %d: %s\n", pid, strerror(errno));
    else
        BeaconPrintf(CALLBACK_OUTPUT, "Sent signal %d to pid %d\n", sig, pid);
}
