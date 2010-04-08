/*
** Copyright (c) 1987, 2001, 2008i, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <me.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adudate.h>
#include    <adutime.h>
/*
[@#include@]...
*/

/**
**
**  Name: ADUDBMSINFO.C - Holds routines to process INGRES dbmsinfo() function.
**
**  Description:
**      This file contains all routines needed to process the dbmsinfo()
**      function.
**
**          adu_dbconst() - Routine to get/calculate value of query constants.
**          adu_dbmsinfo() - INGRES's dbmsinfo() function.
[@func_list@]...
**
**
**  History:
**      05-apr-87 (thurston)
**          Initial creation.
**      07-apr-87 (thurston)
**          Coded adu_dbmsinfo() to look up entry in dbmsinfo() request table,
**          and use the function found there.
**      28-sep-87 (thurston)
**          Added adu_dbconst().
**      11-nov-87 (thurston)
**          Fixed small bug in adu_dbmsinfo() when looking up the DBI request
**	    name in the table of DBI's.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	01-sep-88 (thurston)
**	    Increased size of local buf in adu_dbmsinfo() from DB_MAXSTRING+1
**	    to DB_GW4_MAXSTRING+1.
**	07-aug-1990 (jonb)
**	    Corrected the order of parameters to MEcopy.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	11-jun-2001 (somsa01)
**	    In adu_dbmsinfo(), to prevent excessive stack use, we now
**	    dynamically allocate memory for tbuf.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
**	18-Feb-2008 (kiria01) b120004
**	    Consolidate timezone handling
[@history_template@]...
**/


/*
[@forward_type_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_dbconst() -  Get ADF query constants.
**
** Description:
**      This routine gets the value of one of the ADF query constants.  These 
**      are the values that are supposed to stay constant for the life of the
**	query, such as `_cpu_ms', `_bintim' (or `date("now")'), _dio_cnt, etc.
**      This routine first checks to see if the value has already been
**	calculated, and if so it uses the pre-calculated value.  If not, it
**      calculates the value and attempts to mark it as `calculated'.
[@comment_line@]...
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      kmap                            Pointer to the ADK_MAP structure
**                                      pertaining to this query constant.
**                                      This structure holds the mapping of
**                                      dbmsinfo() request to ADF query
**                                      constant bit. 
**          .adk_dbi			Associated dbmsinfo() request record.
**          .adk_kbit                   Associated ADF query constant bit.
**      dbi                             Pointer to the dbmsinfo() request record
**                                      for this request.  This is an
**                                      ADF_DBMSINFO structure, and contains
**                                      amoung other things the pointer to the
**                                      function to call that will evaluate this
**                                      item. 
**      rdv                             Pointer to DB_DATA_VALUE to hold result.
**	    .db_datatype		Its datatype.
**	    .db_length			Its length.
**	    .db_data			Location to place result.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      rdv				Pointer to DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    E_DB_OK		All OK.
**	    E_DB_ERROR		Something screwy has occurred.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    If an ADF constant block is pointed to by the ADF session CB, it
**	    may be updated to hold a newly calculated query constant.
**
** History:
**      28-sep-87 (thurston)
**          Initial creation.
**	29-aug-2006 (gupsh01)
**	    Added support for ANSI datetime constants CURRENT_DATE, 
**	    CURRENT_TIME, CURRENT_TIMESTAMP, LOCAL_TIME, 
**	    LOCAL_TIMESTAMP.
**	06-sep-2006 (gupsh01)
**	    Tidy up the comments, return the actual date/time datatype
**	    and fix local_time/local_timezone calculations.
**	22-sep-2006 (gupsh01)
**	    Fix printing of CURRENT_TIME and CURRENT_TIMESTAMP.
**	06-oct-2006 (gupsh01)
**	    CURRENT_DATE must show local date not GMT date.
**	18-Apr-2008 (kiria01) b120284
**	    Timezone adjustment was made incorrectly for all date types.
**	    Status flags have been made consistant with TZ changes
**	    and the ADTE date gmt input fixup applied as per adu_6to_dtntrnl.
**	    Removed the unused date buffers and their setup code.
**	11-May-2008 (kiria01) b120004
**	    Dropped above ADTE date gmt input fixup as now applied in a
**	    deffered manner if needed (generally not)
**	18-Dec-2008 (kiria01) SIR 121297
**	    Cater for i8 and cover for potential abends and data size
**	    differences.
**	02-Feb-2009 (kiria01) b121570
**	    Reordered the actions in this routine to make more sense and
**	    removed most of the assumptions about data lengths in the
**	    constant buffers. This is needed as some of the scu routines
**	    do not check a request length but simply change the callers
**	    request.
[@history_template@]...
*/

