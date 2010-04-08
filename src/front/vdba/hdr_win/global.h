/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : global.h
//
//     
//
********************************************************************/
// Checked for CA-Clipper Workbench

#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#ifdef ALLO
	#define GLOBAL
	#define	INIT(x)	x
#else
	#define	GLOBAL	extern
	#define INIT(x)	
#endif




#define		MALLOC(x)		MemAlloc((x))
#define		MALLOCSTR(x)	MemAlloc((x)+1)
#define		FREE(x)			MemFree((x)); (x) = NULL
#define 	SIMPLE_FREE(s)	MemFree((s))
#define		GRP_MALLOC(g, x)		(MemGrpAlloc(g, x))
#define		GRP_MALLOCSTR(g,x)	(MemGrpAlloc(g, (x+1)))
#define		GRP_FREE(g)			(MemGrpClose(g))


GLOBAL	HINSTANCE		hAppInstance;
GLOBAL	HACCEL			hAppAccel;
#define	APP_INST()		(hAppInstance)
#define	APP_ACCEL()		(hAppAccel)


#define	STRINGTABLE_LEN		((WORD) 255)
#include <assert.h>
#include "errout.h"

#endif // GLOBAL_INCLUDED

