
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	irstd.h - ILRF standard header file
**
** Description:
**	standard header file for ILRF routines.  This header defines
**	information concerning frames maintained by ILRF module
**	to allow caching / recovery of cached frames.
**
** History:
**	01-jun-92 (davel)
**		Set MAX_IN_MEM to infinity, essentially removing the
**		in-memory limit on frames.  This can be overridden
**		if the user sets an environment variable before invoking
**		the interpreter.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <si.h>
#include <lo.h>
#include <ifid.h>
#include <ade.h>
#include <frmalign.h>
#include <fdesc.h>
#include <iltypes.h>
#include <ifrmobj.h>
#include <abfrts.h>
#include <ioi.h>
#include <ilrf.h>
#include <ilerror.h>
#include <qu.h>
#include "irint.h"

typedef struct _i_f_ctl
{
	QUEUE	fi_queue;		/* circularly doubly linked list of
					   these structs with a simple QUEUE
					   node as the head */
	FID fid;			/* frame id */
	i4 irbid;			/* IRBLK id */
	PTR frmem;			/* frame is static, this is memory
					** that the client has asked us to
					** save with the frame
					*/
	u_i4 is_current:1;		/* somebody's current frame */
	u_i4 in_mem:1;			/* frame is in memory */
	u_i4 tmp_cache:1;		/* file-cachable (NOT "is cached") */

	/* state dependent information */
	union
	{
		/* for frames in temp. file */
		struct
		{
			i4 pos;		/* temp. file position */ 
		} f;

		/* for frames in memory */
		struct
		{
			IFRMOBJ *frame;	/* in-memory frame */
			i2 tag;		/* storage tag */
		} m;
	} d;
} IR_F_CTL;

/* irbid for NON-CURRENT DB frames */
#define NULLIRBID 0	/* irbid for DB frames - common use among clients */

/*
** maximum number of in-memory frames to allow at once.  This number will
** get exceeded when there are more clients than this, each with a current
** frame.
*/

#ifdef PCINGRES
# define MAX_IN_MEM 4
#else
# define MAX_IN_MEM 32767
#endif

