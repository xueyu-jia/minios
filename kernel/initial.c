#include <minios/const.h>
#include <minios/fs.h>
#include <minios/ksignal.h>
#include <minios/proto.h>
#include <minios/type.h>
#include <minios/assert.h>

void initial() {
    execve("/bin/init", NULL, NULL);
    while (1);
}
