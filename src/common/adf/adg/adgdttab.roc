/*
** Copyright (c) 1987, 2008 Ingres Corporation
*/

/* Jam hints
**
LIBRARY = IMPADFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adp.h>

/**
**
**  Name: ADGDTTAB.ROC - ADF's datatypes table.
**
**  Description:
**      This file contains the datatypes table initialization which is a
**      table of ADI_DATATYPE structures, one per datatype name (there are
**      some datatype name synonyms, which are handled by multiple entries in
**      this table).  This table will actual be copied to the ADF server control
**      block at server startup time (by adg_startup()) since it needs to have
**      some of its fields written at that time.
**
**
**  History:
**      24-feb-87 (thurston)    
**          Initial creation; split off from ADGSTARTUP.C.
**      26-feb-87 (thurston)
**          Made changes to use server CB instead of GLOBALDEFs.
**      12-mar-87 (thurston)
**          Removed all adc_cvinto() and adc_isnull() function pointers from
**          the table, since they are now handled in a datatype independent
**          way.
**      25-mar-87 (thurston)
**          Added the adc_valchk() function pointers, removed the redundant
**          datatype ID code from the COM_VECT, and marked several no longer
**          used function pointers as `< unused >'.
**      02-apr-87 (thurston)
**          Added function ptrs to the "longtext" datatype so that they can
**          be compared, have keys built, and empty/null values built.
**      03-apr-87 (thurston)
**          Changed `intrinsic' to `status_bits'.
**      15-apr-87 (thurston)
**          Added tmlen and tmcvt function pointers for `longtext' to enable
**          the TM to display `longtext' values, which will happen for the
**          typeless NULL:  retrieve (x=null).
**      20-jun-87 (thurston)
**          Added the adc_minmaxdv() function pointers.
**      24-jun-87 (thurston)
**          Added "real" and "double precision" as synonyms for "float".
**      25-apr-88 (thurston)
**          Added "decimal" datatype and its synonyms "dec", and "numeric".
**          Also removed the entries for "text0" and "textn" since they are
**          now obsolete.
**      27-apr-88 (thurston)
**          Removed all references to the adc_1tonet_rti() and
**          adc_2tolocal_rti() routines, since they are not used, and will be
**          replaced by something for GCF.
**      07-jul-88 (thurston)
**          Added the adc_1klen_rti() and adc_2kcvt_rti() FUNC_EXTERNs and used
**          them in the datatypes table.
**      03-aug-88 (thurston)
**          Added entries for the new .adi_tpl_num field, and also updated
**          comments for the "tpls func" and "decompose func", which took up
**          a couple of the reserved slots in the COM VECT.
**      12-aug-88 (thurston)
**          Added function pointers and FUNC_EXTERN for the adc_tpls() entry in
**          the COMVECT for the RTI known datatypes.
**	06-oct-88 (thurston)
**	    Added NULL ptrs for sorted coercion table.
**	27-oct-88 (thurston)
**	    Ooops.  Added the FUNC_EXTERNs and function ptrs for the adc_tpls()
**	    functions, but forgot to do it for the adc_decompose() functions.
**	    Do it now.
**	06-nov-88 (thurston)
**	    Temporarily commented out the READONLY on the datatypes table.
**	09-mar-89 (jrb)
**	    Added sizeof global to get number of bytes for this table.  This is
**	    used at server startup time.
**	18-apr-89 (mikem)
**	    Added definitions of table_key and object_key.  Also
**	    added logical_key synonym for object_key in case
**	    LRC changes it's mind once more.
**	26-may-89 (mikem)
**	    Adding AD_CONDEXPORT to indicate that logical key datatypes can only
**	    be "exported" to the FE's if they specifically ask for them.  
**	    Otherwise they will just look externally to be CHAR.
**	15-jun-89 (jrb)
**	    Made datatypes table GLOBALCONSTDEF.
**	03-aug-89 (jrb)
**	    Added column specification info to dt table for adi_encode_colspec.
**	13-oct-89 (jrb)
**	    Changed column spec info for C datatype so that parentheses are
**	    allowed when specifying a column's length; I had previously thought
**	    this was illegal.
**	07-dec-89 (fred)
**	    Added large datatype support.
**	31-jul-90 (linda)
**	    Added the "isminmax" function, used to tell if a value is the min,
**	    max or null value for the given datatype.
**  	16-jan-92 (stevet)
**  	    Change datatype definition for float so that length can be
**  	    specified (i.e. float(N)).
**      28-sep-1992 (fred)
**     	    Added BIT datatypes.
**      29-oct-1992 (stevet)
**          Changed decimal to be AD_CONDEXPORT.
**      14-jan-1993 (stevet)
**          Removed most adc FUNC_EXTERN.  They are now defined in
**          adfint.h and aduint.h files.  Reminding three will be
**          resolved later.
**  	25-Feb-1993 (fred)
**  	    Allow parenthesized numbers after a long varchar argument.
**  	    The only number allowed should be zero.
**       9-Apr-1993 (fred)
**          Added byte stream datatypes.
*      12-Jul-1993 (fred)
**          Changed bit datatypes so that they cannot appear in
**          tables.  This is necessary until the frontends support the
**          bit datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-may-1996 (shero03)
**	    Addr compress/expand function
**	11-jun-1996 (prida01)
**	    Get RTI datatypes to call adu_lvarcharcmp function.
**	20-nov-98 (inkdo01)
**	    Added character varying as syn for varchar, clob & character large
**	    object as syns for long varchar and blob & binary large object as syns 
**	    for long byte - all for ANSI.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	25-feb-99 (inkdo01)
**	    Added "varbyte" as synonym for "byte varying" to be consistent with 
**	    "varchar"/"character varying".
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-feb-2001 (gupsh01)
**	    Added new data types nchar and nvarchar for unicode support.
**	03-apr-2001 (gupsh01)
**	    Added support for long nvarchar data type.
**	29-Mar-02 (gordy)
**	    Removed adc_1tpls_rti, adc_1decompose_rti and set entries to NULL.
**	    Set adi_tpl_num to 0.  These were intended for Het-Net but are
**	    not actually used.
**	25-nov-2003 (gupsh01)
**	    Added UTF8 datatype to transform to and from unicode data.
**	12-dec-03 (inkdo01)
**	    Add various synonyms (nclob, nchar large object, etc.) to 
**	    long nvarchar for Unicode blobs.
**	23-dec-03 (inkdo01)
**	    Add BIGINT for 8-byte integers.
**	13-apr-04 (inkdo01)
**	    Add TINYINT for 1-byte integers.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPADFLIBDATA
**	    in the Jamfile.
**	1-oct-04 (toumi01)
**	    AD_NOHISTOGRAM for nchar and nvarchar for now (until we can
**	    enhance the stats for two-byte data types)
**	24-dec-04 (inkdo01)
**	    Removed AD_NOHISTOGRAM from nchar, nvarchar as we are now using 
**	    collation weights as the histogram values.
**	16-jan-06 (dougi)
**	    Make "binary", "varbinary", "binary varying" synonyms for 
**	    "byte", "varbyte" and "byte varying", as per SQL2007.
**	20-apr-06 (dougi)
**	    Add data type family values to ADI_DATATYPE for date/time 
**	    enhancements.
**	13-Jul-2006 (kschendel)
**	    Break out some hashprep functions by type.
**	30-jun-2006 (gupsh01)
**	    Renamed olddate to ingresdate and newdate to ansidate.
**	19-sep-2006 (gupsh01)
**	    Removed date entry from this table.
**      16-oct-2006 (stial01)
**          Added locator datatypes.
**      06-nov-2006 (stial01)
**          Set status_bit AD_LOCATOR for locator datatypes
**	22-Oct-07 (kiria01) b119354
**	    Split adi_dtcoerce into a quel and SQL version
**	27-Oct-2008 (kiria01) SIR120473
**	    Added DB_PAT_TYPE datatype for PATCOMP support.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**  27-Feb-2009 (thich01)
**      Add the spatial types as blob synonyms.
**	09-Apr-2009 (kiria01) b121903
**	    Add boolean compare to allow tree compares to cope
**	    with boolean nodes.
**  16-Jun-2009 (thich01)
**      Add a new GEOM type to be treated as an LBYTE.  Change the spatial type
**      from blob synonyms to geom synonyms.  The GEOM type will be index-able.
**  06-Jul-2009 (thich01)
**      Add a generic geometry type before all the spatial synonyms.  This will
**      make the help output more intuitive.
**  20-Aug-2009 (thich01)
**      Change the spatial types to distinct types and create the DB_GEOM_TYPE
**      family.  Move Denise's type to PointNative.
**      05-oct-2009 (joea)
**          Change "boolean" row of Adi_1RO_datatypes array to allow BOOLEAN
**          to be stored in tables (AD_INDB flag), and add functions to the
**          "boolean" adi_dt_com_vect (common datatype functions).
**      12-Nov-2009 (coomi01) Bug 122840
**          Add compare funtion, adu_longcmp, for long byte
**  16-Feb-2010 (thich01)
**      Added geometry as a DB_GEOM_TYPE synonym.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.  Add NOSORT
**          and NOKEY back to GEOM types.  There is an exception in pslindx.c
**          for GEOM family now.
**	18-Mar-2010 (kiria01) b123438
**	    Add the missing initialiser to placate the compiler.
**      25-Mar-2010 (thich01)
**          Added geometry and geometerycollection to the DB_GEOM_FAMILY
**      02-Apr-2010 (thich01)
**          Added a rtree-cmp function to the spatial types for indexing.
**/


