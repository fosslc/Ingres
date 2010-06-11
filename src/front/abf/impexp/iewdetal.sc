/*
**	Copyright (c) 1993, 2004 Ingres Corporation
*/

# include       <compat.h>
# include	<si.h>
# include       <st.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include	<er.h>
# include	<ooclass.h>
# include	<oocat.h>
# include	<abfdbcat.h>
# include	<cu.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<ug.h>
# include	<tm.h>
# include	"erie.h"
# include       "ieimpexp.h"
# include	<cv.h>


/**
** Name: iewdetal.sc
** 
** Defines:
**	IIIEwao_WriteIIAbfObjects()	populates ii_abfobjects (most
**					objects end up here)
**	IIIEwqn_WriteIIQbfNames()	populates qbfnames to ii_qbfnames
**	IIIEwjd_WriteIIJoinDefs()	populates joindefs ii_joindefs
**	IIIEwf_WriteIIForms()		populates forms ii_forms
**	IIIEwir_WriteIIReports()	populates report to ii_reports
**
** Description:
**	This family of routines process the detail line of an object in
** the intermediate file. The file is set up such that the first line
** (preceded by OC_XXX:) is written to ii_objects; the next line is written
** to whichever catalog is appropriate for the given object. Most objects
** end up in ii_abfobjects; other objects (qbfnames, joindefs, forms,
** reports) are written to their own catalogs instead. The catalog
** architecture is somewhat inconsistant and historical. Sigh.
**
** In general, these routines do no i/o per se; they just process the
** buffer passed in to them, which contains the current (detail) line in
** the file. They use cu_gettok() to get an attribute at a time; if its not
** a string, CVal or CVan is used to convert the attribute into the numeric
** type. Unfortunately, these CV routines aren't robust, don't contain
** status checks, and AV if you don't pass it a numeric. But that's what
** we're stuck with.
**
** Once we get the data from the buffer into local variables, we'll
** either update the existing object in the catalogs, or create a new one.
** The caller has already filled in the update field of the IEOBJREC record
** passed to us (objrec) as to whether the object should be updated or
** created.
**
** Keep in mind that the ubiquitous char *inbuf parameter gets hacked up by
** cu_gettok(); in breaking the thing up, it replaces the tabs in the line with
** nulls. But the caller never looks at the buffer again, so it really doesn't
** matter. Thought I should mention it in case dear maintenence person is
** looking at buffers and shme through the debugger.
** 
** History:
**	14-sep-1993 (daver)
**		First Documented.
**      02-oct-1993 (daver)
**              String 'now' works if datatype is date. Unfortunately,
**              ii_objects stores dates as strings, so 'now' is 'now'. Call
**              a UG function to find out what time it is now, and use
**              that to populate the ii_objects.
**	04-jan-1994 (daver)
**		Fix for bug 58031: mark a vision frame as regeneratable if it
**		does not have the custom bit set. Enables vision to "see"
**		changes to escape code.
**	21-jan-1994 (daver)
**		Turn cat writes back on after calling IIOOnewId().
**		Affects IIIEwao_WriteIIAbfObjects(). Fixes bug 58569.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;



/*{
** Name: IIIEwao_WriteIIAbfObjects	- populate ii_abfobjects
**
** Description:
**	Similar to the other routines in this file, this routine populates
** ii_abfobjects with the information in the input buffer inbuf. If the object
** in question is a record type (OC_RECORD), we call a special routine to
** process the record members, which immediate follow the record definition.
**
** Input:
**	IEOBJREC	*objrec		object in question
**	i4		appid		id of application object belongs to
**	char		*appowner	owner of application 
**	char		*inbuf		current line in file
**	FILE		*fp		file pointer to intermediate file 
**
** Output:
**	char		*inbuf		updated if the object was a record.
**	FILE		*fp		also updated for records.
**
** Returns:
**	Whatever the status of the query in question returns.
**
** History:
**	14-sep-1993 (daver)
**		First Documented.
**      02-oct-1993 (daver)
**              String 'now' works if datatype is date. Unfortunately,
**              ii_objects stores dates as strings, so 'now' is 'now'. Call
**              a UG function to find out what time it is now, and use
**              that to populate the ii_objects.
**	04-jan-1994 (daver)
**		Fix for bug 58031: mark a vision frame as regeneratable if it
**		does not have the custom bit set. Enables vision to "see"
**		changes to escape code.
**	21-jan-1994 (daver)
**		Turn cat writes back on after calling IIOOnewId().
**		Fixes bug 58569.
**	07-mar-1994 (daver)
**		Add "4GL procedure" to the list of objects we have to
**		create an ILCODE object for. Fixes bug 60025.
**       6-Jun-2010 (hanal04) Bug 122969
**              Imported object may change all manner of things such
**              the return types of callprocs etc. Set APC_RECOMPILE
**              for each object (and it's dependents) to force a recompilation
**              of the objects in the same application.
*/
STATUS
IIIEwao_WriteIIAbfObjects(objrec, appid, appowner, inbuf, fp)
IEOBJREC	*objrec;
EXEC SQL BEGIN DECLARE SECTION;
i4		appid;	
char		*appowner;
EXEC SQL END DECLARE SECTION;
char		*inbuf;
FILE		*fp;
{

	EXEC SQL BEGIN DECLARE SECTION;
	i4	objid;
	char	*abf_source;
	char	*abf_symbol;
	i4	retadf_type;
	char	*rettype;
	i4	retlength;
	i4	retscale;
	i4	abf_version;
	char	*abf_arg1;
	char	*abf_arg2;
	char	*abf_arg3;
	char	*abf_arg4;
	char	*abf_arg5;
	char	*abf_arg6;
	i4	abf_flags;
        i4      apc_recompile = APC_RECOMPILE; /* Do not mask in other values */
	EXEC SQL END DECLARE SECTION;

	STATUS 	rval;

	objid = objrec->id;
	
	if (IEDebug)
		SIprintf("\nIn WriteIIAbfObjects\t%s:%d(%s),appid=%d\n\n",
		objrec->name, objrec->class,
		IICUrtnRectypeToName(objrec->class), appid);

	abf_source = cu_gettok((char *)NULL);
	abf_symbol = cu_gettok((char *)NULL);
	CVan(cu_gettok((char *)NULL), &retadf_type); 
	rettype  = cu_gettok((char *)NULL);
	CVal(cu_gettok((char *)NULL), &retlength); 
	CVal(cu_gettok((char *)NULL), &retscale); 
	CVan(cu_gettok((char *)NULL), &abf_version); 
	abf_arg1 = cu_gettok((char *)NULL);
	abf_arg2 = cu_gettok((char *)NULL);
	abf_arg3 = cu_gettok((char *)NULL);
	abf_arg4 = cu_gettok((char *)NULL);
	abf_arg5 = cu_gettok((char *)NULL);
	abf_arg6 = cu_gettok((char *)NULL);
	CVal(cu_gettok((char *)NULL), &abf_flags);

	/*
	** if its NOT marked CUSTOM, AND its a Vision frame, set the state
	** bits to indicate that it needs regeneration. Also, mark the frame as
	** "just edited" time wise; that should ensure that the date on the 
	** frame is ahead of the source file.
	*/
	if ( ((abf_flags & CUSTOM_EDIT) == 0) &&
	     ((objrec->class == OC_BRWFRAME) || (objrec->class == OC_APPFRAME)
	   || (objrec->class == OC_MUFRAME) || (objrec->class == OC_UPDFRAME)))
	{
		char	buf[49];	/* temp buffer, holds abf_arg6 */
		i4	mfstate;	/* state of the meta frame */
		char	*p;		/* temp ptr */

		/* 
		** the state of the frame is a bit mask of stuff held in 
		** abf!hdr!metafrm.qsh. here's where the info to regenerate
		** or not to regenerate is. this state info is held in the
		** catalogs, in abf_arg6; however, in vision's internal
		** structures, this is "arg5" because the structure's flags
		** are numbered 0-5 where the catalog's are numbered 1-6. now,
		** the info in abf_arg6 looks like:
		** 	<state bit mask>@<date frame was generated>
		** so... we extract the existing bit mask, or the regenerate
		** bits into it, and then tack on the "@" plus the time of
		** 'now' to make sure the date entered is later than the date
		** of the file.
		*/
		mfstate = 0;
		STcopy(abf_arg6, buf);
		p = STindex(buf, "@", 0);
		if (p != NULL)
		{	
			*p = EOS;
			CVan(buf, &mfstate);
		}
		mfstate |= (MFST_NEWFR | MFST_DOGEN);
		if ((mfstate & MFST_VQEERR) != 0)
			mfstate |= MFST_DOFORM;
		STprintf(abf_arg6, "%d@%s", mfstate, UGcat_now());
	}

	/* 
	** at this point, we know if the object in question exists,
	** its in the object record. our caller has supplied us with a new
	** id if it hadn't existed already. 
	*/

	if (objrec->update)
	{
                abf_flags |= APC_RECOMPILE;

		EXEC SQL UPDATE ii_abfobjects SET
			abf_source = :abf_source, 
			abf_symbol = :abf_symbol,
			retadf_type = :retadf_type,
			rettype = :rettype,
			retlength = :retlength,
			retscale = :retscale,
			abf_version = :abf_version,
			abf_arg1 = :abf_arg1,
			abf_arg2 = :abf_arg2,
			abf_arg3 = :abf_arg3,
			abf_arg4 = :abf_arg4,
			abf_arg5 = :abf_arg5,
			abf_arg6 = :abf_arg6,
			abf_flags = :abf_flags
		WHERE object_id = :objid
		AND	applid = :appid;
		
		rval = FEinqerr();
		if (rval != OK)
			return rval;

                EXEC SQL UPDATE ii_abfobjects dep FROM ii_abfdependencies upd
                        SET abf_flags = dep.abf_flags + :apc_recompile
                        WHERE upd.object_id = :objid
                        AND dep.applid = upd.abfdef_applid
                        AND mod(dep.abf_flags / :apc_recompile, 2) = 0;

		rval = FEinqerr();
		if (rval != OK)
			return rval;
                
	}
	else
	{
		EXEC SQL REPEAT INSERT INTO ii_abfobjects
			(applid, object_id, abf_source, 
			abf_symbol, retadf_type, rettype,
			retlength, retscale, abf_version,
			abf_arg1, abf_arg2, abf_arg3, 
			abf_arg4, abf_arg5, abf_arg6, 
			abf_flags)
		VALUES (:appid, :objid, :abf_source, 
			:abf_symbol, :retadf_type, :rettype,
			:retlength, :retscale, :abf_version,
			:abf_arg1, :abf_arg2, :abf_arg3, 
			:abf_arg4, :abf_arg5, :abf_arg6, 
			:abf_flags) ;
		
		rval = FEinqerr();
		if (rval != OK)
			return rval;
		
		/*
		** if its a frame being added, we have to add an entry
		** for the ILCODE associated with the frame to the
		** ii_objects and ii_abfdependencies catalogs. get a new id
		** for the ILCODE object, then populate.
		** 
		** Have to add an ILCODE object for a 4GL procedure too.
		** Fixes bug 60025 (daver).
		*/
		if ( (objrec->class == OC_OSLPROC) || 
		      ( (objrec->class > OC_APPLFRAME) && 
		        (objrec->class < OC_AMAXFRAME) )  )

		{
			EXEC SQL BEGIN DECLARE SECTION;
			i4	il_id;
			char	il_name[FE_MAXNAME + 1];
			char	now_date[TM_SIZE_STAMP+2];
			EXEC SQL END DECLARE SECTION;

			il_id = IIOOnewId();
			CVla(il_id, il_name);

			/*
			** have to turn cat writes back on;
			** IIOOnewId() is "kind" enough to turn them off
			** for us. How special. Fixes Bug 58569.
			*/
			iiuicw1_CatWriteOn();

			STcopy(UGcat_now(),now_date);

                	/*
                	** New objects are ALWAYS added as if the application
                	** owner inserted them, which will allow for greater
                	** consistancy (all objects owned by the same user id).
                	*/
	                EXEC SQL INSERT INTO ii_objects
	                        (object_id, object_class, object_name,
	                         object_owner, object_env, is_current,
	                         short_remark, object_language, create_date,
	                         alter_date, alter_count, last_altered_by)
	                VALUES  (:il_id, 2010, :il_name,
	                         :appowner, 0, 0,
	                         '', 0, :now_date,
	                         :now_date, 0, dbmsinfo('username'));

			rval = FEinqerr();
			if (rval != OK)
				return rval;
	
			/* 
			** Use 2010 for OC_ILCODE, and 3502 for OC_DTDBREF
			*/
        		EXEC SQL INSERT INTO ii_abfdependencies
                		(object_id, abfdef_applid, abfdef_name,
                 	 	 abfdef_owner, object_class, abfdef_deptype,
                 		 abf_linkname, abf_deporder)
        		VALUES  (:objid, :appid, :il_name,
                		 :appowner, 2010, 3502, 
				 '', 0);

			rval = FEinqerr();
			if (rval != OK)
				return rval;

		} /* if its a new frame */

	} /* we're inserting, not updating */

	if (objrec->class == OC_RECORD) 
	{
		rval = IIIEdrr_DoRecordRecmem(objrec, appid, appowner, 
								inbuf, fp);
	}

	return rval;
}



