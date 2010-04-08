/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<er.h>
# include	<st.h>
# include	<lo.h>
# include	<gl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<qg.h>
# include	<mqtypes.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<trimtxt.h>
# include	<lqgdata.h>
# include 	"framegen.h"
# include 	<erfg.h>

/**
** Name:	fgdefram.c	Construct default frame
**
** Description:
**	This file defines:
**
**	IIFGmdf_makeDefaultFrame	Construct a default frame
**	IIFGmvf_makeVQFrame		Construct a default VQ frame
**	
**
** History:
**	5/25/89 (Mike S) Initial version
**	7/17/89 (Mike S) Save form to database (interface change)
**	8/25/89 (Mike S) add form title
**	10/5/89	(Mike S) add cover routine
**	12/19/89 (dkh) - VMS shared lib changes.
**	10/12/90 (esd) - modified add_column for QBF logical key support.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	19-aug-91 (blaise)
**	    Added missing #include cv.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/* # define's */
# define DEFWIDTH	80		/* Default form width */
 
/* GLOBALDEF's */
/* extern's */
extern	char	mq_tbl[];	/* tablefield name */
extern	i4	qdef_type;	/* query type */
extern  i4	tblfield;	/* Use tablefield? */
extern	i4	mq_fgtype;	/* QBF or Emerald-style form */
extern  TRIMTXT *mq_frmtitle;	/* Form title */

FUNC_EXTERN STATUS 	IIFGfftFormatFrameTitle();
FUNC_EXTERN VOID 	IIFGti_tblindx();
FUNC_EXTERN FRAME 	*mq_makefrm();
FUNC_EXTERN i4  	mqcalwidth();
FUNC_EXTERN char	*UGcat_now();
FUNC_EXTERN VOID	FTterminfo();
FUNC_EXTERN STATUS	IIAMmpMetaPopulate();

/* static's */
static VOID add_column();
static VOID add_lookups();
static VOID add_table();

static METAFRAME	*s_metaframe;
static MQQDEF		mqqdef;		/* Query-definition structure */
static RELINFO		*rels; 		/* relation array pointer */
static ATTRIBINFO	*atts;		/* attribute array pointer */
static i4		aindx;		/* attribute array index */
static i4		frametype;	/* Master-only, master-detail, etc. */

/*{
** Name:	IIFGmdf_makeDefaultFrame - Construct a default frame
**
** Description:
**	Insure the VQ structures are in memory.  Construct a default frame.  
**	If we were told to, update the formgen date in the catalogs.
**	update should be TRUE unless the caller is sure to  update the catalogs.
**
** Inputs:
**	metaframe	METAFRAME *	metaframe for frame to construct
**	update		bool		whether to update the catalogs
**
** Outputs:
**	none
**
**	Returns:
**		STATUS	OK or FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**		Catalogs may be written.
**
** History:
**	10/5/89 (Mike S) initial version
*/
STATUS
IIFGmdf_makeDefaultFrame(metaframe, update)
METAFRAME *metaframe;
bool	  update;
{
	STATUS status;
	USER_FRAME *apobj = (USER_FRAME *)metaframe->apobj;

	/* Get VQ structures */
	if ((metaframe->popmask & MF_P_VQ) == 0)
	{
		if (IIAMmpMetaPopulate(metaframe, MF_P_VQ) != OK)
		{
			return (FAIL);
		}
	}

	/* Create the form */
	if (apobj->class == OC_MUFRAME)
		status = IIFGmm_makeMenu(metaframe);
	else
		status = IIFGmvf_makeVQFrame(metaframe);

	/* Update the formgen date */
	if (status == OK)
	{
		metaframe->formgendate = STtalloc(apobj->data.tag,UGcat_now());
		if (update) IIAMufqUserFrameQuick(apobj);
	}

	return(status);
}