DB_STATUS
adu_dbconst(
ADF_CB		    *adf_scb,
ADK_MAP		    *kmap,
DB_DATA_VALUE       *rdv)
{
    ADK_CONST_BLK	*k = adf_scb->adf_constants;
    bool		kcalced = FALSE;
    PTR			kptr = NULL;
    DB_ERROR		err;
    ADF_DBMSINFO	*dbi  = kmap->adk_dbi;
    i4			kbit  = kmap->adk_kbit;
    DB_STATUS		db_stat;
    bool		datetimetype = FALSE;
    DB_DT_ID		date_dt;
    u_i4		val;
    /* Beware that some of the dbmsinfo functions care not about
    ** caller lengths and will update the DBV regardless so
    ** we arrange to get the constants written directy and if
    ** need be, we conver on return. */
    DB_DATA_VALUE tmp_rdv = *rdv;
    i8 tmp_buf[1024/sizeof(i8)]; /* Make temporary aligned */

    switch (kbit)	    /* look for datetime constants */
    {
    case ADK_CURR_DATE:		/* _current_date */
	datetimetype = TRUE;
	date_dt = DB_ADTE_TYPE;
	break;

    case ADK_CURR_TIME:		/* _current_time */
	datetimetype = TRUE;
	date_dt = DB_TMW_TYPE;
	break;

    case ADK_CURR_TSTMP:	/* _current_timestamp */
	datetimetype = TRUE;
	date_dt = DB_TSW_TYPE;
	break;

    case ADK_LOCAL_TIME:	/* _local_time*/
	datetimetype = TRUE;
	date_dt = DB_TME_TYPE;
	break;

    case ADK_LOCAL_TSTMP:	/* _local_timestamp */
	datetimetype = TRUE;
	date_dt = DB_TSTMP_TYPE;
	break;
    }

    if (k != NULL)
    {
	switch (kbit)	    /* switch on known query constants */
	{
	  case ADK_BINTIM:	/* _bintim */
	    tmp_rdv.db_data = (PTR) &k->adk_bintim;
	    tmp_rdv.db_length = sizeof(k->adk_bintim);
	    break;

	  case ADK_CPU_MS:	/* _cpu_ms */
	    tmp_rdv.db_data = (PTR) &k->adk_cpu_ms;
	    tmp_rdv.db_length = sizeof(k->adk_cpu_ms);
	    break;

	  case ADK_ET_SEC:	/* _et_sec */
	    tmp_rdv.db_data = (PTR) &k->adk_et_sec;
	    tmp_rdv.db_length = sizeof(k->adk_et_sec);
	    break;

	  case ADK_DIO_CNT:	/* _dio_cnt */
	    tmp_rdv.db_data = (PTR) &k->adk_dio_cnt;
	    tmp_rdv.db_length = sizeof(k->adk_dio_cnt);
	    break;

	  case ADK_BIO_CNT:	/* _bio_cnt */
	    tmp_rdv.db_data = (PTR) &k->adk_bio_cnt;
	    tmp_rdv.db_length = sizeof(k->adk_bio_cnt);
	    break;

	  case ADK_PFAULT_CNT:	/* _pfault_cnt */
	    tmp_rdv.db_data = (PTR) &k->adk_pfault_cnt;
	    tmp_rdv.db_length = sizeof(k->adk_pfault_cnt);
	    break;

	  case ADK_CURR_DATE:	/* _current_date */
	    tmp_rdv.db_data = (PTR) &k->adk_curr_date;
	    tmp_rdv.db_length = sizeof(k->adk_curr_date);
	    break;

	  case ADK_CURR_TIME:	/* _current_time */
	    tmp_rdv.db_data = (PTR) &k->adk_curr_time;
	    tmp_rdv.db_length = sizeof(k->adk_curr_time);
	    break;

	  case ADK_CURR_TSTMP:	/* _current_timestamp */
	    tmp_rdv.db_data = (PTR) &k->adk_curr_tstmp;
	    tmp_rdv.db_length = sizeof(k->adk_curr_tstmp);
	    break;

	  case ADK_LOCAL_TIME:	/* _local_time*/
	    tmp_rdv.db_data = (PTR) &k->adk_local_time;
	    tmp_rdv.db_length = sizeof(k->adk_local_time);
	    break;

	  case ADK_LOCAL_TSTMP:	/* _local_timestamp */
	    tmp_rdv.db_data = (PTR) &k->adk_local_tstmp;
	    tmp_rdv.db_length = sizeof(k->adk_local_tstmp);
	    break;

	  default:
	    return (adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "dbmsinfo kbit"));
	}
	
	if (k->adk_set_mask & kbit) /* Has this value been calculated yet? */
	    kcalced = TRUE;
    }
    else
	tmp_rdv.db_data = (PTR)tmp_buf;


    if (!kcalced)
    {
	/* Must calculate value from scratch */

	if (datetimetype)
	{
	    AD_NEWDTNTRNL       dn;
	    struct timevect     tv;
	    DB_STATUS           db_stat;
	    i4			sec_time_zone;
	    HRSYSTIME		hrsystime;

	    TMhrnow(&hrsystime);

	    MEfill ((u_i2) sizeof(i4)*10, NULLCHAR, (PTR) &tv);

	    adu_cvtime((i4) hrsystime.tv_sec, (i4)hrsystime.tv_nsec, &tv);

	    MEfill((u_i2) sizeof(AD_NEWDTNTRNL), NULLCHAR, (PTR) &dn);
	    /* Interpret the time vector */
	    dn.dn_year    = (i2)tv.tm_year + 1900;
	    dn.dn_month   = (i2)tv.tm_mon + 1;
	    dn.dn_day     = tv.tm_mday;
	    dn.dn_seconds = tv.tm_hour * 3600 + tv.tm_min * 60 + tv.tm_sec;
	    dn.dn_nsecond = tv.tm_nsec;
	    dn.dn_dttype  = date_dt;

	    sec_time_zone = TMtz_search(adf_scb->adf_tzcb, TM_TIMETYPE_GMT,
				 (i4) hrsystime.tv_sec);

	    AD_TZ_SETNEW(&dn, sec_time_zone);

	    dn.dn_status = AD_DN_ABSOLUTE;
	    switch (date_dt)
	    {
	    case DB_ADTE_TYPE:	/* _current_date */
		dn.dn_seconds = 0;
		dn.dn_nsecond = 0;
		dn.dn_status2 |= AD_DN2_ADTE_TZ;
		dn.dn_status |= AD_DN_YEARSPEC|AD_DN_MONTHSPEC|AD_DN_DAYSPEC;
		break;

	    case DB_TME_TYPE:	/* _local_time*/
		dn.dn_status2 |= AD_DN2_TZ_OFF_LCL;
		/*FALLTHROUGH*/
	    case DB_TMW_TYPE:	/* _current_time */
		dn.dn_status |= AD_DN_TIMESPEC;
		dn.dn_status2 |= AD_DN2_NO_DATE;
		break;

	    case DB_TSTMP_TYPE:	/* _local_timestamp */
		dn.dn_status2 |= AD_DN2_TZ_OFF_LCL;
		/*FALLTHROUGH*/
	    case DB_TSW_TYPE:	/* _current_timestamp */
		dn.dn_status |= AD_DN_YEARSPEC|AD_DN_MONTHSPEC|AD_DN_DAYSPEC;
		dn.dn_status |= AD_DN_TIMESPEC;
		break;
	    }

	    /* Convert straight into constant data area */
	    if (db_stat = adu_7from_dtntrnl (adf_scb, &tmp_rdv, &dn))
		return (db_stat);
	}
	else if (dbi == NULL  ||  dbi->dbi_func == NULL)
	{
	    /* If no function ptr in dbmsinfo array, set default value */
	    if (db_stat = adc_getempty(adf_scb, &tmp_rdv))
		return (db_stat);
	}
	else
	{
	    /* Get the value from the proper dbmsinfo function */

	    /*
	    ** NOTE: If the dbmsinfo routines ever change the size of
	    ** of the returned object, it will be assumed that the length
	    ** will match the adk_* buffer length. If not, there will
	    ** be a risk of uninitialised data slipping in.
	    */
	    if (db_stat = (*dbi->dbi_func)(dbi, NULL, &tmp_rdv, &err))
		return (db_stat);
	}

	/* We now have the calculated result written directly to the
	** constant buffers if they exist or to the tmp_buf.
	** Either way, we don't need to write the constant again, we just
	** need to convert out to the return buffer/dbv.
	*/

	/* Don't forget to set the `already calculated' bit */
	if (k)
	    k->adk_set_mask |= kbit;
    }

    /* We have the value requested, so just set result and return */
    if (rdv->db_datatype == DB_INT_TYPE)
    {	/* Currently, all query constants are uints or dates */
	u_i8 val;
	switch (tmp_rdv.db_length)
	{
	case 8:
	    val = (u_i8)*(u_i8*)tmp_rdv.db_data;
	    break;
	case 4:
	    val = (u_i8)*(u_i4*)tmp_rdv.db_data;
	    break;
	case 2:
	    val = (u_i8)*(u_i2*)tmp_rdv.db_data;
	    break;
	case 1:
	    val = (u_i8)*(u_i1*)tmp_rdv.db_data;
	    break;
	}
	switch (rdv->db_length)
	{
	case 8:
	    I8ASSIGN_MACRO(val, *(u_i8 *)rdv->db_data);
	    break;
	case 4:
	    {
		u_i4 val4 = (u_i4)val;
		I4ASSIGN_MACRO(val4, *(u_i4 *)rdv->db_data);
	    }
	    break;
	case 2:
	    {
		u_i2 val2 = (u_i2)val;
		I2ASSIGN_MACRO(val2, *(i2*)rdv->db_data);
	    }
	    break;
	case 1:
	    *(u_i1 *)rdv->db_data = (u_i1)val;
	    break;
	default:
	    return (adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "dbmsinfo isz"));
	}
    }
    else if (tmp_rdv.db_length != rdv->db_length)
    {
	return (adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "dbmsinfo len"));
    }
    else
    {
	MEcopy(tmp_rdv.db_data, rdv->db_length, rdv->db_data);
    }
    return (E_DB_OK);
}


