#include "kernel/types.h"
#include "user/user.h"
// this file generated for testing meminfo() by ohkyounghun.
int
main(void)
{
    uint64 mem = meminfo();

    if(mem > 0) {
        printf("meminfo OK: %lu bytes\n", mem);
}
    else {
        printf("meminfo FAIL: %lu\n", mem);
}
    exit(0);
}
//
// Created by 오경훈 on 26. 3. 22..
//