/* This is declared GLOBALCONSTDEF because it contains function pointers.
** If this were changed to GLOBALDEF const we would lose sharability on VMS.
*/

GLOBALCONSTDEF	 ADI_DATATYPE     Adi_1RO_datatypes[] = {
    /* NOTE:    The coercion bit masks are all being initialized to zeros since
    **          we want to assure that BTset() will be used to set the proper
    **          values.  This will be done in ADF's server control block at
    **          server startup time by adg_startup(), after that routine has
    **          first copied this const table into the server CB.
    */
/*---------------------------------------------------------------------------*/
/*  Each array element in the datatype table looks like this:                */
/*---------------------------------------------------------------------------*/
/* { { datatype name }, datatype id, dt family id, status_bits,    <unused>, */
/*      { coercion bitmask },   sorted_coercions_ptr, { colspec info },      */
/*	underlying datatype id (iff status_bits & AD_PERIPHERAL)
/*          {   lenchk func,     compare func,    keybld func,               */
/*              getempty func,   klen func,       kcvt func,                 */
/*              valchk func,     hashprep func,   helem func,                */
/*              hmin func,       hmax func,       dhmin func,                */
/*              dhmax func,      isminmax func,	  xform func,                */
/*              hg_dtln func,    <reserved>,	  adc_1minmaxdv,             */
/*              <unused>,        <unused>,        tmlen func,                */
/*              tmcvt func,      dbtoev func,     dbcvtev func,              */
/*		compr/exp func}
/* }                                                                         */
/*---------------------------------------------------------------------------*/
    { { "abrtsstr" },   DB_DYC_TYPE,    0,      0,          0,
	{ 0, }, { 0, },     NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
            {   NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
		NULL
	    }
    },
    { { "c" },          DB_CHR_TYPE,    0, AD_INTR | AD_INDB,  0,
	{ 0, }, { 0, },	    NULL,	{ TRUE,  TRUE,  1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_ccmp,           adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "char" },       DB_CHA_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE,  1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_varcharcmp,     adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "float" },      DB_FLT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ TRUE,  TRUE, 8, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_float_compare,  adu_fbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "integer" },    DB_INT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ TRUE,  FALSE, 4, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_int_compare,    adu_ibldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "decimal" },  DB_DEC_TYPE,    0, AD_INTR | AD_INDB | AD_CONDEXPORT,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE,  3, 5, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_dec_compare,    adu_decbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     NULL,               NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,		NULL,
		NULL
	    }
    },
    { { "longtext" },   DB_LTXT_TYPE,   0, AD_INTR,            0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_textcmp,        adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "money" },      DB_MNY_TYPE,              0, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_6mny_cmp,       adu_mbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "text" },       DB_TXT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE,  1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_textcmp,        adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "varchar" },    DB_VCH_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_varcharcmp,     adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_vch_hashprep,   adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "boolean" },    DB_BOO_TYPE,    0, AD_INTR | AD_INDB, 0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
            {   adc_1lenchk_rti,    adc_bool_compare,   adu_ibldkey,
                adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
                adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
                adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
                adc_1dhmax_rti,     adc_1isminmax_rti,  NULL,
                adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "table_key" },     DB_TABKEY_TYPE,    0, (AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_1logkey_cmp,    adu_bbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adu_5logkey_exp,    NULL,
		NULL
	    }
    },

    { { "object_key" },    DB_LOGKEY_TYPE,    0, (AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_1logkey_cmp,    adu_bbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adu_5logkey_exp,    NULL,
		NULL
	    }
    },
    { { "long varchar" },    DB_LVCH_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VCH_TYPE,
				/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adu_lvarcharcmp,	NULL,
		adc_1getempty_rti,  NULL,		NULL,
		adc_1valchk_rti,    NULL,		NULL,
		NULL,		    NULL,		NULL,
		NULL,		    /* {@fix_me@} */
				    NULL,		adc_lvch_xform,
		NULL,		    NULL,	        NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "bit" },    DB_BIT_TYPE, 0,
			(AD_INTR /* | AD_INDB */ | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_bit_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "bit varying" },    DB_VBIT_TYPE, 0,
			(AD_INTR /* | AD_INDB */ | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_bit_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,	adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "byte" },    DB_BYTE_TYPE, 0,
			(AD_INTR | AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_byte_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "byte varying" },    DB_VBYTE_TYPE, 0,
			(AD_INTR | AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_byte_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,	adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "long byte" },    DB_LBYTE_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adu_longcmp,	NULL,
		adc_1getempty_rti,  NULL,   	    	NULL,
		adc_1valchk_rti,    NULL,   	    	NULL,
		NULL,	    	    NULL,   	    	NULL,
		NULL,	    	    NULL,               adc_lvch_xform,
		NULL,	    	    NULL,   	    	NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },

