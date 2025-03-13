#include <stdio.h>
#include <stdlib.h>
#include "gc.h"

// global variable to be head of allocated pieces
struct mem_chunk *memory_list_head = NULL;
FILE *logfile = NULL;

void init_gc()
{
    fprintf(stderr, "Initializing garbage collector...\n");

    // Check if already initialized
    if (logfile != NULL)
    {
        fprintf(stderr, "GC already initialized, logfile exists\n");
        return;
    }

    logfile = fopen("gc.log", "w");
    if (logfile == NULL)
    {
        fprintf(stderr, "ERROR: Could not open log file: ");
        perror("");
        // Don't exit - attempt to continue without logging
    }
    else
    {
        fprintf(stderr, "Successfully opened gc.log\n");
        fprintf(logfile, "Garbage collector initialized\n");
        fflush(logfile); // Force write to disk
    }
}
// global variable for debugging
int debug = 0;

void *gc_malloc(int nbytes)
{
    void *ptr = malloc(nbytes);
    if (ptr == NULL)
    {
        perror("malloc failed in gc_malloc");
        return NULL;
    }

    struct mem_chunk *new_node = malloc(sizeof(struct mem_chunk));
    if (new_node == NULL)
    {
        perror("malloc failed in gc_malloc for memory tracking");
        free(ptr); // Free the first allocation since we can't track it
        return NULL;
    }

    new_node->address = ptr;
    new_node->in_use = 1;
    new_node->next = memory_list_head;
    memory_list_head = new_node;

    return ptr;
}

/* Executes the garbage collector.
 * obj is the root of the data structure to be traversed
 * mark_obj is a function that will traverse the data structure rooted at obj
 *
 * The function will also write to the LOGFILE the results messages with the
 * following format strings:
 * "Mark_and_sweep running\n"
 * "Chunks freed this pass: %d\n"
 * "Chunks still allocated: %d\n"
 */

void mark_and_sweep(void *obj, void (*mark_obj)(void *))
{
    // Check if logfile is NULL and reopen if needed
    if (logfile == NULL)
    {
        fprintf(stderr, "Error: logfile is NULL in mark_and_sweep\n");
        logfile = fopen("gc.log", "a");
        if (logfile == NULL)
        {
            fprintf(stderr, "Failed to reopen logfile, continuing without logging\n");
            // Continue execution but skip logging
        }
    }

    // Only try to write to logfile if it's not NULL
    if (logfile != NULL)
    {
        fprintf(logfile, "Mark_and_sweep running\n");
    }

    int chunks_freed = 0;
    int chunks_allocated = 0;

    // 1) RESET: Mark all nodes as not in use
    struct mem_chunk *curr = memory_list_head;
    while (curr != NULL)
    {
        curr->in_use = NOT_USED;
        curr = curr->next;
    }

    // 2) MARK: Traverse the data structure and mark reachable nodes
    if (obj != NULL)
    {
        mark_obj(obj);
    }

    // 3) SWEEP: Free all unmarked nodes
    struct mem_chunk **pp = &memory_list_head;
    while (*pp != NULL)
    {
        if ((*pp)->in_use == NOT_USED)
        {
            // This node is unreachable, free it
            struct mem_chunk *to_free = *pp;
            *pp = to_free->next; // Remove from list

            free(to_free->address); // Free the allocated memory
            free(to_free);          // Free the chunk node
            chunks_freed++;
        }
        else
        {
            // This node is still in use
            pp = &((*pp)->next);
            chunks_allocated++;
        }
    }

    // Only try to write to logfile if it's not NULL
    if (logfile != NULL)
    {
        fprintf(logfile, "Chunks freed this pass: %d\n", chunks_freed);
        fprintf(logfile, "Chunks still allocated: %d\n", chunks_allocated);
    }
    else
    {
        fprintf(stderr, "Chunks freed: %d, Chunks still allocated: %d\n",
                chunks_freed, chunks_allocated);
    }
}

/*
 Mark the chunk of memory pointed to by vptr as in use.
 Return codes:
   0 if successful
   1 if chunk already marked as in use
   2 if chunk is not found in memory list

   Here is a print statement to print an error message:
   fprintf(stderr, "ERROR: mark_one address not in memory list\n");
 */
int mark_one(void *vptr)
{
    // TODO complete this function
    struct mem_chunk *current = memory_list_head;
    while (current != NULL)
    {
        if (current->address == vptr)
        {
            if (current->in_use == USED)
            {
                return 1;
            }
            else
            {
                current->in_use = USED;
                return 0;
            }
        }
        current = current->next;
    }
    return 2; // chunk is not found
}

void print_memory_list()
{
    struct mem_chunk *current = memory_list_head;
    while (current != NULL)
    {
        printf("%lx (%d) -> ", (unsigned long)current->address, current->in_use);
        current = current->next;
    }
    printf("\n");
}

void cleanup_gc()
{
    if (logfile != NULL)
    {
        fclose(logfile);
    }
}