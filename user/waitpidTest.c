#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int pid;
    int ret;

    printf("=== waitpid simple test ===\n");

    // Test 1: normal case (child process exists and terminates)
    pid = fork();
    if (pid == 0) {
        // child exits immediately
        exit(0);
    }
    ret = waitpid(pid);
    if (ret == 0) {
        printf("[PASS] waitpid success\n");
    } else {
        printf("[FAIL] waitpid success (got %d)\n", ret);
    }

    // Test 2: pid is not a child of current process
    ret = waitpid(1); // init process (not our child)
    if (ret == -1) {
        printf("[PASS] waitpid invalid parent\n");
    } else {
        printf("[FAIL] waitpid invalid parent (got %d)\n", ret);
    }

    // Test 3: pid does not exist
    ret = waitpid(999); // non-existent pid
    if (ret == -1) {
        printf("[PASS] waitpid no such pid\n");
    } else {
        printf("[FAIL] waitpid no such pid (got %d)\n", ret);
    }

    // Test 4: already waited child (edge case)
    ret = waitpid(pid);
    if (ret == -1) {
        printf("[PASS] waitpid already waited\n");
    } else {
        printf("[FAIL] waitpid already waited (got %d)\n", ret);
    }

    printf("=== done ===\n");
    exit(0);
}