/*
 * dyn_block_management.c
 *
 *  Created on: Sep 21, 2022
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// PRINT MEM BLOCK LISTS:
//===========================

void print_mem_block_lists()
{
	cprintf("\n=========================================\n");
	struct MemBlock* blk ;
	struct MemBlock* lastBlk = NULL ;
	cprintf("\nFreeMemBlocksList:\n");
	uint8 sorted = 1 ;
	LIST_FOREACH(blk, &FreeMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nFreeMemBlocksList is NOT SORTED!!\n") ;

	lastBlk = NULL ;
	cprintf("\nAllocMemBlocksList:\n");
	sorted = 1 ;
	LIST_FOREACH(blk, &AllocMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nAllocMemBlocksList is NOT SORTED!!\n") ;
	cprintf("\n=========================================\n");

}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//===============================
// [1] INITIALIZE AVAILABLE LIST:
//===============================
void initialize_MemBlocksList(uint32 numOfBlocks)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] initialize_MemBlocksList
	// Write your code here, remove the panic and write your code
	//panic("initialize_MemBlocksList() is not implemented yet...!!");

	LIST_INIT(&AvailableMemBlocksList);
	for(int i = 0; i < numOfBlocks; i++) {
		LIST_INSERT_HEAD(&AvailableMemBlocksList, &(MemBlockNodes[i]));
	}
}

//===============================
// [2] FIND BLOCK:
//===============================
struct MemBlock *find_block(struct MemBlock_List *blockList, uint32 va)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] find_block
	// Write your code here, remove the panic and write your code
	//panic("find_block() is not implemented yet...!!");

	struct MemBlock *element;
	LIST_FOREACH(element, (blockList)) {
		if(element->sva == va) {
			return element;
		}
	}
	return NULL;
}

//=========================================
// [3] INSERT BLOCK IN ALLOC LIST [SORTED]:
//=========================================
void insert_sorted_allocList(struct MemBlock *blockToInsert)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] insert_sorted_allocList
	// Write your code here, remove the panic and write your code
	//panic("insert_sorted_allocList() is not implemented yet...!!");

	struct MemBlock *element, *lastSmallerElement = NULL;
	LIST_FOREACH(element, &(AllocMemBlocksList)) {
		if(element->sva > blockToInsert->sva) break;
		lastSmallerElement = element;
	}
	if(lastSmallerElement == NULL) {
		LIST_INSERT_HEAD(&(AllocMemBlocksList), blockToInsert);
	}
	else {
		LIST_INSERT_AFTER(&(AllocMemBlocksList), lastSmallerElement, blockToInsert);
	}
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
struct MemBlock *alloc_block_FF(uint32 size)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] alloc_block_FF
	// Write your code here, remove the panic and write your code
	//panic("alloc_block_FF() is not implemented yet...!!");
	struct MemBlock *element, *point;
	LIST_FOREACH(element, &(FreeMemBlocksList)) {
		if(element->size == size) {
			point = element;
			LIST_REMOVE(&(FreeMemBlocksList), element);
			return point;
		}
		else if (element->size > size) {
			struct MemBlock *block = LIST_FIRST(&(AvailableMemBlocksList));
			LIST_REMOVE(&(AvailableMemBlocksList), block);
			block->size = size;
			block->sva = element->sva;
			element->size -= size;
			element->sva += size;
			return block;
		}
	}
	return NULL;
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
struct MemBlock *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] alloc_block_BF
	// Write your code here, remove the panic and write your code
	//panic("alloc_block_BF() is not implemented yet...!!");
	uint32 minimumSize = 2147483647;
	struct MemBlock *element, *firstGreaterElement = NULL, *result;
	LIST_FOREACH(element, &(FreeMemBlocksList)) {
		if(element->size >= size && element->size < minimumSize) {
			minimumSize = element->size;
			firstGreaterElement = element;
		}
	}

	if(firstGreaterElement == NULL) result = NULL;
	else if(firstGreaterElement->size == size) {
		LIST_REMOVE(&(FreeMemBlocksList), firstGreaterElement);
		result = firstGreaterElement;
	}
	else {
		result = LIST_FIRST(&(AvailableMemBlocksList));
		LIST_REMOVE(&(AvailableMemBlocksList), result);
		result->size = size;
		result->sva = firstGreaterElement->sva;
		firstGreaterElement->size -= size;
		firstGreaterElement->sva += size;
	}

	return result;
}
//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================

struct MemBlock *alloc_block_NF(uint32 size)
{
	//TODO: [PROJECT MS1 - BONUS] [DYNAMIC ALLOCATOR] alloc_block_NF
	static uint32 last = 0;
	struct MemBlock* block;
	LIST_FOREACH(block, &(FreeMemBlocksList)) {
		if(block->sva < last || block->size < size) continue;

		if(block->size == size) {
			last = block->sva + size;
			LIST_REMOVE(&FreeMemBlocksList, block);
			return block;
		}
		else {
			struct MemBlock *result = LIST_FIRST(&(AvailableMemBlocksList));
			LIST_REMOVE(&(AvailableMemBlocksList), result);
			result->sva = block->sva;
			result->size = size;
			block->sva += size;
			block->size -= size;
			last = result->sva + size;
			return result;
		}
	}
	struct MemBlock * first_fit_block = alloc_block_FF(size);
	if(first_fit_block != NULL) {
		last = first_fit_block->sva;
		if(first_fit_block->size == size && LIST_NEXT(first_fit_block) != NULL) {
			last = LIST_NEXT(first_fit_block)->sva;
		}
	}
	return first_fit_block;
}

//===================================================
// [8] INSERT BLOCK (SORTED WITH MERGE) IN FREE LIST:
//===================================================
void insert_sorted_with_merge_freeList(struct MemBlock *blockToInsert)
{
	//cprintf("BEFORE INSERT with MERGE: insert [%x, %x)\n=====================\n", blockToInsert->sva, blockToInsert->sva + blockToInsert->size);
	//print_mem_block_lists() ;
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] insert_sorted_with_merge_freeList
	// Write your code here, remove the panic and write your code
	//panic("insert_sorted_with_merge_freeList() is not implemented yet...!!");

	if(FreeMemBlocksList.size == 0) {
		LIST_INSERT_HEAD(&FreeMemBlocksList, blockToInsert);
		return;
	}

	struct MemBlock* right = LIST_FIRST(&FreeMemBlocksList);
	if(LIST_LAST(&FreeMemBlocksList)->sva < blockToInsert->sva) {
		LIST_INSERT_TAIL(&FreeMemBlocksList, blockToInsert);
		right = NULL;
	}
	else {
		for(int i = 0; i < FreeMemBlocksList.size; i++) {
			if(right->sva > blockToInsert->sva) {
				LIST_INSERT_BEFORE(&FreeMemBlocksList, right, blockToInsert);
				break;
			}
			right = LIST_NEXT(right);
		}
	}

	struct MemBlock* left = LIST_PREV(blockToInsert);
	if(left != NULL && left->sva + left->size == blockToInsert->sva) {
		blockToInsert->sva = left->sva;
		blockToInsert->size += left->size;

		left->size = left->sva = 0;
		LIST_REMOVE(&FreeMemBlocksList, left);
		LIST_INSERT_HEAD(&AvailableMemBlocksList, left);
	}
	if(right != NULL && blockToInsert->sva + blockToInsert->size == right->sva) {
		blockToInsert->size += right->size;
		right->size = right->sva = 0;

		LIST_REMOVE(&FreeMemBlocksList, right);
		LIST_INSERT_HEAD(&AvailableMemBlocksList, right);
	}
	//cprintf("\nAFTER INSERT with MERGE:\n=====================\n");
	//print_mem_block_lists();
}
