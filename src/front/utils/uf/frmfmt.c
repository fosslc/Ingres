/*
** Copyright (c) 1989, 2008 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>
# include       <er.h>
# include       <cm.h>
# include       <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <adf.h>
# include       <afe.h>
# include       <ft.h>
# include       <fmt.h>
# include       <frame.h>
# include       <ug.h>
# include       <ui.h>
# include       <qg.h>
# include	<mqtf.h>
# include       "eruf.h"

/**
** Name:	frmfmt.c - 	Format default form components	
**
** Description:
**	Generate default form fields, columns, and trim.  This is called, for 
**	instance, from:
**		QBF form generation
**		Visual Query form generation and form fixup
**	
** This file defines:
**
**	IIFDffFormatField	Generate a default field
**	IIFDfcFormatColumn	Format a default tablefield column (Completely)
**	IIFDmcMakeColumn	Format a default tablefield column (Phase 1)
**	IIFDgcGenColumn		Format a default tablefield column (Phase 2)
**	IIFDftFormatTitle	Format a title string
**
** History:
**	7/10/89	(Mike S)	Initial version
**	7/20/89 (Mike S)	Set FLDTYPE entries to empty strings.
**	8/9/89 (Mike S)		Add scrolling field support
**	20-nov-89 (bruceb)
**		Moved code from frame to uf.
**	8-jun-90 (Mike S) Default fields and columns to color 1.
**	09/12/90 (esd) IIFDgcGenColumn: insert attr bytes around title.
**	06/16/92 (dkh) - Added code to copy precison/scale value for
**			 field createion so decimal fields will work.
**      06/21/95 (harpa06)
**              Kill the unconditional conversion of a table name to lowercase 
**              and then the  conversion of the first letter in the table name 
**              to uppercase.
**      20-sep-95 bug #71398 (lawst01)
**         If installation supports lowercase then capitalize the first
**         letter of the field name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (somsa01)
**	    Changed maxcharwidth from 10000 to FE_FMTMAXCHARWIDTH.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/* # define's */
# define FMTBUFSZ	200

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN STATUS      fmt_sdeffmt();
FUNC_EXTERN STATUS      fmt_deffmt();
FUNC_EXTERN STATUS      afe_cancoerce();
FUNC_EXTERN VOID	IIAFddcDetermineDatatypeClass();

/* static's */
static STATUS 	makefmt(); 	/* Get format for datatype */

static ADF_CB  *adfcb = NULL;  /* ADF control block address */
	/*      DB_DATA_VALUE for float */
static const DB_DATA_VALUE fltdbv = { NULL, sizeof(f8), DB_FLT_TYPE };

static MQTF		mqtf;	/* Store tablefield column information */
static const char	_empty[1] = ERx("");

/*{
** Name:        IIFDffFormatField - Format a field of any type
**
** Description:
**      Format a field, given the datatype.  No terribly fancy formatting is
**      supported: the title and field will appear on the same line; the title
**      must come first; the field will be multiline if it doesn't fit
**      on one line.  This is the routine QBF uses to format its default forms.
**
**	We will produce a scrolling field if:
**		The field datatype is of class CHAR_DTYPE, and
**		"maxwidth" is positive, and
**		the default field produced is sigle-line and
**		the default field produced is wider than "maxwidth".
**
**	If a scrolling field is produced, it is the caller's responsibility
**	to set the fdSCRLFD flag in the form's frmflags field.
**		
**      An error occur if:
**              Any line or column numbers are negative.
**              The title column isn't smaller than the field column.
**              The title and data field would overlap.
**              The title doesn't fit on the line.
**              The field can't be wrapped to fit on the line.
**              A non-character type requires wrapping.
**
** Inputs:
**      name            char *  field name
**      fldseq          i4      field sequence number
**      line            i4      start line
**      col             i4      start column for field
**      titcol          i4      start column for title
**      title           char *  title string
**      frmwidth        i4      width of form
**      flags           i4      field flags
**      dbv             DB_DATA_VALUE * field datatype
**      wrapcol         i4      column to start at if we wrap
**	maxwidth	i4	maximum width of produced field
**
** Outputs:
**      field           FIELD ** field structure produced
**
**      Returns:
**              STATUS  OK      if field was produced
**                      FAIL    if memory allocation failed
**                      -1    	if input args are inconsistent
**                      -2      if adf or ade failed for datatype
**
**      Exceptions:
**              none
**
** Side Effects:
**              Dynamic memory is allocated
**
** History:
**      7/7/89 (Mike S) Initial version
*/
STATUS
IIFDffFormatField(name, fldseq, line, col, titcol, title, frmwidth, flags,
                 dbv, wrapcol, maxwidth, field)
