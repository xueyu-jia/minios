#pragma once

int getopt(int argc, char *argv[], const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;
