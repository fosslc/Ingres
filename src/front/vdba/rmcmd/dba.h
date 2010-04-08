/********************************************************************
**
**  Copyright (C) 1995 Ingres Corporation
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : dba.h
**    defines for remote commands - subset of dba.h client side
**
********************************************************************/
/*
**
** History:
**
**
**	04-Jan-96 (spepa01)
**	    Replace strcasecmp() with the CL function STbcompare(). Also
**	    separate the unrelated definitions of sleep() and stricmp().
*/

#ifndef DBA_HEADER
#define DBA_HEADER

#include <string.h>

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef FALSE
#define FALSE       0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define RES_ERR                 0
#define RES_SUCCESS             1
#define RES_TIMEOUT             2
#define RES_NOGRANT             3
#define RES_ENDOFDATA           4
#define RES_ALREADYEXIST        5
#define RES_30100_TABLENOTFOUND 6 

typedef unsigned char UCHAR;   
typedef unsigned char * LPUCHAR;

#ifdef WIN32
#define sleep(x) Sleep(x*1000)
#endif /* WIN32 */

#ifndef WIN32
#define stricmp(a,b) ((int)STbcompare(a, STlength(a), b, STlength(b), TRUE))
#endif

#endif  /* #define DBA_HEADER */
