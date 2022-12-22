/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"


/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] cut_paste_pages
	// Write your code here, remove the panic and write your code
	//panic("cut_paste_pages() is not implemented yet...!!");
	uint32 src_start_address = ROUNDDOWN(source_va, PAGE_SIZE);
	uint32 dest_start_address = ROUNDDOWN(dest_va, PAGE_SIZE);
	uint32 dest_end_address = ROUNDDOWN(dest_va + num_of_pages * PAGE_SIZE, PAGE_SIZE);

	for(uint32 i = dest_start_address; i < dest_end_address; i+=PAGE_SIZE) {
		uint32 *ptr_page_table_dest = NULL;
		struct FrameInfo* res_des = get_frame_info(page_directory, i, &ptr_page_table_dest);
		if(res_des != NULL) return -1;
	}

	for(uint32 i = dest_start_address, j = src_start_address; i < dest_end_address; i+=PAGE_SIZE, j+=PAGE_SIZE) {
		uint32 *ptr_page_table_src = NULL;
		struct FrameInfo* source_frame_info = get_frame_info(page_directory, j, &ptr_page_table_src);

		int perms = (ptr_page_table_src[PTX(j)] & 0x00000FFF);
		map_frame(page_directory, source_frame_info, i, perms);
		unmap_frame(page_directory, j);
	}
	return 0;
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] copy_paste_chunk
	// Write your code here, remove the panic and write your code
	//panic("copy_paste_chunk() is not implemented yet...!!");
	uint32 src_start_address = ROUNDDOWN(source_va, PAGE_SIZE);
	uint32 dest_start_address = ROUNDDOWN(dest_va, PAGE_SIZE);
	uint32 dest_end_address = ROUNDUP(dest_va + size, PAGE_SIZE);

	for(uint32 i = dest_start_address; i < dest_end_address; i+=PAGE_SIZE) {
		uint32 *ptr_page_table_dest = NULL;
		struct FrameInfo* res_des = get_frame_info(page_directory, i, &ptr_page_table_dest);
		if(res_des != NULL && (ptr_page_table_dest[PTX(i)] & PERM_WRITEABLE) == 0) return -1;
	}

	for(uint32 i = dest_start_address, j = src_start_address; i < dest_end_address; i+=PAGE_SIZE, j+=PAGE_SIZE) {
		uint32 *ptr_page_table_src = NULL;
		struct FrameInfo* source_frame_info = get_frame_info(page_directory, j, &ptr_page_table_src);

		uint32 *ptr_page_table_dest = NULL;
		struct FrameInfo* dest_frame_info = get_frame_info(page_directory, i, &ptr_page_table_dest);

		if(dest_frame_info == NULL) {//page does not exist
			struct FrameInfo* ptr_frame_info = NULL;
			int ret = allocate_frame(&ptr_frame_info);
			if(ret == E_NO_MEM) return -1;
			int perms = PERM_WRITEABLE | (ptr_page_table_src[PTX(j)] & PERM_USER);
			map_frame(page_directory, ptr_frame_info, i, perms);
		}
	}
	// copy content
	for(uint32 kdest = dest_va, ksrc = source_va; kdest < dest_va + size; kdest++, ksrc++) {
		unsigned char *ptr_src = (unsigned char *)(ksrc);
		unsigned char *ptr_dest = (unsigned char *)(kdest);
		(*ptr_dest) = (*ptr_src);
	}
	return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should share the given size from dest_va with the source_va
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] share_chunk
	// Write your code here, remove the panic and write your code
	//panic("share_chunk() is not implemented yet...!!");
	uint32 sourceStartAddress = ROUNDDOWN(source_va, PAGE_SIZE);
	uint32 destStartAddress = ROUNDDOWN(dest_va, PAGE_SIZE);
	uint32 destEndAddress = ROUNDUP(dest_va + size, PAGE_SIZE);

	for(int i = destStartAddress; i < destEndAddress ; i+=PAGE_SIZE){
		uint32 *ptr_page_table1 = NULL;
		struct FrameInfo* res1 = get_frame_info(page_directory, i, &ptr_page_table1);
		if(res1 != NULL) return -1;
	}

	for(int i = destStartAddress, count = 0 ; i < destEndAddress ; i+=PAGE_SIZE, count++){
		uint32 *ptr_page_table2 = NULL;
		uint32 sourceAddress = (sourceStartAddress + (count * PAGE_SIZE));
		struct FrameInfo* res2 = get_frame_info(page_directory, sourceAddress, &ptr_page_table2);
		map_frame(page_directory, res2, i, perms);
	}
	return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate in RAM the given range [va, va+size)
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] allocate_chunk
	// Write your code here, remove the panic and write your code
	//panic("allocate_chunk() is not implemented yet...!!");
	uint32 startAddress = ROUNDDOWN(va, PAGE_SIZE);
	uint32 endAddress = ROUNDUP((va + size), PAGE_SIZE);

	for(int i = startAddress; i < endAddress; i+=PAGE_SIZE) {
		uint32 * ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory, i, &ptr_page_table);
		if(ptr_frame_info != NULL) {
			return -1;
		}
	}

	for(int i = startAddress; i < endAddress; i+=PAGE_SIZE) {
		uint32 * ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory, i, &ptr_page_table);

		int ret = allocate_frame(&ptr_frame_info);
		if(ret == E_NO_MEM) return -1;
		else {
			ptr_frame_info->va = i;
			map_frame(page_directory, ptr_frame_info, i, perms);
		}
	}
	return 0;
}

