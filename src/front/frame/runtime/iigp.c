/*
**	iigp.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		 
# include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<fsicnsts.h>
# include	<er.h>
# include	"iigpdef.h"
# include	"erfr.h"

/**
** Name:	iigp.c	-	Global parameters / init code
**
** Description:
**
**	Routines for accessing / setting global parameters. defines:
**
**		IIFRgpinit - initialize structure.
**		IIFRgpfree - free storage
**		IIFRgpsetio - set a parameter.
**		IIFRgpcontrol - parameter state control.
**
** History:
**	4/88 (bobm)	created
**	05/20/88 (dkh) - ER changes.
**	08/04/89 (dkh) - Added support for 80/132 runtime switching.
**	19-feb-92 (leighb) DeskTop Porting Change:
**		adh_evcvtdb() has only 3 args, bogus 4th one deleted.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*
** Adding new parameters / modifying defaults:
**
**	The Set table is the only thing that has to be modified to redefine
**	and add new parameters.  The description below concerns the runtime
**	structure, which need not be modified as new things are added.
**
**	Order of items in the Set[] table does not matter.  The only
**	rules that need be followed for alternate set designators and
**	parameter id's is that they be non-negative.  The pid's
**	should also be reasonably small since they are going to
**	become array indexes.  Every parameter should have an alt=0 entry
**	in Set[], defining the default.
**
** Description of data structure:
**
**	This is the data structure used by the IIFRgp... calls
**	The idea is that we want IIFRgpsetio, IIFRgpcontrol to be
**	efficient, ie. avoiding linear searches, but we don't want
**	to require compiler building of an involved structure to make
**	it work, because that is hard to maintain.  What goes on here:
**
**	The Set[] table is the table of actual parameters for the
**	default and alternate parameter sets.  The alternates will
**	inherit all values from default for unmentioned pid's.  What
**	we will do is construct arrays which can be indexed by pid
**	at runtime.
**
**	RUNTIME:
**
**	We copy the GP_PARM's from Set[] into TWO contigous
**	arrays, one of which is pointed into by our index arrays.
**	Each array contains GP_PARM's both for default and alternate
**	sets of parameters.  We can recover defaults by simply doing
**	one memory copy from the permanent set into the set in
**	active use.
**
**	For the defaults and all alternate sets, we allocate arrays
**	of pointers long enough to be indexed by the parameter id's.
**	the pointer will point to the appropriate parameter.  When
**	we choose an alternate or default parameter set, we then
**	simply choose the appropriate index array to use.
**
**	Picture:
**
**	Parm_sets:
**		alt = 0			(pointer array indexed by pid)
**		array --------------->0 1 .........  max. id
**						|
**		 ....				| points to appropriate item
**						|
**						V
**					    Parm_cur
**						^
**						|
**						| points to different item
**						| for all pid's having a
**						| value different than the
**						| alt = 0 set.
**		alt = n				|
**		array --------------->0 1 .........  max. id
**
**		IIFR_gparm = array item of currently active set
**
**		Parm_save is a copy of the original Parm_cur.
**
**	What runtime routines do to use this structure:
**
**		IIFRgpsetio - index IIFR_gparm based on pid, and set pointed to
**			item
**		IIFRgpget - read indexed item in IIFR_gparm.
**		IIFRgpcontrol:
**			FSPS_OPEN - find entry in Parm_sets and set IIFR_gparm,
**				block copy Parm_save to Parm_cur.
**			FSPS_CLOSE - nothing
**			PSPS_RESET - block copy Parm_save to Parm_cur.
**
**/

/*
** compiler initializable structure - no unions
*/
typedef struct
{
    i4  pid;    /* parameter id */
    i4  alt;    /* alternate set this belongs to, or 0 */
    i4  type;    /* no lengths - implied by type */
    i4 ival;
    f8 fval;
    char *str;
} SETUP;

/*
** parameter set structure
*/
typedef struct
{
    i4  alt;
    GP_PARM **array;
} PARMSET;


/*
** Setup array defining initial parameter settings by id.
** This isn't readonly.  We sort it for convenience in the
** initialization algorithm.
**
** There should be an alt = 0 entry here for every parameter id
** defined in fsiconsts.h.  Parameters which differ for an alternate
** set should be entered as another entry with alt = the alternate set.
** Seperate entries have to be made for an alternate set only for those
** parameters differing from the alt = 0 entry.
*/
static SETUP Set[] =
{
    { FSP_SROW, 0, DB_INT_TYPE, 0, 0, NULL},
    { FSP_SCOL, 0, DB_INT_TYPE, 0, 0, NULL},
    { FSP_ROWS, 0, DB_INT_TYPE, 0, 0, NULL},
    { FSP_COLS, 0, DB_INT_TYPE, 0, 0, NULL},
    { FSP_BORDER, 0, DB_INT_TYPE, FSBD_DEF, 0, NULL},
    { FSP_STYLE, 0, DB_INT_TYPE, FSSTY_DEF, 0, NULL},
    { FSP_SCRWID, 0, DB_INT_TYPE, FSSW_CUR, 0, NULL}
};

