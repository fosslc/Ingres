/*
** Copyright (c) 2004 Ingres Corporation All rights reserved
*/

# ifndef		SA_INCLUDED
# define		SA_INCLUDED 1

/*
** Name: SACL.H - This file contains the external structures for SA
**
** Description:
**	SACL.H is designed to be included by SA.H in gl!hdr!hdr, it contains
**	all the structures and definitions needed to call the compatibility
**	library SA routines. These routines provide an interface for managing
**	operating system audit logs.
**
** History:
**	6-jan-94 (stephenb)
**	    Initial creation.
**	10-feb-94 (stephenb)
**	    Add more SA return status codes and update SA_AUD_REC in line with
**	    latest spec.
*/

/* SA return status codes */

# define	SA_OK			0
# define	SA_NOACCESS		(E_CL_MASK + E_SA_MASK + 0x01)
# define	SA_BADPARAM		(E_CL_MASK + E_SA_MASK + 0x02)
# define	SA_NOPRIV		(E_CL_MASK + E_SA_MASK + 0x03)
# define	SA_NOSUPP		(E_CL_MASK + E_SA_MASK + 0x04)
# define 	SA_NOOPEN		(E_CL_MASK + E_SA_MASK + 0x05)
# define	SA_NOWRITE		(E_CL_MASK + E_SA_MASK + 0x06)
# define	SA_NOMEM		(E_CL_MASK + E_SA_MASK + 0x07)
# define	SA_MFREE		(E_CL_MASK + E_SA_MASK + 0x08)
# define	SA_NOCLOSE		(E_CL_MASK + E_SA_MASK + 0x09)
# define	SA_NOFLUSH		(E_CL_MASK + E_SA_MASK + 0x0A)
# define	SA_NEWMEM		(E_CL_MASK + E_SA_MASK + 0x0B)

/* file access states */

# define	SA_WRITE		0x01

/*
** Name: SA_DESC - Structure used as a descriptor to decsribe an audit trail
**
** Description:
**	This structure is used to provide a unique handle to an open audit
**	trail, it is constructed by SAopen() and will be operating system
**	dependent, for example, on systems where the audit trail is written 
**	directly to a file it would be a file descriptor, and on systems where
**	an audit daemon does the work it may be a socket id. The user of
**	SA may not refference the contets of SA_DESC directly, it will be
**	constructed and maintained by SA and used in calls to SA to refference
**	a particular audit trail. The user should only hold a pointer to
**	it's location.
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation as a char string, just to allow mainline
**	    to compile.
*/
typedef struct _SA_DESC
{
    char	sa_desc[100];
} SA_DESC;

/*
** Name: SA_AUD_REC - structure containing a record to be audited
**
** Description:
**	This structure contains all the fields that make up an Ingres audit
**	record, if the record is to be written the the structure will be
**	filled out by the caller, otherwise the structure will be filled out
**	by SA, in any case enough memory to contain all the fields in the
**	structure should be allocated by the caller.
**
** History:
**	7-jan-94 (stephenb)
**	    Initial Creation.
**	10-feb-94 (stephenb)
**	    Updated in line with current spec.
*/

# define	SA_MAX_EVENTLEN		20
# define	SA_MAX_MESSTXT		80
# define	SA_MAX_PRIVLEN		32
# define	SA_MAX_ACCLEN		20
# define	SA_MAX_SESSIDLEN	16
# define	SA_MAX_TXTLEN		256

typedef struct _SA_AUD_REC
{
    SYSTIME	*sa_evtime;	/* Desc: Time the event occured
				** Length: As determined by SYSTIME
				** Null: YES
				*/
    char	*sa_eventtype;  /* Desc: The type of audit event
				** length: SA_MAX_EVENT, Null Terminated
				** Null: NO
				*/
    char	*sa_ruserid;     /* Desc: Real identity of the user
				** length: GL_MAXNAME, Blank Pad
				** Null: NO
				*/
    char	*sa_euserid;    /* Desc: Effective identity of the user
				** length: GL_MAXNAME, Blank Pad
				** Null: NO
				*/
    char	*sa_dbname;	/* Desc: Database action applies to
				** length GL_MAXNAME, Blank Pad
				** Null: YES
				*/
    char	*sa_messtxt;	/* Desc: Textual message
				** length: SA_MAX_MESSTXT, Null Terminated
				** Null: NO
				*/
    bool	sa_status;	/* Desc: Did operation succeed ?
				** Length: Single Byte, contains 'Y' or 'N'
				** Null: NO
				*/
    char	*sa_userpriv;	/* Desc: privilege list of the user
				** length: SA_MAX_PRIVLEN, fixed
				** Null: NO
				*/
    char	*sa_objpriv;	/* Desc: privileges changed by GRANT
				** length: SA_MAX_PRIVLEN, fixed
				** Null: NO
				*/
    char	*sa_accesstype;	/* Desc: access type of audit operation
				** length: SA_MAX_ACCLEN, Null Terminated
				** Null: NO
				*/
    char	*sa_objowner;	/* Desc: Owner of the object being accessed
				** length: GL_MAXNAME, Blank Pad
				** Null: YES
				*/
    char	*sa_objname;	/* Desc: Name of the object being accessed
				** length: GL_MAXNAME, Blank Pad
				** Null: YES
				*/
    char	*sa_detail_txt;	/* Desc: Additional text detail
				** length: SA_MAX_TXTLEN, Null Terminated
				** Null: YES
				*/
    i4		sa_detail_int;	/* Desc: Additional integer detail
				** length: 4 bytes
				** Null: YES (indicated by 0)
				*/
    SL_LABEL	*sa_sec_label;	/* Desc: Security label
				** length: as determined by SL_LABEL
				** Null: YES
				*/
    char	*sa_sess_id;	/* Desc: Unique session ID
				** Length: SA_MAX_SESSIDLEN, Null Terminated
				** Null: YES
				*/
} SA_AUD_REC;
# endif