/*{
** Name:	IIFGmvf_makeVQFrame	Construct a default VQ frame
**
** Description:
**	Construct a default frame for a Visual Query.
**
**	We construct an MQQDEF structure, and call the QBF frame-construction
**	routines.  Within each section (master, detail) we order the columns:
**
**		column 1 from primary table
**		all columns from lookup tables joined to primary column 1
**		column 2 from primary table
**		all columns from lookup tables joined to primary column 2
**		etc.
**
**	We use the following as inputs to mq_makefrm, and mqgetwidth:
**
**	Parameters:
**	MQQDEF	mqqdef
**		mqcr.name	form name
**		mqtabs		RELINFO array
**		mqnumtabs	size of above
**		mqatts		ATTRIBINFO array
**		mqnumatts	size of above
**		
**	RELINFO (dynamic)
**		mqrelid		relation name
**		mqrangevar	relation name
**		mqtype		table role 
**
**	ATTRIBINFO (dynamic)
**		col		attribute name
**		formfield	field name
**		ttype		column role
**		dbv		database value
**		opupdate	update rules
**		jpart		join usage
**		intbl		in table field?
**		mandfld		mandatory?
**		opupdate	== 1 for a sequenced field (a kludge)
**		lkeyinfo	logical key?
**
**	Globals:
**	mq_tbl			tablefield name
**	qdef_type		query type
**	tblfield		Use a table field?
**	mq_fgtype		QBF or Emerald-style form
**	mq_frmtitle		Form title
**
** Inputs:
**	metaframe	METAFRAME *	Visual Query's metaframe
**
** Outputs:
**	none
**
**	Returns:
**			STATUS		OK if form is written to database
**					FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
**		The form name is copied to the global mq_frame.
**
** History:
**	5/26/89	(Mike S)	Initial version
**	8/10/89	(Mike S)	Clear change bits
**	10/5/89 (Mike S)	Rename
**	4-jan-1994 (mgw) Bug #58241
**		Implement owner.table for query frame creation.
*/

