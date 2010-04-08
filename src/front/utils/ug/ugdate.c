/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<tm.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:    ugdate.c -	Front-End Internal Date Conversion Module.
**
** Description:
**	Routines to do conversions between C25 catalog format for dates
**	and DATE internal types.  Defines:
**
**	UGdt_now()	set internal date as "now."
**	UGcat_now()	return "now" as catalog date string.
**	UGdt_to_cat()	convert internal date to catalog date string.
**	UGcat_to_dt()	convert catalog date string to internal date.
**
** History:
**	Revision 6.2  88/05  wong
**	Modified 'Textarea' to be DB_TEXT_STRING to save 'MEfill()' call.
**
**	Revision 6.0  87/05  bobm
**	Initial revision.
**
**	06/03/89 (dkh) - Added changes to account for fact that date_gmt
**			 only convert to a "char" datatype.  UGcat_to_dt
**			 was broken as a result.
**	01/02/90 (dkh) - Changed to only copy AFE_DATESIZE number of bytes
**			 to "dstr" in UGdt_to_cat().  This prevent potential
**			 memory thrashing.
**	02/14/91 (tom) - Added date part function.
**      04/03/93 (smc) - Cast parameter of STlcopy & STlength to match 
**                       prototype. Added forward declaration of err_rtn(). 
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

VOID	UGdt_to_cat();
ADF_CB	*FEadfcb();
static  err_rtn();

static ADF_CB	*Afe_cb = NULL;

/*
** DB_DATA_VALUES to use with ADF
**
**	Textarea is large enough for %d days %d seconds specification
**	of interval, also large enough for a catalog date.
*/
static AFE_DCL_TXT_MACRO(40+1)		Textarea = {0, {EOS}};
static DB_DATA_VALUE	Dstr =
	AFE_DCMP_INIT_MACRO(DB_TXT_TYPE, sizeof(Textarea), (PTR)&Textarea);

static DB_DATA_VALUE	Dchar =
	AFE_DCMP_INIT_MACRO(DB_CHA_TYPE,sizeof(Textarea.text),(PTR)(Textarea.text));

static DB_DATA_VALUE	Dunit =
	AFE_DCMP_INIT_MACRO(DB_CHA_TYPE, 0, (PTR)(Textarea.text));

static DB_DATA_VALUE	Ddate =
	AFE_DCMP_INIT_MACRO(DB_DTE_TYPE, sizeof(DATE), (PTR)NULL);

static DB_DATA_VALUE	Dint =
	AFE_DCMP_INIT_MACRO(DB_DTE_TYPE, sizeof(DATE), (PTR)NULL);

static i2 i2val;

static DB_DATA_VALUE	Dnat =
	AFE_DCMP_INIT_MACRO(DB_INT_TYPE, sizeof(i2), (PTR) &i2val);

static DATE Epdate;

static DB_DATA_VALUE	Epoch =
	AFE_DCMP_INIT_MACRO(DB_DTE_TYPE, sizeof(DATE), (PTR) &Epdate);

static DATE Resdate;

static DB_DATA_VALUE	Dtres =
	AFE_DCMP_INIT_MACRO(DB_DTE_TYPE, sizeof(DATE), (PTR) &Resdate);

static f8 Fval;

static DB_DATA_VALUE	F8res =
	AFE_DCMP_INIT_MACRO(DB_FLT_TYPE, sizeof(f8), (PTR) &Fval);

/*
** set up and save as much of the "date_gmt" function call as we can to
** avoid having to reassign this all the time.
*/
static DB_DATA_VALUE	*Darr[1] = {&Ddate};
static AFE_OPERS	Ops	= {1, Darr};
static ADI_FI_ID	Func_id	= ADI_NOFI;

/*
** same sort of thing for the "-", "+", "interval" and "date_part" call.  
** For interval, we use the Dstr variable for
** the "seconds" argument.  NOTE - MFunc_id = IFunc_id is used
** to key the fact that we need initialization, so be sure they
** are compiler initialized to the same value.  We will use Dtres for
** the result of the minus call, so we set up that DB_DATA_VALUE as
** the operand  for "interval"
*/
static DB_DATA_VALUE	*DMarr[2] 	= { &Ddate, &Epoch };
static AFE_OPERS	MOps		= {2, DMarr};
static ADI_FI_ID	MFunc_id	= ADI_NOFI;

