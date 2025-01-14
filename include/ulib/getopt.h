#pragma once

extern char *optarg;
extern int optind, opterr, optopt;

int getopt(int argc, char *argv[], const char *optstring);
