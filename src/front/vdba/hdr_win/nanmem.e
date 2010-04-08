/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : nanmem.e
//
//    
//
********************************************************************/
/************************************************************************/
/*																																				*/
/*		NANMEM.E																														*/
/*																																				*/
/*		Description:																											*/
/*																																				*/
/*				BEAST memory block suballocator APIFUNC definitions 							*/
/*																																				*/
/*																																				*/
/*																																				*/
/*		Version 		Author	Date			Comment 														*/
/*		=================================================================== 	*/
/*		1.00			GM				03/12/91		created 														*/
/*								GM				05/22/91		added segment allocation routines		*/
/*								GM				05/27/91		changed retcodes for allocs 					*/
/*								GM				11/07/91		added flat model support						*/
/*								GM				01/24/92		removed flat model support						*/
/*																																				*/
/*		To do:																															*/
/*																																				*/
/************************************************************************/


RETCODE APIFUNC 		MemInit 						(VOID);
RETCODE APIFUNC 		MemExit 						(VOID);
LPVOID	APIFUNC 		MemAlloc						(UINT cb);
LPVOID	APIFUNC 		MemCAlloc						(UINT uiCount, UINT cbCell);
RETCODE APIFUNC 		MemFree 						(LPVOID lp);

BOOL	APIFUNC 		MemWalk			(VOID);
DWORD	APIFUNC 		MemTotal		(VOID);
UINT	APIFUNC 		MemGrpOpen		(VOID);
RETCODE APIFUNC 		MemGrpClose 	(UINT uiGroup);
LPVOID	APIFUNC 		MemGrpAlloc		(UINT uiGroup, UINT cb);
LPVOID	APIFUNC 		MemGrpCAlloc	(UINT uiGroup, UINT uiCount, UINT cbCell);
BOOL	APIFUNC 		MemCheckPtr 	(LPVOID lp, UINT cb);
WORD	APIFUNC			MemLen	 		(LPVOID lp);

LPVOID	APIFUNC 		MemAllocBlk 	(UINT cb);
LPVOID	APIFUNC 		MemReallocBlk	(LPVOID lp, UINT cb);
RETCODE APIFUNC 		MemFreeBlk		(LPVOID lp);

#ifndef WIN32

		SEL 			APIFUNC 		MemAllocSeg 					(WORD cb);
		SEL 			APIFUNC 		MemReallocSeg			(SEL sel, WORD cb);
		RETCODE APIFUNC 		MemFreeSeg						(SEL sel);
		SEL 			APIFUNC 		MemAllocSharedSeg		(WORD cb);
		SEL 			APIFUNC 		MemReallocSharedSeg 	(SEL sel, WORD cb);
		RETCODE APIFUNC 		MemSetShare 					(BOOL fShare);

		//
		// MEMTRESHOLD is the maximum allocation request for suballocation.
		// Bigger requests are redirected (transparently) to MemAllocSeg.
		//

		#define MEMTRESHOLD 	((USHORT)0x0FE4)


		//
		// A block is a reallocatable piece of memory, OS dependent !
		//

#else

		LPVOID	APIFUNC 		MemRealloc						(LPVOID lp, UINT cb);

#endif


//
// error codes
//

#define ERROR_MEM_HANDLE						((USHORT)42000)
#define ERROR_MEM_LOW							((USHORT)42001)


/*
** end of file
*/
