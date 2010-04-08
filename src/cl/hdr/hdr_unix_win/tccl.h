/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: TC.H - Global definitions for the SI compatibility library.
**
** Description:
**      The file contains the type used by TC and the definition of the
**      TC functions.  These are used for COMM-files I/O.
**
** History:
**      22-apr-89 (eduardo)
**          Initial version.
**	23-may-89 (mca)
**	    Eduardo's VMS version slightly modified for initial UNIX use.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Remove superfluous "typedef" before structure.
**	21-aug-90 (rog)
**	    Changed some definitions inside the structure TCFILE to make it
**		more portable.
**	27-aug-90 (rog)
**	    Remove TC_ESCAPE because it shouldn't be globally visible.
**	28-aug-90 (rog)
**	    Back out the above two changes becuase they need to go through
**	    the proper review process.
**	08-oct-90 (rog)
**	    Change TCFILE to use uchar's instead of char's.  Also, TCgetc()
**		returns nat's, not i4's.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	25-mar-97 (mcgem01)
**	    Now that NT is using WIN32 API, the file identifiers are
**	    HANDLES.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* 
** Forward and/or External typedef/struct references.
*/
typedef struct _TCFILE TCFILE;


/*
**  Forward and/or External function references.
*/

/* 
** Defined Constants
*/

/* TC special characters */

#define	    TC_EOF	    -1	    /* end of file */
#define	    TC_TIMEDOUT	    (i4)-2 /* time out on TCgetc */
#define	    TC_EOQ	    034	    /* end of query */
#define	    TC_EQR	    035	    /* end of query results */
#define	    TC_BOS	    036	    /* beginning of session */

/* The following symbols are machine dependent */

/* special status */

#define	    TC_BAD_SYNTAX   10

#define     TCBUFSIZE	    1024


/*
** Type Definitions.
*/

/*
 * Sync files are a temporary measure - currently the SEP mainline
 * implicitly requires that a TCFILE be able to support more than one
 * process on each end. This restriction is scheduled to be removed almost
 * immediately.  5/25/89 mca
 */

# ifdef NT_GENERIC
struct _TCFILE
{
	uchar	_buf[TCBUFSIZE];
	uchar	*_pos;
	uchar	*_end;
	HANDLE	_id;
	char	_flag;
	char	_fname[256];
	HANDLE	_sync_id;
	char	_sync_fname[256];
	int	_size;
};
# else
struct _TCFILE
{
	uchar	_buf[TCBUFSIZE];
	uchar	*_pos;
	uchar	*_end;
	int	_id;
	char	_flag;
	char	_fname[256];
	int	_sync_id;
	char	_sync_fname[256];
	int	_size;
};
# endif /* NT_GENERIC */


#define	    _TCREAD	    01
#define	    _TCWRT	    02
#define	    _TCOPEN	    04
#define	    _TCCREAT        010
#define	    _TCEOF	    020

