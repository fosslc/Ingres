/******************************************************************************
**
** Copyright (c) 1998, 2008 Ingres Corporation
**
******************************************************************************/

/*
** Name: ddfcom.h - Data Dictionary Facility. Common Data and Functions
**
** Description:
**  This file contains common include, structure and function declarations 
**	used by the ddf module
**
** History: 
**  02-mar-98 (marol01)
**          Created
**	29-Jul-1998 (fanra01)
**	    Updated case of include filenames.
**	    Removed include of dbdbms.h.
**	    Cast assignments in CLEAR_STATUS macro.
**      07-Jan-1999 (fanra01)
**          Add auth type defines.
**      12-Mar-1999 (schte01)
**        Move #includes and #defines to column 1.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	02-may-2008 (joea)
**	    Fix G_ME_REQ_MEM to avoid problems when used in an if statement.
**/

#ifndef DDF_COMMON_INCLUDED
#define DDF_COMMON_INCLUDED

#ifndef DO_NOT_INCLUDE_SCF_HDR
#include <compat.h>
#include <me.h>
#include <st.h>
#include <tr.h>
#include <cs.h>
#include <tm.h>
#include <dl.h>
#include <nm.h>
#include <gl.h>
#include <sl.h>
#include <cv.h>
#include <ex.h>
#include <pc.h>
#include <er.h>
#include <cs.h>
#include <cm.h>
#include <pm.h>
#include <iicommon.h>
#include <gc.h>
#include <stdarg.h>
#endif

/* Error Type */
typedef struct _GSTATUS 
{
	DB_STATUS   number;
	char *info;
} *GSTATUS; 

#define GSTAT_OK	0

/* Authentication methods */
#define DDS_REPOSITORY_AUTH     0
#define DDS_OS_USRPWD_AUTH      1

GSTATUS 
GOutOfMemory();

u_i4 
GTraceLevel();

#define G_ME_REQ_MEM(T,X,Y,Z)	((X = (Y*) MEreqmem (T, sizeof (Y) * (Z), TRUE, NULL)) ? NULL : GOutOfMemory())
#define G_ST_ALLOC(X,Y)				(Y != NULL) ? ((X = STalloc (Y)) ? NULL : GOutOfMemory()) : NULL

GSTATUS 
GAlloc(
	PTR		*object, 
	u_i4	length,
	bool	preserved);

GSTATUS 
DDFStatusAlloc (
	DB_STATUS status);

GSTATUS
DDFStatusInfo (
	GSTATUS status,
	char *text, 
	... );

void
DDFStatusFree(
	u_i4	trace_level,
	GSTATUS *status);

#define CLEAR_STATUS(X) { GSTATUS clear_err = (GSTATUS)GSTAT_OK; clear_err = X; \
						   if (clear_err != GSTAT_OK) \
							  DDFStatusFree(TRC_INFORMATION, &clear_err); }

#define G_ASSERT(X, Y) if (X) return(DDFStatusAlloc(Y));

/* Trace management */

void
DDFTraceInit(
		u_i4	level,
    char*	filename);

void
DDFTrace(
    char *format,
    ...);

#define G_TRACE(X)  if (X <= GTraceLevel()) DDFTrace

#ifdef _DEBUG
#define G_DTRACE(X) if (X <= GTraceLevel()) DDFTrace
#else
#define G_DTRACE(X)
#endif

# define NUM_MAX_TIME_STR       25
# define NUM_MAX_INT_STR				10
# define NUM_MAX_ADD_STR				32
#	define ERROR_BLOCK						2048

#define TRC_EVERYTIME	0
#define TRC_INFORMATION	1
#define TRC_EXECUTION	2

GSTATUS 
GuessFileType (
		char* data, 
		int len, 
		char **category,
		char **ext);


#endif