/*BONUS*/
//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	//TODO: [PROJECT MS2 - BONUS] [CHUNK OPERATIONS] calculate_allocated_space
	// Write your code here, remove the panic and write your code
	//panic("calculate_allocated_space() is not implemented yet...!!");
	int count_tables = 0, count_pages = 0;
	uint32 startAddress = ROUNDDOWN(sva, PAGE_SIZE * 1024);
	uint32 endAddress = ROUNDUP(eva, PAGE_SIZE);

	for(int i = startAddress; i < endAddress; i+=PAGE_SIZE * 1024) {
		uint32 * ptr_page_table = NULL;
		get_page_table(page_directory, i, &ptr_page_table);
		if(ptr_page_table != NULL) count_tables++;
	}
	startAddress = ROUNDDOWN(sva, PAGE_SIZE);
	for(int i = startAddress; i < endAddress; i+=PAGE_SIZE) {
		uint32 * ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory, i, &ptr_page_table);
		if(ptr_frame_info != NULL) count_pages++;
	}
	*num_tables = count_tables;
	*num_pages = count_pages;
}

/*BONUS*/
//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
// calculate_required_frames:
// calculates the new allocation size required for given address+size,
// we are not interested in knowing if pages or tables actually exist in memory or the page file,
// we are interested in knowing whether they are allocated or not.
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	//TODO: [PROJECT MS2 - BONUS] [CHUNK OPERATIONS] calculate_required_frames
	// Write your code here, remove the panic and write your code
	//panic("calculate_required_frames() is not implemented yet...!!");
	uint32 count = 0;
	uint32 startAddress = ROUNDDOWN(sva, PAGE_SIZE);
	uint32 endAddress = ROUNDUP(sva + size, PAGE_SIZE);

	for(int i = startAddress; i < endAddress; i+=PAGE_SIZE) {
		uint32 * ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory, i, &ptr_page_table);
		if(ptr_frame_info == NULL) count++;
	}
	startAddress = ROUNDDOWN(sva, PAGE_SIZE * 1024);
	for(int i = startAddress; i < endAddress; i+=PAGE_SIZE * 1024) {
		uint32 * ptr_page_table = NULL;
		get_page_table(page_directory, i, &ptr_page_table);
		if(ptr_page_table == NULL) count++;
	}
	return count;
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	// Write your code here, remove the panic and write your code
	panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================

void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3] [USER HEAP - KERNEL SIDE] free_user_mem
	// Write your code here, remove the panic and write your code
	//panic("free_user_mem() is not implemented yet...!!");
	uint32 start_sva = ROUNDDOWN(virtual_address, PAGE_SIZE);
	uint32 end_sva = ROUNDUP(virtual_address + size, PAGE_SIZE);

	for(uint32 i = start_sva; i < end_sva; i+=PAGE_SIZE) {// remove env page
		pf_remove_env_page(e, i);
	}

	for(uint32 i = 0; i < e->page_WS_max_size; i++) {// clear ws
		uint32 va_of_ws = env_page_ws_get_virtual_address(e, i);
		if(va_of_ws >= start_sva && va_of_ws < end_sva) {
			unmap_frame(e->env_page_directory, va_of_ws);
			env_page_ws_clear_entry(e, i);
			e->page_last_WS_index = i;
		}
	}

	for(uint32 i = start_sva; i < end_sva; i+=PAGE_SIZE) {
		uint32 * ptr_page_table = NULL;
		bool exist = 0;

		get_page_table(e->env_page_directory, i, &ptr_page_table);
		if(ptr_page_table != NULL) {
			for(int j = 0; j < 1024; j++) {
				if(ptr_page_table[j] != 0) {
					exist = 1;
					break;
				}
			}
			if(!exist) {
				kfree((void*)ptr_page_table);
				pd_clear_page_dir_entry(e->env_page_directory, i);
			}
		}
	}
	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");

	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Free any BUFFERED pages in the given range
	//4. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3 - BONUS] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory

	/**/
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//