/*{
** Name: IIIEwqn_WriteIIQbfNames	- populate ii_qbfnames
**
** Description:
**	Similar to the other routines in this file, this routine populates
** ii_qbfnames with the information in the input buffer inbuf. 
**
** NOTE: I have not been able to create a qbfname object in a copyapp file;
** that is, copyapp may have stopped writing qbfname objects some time ago.
** None of this code could therefore be tested. However, it is quite likely
** that there may be problems: the catalog, ii_qbfnames has an additional column
** in it (relowner), which is not reflected in the file format code. Just an FYI
** in case this comes up as a bug.
**
** Input:
**	IEOBJREC	*objrec		object in question
**	char		*inbuf		current line in file
**
** Output:
**	None
**
** Returns:
**	Whatever the status of the query in question returns.
**
** History:
**	14-sep-1993 (daver)
**		First Documented.
*/
STATUS
IIIEwqn_WriteIIQbfNames(objrec, inbuf)
IEOBJREC        *objrec;
char            *inbuf;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	object_id;
	char	*relname;
	char	*frname;
	i4	qbftype;
	EXEC SQL END DECLARE SECTION;

	object_id = objrec->id;

	if (IEDebug)
		SIprintf("\nIn IIIEwqn_WriteIIQbfNames   %s:%d, a %s\n\n",
		objrec->name, objrec->class,
		IICUrtnRectypeToName(objrec->class));

	relname = cu_gettok((char *)NULL);
	frname = cu_gettok((char *)NULL);
	CVan(cu_gettok((char *)NULL), &qbftype); 

	if (objrec->update)
	{
		EXEC SQL UPDATE ii_qbfnames SET
			relname = :relname,
			frname = :frname,
			qbftype = qbftype
		WHERE object_id = :object_id;
	}
	else
	{
		EXEC SQL INSERT INTO ii_qbfnames
			(object_id, relname, relowner, 
			 frname, qbftype)
		VALUES  (:object_id, :relname, dbmsinfo('username'),
			 :frname, :qbftype) ;
	}

	return(FEinqerr());
}



