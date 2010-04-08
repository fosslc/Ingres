/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include <compat.h>
# include <ds.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include "ioistd.h"
# include <feds.h>
# include <ooclass.h>

/**
** Name:	ioirtt.c -	Intermediate Run-Time Table DS Templates.
**
** History:
**	11/90 (Mike S)
**		Relocate new members of ABRTSOBJ structure: 
**		abcndate and abroappl
**
**	Revision 6.3/03/00  89/11  wong
**	Added 'language' member to ABRTSOBJ strucutre.
**	89/08  billc  Added global variable/constant template and
**	added member elements to ABRTSOBJ structure.
**
**	Revision 6.2  89/02  bobm
**	Changes for release 6 run-time structures.
**
**	Revision 5.1  86/09  bobm
**	Initial revision.
**
**	10/11/92 (dkh) - Removed readonly qualification on references
**			 declaration to DSnat, DSstring and DSchar.  Alpha
**			 cross compiler does not like the qualification
**			 for globalrefs.
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
**      09/10/93 (kchin)
**              Use macro OFFSET_OF to get offset of abrfrname, abrform,
**              abrfrvar, and abrfrtype for frptrs below.  Original hard
**              coded offset of these structure elements will introduce problem
**              when porting to 64-bit platform (axp_osf), since compiler
**              will perform alignment of pointers to 8-byte boundary.
**       01/12/93 (smc)
**		Bug #58882
**          	Altered above change to use CL_OFFSETOF to prevent truncation
**		warnings with lint.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** standard DS templates
*/

# ifdef NT_GENERIC
GLOBALDLLREF DSTEMPLATE	DSnat, DSstring, DSchar;
# else /* NT_GENERIC */
GLOBALREF DSTEMPLATE	DSnat, DSstring, DSchar;
# endif /* NT_GENERIC */

/*
** ABRTSFORM template (compiled form address ignored)
*/
static DSPTRS formptrs[] =
{
	{ 0, DSSTRP, 0, 0}
};

static DSTEMPLATE DSarform =
{
	DS_ARFORM,
	sizeof(ABRTSFO),
	sizeof(formptrs)/sizeof(formptrs[0]),
	DS_IN_CORE,
	formptrs,
	NULL,
	NULL
};

/*
** templates for ABRTSVAR UNION.  We treat this as a DSUNION, using
** the OC_ codes for tags.  We COULD simply treat it as a single ABRTSVAR
** structure since no relocation is needed inside them.  Reason for
** going to this extra work:
**
**	probably the most common structure, ABRTSNUSER, is also a
**	lot smaller than other members of the union.  This way we
**	save copying / allocating a lot of useless bytes.
**
** If anything inside the POINTED-TO structure required relocating, we
** would need a separate template for each pointed to structure.  As it
** is, we simply copy either a SNUSER or an entire ABRTSVAR.
**
** All this baroqueness is due to the fact that DSUNION is intended to
** actually handle embedded unions inside a structure, with potentially
** different sets of pointers in each union member, hence you require
** a DSTEMPLATE for each union layout in the structure itself, as well
** as anything the union element can point at.  There is no special
** case for instances like this where we simply have a single pointer
** to different types of structures.
*/

static DSTEMPLATE DSvar =
{
	DS_ARVAR,
	sizeof(ABRTSVAR),
	0,
	DS_IN_CORE,
	NULL,
	NULL,
	NULL
};

static DSTEMPLATE DSvsnuser =
{
	DS_ARVSNUSER,
	sizeof(ABRTSNUSER),
	0,
	DS_IN_CORE,
	NULL,
	NULL,
	NULL
};

static DSPTRS Nusptr[] =
{
	{ 0, DSNORMAL, 0,  DS_ARVSNUSER}
};

static DSPTRS Vptr[] =
{
	{ 0, DSNORMAL, 0,  DS_ARVAR}
};

static DSTEMPLATE DSuser =
{
	OC_OSLFRAME,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Nusptr,
	NULL,
	NULL,
};

