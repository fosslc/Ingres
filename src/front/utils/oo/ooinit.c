/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ooclass.h>
#include	<oodefine.h>
#include	<oosymbol.h>
# include	<uigdata.h>
# include	"ooldef.h"
#include	<ooproc.h>
#include	"eroo.h"
# include	<cv.h>

/**
** Name:	ooinit.c -	Object Manager Initialization Module.
**
** Description:
**	Contains the Object Manager Module initialization routine.
**	and routines for managing the Object Table.
**
**	IIOOinit()	initialize object manager variables.
**	OOhash()	probe or insert into iiOOtable.
**	OOp()		return an (OO_OBJECT *) for an OOID
**	OOpclass()	return an (OO_CLASS *) to class structure in memory
**
** Globals defined:
**	OO_OBJECT iiOOnil	the nil object
**	i4 IIuserIndex		order of User, Dba names in sort order (0 or 1).
**	OOTABENTRY **iiOOtable	Object hash table
**	i4 iiOOsize=0;		number of start-up objects
**	i4 DDebug=0;		debug flag
**
** History:
**	Revision 6.4  90/12/19  dave
**	VMS shared lib changes.  Added include of ooldef.h and declaration of
**	IIOO_DBA as indirect reference to iiOODBA.
**	90/09/06 (dkh) - Changed to use IIOOgdGetDBA() instead of direct
**			 reference to IIOO_DBA.  This eliminates exporting
**			 data references through the shared library layer.
**      22-oct-90 (sandyd)
**              Fixed #include of local ooldef.h to use "" instead of <>.
**      12-sep-91 (davel)
**              Fixed bug 39706 in OOhash() - do not overwrite the nil object
**		in the hash table.
**	10/15/93 (kchin)
**		Changed datatype of variable parameters to PTR in D() and DO(),
**		this is to make it tally with the change made in FEmsg(),
**		in front/utils/ug/femsg.c.
**
**	Revision 6.3  90/04  wong
**	Add initialization of class names from 'env' member for
**	internationalization.
**
**	Revision 6.2  88/12  wong
**	Prefixed some names with 'iiOO' or 'IIoo'.
**	89/06  wong  JupBug #6470.
**	Reworked hash algorithm to handle overflow correctly.
**
**	Revision 4.0  85/12  peterk
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-Jun-2001 (horda03)
**          Rather than fix the size of the iiOOtable as OOTABLESIZE, as no
**          matter how large we make it, someone is bound to require more
**          entries, allow the user to specify a larger size. The
**          environment variable II_OO_TABLE_SIZE has been added, and if
**          defined greater than OOTABLESIZE, the new value will be used.
**          (45781)
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**/

/* History:
**	09-jul-1991 (jhw)  Modified bit initialization as per JohnR's change
**		for hp3_ux5 but with correct bits:
**			levelBits	= 0x7fff (15 bits, all levels)
**			dirty		= 0 (1 bit, FALSE [not dirty])
**			inDatabase	= 1 (1 bit, n/a since 'dbObject' FALSE)
**			dbObject	= 0 (1 bit, FALSE [not a DB object])
**			tag		= 0 (14 bits, n/a, no tag)
**      03-Feb-96 (fanra01)
**          Extracted data for DLLs on Windows NT.
**      14-Feb-96 (fanra01)
**          Added iiOODBAsize to get correct size of structure.
**      21-nov-96 (hanch04)
**          Problems linking for unix
**      22-Jun-2001 (horda03)
**	    (hanje04) X-Integ of 451714
**          Rather than fix the size of the iiOOtable as OOTABLESIZE, as no
**          matter how large we make it, someone is bound to require more
**          entries, allow the user to specify a larger size. The
**          environment variable II_OO_TABLE_SIZE has been added, and if
**          defined greater than OOTABLESIZE, the new value will be used.
**          (45781)
**      24-Jul-2001 (horda03)
**          Added me.h. Required in above change.
**	25-Aug-2009 (kschendel) 121804
**	    Need ooproc.h to satisfy gcc 4.3.
*/

static i4	 	OOtabsize = 0; /* hash table size */

# if defined(NT_GENERIC)
GLOBALREF OO_OBJECT	iiOOnil[];


GLOBALREF i4		IIuserIndex;
GLOBALREF OO_OBJECT	**iiOOtable;	/* hash table */

GLOBALREF i4	iiOOsize;
GLOBALREF bool	DDebug;

