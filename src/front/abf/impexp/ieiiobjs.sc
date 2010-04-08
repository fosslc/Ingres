/*
**	Copyright (c) 1993, 2004 Ingres Corporation
*/

# include       <compat.h>
# include	<si.h>
# include       <st.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include	<tm.h>
# include	<ug.h>
# include	<ooclass.h>
# include	<oocat.h>
# include	<abfdbcat.h>
# include	<cu.h>
# include	"erie.h"
# include       "ieimpexp.h"

/**
** Name:        ieiiobjs.sc -    Write the objrec structure to ii_objects
**
** Defines:
**
**      IIIEwio_WriteIIObjects()      - populate ii_objects catalog
**
** History:
**      18-aug-1993 (daver)
**		First Written.
**	02-oct-1993 (daver)
**		String 'now' works if datatype is date. Unfortunately,
**		ii_objects stores dates as strings, so 'now' is 'now'. Call 
**		a UG function to find out what time it is now, and use
**		that to populate the ii_objects.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Need cu.h to satisfy gcc 4.3.
*/

EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;


/*{
** Name: IIIEwio_WriteIIObjects		- populate ii_objects catalog
**
** Description:
**
** Write the objrec struct to ii_objects. No file i/o here;
** fp, inbuf still pointing to line with object on it. If performance
** becomes an issue, we *could* hold our catalog writes by storing
** representations in memory, until an entire object is read in. I doubt if
** the effort required will be worth it.
**
** Input:
**	IEOBJREC  *objrec	pointer to structure containing object info
**	i4        appid		application id object belongs to
**	char      *appowner	app. owner (object becomes owned by them)
**
** Output:
**	None
**
** Returns:
**	OK
**	Ingres status
**
** Side Effects:
**	Populates the catalogs (either updates or inserts) the object
**	described by the data in the objrec structure.
**
** History:
**	18-aug-1993 (daver)
**		First Written/Documented.
**	02-oct-1993 (daver)
**		String 'now' works if datatype is date. Unfortunately,
**		ii_objects stores dates as strings, so 'now' is 'now'. Call 
**		a UG function to find out what time it is now, and use
**		that to populate the ii_objects.
*/
STATUS
IIIEwio_WriteIIObjects(objrec, appid, appowner)
IEOBJREC	*objrec;
EXEC SQL BEGIN DECLARE SECTION;
i4		appid;	
char		*appowner;
EXEC SQL END DECLARE SECTION;
{
	i4	rectype;			/* good ol' record type! */

	EXEC SQL BEGIN DECLARE SECTION;
	char	*shortrem;
	char	*longrem;
	i4	objid;
	i4	objclass;
	char	*objname;
	char	now_date[TM_SIZE_STAMP+2];
	EXEC SQL END DECLARE SECTION;

	STATUS	rval;

	if (IEDebug)
		SIprintf("\nIn WriteIIObjects: %s  %d:%s; appid=%d\n\n",
		objrec->name, objrec->class,
			IICUrtnRectypeToName(objrec->class), appid);

	/* set up the char strings for updating */
	shortrem = objrec->short_remark;
	longrem = objrec->long_remark;
	objid = objrec->id;
	objclass = objrec->class;
	objname = objrec->name;

	STcopy(UGcat_now(),now_date);
	if (objrec->update)
	{
		/*
		** First update ii_objects with the info in objrec.
		** then update the longremarks. 
		**
		** Change the ownership of the object as well. This
		** will make ALL items owned by the central, czarist
		** application owner. IF we were to allow folks to 
		** specify the ownership of these objects, we'd set the
		** appowner parameter in the iiimport module, and allow the
		** new owner name to propagate itself around.
		*/
		EXEC SQL UPDATE ii_objects SET 
			object_owner = :appowner,
			short_remark = :shortrem,
			alter_date = :now_date,
			alter_count = alter_count + 1,
			last_altered_by = dbmsinfo('username')
		WHERE object_id = :objid
		AND object_name = :objname
		AND object_class = :objclass;

		rval = FEinqerr();
		if (rval != OK)
			return (rval);

		if (*longrem != EOS)
		{
			EXEC SQL UPDATE ii_longremarks SET
			long_remark = :longrem
			WHERE object_id = :objid;

			rval = FEinqerr();
			if (rval != OK)
				return (rval);
		}
	}
	else
	{
		/*
		** New objects are ALWAYS added as if the application
		** owner inserted them, which will allow for greater
		** consistancy (all objects owned by the same user id).
		*/
		EXEC SQL REPEAT INSERT INTO ii_objects 
			(object_id, object_class, object_name,
			 object_owner, object_env, is_current,
			 short_remark, object_language, create_date,
			 alter_date, alter_count, last_altered_by)
		VALUES  (:objid, :objclass, :objname,
			 :appowner, 0, 0,
			 :shortrem, 0, :now_date,
			 :now_date, 0, dbmsinfo('username'));
		
		rval = FEinqerr();
		if (rval != OK)
			return (rval);

		if (*longrem != EOS)
		{
			EXEC SQL REPEAT INSERT INTO ii_longremarks
				(object_id, remark_sequence,
				 remark_language, long_remark)
			VALUES	(:objid, 0,
				 0, :longrem);

			rval = FEinqerr();
			if (rval != OK)
				return (rval);
		}
	}
	return OK;
}