static DSTEMPLATE DSouser =
{
	OC_OLDOSLFRAME,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Nusptr,
	NULL,
	NULL,
};

static DSTEMPLATE DSqbf =
{
	OC_QBFFRAME,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Vptr,
	NULL,
	NULL,
};

static DSTEMPLATE DSrw =
{
	OC_RWFRAME,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Vptr,
	NULL,
	NULL,
};

static DSTEMPLATE DSgr =
{
	OC_GRFRAME,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Vptr,
	NULL,
	NULL,
};

static DSTEMPLATE DSgbf =
{
	OC_GBFFRAME,
	sizeof(PTR),
	1,
	DS_IN_CORE,
	Vptr,
	NULL,
	NULL,
};

static DSPTRS frptrs[] =
{
        {CL_OFFSETOF(ABRTSFRM, abrfrname), DSSTRP, 0, 0},
        {CL_OFFSETOF(ABRTSFRM, abrform), DSNORMAL, 0, DS_ARFORM},
        {CL_OFFSETOF(ABRTSFRM, abrfrvar), DSUNION, 
	      CL_OFFSETOF(ABRTSFRM, abrfrtype), 0}
};

static DSTEMPLATE DSarfrm =
{
	DS_ARFRM,		/* ds_id */
	sizeof(ABRTSFRM),	/* ds_size */

	sizeof(frptrs)/sizeof(frptrs[0]),	/* ds_cnt */

	DS_IN_CORE,		/* dstemp_type */
	frptrs,			/* ds_ptrs */
	NULL,			/* dstemp_file */
	NULL			/* dstemp_func */
};

# define	GLname		0
# define	GLtypename	(GLname + sizeof(char *))
# define	ConstValue	(GLtypename + sizeof(char *))

static DSPTRS	glptrs[] =
{
	{GLname,	DSSTRP, 0, 0},
	{GLtypename,	DSSTRP, 0, 0},
	{ConstValue,	DSARRAY, ConstValue+sizeof(PTR), DS_CHAR},
};

static DSTEMPLATE DSglobs =
{
	OC_GLOBAL,		/* ds_id */
	sizeof(ABRTSGL),	/* ds_size */

	sizeof(glptrs)/sizeof(glptrs[0]),	/* ds_cnt */

	DS_IN_CORE,		/* dstemp_type */
	glptrs,			/* ds_ptrs */
	NULL,			/* dstemp_file */
	NULL			/* dstemp_func */
};

/* record attributes */
# define	RAname		0
# define	RAtname		RAname+sizeof(PTR)

static DSPTRS raptrs[] =
{
	{RAname, DSSTRP, 0, 0},
	{RAtname, DSSTRP, 0, 0}
};

static DSTEMPLATE DSattrs =
{
	OC_RECMEM,		/* ds_id */
	sizeof(ABRTSATTR),	/* ds_size */

	sizeof(raptrs)/sizeof(raptrs[0]),	/* ds_cnt */

	DS_IN_CORE,		/* dstemp_type */
	raptrs,			/* ds_ptrs */
	NULL,			/* dstemp_file */
	NULL			/* dstemp_func */
};

/* record definitions */
# define	TYname		0
# define	TYflds		TYname+sizeof(PTR)
# define	TYsize		TYflds+sizeof(PTR)

static DSPTRS typtrs[] =
{
	{TYname, DSSTRP, 0, 0},
	{TYflds, DSARRAY, TYsize, OC_RECMEM}
};

static DSTEMPLATE DStypes =
{
	OC_RECORD,		/* ds_id */
	sizeof(ABRTSTYPE),	/* ds_size */

	sizeof(typtrs)/sizeof(typtrs[0]),	/* ds_cnt */

	DS_IN_CORE,		/* dstemp_type */
	typtrs,			/* ds_ptrs */
	NULL,			/* dstemp_file */
	NULL			/* dstemp_func */
};

# define	PRpname		0

static DSPTRS prptrs[] =
{
	{PRpname, DSSTRP, 0, 0}
};