/*{
** Name: adu_dbmsinfo() - INGRES's dbmsinfo() function.
**
** Description:
**      INGRES's dbmsinfo() function.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype. 
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-apr-87 (thurston)
**          Initial creation.
**      07-apr-87 (thurston)
**          Coded to look up entry in dbmsinfo() request table, and use the
**          function found there.
**      11-nov-87 (thurston)
**          Fixed small bug when looking up the DBI request name in the table of
**	    DBI's.  If two names of unequal length were in the tables such that
**          they were identical up to the length of the shorter, and the shorter
**	    one came first, then the user requesting the longer name would
**	    erroneously always get the shorter one.  Example:  Assume the names
**	    "database" (get name of database) and "database_open_cnt" (get the
**          number of sessions that have the current database open) were both in
**          the DBI table, and in that order.  A TM user, in database "whatever"
**	    attempts to find out how many others are using database "whatever"
**	    by doing:
**			retrieve (x = dbmsinfo("database_open_cnt"))
**	    Much to this user's suprise, the dbmsinfo() function was
**	    errorneously finding the DBI request for "database", and returning:
**			+--------------------------------+
**			|x                               |
**			+--------------------------------+
**			|whatever                        |
**			+--------------------------------+
**	    This is the bug that was just fixed.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	01-sep-88 (thurston)
**	    Increased size of local buf from DB_MAXSTRING+1 to
**	    DB_GW4_MAXSTRING+1.
**	22-jan-90 (jrb)
**	    Aligned tbuf since it will be the db_data pointer of a db_data_value
**	    and changed its size to DB_GW4_MAXSTRING+DB_CNTSIZE.
**	11-jun-2001 (somsa01)
**	    In adu_dbmsinfo(), to prevent excessive stack use, we now
**	    dynamically allocate memory for tbuf.
**	12-aug-02 (inkdo01)
**	    Modification to above to impose MEreqmem overhead only if string is
**	    bigger than 2004.
[@history_template@]...
*/

