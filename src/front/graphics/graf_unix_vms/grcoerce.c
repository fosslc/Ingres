
/*
**	grcoerce.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<fmt.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include	<afe.h>
# include	<ergr.h>
# include	<er.h>
# include	<me.h>
# include	<ug.h>
# include	<dvect.h>
# include	"qry.h"
# include	"grcoerce.h"

extern ADF_CB *GR_cb;

/*
** set up as much of the "date_part" function call as we can to
** avoid having to reassign this all the time.
*/

static DB_DATA_VALUE Op_dpart = AFE_DCMP_INIT_MACRO(DB_CHA_TYPE,5,NULL);
static DB_DATA_VALUE *Op_db[2] = { &Op_dpart, NULL };
static AFE_OPERS Ops = { 2, Op_db };

/*
** set up a "convert to string" DB_DATA_VALUE to use
*/
static GRC_STORE Dsa;
static DB_DATA_VALUE Dstr = AFE_DCMP_INIT_MACRO(DB_LTXT_TYPE,GRC_SLEN-1,(PTR) &Dsa);

/*
** format for date string conversion for fields / legends.  Df_str is
** set to NULL after Dfmt has been built.
*/
static FMT *Dfmt;
static char *Df_str = ERx("d\"3-feb-1901\"");

/**
** Name: grcoerce.c - graf library type coercion routine.
**
** Description:
**	This are the routines used to coerce types when mapping a
**	query to a graph:
**
**	GRcoerce_type: coerce a database type into internal types
**	used to produce graphics data, and optionally, into
**	a string.
**
**	GRzero_pt: produce a data value for use in filling in "empty"
**	spots when dense data must be constructed.
**
** History:
**	2/87 (bobm)	written
**	20-may-88 (bruceb)
**		Changed MEalloc call into MEreqmem call.
**	27-jun-89 (Mike S)
**		Call fmt_format with the right number of args.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: GRcoerce_type - coerce into a graphics type.
**
** Description:
**	Uses a GRDV code to perform a conversion of a database value into
**	the expected type for graphics use.  It will also produce a string
**	from the database value if desired.  The caller allocates all
**	storage, using the "grcoerce.h" constants which determine how much
**	is needed.  Note that "Date" type is converted into the integer
**	YYMMDD format used for VE plotting routines, with intervals returned
**	as a number < GRC_INTERVAL.   This is a number < than any Ingres date
**	Post 1999 dates are passed back as numbers >= 1000101.	This works
**	in VE plot routines.
**
** Inputs:
**	col	COLDATA structure correspoding to graphics type desired.
**	source	source DB_DATA_VALUE (database type)
**
** Outputs:
**	targ	target DB_DATA_VALUE to be produced, based on type.
**	str	returned string.  If NULL, don't produce string.
**
** Exceptions:
**	fatal errors for bad codes/unexpected failures, which are bugs
**
** Returns:
**	VOID
**
** History:
**	8/86 (bobm)	written
*/