static DSTEMPLATE DSarpro =
{
	DS_ARPRO,		/* ds_id */
	sizeof(ABRTSPRO),	/* ds_size */
	1,			/* ds_cnt */
	DS_IN_CORE,		/* dstemp_type */
	prptrs,			/* ds_ptrs */
	NULL,			/* dstemp_file */
	NULL			/* dstemp_func */
};

/*
** NOTE: although we defined the OFFSETOFs, we do not relocate the form
** 	array pointer.  This is because the ABF runtime system never
**	accesses the form elements this way.  The individual form
**	structures are pointed to by the frame elements.  If we sent
**	the array pointer through DS, we would duplicate all but
**	the zero'th element because of this.  If we really wanted to
**	assure reconstruction of an array, we would have to define
**	the array here, and coerce OFFSETOFs into the frame pointers,
**	ignoring them, and reconstructing the pointers after relocating.
*/

static DSPTRS roptrs[] =
{
	{CL_OFFSETOF(ABRTSOBJ, abrodbname), DSSTRP, 0, 0},
	{CL_OFFSETOF(ABRTSOBJ, abrosframe), DSSTRP, 0, 0},
	{CL_OFFSETOF(ABRTSOBJ, abrofhash), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ, abrohcnt), DS_NAT},
	{CL_OFFSETOF(ABRTSOBJ, abrophash), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ, abrohcnt), DS_NAT},
	{CL_OFFSETOF(ABRTSOBJ, abroftab), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ, abrofcnt), DS_ARFRM},
	{CL_OFFSETOF(ABRTSOBJ, abroptab), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ, abropcnt), DS_ARPRO},
	{CL_OFFSETOF(ABRTSOBJ, abroleid), DSSTRP, 0, 0},
	{CL_OFFSETOF(ABRTSOBJ, abpassword), DSSTRP, 0, 0},
	{CL_OFFSETOF(ABRTSOBJ, abroghash), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ, abrohcnt), DS_NAT},
	{CL_OFFSETOF(ABRTSOBJ, abrothash), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ, abrohcnt), DS_NAT},
	{CL_OFFSETOF(ABRTSOBJ,abrogltab), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ,abroglcnt), OC_GLOBAL},
	{CL_OFFSETOF(ABRTSOBJ,abrotytab), DSARRAY, 
	      CL_OFFSETOF(ABRTSOBJ,abrotycnt), OC_RECORD},
	{CL_OFFSETOF(ABRTSOBJ,abcndate), DSSTRP, 0, 0},
	{CL_OFFSETOF(ABRTSOBJ,abroappl), DSSTRP, 0, 0}
};

static DSTEMPLATE DSarobj =
{
	DS_AROBJ,		/* ds_id */
	sizeof(ABRTSOBJ),	/* ds_size */

	sizeof(roptrs)/sizeof(roptrs[0]),	/* ds_cnt */

	DS_IN_CORE,		/* dstemp_type */
	roptrs,			/* ds_ptrs */
	NULL,			/* dstemp_file */
	NULL			/* dstemp_func */
};

/*
**      08-May-96 (fanra01)
**          If the DSTab array is modified ensure that the function rtt_init
**          is updated.
**
*/
static DSTEMPLATE *DSTab[] = {
	&DSarobj,
	&DSarpro,
	&DSglobs,
	&DStypes,
	&DSattrs,
	&DSarfrm,
	&DSarform,
	&DSuser,
	&DSouser,
	&DSqbf,
	&DSrw,
	&DSgbf,
	&DSgr,
	&DSvsnuser,
	&DSvar,
	NULL,
	NULL,
	NULL,
};

GLOBALDEF DSTARRAY Rtt_template = {
	sizeof(DSTab)/sizeof(DSTab[0]), DSTab
};


static i4  Rtt_initialised = 0;

/*
** Name : rtt_init
**
** Description :
**      Function to initialise portions of the Rtt_template structure which
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
rtt_init()
{
    if(!Rtt_initialised)
    {
        DSTab[15] = &DSnat;
        DSTab[16] = &DSstring;
        DSTab[17] = &DSchar;
        Rtt_initialised = 1;
    }
    return;
}