#define NUM_SETS (sizeof(Set)/sizeof(Set[0]))

/*
** runtime structure set up by IIFRgpinit
*/
static i2 Loc_tag = -1;

static bool Is_open = FALSE;	/* state flag for IIFRgpcontrol */

static PARMSET *Parm_sets;	/* parameter sets */
static i4  Num_pset;		/* length of Parm_sets */

static GP_PARM Parm_save[NUM_SETS];	/* saved parameters */
static GP_PARM Parm_cur[NUM_SETS];	/* current parameter */

static i4  compar();

/*
** Only items which need to be known to GPGET() macro
*/
GLOBALREF GP_PARM **IIFR_gparm;		/* parameter index array */

GLOBALREF i4  IIFR_maxpid ;		/* Maximum id, for error checks */

/*{
** Name:	IIFRgpinit -	Initialize global parameters
**
** Description:
**	Routine called when user enters forms mode.  It sets up
**	the structure for managing global parmeters, as described above
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
**
** Exceptions:
**	Memory allocation failure.
**
** Side Effects:
**	Allocates memory.
**
** History:
**	4/88 (bobm)	written
*/

VOID
IIFRgpinit()
{
    i4  i,j;
    GP_PARM **parr;
    PARMSET *ps;

    /*
    ** sort Set by alt item.  This allows us to determine
    ** very easily how many alternate sets have been defined.
    ** Both for loops below depend on this sort.
    */
    IIUGqsort((char *) Set, NUM_SETS, sizeof(SETUP), compar);

    /*
    ** count changes of alt item value, and get maximum
    ** parameter id.  Also copy individual parameters into
    ** arrays.
    */
    if (Set[0].alt != 0)
	IIFDerror(E_FR0050_NegAlt,0);

    Num_pset = 1;
    IIFR_maxpid = 0;
    for (i=0; i < NUM_SETS; ++i)
    {
	if (i > 0 && Set[i].alt != Set[i-1].alt)
	    ++Num_pset;
	if (Set[i].pid > IIFR_maxpid)
	    IIFR_maxpid = Set[i].pid;
	if (Set[i].pid < 0)
	{
	    IIFDerror(E_FR0051_NegPid,0);
	    Set[0].pid = 0;
	}
	switch (Parm_save[i].type = Set[i].type)
	{
	case DB_INT_TYPE:
	    Parm_save[i].dat.db_i4type = Set[i].ival;
	    break;
	case DB_FLT_TYPE:
	    Parm_save[i].dat.db_f8type = Set[i].fval;
	    break;
	case DB_VCH_TYPE:
	    Parm_save[i].dat.db_cptype = Set[i].str;
	    Parm_save[i].length = STlength(Set[i].str);
	    break;
	default:
	    IIFDerror(E_FR0052_BadType,2,&i,&(Set[i].type));
	    Parm_save[i].type = DB_INT_TYPE;
	    Parm_save[i].dat.db_i4type = 0;
	}
	STRUCT_ASSIGN_MACRO(Parm_save[i],Parm_cur[i]);
    }

    /*
    ** allocate PARMSET array and an index array for each.
    **
    ** For string variables we allocate a seperate buffer for the
    ** current string, so that at runtime we can overwrite the buffer
    ** if the current string length is long enough, or allocate a new
    ** one if it is not.
    */
    ++IIFR_maxpid;
    Loc_tag = FEgettag();
    Parm_sets = (PARMSET *) FEreqmem(Loc_tag,
	    (sizeof(PARMSET)+IIFR_maxpid)*Num_pset, FALSE, NULL);
    if (Parm_sets == NULL)
	IIUGbmaBadMemoryAllocation(ERx("IIFRgpinit"));
    for (i=0; i < IIFR_maxpid; ++i)
    {
	if (Parm_cur[i].type == DB_VCH_TYPE)
	{
	    Parm_cur[i].dat.db_cptype =
				FEtsalloc(Loc_tag, Parm_cur[i].dat.db_ctype);
	    if (Parm_cur[i].dat.db_ctype == NULL)
		IIUGbmaBadMemoryAllocation(ERx("IIFRgpinit"));
	}
    }

    /*
    ** set array pointers.
    */
    parr = (GP_PARM **)(Parm_sets+1);
    for (i=0; i < Num_pset; ++i)
    {
	Parm_sets[i].array = parr;
	for (j=0; j < IIFR_maxpid; ++j)
	    parr[j] = NULL;
	parr += IIFR_maxpid;
    }
    IIFR_gparm = Parm_sets[0].array;

    /*
    ** on this pass through, set up index arrays.
    ** Since we sorted, all values belonging to a given
    ** alternate will be in a clump.  Since 0 is the first
    ** set, we can also copy the 0 index array as a starting
    ** point whenver we encounter the first item of a new
    ** alternate.
    */
    ps = Parm_sets;
    parr = ps->array;
    ps->alt = Set[0].alt;
    parr[Set[0].pid] = Parm_cur;
    for (i=1; i < NUM_SETS; ++i)
    {
	if (Set[i].alt != Set[i-1].alt)
	{
	    ++ps;
	    parr = ps->array;
	    ps->alt = Set[i].alt;
	    MEcopy((PTR) Parm_sets[0].array,
		(u_i2) (IIFR_maxpid*sizeof(GP_PARM *)), (PTR) parr);
	}
	parr[Set[i].pid] = Parm_cur + i;
    }
}