static DB_DATA_VALUE	*DIarr[2] 	= { &Dchar, &Dtres};
static AFE_OPERS	IOps		= {2, DIarr};
static ADI_FI_ID	IFunc_id	= ADI_NOFI;

static DB_DATA_VALUE	*DAarr[2]	= { &Epoch, &Ddate};
static AFE_OPERS	AOps		= {2, DAarr};
static ADI_FI_ID	AFunc_id	= ADI_NOFI;

static DB_DATA_VALUE	*DParr[2]	= { &Dunit, &Dtres};
static AFE_OPERS	POps		= {2, DParr};
static ADI_FI_ID	PFunc_id	= ADI_NOFI;

static VOID	set_glob();
static VOID	set_sysglob();
static VOID	dt_part();

static  char *dp_err = ERx("IIUGdp_DateParts");

/*{
** Name:	UGdt_now() -	Set Internal Date as "now."
**
** Description:
**	Return "now" as an internal date structure.
**
** Inputs:
**
** Outputs:
**	dt - filled in date structure.
**
** History:
**	1/88 (bobm)	abstracted form UGcat_now()
*/
VOID
UGdt_now(dt)
DATE *dt;
{
    if (Afe_cb == NULL)
	set_glob();

    Ddate.db_data = (PTR) dt;

    Textarea.text[0] = 'N';
    Textarea.text[1] = 'O';
    Textarea.text[2] = 'W';
    Textarea.count = 3;

    if (adc_cvinto(Afe_cb, &Dstr, &Ddate) != OK)
	err_rtn(ERx("IIUGdt_now"));
}

/*{
** Name:	UGcat_now() -	Return "now" as Catalog Date String.
**
** Description:
**	Return "now" as an appropriate string.  Invalidates pointer
**	for earlier call.  If caller wants to retain a "now" string,
**	they must save a copy.
**
**	NOTE - IIUGdtsDateToSys also invalidates this buffer.  To be
**		on the safe side, you should assume that all routines
**		in this source file may overwrite the UGcat_now buffer.
**
** Returns:
**	{char *}  A reference to a string containing the date string
**			representing "now."
**
** History:
**	5/87 (bobm)	written
**	1/88 (bobm)	abstracted UGdt_now()
*/

char *
UGcat_now()
{
	DATE	dt;

	UGdt_now(&dt);
	UGdt_to_cat(&dt, (char *)NULL);

	return (char *)Textarea.text;
}

/*{
** Name:	UGdt_to_cat() -	Convert Internal Date to Catalog Date String.
**
** Description:
**	Convert DATE to FE catalog date string.
**
** Inputs:
**	dt	DATE
**	dstr	buffer address
**
** Outputs:
**	dstr	filled in buffer
**
** History:
**	5/87 (bobm)	written
**	01/02/90 (dkh) - Changed to only copy AFE_DATESIZE number of bytes
**			 to "dstr" to prevent memory thrashing.
*/

VOID
UGdt_to_cat (dt, dstr)
DATE	*dt;
char	*dstr;
{
	if (Afe_cb == NULL)
		set_glob();

	Ddate.db_data = (PTR) dt;

	if (afe_clinstd(Afe_cb, Func_id, &Ops, &Dchar) != OK)
		err_rtn (ERx("IIUGdt_to_cat"));

	if ( dstr != NULL )
	{
		STlcopy(Dchar.db_data, dstr, AFE_DATESIZE);
		_VOID_ STtrmwhite(dstr);
	}
}

/*{
** Name:	UGcat_to_dt() -	Convert Catalog Date String to Internal Date.
**
** Description:
**	Convert FE catalog date string to DATE.
**
** Inputs:
**	dstr	date
**	dt	DATE address to fill in
**
** Outputs:
**	dt	filled in
**
** History:
**	5/87 (bobm)	written
*/
VOID
UGcat_to_dt ( dstr, dt )
char	*dstr;
DATE	*dt;
{
    if (Afe_cb == NULL)
	set_glob();

    _VOID_ STlcopy(dstr, (char *)Textarea.text, sizeof(Textarea.text)-1);
    Textarea.count = STlength(dstr);

    Ddate.db_data = (PTR) dt;

    if (adc_cvinto(Afe_cb, &Dstr, &Ddate) != OK)
	err_rtn(ERx("IIUGcat_to_dt"));
}

