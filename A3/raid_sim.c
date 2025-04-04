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
#include <getopt.h>
#include "raid.h"

/*
 * This file implements the main RAID simulator program.
 * It provides both a command-line interface for interacting with the RAID
 * system and a transaction file interface.
 * It initializes the disk controllers using command line arguments as
 * configuration parameters, and parses the RAID commands and calls the
 * controller functions that implement these commands.
 */

// Maximum length of a buffer to hold a RAID command
#define MAX_CMD_LENGTH 256

// Global variables for RAID configuration
int num_disks = DEFAULT_NUM_DISKS;
int block_size = DEFAULT_BLOCK_SIZE;
int disk_size = DEFAULT_DISK_SIZE;

/* Print usage information for the program, which has name prog_name.
 */
static void print_usage(char *prog_name)
{
    fprintf(stderr, "Usage: %s [-n num_disks] [-b block_size] [-d disk_size] [-t file_name]\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n num_disks   Number of data disks (default: %d)\n", DEFAULT_NUM_DISKS);
    fprintf(stderr, "  -b block_size  Size of each block in bytes (default: %d)\n", DEFAULT_BLOCK_SIZE);
    fprintf(stderr, "  -d disk_size   Size of each disk in bytes (default: %d)\n", DEFAULT_DISK_SIZE);
    fprintf(stderr, "  -t file_name   Use the transaction file named file_name instead of stdin for input\n");
    exit(1);
}

/* Print the preamble when the shell interface is used
 */
static void print_command_shell_header()
{
    printf("RAID 4 Simulator Shell\n");
    printf("System configuration:\n");
    printf("  Number of data disks: %d\n", num_disks);
    printf("  Block size: %d bytes\n", block_size);
    printf("  Disk size: %d bytes\n", disk_size);

    printf("Available commands:\n");
    printf("  wb <block_num> <file from local> \n");
    printf("  rb <block_num> \n");
    printf("  kill <disk_num> \n");
    printf("  exit \n");
}

/* Print the block block_num from the RAID system to stdout.
 *
 * Returns 0 on success and -1 on error.
 */
static int print_block(int block_num)
{
    char *block = malloc(block_size);
    if (!block)
    {
        perror("Failed to allocate memory for block");
        return -1;
    }

    if (read_block(block_num, block) == NULL)
    {
        fprintf(stderr, "Failed to read block from RAID");
        free(block);
        return -1;
    }

    if (fwrite(block, 1, block_size, stdout) != (size_t)block_size)
    {
        fprintf(stderr, "Failed to write block to stdout");
        free(block);
        return -1;
    }

    fprintf(stderr, "Block %d printed\n", block_num);
    free(block);
    return 0;
}

/* Parse a command line into a command structure.
 *
 * Returns a pointer to the parsed command structure, or NULL on error.
 * The memory returned must be freed by the called using cleanup_command.
 *
 * This function modifies the input line by replacing spaces with nulls.
 */

// each command line (either through input or transaction file) has 3 tokens
command_t *parse_command(char *line)
{
    // creates a command struct
    command_t *cmd = malloc(sizeof(command_t));
    if (!cmd)
    {
        perror("Failed to allocate command structure");
        return NULL;
    }
    // sets the arg1 and arg2 fields in the struct to null
    cmd->arg1 = NULL;
    cmd->arg2 = NULL;

    // assign the first element
    cmd->cmd = strtok(line, " ");
    if (!cmd->cmd)
    {
        free(cmd);
        return NULL;
    }
    cmd->arg1 = strtok(NULL, " ");
    cmd->arg2 = strtok(NULL, " ");

    return cmd;
}

/* Copy a block from a local file named filename to the RAID system at
 * block block_num.
 *
 * Returns 0 on success and -1 on error.
 */
static int copy_block_to_raid(int block_num, char *filename)
{
    // Open the file
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        char msg[MAX_NAME];
        snprintf(msg, sizeof(msg), "Error opening %s", filename);
        perror(msg);
        return -1;
    }

    // Allocate buffer dynamically
    // the buffer is just a fancy word for a variable that stores data temporarily
    char buffer[block_size];

    // Read the first block_size bytes from the file
    // we read only the block_size byte, since we want to store the file info in the block
    // fp is the file pointer
    // 1 is the size of each element (so we read block_size elements)
    // the return value of fread is actually just the # of elements read
    // but in this case the # of elements and the # of bytes are the same, since size is 1
    size_t bytes_read = fread(buffer, 1, block_size, fp);
    // if we don't manage to fill up the block, we return an error dependent on what happens
    if (bytes_read < (size_t)block_size)
    {
        if (ferror(fp))
        {
            fprintf(stderr, "Error reading file");
        }
        else if (feof(fp))
        {
            fprintf(stderr, "Error: File is smaller than block size\n");
        }
        fclose(fp);
        return -1;
    }

    // call write block from controller.c to write the data to the block
    if (write_block(block_num, buffer) != 0)
    // error checking, if the return value of write_block is not equal to 0 (success)
    {
        fprintf(stderr, "Failed to write block to RAID");
        // close the file
        fclose(fp);
        return -1;
    }
    // else, if write_block succeeded
    fclose(fp);
    fprintf(stderr, "Block %d written to RAID\n", block_num);
    return 0;
}

