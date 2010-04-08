/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<cm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
exec sql include <ooclass.sh>;
exec sql include <uigdata.sh>;
#include	<ilerror.h>
#include	"eram.h"

/**
** Name:	iamfadd.sc - fid add
**
** Description:
**	Get a new frame number from the database
**
** History:
**	Revision 6.2  88/08  kenl
**	Changed QUEL to SQL.
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	02/89  wong  Removed 'new_id()' and changed to call 'IIOOnewId()'.
**
**	Revision 6.0  86/08  bobm
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

/*{
** Name:	IIAMfaFidAdd() -
**
** Description:
**	Get a new unique frame identification number from the
**	database.
**
** Inputs:
**	fname	name of frame
**	frem	remarks to associate with frame
**	fown	owner of frame		( no longer used )
**
** Outputs:
**	fid	new number for use
**
**	return:
**		OK		success
**		ILE_FRDWR	ingres failure
**
** Side Effects:
**	updates ii_id table, ii_objects table
**
** History:
**	8/86 (rlm)	written
**	18-aug-88 (kenl)
**		Changed QUEL to SQL.
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
*/

#ifdef PCINGRES
GLOBALREF i4	Iamtrycnt;	/* retry query count */
#endif

STATUS
IIAMfaFidAdd(fname,frem,fown,fid)
char	*fname;
EXEC SQL BEGIN DECLARE SECTION;
char	*frem;
EXEC SQL END DECLARE SECTION;
char	*fown;
EXEC SQL BEGIN DECLARE SECTION;
OOID	*fid;
EXEC SQL END DECLARE SECTION;
{
        EXEC SQL BEGIN DECLARE SECTION;
                i4      	errchk;
                i4      	rows;
                char    	*fidnm;
                char    	*nowstr;
		UIDBDATA	*uidbdata;
        EXEC SQL END DECLARE SECTION;

# ifdef PCINGRES
	i4	iamerr1100();
	i4	(*oldprinterr)();
	i4	(*IIseterr())();	/* function returning "pointer to
					   function returning nat" */
# endif
	char	*iiam29fid_name();
	char	*UGcat_now();
	OOID	IIOOnewId();

	/*
	** new_id() handles locking to assure unique id's for simultaneous
	** processes.	Since the id is the key, we don't have to worry
	** about locking the ii_objects table append.
	*/

	if ( (*fid = IIOOnewId()) <= OC_UNDEFINED )
		return ILE_FRDWR;

	fidnm = iiam29fid_name(*fid);

	nowstr = UGcat_now();

#ifdef PCINGRES
	/* the following append seems to often generate error 1100 in backend so
	   catch here and try to silently recover by rerunning the query.  The
	   backend will probably have cleaned up enough to successfully run
	   the query this time */
	oldprinterr = IIseterr(iamerr1100);
	Iamtrycnt = 0;

    TRYAGAIN:

#endif

	uidbdata = IIUIdbdata();
	iiuicw1_CatWriteOn();
        EXEC SQL REPEATED INSERT INTO ii_objects
            (object_id, object_class, object_name, object_owner, object_env, 
			is_current, short_remark, object_language, create_date, 
			alter_date, alter_count, last_altered_by)
            VALUES (:*fid, :OC_ILCODE, :fidnm, :uidbdata->dba, 0, 0, :frem, 0,
             :nowstr, :nowstr, 0, '');
        EXEC SQL INQUIRE_INGRES (:errchk = errorno, :rows = rowcount);

	iiuicw0_CatWriteOff();

#ifdef PCINGRES
	/* if unable to append for whatever reason, delete the old
	   object and try to insert again */
	if (Iamtrycnt++ == 0 && rows == 0)
	{
		iiuicw1_CatWriteOn();
		EXEC SQL DELETE FROM ii_objects
				WHERE id = :*fid;
		iiuicw0_CatWriteOff();
		goto TRYAGAIN;
	}

	_VOID_ IIseterr(oldprinterr);
#endif

	if ( errchk != 0 || rows == 0 )
		return ILE_FRDWR;

	return OK;
}

/*
** 'iiam29fid_name()' generates a name to use when adding a new IL object to
** the database.  A new call wipes out the result from the old - use
** only as a temporary.	 'iiam35fid_unname()' decodes ID from name, and is
** thus intimately dependent on the format set by 'iiam29fid_name()'.
*/
static char	Fid_n[FE_MAXNAME+1] ZERO_FILL;

char *
iiam29fid_name ( id )
OOID	id;
{
	return STprintf(Fid_n, ERx("%d"), id);
}

OOID
iiam35fid_unname ( name )
char	*name;
{
	OOID	id;

	CVal( !CMdigit(name) ? name+3 : name, &id );
	return id;
}
