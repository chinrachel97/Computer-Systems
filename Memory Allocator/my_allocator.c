/* 
    File: my_allocator.c

    Author: Rachel Chin
            Department of Computer Science
            Texas A&M University
    Date  : 02/05/18

    Modified: 

    This file contains the implementation of the module "MY_ALLOCATOR".

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "my_allocator.h"

/*--------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */ 
/*--------------------------------------------------------------------------*/

Addr ALL_MEMORY;
node** FREE_LIST;
int ALL_MEMORY_SIZE;
int FREE_LIST_SIZE;
int BASIC_BLOCK_SIZE;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

// add item to the end of the list
void push(node* n, int index){
	if(FREE_LIST[index] == NULL)
		FREE_LIST[index] = n;
	else{
		node* curr = FREE_LIST[index];
		while(curr->next != NULL)
			curr = curr->next;
		curr->next = n;
		n->prev = curr;
	}
}

// delete a node
void delete_n(node* n, int index){
	if(FREE_LIST[index] == n)
		FREE_LIST[index] = n->next;
	else{
		n->prev->next = n->next;
		if(n->next != NULL)
			n->next->prev = n->prev;
	}
}

// look for available block
node* find_free_block(int index){	
	node* curr = FREE_LIST[index];
	while(curr != NULL && curr->is_used)
		curr = curr->next;
	return curr;
}

// find the partner block 
node* find_buddy(int index, node* buddy1){		
	node* curr = find_free_block(index);
	while((curr != NULL && curr->is_used) ||
		buddy1 == curr ||
		!((uintptr_t)buddy1^(uintptr_t)curr)){
		curr = curr->next;
	}
	return curr;
}

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

	/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FUNCTIONS FOR MODULE MY_ALLOCATOR */
/*--------------------------------------------------------------------------*/

/* Don't forget to implement "init_allocator" and "release_allocator"! */


extern unsigned int init_allocator(unsigned int basic_block_size, 
				unsigned int length){
	// check whether user gives correct input
	BASIC_BLOCK_SIZE = basic_block_size;
	int bbs = basic_block_size;
	int l = length;
	while(bbs % 2 == 0)
		bbs /= 2;
	while(l % 2 == 0)
		l /= 2;
	if(bbs != 1 || l != 1){
		printf("Please input powers of 2 only.\n");
		return -1;
	}
	
	// initialize memory and free list
	ALL_MEMORY_SIZE = length;
	ALL_MEMORY = malloc(length);
	FREE_LIST_SIZE = log2(length) - log2(basic_block_size) + 1;
    FREE_LIST = (node**) malloc(FREE_LIST_SIZE * sizeof(node));
	
	// initialize first pointer to memory block
	node* head = (node*) ALL_MEMORY;
	head->next = NULL;
	head->prev = NULL;
	head->block_size = length;
	head->side = '\0';
	head->is_used = false;
	FREE_LIST[FREE_LIST_SIZE-1] = head;
	
	return 0;								
}

// recursively split blocks into needed size
int split(int target){
	int target_index = log2(target) - log2(BASIC_BLOCK_SIZE);
	
	node* free_block = find_free_block(target_index);
	if(target_index < FREE_LIST_SIZE && free_block == NULL)
		split(target*2);
	
	free_block = find_free_block(target_index);
	
	// delete the free block from its original list
	if(free_block != NULL)
		delete_n(free_block, target_index);
	else
		return -1;
	
	// get the first/second half pointers
	node* fst_ptr = free_block;
	node* snd_ptr = (node*)((char*)free_block + (fst_ptr->block_size / 2));
	
	fst_ptr->next = snd_ptr;
	snd_ptr->next = NULL;

	fst_ptr->prev = free_block->prev;
	snd_ptr->prev = fst_ptr;
	
	fst_ptr->block_size /= 2;
	snd_ptr->block_size = fst_ptr->block_size;
	
	fst_ptr->side = 'L';
	snd_ptr->side = 'R';
	
	fst_ptr->is_used = false;
	snd_ptr->is_used = false;
	
	// push the free block into the smaller size
	if(target_index-1 >= 0)
		push(fst_ptr, target_index-1);
	
	return 0;
}

extern Addr my_malloc(unsigned int length) {
	// check if requesting too much memory
	if(length > ALL_MEMORY_SIZE){
		printf("ERROR: requesting too much memory.\n");
		return NULL;
	}
	
	// find the amount needed to give
	int amt = 0;
	if(length < BASIC_BLOCK_SIZE)
		amt = BASIC_BLOCK_SIZE;
	else{
		int i = 1;
		while(amt < length){
			amt = pow(2, i);
			++i;
		}
	}
    
	if(length > (amt - sizeof(node)))
		amt *= 2;
	
	// get the index 
	int amt_index = log2(amt) - log2(BASIC_BLOCK_SIZE);
	
	// find a free block 
	node* free_block = find_free_block(amt_index);
	if(free_block == NULL){
		split(amt*2);
		free_block = find_free_block(amt_index);
	}
	if(free_block == NULL){
		printf("ERROR: There is not enough memory. Please allocate more.\n");
		release_allocator();
		exit(0);
		return NULL;
	}
	
	free_block->is_used = true;
	Addr ret = (Addr)((char*)free_block + sizeof(node));
	return ret;
}

// recursively merge two partner blocks together
int merge(node* A, node* B){
	int index = log2(A->block_size) - log2(BASIC_BLOCK_SIZE);
	node* start;
	
	delete_n(A, index);
	delete_n(B, index);
	
	// A is left, B is right
	if(A->side == 'L'){
		start = A;
		
		// set A->next = NULL
		A->next = NULL;
		
		// push A into FREE_LIST[index+1]
		if(index+1 < FREE_LIST_SIZE)
			push(A, index+1);
	}
	// do the reverse
	else{
		start = B;
		B->next = NULL;
		if(index+1 < FREE_LIST_SIZE)
			push(B, index+1);
	}
	start->block_size *= 2;
	start->is_used = false;
	node* another = find_buddy(index+1, start);
	if(another != NULL)
		merge(start, another);
	else
		return 0;
}

extern int my_free(Addr a) {
	node* n = (node*)((char*)a - sizeof(node));
	if(n == NULL)
		return 0;
	
	int index = log2(n->block_size) - log2(BASIC_BLOCK_SIZE);
	n->is_used = false;
	
	node* buddy = find_buddy(index, n);
	
	if(buddy != NULL)
		merge(n, buddy);
	
    return 0;
}

extern int release_allocator(){
	free(ALL_MEMORY);
	free(FREE_LIST);
	return 0;
} 

void print_allocator (){
	printf("Printing allocator.\n");
	for(int i=0; i<FREE_LIST_SIZE; ++i){
		node* it = FREE_LIST[i];
		int count = 0;
		if(FREE_LIST[i] != NULL){
			while(it != NULL){
				++count;
				it = it->next;
			}
			printf("[%d]-> %d: %d\n", i, FREE_LIST[i]->block_size, count);
		}
		else
			printf("[%d]-> Empty.\n", i);
	}
	printf("Printing ended.\n");
}
