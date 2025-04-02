/* This code is provided solely for the personal and private use of students
 * taking the CSC209H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Karen Reid, Paul He, Philip Kukulak
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2025 Karen Reid
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raid.h"

int debug = 1; // Set to 1 to enable debug output, 0 to disable

/*
 * Main function for the disk simulation process, which runs in a child process
 * created by the RAID controller.
 *
 * id is the disk number or index into the controllers table,
 * to_parent is the pipe descriptor for writing to the parent,
 * from_parent is the pipe descriptor for reading from the parent.
 *
 * Returns 0 on success and 1 on failure.
 */
int start_disk(int id, int to_parent, int from_parent)
{
    int status = 0;

    // Allocate memory for disk data
    char disk_data[disk_size];
    memset(disk_data, 0, disk_size);

    if (debug)
    {
        printf("[%d] Waiting for command\n", id);
    }

    // Main command loop to handle requests from the parent.
    // This is an infinite loop that is only terminated when
    // an exit command is received.
    while (1)
    {
        disk_command_t cmd;

        // Read command through the pipe from the parent
        ssize_t cmd_return = read(from_parent, &cmd, sizeof(cmd));

        // if read fails
        if (cmd_return != sizeof(cmd))
        {
            fprintf(stderr, "[%d] Error reading command\n", id);
            status = 1;
            break;
        }

        // The type of command received from the parent
        // determines which action is taken next.
        switch (cmd)
        {
        case CMD_READ:
            // TODO: Handle READs

            // We first need to read the stripe number from the parent process
            int stripe_num;
            ssize_t bytes_read = read(from_parent, &stripe_num, sizeof(stripe_num));
            if (bytes_read != sizeof(stripe_num))
            {
                fprintf(stderr, "[%d] Error reading stripe number\n", id);
                status = 1;
                break;
            }

            // debug message
            if (debug)
            {
                printf("[%d] cmd: %d. Block num: %d, size: %d\n", id, CMD_READ, stripe_num, block_size);
                printf("[%d] Writing data to parent. Block num: %d\n", id, stripe_num);
            }

            // Then, we return the block data, by indexing disk_data at the stripe location
            ssize_t bytes_written = write(to_parent, &disk_data[stripe_num * block_size], block_size);
            if (bytes_written != block_size)
            {
                fprintf(stderr, "[%d] Error writing block data to parent\n", id);
                status = 1;
                break;
            }

            break;

        case CMD_WRITE:
            // TODO: Handle WRITEs

            // We first need to read the stripe number from the parent process
            int stripe_num;
            ssize_t bytes_read = read(from_parent, &stripe_num, sizeof(stripe_num));
            if (bytes_read != sizeof(stripe_num))
            {
                fprintf(stderr, "[%d] Error reading stripe number\n", id);
                status = 1;
                break;
            }

            // Debug message
            if (debug)
            {
                printf("[%d] Read block from pipe. Block num: %d, size: %d\n", id, stripe_num, block_size);
                printf("[%d] Writing data to disk. Block num: %d, size: %d\n", id, stripe_num, block_size);
            }

            // We then need to read the actual block data to be written;
            // calculate where in the disk_data array this block should be stored
            char *block_storage_location = &disk_data[stripe_num * block_size];

            // read the block data from the parent process through the pipe
            // and store it into the calculated location in disk_data
            ssize_t bytes_read_from_parent = read(from_parent, block_storage_location, block_size);

            // erorr checking to see if we received the expected amount of data
            if (bytes_read_from_parent != block_size)
            {
                fprintf(stderr, "[%d] Error reading block data from parent\n", id);
                status = 1;
                break;
            }
            break;

        case CMD_EXIT:
            // TODO: Handle EXITs

            // before we exit, we need to first checkpoint the disk
            if (checkpoint_disk(disk_data, id) != 0)
            {
                fprintf(stderr, "[%d] Error when checkpointing disk\n", id);
                status = 1;
            }

            if (debug)
            {
                printf("[%d] Checkpointed disk successfully\n", id);
            }

            return status;
            break;

        default:
            fprintf(stderr, "Error: Unknown command %d received\n", cmd);
            status = 1;
            break;
        }
    }

    // Checkpoint and cleanup before exiting
    // TODO

    exit(status);
}

/* Save the disk's data, pointed to by disk_data, to a file named id.
 *
 * Returns 0 on success, and -1 on failure.
 */
static int checkpoint_disk(char *disk_data, int id)
{
    if (!disk_data)
    {
        fprintf(stderr, "Error: Invalid parameters for checkpoint\n");
        return -1;
    }

    // Create a file name for this disk
    char disk_name[MAX_NAME];
    if (snprintf(disk_name, sizeof(disk_name), "disk_%d.dat", id) >= (int)sizeof(disk_name))
    {
        fprintf(stderr, "Error: Disk name too long for disk %d\n", id);
        return 1;
    }

    FILE *fp = fopen(disk_name, "wb");
    if (!fp)
    {
        perror("Failed to create checkpoint file");
        return -1;
    }

    size_t bytes_written = fwrite(disk_data, 1, disk_size, fp);
    if (bytes_written != (size_t)disk_size)
    {
        if (ferror(fp))
        {
            fprintf(stderr, "Failed to write checkpoint data");
        }
        else
        {
            fprintf(stderr, "Error: Incomplete write during checkpoint\n");
        }
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0)
    {
        perror("Failed to close checkpoint file");
        return -1;
    }

    return 0;
}
