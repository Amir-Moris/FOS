/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault
void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//	TODO: [PROJECT MS3] [FAULT HANDLER] page_fault_handler
	// Write your code here, remove the panic and write your code
	//    panic("page_fault_handler() is not implemented yet...!!");

	if(env_page_ws_get_size(curenv) < curenv->page_WS_max_size) {//Placement
		fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		int ret  = allocate_chunk(curenv->env_page_directory, fault_va, PAGE_SIZE, PERM_USER|PERM_WRITEABLE);
		if(ret == -1) return;
		int ret_2 = pf_read_env_page(curenv, (void*) fault_va);
		if(ret_2 == E_PAGE_NOT_EXIST_IN_PF){
			if(fault_va < USER_HEAP_START || fault_va >= USTACKTOP ){
				panic("ILLEGAL MEMORY ACCESS");
			}
		}
		env_page_ws_set_entry(curenv, curenv->page_last_WS_index, fault_va);
		curenv->page_last_WS_index++;
		if(curenv->page_last_WS_index == curenv->page_WS_max_size)
			curenv->page_last_WS_index = 0;
	}
	else {//RE-Placement
		//env_page_ws_invalidate();
		// if it's in working set or not
		while(1) {
			uint32 va = env_page_ws_get_virtual_address(curenv, curenv->page_last_WS_index);
			uint32 perms = pt_get_page_permissions(curenv->env_page_directory, va);
			uint32 isUsed = perms & PERM_USED;
			if(isUsed == 0x0) {
				//check if it's modified or not
				uint32 isModified = perms & PERM_MODIFIED;
				if(isModified == 0x0){
					//remove it
					unmap_frame(curenv->env_page_directory, va);
					env_page_ws_clear_entry(curenv, curenv->page_last_WS_index);

				}
				else {
					uint32 *ptr_page_table;
					struct FrameInfo *frameInfo = get_frame_info(curenv->env_page_directory, va, &ptr_page_table);
					int ret = pf_update_env_page(curenv, va, frameInfo);
					if(ret == E_NO_PAGE_FILE_SPACE) return;
					unmap_frame(curenv->env_page_directory, va);
					env_page_ws_clear_entry(curenv, curenv->page_last_WS_index);
				}
				// placement
				fault_va = ROUNDDOWN(fault_va,PAGE_SIZE);
				int ret = allocate_chunk(curenv->env_page_directory, fault_va, PAGE_SIZE, PERM_USER|PERM_WRITEABLE);
				if(ret == -1) return;
				int ret_2 = pf_read_env_page(curenv,(void*) fault_va);
				if(ret_2 == E_PAGE_NOT_EXIST_IN_PF) {
					if(fault_va < USER_HEAP_START || fault_va >= USTACKTOP ){
						panic("ILLEGAL MEMORY ACCESS");
					}
				}
				env_page_ws_set_entry(curenv,curenv->page_last_WS_index, fault_va);

				curenv->page_last_WS_index++;
				if(curenv->page_last_WS_index == curenv->page_WS_max_size)
					curenv->page_last_WS_index = 0;

				return;
			}
			else {
				// make it unused;
				pt_set_page_permissions(curenv->env_page_directory, va, perms, PERM_USED);
			}

			curenv->page_last_WS_index++;
			if(curenv->page_last_WS_index == curenv->page_WS_max_size)
				curenv->page_last_WS_index = 0;
		}
	}
}
void PLACEMENT(struct Env * curenv, uint32 fault_va){
	struct FrameInfo *frame_info_ptr =NULL ;
	//allocation of faulted page.
	int ret = allocate_frame(&frame_info_ptr);
	if(ret!=E_NO_MEM)
	{
		//map then read.
		map_frame(curenv->env_page_directory ,frame_info_ptr ,fault_va, PERM_USER |PERM_WRITEABLE|PERM_PRESENT );
		ret = pf_read_env_page(curenv,(void *)fault_va);
		//check if the page doesn't exist.
		if (ret == E_PAGE_NOT_EXIST_IN_PF)
		{
			if((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)){}
		else
			{
				panic("ILLEGAL MEM ACCESS");
			}
		}
		int size = curenv->page_WS_max_size ;
		//ybd2 yset el working set ba5r el t3delat.
		env_page_ws_set_entry(curenv,curenv->page_last_WS_index ,fault_va);
		curenv->page_last_WS_index++ ;
		//yshof 34an el working set momken tkon kbera fa yrg3 lel awl tani bl moduoles.
		curenv->page_last_WS_index = curenv->page_last_WS_index %  curenv->page_WS_max_size ;
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	// Write your code here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");


}
