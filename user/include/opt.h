#ifndef USER_OPT_H
#define USER_OPT_H
int getopt(int argc, char *argv[],
                const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

#endif