/*
** The following group of entries are datatype name `synonyms'.
*/
    { { "character" },  DB_CHA_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_varcharcmp,     adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "character varying" },    DB_VCH_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_varcharcmp,     adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_vch_hashprep,   adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "vchar" },      DB_TXT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE,  1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_textcmp,        adu_cbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "clob" },    DB_LVCH_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VCH_TYPE,
				/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adu_lvarcharcmp,	NULL,
		adc_1getempty_rti,  NULL,		NULL,
		adc_1valchk_rti,    NULL,		NULL,
		NULL,		    NULL,		NULL,
		NULL,		    /* {@fix_me@} */
				    NULL,		adc_lvch_xform,
		NULL,		    NULL,	        NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "character large object" },    DB_LVCH_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VCH_TYPE,
				/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adu_lvarcharcmp,	NULL,
		adc_1getempty_rti,  NULL,		NULL,
		adc_1valchk_rti,    NULL,		NULL,
		NULL,		    NULL,		NULL,
		NULL,		    /* {@fix_me@} */
				    NULL,		adc_lvch_xform,
		NULL,		    NULL,	        NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "char large object" },    DB_LVCH_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VCH_TYPE,
				/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adu_lvarcharcmp,	NULL,
		adc_1getempty_rti,  NULL,		NULL,
		adc_1valchk_rti,    NULL,		NULL,
		NULL,		    NULL,		NULL,
		NULL,		    /* {@fix_me@} */
				    NULL,		adc_lvch_xform,
		NULL,		    NULL,	        NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "varbyte" },    DB_VBYTE_TYPE, 0,
			(AD_INTR | AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_byte_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,	adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "binary large object" },    DB_LBYTE_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    NULL,	    	NULL,
		adc_1getempty_rti,  NULL,   	    	NULL,
		adc_1valchk_rti,    NULL,   	    	NULL,
		NULL,	    	    NULL,   	    	NULL,
		NULL,	    	    NULL,               adc_lvch_xform,
		NULL,	    	    NULL,   	    	NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "blob" },    DB_LBYTE_TYPE, 0,
			(AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
				AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    NULL,	    	NULL,
		adc_1getempty_rti,  NULL,   	    	NULL,
		adc_1valchk_rti,    NULL,   	    	NULL,
		NULL,	    	    NULL,   	    	NULL,
		NULL,	    	    NULL,               adc_lvch_xform,
		NULL,	    	    NULL,   	    	NULL,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "binary" },    DB_BYTE_TYPE, 0,
			(AD_INTR | AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_byte_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "binary varying" },    DB_VBYTE_TYPE, 0,
			(AD_INTR | AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_byte_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,	adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "varbinary" },    DB_VBYTE_TYPE, 0,
			(AD_INTR | AD_INDB | AD_CONDEXPORT),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adc_byte_compare,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,	adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
    { { "f" },          DB_FLT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ TRUE,  FALSE, 8, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_float_compare,  adu_fbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "real" },       DB_FLT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 4, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_float_compare,  adu_fbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "double precision" }, DB_FLT_TYPE, 0, AD_INTR | AD_INDB, 0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 8, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_float_compare,  adu_fbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "int" },        DB_INT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ TRUE,  FALSE, 4, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_int_compare,    adu_ibldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "i" },          DB_INT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ TRUE,  FALSE, 4, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_int_compare,    adu_ibldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "smallint" },   DB_INT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 2, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_int_compare,    adu_ibldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "bigint" },   DB_INT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 8, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_int_compare,    adu_ibldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "tinyint" },   DB_INT_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_int_compare,    adu_ibldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "dec" },      DB_DEC_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 3, 5, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_dec_compare,    adu_decbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     NULL,               NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "numeric" },  DB_DEC_TYPE,    0, AD_INTR | AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 3, 5, 0 }, 0,
	    {   adc_1lenchk_rti,    adc_dec_compare,    adu_decbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     NULL,               NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
/*
** standard SQL date/time types
*/
    { { "ingresdate" },       DB_DTE_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "ansidate" },       DB_ADTE_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "time without time zone" },       DB_TMWO_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "time" },       DB_TMWO_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "time with time zone" },       DB_TMW_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "time with local time zone" },       DB_TME_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "timestamp without time zone" },  DB_TSWO_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 6 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "timestamp" },  DB_TSWO_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 6 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "timestamp with time zone" },  DB_TSW_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 6 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "timestamp with local time zone" },  DB_TSTMP_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 6 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "interval year to month" },   DB_INYM_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
    { { "interval day to second" },   DB_INDS_TYPE,    DB_DTE_TYPE, AD_INDB,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 0, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_4date_cmp,      adu_dbldkey,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_1hashprep_rti,  adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
/*
** "fe_query" is reserved for frontend use
*/
    { { "fe_query" },   DB_QUE_TYPE,            0, 0,          0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL
	    }
    },