char    *name;
i4      fldseq;
i4      line;
i4      col;
i4      titcol;
char    *title;
i4      frmwidth;
i4      flags;
DB_DATA_VALUE   *dbv;
i4      wrapcol;
i4	maxwidth;
FIELD   **field;
{
	/* Size of title in chars */
        i4      titsize = title != NULL ? STlength(title) : 0;
        FIELD   *fld;                           /* Field to create */
        FLDHDR  *fhdr;                          /* Field header */
        FLDTYPE *ftype;                         /* Field type info */
        FLDVAL  *fval;                          /* Field value info */
        i4      dataln;                         /* Length of data in field */
        i4      datacols;                       /* Columns available for data */
	char	fmtbuf[FMTBUFSZ];		/* Format buffer */
	i4	class;				/* Datatype class */
	DB_DATA_VALUE dbv2;			/* Used for scrolling format */
	bool	can_wrap;

        /* Check input args for reasonability */
        if (    (line < 0) || (col < 0) || (titcol < 0) || (wrapcol < 0) ||
                (titcol + titsize > col)                                 ||
                (col >= frmwidth)                                        ||
                (wrapcol - titcol + col >= frmwidth)
           )
        {
                IIUGerr(E_UF0032_Bad_Format_Params, 0, 1, name);
                return ((STATUS)-1);
        }
	/*
	** Check if wrapping is allowed, we can wrap any characeter
	** datatype
	*/
	IIAFddcDetermineDatatypeClass(abs(dbv->db_datatype), &class);
	if (class == CHAR_DTYPE)
		can_wrap=TRUE;
	else
		can_wrap=FALSE;
			
	/* If no color is specified, use color 1 */
	if ((flags & fdCOLOR) == 0)
		flags |= fd1COLOR;

        /* Allocate field struct and begin to fill in fields */
        if ((fld = FDnewfld(name, fldseq, FREGULAR)) == NULL)
                return (FAIL);
        fhdr = &(fld->fld_var.flregfld->flhdr);
        ftype = &(fld->fld_var.flregfld->fltype);
        fval = &(fld->fld_var.flregfld->flval);
	/* Field title */
        fhdr->fhtitle = title != NULL ? FEsalloc(title) : _empty;
        fhdr->fhtitx = 0;                       /* Offset to title start */
        fhdr->fhtity = 0;                       /* Offset to title start */
        ftype->ftdatax = col - titcol;          /* Offset to data */
        fval->fvdatay = 0;                      /* Offset to data */
        ftype->ftdatatype = dbv->db_datatype;   /* Field data datatype */
        ftype->ftlength = dbv->db_length;       /* Field data internal length */
	ftype->ftprec = dbv->db_prec;		/* Field data precision/scale */
	fhdr->fhdflags = flags;                 /* Display flags */
        fhdr->fhseq = fldseq;                   /* Field sequence number */
 
        /*
        **      Get a tentative one-line format
        */
	if (makefmt(name, dbv, fmtbuf, &dataln) != OK)
		return((STATUS)-2);
        ftype->ftwidth = dataln;                /* Field data external length */ 
	/* See if field fits */
        if (col + dataln <= frmwidth)
        {
                /* If field fits on one line, it's easy */
                ftype->ftdataln = dataln;       /* Data chars. per line */
                fhdr->fhposx = titcol;          /* field start coordinate */
                fhdr->fhposy = line;            /* field start coordinate */
                fhdr->fhmaxy = 1;               /* Number of lines in field */
        }
        else if (wrapcol + col - titcol + dataln <= frmwidth)
        {
                /*
                ** Otherwise, if it fits starting at wrapcol, put it
                ** on the next line
                */
                ftype->ftdataln = dataln;       /* Data chars. per line */
                fhdr->fhposx = wrapcol;         /* field start coordinate */
                fhdr->fhposy = line + 1;        /* field start coordinate */
                fhdr->fhmaxy = 1;               /* Number of lines in field */
        }
        else
        {
                /*
                ** We need a multiline format.  If we were going to start
                ** past wrapcol, start at wrapcol on the next line down.
                ** First check that this type can wrap.
                */
		if (!can_wrap)
		{
                       IIUGerr(E_UF0033_Cannot_Wrap, 0, 2, name,
                                &dbv->db_datatype);
                        return((STATUS) -1);
                }
                if (titcol > wrapcol)
                {
                        fhdr->fhposx = wrapcol; /* field start coordinate */
                        fhdr->fhposy = line + 1;/* field start coordinate */
                }
                else
                {
                        fhdr->fhposx = titcol;  /* field start coordinate */
                        fhdr->fhposy = line + 1;/* field start coordinate */
                }
                /*
                ** Calculate number of lines in format.  Data begins at column
                ** (fhposx + ftdatax), so there's space for
                ** (frmwidth - (fhposx + ftdatax)) characters.
                */
                datacols = frmwidth - (fhdr->fhposx + ftype->ftdatax);
                fhdr->fhmaxy = (dataln + datacols - 1) / datacols;
                                        /* number of lines in field */
                ftype->ftdataln = (dataln + fhdr->fhmaxy - 1) / fhdr->fhmaxy;
                                        /* number of data chars per line */
                /*
                ** Generate the wrap format.
                */
                if (fmt_deffmt(adfcb, dbv, ftype->ftdataln, (bool)FALSE, fmtbuf)                    != OK)
                {
                        IIUGerr(E_UF0034_Cannot_get_wrap, 0, 1, name);
                        return((STATUS)-2);
                }
        }
 
	/* See if we want a scrolling field */
	if ((maxwidth > 0) && 
	    (fhdr->fhmaxy == 1) && 
	    (maxwidth < ftype->ftdataln))
	{
		/* We want one. We can only have one for wrappable fields */
		if (can_wrap)
		{
			/*
			** A cheap trick.  Create a field for a smaller
			** datatype, but keep the large internal length.
			*/
			STRUCT_ASSIGN_MACRO((*dbv), dbv2);
			dbv2.db_length -= ftype->ftdataln - maxwidth;
			if (makefmt(name, &dbv2, fmtbuf, &dataln) != OK)
				return ((STATUS)-2);
			ftype->ftdataln = ftype->ftwidth = dataln;
			fhdr->fhd2flags |= fdSCRLFD;
		}
	}

        /* Whatever we've gone through to get here, the following are correct */
        ftype->ftfmtstr = FEsalloc(fmtbuf);     /* Field format string */
	ftype->ftvalstr = ftype->ftvalmsg = ftype->ftdefault = _empty;
        fhdr->fhmaxx = ftype->ftdatax + ftype->ftdataln;
                                                /* Number of chars per line */
        *field = fld;
        return(OK);
}