/* Execute a parsed command cmd.
 *
 * This function implements the RAID shell commands:
 * - exit: Exit the program
 * - wb: Write a block from a local file to the RAID system
 * - rb: Read a block from the RAID system to stdout
 * - kill: Kills one of the disk processes
 *
 * Returns 0 on success and -1 on error.
 */
int execute_command(command_t *cmd)
{
    // should be redundant? since cmd is never created if this field doesnt exist
    if (!cmd->cmd)
    {
        return -1;
    }

    // if the command is exit, exit the program
    if (strcmp(cmd->cmd, "exit") == 0)
    {
        checkpoint_and_wait();
        exit(0);
    }

    // if the command is wb <block_num> <filename>, reading the first
    // block num bytes from a regular file named filename, and write
    // that data to block number block_num
    else if (strcmp(cmd->cmd, "wb") == 0)
    {
        if (cmd->arg2 == NULL || cmd->arg1 == NULL)
        {
            printf("Usage: wb <block_num> <file from local>\n");
            return -1;
        }
        printf("wb\n");
        copy_block_to_raid(atoi(cmd->arg1), cmd->arg2);
        return 0;
    }

    // if the command is rb <block_num>, we read the block number block_num
    // from the RAID system and print it to stdout. Use ASCII chars to make testing easier.
    else if (strcmp(cmd->cmd, "rb") == 0)
    {
        if (cmd->arg1 == NULL)
        {
            printf("Usage: rb <block_num>\n");
            return -1;
        }
        print_block(atoi(cmd->arg1));
        return 0;
    }

    // if the command is kill <disk_num> we simulate disk failure by sending SIGNIT signal
    // to the disk process with id disk_num
    else if (strcmp(cmd->cmd, "kill") == 0)
    {
        if (cmd->arg1 == NULL)
        {
            printf("Usage: kill <disk_num>\n");
            return -1;
        }
        simulate_disk_failure(atoi(cmd->arg1));
        return 0;
    }
    // if the command isnt recognized (typo, for example)
    else
    {
        printf("Unknown command: %s\n", cmd->cmd);
        return -1;
    }
}

/* Free the memory allocated for a command structure cmd.
 */
static void cleanup_command(command_t *cmd)
{
    if (cmd)
    {
        free(cmd);
    }
}

/* The main entry point for the RAID simulation program.
 */
int main(int argc, char **argv)
{
    // by default commands are read from stdin unless the -t option is provided
    FILE *tf = stdin;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:b:d:t:h")) != -1)
    {
        switch (opt)
        {
        case 'n':
            num_disks = atoi(optarg);
            if (num_disks <= 0)
            {
                fprintf(stderr, "Error: Number of disks must be positive\n");
                print_usage(argv[0]);
            }
            break;
        case 'b':
            block_size = atoi(optarg);
            if (block_size <= 0)
            {
                fprintf(stderr, "Error: Block size must be positive\n");
                print_usage(argv[0]);
            }
            break;
        case 'd':
            disk_size = atoi(optarg);
            if (disk_size <= 0)
            {
                fprintf(stderr, "Error: Disk size must be positive\n");
                print_usage(argv[0]);
            }
            break;
        case 't':
            // we want to use the transaction file instead of the shell
            tf = fopen(optarg, "r");
            if (!tf)
            {
                perror("Failed to open transactions file");
                print_usage(argv[0]);
            }
            break;
        case 'h':
        default:
            print_usage(argv[0]);
        }
    }

    // Initialize disk processes and parity disk process
    if (init_all_controllers(num_disks + 1) == -1)
    {
        fprintf(stderr, "Failed to initialize disk processes\n");
        return -1;
    }

    // if we didn't use the t flag, we are in the interactive shell mode
    if (tf == stdin)
    {
        print_command_shell_header();
    }

    while (1)
    {
        if (tf == stdin)
        {
            printf("raid> ");
        }

        char line[MAX_CMD_LENGTH];
        // read a line from tf (or standard input, if we didn't have the t flag)
        if (!fgets(line, sizeof(line), tf))
        {
            // if read fails
            if (!feof(tf))
            {
                // if read fails AND we are not at the end of file
                fprintf(stderr, "Error reading command");
            }
            // exit the program
            break;
        }

        // Remove trailing newline
        char *newline = strchr(line, '\n');
        if (newline)
        {
            *newline = '\0';
        }

        // Parse and execute command
        command_t *cmd = parse_command(line);
        if (!cmd)
        {
            fprintf(stderr, "Failed to parse command\n");
            continue;
        }
        if (execute_command(cmd) == -1)
        {
            fprintf(stderr, "Command execution failed\n");
        }
        cleanup_command(cmd);
    }
    checkpoint_and_wait();
    return 0;
}