/*
** comparison function for sorting Set[] array
*/
static i4
compar(p1,p2)
char *p1;
char *p2;
{
    if (((SETUP *) p1)->alt < ((SETUP *) p2)->alt)
	return (-1);
    if (((SETUP *) p1)->alt > ((SETUP *) p2)->alt)
	return (1);
    return(0);
}

/*{
** Name:	IIFRgpfree -	free memory
**
** Description:
**	Free memory allocated at time of IIFRgpinit.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
**
** Side Effects:
**	Allocates memory.
**
** History:
**	4/88 (bobm)	written
*/

VOID
IIFRgpfree ()
{
    if (Loc_tag > 0)
	FEfree(Loc_tag);
    IIFR_maxpid = -1;
}

/*{
** Name:	IIFRgpsetio - set a global parameter
**
** Description:
**	Set a global parameter for a pending runtime call.  It is perfectly
**	legal to set a given parameter several times before actually making
**	the runtime call - the final call will be effective.
**
**	It is an error to call this routine except between pairs of
**	IIFRgpcontrol(FSPS_OPEN) and IIFRgpcontrol(FSPS_CLOSE|RESET) calls.
**
**	This routine is part of RUNTIME's external interface.
**	
**	The datatype specification is duplicated from other runtime routines,
**	so that it may be used in a similar fashion.
**
** Inputs:
**	pid	identifier for parameter being set.
**	nullind	null indicator
**	isvar	is data actually a pointer?
**	type	data type
**	len	data length
**	val	data.
**
** Example and Code Generation:
**	## .... with ...... startrow = 7
**
**	....
**	IIFRgpsetio (FSP_SROW,0,0,DB_INT_TYPE,sizeof(i4),7);
**	....
**
*/

/* VARARGS5 */
VOID
IIFRgpsetio(i4 pid,i2 *nullind,bool isvar,i4 type,i4 len,PTR val)
{
    DB_EMBEDDED_DATA edv;
    DB_DATA_VALUE dbv;
    GP_PARM *pptr;
    i4  slen;

    AFE_DCL_TXT_MACRO(DB_MAXSTRING+1) txt;

    /*
    ** no error message - let higher levels handle "no forms" if appropriate
    **
    ** In particular, ## message when forms mode unset shouldn't error.
    */
    if (IIFR_maxpid <= 0)
	return;

    if (pid < 0 || pid >= IIFR_maxpid)
    {
	IIFDerror(E_FR0053_BadPid,1,&pid);
	return;
    }

    if ((pptr = IIFR_gparm[pid]) == NULL)
    {
	IIFDerror(E_FR0053_BadPid,1,&pid);
	return;
    }

    /*
    **  Build an embedded data value from the caller's parameters
    */

    edv.ed_type = type;
    edv.ed_length = len;
    edv.ed_data = isvar ? val : (PTR) &val;
    edv.ed_null = nullind;

    /*
    **  Convert the embedded data value into a db data value of
    **  the type required by the set operation.
    */

    switch(pptr->type)
    {
	case DB_INT_TYPE:
	    dbv.db_datatype = DB_INT_TYPE;
	    dbv.db_length = sizeof(i4);
	    dbv.db_prec = 0;
	    dbv.db_data = (PTR) &(pptr->dat.db_i4type);
	    break;

	case DB_FLT_TYPE:
	    dbv.db_datatype = DB_FLT_TYPE;
	    dbv.db_length = sizeof(f8);
	    dbv.db_prec = 0;
	    dbv.db_data = (PTR) &(pptr->dat.db_f8type);
	    break;

	case DB_VCH_TYPE:
	    dbv.db_datatype = DB_VCH_TYPE;
	    dbv.db_length = DB_MAXSTRING;
	    dbv.db_prec = 0;
	    dbv.db_data = (PTR) &txt;
	    break;

	default:
	    dbv.db_datatype = DB_INT_TYPE;
	    dbv.db_length = sizeof(i4);
	    dbv.db_prec = 0;
	    dbv.db_data = (PTR) &(pptr->dat.db_i4type);
	    IIFDerror(E_FR0054_BadType,0);
	    break;
    }

    if ( adh_evcvtdb( FEadfcb(), &edv, &dbv ) != OK )	 
    {
	IIFDerror(E_FR0055_ConvFail, 0);
	return;
    }

    /*
    ** If a string, copy over old buffer, or allocate new buffer
    ** if longer.
    */
    if (pptr->type == DB_VCH_TYPE)
    {
	(txt.text)[DB_MAXSTRING] = EOS;
	STtrmwhite((char *) txt.text);
	if ((slen = STlength((char *) txt.text)) > pptr->length)
	{
	    if ((pptr->dat.db_cptype = FEtsalloc(Loc_tag,txt.text)) == NULL)
		IIUGbmaBadMemoryAllocation(ERx("IIFRgpsetio"));
	    pptr->length = slen;
	}
	else
	    STcopy(txt.text,pptr->dat.db_cptype);
    }
}