STATUS
IIFGmvf_makeVQFrame( metaframe )
METAFRAME	*metaframe;
{
	MFTAB		*tptr;		/* Metaframe table pointer */
	RELINFO		*rptr;		/* relation pointer */
	i4		tblwidth = 0;	/* Tablefield width */
	i4		tindx, lindx, hindx;
					/* Table indices */
	i4		cindx;		/* Column indices */
	FRAME		*frame;	
	i4 		sections;
	i4		pass;
	i4		status;
	STATUS		ret_val;
	USER_FRAME	*apobj = (USER_FRAME *)metaframe->apobj;
	TAGID		tag;		/* Default tag */
	TRIMTXT		*trimtxt;
	STATUS		mstatus;
	i4		term_cols;	/* Form width */
	char		tmpname[FE_MAXNAME+1];
	char		tmpown[FE_MAXNAME+1];
	FE_RSLV_NAME	tmprslv;

	/* If the form system is active, use current width */
	if (IILQgdata()->form_on)
	{
		FT_TERMINFO	terminfo;

		FTterminfo(&terminfo);
		term_cols = terminfo.cols;
	}
	else
	{
		term_cols = DEFWIDTH;
	}

	/* Start a new tag region */
	tag = FEbegintag();

	/* Assume failure */
	ret_val = FAIL;

	/* Set up globals and statics */
	STcopy(TFNAME, mq_tbl);
	mq_fgtype = MQFORM_VISION;
	s_metaframe = metaframe;

	frametype = metaframe->mode & MFMODE;
	switch (frametype)
	{
	    case MF_MASDET: 	/* master-detail */
		qdef_type = mqMD_JOIN;
		tblfield = TRUE;
		sections = 2;
		break;

	    case MF_MASSIM: 	/* master-only in simple fields */
		qdef_type = mq1to1_JOIN;
		tblfield = FALSE;
		sections = 1;
		break;

	    case MF_MASTAB:	/* master-only in a table field */
	           qdef_type = mq1to1_JOIN;
        	   tblfield = TRUE; 
        	   sections = 1; 
        	   break;

	    default:
	    	IIUGerr (E_FG001B_BadMfMode, 0, 1, &metaframe->mode);
		goto cleanup;
	}
	
	/* Initialize MQQDEF structure */
	STcopy(apobj->form->name, mqqdef.mqcr.name);
	mqqdef.mqnumtabs = metaframe->numtabs;
	mqqdef.mqnumatts = 0;

	/*
	** Calculate number of fields and columns on form, which is the
	** number of displayed fields in the metaframe. 
	*/
	for ( tindx = 0; tindx < metaframe->numtabs; tindx ++ )
	{
		tptr = metaframe->tabs[tindx];
		for ( cindx = 0; cindx < tptr->numcols; cindx ++ )
		{
			if ((tptr->cols[cindx]->flags & COL_USED) != 0)
				mqqdef.mqnumatts++;
		}
	}
	
	/*	Allocate relation and attribute arrays and pointer arrays */
	rels = (RELINFO *)FEreqmem((u_i4)0, 
				   (u_i4)(sizeof(RELINFO)*mqqdef.mqnumtabs),
				   FALSE,
				   &mstatus);
	atts = (ATTRIBINFO *)FEreqmem((u_i4)0, 
				   (u_i4)(sizeof(ATTRIBINFO)*mqqdef.mqnumatts),
				   FALSE,
				   &mstatus);
	if ( rels == (RELINFO *)NULL ||
	    atts == (ATTRIBINFO *)NULL )
	{
	    IIUGerr (E_FG001A_Deffrm_NoDynMem, 0, 0);
	    goto cleanup;
	}

	/* Create all RELINFO structures */
	for (tindx = 0, rptr = rels; tindx < mqqdef.mqnumtabs; tindx++, rptr++ )
	{
		tptr = metaframe->tabs[tindx];
		mqqdef.mqtabs[tindx] = rptr;

		tmpname[0] = EOS;
		tmpown[0] = EOS;
		tmprslv.name = tptr->name;
		tmprslv.name_dest = tmpname;
		tmprslv.owner_dest = tmpown;
		tmprslv.is_nrml = TRUE;
		FE_decompose(&tmprslv);
		STcopy(tmpname, rptr->mqrelid);
		STcopy(tmpname, rptr->mqrangevar);
		rptr->mqtype = (tptr->tabsect > TAB_MASTER);
	}

	/* Create all ATTRIBINFO structures */
	aindx = 0;	
	for (pass = 0; pass < sections; pass++)
	{
		IIFGti_tblindx(metaframe, pass, &lindx, &hindx);
		add_table(lindx);
	}
	mqqdef.mqnumatts = aindx;

	/* Calculate tablefield width and create tablefield data */
	if (tblfield)
	{
		tblwidth = mqgetwidth(&mqqdef);
		if (tblwidth < 0) goto cleanup;
	}

	/* Create form title */
	if (IIFGfftFormatFrameTitle(metaframe, term_cols, &trimtxt) == OK)
		mq_frmtitle = trimtxt;	/* Set Global title pointer */
	
	/* Create frame and save it to database */
	if ((frame = mq_makefrm(&mqqdef, tblwidth)) == NULL)
		goto cleanup;

	status = IIFGscfSaveCompForm((USER_FRAME *)metaframe->apobj, frame);
	if (status == 0)
	{
		ret_val = OK;
	}

	/* Release all dynamic memory */
cleanup:
	_VOID_ FEendtag();
	IIUGtagFree(tag);
		
	return (ret_val);
}

/*
**	Create ATTRIBINFO structures for all displayed columns in a table.
*/
static VOID 
add_table( tindx )
i4		tindx;	/* table index */
{
	MFTAB *tptr = s_metaframe->tabs[tindx];
	MFCOL *cptr;
	i4   cindx;

	/* Loop through all columns in table */
	for (cindx = 0; cindx < tptr->numcols; cindx++)
	{
		cptr = tptr->cols[cindx];
		if ((cptr->flags & COL_USED) != 0)
		{
			add_column(tptr, cptr);

			/*
			**	If this is a primary lookup column, add the
			**	table(s) we're linked to.
			*/
			if ((cptr->flags & COL_LUPJOIN) != 0)
				add_lookups(tindx, cindx);
		}
	}
}

