/*
 * bofdefs.h - Common includes and macros for BusyBOF tools
 *
 * Every BOF source should #include "bofdefs.h" as its first include.
 * This pulls in beacon.h plus standard POSIX headers commonly needed.
 */
#ifndef _BOFDEFS_H_
#define _BOFDEFS_H_

/* Beacon API */
#include "beacon.h"

/* Standard C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* POSIX */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <signal.h>

/* Helpers */
#define MAX_OUTPUT  (1024 * 256)  /* 256 KB output buffer */
#define PATH_MAX_BB 4096

/* Convenience: print error and return.
 * Uses GNU ##__VA_ARGS__ extension (supported by GCC + Clang, which is all
 * you'd ever compile a BOF with). */
#define BOF_ERROR(fmt, ...) do { \
    BeaconPrintf(CALLBACK_ERROR, fmt, ##__VA_ARGS__); \
    return; \
} while(0)

/*
 * Pipe input support.
 *
 * When a BOF runs as part of a pipe chain (e.g. bf-ls | bf-basename),
 * the executor sets BOF_PIPE=<tempfile> containing the previous stage's
 * output (one item per line).
 *
 * Path-processing BOFs (basename, dirname, du, stat, etc.) should call
 * bof_pipe_input() at the top of go(). If it returns non-NULL, iterate
 * over lines instead of processing a single path argument.
 *
 * Text-processing BOFs (grep, sort, head, etc.) do NOT need this —
 * the pipe file path is concatenated into their args and they fopen it
 * normally.
 */
static inline FILE *bof_pipe_input(void) {
    const char *p = getenv("BOF_PIPE");
    if (p && *p) return fopen(p, "r");
    return NULL;
}

/*
 * Auto-trim BeaconDataExtract results.
 *
 * When a BOF runs as a pipe sink, the executor concatenates
 * " /tmp/.bofpipe-XXXXX" onto the first packed string.  If that
 * string was empty, the result starts with a leading space.
 * Wrapping every extract through bof_trim avoids the issue
 * transparently — no BOF needs to know about the quirk.
 */
static inline char *bof_trim(char *s) {
    if (s) while (*s == ' ' || *s == '\t') s++;
    return s;
}

#define BeaconDataExtract(parser, size) \
    bof_trim(BeaconDataExtract(parser, size))

#endif /* _BOFDEFS_H_ */
