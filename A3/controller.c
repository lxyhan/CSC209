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
#include <signal.h>
#include <sys/wait.h>
#include "raid.h"

/*
 * This file implements the RAID controller that manages communication between
 * the main RAID simulator and the individual disk processes. It uses pipes
 * for inter-process communication (IPC) and fork to create child processes
 * for each disk.
 */

// Global array to store information about each disk's communication pipes.
static disk_controller_t *controllers;

/* Ignoring SIGPIPE allows us to check write calls for error rather than
 * terminating the whole system.
 */
static void ignore_sigpipe()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        perror("Failed to set up SIGPIPE handler");
    }
}

/* Initialize the num-th disk controller, creating pipes to communicate
 * and creating a child process to handle disk requests.
 *
 * Returns 0 on success and -1 on failure.
 */
static int init_disk(int num)
{
    ignore_sigpipe();

    // Create pipes for communication
    if (pipe(controllers[num].to_disk) < 0)
    {
        perror("Failed to create to_disk pipe");
        return -1;
    }

    if (pipe(controllers[num].from_disk) < 0)
    {
        perror("Failed to create from_disk pipe");
        close(controllers[num].to_disk[0]);
        close(controllers[num].to_disk[1]);
        return -1;
    }

    pid_t pid = fork();

    // if its not the child process
    if (pid < 0)
    {
        // Error handling
        perror("Failed to fork");
        close(controllers[num].to_disk[0]);
        close(controllers[num].to_disk[1]);
        close(controllers[num].from_disk[0]);
        close(controllers[num].from_disk[1]);
        return -1;
    }

    // if the child process was sucessfully created
    if (pid == 0)
    {
        // Close unused pipe ends (write end of child process)
        close(controllers[num].to_disk[1]); // Close write end of to_disk
        // (read end of parent process)
        close(controllers[num].from_disk[0]); // Close read end of from_disk

        // Run the disk simulation
        // The child never returns from this function
        start_disk(num, controllers[num].from_disk[1], controllers[num].to_disk[0]);
        exit(1); // this code shouldn't run right?
    }

    controllers[num].pid = pid;

    close(controllers[num].to_disk[0]);   // Close read end of to_disk
    close(controllers[num].from_disk[1]); // Close write end of from_disk

    return 0;
}

/* Restart the num-th disk, whose process is assumed to have already been killed.
 *
 * This function is very similar to init_disk.
 * However, since the other processes have all been started,
 * it needs to close a larger set of open pipe descriptors
 * inherited from the parent.
 *
 * Returns 0 on success and -1 on failure.
 */
int restart_disk(int num)
{
    ignore_sigpipe();

    // TODO: Complete this function

    // need to close these two
    close(controllers[num].to_disk[0]);
    close(controllers[num].from_disk[1]);

    // these are already closed
    close(controllers[num].to_disk[1]);
    close(controllers[num].from_disk[0]);

    // call initialize disk again
    return init_disk(num);
}

