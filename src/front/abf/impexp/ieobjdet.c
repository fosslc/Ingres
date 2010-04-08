/*
**	Copyright (c) 1993, 2004 Ingres Corporation
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
** Name:        ieobjdet.c -	Process an object's detail line
**
** Defines:
**
**      IIIEdod_DoObjectDetail()     process a detail line
**
** History:
**      18-aug-1993 (daver)
**		First Written.
**	14-sep-1993 (daver)
**		Cleaned up, documented, messages into erie.msg, etc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name: IIIEdod_DoObjectDetail		- process an object's detail line
**
** Description:
**
** At this stage, we've written the object record in objrec.
** We need to read the detail record for that object and write to 
** whatever catalog is appropriate (e.g., ii_abfobjects, ii_forms, etc). 
** Then we write the detail line to that catalog by calling the specific
** routine to do so (via a switch statement).
**
** Input:
**	IEOBJREC        *objrec		- pointer to struct w/object info
**	i4              appid		- application id
**	char            *appowner	- application owner
**	char            *inbuf		- current line in file
**	FILE            *fp		- file pointer
**
** Output:
**	char		*inbuf		- the next line in the file
**	FILE		*fp		- points to next line in file
**
** Returns:
**	OK			a-ok
**	CUQUIT			something went wrong
**
** Side Effects:
**	Populates catalogs with info from line in file
**
** History:
**	18-aug-1993 (daver)
**		First Written/Documented.
**	14-sep-1993 (daver)
**		Cleaned up, documented, messages into erie.msg, etc.
*/
STATUS
IIIEdod_DoObjectDetail(objrec, appid, appowner, inbuf, fp)
IEOBJREC	*objrec;
i4		appid;	
char		*appowner;
char		*inbuf;
FILE		*fp;
{
	i4	rectype;
	STATUS	filestatus;

	/*
	** Get next record 
	** If its not a detail line, issue an error message and exit.
	*/
        if (SIgetrec(inbuf, IE_LINEMAX, fp) != OK)
	{
		IIUGerr(E_IE0026_Unexpected_EOF, UG_ERR_ERROR, 1, 
				(PTR)objrec->name);
                return (CUQUIT);
	}

        rectype = cu_rectype(inbuf);

        if (rectype != CUD_LINE)
        {
		IIUGerr(E_IE0027_Not_Data_Line, UG_ERR_ERROR, 2, 
				(PTR)objrec->name, (PTR)rectype);
                return(CUQUIT);
        }

	cu_gettok(inbuf);	/* scan opening tab */

	switch (objrec->class)
	{
	  case OC_HLPROC:	
	  case OC_OSLPROC:	
	  case OC_DBPROC:	
	  case OC_OSLFRAME:
	  case OC_RWFRAME:
	  case OC_QBFFRAME:
	  case OC_UNFRAME:
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_BRWFRAME:
	  case OC_UPDFRAME:
	  case OC_RECORD:
	  case OC_GLOBAL:
	  case OC_CONST:
	  case OC_AFORMREF:
	  case OC_GRFRAME:
	  case OC_GBFFRAME:
		/* OC_RECMEM also gets processed here, as part of OC_RECORD */
		filestatus = IIIEwao_WriteIIAbfObjects(objrec, appid, 
							appowner, inbuf, fp);
		break;
		
	  case OC_QBFNAME:	
		filestatus = IIIEwqn_WriteIIQbfNames(objrec, inbuf);
		break;

	  case OC_JOINDEF:
		filestatus = IIIEwjd_WriteIIJoinDefs(objrec, inbuf, fp);
		break;

	  case OC_FORM:
		filestatus = IIIEwf_WriteIIForms(objrec, inbuf);
		break;
		
	  case OC_RBFREP:
	  case OC_REPORT:
	  case OC_RWREP:
		filestatus = IIIEwir_WriteIIReports(objrec, inbuf);
		break;

	  default:
		IIUGerr(E_IE0028_Unknown_Record_Type, UG_ERR_ERROR,
				2, objrec->name, objrec->class);
		return (CUQUIT);
	}

	/* other errors are DBMS errors */
	if ( (filestatus != OK) &&
		(filestatus != ENDFILE) && (filestatus != CUQUIT) )
	{
		IIUGerr(E_IE003A_DBMS_Error, UG_ERR_ERROR, 0);
		return (CUQUIT);
	}

	return (filestatus);
}
