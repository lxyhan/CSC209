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
static disk_controller_t* controllers;

/* Ignoring SIGPIPE allows us to check write calls for error rather than
 * terminating the whole system.
 */
static void ignore_sigpipe() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("Failed to set up SIGPIPE handler");
    }
}

/* Initialize the num-th disk controller, creating pipes to communicate
 * and creating a child process to handle disk requests.
 *
 * Returns 0 on success and -1 on failure.
 */
static int init_disk(int num) {
    ignore_sigpipe();

    // TODO: Complete this function
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
int restart_disk(int num) {
    ignore_sigpipe();

    // TODO: Complete this function
    return 0;
}

/* Initialize all disk controllers by initializing the controllers
 * array and calling init_disk for each disk.
 *
 * total_disks is the number of data disks + 1 for the parity disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int init_all_controllers(int total_disks) {
    // TODO: Complete this function
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
int read_block_from_disk(int block_num, char* data, int parity_flag) {
    if (!data) {
        fprintf(stderr, "Error: Invalid data buffer\n");
        return -1;
    }

    // Identify the stripe to read from
    int disk_num;
    if (parity_flag == 1) {
        disk_num = num_disks;
    } else {
        disk_num = block_num % num_disks;
    }

    disk_command_t cmd = CMD_READ;

    // Each disk has a linear array of blocks, so the block number on an
    // individual disk is the same as the stripe number
    block_num = block_num / num_disks;

    // Write the command and the block number to the disk process
    // Then read the block from the disk process
    // TODO: Complete this function

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
int write_block_to_disk(int block_num, char *data, int parity_flag) {
    // TODO: complete this function (see read_block_from disk for some tips)
    return 0;
}

/* Write the memory pointed to by data to the block at block_num on the
 * RAID system, handling parity updates.
 * If block_num is invalid (outside the range 0 to disk_size/block_size)
 * then return NULL.
 *
 * Returns 0 on success and the disk number we tried to read from on failure.
 */
int write_block(int block_num, char *data) {
    // TODO: Complete this function (see handout for more details)
    return 0;
}

/* Read the block at block_num from the RAID system into
 * the memory pointed to by data.
 * If block_num is invalid (outside the range 0 to disk_size/block_size)
 * then return NULL.
 *
 * Returns a pointer to the data buffer on success and NULL on failure.
 */
char *read_block(int block_num, char *data) {
    // TODO: Complete this function
}

/* Send exit command to all disk processes.
 *
 * Returns when all disk processes have terminated.
 */
void checkpoint_and_wait() {
    for (int i = 0; i < num_disks + 1; i++) {
        disk_command_t cmd = CMD_EXIT;
        ssize_t bytes_written = write(controllers[i].to_disk[1], &cmd, sizeof(cmd));
        if (bytes_written != sizeof(cmd)) {
            fprintf(stderr, "Warning: Failed to send exit command to disk %d\n", i);
        }
    }
    // wait for all disks to exit
    // we aren't going to do anything with the exit value
    for (int i = 0; i < num_disks + 1; i++) {
        wait(NULL);
    }
}


/* Simulate the failure of a disk by sending the SIGINT signal to the
 * process with id disk_num.
 */
void simulate_disk_failure(int disk_num) {
    if(debug) {
        printf("Simulate: killing disk %d\n", disk_num);
    }
    kill(controllers[disk_num].pid, SIGINT);
}

/* Restore the disk process after it has been killed.
 * If some aspect of restoring the disk process fails, 
 * then you can consider it a catastropic failure and 
 * exit the program.
 */
void restore_disk_process(int disk_num) {
    // TODO: Complete this function
}