/* Initialize all disk controllers by initializing the controllers
 * array and calling init_disk for each disk.
 *
 * total_disks is the number of data disks + 1 for the parity disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int init_all_controllers(int total_disks)
{
    // TODO: Complete this function

    // Allocate memory for controllers on the heap
    controllers = malloc(total_disks * sizeof(disk_controller_t));
    if (!controllers)
    {
        perror("Failed to allocate memory for controllers");
        return -1;
    }

    for (int i = 0; i < total_disks; i++)
    {
        // if init disk returns an error
        if (init_disk(i) != 0)
        {
            // Cleanup already initialized disks
            for (int j = 0; j < i; j++)
            {
                // close the ends that are actually being used
                close(controllers[j].to_disk[1]);   // Write end of child
                close(controllers[j].from_disk[0]); // Read end of parent
                // the other 2 ends are already closed by the child process in init_disk
                kill(controllers[j].pid, SIGTERM);
            }
            free(controllers);
            controllers = NULL;
            return -1;
        }
    }

    return 0;
}

/* Read the block of data at block_num from the appropriate disk.
 * The block is stored to the memory pointed to by data.
 *
 * If parity_flag == 1, read from parity disk.
 * If parity_flag == 0, read from data disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int read_block_from_disk(int block_num, char *data, int parity_flag)
{
    if (!data)
    {
        fprintf(stderr, "Error: Invalid data buffer\n");
        return -1;
    }

    // Identify the stripe to read from
    int disk_num;
    if (parity_flag == 1)
    {
        disk_num = num_disks;
    }
    else
    {
        disk_num = block_num % num_disks;
    }

    // this is a constant
    disk_command_t cmd = CMD_READ;

    // Each disk has a linear array of blocks, so the block number on an
    // individual disk is the same as the stripe number
    int stripe_num = block_num / num_disks;

    // Write the command and the block number to the disk process
    // Then read the block from the disk process

    // Send the command to the disk
    ssize_t cmd_written = write(controllers[disk_num].to_disk[1], &cmd, sizeof(cmd));
    if (cmd_written != sizeof(cmd))
    {
        // Handle disk failure
        fprintf(stderr, "Failed to send READ command to disk %d\n", disk_num);
        restore_disk_process(disk_num);
        return -1;
    }

    // Then send the stripe number to the disk
    ssize_t block_written = write(controllers[disk_num].to_disk[1], &stripe_num, sizeof(stripe_num));
    if (block_written != sizeof(stripe_num))
    {
        // Handle disk failure
        fprintf(stderr, "Failed to send block number to disk %d\n", disk_num);
        restore_disk_process(disk_num);
        return -1;
    }

    // Read the block data returned by the disk
    ssize_t bytes_read = read(controllers[disk_num].from_disk[0], data, block_size);
    if (bytes_read != block_size)
    {
        // Handle disk failure
        fprintf(stderr, "Failed to read data from disk %d\n", disk_num);
        restore_disk_process(disk_num);
        return -1;
    }

    return 0;
}

/* Write a block of data to the block at block_num on the appropriate disk.
 * The block is stored at the memory pointed to by data.
 *
 * If parity_flag == 1, write to parity disk.
 * If parity_flag == 0, write to data disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int write_block_to_disk(int block_num, char *data, int parity_flag)
{
    if (!data)
    {
        fprintf(stderr, "Error: Invalid data buffer for writing data");
    }

    int disk_num;
    if (parity_flag == 1)
    {
        // assign it to the last disk, which isthe parity disk
        disk_num = num_disks;
    }
    else
    {
        disk_num = block_num % num_disks;
    }

    disk_command_t cmd = CMD_WRITE;

    int stripe_num = block_num / num_disks;

    // send the command to the disk (READ or WRITE)
    // to_disk[1] is write, to_disk[0] is read
    // controllers is an array of all the disk processes, disk_num is the specific
    // disk that we want to write to
    ssize_t cmd_written = write(controllers[disk_num].to_disk[1], &cmd, sizeof(cmd));
    if (cmd_written != sizeof(cmd))
    {
        // handle possible disk failure
        restore_disk_process(disk_num);
        return -1;
    }

    // 2. Send the block number (stripe_num) to the disk
    ssize_t block_written = write(controllers[disk_num].to_disk[1], &stripe_num, sizeof(stripe_num));
    if (block_written != sizeof(stripe_num))
    {
        // Handle possible disk failure
        restore_disk_process(disk_num);
        return -1;
    }

    // 3. Send the actual block data
    ssize_t data_written = write(controllers[disk_num].to_disk[1], data, block_size);
    if (data_written != block_size)
    {
        // Handle possible disk falure
        restore_disk_process(disk_num);
        return -1;
    }

    return 0;
}

/* Write the memory pointed to by data to the block at block_num on the
 * RAID system, handling parity updates.
 * If block_num is invalid (outside the range 0 to disk_size/block_size)
 * then return NULL.
 *
 * Returns 0 on success and the disk number we tried to read from on failure.
 */
int write_block(int block_num, char *data)
{
    // here, data is the buffer and block_num is the block number we want to write to
    // we first need to call write_block_to_disk with parity flag 0 to write it to data storage
    // there is no need to process data since it is already the same number of bytes as block_num that
    // we set at the start of the simulation
    // we do however need to calculate the parity block
    // we then need to call write_block_to_disk with parity flag 1 to write it to the parity disk

    // Check if block_num is in range
    if (block_num < 0 || block_num >= (disk_size / block_size) * num_disks)
    {
        fprintf(stderr, "Error: Invalid block number %d\n", block_num);
        return -1;
    }

    int stripe_num = block_num / num_disks;

    // buffers for old_data and the parity
    char old_data[block_size];
    char parity_data[block_size];

    // get the data from the disk to replace
    if (read_block_from_disk(block_num, old_data, 0) != 0)
    {
        // Handle error
        return block_num % num_disks;
    }

    // get the current parity disk
    if (read_block_from_disk(stripe_num, parity_data, 1) != 0)
    {
        // Handle error
        return num_disks; // Parity disk
    }

    // to update the parity:
    // 1. we first xor the current parity with the old data disk contents, to cancel it out
    // 2. we then xor the intermediate parity with the new data disk contents
    for (int i = 0; i < block_size; i++)
    {
        parity_data[i] = parity_data[i] ^ old_data[i] ^ data[i];
    }

    // write the new data to the data disk
    if (write_block_to_disk(block_num, data, 0) != 0)
    {
        return block_num % num_disks;
    }

    // update the parity disk
    if (write_block_to_disk(stripe_num, parity_data, 1) != 0)
    {
        return num_disks;
    }

    return 0;
}

