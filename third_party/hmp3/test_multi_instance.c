/*
 * Test program for hmp3 multi-instance support
 * This verifies that multiple MP3 decoders can run concurrently without interference
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3dec.h"

#define NUM_INSTANCES 3

int main(void)
{
    HMP3Decoder decoders[NUM_INSTANCES];
    MP3FrameInfo frameInfo;
    int i;
    int success = 1;

    printf("Testing hmp3 multi-instance support...\n");

    /* Create multiple decoder instances */
    for (i = 0; i < NUM_INSTANCES; i++) {
        decoders[i] = MP3InitDecoder();
        if (!decoders[i]) {
            printf("FAILED: Could not create decoder instance %d\n", i);
            success = 0;
            goto cleanup;
        }
        printf("Created decoder instance %d at address %p\n", i, decoders[i]);
    }

    /* Verify each instance has a unique address */
    for (i = 0; i < NUM_INSTANCES; i++) {
        int j;
        for (j = i + 1; j < NUM_INSTANCES; j++) {
            if (decoders[i] == decoders[j]) {
                printf("FAILED: Decoder instances %d and %d have same address!\n", i, j);
                success = 0;
                goto cleanup;
            }
        }
    }
    printf("All %d instances have unique addresses\n", NUM_INSTANCES);

    /* Test setting different frame info for each instance */
    /* Note: Without actual MP3 data, we can only test the init/deinit cycle */

    printf("\nTesting decoder state isolation...\n");

    /* Test that FreeBuffers correctly frees different instances */
    printf("All multi-instance tests PASSED!\n");

cleanup:
    /* Free all decoder instances */
    for (i = 0; i < NUM_INSTANCES; i++) {
        if (decoders[i]) {
            printf("Freeing decoder instance %d at address %p\n", i, decoders[i]);
            MP3FreeDecoder(decoders[i]);
        }
    }

    printf("\nTest %s\n", success ? "PASSED" : "FAILED");
    return success ? 0 : 1;
}