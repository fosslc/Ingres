/*
**	Copyright (c) 1993, 2004 Ingres Corporation	
*/

# include       <compat.h>
# include       <si.h>
# include       <st.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <ooclass.h>
# include       <oocat.h>
# include       <abfdbcat.h>
# include       <cu.h>
# include	<ug.h>
# include	"erie.h"
# include       "ieimpexp.h"
# include	<cv.h>


/**
** Name: ierecord.sc 		- process a record structure
** 
** Defines:
**	IIIEdrr_DoRecordRecmem()	- process the record types
**	IIIEwac_WriteIIAbfClasses()	- populate ii_abfclasses
**
** History:
**	15-sep-1993 (daver)
**		First Documented. Cleaned and ERmsg-ized.
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
** Name:        IIIEdrr_DoRecordRecmem() - process the record types
**
** Description:
**
** We're processing a record. The caller has just written the appid 
** and record object id to ii_abfobjects catalog. Now comes the fun part.
** We now have to read out of the file for the OC_RECMEMs and the
** inevitable CUC_AODEPEND records. The format of a record type looks
** like this in the intermediate file:
**
** 	OC_RECORD:	<ii_objects info for that record>
** 		<ii_abfobjects info for that record>
** 	OC_RECMEM:	<ii_objects info for that record member>
** 		<ii_abfclasses info for the record, record member pair>
** 
** 	<more OC_RECMEM definitions>
** 
** 	CUC_AODEPEND:
** 		<either a DUMMY entry, or a real dependency i.e.,
** 		records with other records as elements>
** 
** A dependency record (CUC_AODEPEND) always ends the definition.
** Each OC_RECMEM must be added to ii_objects and ii_abfclasses
** (and not ii_abfobjects). So, the psuedocode goes like this:
**
** Read a record from the file 
** WHILE still a recmem
** 	write the recmem header to ii_objects
** 	read the recmem detail from the file
** 	write the recmem detail to ii_abfclasses
** 	read another record
** ENDLOOP
**
** (( the record that falls out of the loop should be ))
** (( the dependency record for the parent OC_RECORD ))
**
** IF that record isn't a CUC_AODEPEND or CUC_ADEPEND
** 	return ok, there's no dependencies, we're outta here
** If we're updating that record
**	Delete the existing dependencies
** (( now we're going to populate the record's dependencies ))
** Read the next record
** WHILE the record is a CUD_LINE
** 	write the dependency to ii_abfdependencies
** 	read the next record
** ENDLOOP
**
** One issue that crops up is what happends if we hit EOF while we're reading
** things in. The rule of thumb followed is if EOF occurs in the middle of 
** a definition (i.e, after a title record has been read in), its an error;
** however, if we hit EOF when we'd normally find a "dummy" dependency record,
** its not an error. 
**
** Inputs:
**	IEOBJREC        *objrec		object info for RECORD being processed
**	i4              appid		application id record belongs to
**	char            *appowner	owner of said application
**	char            *inbuf		ptr to buffer (holds 1 line from file)
**	FILE            *fp		file ptr to intermediate file
**     
** Outputs:
**	char            *inbuf		next line in file after record def
**	FILE            *fp		file ptr to above file
**	
** History:
** 	15-sep-1993 (daver)
**		First Documented, cleansed, er-msgized, etc.
**	26-jan-1994 (daver)
**		If we're updating the record structure, make sure the
**		existing dependenies are deleted before more are inserted.
**		Fixes bug 58570.
*/
STATUS
IIIEdrr_DoRecordRecmem(objrec, appid, appowner, inbuf, fp)
IEOBJREC	*objrec;
EXEC SQL BEGIN DECLARE SECTION;
i4              appid;
char            *appowner;
EXEC SQL END DECLARE SECTION;
char            *inbuf;
FILE            *fp;
{
	IEOBJREC	recmem;
	i4             rectype;
	STATUS		rval;

	/* Read a record member's object record from the file */
    	rval = SIgetrec(inbuf, IE_LINEMAX, fp);
	if (rval != OK)
	{
		/*
		** EOF is unnacceptable here. It means there was a record
		** definition without any record members defined for it. 
		** This is an error!
		*/
		IIUGerr(E_IE0026_Unexpected_EOF, UG_ERR_ERROR, 1,
							(PTR)objrec->name);
		return CUQUIT;
	}

	rectype = cu_rectype(inbuf);

	/* while still a recmem */
	while (rectype == OC_RECMEM)
	{
		/* write the recmem header to ii_objects */

		/* load the info into the recmem structure */
		IIIElos_LoadObjrecStruct(&recmem, inbuf);
		/*
		** find out if the item exists already
		** FALSE is passed to indicate we're NOT using 
		** -check flag; this will also get us a new id if 
		** one does not exist 
		*/
		if (IIIEfoi_FindObjectId(&recmem, FALSE, appid, 
							objrec->id) != OK)
		{
			/* error message printed inside IIIEfoi_FindObjectId */
			return IIIEbe_Bad_Exit(fp);
		}

		/*
		** DBMS errors get passed back up as simply != OK.
		** They'll get printed with the SQLPRINT command, and then
		** flagged and noted in IIIEdod_DoObjectDetail()
		*/
                rval = IIIEwio_WriteIIObjects(&recmem, appid, appowner);
		if (rval != OK)
			return rval;
	
		/* read the recmem detail from the file  */
    		rval = SIgetrec(inbuf, IE_LINEMAX, fp);
		if (rval != OK)
		{
			/*
			** EOF is also unnacceptable here. It means there was 
			** an incomplete record memeber definition (just the
			** header, without any detail info).
			** This is an error!
			*/
			IIUGerr(E_IE0026_Unexpected_EOF, UG_ERR_ERROR, 1,
							(PTR)objrec->name);
			return CUQUIT;
		}

		/* write the recmem detail to ii_abfclasses */
		rval = IIIEwac_WriteIIAbfClasses(&recmem, objrec->id, 
							appid, inbuf);
		/* again, DBMS error are passed back up */
		if (rval != OK)
		{
			if (IEDebug) SIprintf("rval is %d\n");
			return rval;
		}
		

		/*
		** Read another record -- this time, EOF *is* sorta
		** acceptable. It would show up at the end of a record member's
		** definition. Fair enough. Previous versions always threw in a
		** dummy record to avoid an outer join with ii_abfdependencies,
		** but I'm really not 100% sure that tuple is necessary. So 
		** lets let the file end here, if it does.
		*/
    		rval = SIgetrec(inbuf, IE_LINEMAX, fp);
		if (rval != OK)
			return rval;	/* could be ENDFILE */
				
		/*
		** at this stage, we've read the RECMEM's for this RECORD. Now 
		** we have to read the dependency record, which consists
		** of one or more rows starting with CUD_LINE
		*/
		rectype = cu_rectype(inbuf);
	}

	/*
	** No dependency record was written. If the RECMEMs for this
	** RECORD are all of native ingres types, there's really no 
	** reason for dependencies to exist, anyway. Dependencies would exist 
	** if a given record member were of a record type itself (and
	** therefore *dependent* on that record type definition).
	*/
	if (rectype != CUC_AODEPEND)
		return OK;

	/*
	** This is *not* OK. This means that the title CUC_AODEPEND:
	** was written, but the actual dependency wasn't. Someone's been
	** screwing around with the file, and we'll have none of it.
	*/
    	rval = SIgetrec(inbuf, IE_LINEMAX, fp);
	if (rval != OK)
	{
		/*
		** EOF is unnacceptable here. It means there was a dependency
		** definition without any record members defined for it. 
		** (its gotta have at least that "DUMMY" line).
		** This is an error!
		*/
		IIUGerr(E_IE0026_Unexpected_EOF, UG_ERR_ERROR, 1,
							(PTR)objrec->name);
		return CUQUIT;
	}


	/* 
	** now we need to delete the exsiting dependencies of the record
	** structure. all the other objects have their depenencies deleted
	** in IIIEdcr_DoComponentRecs(); records don't since they're an
	** exception to the copyapp model. deleting the dependencies prevents
	** duplicate rows from being inserted, fixing bug 58570
	*/
	if (objrec->update)
	{
		rval = IIIEdoc_DeleteOldComponent(objrec, appid,
							(i4)CUC_AODEPEND);
		/* bail if there's an ingres error */
		if (rval != OK)
			return (CUQUIT);
	}
		

	rectype = cu_rectype(inbuf);

	while (rectype == CUD_LINE)
	{
		cu_gettok(inbuf);	/* prime cu_gettok */

		/* write the dependency to ii_abfdependencies */
		rval = IIIEwad_WriteAbfDepend(inbuf, objrec, 
						appid, appowner);

		/* DBMS errors handled above */
		if (rval != OK)
			return rval;
		
		/*
		** read another record -- this is totally acceptable
		** to be EOF, 'cause its the end of the definiton! 
		*/
    		rval = SIgetrec(inbuf, IE_LINEMAX, fp);
		if (rval != OK)
			return rval;	/* could be ENDFILE */

               	rectype = cu_rectype(inbuf);

	} /* end while doing dependencies */

	return OK;
} 





