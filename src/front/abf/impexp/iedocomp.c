/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

# include       <compat.h>
# include	<si.h>
# include       <st.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include	<ooclass.h>
# include	<oocat.h>
# include	<abfdbcat.h>
# include	<cu.h>
# include	<ug.h>
# include	"erie.h"
# include       "ieimpexp.h"

/**
** Name:        iedocomp.c  	- Process component records.
**
** Defines:
**	IIEdcr_DoComponentRecs() - process the records!
**
** History:
**      18-aug-1992 (daver)
**		First Written.
**	14-sep-1993 (daver)
**		Enhanced error handling, erie.msg messages, etc.
**	22-dec-1993 (daver)
**		Fixed bug 58196: "expect" EOF in the case of a blank form
**		as the last component in an iiexport file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name: IIEdcr_DoComponentRecs()	- process component record
**
** Description:
**
** This routine processes all the components of a given object. Given
** the object information, application id, file information, this routine
** will read all the components of our current record, and write the
** information to the appropriate system catalogs. When we 
** finish, the file pointer information (fp and inbuf) point to the 
** line AFTER the last component; that is, the NEXT object to be 
** processed, or EOF.
**
** In the beginning, fp and inbuf point to the detail record of our object.
** We first read in the next line. MOST objects have at least one component
** record, even if its only just a DUMMY record for a CUC_AODEPEND 
** (a dummy dependency record). (The exceptions are JoinDefs and RECMEMS,
** to be dealt with later). Unlike the object record, the component
** record's structure is like this:
**
**	NAME_OF_COMPONENT:
**	\t<data>\t<data>\t<data>...
**
** which means that the only thing on this first line will be a component
** "title" record. That's our "outer" loop. The "inner" loop reads the
** file, and calls individual write routines to populate the data to the
** specific component catalog.
**
** Psuedocode:
**	+ Read a "Component Title" Record
**	+ Figure out if its a Component (could be a new object, like a
**		RECMEM or JoinDef, for instance)
** 	+ WHILE the record we have is a Component Title Record:       (outer)
** 
** 		- If the object is being updated, Delete the old 
** 			component entries; its a 1:N relationship,
** 			so thats what we gotta do.
** 		- Read a Component detail record (saving the component type)
**
** 		- WHILE the record is a Component detail record:      (inner)
** 			= Write to the appropriate component catalog
** 			= Read another record
** 		- ENDWHILE                                            (inner)
** 
** 	+ENDWHILE                                                    (outer)
** 
** When we fall out of the inner loop, we're pointing to the record AFTER
** our current component definition. Conveniently, we flip to the top of
** the loop: if its another component, stay where we are, and keep
** processing all the components for this object. If its not
** another component, it'll be a new object or EOF, and we're outta here.
** 
** Input:
**	IEOBJREC        *objrec		- parent object of these components
**				->id	- object id for this component
**	i4              appid		- application id 
**	char		*appowner	- application owner
**	char            *inbuf		- current line of the file
**	FILE            *fp		- current place in file
**
** Output:
**	char		*inbuf		the next line of the file, read in
**	FILE		*fp		updated pointer in the file
**
** Returns:
**	OK		More objects to process.
**	CUQUIT		Big time Error. Issue that "Processing Stops" message.
**	ENDFILE		EOF hit. Its ok. Time to leave. We're done.
**
** Side Effects:
**	Sends queries to the system catalogs which reflect the component
**	information read from the intermediate file.
**
** History:
**	18-aug-1993 (daver)
**		First Written/Documented.
**	14-sep-1993 (daver)
**		Enhanced error handling, erie.msg messages, etc.
**	22-dec-1993 (daver)
**		Fixed bug 58196: "expect" EOF in the case of a blank form
**		as the last component in an iiexport file.
*/
STATUS
IIIEdcr_DoComponentRecs(objrec, appid, appowner, inbuf, fp)
IEOBJREC	*objrec;
i4		appid;	
char		*appowner;
char		*inbuf;
FILE		*fp;
{
	i4	component_type;			/* component in question */
	i4	rectype;			/* good ol' record type! */
	STATUS	fstatus = OK;

	if (IEDebug)
		SIprintf("\nIn IIIEdcr_DoComponentRec\n%s:%d:appid=%d\n\n",
		objrec->name, objrec->class,appid);


	/*
	** If int files get created that don't have component lines for an
	** object, and that object is the last thing to get written, this
	** EOF check might get triggered accidentally. If so, this is
	** probably the culprit; just make sure the SIgetrec status is
	** ENDFILE, and then return that. Everything will be a-ok.
	**
	** But the objects that exist now that don't have components 
	** can't get here; the caller if's them around this
	** routine. Probably want to just add that object type to that
	** if construct. Just an FYI, dear software maintainer person. 
	*/
	if (SIgetrec(inbuf, IE_LINEMAX, fp) != OK)
	{
		IIUGerr(E_IE0026_Unexpected_EOF, UG_ERR_ERROR, 1, 
						(PTR)objrec->name);
		return (CUQUIT);
	}

	/* this is the only thing on this row... the component "title" */
	component_type = cu_rectype(inbuf);

	while ( (fstatus == OK) && (component_type < OC_OBJECT) )
	{

		/* OUTER LOOP.
		** ok, gang, this is it. the next SIgetrec will be detail
		** records to populate the given catalog.
		**
		** either way, the algorithm goes as follows:
		** 
		** we have to delete the old component. so call a delete
		** routine with the name and the type. It will switch on
		** type and delete from the correct catalog, e.g:
		**	delete from xxx where blah = blah.
		** then read another record, saving the components rec.
		** we have the detail line in inbuf. then call the appropriate
		** routine to write the individual row. 
		*/
		if (objrec->update)
		{
			fstatus = IIIEdoc_DeleteOldComponent(objrec, appid, 
							component_type);
			/*
			** if there's an Ingres error, its been printed
			** in the above ESQL/C routine, and we should bail!
			*/
			if (fstatus != OK)
				return (CUQUIT);
		}

		/*
		** read a new record; check file status.
		** return the status if its not OK. EOF in this case
		** is OK, since the last component might be something
		** like a blank form which doesn't have any fields or 
		** trim (as noted in bug 58196). the caller will raise
		** an error if its anything but EOF.
		*/
		fstatus = SIgetrec(inbuf, IE_LINEMAX, fp);
		if (fstatus != OK)
		{
			return (fstatus);
		}

		/* we expect this to be a component line */
		rectype = cu_rectype(inbuf);

		while ((fstatus == OK) && (rectype == CUD_LINE))
		{
			/* prime cu_gettok; clear leading \t */
			cu_gettok(inbuf);

			/* call the routine to populate the right catalog */
			switch (component_type)
			{
			  case CUC_AODEPEND:
			  case CUC_ADEPEND:
				fstatus = IIIEwad_WriteAbfDepend(inbuf, objrec, 
							    appid, appowner);
				break;
	
			  case CUC_FIELD:
				fstatus = IIIEwfc_WriteField(inbuf, objrec->id);
				break;
	
			  case CUC_TRIM:
				fstatus = IIIEwtc_WriteTrim(inbuf, objrec->id);
				break;
	
			  case CUC_RCOMMANDS:
				fstatus = IIIEwrc_WriteRCommands(inbuf, 
								objrec->id);
				break;
	
			  case CUC_VQJOIN:
				fstatus = IIIEwvj_WriteVQJoins(inbuf, 
								objrec->id);
				break;
	
			  case CUC_VQTAB:
				fstatus = IIIEwvt_WriteVQTabs(inbuf, 
								objrec->id);
				break;
	
			  case CUC_VQTCOL:
				fstatus = IIIEwvc_WriteVQCols(inbuf, 
								objrec->id);
				break;
	
			  case CUC_MENUARG:
				/* 
				** If we're copying application BRANCHES, we'll
				** want to modify this. For now, only write the
				** menuargs stuff if we're updating. No point 
				** in specifying parameters for non-existant 
				** child frames now. 
				*/
				if (objrec->update)
				    fstatus = IIIEwma_WriteMenuArgs(inbuf, 
								objrec->id);
				break;
			
			  case CUC_FRMVAR:
				fstatus = IIIEwfv_WriteFrameVars(inbuf, 
								objrec->id);
				break;
	
			  case CUC_ESCAPE:
				fstatus = IIIEwec_WriteEscape(inbuf, 
								objrec->id);
				break;
			
			  default:
				IIUGerr(E_IE0029_Unknown_Component,
					UG_ERR_ERROR, 2, (PTR)objrec->name,
					(PTR)component_type);
				return (CUQUIT);

			} /* end switch */

			/* make sure fstatus is ok from dbms operation ... */
			if (fstatus != OK)
			{
				if (IEDebug)
					SIprintf("fstatus is %d\n", fstatus);
				
				/* dbms errs are not EOF or internal (CUQUIT)*/
				if ( (fstatus != ENDFILE) && 
				     (fstatus != CUQUIT) )
				{
					IIUGerr(E_IE003A_DBMS_Error, 
						UG_ERR_ERROR, 0);
					return (CUQUIT);
				}
				return (fstatus);
			}

			/*
			** This last SIgetrec read is perfectly fine not
			** to return OK. This is where we *expect* us to
			** hit EOF: at the end of the component section.
			** Now, it might not be, but if it is, just break
			** outta the loop. We'll return ENDFILE, and all
			** will be well.
			*/
		    	fstatus = SIgetrec(inbuf, IE_LINEMAX, fp);
			if (fstatus != OK)
				break;

			rectype = cu_rectype(inbuf);

		} /* end inner loop reading ea component line */

		/* *might* be the only thing on this row; a component "title" */
		component_type = cu_rectype(inbuf);

		/*
		** when we fall out of the inner loop its 'cause we read a 
		** title of some sort. if its another component, start up 
		** the loop again. If its a new object, exit this routine 
		** and keep processing. We check at the top of the loop.
		*/

	} /* end outer loop */

	/* now we're either pointing to a new object, or EOF */
	return (fstatus);
}