/*{
** Name:	IIUGdtsDateToSys() -	Convert catalog date to SYSTIME.
**
** Description:
**	Convert FE catalog date string to CL SYSTIME.  If a NULL or
**	empty string is passed in, epoch date ( = "very old" ) will
**	be returned.
**
** Inputs:
**	dstr	date string.
**
** Outputs:
**	st	filled in SYSTIME structure.
**
** History:
**	7/89 (bobm)	written
*/
VOID
IIUGdtsDateToSys( dstr, st )
char *dstr;
SYSTIME *st;
{
    DATE dt;

    st->TM_msecs = 0;
    if (dstr == NULL || *dstr == EOS)
    {
	st->TM_secs = 0;
	return;
    }

    if (Afe_cb == NULL)
	set_glob();

    /*
    ** first time - initialize for "-", "+" and "interval" functions
    */
    if (IFunc_id == MFunc_id)
	set_sysglob();

    /*
    ** convert string to a DATE
    */
    UGcat_to_dt(dstr, &dt);
    Ddate.db_data = (PTR) &dt;

    /*
    ** subtract epoch date - this results in a DATE structure which
    ** will be an interval.
    */
    if (afe_clinstd(Afe_cb, MFunc_id, &MOps, &Dtres) != OK)
	err_rtn(ERx("IIUGdtsDateToSys"));

    /*
    ** call interval("seconds",Dtres) to convert into seconds since epoch
    */
    STcopy(ERx("seconds"), Dchar.db_data);
    Dchar.db_length = 7;
    if (afe_clinstd(Afe_cb, IFunc_id, &IOps, &F8res) != OK)
	err_rtn(ERx("IIUGdtsDateToSys"));
    Dchar.db_length = sizeof(Textarea.text);

    /* whew! */
    st->TM_secs = Fval + 0.01;
}

/*{
** Name:	IIUGdfsDateFromSys() -	Convert SYSTIME to catalog date
**
** Description:
**	Convert SYSTIME to FE catalog date string.
**
** Inputs:
**	st	SYSTIME structure.
**	dstr	buffer address.
**
** Outputs:
**	dstr	date string.  Must point to buffer long enough to
**		hold string.
**
** History:
**	8/89 (bobm)	written
*/
VOID
IIUGdfsDateFromSys( st, dstr )
SYSTIME *st;
char *dstr;
{
    DATE dt;

    if (Afe_cb == NULL)
	set_glob();

    if (IFunc_id == MFunc_id)
	set_sysglob();

    /*
    ** turn seconds into character string seconds interval.
    **
    ** NOTE: we express this as "%d days %d seconds" due to a bug in adt
    ** handling of large numbers of seconds.  Once this is fixed, this
    ** should simply become "%d seconds"
    */
    STprintf((char *)Textarea.text, ERx("%ld days %ld seconds"),
		st->TM_secs / 86400L, st->TM_secs % 86400L);
    Textarea.count = STlength((char *)Textarea.text);
    Ddate.db_data = (PTR) &dt;
    if (adc_cvinto(Afe_cb, &Dstr, &Ddate) != OK)
	err_rtn(ERx("IIUGdfsDateFromSys"));

    /*
    ** add interval to Epoch date, yielding a date result.
    */
    if (afe_clinstd(Afe_cb, AFunc_id, &AOps, &Dtres) != OK)
	err_rtn(ERx("IIUGdfsDateFromSys"));

    /*
    ** convert to catalog string.
    */
    UGdt_to_cat (Dtres.db_data, dstr);
}


