/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <ug.h>
# include       <adf.h>
# include       <afe.h>
# include       <fmt.h>

/**
** Name:	vqcatloc.c - Convert catalog time to local time for reports
**			
**
** Description:
**	This file defines:
**
**	IIVQctlCatToLocal  	Convert catalog time to local time
**	IIVQntlNowToLocal  	Convert current time to local time
**
** History:
**	1/26/90 (Mike S) Extracted from vqapprep.qsc
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
# define DATESIZE 40
# define FMTSTR ERx("d\"Feb 3, 1901 04:05 p.m.\"")

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	char	*UGcat_now();

/* static's */

/* formatting information for the date displays */
static FMT fmt;
static char fmtbuf[30] = {0};
static char catnowbuf[80];
static char locnowbuf[DATESIZE];

/*{
** Name:        IIVQctlCatToLocal  - convert catalog format date to local time
**
** Description:
**      This routine converts the catalog format date string to a
**      string which is in the local time zone.
**
** Inputs:
**      char *instr;    - input date string
**      char *outstr;   - output string (should be at least 40, there
**                        are instructions in the message file to not
**                        make the string larger than 39)
**
** Outputs:
**      Returns:
**              none
**
**      Exceptions:
**              none
**
** Side Effects:
**
** History:
**      11/30/89 (tom)  created
*/
VOID
IIVQctlCatToLocal(instr, outstr)
char *instr;
char *outstr;
{
        AFE_DCL_TXT_MACRO(DATESIZE + 1) datestr;
        DB_DATA_VALUE idbv;
        DB_DATA_VALUE odbv;
        DATE date;
        i4  dummy;
 
        
        /* convert input string to date struct */
        UGcat_to_dt(instr, &date);
        
        /* convert date struct to DB_DATA_VALUE */
        idbv.db_datatype = DB_DTE_TYPE;
        idbv.db_length = sizeof(DATE);
        idbv.db_data = (PTR)&date;
 
        /* set union member of the static FMT struct */
 
        /* setup the output DB_DATA_VALUE */
        odbv.db_datatype = DB_TXT_TYPE;
        odbv.db_length = DATESIZE + 1 + sizeof(datestr.count);
        odbv.db_data = (PTR) &datestr;
 
        /* if we havn't initialized the formating information */
        if (fmtbuf[0] == EOS)
        {
                fmt.fmt_var.fmt_template = fmtbuf;
 
                f_setfmt(FEadfcb(), &fmt, FMTSTR, &dummy);
        }
 
        f_fmtdate(FEadfcb(), &idbv, &fmt, &odbv);
 
        STlcopy((PTR) datestr.text, outstr, datestr.count);
}

/*{
** Name:	IIVQntlNowToLocal  	Convert current time to local time
**
** Description:
**	Get current time displayed in the same format used for catalog times.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**		Current local time in desired format
**
**	Exceptions:
**		none
**
** Side Effects:
**	This routine uses a static buffer.  Each time it's called the
**	buffer will be overwritten.
**
** History:
**	1/26/90 (Mike S)	Initial Version
*/
char	*
IIVQntlNowToLocal()
{
	/* Get "Now" in catalog form in a buffer */
	STcopy(UGcat_now(), catnowbuf);

	/* Convert it to local time */
	IIVQctlCatToLocal(catnowbuf, locnowbuf);
	return (locnowbuf);
}
