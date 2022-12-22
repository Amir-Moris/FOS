#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//==================================================================//
//==================================================================//
//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)//
//==================================================================//
//==================================================================//

void initialize_dyn_block_system()
{
    //TODO: [PROJECT MS2] [KERNEL HEAP] initialize_dyn_block_system

    //[1] Initialize two lists (AllocMemBlocksList & FreeMemBlocksList) [Hint: use LIST_INIT()]
    LIST_INIT(&AllocMemBlocksList);
    LIST_INIT(&FreeMemBlocksList);
    uint32 memSize = 0;
#if STATIC_MEMBLOCK_ALLOC
    //DO NOTHING
#else
    /*[2] Dynamically allocate the array of MemBlockNodes
     *     remember to:
     *         1. set MAX_MEM_BLOCK_CNT with the chosen size of the array
     *         2. allocation should be aligned on PAGE boundary
     *     HINT: can use alloc_chunk(...) function
     */
    MAX_MEM_BLOCK_CNT = (KERNEL_HEAP_MAX - KERNEL_HEAP_START)/PAGE_SIZE;
    MemBlockNodes =  (struct MemBlock *)KERNEL_HEAP_START;
    memSize =ROUNDUP(MAX_MEM_BLOCK_CNT * sizeof(struct MemBlock), PAGE_SIZE);
    int ret = allocate_chunk(ptr_page_directory,KERNEL_HEAP_START,memSize,PERM_WRITEABLE);

#endif
    //[3] Initialize AvailableMemBlocksList by filling it with the MemBlockNodes
    initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);
    //[4] Insert a new MemBlock with the remaining heap size into the FreeMemBlocksList
    struct MemBlock* freeBlock = LIST_FIRST(&AvailableMemBlocksList);
    freeBlock->size = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) - memSize;
    freeBlock->sva = KERNEL_HEAP_START + memSize;

    LIST_REMOVE(&AvailableMemBlocksList, freeBlock);
    LIST_INSERT_HEAD(&FreeMemBlocksList, freeBlock);
}
struct MemBlock * blockStrategy(int size) {
	struct MemBlock *block = NULL;
	if(isKHeapPlacementStrategyFIRSTFIT()) {
		block = alloc_block_FF(size);
	}
	else if(isKHeapPlacementStrategyBESTFIT()) {
		block = alloc_block_BF(size);
	}
	else {
		block = alloc_block_NF(size);
	}
	return block;
}
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kmalloc
	// your code is here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	size = ROUNDUP(size, PAGE_SIZE);
	struct MemBlock *block = blockStrategy(size);

	if(block != NULL) {
		int ret = allocate_chunk(ptr_page_directory, block->sva, block->size, PERM_WRITEABLE);
		if(ret == 0) {
			insert_sorted_allocList(block);
			return (void *)block->sva;
		}
	}
	//change this "return" according to your answer
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");
	struct MemBlock *element = find_block(&AllocMemBlocksList, (uint32)virtual_address);
	if(element == NULL) return ;
	LIST_REMOVE(&AllocMemBlocksList, element);

	uint32 start_sva = ROUNDDOWN(element->sva, PAGE_SIZE);
	uint32 end_sva = ROUNDUP(element->sva + element->size, PAGE_SIZE);

	for(uint32 i = start_sva; i < end_sva; i+=PAGE_SIZE) {
		unmap_frame(ptr_page_directory, i);
	}
	insert_sorted_with_merge_freeList(element);
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");
	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	uint32* ptr_page_table = NULL;
	struct FrameInfo * frame_ptr = to_frame_info(physical_address);
	return frame_ptr->va;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details
	uint32* ptr_page_table = NULL;
	get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	if(ptr_page_table == NULL) return 0;
	return (ptr_page_table[PTX(virtual_address)] >> 12) * PAGE_SIZE;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT MS2 - BONUS] [KERNEL HEAP] krealloc
	// Write your code here, remove the panic and write your code
	//panic("krealloc() is not implemented yet...!!");
	if(virtual_address == NULL) {
		return kmalloc(new_size);
	}
	else if(new_size == 0){
		kfree(virtual_address);
		return virtual_address;
	}

	struct MemBlock *block = find_block(&AllocMemBlocksList, (uint32)virtual_address);
	if(block==NULL) return kmalloc(new_size);

	if(block->size >= new_size) {
		return virtual_address;
	}

	int chunk = allocate_chunk(ptr_page_directory, block->sva+block->size, new_size-block->size, PERM_WRITEABLE);
	if(chunk == 0) {
		block->size += ROUNDUP(new_size - block->size, PAGE_SIZE);
		return virtual_address;
	}
	void *ans = kmalloc(new_size);
	if(ans == NULL) return NULL;
	copy_paste_chunk(ptr_page_directory,  (uint32)virtual_address,  (uint32)ans, block->size);
	kfree(virtual_address);
	return ans;
}
