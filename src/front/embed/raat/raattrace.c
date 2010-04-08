/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <nm.h>
# include <st.h>
# include <tr.h>
# include <raat.h>
/*
** Name: raattrace.c
**
** Description:
**	Support simple tracing of RAAT interface to track pilot error.
**
** History:
**	12-july-1996 (sweeney)
**	    Created.
**      28-may-97 (dilma04)
**          Added tracing of row locking flags.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static i4  initialized = 0 ;
static i4  tracelevel = 0 ;

/* from common/hdr/hdr/raat.h */
typedef struct {long opcode; char *name;} OP;

OP raat_ops[] = {
	{RAAT_SESS_START,	"RAAT_SESS_START"},
	{RAAT_SESS_END,		"RAAT_SESS_END"},

	{RAAT_TX_BEGIN,		"RAAT_TX_BEGIN"},
	{RAAT_TX_COMMIT,	"RAAT_TX_COMMIT"},
	{RAAT_TX_ABORT,		"RAAT_TX_ABORT"},

	{RAAT_TABLE_OPEN,	"RAAT_TABLE_OPEN"},
	{RAAT_TABLE_CLOSE,	"RAAT_TABLE_CLOSE"},
	{RAAT_TABLE_LOCK,	"RAAT_TABLE_LOCK"},

	{RAAT_RECORD_GET,	"RAAT_RECORD_GET"},
	{RAAT_RECORD_PUT,	"RAAT_RECORD_PUT"},
	{RAAT_RECORD_POSITION,	"RAAT_RECORD_POSITION"},
	{RAAT_RECORD_DEL,	"RAAT_RECORD_DEL"},
	{RAAT_RECORD_REPLACE,	"RAAT_RECORD_REPLACE"},
	{RAAT_BLOB_GET,		"RAAT_BLOB_GET"},
	{RAAT_BLOB_PUT,		"RAAT_BLOB_PUT"},
	{0L,			"Unknown RAAT operation"}
} ;

static char *
lookup(i4 opcode)
{
OP	*op = raat_ops;

while(op->opcode)
	{
	if ( opcode == op->opcode )
		break;
	op++;
	}

return(op->name);
}
	

/*
** Name:
**	IIraat_trace()
** Description:
**	Log some meaningful information to a trace file.
**
** Inputs:
**	op:		the RAAT operation to be performed.
**	raat_cb:	the RAAT control block.
**
** Outputs:
**
** Side effects:
**
** returns:
**
** History:
**	12-july-1996 (sweeney)
**	    Created.
**	18-jul-1996 (sweeney)
**	    Elide debug printf() left in in error.
**      28-may-97 (dilma04)
**          Added tracing of row locking flags.
*/
i4
IIraat_trace(i4 op, RAAT_CB *raat_cb)
{
CL_ERR_DESC	syserr;

if ( !initialized)
	{
	char 	*s = NULL;

	/* just the once */

	initialized = TRUE;

	/* grab trace file and level from environment. */

	NMgtAt( "II_RAAT_TRACE", &s );

	if ( s && *s )
		CVan( s, &tracelevel );

	NMgtAt( "II_RAAT_LOG", &s );

	if ( s && *s )
		TRset_file( TR_F_OPEN, s, STlength(s), &syserr );

	}

if ( !tracelevel )
	return( OK ); /* tracing off or not set */

TRdisplay( "RAAT: operation %s (%d)\n", lookup(op), op ) ;

if (tracelevel > 1 )
        /* print the flags from the control block */
        /* XXX "flag" below is a long -- does %v default to 32 bits? */
        TRdisplay( "flags: %v\n", "RAAT_TX_READ,RAAT_TX_WRITE,RAAT_LK_XT,\
RAAT_LK_ST,RAAT_LK_XP,RAAT_LK_SP,RAAT_LK_XR,RAAT_LK_SR,RAAT_LK_NL,\
RAAT_BAS_RECNUM,RAAT_REC_CURR,RAAT_REC_NEXT,RAAT_REC_PREV,RAAT_REC_YNUM,\
RAAT_REC_BYPOS,RAAT_REC_FIRST,RAAT_REC_LAST,RAAT_TBL_INFO,RAAT_TEMP_TBL,,,\
RAAT_INTERNAL_REPOS", raat_cb->flag );
 
}
