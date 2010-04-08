/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: uleint.h - Internal typedefs and defines needed by ULE.
**
**  Description:
**	This file contains definitions of all of the structures and
**	symbolic constants needed by the ULE facility.
**
**	This file defines the following typedefs:
**	    ULE_MHDR - Message header in the log
**
**  History:
**	17-oct-2000 (somsa01)
**	    Created.
*/


/*}
**  Name: ULE_MHDR - Message header in the log
**
**  Description:
**	This structure is a set of character arrays to build
**	the header with.
**
**  History:
**	03-aug-1987 (fred)
**	    Created.
**	17-oct-2000 (somsa01)
**	    Moved here from uleformat.c .
**      26-Apr-2007 (hanal04) SIR 118196
**          Add process pid to ULE header.
**      26-Sep-2007 (wonca01) BUG 65038
**          Increase the buffer size for hostname
**	10-Sep-2008 (jonj)
**	    SIR 120874: Add ule_source error source information.
*/

/* Space reserved for source information */
#define SourceInfoLen 12 	/* file name */		\
		     + 1 	/* "." */		\
		     + 3	/* ext */		\
		     + 1	/* ":" */		\
		     + 5	/* line number */

typedef struct _ULE_MHDR
{
    char	ule_node[18];	/* node name (where applicable) */
    char	ule_pad1[3];	/* ::[ */
    char	ule_server[18];	/* server name */
    char	ule_pad2[2];	/* ', ' */
    char        ule_pid[10];    /* server pid */
    char        ule_pad3[3];    /* ', ' */
    char	ule_session[sizeof(SCF_SESSION)*2];	/* session id */
    char	ule_pad4[2];	/* ', ' */
    char	ule_source[SourceInfoLen];	/* error source info */
    char	ule_pad5[3];	/* ']: ' */
}   ULE_MHDR;


/*
**  Defines of other constants.
*/

#define	ULE_MSG_HDR_SIZE	sizeof(ULE_MHDR)

