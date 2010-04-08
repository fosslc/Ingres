/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <sl.h>
#include <st.h>
#include <iicommon.h>
#include <adf.h>
#include <adudate.h>

/********   taken from adumoney.h    -  since we can't include it here ****/
/***
** Name: AD_MONEYNTRNL - Internal representation for money.
**
** Description:
**        This struct defines the internal representation for money.
***/ 
typedef struct
{
    f8      mny_cents;
} AD_MONEYNTRNL;

static i4  get_date_dfmt();
static i4  get_money_dfmt();
/**
**
**  Name: RAATCNVT.C - RAAT conversion routines for date/money
**
**  Description:
**
**      This file contains routines to convert from a date in string
**      format to the DMF internal format and visa versa, and routines
**      to convert a money string to the DMF internal format and 
**      visa versa.
**
**  routines:
** 
**  IIcraat_strtodate  - converts string to DMF internal date
**  IIcraat_datetostr  - converts DMF internal date to string
**  IIcraat_strtomoney - converts string to DMF internal money
**  IIcraat_moneytostr - converts DMF internal money to string
**
**  internal routines:
** 
**  get_date_dfmt  - reads environment variables to set options
**  get_money_dfmt - reads environment variables to set options
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	17-may-95 (shust01)
**	    changed checking of II_DATE_FORMAT to a case-insensitive
**          compare. fixed bug in get_money_dfmt in call to CVan()
**	23-may-95 (shust01)
**	    added call to TMtz_init() to fix SEGV error when calling 
**          adu_6datetostr().
**	09-aug-95 (shust01)
**	    changed routine names from IIraat_... to IIcraat_... since
**	    IIraat... calls should only be when going to DMF direct, 
**	    which these aren't.
**	10-aug-95 (shust01)
**	    added call to TMtz_init() to fix BUS error when calling 
**          adu_14strtodate().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
**/


/*
** Name: IIcraat_strtodate
**
** Description:
**	converts user-supplied date in string format and converts it to
**	DMF format, which get returned to user.
**
** Inputs:
**	str             date string
**
** Outputs:
**	date            pointer to date in DMF format
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	09-aug-95 (shust01)
**	    changed routine name from IIraat_strtodate to IIcraat_strtodate 
**	    since this routine doesn't call DMF direct. 
**	10-aug-95 (shust01)
**	    added call to TMtz_init() to fix BUS error.
**      06-Jan-97 (merja01)
**          Set adf_scb.adf_constants to NULL.  We don't put an address in 
**          this ptr and it was causing segv in adu_dbconst on axp.osf.
**
*/
int
IIcraat_strtodate(char *str, char *date)
{
    ADF_CB        adf_scb;
    DB_DATA_VALUE adc_dvfrom;
    DB_DATA_VALUE adc_dvinto;
    DB_STATUS     status;

    /* set up info for call */
    adf_scb.adf_dfmt = get_date_dfmt();
    adf_scb.adf_errcb.ad_ebuflen = 0;	
    adf_scb.adf_constants = NULL;
    adc_dvfrom.db_datatype = DB_CHR_TYPE;
    adc_dvfrom.db_prec     = 0;
    adc_dvfrom.db_length   = STlength(str) + 1; /* 1 is for null byte */
    adc_dvfrom.db_data     = str;
    adc_dvinto.db_datatype = DB_DTE_TYPE;
    adc_dvinto.db_prec     = 0;
    adc_dvinto.db_length   = 12;  /* length of internal date */
    adc_dvinto.db_data     = date;

    /* we were hoping to call adc_cvinto() instead of adu_14strtodate()
    since adc_cvinto() is a higher level routine, but too many variables
    (such as Adf_globs) would have to be initialized.   */

    status = TMtz_init(&(adf_scb.adf_tzcb));
    if( status == OK)
       status = adu_14strtodate(&adf_scb, &adc_dvfrom, &adc_dvinto);
/*  status = adc_cvinto(&adf_scb, &adc_dvfrom, &adc_dvinto); */
    return(status == E_DB_OK ? 0 : -1);
}