/*{
** Name: IIIEwac_WriteIIAbfClasses		- populate ii_abfclasses
**
** Description:
**	This routine write the info for a record member into the
** ii_abfclasses catalog. This is another unnormalized (in the Structured 
** Design sence) table. Here, we have the record member info in objrec, 
** and also need the parent record object's id as well (which we pass in as
** recid). All of it goes into the catalog.
**
** This routine is analogous with the other detail writing routines in
** iewdetal.sc, but because of the strange requirements of record and record
** memeber processing, it lives here.
**
** Input:
**	IEOBJREC        *objrec		ptr to record member object 
**					(record ELEMENT, *not* parent record)
**
**	i4              recid		object id of PARENT RECORD object
**
**	i4              appid		application id this belongs to
**
**	char            *inbuf		current line in file
**
** Output:
**	None
**
** Returns:
**	Whatever the status of the query in question returns.
**
** History:
**	15-sep-1993 (daver)
**		First Documented.
*/
STATUS
IIIEwac_WriteIIAbfClasses(objrec, recid, appid, inbuf)
IEOBJREC	*objrec;
EXEC SQL BEGIN DECLARE SECTION;
i4		recid;
i4		appid;
EXEC SQL END DECLARE SECTION;
char		*inbuf;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	object_id;
	i4	class_id;
	i4	class_order;
	i4	adf_type;
	char	*type_name;
	i4	adf_length;
	i4	adf_scale;
	EXEC SQL END DECLARE SECTION;

	object_id = objrec->id;


	if (IEDebug)
		SIprintf("\nIn WriteIIAbfClasses:\t%s:%d(%s), rid %d\n\n",
		objrec->name, objrec->class, 
		IICUrtnRectypeToName(objrec->class), recid);

	cu_gettok(inbuf);		/* clear off the opening tab */

	CVan(cu_gettok((char *)NULL), &class_order); 
	CVal(cu_gettok((char *)NULL), &adf_type); 
	type_name = cu_gettok((char *)NULL);
	CVal(cu_gettok((char *)NULL), &adf_length); 
	CVal(cu_gettok((char *)NULL), &adf_scale); 

	if (objrec->update)
	{
		EXEC SQL UPDATE ii_abfclasses SET
			class_order = :class_order,
			adf_type = :adf_type,
			type_name = :type_name,
			adf_length = :adf_length,
			adf_scale = :adf_scale
		WHERE appl_id = :appid
		AND   class_id = :recid
		AND   catt_id = :object_id;

	}
	else
	{
		EXEC SQL INSERT INTO ii_abfclasses
			(appl_id, catt_id, class_id, class_order,
			 adf_type, type_name, adf_length,
			 adf_scale) 
		VALUES	(:appid, :object_id, :recid, :class_order,
			 :adf_type, :type_name, :adf_length,
			 :adf_scale) ;
	}

	return(FEinqerr());
}
