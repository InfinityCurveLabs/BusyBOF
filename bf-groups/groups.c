/*
 * groups.c — BOF: print group memberships
 * Usage: groups [--user <name>]
 */
#include "bofdefs.h"

void go(char *args, int alen) {
    uid_t uid;
    char *username = NULL;

    datap parser;
    BeaconDataParse(&parser, args, alen);
    char *user_str = BeaconDataExtract(&parser, NULL);

    if (user_str && *user_str) {
        struct passwd *pw = getpwnam(user_str);
        if (!pw) BOF_ERROR("groups: '%s': no such user", user_str);
        uid = pw->pw_uid;
        username = user_str;
    } else {
        uid = getuid();
    }

    if (!username) {
        struct passwd *pw = getpwuid(uid);
        username = pw ? pw->pw_name : "?";
    }

    BeaconPrintf(CALLBACK_OUTPUT, "%s :", username);

    /* Read all groups from /etc/group and check membership */
    FILE *fp = fopen("/etc/group", "r");
    if (!fp) BOF_ERROR("groups: cannot read /etc/group");

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        /* Format: name:x:gid:user1,user2,... */
        char *name = strtok(line, ":");
        strtok(NULL, ":"); /* skip password */
        char *gidstr = strtok(NULL, ":");
        char *members = strtok(NULL, ":");

        if (!name || !gidstr) continue;

        /* Check primary group */
        struct passwd *pw = getpwuid(uid);
        if (pw && (gid_t)atoi(gidstr) == pw->pw_gid) {
            BeaconPrintf(CALLBACK_OUTPUT, " %s", name);
            continue;
        }

        /* Check supplementary membership */
        if (members) {
            char *saveptr;
            char *m = strtok_r(members, ",", &saveptr);
            while (m) {
                if (strcmp(m, username) == 0) {
                    BeaconPrintf(CALLBACK_OUTPUT, " %s", name);
                    break;
                }
                m = strtok_r(NULL, ",", &saveptr);
            }
        }
    }
    fclose(fp);
    BeaconPrintf(CALLBACK_OUTPUT, "\n");
}