/*{
** Name: IIIEwjd_WriteIIJoinDefs	- populate ii_joindefs
**
** Description:
**	Similar to the other routines in this file, this routine populates
** ii_joindefs with the information in the input buffer inbuf. 
**
** However, dissimilar to other routines, and the basic model of frontend
** objects, is our dinosaur of dinosaurs, the joindef. Unlike other objects, who
** generally have an single "object" tuple in both ii_objects and ii_abfobjects,
** and detail tuples in other supporting tables, the ancient joindef is found in
** multiple rows, in (DBMS) unnormalized form, and in a seperate catalog,
** ii_joindefs. They are not well integrated with Vision in general.
** 
** Therefore, we must delete the multi-row representation of the current
** joindef, and read in the multiple rows in the file to populate the new
** joindef. At the end of this operation, the inbuf and fp parameters point to
** the line following the joindef (i.e, we stop reading in from the file once 
** we no longer see a joindef detail line, begging with a CUD_LINE (tab) 
** character).
**
** Input:
**	IEOBJREC	*objrec		object in question
**	char		*inbuf		current line in file
**	FILE		*fp		file pointer to intermediate file 
**
** Output:
**	char		*inbuf		contains the line following the joindef
**	FILE		*fp		also set to the line after the joindef
**
** Returns:
**	OK
**	ENDFILE			Joindef is last thing in the file
**	CUQUIT			Internal error occured.
**	Ingres errors		SQL good. Fire bad. 
**
** History:
**	14-sep-1993 (daver)
**		First Documented.
*/
STATUS
IIIEwjd_WriteIIJoinDefs(objrec, inbuf, fp)
IEOBJREC        *objrec;
char            *inbuf;
FILE		*fp;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	object_id;
	i4	qtype;
	char	*qinfo1;
	char	*qinfo2;
	char	*qinfo3;
	char	*qinfo4;
	char	*qinfo5;		/* going to need it eventually! */
	EXEC SQL END DECLARE SECTION;
	STATUS	rval;
	i4	rectype;

	object_id = objrec->id;

	/*
	** if a joindef exists, have to delete all rows
	** in ii_joindefs for this object...
	** so, go ahead and issue the delete.
	** Then, everthing becomes an insert. This is the 
	** approach copyapp took with everthing.
	*/
	if (objrec->update)
	{
		EXEC SQL DELETE FROM ii_joindefs
			 WHERE 	object_id = :object_id;

		rval = FEinqerr();
		if (rval != OK)
			return rval;
	}

	if (IEDebug)
		SIprintf("\nIn IIIEwjd_WriteIIJoinDefs\n%s:%d\n\n \n\n",
		objrec->name, objrec->class);

	/* initialy, we're looking at a detail line.*/
	rectype = CUD_LINE;

	while (rectype == CUD_LINE)
	{
		/* get the info to insert */
		CVal(cu_gettok((char *)NULL), &qtype); 
		qinfo1 = cu_gettok((char *)NULL);
		qinfo2 = cu_gettok((char *)NULL);
		qinfo3 = cu_gettok((char *)NULL);
		qinfo4 = cu_gettok((char *)NULL);

		/*
		** There's a bug in copyapp that will manifest itself here
		** eventually. Might as well tell you what I know now.
		**
		** First off, any bug in copyapp out will show up in IIEXPORT,
		** since it calls the same copyutil routines that copyapp
		** does. I re-wrote the stuff for IIIMPORT, so copyapp in bugs
		** won't affect IIIMPORT. But copyapp out is another story.
		** Here's the problem:
		**
		** Joindefs have/had an "unused" column, qinfo5. So its not 
		** written to copyapp files. Fine. But, for 6.5 joindefs, you 
		** can use owner.table to fully qualify a table reference. 
		** The owner of the talbe is stored in qinfo5. Terrific.
		** Anyway, this won't get written to a copyapp file, or an 
		** iiexport file until it gets fixed.
		**
		** Its also important to note that for some reason this column
		** is nullable, while the others are not, and use "" when no
		** value is present. So we'll need to use a null indicator in
		** the query.
		**
		** This routine will have to somehow know the version of 
		** the file. We could either pass this on straight through all 
		** the other callers, or set up some global.
		**
		** So, going to have to get qinfo5 eventually; the problem with
		** having the field nullable is that what would get written out
		** to the file? The current format would just have a tab (the
		** copyapp field delimiter). Anyway, the following line will
		** probably have to be added eventually:
		**
		**	qinfo5 = cu_gettok((char *)NULL);
		*/

		EXEC SQL INSERT INTO ii_joindefs
			(object_id, qtype, qinfo1, qinfo2,
			 qinfo3, qinfo4)
			/* will have to add qinfo5 = :qinfo5 */
		VALUES	(:object_id, :qtype, :qinfo1, :qinfo2,
			 :qinfo3, :qinfo4);
			/* will have to add qinfo5 = :qinfo5 */
		
		/*
		** A DBMS error occured. The caller will note it and 
		** mention something to the user.
		*/
		rval = FEinqerr();
		if (rval != OK)
			break;

		/*
		** This one's OK. If SIgetrec returns ENDFILE (or another
		** file related problem), pass it back up. In fact, EOF is fine
		** here, as joindefs are often the last thing in a int file
		*/
	    	rval = SIgetrec(inbuf, IE_LINEMAX, fp);
		if (rval != OK)
			break;
		
		rectype = cu_rectype(inbuf);

		/*
		** Have to prime the cu_gettok pump, so to speak. The first
		** time cu_gettok is called, its sets internal statics based on
		** the string passed. Only scan off the initial token (a leading
		** tab in the CUD_LINE case) if we're still reading Joindef
		** info; otherwise, we'll scan off the object token (OC_XXX)
		** of the application object which follows our JoinDef in the
		** file.
		*/
		if (rectype == CUD_LINE)
			cu_gettok(inbuf);	/* clear the rectype token */
	}

	/*
	** A DBMS error will be a positive integer, and will be printed
	** by the SQLPRINT command issued at the top of the file, and is 
	** handled by the caller. This impossible condition check makes 
	** sure we don't end up with another Component token before finding 
	** an object token. 
	**
	** If its the End Of File, that's ok; a joindef may very well be the 
	** last thing written out to the file.
	*/
	if ( (rectype < OC_OBJECT) && (rval != ENDFILE))
	{
		IIUGerr(E_IE003B_Joindef_Error, UG_ERR_ERROR, 1, 
				(PTR)objrec->name);
		return (CUQUIT);
	}

	return(rval);
}



