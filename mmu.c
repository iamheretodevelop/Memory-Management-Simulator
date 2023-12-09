#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

void TOUPPER(char * arr){
  
    for(int i=0;i<strlen(arr);i++){
        arr[i] = toupper(arr[i]);
    }
}

void get_input(char *args[], int input[][2], int *n, int *size, int *policy) 
{
  	FILE *input_file = fopen(args[1], "r");
	  if (!input_file) {
		    fprintf(stderr, "Error: Invalid filepath\n");
		    fflush(stdout);
		    exit(0);
	  }

    parse_file(input_file, input, n, size);
  
    fclose(input_file);
  
    TOUPPER(args[2]);
  
    if((strcmp(args[2],"-F") == 0) || (strcmp(args[2],"-FIFO") == 0))
        *policy = 1;
    else if((strcmp(args[2],"-B") == 0) || (strcmp(args[2],"-BESTFIT") == 0))
        *policy = 2;
    else if((strcmp(args[2],"-W") == 0) || (strcmp(args[2],"-WORSTFIT") == 0))
        *policy = 3;
    else {
       printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
       exit(1);
    }
        
}

void allocate_memory(list_t *freelist, list_t *alloclist, int pid, int blocksize, int policy) {
    node_t *current = freelist->head;
    block_t *blk = NULL;
    node_t *fragment_node = NULL;

    while (current != NULL) {
        blk = current->blk;
        if (blk->end - blk->start + 1 >= blocksize) {
            // Found a block large enough
            list_remove_node(freelist, current); // Remove the block from the free list

            // Set the blk.pid = pid
            blk->pid = pid;
            // Set the blk.end = blk->start + blocksize - 1
            blk->end = blk->start + blocksize - 1;

            // Add the blk to the ALLOC_LIST in ascending order by address
            list_add_ascending_by_address(alloclist, blk);

            // Deal with the remaining left-over memory (fragment)
            fragment_node = list_alloc_node();
            fragment_node->blk = malloc(sizeof(block_t));
            fragment_node->blk->pid = 0;
            fragment_node->blk->start = blk->end + 1;
            fragment_node->blk->end = blk->end; // original blk.end before it was changed

            // Add the fragment to the FREE_LIST based on policy
            if (policy == 1) {
                // Use list_add_to_back for FIFO order
                list_add_to_back(freelist, fragment_node->blk);
            } else if (policy == 2) {
                // Use list_add_ascending_by_blocksize for BEST FIT
                list_add_ascending_by_blocksize(freelist, fragment_node->blk);
            } else if (policy == 3) {
                // Use list_add_descending_by_blocksize for WORST FIT
                list_add_descending_by_blocksize(freelist, fragment_node->blk);
            }

            free(fragment_node);
            return;
        }
        current = current->next;
    }

    // If no space is found, print the message "Error: Memory Allocation <blocksize> blocks"
    printf("Error: Memory Allocation %d blocks\n", blocksize);
}

void deallocate_memory(list_t *alloclist, list_t *freelist, int pid, int policy) {
    // Implementation of deallocate_memory
    node_t *current = alloclist->head;
    block_t *blk = NULL;
    node_t *fragment_node = NULL;

    while (current != NULL) {
        blk = current->blk;
        if (blk->pid == pid) {
            // Found the block with the given pid
            list_remove_node(alloclist, current); // Remove the block from the allocated list

            // Set the blk.pid back to 0
            blk->pid = 0;

            // Add the blk back to the FREE_LIST based on policy
            if (policy == 1) {
                // Use list_add_to_back for FIFO order
                list_add_to_back(freelist, blk);
            } else if (policy == 2) {
                // Use list_add_ascending_by_blocksize for BEST FIT
                list_add_ascending_by_blocksize(freelist, blk);
            } else if (policy == 3) {
                // Use list_add_descending_by_blocksize for WORST FIT
                list_add_descending_by_blocksize(freelist, blk);
            }

            // Deallocate_memory, return after the first block is deallocated
            return;
        }
        current = current->next;
    }

    // If the block with the given pid is not found, print "Error: Can't locate Memory Used by PID: <pid>"
    printf("Error: Can't locate Memory Used by PID: %d\n", pid);
}

list_t *coalese_memory(list_t *list) {
    // Sort the list in ascending order by address
    list_sort_by_address(list);

    // Iterate through the list and merge adjacent blocks
    node_t *current = list->head;
    node_t *next_node = NULL;
    node_t *new_list = list_alloc();
    block_t *new_blk = NULL;

    while (current != NULL) {
        new_blk = current->blk;
        next_node = current->next;

        while (next_node != NULL && new_blk->end + 1 == next_node->blk->start) {
            // Merge adjacent blocks
            new_blk->end = next_node->blk->end;
            free(next_node->blk);
            node_t *temp = next_node;
            next_node = next_node->next;
            free(temp);
        }

        // Add the merged or unmodified block to the new list
        list_add_to_back(new_list, new_blk);

        current = next_node;
    }

    return new_list;
}

void print_list(list_t * list, char * message){
    node_t *current = list->head;
    block_t *blk;
    int i = 0;
  
    printf("%s:\n", message);
  
    while(current != NULL){
        blk = current->blk;
        printf("Block %d:\t START: %d\t END: %d", i, blk->start, blk->end);
      
        if(blk->pid != 0)
            printf("\t PID: %d\n", blk->pid);
        else  
            printf("\n");
      
        current = current->next;
        i += 1;
    }
}

/* DO NOT MODIFY */
int main(int argc, char *argv[]) 
{
   int PARTITION_SIZE, inputdata[200][2], N = 0, Memory_Mgt_Policy;
  
   list_t *FREE_LIST = list_alloc();   // list that holds all free blocks (PID is always zero)
   list_t *ALLOC_LIST = list_alloc();  // list that holds all allocated blocks
   int i;
  
   if(argc != 3) {
       printf("usage: ./mmu <input file> -{F | B | W }  \n(F=FIFO | B=BESTFIT | W-WORSTFIT)\n");
       exit(1);
   }
  
   get_input(argv, inputdata, &N, &PARTITION_SIZE, &Memory_Mgt_Policy);
  
   // Allocated the initial partition of size PARTITION_SIZE
   
   block_t * partition = malloc(sizeof(block_t));   // create the partition meta data
   partition->start = 0;
   partition->end = PARTITION_SIZE + partition->start - 1;
                                   
   list_add_to_front(FREE_LIST, partition);          // add partition to free list
                                   
   for(i = 0; i < N; i++) // loop through all the input data and simulate a memory management policy
   {
       printf("************************\n");
       if(inputdata[i][0] != -99999 && inputdata[i][0] > 0) {
             printf("ALLOCATE: %d FROM PID: %d\n", inputdata[i][1], inputdata[i][0]);
             allocate_memory(FREE_LIST, ALLOC_LIST, inputdata[i][0], inputdata[i][1], Memory_Mgt_Policy);
       }
       else if (inputdata[i][0] != -99999 && inputdata[i][0] < 0) {
             printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));
             deallocate_memory(ALLOC_LIST, FREE_LIST, abs(inputdata[i][0]), Memory_Mgt_Policy);
       }
       else {
             printf("COALESCE/COMPACT\n");
             FREE_LIST = coalese_memory(FREE_LIST);
       }   
     
       printf("************************\n");
       print_list(FREE_LIST, "Free Memory");
       print_list(ALLOC_LIST,"\nAllocated Memory");
       printf("\n\n");
   }
  
   list_free(FREE_LIST);
   list_free(ALLOC_LIST);
  
   return 0;
}