/*{
** Name:	IIFDfcFormatColumn	- Format a tablefield column
**
** Description:
**	Generate a default tablefield column.  The column is generated at
**	x-coordinate 0; the true coordinate must be set at insertion into
**	the tablefield.
**
** Inputs:
**	name		char *		Column name
**	colseq		i4		Column sequence number
**      title           char *  	title string
**      dbv             DB_DATA_VALUE * field datatype
**	maxwidth	i4		maximum column width
**					ignored if <= 0
**
** Outputs:
**      column           FLDCOL ** field structure produced
**
**      Returns:
**              STATUS  OK      if field was produced
**                      errors	from IIFDmcMakeColumn or IIFDgcGenColumn
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	7/10/89 (Mike S)	Initial version
*/
STATUS
IIFDfcFormatColumn(name, colseq, title, dbv, maxwidth, column)
char	*name;
i4	colseq;
char	*title;
DB_DATA_VALUE *dbv;
i4	maxwidth;
FLDCOL	**column;
{
	STATUS	status;

	status = IIFDmcMakeColumn(name, title, dbv, TRUE, maxwidth, &mqtf, 
				 (i4 *)NULL);
	if (status == OK)
		status = IIFDgcGenColumn(&mqtf, colseq, dbv, column);
	return (status);
}


