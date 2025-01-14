#pragma once

#include <uapi/minios/stat.h> // IWYU pragma: export

int stat(const char *path, struct stat *buf);