/*
** Name: IIcraat_datetostr
**
** Description:
**	converts user-supplied date in DMF format and converts it to
**	string format, which get returned to user.
**
** Inputs:
**	date            pointer to date in DMF format
**	len             size of str buffer to contain converted date
**
** Outputs:
**	str             date string
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	23-may-95 (shust01)
**	    added call to TMtz_init() to fix SEGV error when calling 
**          adu_6datetostr().
**	09-aug-95 (shust01)
**	    changed routine name from IIraat_datetostr to IIcraat_datetostr 
**	    since this routine doesn't call DMF direct. 
*/
int
IIcraat_datetostr(char *date, char *str, i4  len)
{
    ADF_CB        adf_scb;
    DB_DATA_VALUE adc_dvfrom;
    DB_DATA_VALUE adc_dvinto;
    DB_STATUS     status;

    /* set up info for call */
    adf_scb.adf_dfmt = get_date_dfmt();
    adf_scb.adf_errcb.ad_ebuflen = 0;
    adc_dvfrom.db_datatype = DB_DTE_TYPE;
    adc_dvfrom.db_prec     = 0;
    adc_dvfrom.db_length   = 12;  /* length of internal date */
    adc_dvfrom.db_data     = date;
    adc_dvinto.db_datatype = DB_CHR_TYPE;
    adc_dvinto.db_prec     = 0;
    adc_dvinto.db_length   = len + 1; /* 1 is for null byte */
    adc_dvinto.db_data     = str;  
    status = TMtz_init(&(adf_scb.adf_tzcb));
    if( status == OK)
       status = adu_6datetostr(&adf_scb, &adc_dvfrom, &adc_dvinto);
/*  status = adc_cvinto(&adf_scb, &adc_dvfrom, &adc_dvinto); */
    return(status == E_DB_OK ? 0 : -1);
}

/*
** Name: IIcraat_moneytostr
**
** Description:
**	converts user-supplied money in DMF format and converts it to
**	string format, which get returned to user.
**
** Inputs:
**	date            pointer to money in DMF format
**	len             size of str buffer to contain converted  money
**
** Outputs:
**	str             money string
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	09-aug-95 (shust01)
**	    changed routine name from IIraat_moneytostr to IIcraat_moneytostr 
**	    since this routine doesn't call DMF direct. 
*/
int
IIcraat_moneytostr(char *money, char *str, i4  len)
{
    ADF_CB        adf_scb;
    DB_DATA_VALUE adc_dvfrom;
    DB_DATA_VALUE adc_dvinto;
    AD_MONEYNTRNL int_money;
    DB_STATUS     status;

    /* set up info for call */
    get_money_dfmt(&adf_scb);
    int_money.mny_cents          = *(f8 *)money;

    adc_dvfrom.db_datatype = DB_MNY_TYPE;
    adc_dvfrom.db_length   = 8;  /* length of internal money */
    adc_dvfrom.db_prec     = 0;
    adc_dvfrom.db_data     = (char *)&int_money;
    adc_dvinto.db_datatype = DB_CHR_TYPE;
    adc_dvinto.db_prec     = 0;
    adc_dvinto.db_length   = len + 1; /* 1 is for null byte */
    adc_dvinto.db_data     = str;  
    status = adu_9mnytostr(&adf_scb, &adc_dvfrom, &adc_dvinto);
    return(status == E_DB_OK ? 0 : -1);
}

/*
** Name: IIcraat_strtomoney
**
** Description:
**	converts user-supplied money in string format and converts it to
**	DMF format, which get returned to user.
**
** Inputs:
**	str             money string
**
** Outputs:
**	money           pointer to money in DMF format
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	09-aug-95 (shust01)
**	    changed routine name from IIraat_strtomoney to IIcraat_strtomoney 
**	    since this routine doesn't call DMF direct. 
*/
int
IIcraat_strtomoney(char *str, char *money)
{
    ADF_CB        adf_scb;
    DB_DATA_VALUE adc_dvfrom;
    DB_DATA_VALUE adc_dvinto;
    AD_MONEYNTRNL int_money;
    DB_STATUS     status;

    /* set up info for call */
    get_money_dfmt(&adf_scb);
    int_money.mny_cents          = *(f8 *)money;

    adc_dvfrom.db_datatype = DB_CHR_TYPE;
    adc_dvfrom.db_prec     = 0;
    adc_dvfrom.db_length   = STlength(str); 
    adc_dvfrom.db_data     = str;
    adc_dvinto.db_datatype = DB_MNY_TYPE;
    adc_dvinto.db_prec     = 0;
    adc_dvinto.db_length   = 8;  /* length of internal money */
    adc_dvinto.db_data     = money;
    status = adu_2strtomny(&adf_scb, &adc_dvfrom, &adc_dvinto);
    return(status == E_DB_OK ? 0 : -1);
}

