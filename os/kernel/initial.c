#include <kernel/const.h>
#include <kernel/fs.h>
#include <kernel/ksignal.h>
#include <kernel/proto.h>
#include <kernel/type.h>

void initial() {
    execve("/bin/init", NULL, NULL);
    while (1);
}
