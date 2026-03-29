#include "kernel/types.h" // Include xv6 fixed-width integer types such as int and uint64.
#include "kernel/stat.h" // Include file status declarations to match the style of other user test programs.
#include "user/user.h" // Include user-level syscall prototypes such as fork(), getnice(), and exit().

// This getnice test program was written with Codex assistance for the assignment.
// It checks the default nice value for known processes and verifies failure on an invalid pid.
int // Declare that main returns an integer status code to the xv6 runtime.
main(int argc, char *argv[]) // Define the test program entry point and accept standard xv6 arguments.
{
    int ret; // Store each getnice() return value so we can compare it with the expected result.

    printf("=== getnice simple test ===\n"); // Print a header so the test output is easy to identify on the xv6 console.

    ret = getnice(1); // Query the nice value of init, which should exist after boot.
    if (ret == 20) { // Check whether init has the default nice value required by the assignment.
        printf("[PASS] getnice init default value\n"); // Report success when init's nice value is correct.
    } else { // Handle the case where init does not have the expected nice value.
        printf("[FAIL] getnice init default value (got %d)\n", ret); // Print the returned value for debugging.
    } // End the first validation branch.

    ret = getnice(2); // Query the nice value of the shell process, which should also exist in the normal boot flow.
    if (ret == 20) { // Check whether the shell also starts with the default nice value.
        printf("[PASS] getnice shell default value\n"); // Report success when the shell's nice value is correct.
    } else { // Handle the case where the shell does not have the expected nice value.
        printf("[FAIL] getnice shell default value (got %d)\n", ret); // Print the unexpected return value.
    } // End the second validation branch.

    ret = getnice(999); // Query a pid that should not exist so we can verify the failure path.
    if (ret == -1) { // Confirm that the syscall reports failure for a non-existent process.
        printf("[PASS] getnice invalid pid\n"); // Report success for the invalid-pid test case.
    } else { // Handle the case where the syscall incorrectly accepted a non-existent pid.
        printf("[FAIL] getnice invalid pid (got %d)\n", ret); // Print the incorrect value that was returned.
    } // End the invalid-pid validation branch.

    printf("=== done ===\n"); // Print a footer so it is clear that the test finished running.
    exit(0); // Terminate the test program successfully.
} // End of the getnice test program.