/*
** Name: get_date_dfmt
**
** Description:
**	reads the II_DATE_FORMAT environment variable and sets the
**	appropriate DB_???_DFMT value.
**
** Inputs:
**	none
**
** Returns:
**	DB_???_DFMT value for appropriate date format.
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	17-may-95 (shust01)
**	    changed checking of II_DATE_FORMAT to a case-insensitive
**          compare
**	24-Jun-1999 (shero03)
**	    Support ISO4.
*/
static i4
get_date_dfmt()
{
    char *datebuf;
    i4  dfmt;

    NMgtAt("II_DATE_FORMAT", &datebuf);
    if (datebuf &&  *datebuf)
    {
       if (!STbcompare("US", 2, datebuf, 2, TRUE))
	  dfmt = DB_US_DFMT;
       else if (!STbcompare("FINLAND", 7, datebuf, 7, TRUE))
	  dfmt = DB_FIN_DFMT;
       else if (!STbcompare("SWEDEN", 6, datebuf, 6, TRUE))
	  dfmt = DB_FIN_DFMT;
       else if (!STbcompare("MULTINATIONAL", 13, datebuf, 13, TRUE))
	  dfmt = DB_FIN_DFMT;
       else if (!STbcompare("ISO4", 4, datebuf, 4, TRUE))
	  dfmt = DB_ISO4_DFMT;
       else if (!STbcompare("ISO", 3, datebuf, 3, TRUE))
	  dfmt = DB_ISO_DFMT;
       else if (!STbcompare("GERMAN", 6, datebuf, 6, TRUE))
	  dfmt = DB_GERM_DFMT;
       else if (!STbcompare("YMD", 3, datebuf, 3, TRUE))
	  dfmt = DB_YMD_DFMT;
       else if (!STbcompare("MDY", 3, datebuf, 3, TRUE))
	  dfmt = DB_MDY_DFMT;
       else if (!STbcompare("DMY", 3, datebuf, 3, TRUE))
	  dfmt = DB_DMY_DFMT;
       else
	  dfmt = DB_US_DFMT; /* hopefully, should never happen */
    }
    else
       dfmt = DB_US_DFMT;
    return(dfmt);
}
/*
** Name: get_money_dfmt
**
** Description:
**	reads the II_MONEY_FORMAT and II_MONEY_PREC environment variables
**	and sets db_mny_prec, db_mny_lort and db_mny_sym in ADF_CB 
**      control block.
**
** Inputs:
**	adf_scb     pointer to ADF_CB control block
**
** Returns:
**	adf_scb
**         adf_mfmt
**            db_mny_prec = number of decimal digits for money
**            db_mny_lort = location of currency symbol
**            db_mny_sym  = currency symbol to be used
**
** History:
**	10-may-95 (shust01)
**	    First written.
**	17-may-95 (shust01)
**          fixed bug in get_money_dfmt in call to CVan()
**	01-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE for Sir 92541.
*/
static i4  
get_money_dfmt(ADF_CB *adf_scb)
{
    char *moneybuf;
    i4   prec; 
    char loc;
    char sym[5];

    NMgtAt("II_MONEY_FORMAT", &moneybuf);
    if (moneybuf && *moneybuf)
    {
       if (*moneybuf == 'T')
	  loc = DB_TRAIL_MONY;
       else if (*moneybuf == 'L')
	  loc = DB_LEAD_MONY; 
       else if (*moneybuf == 'N')
       {	
       	  loc = DB_NONE_MONY;
	  sym[0] = EOS;
       }
       if ((loc == DB_TRAIL_MONY) || (loc == DB_LEAD_MONY))
       {	
           while ((*moneybuf != '\0') && (*moneybuf != ':'))
	      moneybuf++;
           if (*moneybuf)
	      STcopy(moneybuf+1, sym);
           else
	      STcopy("$", sym);
       }
    }
    else /* if II_MONEY_FORMAT not set */
    {
       loc = DB_LEAD_MONY; 
       STcopy("$", sym);
    }

    NMgtAt("II_MONEY_PREC", &moneybuf);
    if (moneybuf && *moneybuf)
    {
       CVan(moneybuf, &prec);
       if (prec < 0 || prec > 2)
	  prec = 2;
    }
    else
       prec = 2;
    adf_scb->adf_errcb.ad_ebuflen = 0;
    adf_scb->adf_mfmt.db_mny_prec = prec;
    adf_scb->adf_mfmt.db_mny_lort = loc;
    STcopy(sym, adf_scb->adf_mfmt.db_mny_sym);
    adf_scb->adf_decimal.db_decspec = FALSE; /* not specifying separator */
}