/*{
** Name:	IIFDmcMakeColumn - Format tablefield column (phase 1)
**
** Description:
**	Begin to format a tablefield column, and store the results in an 
**	MQTF structure.  If the build flag is FALSE, we don't actually do this, **	we just test its feasibility.
**
** Inputs:
**	name		char *		Column name
**      title           char *  	title string
**      dbv             DB_DATA_VALUE * field datatype
**	build		bool		TRUE to actually fill mqtf
**	maxwidth	i4		maximum column width
**					ignored if <= 0
**
**
** Outputs:
**      mqtf		MQTF *		structure filled with tablefield data
**					ignored if build is FALSE
**	dataln		i4 *		Data external length
**					Not set if NULL
**
**      Returns:
**              STATUS  OK      if no errors occured
**			-2	If a datatype error occurs
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	7/10/89	(Mike S)	Initial version
*/
STATUS
IIFDmcMakeColumn(name, title, dbv, build, maxwidth, mqtf, dataln)
char	*name;
char	*title;
DB_DATA_VALUE	*dbv;
bool	build;
i4	maxwidth;
MQTF	*mqtf;
i4	*dataln;
{
	char	fmtbuf[FMTBUFSZ];
	i4	length;			/* External data length */
	i4	class;			/* Data type class */
	DB_DATA_VALUE dbv2;		/* For scrolling format */

	/* Generate a default format. */
	if (makefmt(name, dbv, fmtbuf, &length) != OK)
		return ((STATUS)-2);

	mqtf->flags = mqtf->flags2 = (i4)0;

	/* See if we want a scrolling field */
	if ((maxwidth > 0) && (maxwidth < length))
	{
		IIAFddcDetermineDatatypeClass(abs(dbv->db_datatype), &class);
		if (class == CHAR_DTYPE )
		{
			/*
			** We want one.  Use the same cheap trick as above --
			** generate a format for a smaller datatype, but keep
			** the large internal size.
			*/
			STRUCT_ASSIGN_MACRO((*dbv), dbv2);
			dbv2.db_length -= length - maxwidth;
			if (makefmt(name, &dbv2, fmtbuf, &length) != OK)
				return ((STATUS)-2);
			mqtf->flags2 |= fdSCRLFD;
		}
	}

	/* If we're building, fill in the structure */
        if (build)
        {
        	mqtf->title = title != NULL ? FEsalloc(title) : _empty;
                mqtf->format = FEsalloc(fmtbuf);
                CVlower(mqtf->name = FEsalloc(name));
                mqtf->length = length;
	}
	if (dataln != NULL) *dataln = length;
	return (OK);
}


/*{
** Name:	IIFDgcGenColumn	- Format a Tablefield column (Phase 2)
**
** Description:
**	Make a tablefield column from an MQTF structure
**
** Inputs:
**      mqtf		MQTF *		structure filled with tablefield data
**	colseq		i4		column sequence number
**      dbv             DB_DATA_VALUE * field datatype
**
** Outputs:
**	column		FLDCOL **	Field structure produced
**
**	Returns:
**		STATUS		OK
**				FAIL	if memory allocation fails
**
**	Exceptions:
**		none	
**
** Side Effects:
**	Dynamic memory is allocated.
**
** History:
**	7/10/89 (Mike S)	Initial version
**      9/12/90 (esd)           Allow space for 3270 attr bytes around title.
*/
STATUS
IIFDgcGenColumn(mqtf, colseq, dbv, column)
MQTF	*mqtf;
i4	colseq;
DB_DATA_VALUE	*dbv;
FLDCOL	**column;
{
	FLDCOL      		*col;
	register FLDHDR		*hdr;
	register FLDTYPE 	*type;
	i4 titlen;	/* Title length */
	i4 dataln;	/* Data length */

	/* Allocate column structure */
	if ((col = FDnewcol(mqtf->name, colseq)) == NULL)
		return FAIL;
 
	/* Point to column structs */
	hdr = &col->flhdr;
	type = &col->fltype;

	/* Fill in column fields */
        hdr->fhtitle = mqtf->title;		/* Column title */
        hdr->fhtitx = hdr->fhposx = type->ftdatax = 0;
						/* Set x-coordinates to 0 */
        hdr->fhtity = 1;			/* title on line 1 */		
        type->ftfmtstr = mqtf->format;		/* format string */
        type->ftdatatype = dbv->db_datatype;	/* Datatype */
        type->ftlength = dbv->db_length;	/* Internal length */
	type->ftprec = dbv->db_prec;		/* precision/scale value */
        type->ftwidth = mqtf->length;		/* External length */
        titlen = STlength(mqtf->title);
        dataln = mqtf->length;
        hdr->fhmaxx = max(titlen, dataln) + 2 * FTspace(); /* Column width */
        hdr->fhmaxy = 1;			/* Column height */
        type->ftdataln = mqtf->length;		/* Column data length */
        hdr->fhposy = 0;			/* Column height offset */
	hdr->fhdflags |= mqtf->flags;		/* Column flags */
	hdr->fhd2flags |= mqtf->flags2;
	type->ftvalstr = type->ftvalmsg = type->ftdefault = _empty;
						/* No values for these */

	/* If no color is specified, use color 1 */
	if ((hdr->fhdflags & fdCOLOR) == 0)
		hdr->fhdflags |= fd1COLOR;

	*column = col;
	return (OK);
}