/*
** "II_DIFFERENT" is used internally for sorting unsortable things.
*/
    { { "II_DIFFERENT" },     DB_DIF_TYPE,      0, 0,          0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL
	    }
    },
/*
** "UTF8" is used internally for transforming to and from unicode datatypes.
*/
    { { "UTF8" },     DB_UTF8_TYPE,      0, 0,          0,
	{ 0, }, { 0, },     NULL,       { FALSE, FALSE, 0, 0, 0 }, 0,
            {   adc_1lenchk_rti,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
		NULL
	    }
    },
/* 
** "nchar" for fixed length unicode character data type 
*/
    { { "nchar" },  DB_NCHR_TYPE, 0,
			(AD_INTR | AD_INDB),  0,
	{ 0, }, { 0, },     NULL,       { FALSE, TRUE, 1, 0, 0 }, 0,
	    {	adc_1lenchk_rti,     adu_nvchrcomp,	 adu_nvchbld_key,
		adc_1getempty_rti,   adc_1klen_rti,	 adc_2kcvt_rti,
		adc_1valchk_rti,     adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti,	     adc_1hmax_rti,	 adc_2dhmin_rti,
		adc_1dhmax_rti,	     adc_1isminmax_rti,	 NULL,
		adc_1hg_dtln_rti,    NULL,		 adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,      adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
/* 
** "nvarchar" for variable length unicode character data type 
*/
    { { "nvarchar" },  DB_NVCHR_TYPE, 0,
			(AD_INTR | AD_INDB),  0,
	{ 0, }, { 0, },     NULL,       { FALSE, TRUE, 1, 0, 0 }, 0,
            {   adc_1lenchk_rti,     adu_nvchrcomp,      adu_nvchbld_key,
                adc_1getempty_rti,   adc_1klen_rti,      adc_2kcvt_rti,
                adc_1valchk_rti,     adc_1hashprep_rti,  adc_1helem_rti,
                adc_1hmin_rti,       adc_1hmax_rti,      adc_2dhmin_rti,
                adc_1dhmax_rti,      adc_1isminmax_rti,  NULL,
                adc_1hg_dtln_rti,    NULL,               adc_1minmaxdv_rti,
                NULL,		    NULL,		adc_1tmlen_rti,
                adc_2tmcvt_rti,      adc_1dbtoev_ingres, NULL,
		NULL
	    }
    },
/*
** "long nvarchar" for variable length unicode character data type
*/
    { { "long nvarchar" },    DB_LNVCHR_TYPE, 0,
                        (AD_INDB | AD_PERIPHERAL |
                                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },     NULL,       { FALSE, TRUE, 0, 0, 0 }, DB_NVCHR_TYPE,
                                /*      { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    adu_lvarcharcmp,    NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               /* {@fix_me@} */
                                    NULL,               adc_lvch_xform,
                NULL,               NULL,               NULL,
                NULL,		    NULL,		adc_1tmlen_rti,
                adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },

/*
** "nclob" for variable length unicode character data type
*/
    { { "nclob" },    DB_LNVCHR_TYPE, 0,
                        (AD_INDB | AD_PERIPHERAL |
                                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },     NULL,       { FALSE, TRUE, 0, 0, 0 }, DB_NVCHR_TYPE,
                                /*      { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    adu_lvarcharcmp,    NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               /* {@fix_me@} */
                                    NULL,               adc_lvch_xform,
                NULL,               NULL,               NULL,
                NULL,		    NULL,		adc_1tmlen_rti,
                adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },

/*
** "nchar large object" for variable length unicode character data type
*/
    { { "nchar large object" },    DB_LNVCHR_TYPE, 0,
                        (AD_INDB | AD_PERIPHERAL |
                                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },     NULL,       { FALSE, TRUE, 0, 0, 0 }, DB_NVCHR_TYPE,
                                /*      { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    adu_lvarcharcmp,    NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               /* {@fix_me@} */
                                    NULL,               adc_lvch_xform,
                NULL,               NULL,               NULL,
                NULL,		    NULL,		adc_1tmlen_rti,
                adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },

/*
** "national character large object" for variable length unicode 
** character data type
*/
    { { "national character large object" },    DB_LNVCHR_TYPE, 0,
                        (AD_INDB | AD_PERIPHERAL |
                                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
	{ 0, }, { 0, },     NULL,       { FALSE, TRUE, 0, 0, 0 }, DB_NVCHR_TYPE,
                                /*      { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    adu_lvarcharcmp,    NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               /* {@fix_me@} */
                                    NULL,               adc_lvch_xform,
                NULL,               NULL,               NULL,
                NULL,		    NULL,		adc_1tmlen_rti,
                adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },

/*
** Placeholders for obsolete security label stuff.
** These exist because I don't want to introduce a hole into the FI
** table, and we need to ensure that the old FI's are not chosen
** when searching for the proper FI for some operator.  (like boolean
** equals, for instance.)  It is apparently a bad idea to just
** renumber the FI table, else we could simply delete entries.
** FIXME (kschendel) above was written before the fi_defn work, can
** these be deleted now?
*/
    { { "II_NOTUSED SEC LBL" },     DB_SEC_TYPE,      0, 0,          0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL
	    }
    },
    { { "II_NOTUSED SEC LBL1" },     DB_SECID_TYPE,      0, 0,          0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL
	    }
    },
/*
** long nvarchar locator 
*/
    { { "long nvarchar locator" },    DB_LNLOC_TYPE, 0,
                        (AD_INTR | AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY | AD_LOCATOR),  0,
	{ 0, }, { 0, }, NULL, { FALSE, FALSE, 0, 0, 0 }, DB_NVCHR_TYPE,
                         /*   { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    NULL,		NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
		NULL
	    }
    },
/*
** long byte locator
*/
    { { "long byte locator" },    DB_LBLOC_TYPE, 0,
                        (AD_INTR | AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY | AD_LOCATOR),  0,
	{ 0, }, { 0, }, NULL, { FALSE, FALSE, 0, 0, 0 }, DB_LBYTE_TYPE,
                        /*    { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    NULL,		NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
		NULL
	    }
    },
/*
** long varchar locator
*/
    { { "long varchar locator" },    DB_LCLOC_TYPE, 0,
                        (AD_INTR | AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY | AD_LOCATOR),  0,
	{ 0, }, { 0, }, NULL, { FALSE, FALSE, 0, 0, 0 }, DB_LVCH_TYPE,
                        /*    { embed#, (#), def len, scale, prec } */
            {   adc_1lenchk_rti,    NULL,		NULL,
                adc_1getempty_rti,  NULL,               NULL,
                adc_1valchk_rti,    NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
                NULL,               NULL,               NULL,
		NULL
	    }
    },
    { { "pattern" },    DB_PAT_TYPE,    0, AD_INTR,  0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
	    {   adc_1lenchk_rti,    adu_varcharcmp,     NULL,
		adc_1getempty_rti,  adc_1klen_rti,      adc_2kcvt_rti,
		adc_1valchk_rti,    adc_vch_hashprep,   adc_1helem_rti,
		adc_1hmin_rti,      adc_1hmax_rti,      adc_2dhmin_rti,
		adc_1dhmax_rti,     adc_1isminmax_rti,	NULL,
		adc_1hg_dtln_rti,   NULL,               adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     NULL,               NULL,
		NULL
	    }
    },
/* GeometryCollection type - to be treated the same as LBYTE */
    { { "geometrycollection" },    DB_GEOMC_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* Geometry type - abstract container of Geometry Family 
   To be treated the same as LBYTE */
    { { "geometry" },    DB_GEOM_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* Point in the geometry family */
    { { "point" },    DB_POINT_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* MultiPoint in the geometry family */
    { { "multipoint" },    DB_MPOINT_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* LineString in the geometry family*/
    { { "linestring" },    DB_LINE_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* MultiLineString in the geometry family*/
    { { "multilinestring" },    DB_MLINE_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* Polygon in the geometry family */
    { { "polygon" },    DB_POLY_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* MultiPolygon in the geometry family */
    { { "multipolygon" },    DB_MPOLY_TYPE, DB_GEOM_TYPE,
            (AD_INDB | AD_PERIPHERAL | AD_CONDEXPORT |
                AD_NOHISTOGRAM | AD_NOSORT | AD_NOKEY),  0,
    { 0, }, { 0, },     NULL,   { FALSE, TRUE, 0, 0, 0 }, DB_VBYTE_TYPE,
            /*  { embed#, (#), def len, scale, prec } */
        {   adc_1lenchk_rti,    adu_rtree_cmp,           NULL,
        adc_1getempty_rti,  NULL,               NULL,
        adc_1valchk_rti,    NULL,               NULL,
        NULL,               NULL,               NULL,
        NULL,               NULL,               adc_lvch_xform,
        NULL,               NULL,               NULL,
        NULL,           NULL,       adc_1tmlen_rti,
        adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/* NBR type - DB_GEOM_TYPE family for indexing, but not a typcial geom type.*/
    { { "NBR" },    DB_NBR_TYPE, DB_GEOM_TYPE,
			(AD_INTR | AD_INDB | AD_CONDEXPORT | AD_NOSORT | 
                         AD_NOKEY),  0,
	{ 0, }, { 0, },	    NULL,	{ FALSE, TRUE, 1, 0, 0 }, 0,
			/*	{ embed#, (#), def len, scale, prec } */
	    {   adc_1lenchk_rti,    adu_rtree_cmp,	adu_bitbldkey,
		adc_1getempty_rti,  adc_1klen_rti,	adc_2kcvt_rti,
		adc_1valchk_rti,    adc_inplace_hashprep, adc_1helem_rti,
		adc_1hmin_rti, 	    adc_1hmax_rti,	adc_2dhmin_rti,
		adc_1dhmax_rti,	    adc_1isminmax_rti,  NULL,
		adc_1hg_dtln_rti,   NULL,	        adc_1minmaxdv_rti,
		NULL,		    NULL,		adc_1tmlen_rti,
		adc_2tmcvt_rti,     adc_1dbtoev_ingres, NULL,
        NULL }
    },
/*
** END OF DATA TYPES
*/
    { { "END_OF_DTs" }, DB_NODT,                0, 0,          0,  
	{ 0, }, { 0, },	    NULL,	{ FALSE, FALSE, 0, 0, 0 }, 0,
	    {   NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL,               NULL,               NULL,
		NULL
	    }
    }
};                                      /* ADF's DATATYPE table */

GLOBALDEF   const	i4	    Adi_dts_size = sizeof(Adi_1RO_datatypes);