VOID
GRcoerce_type(col, source, targ, str)
COLDATA		*col;
DB_DATA_VALUE	*source;
DB_DATA_VALUE	*targ;
char		*str;
{
    i4	erarg;
    STATUS	rc;
    i4		nullchk;
    i4		month, day, year;

    DB_TEXT_STRING	*sdata;

    PTR		area;
    i4		size;

    /*
    ** dates are special.  Only types which "date_part" applies to are
    ** converted to this, and we use that to get the pieces of the date the
    ** VE integer format.  This if clause returns.
    */
    if (col->type == GRDV_DATE)
    {
	/*
	** first time, build format.  Need to PERMANENTLY allocate
	** area, so we call ME direct (untagged).
	*/
	if (Df_str != NULL)
	{
	    if ((rc = fmt_areasize(GR_cb,Df_str,&size)) == OK)
	    {
		if ((area = MEreqmem((u_i4)0, (u_i4)size, (bool)FALSE,
		    &rc)) != NULL)
		{
		    rc = fmt_setfmt(GR_cb,Df_str,area,&Dfmt,NULL);
		}
	    }
	    if (rc != OK)
		IIUGerr(S_GR0002_format_error, UG_ERR_FATAL, 0);
	    Df_str = NULL;
	}

	targ->db_datatype = DB_INT_TYPE;
	targ->db_length = sizeof(i4);

	if (AFE_NULLABLE_MACRO(source->db_datatype))
	{
	    AFE_MKNULL_MACRO(targ);
	    if (adc_isnull(GR_cb, source, &nullchk) != OK)
		IIUGerr(S_GR0003_adc_isnull, UG_ERR_FATAL, 0);
	}
	else
	    nullchk = FALSE;

	/* null date becomes null integer */
	if (nullchk)
	{
	    rc = adc_getempty(GR_cb,targ);
	    if (rc == OK && str != NULL)
	    {
		sdata = (DB_TEXT_STRING *) Dstr.db_data;
		if((rc = afe_dpvalue(GR_cb,source,&Dstr)) == OK ||
			GR_cb->adf_errcb.ad_errcode == E_AF5000_VALUE_TRUNCATED)
		{
		    rc = OK;
		    (sdata->db_t_text)[sdata->db_t_count] = EOS;
		    STcopy(sdata->db_t_text,str);
		}
	    }
	    if (rc != OK)
		IIUGerr(S_GR0004_adc_call, UG_ERR_FATAL, 0);
	    return;
	}

	/*
	** non-null.  Get mo / day / year parts and construct integer.
	*/
	Op_dpart.db_data = ERx("month");
	Op_dpart.db_length = 5;
	Op_db[1] = source;
	if ((rc = afe_clinstd(GR_cb, col->funcid, &Ops, &(col->res))) == OK)
	{
	    month = *((i2 *)(col->res.db_data));
	    Op_dpart.db_data = ERx("day");
	    Op_dpart.db_length = 3;
	    if ((rc = afe_clinstd(GR_cb, col->funcid, &Ops, &(col->res))) == OK)
	    {
		day = *((i2 *)(col->res.db_data));
		Op_dpart.db_data = ERx("year");
		Op_dpart.db_length = 4;
		if ((rc=afe_clinstd(GR_cb,col->funcid,&Ops,&(col->res))) == OK)
		{
		    year = *((i2 *)(col->res.db_data));
		    *((i4 *)(targ->db_data)) = VE_DATE(month,day,year);
		    if (str != NULL)
		    {
			if (fmt_format(GR_cb,Dfmt,source,&Dstr,TRUE) != OK)
			    IIUGerr(S_GR0005_fmt_fail, UG_ERR_FATAL, 0);
			sdata = (DB_TEXT_STRING *) Dstr.db_data;
			(sdata->db_t_text)[sdata->db_t_count] = EOS;
			STcopy(sdata->db_t_text,str);
		    }
		}
	    }
	}

	if (rc != OK)
	    IIUGerr(S_GR0006_afe_clinstd, UG_ERR_FATAL, 0);

	return;
    }

    /*
    ** for other types, just use the "datatype" portion of the
    ** type to coerce appropriately.
    */
    switch(col->type & DVEC_MASK)
    {
    case DVEC_I:
	targ->db_datatype = DB_INT_TYPE;
	targ->db_length = sizeof(i4);
	if (AFE_NULLABLE_MACRO(source->db_datatype))
		AFE_MKNULL_MACRO(targ);
	rc = adc_cvinto(GR_cb,source,targ);
	break;
    case DVEC_F:
	targ->db_datatype = DB_FLT_TYPE;
	targ->db_length = sizeof(f8);
	if (AFE_NULLABLE_MACRO(source->db_datatype))
		AFE_MKNULL_MACRO(targ);
	rc = adc_cvinto(GR_cb,source,targ);
	break;
    case DVEC_S:
	/*
	** if a string type, we do the same coercion for the str fill
	** in, if it was desired.  Save making another call by just
	** copying the string, then NULL'ing str argument.
	*/
	targ->db_datatype = DB_LTXT_TYPE;
	targ->db_length = GRC_SLEN - 1;
	sdata = (DB_TEXT_STRING *) targ->db_data;
	if((rc = afe_dpvalue(GR_cb,source,targ)) == OK ||
			GR_cb->adf_errcb.ad_errcode == E_AF5000_VALUE_TRUNCATED)
	{
	    rc = OK;
	    (sdata->db_t_text)[sdata->db_t_count] = EOS;
	    if (str != NULL)
	    {
		STcopy(sdata->db_t_text,str);
		str = NULL;
	    }
	}
	break;
    default:
	erarg = col->type;
	IIUGerr(S_GR0007_bad_type, UG_ERR_FATAL, 1, &erarg);
	break;
    }

    /*
    ** if no errors thus far, and we asked for str, do it
    */
    if (rc == OK && str != NULL)
    {
	sdata = (DB_TEXT_STRING *) Dstr.db_data;
	if((rc = afe_dpvalue(GR_cb,source,&Dstr)) == OK ||
			GR_cb->adf_errcb.ad_errcode == E_AF5000_VALUE_TRUNCATED)
	{
	    rc = OK;
	    (sdata->db_t_text)[sdata->db_t_count] = EOS;
	    STcopy(sdata->db_t_text,str);
	}
    }

    if (rc != OK)
	IIUGerr(S_GR0008_adc_fail, UG_ERR_FATAL, 0);
}

/*{
** Name: GRzero_pt - produce a "zero" datavalue.
**
** Description:
**	Produce a datavalue for use in filling in data which is required
**	to be "dense" for proper graphical representation.  This will
**	normally be a "zero" value, in order to produce invisible bars.
**	With some types this may not be possible.  The minimum y value
**	may be used in producing it.  The caller allocates storage.  The
**	miny value is of the DB_ type that "type" gets coerced into.
**
** Inputs:
**	type	GRDV type.
**	miny	minimum y value collected.
**
** Outputs:
**	zero	produced zero value.
**
** Returns:
**	VOID
**
** Exceptions:
**	fatal errors for conditions which indicate coding errors.
**
** History:
**	8/86 (bobm)	written
*/

VOID
GRzero_pt(type, miny, zero)
i4		type;
DB_DATA_VALUE	*miny;
DB_DATA_VALUE	*zero;
{
    DB_TEXT_STRING	*sdata;
    i4		erarg;

    zero->db_datatype = miny->db_datatype;
    zero->db_length = miny->db_length;

    /*
    ** for dates, copy the minimum y value
    */
    if (type == GRDV_DATE)
    {
	MEcopy(miny->db_data, (i2) miny->db_length, zero->db_data);
	return;
    }

    switch(type & DVEC_MASK)
    {
    case DVEC_S:
	sdata = (DB_TEXT_STRING *) zero->db_data;
	sdata->db_t_count = 1;
	STcopy(ERx(" "),sdata->db_t_text);
	break;
    case DVEC_I:
	*((i4 *)(zero->db_data)) = 0;
	break;
    case DVEC_F:
	*((f8 *)(zero->db_data)) = 0.0;
	break;
    default:
	erarg = type;
	IIUGerr(S_GR0009_bad_type, UG_ERR_FATAL, 1, &erarg);
	break;
    }
}
