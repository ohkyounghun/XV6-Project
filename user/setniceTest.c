#include "kernel/types.h" // Include xv6 fixed-width integer types used by user-space programs.
#include "kernel/stat.h" // Include file status declarations to match the style of the existing test programs.
#include "user/user.h" // Include user-level syscall prototypes such as getnice(), setnice(), printf(), and exit().

// This setnice test program was written with Codex assistance for the assignment.
// It checks successful nice updates and verifies the failure cases for invalid pid and invalid nice range.
int // Declare that main returns an integer status code to the xv6 runtime.
main(int argc, char *argv[]) // Define the test program entry point and accept the standard xv6 arguments.
{
    int ret; // Store each syscall return value so we can compare it with the expected result.
    int original_nice; // Store the original nice value so we can restore it after the update test.

    printf("=== setnice simple test ===\n"); // Print a header so the test output is easy to identify on the xv6 console.

    original_nice = getnice(1); // Read init's current nice value before we change it so we can restore it later.
    if (original_nice == 20) { // Confirm that init begins with the default nice value expected by the assignment.
        printf("[PASS] getnice before setnice\n"); // Report success when the initial state is correct.
    } else { // Handle the case where init does not begin with the expected default value.
        printf("[FAIL] getnice before setnice (got %d)\n", original_nice); // Print the unexpected initial value for debugging.
    } // End the initial-value validation branch.

    ret = setnice(1, 10); // Try changing init's nice value to a valid value inside the allowed range.
    if (ret == 0) { // Check whether the syscall reported success for the valid update request.
        printf("[PASS] setnice valid update\n"); // Report success when the valid update was accepted.
    } else { // Handle the case where the valid update request failed.
        printf("[FAIL] setnice valid update (got %d)\n", ret); // Print the failure code returned by the syscall.
    } // End the valid-update validation branch.

    ret = getnice(1); // Read init's nice value again to verify that the update actually took effect.
    if (ret == 10) { // Check whether the stored nice value now matches the value we requested.
        printf("[PASS] getnice after setnice\n"); // Report success when the updated nice value is visible through getnice().
    } else { // Handle the case where the nice value did not change as expected.
        printf("[FAIL] getnice after setnice (got %d)\n", ret); // Print the unexpected post-update value for debugging.
    } // End the post-update validation branch.

    ret = setnice(1, original_nice); // Restore init's original nice value so the test leaves the system in a clean state.
    if (ret == 0) { // Check whether the restore operation also succeeded.
        printf("[PASS] setnice restore original value\n"); // Report success when the original value was restored correctly.
    } else { // Handle the case where restoring the original value failed.
        printf("[FAIL] setnice restore original value (got %d)\n", ret); // Print the failure code returned by the restore attempt.
    } // End the restore validation branch.

    ret = setnice(999, 10); // Try updating a pid that should not exist so we can verify the invalid-pid failure path.
    if (ret == -1) { // Confirm that the syscall rejects requests for non-existent processes.
        printf("[PASS] setnice invalid pid\n"); // Report success for the invalid-pid test case.
    } else { // Handle the case where the syscall incorrectly accepted a non-existent pid.
        printf("[FAIL] setnice invalid pid (got %d)\n", ret); // Print the incorrect return value for debugging.
    } // End the invalid-pid validation branch.

    ret = setnice(1, 40); // Try updating with a nice value that is above the allowed range.
    if (ret == -1) { // Confirm that the syscall rejects values larger than the allowed maximum.
        printf("[PASS] setnice above max range\n"); // Report success for the too-large range check.
    } else { // Handle the case where the syscall incorrectly accepted an out-of-range large value.
        printf("[FAIL] setnice above max range (got %d)\n", ret); // Print the incorrect return value for debugging.
    } // End the above-range validation branch.

    ret = setnice(1, -1); // Try updating with a negative nice value so we can verify the lower-bound range check.
    if (ret == -1) { // Confirm that the syscall rejects values smaller than the allowed minimum.
        printf("[PASS] setnice below min range\n"); // Report success for the negative range check.
    } else { // Handle the case where the syscall incorrectly accepted a negative nice value.
        printf("[FAIL] setnice below min range (got %d)\n", ret); // Print the incorrect return value for debugging.
    } // End the below-range validation branch.

    printf("=== done ===\n"); // Print a footer so it is clear that the test finished running.
    exit(0); // Terminate the test program successfully.
} // End of the setnice test program.
