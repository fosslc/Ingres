/*
**Copyright (c) 2004 Ingres Corporation
*/
#ifndef WPS_INCLUDED
#define WPS_INCLUDED

#include <ddfcom.h>
#include <ddginit.h>
#include <wsf.h>
#include <wcs.h>
#include <htmlpars.h>


GSTATUS 
WPSInitialize ();

GSTATUS 
WPSTerminate ();

GSTATUS 
WPSPerformMacro (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	WPS_FILE 		 *cache);

GSTATUS 
WPSPerformVariable (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer);

#endif
