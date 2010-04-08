SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _QS
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	02-nov-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
*/


#define _QS_CLASS   7


E_QS0000_OK:SS00000_SUCCESS
"  Normal QSF return status."
E_QS0001_NOMEM:SS50003_EXHAUSTED_RESOURCE
"  QSF dynamic memory pool is exhausted."
E_QS0002_SEMINIT:SS5000B_INTERNAL_ERROR
"  Could not initialize semaphore from SCF."
E_QS0003_OP_CODE:SS5000B_INTERNAL_ERROR
"  Unknown QSF op code."
E_QS0004_SEMRELEASE:SS5000B_INTERNAL_ERROR
"  Could not release semaphore from SCF."
E_QS0005_ACTIVE_OBJECTS:SS5000B_INTERNAL_ERROR
"  Attempting to shut down QSF while there are still active objects."
E_QS0006_ACTIVE_SESSIONS:SS5000B_INTERNAL_ERROR
"  Attempting to shut down QSF while there are still active sessions."
E_QS0007_MEMPOOL_RELEASE:SS5000B_INTERNAL_ERROR
"  Could not release QSF's dynamic memory pool."
E_QS0008_SEMWAIT:SS5000B_INTERNAL_ERROR
"  Error while attempting to set a semaphore from SCF."
E_QS0009_BAD_OBJTYPE:SS5000B_INTERNAL_ERROR
"  Unknown type of object.  QSF only recognizes objects which are
  QUERY TEXTs, QUERY TREEs, or QUERY PLANs."
E_QS000A_OBJ_ALREADY_EXISTS:SS5000B_INTERNAL_ERROR
"  An object with the requested name and type already exists in QSF memory."
E_QS000B_BAD_OBJNAME:SS5000B_INTERNAL_ERROR
"  Illegal name for a QSF object."
E_QS000C_ULM_ERROR:SS5000B_INTERNAL_ERROR
"  Unexpected ULM error detected by QSF."
E_QS000D_CORRUPT:SS5000B_INTERNAL_ERROR
"  QSF's dynamic memory has been corrupted."
E_QS000E_SEMFREE:SS5000B_INTERNAL_ERROR
"  Could not free semaphore from SCF."
E_QS000F_BAD_HANDLE:SS5000B_INTERNAL_ERROR
"  The supplied object handle does not refer to a QSF object."
E_QS0010_NO_EXLOCK:SS5000B_INTERNAL_ERROR
"  QSF object is not exclusively locked by you."
E_QS0011_ULM_STREAM_NOT_CLOSED:SS5000B_INTERNAL_ERROR
"  Could not close a ULM memory stream."
E_QS0012_OBJ_NOT_LOCKED:SS5000B_INTERNAL_ERROR
"  QSF object is not locked by you."
E_QS0013_BAD_LOCKID:SS5000B_INTERNAL_ERROR
"  The lock ID supplied does not match the one assigned
  to the QSF object when it was exclusively locked."
E_QS0014_EXLOCK:SS5000B_INTERNAL_ERROR
"  QSF Object is already locked exclusively."
E_QS0015_SHLOCK:SS5000B_INTERNAL_ERROR
"  QSF Object is already locked shared."
E_QS0016_BAD_LOCKSTATE:SS5000B_INTERNAL_ERROR
"  Illegal lock state specified."
E_QS0017_BAD_REQUEST_BLOCK:SS5000B_INTERNAL_ERROR
"  QSF request control block is not set up properly."
E_QS0018_BAD_PIECE_SIZE:SS5000B_INTERNAL_ERROR
"  Request for an illegal piece size.  Size must be a positive number of bytes."
E_QS0019_UNKNOWN_OBJ:SS5000B_INTERNAL_ERROR
"  No object with the supplied name and type exists in QSF's dynamic memory."
E_QS001A_NOT_STARTED:SS5000B_INTERNAL_ERROR
"  QSF has not been started for the server."
E_QS001B_NO_SESSION_CB:SS5000B_INTERNAL_ERROR
"  Could not find the QSF session."
E_QS001C_EXTRA_OBJECT:SS5000B_INTERNAL_ERROR
"  Created an additional query plan for this name."
E_QS001D_NOT_OWNER:SS5000B_INTERNAL_ERROR
"  Tried to release an object semaphore that you do not own."
E_QS001E_ORPHANED_OBJ:SS5000B_INTERNAL_ERROR
"  An orphaned %0c object was found and destroyed during QSF session exit."
E_QS001F_TOO_MANY_ORPHANS:SS5000B_INTERNAL_ERROR
"  Too many orphaned objects were found during QSF session exit."
E_QS0020_QS_INIT_MO_ERROR:SS5000B_INTERNAL_ERROR
"An error was encountered when qsr_startup() attempted to register the QSF
data structures with the Managed Objects facility."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This process has encountered an error when registering itself with the
**	Managed Objects facility.
**
** System Status:
**	This process will exit abnormally. Other processing may or may
**	not continue successfully, depending on the reason for the error.
**
** Recommendation:
**	Consult the error log for further information about the error.
**	If the error cannot be resolved, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_QS0500_INFO_REQ:SS5000B_INTERNAL_ERROR
"  QSF info request #%0d:  %1c"
E_QS9001_NOT_YET_AVAILABLE:SS5000B_INTERNAL_ERROR
"  This QSF function is not yet available."
E_QS9002_UNEXPECTED_EXCEPTION:SS5000B_INTERNAL_ERROR
"  Unexpected exception detected by QSF."
E_QS9999_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"  A QSF internal error has occurred."

NO_SQLSTATE_MAPPING