DB_STATUS
adu_dbmsinfo(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *rdv)
{
    i4			i = Adf_globs->Adi_num_dbis;
    i4			found = FALSE;
    ADF_DBMSINFO	*dbi = Adf_globs->Adi_dbi;
    i4			dbi_size = sizeof(dbi->dbi_reqname);
    i4			in_size  = ((DB_TEXT_STRING *)dv1->db_data)->db_t_count;
    char		*in_str  = (char *)
				    ((DB_TEXT_STRING *)dv1->db_data)->db_t_text;
    register i4	j;
    register char	*c1;
    register char	*c2;
    char		ch_tmp;
    ALIGN_RESTRICT	*tbuf;
    DB_STATUS		db_stat = E_DB_OK;
    i4			cmp;
    DB_DATA_VALUE	tmp_dv;
    DB_ERROR		err;
    ADK_MAP		*kmap;
    char		localbuf[2004];
    bool		uselocal = TRUE;


    if (in_size <= dbi_size)    /* No possible match if input is bigger */
    {
	/* Find the request in the dbmsinfo() request table */
	while (i--)
	{
	    cmp = TRUE;
	    c1  = dbi->dbi_reqname;
	    c2  = in_str;
	    j   = 0;
	    while (j < in_size  &&  *c1 != 0)
	    {
		CMtolower(c2, &ch_tmp);
		if (*c1 != ch_tmp)
		{
		    cmp = FALSE;
		    break;
		}
		c1++;
		c2++;
		j++;
	    }

	    if (    cmp
		&& (in_size == dbi_size  ||  (*c1 == 0  &&  j == in_size))
	       )
	    {
		found = TRUE;
		break;
	    }

	    dbi++;
	}
    }


    if (!found)
    {
	((DB_TEXT_STRING *)rdv->db_data)->db_t_count = 0;
	db_stat = E_DB_OK;
    }
    else
    {
	/* {@fix_me@} ... eventually, we have to handle 1 input, and
	**		    lenspecs that are not FIXED LENGTH
	*/
	if (dbi->dbi_num_inputs || dbi->dbi_lenspec.adi_lncompute != ADI_FIXED)
	    return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

	tmp_dv.db_datatype = dbi->dbi_dtr;
	tmp_dv.db_length   = dbi->dbi_lenspec.adi_fixedsize;

	if (tmp_dv.db_length >= 2004)
	{
	    uselocal = FALSE;
	    tbuf = (ALIGN_RESTRICT *)MEreqmem(0,
					  ((DB_GW4_MAXSTRING + DB_CNTSIZE - 1) /
					  sizeof(ALIGN_RESTRICT)) + 1,
					  FALSE, NULL);
	    tmp_dv.db_data     = (PTR)tbuf;
	}
	else tmp_dv.db_data     = (PTR)&localbuf[0];

	/*
	** Is request one of the query constants?  If so, use adu_dbconst()
	** instead of the dbi function found in the dbi table.
	*/
	if      (dbi == Adf_globs->Adk_bintim_map.adk_dbi)
	    kmap = &Adf_globs->Adk_bintim_map;
	else if (dbi == Adf_globs->Adk_cpu_ms_map.adk_dbi)
	    kmap = &Adf_globs->Adk_cpu_ms_map;
	else if (dbi == Adf_globs->Adk_et_sec_map.adk_dbi)
	    kmap = &Adf_globs->Adk_et_sec_map;
	else if (dbi == Adf_globs->Adk_dio_cnt_map.adk_dbi)
	    kmap = &Adf_globs->Adk_dio_cnt_map;
	else if (dbi == Adf_globs->Adk_bio_cnt_map.adk_dbi)
	    kmap = &Adf_globs->Adk_bio_cnt_map;
	else if (dbi == Adf_globs->Adk_pfault_cnt_map.adk_dbi)
	    kmap = &Adf_globs->Adk_pfault_cnt_map;
	else if (dbi == Adf_globs->Adk_curr_date_map.adk_dbi)
	    kmap = &Adf_globs->Adk_curr_date_map;
	else if (dbi == Adf_globs->Adk_curr_time_map.adk_dbi)
	    kmap = &Adf_globs->Adk_curr_time_map;
	else if (dbi == Adf_globs->Adk_curr_tstmp_map.adk_dbi)
	    kmap = &Adf_globs->Adk_curr_tstmp_map;
	else if (dbi == Adf_globs->Adk_local_time_map.adk_dbi)
	    kmap = &Adf_globs->Adk_local_time_map;
	else if (dbi == Adf_globs->Adk_local_tstmp_map.adk_dbi)
	    kmap = &Adf_globs->Adk_local_tstmp_map;
	else
	    kmap = NULL;

	if (kmap == NULL)
	    db_stat = (*dbi->dbi_func)(dbi, NULL, &tmp_dv, &err);
	else
	    db_stat = adu_dbconst(adf_scb, kmap, &tmp_dv);
	
	if (DB_SUCCESS_MACRO(db_stat))
	{
	    db_stat = adu_ascii(adf_scb, &tmp_dv, rdv);
	}

	if (!uselocal)
	    MEfree((PTR)tbuf);
    }

    return (db_stat);
}

/*
[@function_definition@]...
*/