/*{
** Name:	IIFDftFormatTitle - format title string
**
** Description:
**	Format a string into a title by:
**		Lowercasing it.
**		Capitalizing the first character.
**		Replacing an underscore by a blnak and capitalizing
**		the next character.
**	
** Inputs:
**	title	char *	string to format (modified)
**
** Outputs:
**	none
**
**	Returns:
**		char *	title pointer (echoed)
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	7/10/89 (Mike S)	Initial version
**	6/91 (Mike S) 
**		Convert title to lowercase before processing it.  This is
**		important if we're starting with a database column name from
**		an uppercase gateway, like RDB.  Per FRC decision. 6/3/91
**	6/21/95 (harpa06)
**		Kill the unconditional conversion of a table name to lowercase 
**		and then the  conversion of the first letter in the table name 
**		to uppercase.
*/
char *IIFDftFormatTitle(char *title)
{
	register char *cp = title;

	if ((cp != NULL) && (*cp != EOS)) 
	{
            if (IIUIcase() != UI_MIXEDCASE)
               {
                CVlower(cp);
                CMtoupper(cp, cp);
               }
		/* Loop through, looking for underscores */
        	while ((cp = STindex(cp, ERx("_"), 0)) != NULL)
        	{
        		*cp = ' ';
			CMnext(cp);
        		if (*cp == EOS)
        			break;
        		else
        			CMtoupper(cp, cp);
        	}
	}
	return(title);
}

/*{
** Name:	makefmt		-	Make default format for datatype
**
** Description:
**	Generate the default format for a given datatype.  Numeric fields are
**	explicitly left-justified.
**
** Inputs:
**	name	char *			Field or column name
**	dbv	DB_DATA_VALUE *		Datatype
**
** Outputs:
**	fmt	char *			Format string (filled in)
**	length	i4 *			External data length
**
**	Returns:
**		STATUS			OK
**					FAIL	
**
**	Exceptions:
**		none	
**
** Side Effects:
**		none
**
** History:
**	7/10/89 (Mike S)	Initial version
**	07-mar-2001 (somsa01)
**	    Changed maxcharwidth from 10000 to FE_FMTMAXCHARWIDTH.
*/
static STATUS
makefmt(name, dbv, fmt, length)
char	*name;
DB_DATA_VALUE	*dbv;
char	*fmt;
i4	*length;
{
        bool    numeric;                        /* TRUE for numeric type */
        char    fmtstr[FMTBUFSZ];		/* format string */
        i4      dummy;

        /*
        **      Initialize ADF (if need be) and generate a format.
        **      We see if it's a numeric type by seeing if it's coerceable to
        **      float.
        */
        if (adfcb == NULL) adfcb = FEadfcb();
        if (fmt_sdeffmt(adfcb, dbv, FE_FMTMAXCHARWIDTH, (bool) FALSE, fmt,
                        length, &dummy) != OK)
        {
                IIUGerr(E_UF0035_Cannot_get_default, 0, 1, name);
                return(FAIL);
        }
        if (afe_cancoerce(adfcb, dbv, &fltdbv, &numeric) != OK)
        {
                IIUGerr(E_UF0036_Cannot_check_datatype, 0, 1, name);
                return(FAIL);
        }
        if (numeric)
        {
                /* Left-justify */
		STcopy(fmt, fmtstr);
                STpolycat(2, ERx("-"), fmtstr, fmt);
	}
	return (OK);
}

