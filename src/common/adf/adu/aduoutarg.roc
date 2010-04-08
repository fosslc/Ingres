/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>

/**
**
**  Name: OUTARG.ROC - define read-only output formatting table.
**
**  Description:
**        This file defines the read-only table, Out_arg.  This is used
**	to define the default output formating for a session.
**
**  History:
**      30-may-86 (ericj)    
**          Converted from outarg.c.  Out_arg used to be a writable object.
**	15-jan-87 (thurston)
**	    Modified to jive with new layout of ADF_OUTARG struct.
**	23-oct-88 (thurston)
**	    Made the default f4width and f8width be 11 on all machines ... used
**	    to be 10 on non-IEEE machines.  This was approved by the LRC some
**	    time ago.
**	14-jul-93 (ed)
**	    unnested dbms.h
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	12-apr-04 (inkdo01)
**	    Added ad_i8width (replaced ad_1_rsvd).
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPADFLIBDATA
**	    in the Jamfile.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* Jam hints
**
LIBRARY = IMPADFLIBDATA
**
*/


/*
**   For global initialization of default output arguments.
*/

GLOBALDEF const   ADF_OUTARG		Adf_outarg_default =
{
    6,		/* ad_c0width */
    6,		/* ad_t0width */
    6,		/* ad_i1width */
    6,		/* ad_i2width */
    13,		/* ad_i4width */
    11,		/* ad_f4width */
    11,		/* ad_f8width */
    3,		/* ad_f4prec */
    3,		/* ad_f8prec */
    22,		/* ad_i8width */
    -1,		/* ad_2_rsvd: <RESERVED> */
    'n',	/* ad_f4style */
    'n',	/* ad_f8style */
    NULLCHAR,	/* ad_3_rsvd: <RESERVED> */
    NULLCHAR	/* ad_4_rsvd: <RESERVED> */
};
