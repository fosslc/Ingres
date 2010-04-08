#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#ifndef	VMS
#include    <si.h>	    /* Needed for "erloc.h", if not VMS */
#endif
#include    <cs.h>	    /* Needed for "erloc.h" */
#include    "erloc.h"

/*{
** Name: cer_excep --- Exception handler in ER routine (ERget()).
**
** Description
**	If error occures in ERget routine, it passes this routine to assign
**	the pointer of internal error message to the character pointer 
**	returned ERget(). 
**	This routine will not be used extanally.
**
** Input		status  -----  Error status
**		    char *msgpp ----- pointer of message pointer to set message
**
** Output
**	    none.
**  Return
**	    none.
**
**  Exception
**	none.
** Sideeffect
**	    If error occurses in ERget(), ERget() will return 
**	    the internal error message  assigned here.
**
** History
**	    29-Apr-1987 -- first written (kobayashi).
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions 
*/
VOID
cer_excep(status,msgpp)
STATUS	status;
char **msgpp;
{
	switch (status)
	{
		case ER_FILE_FORMAT:
			*(char **)msgpp =
			ERx("ERget: Bad file format.");
			break;

		case ER_FILE_VERSION:
			*(char **)msgpp =
			ERx("ERget: Bad file version.");
			break;

		case ER_FILE_CORRUPTION:
			*(char **)msgpp =
			ERx("ERget: Corrupted data in file.");
			break;

		case(ER_NO_FILE):
			*(char **)msgpp = 
			    ERx("ERget: Message File not found.");
			break;

		case(ER_NOT_FOUND):
			*(char **)msgpp =
			ERx("ERget: Message not found.");
			break;

		case(ER_BADPARAM):
			*(char **)msgpp =
			ERx("ERget: Bad parameter.");
			break;

		case(ER_BADREAD):
			*(char **)msgpp =
			ERx("ERget: Error reading message file.");
			break;

		case(ER_TOOSMALL):
			*(char **)msgpp =
			ERx("ERget: Buffer is too small.");
			break;

		case(ER_TRUNCATED):
			*(char **)msgpp =
			ERx("ERget: Buffer is truncated.");
			break;

		case(ER_BADOPEN):
			*(char **)msgpp =
			ERx("ERget: Error opening message file.");
			break;

		case(ER_BADRCV):
			*(char **)msgpp =
			ERx("ERget: Bad message receive.");
			break;

		case(ER_BADSEND):
			*(char **)msgpp =
			ERx("Bad message send.");
			break;

		case(ER_DIRERR):
			*(char **)msgpp =
			ERx("ERget: Directory error.");
			break;

		case(ER_NOALLOC):
			*(char **)msgpp =
			ERx("ERget: Can not allocate.");
			break;

		case(ER_MSGOVER):
			*(char **)msgpp =
			ERx("ERget: Message overflow error.");
			break;

		case(ER_NOFREE):
			*(char **)msgpp =
			ERx("ERget: Can not free.");
			break;

		case(ER_BADCLASS):
			*(char **)msgpp =
			ERx("ERget: Bad class found.");
			break;

		case(ER_BADLANGUAGE):
			*(char **)msgpp =
			ERx("ERget: Bad language found.");
			break;
		default:
			*(char **)msgpp =
			ERx("ERget: Not found internal message.");
			break;
	}
	return;
}