GLOBALREF char	iiOODBA[];
GLOBALREF i4    iiOODBAsize;
# else
GLOBALDEF OO_OBJECT     iiOOnil[1] = {
        0, {0x7fff, 0, 1, 0, 0}, 1, ERx("nil"), 0, ERx(""), 1,
        ERx("the undefined object"),
};
 
GLOBALDEF i4            IIuserIndex = 0;
GLOBALDEF OO_OBJECT     **iiOOtable ZERO_FILL;
 
GLOBALDEF i4    iiOOsize        = 0;
GLOBALDEF bool  DDebug          = FALSE;
 
GLOBALDEF char  iiOODBA[FE_MAXNAME+1] ZERO_FILL;
GLOBALDEF i4    iiOODBAsize = sizeof(iiOODBA);
# endif             /* NT_GENERIC  */


/*{
** Name:	IIOOinit() -	Initialize Object Manager Tables.
**
** Description:
**	Initializes the Object Manager module tables.  Clients of the OO module
**	should pass in their own objects so other clients need not link in
**	unneeded code.
**
** Inputs:
**	objects	{OO_OBJECT []}  Array of client defined modules
**	
** Side Effects:
**	Sets up iiOOtable hash table.
**
** History:
**	12/85 (peterk) -- Written.
**	01/86 (jhw) -- Abstracted 'OOsetuser()'.
**	4/87 (peterk) -- modified for hashed iiOOtable.
**	01/89 (jhw) -- Added OO_OBJECT [] parameter.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**      22-Jun-2001 (horda03)
**         Allocate the memory for iiOOtable as the greater of OOTABLESIZE
**         and the environemnt variable II_OO_TABLE_SIZE.
**         (45781)
*/

VOID
IIOOinit ( objects )
OO_OBJECT	*objects[];
{
	register OO_OBJECT	**p;
	register i4		i;
        char                    *tblsize;
        i4                      size = 0;
#ifdef DDEBUG
	char		*ddebugp;
#endif

	VOID	OOsetuser();

        if (!OOtabsize)
        {
           NMgtAt( "II_OO_TABLE_SIZE", &tblsize);

           if (tblsize && *tblsize)
           {
               CVal(tblsize, &size);
           }

           OOtabsize = (size > OOTABLESIZE) ? size : OOTABLESIZE;

           /* Now allocate the memory for the iiOOtable */

           iiOOtable = (OO_OBJECT **) MEreqmem( 0, sizeof(OO_OBJECT *) * OOtabsize, TRUE, NULL );
        }

	if ( OOhash(OC_OBJECT, (OO_OBJECT *)NULL) == 0 )
	{ /* clear only on first entry */
		for (i = 0 ; i < OOtabsize ; ++i)
		{
			iiOOtable[i] = NULL;
		}

		for (p = IIooObjects ; *p != (OO_OBJECT *)NULL ; ++p)
		{
			if ( (*p)->name == NULL && (*p)->env != 0 )
				(*p)->name = ERget((*p)->env);
			_VOID_ OOhash((*p)->ooid, *p);
		}
		iiOOsize = p - IIooObjects;
	}

	if ( objects != NULL )
	{
		for (p = objects ; *p != (OO_OBJECT *)NULL ; ++p)
		{
			if ( (*p)->name == NULL && (*p)->env != 0 )
				(*p)->name = ERget((*p)->env);
			_VOID_ OOhash((*p)->ooid, *p);
		}
		iiOOsize += p - objects;
	}
#ifdef DDEBUG
	DDebug = ( NMgtAt(ERx("DDEBUG"), &ddebugp) == OK &&
			ddebugp != NULL && *ddebugp != EOS );
#endif
	OOsetuser(IIUIdbdata()->user);
	STlcopy( ERget(F_OO0001_DBA), iiOODBA, iiOODBAsize-1 );
}

/*{
** Name:	OOsetuser() -	Set Database User Index for Object Utility.
**
** Description:
**	This routine sets the global variable UserIndex to 0 or 1 according
**	to whether the passed database usercode precedes or follows
**	the DBA's database usercode in collating sequence.  The Object Utility
**	uses this information to retrieve database objects accessible to a user.
**
** Side Effects:
**	Sets global 'UserIndex'.
**
** History:
**	01/86 (jhw) -- Abstracted from 'OOinit()'.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
*/

VOID
OOsetuser (user)
char	*user;		/* user code */
{
	IIuserIndex = (STcompare(user, IIUIdbdata()->dba) <= 0 ? 0 : 1);
}