/*
**	Add columns from all the lookup tables joined to the current column.
*/
static VOID
add_lookups(tindx, cindx )
i4		tindx;		/* master lookup join column's table index */
i4		cindx;		/* master lookup join column's column index */
{
	i4 	jindx;
	MFJOIN	*jptr;
	MFTAB	*tptr;
	i4	i;

	for (jindx = 0; jindx < s_metaframe->numjoins; jindx++)
	{
		jptr = s_metaframe->joins[jindx];
		if (jptr->tab_1 == tindx &&
		    jptr->col_1 == cindx &&
		    (jptr->type == JOIN_MASLUP ||
		     jptr->type == JOIN_DETLUP) )
		{
			/*
			** We've found a lookup table.  Add it if it has 
			** displayed columns.  If it doesn't, or if this
			** isn't the first use of this table in the section,
			** table name to an empty string, so it won't be
			** used in the title line.
			*/
			tptr = s_metaframe->tabs[jptr->tab_2];
			for (i = 0; i < tptr->numcols; i++)
			{
				if ((tptr->cols[i]->flags & COL_USED) != 0)
					break;
			}

			if (i < tptr->numcols)
			{
				/* We found a displayed column */
				MFTAB	**tabs = s_metaframe->tabs;	
						/* tables array */
				i4	j;

				add_table(jptr->tab_2);
				for (j = tindx; j < jptr->tab_2; j++)
				{
					if (STcompare(tabs[j]->name, 
						      tptr->name) == 0)
					{
						/* 
						** An earlier use of the 
						** table.
						*/
						*rels[jptr->tab_2].mqrelid = 
							EOS;
						break;
					}
				}
			}
			else
			{
				*rels[jptr->tab_2].mqrelid = EOS;
			}
		}
	}
}

/*
**	Add an ATTRIBINFO structure.
**
** History:
**	5/26/89	(Mike S)	Initial version
**	09/27/90 (esd)
**		Set lkeyinfo in ATTRIBINFO to 0 to indicate to the QBF routines
**		mq_makefrm & mqgetwidth that the attribute is not a logical key.
**		(This may be a lie, but Emerald doesn't currently support
**		logical keys anyway).  (bug 8593).
**	28-feb-1991 (pete)
**		CVlower form field name. UPPER CASE gateways cause a problem
**		otherwise -- FRS doesn't do too well with UPPER CASE field
**		names. Bug 36223.
*/
static VOID
add_column(tptr, cptr)
MFTAB 	*tptr;	/* table pointer */
MFCOL	*cptr;	/* column pointer */
{
	ATTRIBINFO	*aptr = atts + aindx;

	mqqdef.mqatts[aindx++] = aptr;

	/* column name, field name */
	STcopy(cptr->name, aptr->col);
	STcopy(cptr->alias, aptr->formfield);
	CVlower(aptr->formfield);	/* FRS requires Lower Case field names*/

	/* relation usage type */
	if (tptr->tabsect == TAB_MASTER)
		aptr->ttype = (tptr->usage < TAB_LOOKUP) ?
					mqIN_MASTER : mqIN_LU_MASTER;
	else
		aptr->ttype = (tptr->usage < TAB_LOOKUP) ?
					mqIN_DETAIL : mqIN_LU_DETAIL;
	
	/* Data value type */
	STRUCT_ASSIGN_MACRO(cptr->type, aptr->dbv);

	/* We use opupdate to indicate sequenced fields */
	if ((cptr->flags & COL_SEQUENCE) != 0)
		aptr->opupdate = 1;
	else
		aptr->opupdate = 0;

	/* Set flags in join-type word */
	aptr->jpart = mqNOT_JOIN;
	if ((cptr->flags & COL_DETJOIN) != 0)
		if ((cptr->flags & COL_LUPJOIN) != 0)
			aptr->jpart = mq2_JOIN_FIELD;
		else
			aptr->jpart = mqJOIN_FIELD;
	else if ((cptr->flags & COL_LUPJOIN) != 0)
		aptr->jpart = mqLU_JOIN_FIELD;

	/* "In tablefield" flag */
	aptr->intbl = (frametype == MF_MASTAB) || (tptr->tabsect > TAB_MASTER);

	/* Not a logical key */
	aptr->lkeyinfo = 0;

	/* 
	**	mandatory flag.  TRUE if not nullable and no default,
	** 	and we're not in a lookup table.  Also true if it has
	**	a lookup on it.
	*/
	aptr->mandfld = FALSE;
	if ((cptr->type.db_datatype > 0) &&
	    ((cptr->flags & COL_DEFAULT) == 0) &&
	    (tptr->usage < TAB_LOOKUP))
		aptr->mandfld = TRUE;

	if ((cptr->flags & COL_LUPJOIN) != 0)
		aptr->mandfld = TRUE;

}

