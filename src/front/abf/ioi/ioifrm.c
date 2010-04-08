/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<ds.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"ioistd.h"
#include	<feds.h>
#include	<ifrmobj.h>

/**
** Name:	ioifrm.c -	Frame DS templates
**
** Description:
**	image file frame i/o.
**
** History:
**	Revision 5.1  86/09  bobm
**	Initial revision.
**
**	9/90 (Mike S)
**		Use CL_OFFSETOF to calcualte structure offsets portably.
**		Improves porting changes 130645, 130696, 130710, 130719, 130725
**	13-mar-92 (davel)
**		Fix bug 43051 - add DSPTRS entry for member fdv2_typename of 
**		FDESCV2.  Also use CL_OFFSETOF to calcualte structure offsets.
**	10/11/92 (dkh) - Removed readonly qualification on references
**			 declaration to DSnat, DSi4, DSstring and DSf8.
**			 Alpha cross compiler does not like the qualification
**			 for globalrefs.
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
**	14-apr-93 (davel)
**		Add Decimal constants.
**      07-May-96 (fanra01)
**              Bug# 76270
**              Added modifications for linkage problems using DLLs on NT.
**              Added function to initialise portion of the Temtable which are
**              not constants in NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifdef NT_GENERIC
GLOBALDLLREF DSTEMPLATE DSnat, DSi4, DSstring, DSf8;
# else /* NT_GENERIC */
GLOBALREF DSTEMPLATE DSnat, DSi4, DSstring, DSf8;
# endif /* NT_GENERIC */

/*
** Ofptrs - OFDESC structure pointer offsets.  The data pointer actually
** contains an offset value when read / written, hence only the character
** pointers are mentioned.
*/

static DSTEMPLATE DSshort =
{
	DS_I2,
	sizeof(i2),
	0,
	DS_IN_CORE,
	NULL,
	NULL,
	NULL
};

static DSPTRS Ofptrs[] =
{
	{ CL_OFFSETOF(FDESCV2, fdv2_name), DSSTRP, 0, 0},
	{ CL_OFFSETOF(FDESCV2, fdv2_tblname), DSSTRP, 0, 0},
	{ CL_OFFSETOF(FDESCV2, fdv2_typename), DSSTRP, 0, 0}
};

/*
** FDESCV2 structure template.
*/
static DSTEMPLATE DSofdesc =
{
	DS_OISYMTAB,
	sizeof(FDESCV2),
	sizeof(Ofptrs)/sizeof(Ofptrs[0]),
	DS_IN_CORE,
	Ofptrs,
	NULL,
	NULL
};

/*
** IL_DB_VDESC structure template.
*/
static DSTEMPLATE DSdbd =
{
	DS_OIDBDV,
	sizeof(IL_DB_VDESC),
	0,
	DS_IN_CORE,
	NULL,
	NULL,
	NULL
};

static DSPTRS Fcptrs[] =
{
	{ CL_OFFSETOF(FLT_CONS, str), DSSTRP, 0, 0}
};

static DSPTRS Dcptrs[] =
{
	{ CL_OFFSETOF(DEC_CONS, str), DSSTRP, 0, 0},
	{ CL_OFFSETOF(DEC_CONS, val), DSSTRP, 0, 0}
};

/*
** FLT_CONS structure - contains floating point and string value.
*/
static DSTEMPLATE DSfcons =
{
	DS_OIFCONS,
	sizeof(FLT_CONS),
	1,
	DS_IN_CORE,
	Fcptrs,
	NULL,
	NULL
};

/*
** DEC_CONS structure - contains two strings and an i2 prec/scale
*/
static DSTEMPLATE DSdcons =
{
	DS_OIDCONS,
	sizeof(DEC_CONS),
	sizeof(Dcptrs)/sizeof(Dcptrs[0]),
	DS_IN_CORE,
	Dcptrs,
	NULL,
	NULL
};

static DSPTRS Txptrs[] =
{
	{ 0, DS2VSIZE, 1, 0},
};

/*
** DB_TEXT_STRING
*/
static DSTEMPLATE DSdbtxt =
{
	DS_OIDBTEXT,
	-2,
	1,
	DS_IN_CORE,
	Txptrs,
	NULL,
	NULL
};

static DSPTRS Txxptrs[] =
{
	{ 0, DSNORMAL, 0, DS_OIDBTEXT }
};

/*
** template for a POINTER-TO a DB_TEXT_STRING
*/
static DSTEMPLATE DSdbtptr =
{
	DS_OIDTXTPTR,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Txxptrs,
	NULL,
	NULL
};


/*
** Fptrs - IFRMOBJ pointer and no. array elements offsets
*/

static DSPTRS Fptrs[] =
{
	{ CL_OFFSETOF(IFRMOBJ, sym.fdsc_ofdesc), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, sym.fdsc_cnt), DS_OISYMTAB },
	{ CL_OFFSETOF(IFRMOBJ, dbd), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_dbd), DS_OIDBDV },
	{ CL_OFFSETOF(IFRMOBJ, il), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_il), DS_I2 },
	{ CL_OFFSETOF(IFRMOBJ, c_const), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_cc), DS_STRING },
	{ CL_OFFSETOF(IFRMOBJ, f_const), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_fc), DS_OIFCONS },
	{ CL_OFFSETOF(IFRMOBJ, d_const), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_dc), DS_OIDCONS },
	{ CL_OFFSETOF(IFRMOBJ, x_const), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_xc), DS_OIDTXTPTR},
	{ CL_OFFSETOF(IFRMOBJ, i_const), DSARRAY, 
	  CL_OFFSETOF(IFRMOBJ, num_ic), DS_I4 }
};

/*
** IFRMOBJ template
*/
static DSTEMPLATE DSfrm =
{
	DS_OIFRM,
	sizeof(IFRMOBJ),
	sizeof(Fptrs)/sizeof(Fptrs[0]),
	DS_IN_CORE,
	Fptrs,
	NULL,
	NULL
};

/*
**      08-May-96 (fanra01)
**          If the Temtable array is modified ensure that the frm_init
**          function is updated.
**
*/
static DSTEMPLATE *Temtable[] =
{
	&DSfrm,
	&DSofdesc,
	&DSfcons,
	&DSdcons,
	&DSdbtxt,
	&DSdbd,
	&DSdbtptr,
        NULL,
        NULL,
        NULL,
        NULL,
	&DSshort
};

GLOBALDEF DSTARRAY Frm_template =
{
	sizeof(Temtable)/sizeof(Temtable[0]), Temtable
};



static i4  Frm_initialised = 0;

/*
** Name : frm_init
**
** Description :
**      Function to initialise portions of the Frm_template structure which
**      are not constants and cannot be initialised at compile time.
**
** Outputs :
**      None
**
** Inputs :
**      None
**
** Returns:
**      None
**
** History :
**      07-May-96 (fanra01)
**          Created.
*/

VOID
frm_init()
{
    if(!Frm_initialised)
    {
        Temtable[7] =   &DSi4;
        Temtable[8] =   &DSnat;
        Temtable[9] =   &DSstring;
        Temtable[10] =  &DSf8;
        Frm_initialised = 1;
    }
}