/*
**	D	-- debug output
*/
#ifdef DDEBUG
/* VARARGS1 */
VOID
D(s, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
PTR	s, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a12;
{
	if (DDebug)
		FEmsg(s,TRUE,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);
}
#else
VOID
D(){}
#endif

#ifdef DDEBUG
/* VARARGS1 */
VOID
D0(s, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
PTR	s, a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a12;
{
	if (DDebug)
		FEmsg(s,FALSE,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);
}
#else
VOID
D0(){}
#endif

/*{
** Name:	OOhash() -	hash an object id into hash table
**
** Description:
**	This routine is used to probe the iiOOtable hash table for
**	for a given ID, or to insert an ID and corresponding
**	OO_OBJECT pointer into the hash table.
**
** Input params:
**	OOID		id;	an object ID
**	OO_OBJECT	*obj;	if NULL, return index in iiOOtable
**					where ID is found, or 0 if
**					not found;
**				else insert ID and obj at appropriate
**					iiOOtable entry and return index, or
**					0 if no entry available.
**
** Returns:
**	{nat}	index in iiOOtable (or 0)
**
** Side Effects:
**	If obj != NULL, and an available slot is found, ID and obj are
**	inserted into iiOOtable.
*/

i4
OOhash ( id, object )
register OOID	id;
OO_OBJECT	*object;
{
	register i4	probe, orig;

	probe = orig = ((unsigned)id) % (OOtabsize - 1) + 1;

	/*
	** N.B.: probe will always be in the range:
	**	1 <= probe <= OOtabsize - 1
	*/
	while ( iiOOtable[probe] != NULL && iiOOtable[probe]->ooid != id )
	{ /* overflow */
		/* increment probe, wrapping to start of iiOOtable
		** but skip entry 0 (reserved for NULL ptr);
		*/
		++probe;
		if ( probe == OOtabsize )
			probe = 1;

		/* if we've cycled thru entire iiOOtable -
		**	  no hit and no empty slots.
		*/
		if ( probe == orig )
		{
			if ( object != NULL )
				IIUGerr( E_OO000E_No_available_slot,
						UG_ERR_ERROR, 1, id
				);
			return 0;
		}
	}

	if ( object != NULL )
		iiOOtable[probe] = object;
	else if ( iiOOtable[probe] == NULL )
		return 0;	/* not found */
	return probe;
}

/*{
** Name:	OOp() -	get ptr to object instance for an id.
**
** Description:
**	Given an OOID return a pointer to the corresponding
**	(OO_OBJECT) structure, in memory.
**
**	If object is not memory-resident, fetch its attributes
**	from ii_objects table in current database.
**	If object id cannot be resolved, a NULL ptr is returned.
**
** Input params:
**	OOID	id;		an object id
**
** Returns:
**	(OO_OBJECT *) 		ptr to object instance or NULL if error
**
** Side Effects:
**	If object not already in memory (in iiOOtable), causes
**	"Level-0" fetch (OO_OBJECT attributes, not including
**	long_remark) of object from ii_objects in currently open database
**	and inserts object into iiOOtable.
**
*/

OO_OBJECT *
OOp ( id )
OOID	id;
{
	register i4	p;
	OO_OBJECT	tmp;

	tmp.ooid = id;
	for (;;)
	{
		if ( (p = OOhash(id, (OO_OBJECT *)NULL)) != 0 )
			return iiOOtable[p];

		if ( (id = fetch0(&tmp)) == nil )
			return NULL;
	}
}


/*{
** Name:	OOpclass() -	return ptr to Class structure in memory
**
** Description:
**	Given an OOID for a Class object return a ptr to the corresponding
**	(OO_CLASS) structure, in memory.  This routine should never return
**	NULL, since all Class objects should be memory-resident.
**
** Input params:
**	OOID	id;		an object id for a Class object
**
** Returns:
**	(OO_CLASS *)		ptr to Class object in memory.
*/

OO_CLASS *
OOpclass(class)
register OOID class;
{
	register OO_CLASS	*classp = (OO_CLASS *)OOp(class);

	if ( classp == NULL )
		return NULL;

	/* check if rest of type data at level 1 already present */
	if ( !BTtestF(O2->level, classp->data.levelbits) )
	{
		if ( OOsndSelf(classp, _fetch) == nil )
			return NULL;
	}
	return classp;
}



/*{
** Name:	IIOOgdGetDBA - Return pointer to iiOODBA.
**
** Description:
**	This routine simply returns the address of iiOODBA to the caller
**	so we can elminate exporting direct data references through
**	the shared library layer.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		ptr	Address of iiOODBA.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/06/90 (dkh) - Initial version.
*/
char *
IIOOgdGetDBA()
{
	return(iiOODBA);
}