/*{
** Name:	IIFRgpcontrol - Global Parameter State
**
** Description:
**	This is the "bracket" routine to flag the beginning and end of
**	parameter setting calls, or to reset the parameters to the default
**	state
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	state	FSPS_OPEN	start of a new series of set calls.  All
**				parameter values will be initialized.  It
**				is an error to call this again without
**				an intervening FSPS_CLOSE or FSPS_RESET.
**
**		FSPS_CLOSE	end of a series.  It is an error to call
**				this unless the last call was FSPS_OPEN.
**
**		FSPS_RESET	reset all parameters to default values.
**
**	alt	an identifier for a set of alternate values to be set by
**		FSPS_OPEN which is different than the normal defaults.  Use
**		0 to indicate the normal defaults.  Ignored on FSPS_CLOSE
**		and FSPS_RESET.
**
** Example and Code Generation:
**	## display form with style = popup(startrow=7)
**
**	......
**	IIFRgpcontrol(FSPS_OPEN,0);
**
**	IIFRgpsetio(FSP_STYLE,0,0,DB_INT_TYPE,sizeof(i4),FSSTY_POP);
**	IIFRgpsetio(FSP_SROW,0,0,DB_INT_TYPE,sizeof(i4),7);
**
**	IIFRgpcontrol(FSPS_CLOSE,0); * keep lint happy *
**	IIdispfrm(....);
**
**	or, if statement aborting error occurs after some set calls:
**
**	IIFRgpcontrol(FSPS_RESET,0);
**
*/

VOID
IIFRgpcontrol(i4 state,i4 alt)
{
    i4  i;

    /*
    ** no error message - let higher levels handle "no forms" if appropriate
    **
    ** In particular, ## message when forms mode unset shouldn't error.
    */
    if (IIFR_maxpid <= 0)
	return;

    switch(state)
    {
    case FSPS_OPEN:
	if (Is_open)
	{
	    IIFDerror(E_FR0056_BadOpen,0);
	}
	Is_open = TRUE;
	for (i=0; i < Num_pset; ++i)
	    if (Parm_sets[i].alt == alt)
		break;
	if (i >= Num_pset)
	{
	    IIFDerror(E_FR0057_BadAlt,1,&i);
	    i = 0;
	}
	IIFR_gparm = Parm_sets[i].array;
	MEcopy((PTR) Parm_save, (u_i2) (NUM_SETS*sizeof(GP_PARM)),
		(PTR) Parm_cur);
	break;
    case FSPS_RESET:
	Is_open = FALSE;
	MEcopy((PTR) Parm_save, (u_i2) (NUM_SETS*sizeof(GP_PARM)),
		(PTR) Parm_cur);
	IIFR_gparm = Parm_sets[0].array;
	break;
    case FSPS_CLOSE:
	if (!Is_open)
	{
	    IIFDerror(E_FR0058_BadClose,0);
	}
	Is_open = FALSE;
	break;
    default:
	IIFDerror(E_FR0059_BadState,1,&i);
	break;
    }
}