/* Read the block at block_num from the RAID system into
 * the memory pointed to by data.
 * If block_num is invalid (outside the range 0 to disk_size/block_size * num_disks)
 * then return NULL.
 *
 * Returns a pointer to the data buffer on success and NULL on failure.
 */
char *read_block(int block_num, char *data)
{
    // TODO: Complete this function

    // Check if block_num is in range
    if (block_num < 0 || block_num >= (disk_size / block_size) * num_disks)
    {
        fprintf(stderr, "Error: Invalid block number %d\n", block_num);
        return NULL;
    }

    if (read_block_from_disk(block_num, data, 0) != 0)
    {
        // failed to read data from disk
        fprintf(stderr, "Failed to read data from disk");
        return NULL;
    }

    return data;
}

/* Send exit command to all disk processes.
 *
 * Returns when all disk processes have terminated.
 */
void checkpoint_and_wait()
{
    for (int i = 0; i < num_disks + 1; i++)
    {
        disk_command_t cmd = CMD_EXIT;
        ssize_t bytes_written = write(controllers[i].to_disk[1], &cmd, sizeof(cmd));
        if (bytes_written != sizeof(cmd))
        {
            fprintf(stderr, "Warning: Failed to send exit command to disk %d\n", i);
        }
    }
    // wait for all disks to exit
    // we aren't going to do anything with the exit value
    for (int i = 0; i < num_disks + 1; i++)
    {
        wait(NULL);
    }
}

/* Simulate the failure of a disk by sending the SIGINT signal to the
 * process with id disk_num.
 */
void simulate_disk_failure(int disk_num)
{
    if (debug)
    {
        printf("Simulate: killing disk %d\n", disk_num);
    }
    kill(controllers[disk_num].pid, SIGINT);
}

/* Restore the disk process after it has been killed.
 * If some aspect of restoring the disk process fails,
 * then you can consider it a catastropic failure and
 * exit the program.
 */
void restore_disk_process(int disk_num)
{
    // TODO: Complete this function

    // first we restart the disk
    if (restart_disk(disk_num) != 0)
    {
        fprintf(stderr, "disk %d failed to restart\n", disk_num);
        exit(1);
    }

    // then we recalculate the lost data

    // number of stripes
    int blocks_per_disk = disk_size / block_size;

    // buffer for the lost data
    char lost_disk_buffer[block_size];
    // this stores the parity block
    char parity_block[block_size];
    // this stores the xor of other survivng data disks
    char other_blocks[block_size];

    for (int stripe = 0; stripe < blocks_per_disk; stripe++)
    {
        memset(lost_disk_buffer, 0, block_size);

        if (read_block_from_disk(stripe, parity_block, 1) != 0)
        {
            fprintf(stderr, "Failed to read parity disk at block %d\n", stripe);
            exit(1);
        }

        for (int i = 0; i < num_disks; i++)
        {
            if (i != disk_num)
            {
                // this calculates the exact block number, as distributed across the disks
                int block_num = i + (stripe * num_disks);

                // error check for if read fails
                if (read_block_from_disk(block_num, other_blocks, 0) != 0)
                {
                    fprintf(stderr, "Failed to read block %d\n", block_num);
                    exit(1);
                }

                for (int j = 0; j < block_size; j++)
                {
                    lost_disk_buffer[j] ^= other_blocks[j];
                }
            }
        }

        for (int j = 0; j < block_size; j++)
        {
            lost_disk_buffer[j] ^= parity_block[j];
        }

        int block_num = disk_num + (stripe * num_disks);

        if (write_block_to_disk(block_num, lost_disk_buffer, 0) != 0)
        {
            fprintf(stderr, "Failed to write recovered data for block %d\n", block_num);
            exit(1);
        }
    }

    // finally we write it to the newly re-initalized disk
}