/*{
** Name:	IIUGdp_DateParts() -	given a systime return most date parts
**
** Description:
**	Given a system time, return most date parts.
**
** Inputs:
**	st	SYSTIME structure.
**	yr,mon,day,hour,min,sec  - ptr to caller's date parts to return  
**
**
** History:
**	2/13/91 (tom)  - created	
*/
VOID
IIUGdp_DateParts( st, yr, mon, day, hour, min_, sec )
SYSTIME *st;
i4  *yr, *mon, *day, *hour, *min_, *sec;
{
    DATE dt;

    if (Afe_cb == NULL)
    {
	set_glob();
    }

    if (IFunc_id == MFunc_id)
    {
	set_sysglob();
    }

    /*
    ** turn seconds into character string seconds interval.
    **
    ** NOTE: we express this as "%d days %d seconds" due to a bug in adt
    ** handling of large numbers of seconds.  Once this is fixed, this
    ** should simply become "%d seconds"
    */
    STprintf((char *)Textarea.text, ERx("%ld days %ld seconds"),
		st->TM_secs / 86400L, st->TM_secs % 86400L);
    Textarea.count = STlength((char *)Textarea.text);
    Ddate.db_data = (PTR) &dt;
    if (adc_cvinto(Afe_cb, &Dstr, &Ddate) != OK)
    {
	err_rtn(dp_err);
    }

    /*
    ** add interval to Epoch date, yielding a date result.
    */
    if (afe_clinstd(Afe_cb, AFunc_id, &AOps, &Dtres) != OK)
    {
	err_rtn(dp_err);
    }
    /* OK, Dtres now contains the DB datavalue which represents the
       relevent date. now we apply the date part function to extract the
       date parts */

    dt_part(ERx("year"), yr);
    dt_part(ERx("month"), mon);
    dt_part(ERx("day"), day);
    dt_part(ERx("hour"), hour);
    dt_part(ERx("minute"), min_);
    dt_part(ERx("second"), sec);
}

static VOID
dt_part(unit, val) 
char *unit;
i4  *val;
{
    STcopy(unit, Dunit.db_data);
    Dunit.db_length = STlength(unit);
    if (afe_clinstd(Afe_cb, PFunc_id, &POps, &Dnat) != OK)
    {
	err_rtn(dp_err);
    }
    *val = i2val;
}
/*
** local routine for one-time initialization
*/
static VOID
set_glob()
{
    ADI_OP_ID oid;
    ADI_FI_DESC fdesc;

    Afe_cb = FEadfcb();

    /*
    ** get function instance id for date_gmt
    */
    if (afe_opid(Afe_cb, ERx("date_gmt"), &oid) != OK  ||
		afe_fdesc(Afe_cb, oid, &Ops, (AFE_OPERS *)NULL, &Dchar, &fdesc)
			!= OK)
	err_rtn(ERx("IIUGdate(set_glob)"));
    Func_id = fdesc.adi_finstid;
    Dchar.db_length = sizeof(Textarea.text);
}

/*
** added one-time initialization for SYSTIME / date conversions
*/
static VOID
set_sysglob()
{
    ADI_FI_DESC fdesc;
    ADI_OP_ID oid;

    UGcat_to_dt(ERx("1970_01_01 00:00:00 GMT"), &Epdate);

    if (afe_opid(Afe_cb, ERx("-"), &oid) != OK  ||
	afe_fdesc(Afe_cb, oid, &MOps, (AFE_OPERS *)NULL, &Dtres, &fdesc) != OK)
	err_rtn(ERx("IIUGdate(set_sysglob)"));
    MFunc_id = fdesc.adi_finstid;

    if (afe_opid(Afe_cb, ERx("interval"), &oid) != OK  ||
	afe_fdesc(Afe_cb, oid, &IOps, (AFE_OPERS *)NULL, &F8res, &fdesc) != OK)
	err_rtn(ERx("IIUGdate(set_sysglob)"));
    IFunc_id = fdesc.adi_finstid;

    if (afe_opid(Afe_cb, ERx("+"), &oid) != OK  ||
	afe_fdesc(Afe_cb, oid, &AOps, (AFE_OPERS *)NULL, &Dtres, &fdesc) != OK)
	err_rtn(ERx("IIUGdate(set_sysglob)"));
    AFunc_id = fdesc.adi_finstid;

    if (afe_opid(Afe_cb, ERx("date_part"), &oid) != OK  ||
	afe_fdesc(Afe_cb, oid, &POps, (AFE_OPERS *)NULL, &Dnat, &fdesc) != OK)
	err_rtn(ERx("IIUGdate(set_sysglob)"));
    PFunc_id = fdesc.adi_finstid;
}


static
err_rtn(where)
char *where;
{
	FEafeerr(Afe_cb);
	EXsignal(EXFEBUG, 1, where);
}
