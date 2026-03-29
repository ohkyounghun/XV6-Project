#include "kernel/types.h" // Include xv6 fixed-width integer types used by user-space programs.
#include "kernel/stat.h" // Include file status declarations to match the style of the existing test programs.
#include "user/user.h" // Include user-level syscall prototypes such as ps(), printf(), and exit().

// This ps test program was written with Codex assistance for the assignment.
// It exercises printing all processes, printing one specific process, and printing a non-existent pid.
int // Declare that main returns an integer status code to the xv6 runtime.
main(int argc, char *argv[]) // Define the test program entry point and accept the standard xv6 arguments.
{
    int ret; // Store each ps() return value so we can verify the syscall reported success.

    printf("=== ps simple test ===\n"); // Print a header so the test output is easy to identify on the xv6 console.

    printf("[INFO] ps(0) should print all active processes.\n"); // Explain what the first ps call is expected to display.
    ret = ps(0); // Ask the kernel to print the full process table.
    if (ret == 0) { // Check whether the syscall reported success for the full-table print.
        printf("[PASS] ps all processes\n"); // Report success for the full-table print request.
    } else { // Handle the case where the syscall reported failure for the full-table print.
        printf("[FAIL] ps all processes (got %d)\n", ret); // Print the unexpected return value for debugging.
    } // End the full-table validation branch.

    printf("[INFO] ps(1) should print only the init process.\n"); // Explain what the second ps call is expected to display.
    ret = ps(1); // Ask the kernel to print only the process whose pid is 1.
    if (ret == 0) { // Check whether the syscall reported success for the single-process print.
        printf("[PASS] ps specific pid\n"); // Report success for the single-pid print request.
    } else { // Handle the case where the syscall reported failure for the single-pid print.
        printf("[FAIL] ps specific pid (got %d)\n", ret); // Print the unexpected return value for debugging.
    } // End the single-pid validation branch.

    printf("[INFO] ps(999) should print no process line for a non-existent pid.\n"); // Explain what the invalid-pid ps call is expected to display.
    ret = ps(999); // Ask the kernel to print a process that should not exist so we can verify silent handling.
    if (ret == 0) { // Check whether the syscall still reports success for the silent no-match case.
        printf("[PASS] ps invalid pid handled silently\n"); // Report success for the invalid-pid test case.
    } else { // Handle the case where the syscall unexpectedly reported failure for an invalid pid.
        printf("[FAIL] ps invalid pid handled silently (got %d)\n", ret); // Print the unexpected return value for debugging.
    } // End the invalid-pid validation branch.

    printf("=== done ===\n"); // Print a footer so it is clear that the test finished running.
    exit(0); // Terminate the test program successfully.
} // End of the ps test program.