/*{
** Name: IIIEwf_WriteIIForms	- populate ii_forms
**
** Description:
**	Similar to the other routines in this file, this routine populates
** ii_forms with the information in the input buffer inbuf. One caveat:
** when processing a form which exists, make sure that entries are deleted 
** in ii_encoded forms; otherwise, all this groovy new form information 
** shme will get copied in, but never see the light of day.
**
**
** Input:
**	IEOBJREC	*objrec		object in question
**	char		*inbuf		current line in file
**
** Output:
**	None
**
** Returns:
**	OK
**	Ingres errors
**
** History:
**	14-sep-1993 (daver)
**		First Documented.
*/
STATUS
IIIEwf_WriteIIForms(objrec, inbuf)
IEOBJREC        *objrec;
char            *inbuf;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	object_id;
	i4	frmaxcol;
	i4	frmaxlin;
	i4	frposx;
	i4	frposy;
	i4	frfldno;
	i4	frnsno;
	i4	frtrimno ;
	i4	frversion;
	i4	frscrtype;
	i4	frscrmaxx;
	i4	frscrmaxy;
	i4	frscrdpix;
	i4	frscrdpiy;
	i4	frflags;
	i4	fr2flags;
	i4	frtotflds;
	EXEC SQL END DECLARE SECTION;

	STATUS	status;

	object_id = objrec->id;

	CVan(cu_gettok((char *)NULL), &frmaxcol); 
	CVan(cu_gettok((char *)NULL), &frmaxlin); 
	CVan(cu_gettok((char *)NULL), &frposx); 
	CVan(cu_gettok((char *)NULL), &frposy); 
	CVan(cu_gettok((char *)NULL), &frfldno); 
	CVan(cu_gettok((char *)NULL), &frnsno); 
	CVan(cu_gettok((char *)NULL), &frtrimno); 
	CVan(cu_gettok((char *)NULL), &frversion); 
	CVan(cu_gettok((char *)NULL), &frscrtype); 
	CVan(cu_gettok((char *)NULL), &frscrmaxx); 
	CVan(cu_gettok((char *)NULL), &frscrmaxy); 
	CVan(cu_gettok((char *)NULL), &frscrdpix); 
	CVan(cu_gettok((char *)NULL), &frscrdpiy); 
	CVal(cu_gettok((char *)NULL), &frflags); 
	CVal(cu_gettok((char *)NULL), &fr2flags); 
	CVal(cu_gettok((char *)NULL), &frtotflds); 


	if (IEDebug)
		SIprintf("\nIn IIIEwf_WriteIIForms     %s:%d, a %s\n\n",
		objrec->name, objrec->class, 
		IICUrtnRectypeToName(objrec->class));


	if (objrec->update)
	{
		EXEC SQL REPEAT UPDATE ii_forms SET
			frmaxcol = :frmaxcol,
			frmaxlin = :frmaxlin,
			frposx = :frposx,
			frposy = :frposy,
			frfldno = :frfldno,
			frnsno = :frnsno,
			frtrimno  = :frtrimno ,
			frversion = :frversion,
			frscrtype = :frscrtype,
			frscrmaxx = :frscrmaxx,
			frscrmaxy = :frscrmaxy,
			frscrdpix = :frscrdpix,
			frscrdpiy = :frscrdpiy,
			frflags = :frflags,
			fr2flags = :fr2flags,
			frtotflds = :frtotflds
		WHERE object_id = :object_id;

		status = FEinqerr();
		if (status != OK)
			return status;
		
		EXEC SQL DELETE 
		FROM ii_encoded_forms
		WHERE object_id = :object_id;
		
	}
	else
	{
		EXEC SQL INSERT INTO ii_forms
			(object_id, frmaxcol, frmaxlin, frposx, frposy,
			 frfldno, frnsno, frtrimno , frversion, frscrtype,
			 frscrmaxx, frscrmaxy, frscrdpix, frscrdpiy,
			 frflags, fr2flags, frtotflds)
		VALUES
			(:object_id, :frmaxcol, :frmaxlin, :frposx, :frposy,
			 :frfldno, :frnsno, :frtrimno , :frversion, :frscrtype,
			 :frscrmaxx, :frscrmaxy, :frscrdpix, :frscrdpiy,
			 :frflags, :fr2flags, :frtotflds) ;

	}
	return(FEinqerr());
}




