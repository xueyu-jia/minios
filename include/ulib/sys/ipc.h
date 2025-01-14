#pragma once

#include <sys/types.h>

#define IPC_CREAT 00001000  //<! create if key is nonexistent
#define IPC_EXCL 00002000   //<! fail if key exists
#define IPC_NOWAIT 00004000 //<! return error on wait

#define IPC_RMID 0 //<! remove resource
#define IPC_SET 1  //<! set ipc_perm options
#define IPC_STAT 2 //<! get ipc_perm options
#define IPC_INFO 3 //<! see ipcs

key_t ftok(const char *path, int id);