/*{
** Name: IIIEwir_WriteIIReports		- populate ii_reports
**
** Description:
**	Similar to the other routines in this file, this routine populates
** ii_reports with the information in the input buffer inbuf. 
**
** Input:
**	IEOBJREC	*objrec		object in question
**	char		*inbuf		current line in file
**
** Output:
**	None
**
** Returns:
**	OK
**	Ingres errors
**
** History:
**	14-sep-1993 (daver)
**		First Documented.
*/
STATUS
IIIEwir_WriteIIReports(objrec, inbuf)
IEOBJREC	*objrec;
char		*inbuf;
{

	EXEC SQL BEGIN DECLARE SECTION;
	i4	object_id;
	char	*reptype;
	i4	repacount;
	i4	repscount;
	i4	repqcount;
	EXEC SQL END DECLARE SECTION;

	/*
	** NOTE:
	** One column in ii_reports, repbcount, does not get written to the 
	** copyapp file. This is the number of "br"'s that occur. Its 
	** unnormalized data for some reason or another; seems like we may
	** need to count these things if it really matters.
	**
	** However, the documentation claims this is for internal consistancy
	** only, and report writer might update this number itself, when
	** processing a report. So, set it to 0 for now.
	**
	** If it is indeed vital, we'll add another processing step in the
	** ii_rcommands segment to add up the .br's, and do an update to 
	** iireports' repbcount column.
	*/

	object_id = objrec->id;

	reptype = cu_gettok((char *)NULL);
	CVan(cu_gettok((char *)NULL), &repacount); 
	CVan(cu_gettok((char *)NULL), &repscount); 
	CVan(cu_gettok((char *)NULL), &repqcount); 

	if (IEDebug)
		SIprintf("\nIn IIIEwir_WriteIIReports      %s:%d, a %s\n\n",
		objrec->name, objrec->class, 
		IICUrtnRectypeToName(objrec->class));

	if (objrec->update)
	{
		EXEC SQL UPDATE ii_reports SET
			reptype = :reptype,
			repacount = :repacount,
			repscount = :repscount,
			repqcount = :repqcount,
			repbcount = 0
		WHERE object_id = :object_id ;
	}
	else
	{
		EXEC SQL INSERT INTO ii_reports 
			(object_id, reptype, repacount, 
			 repscount, repqcount, repbcount)
		VALUES  (:object_id, :reptype, :repacount,
			 :repscount, :repqcount, 0) ;

	}
	return(FEinqerr());
}
