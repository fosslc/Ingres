/*
** Copyright (C) 1992, 2008 Ingres Corporation  
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <cm.h>
# include <tr.h>

# include <exhdef.h>
# include <csinternal.h>
# include "cslocal.h"

# include "csmgmt.h"

/**
** Name:	csmosem.c	- MO interface for CS Semaphores.
**
** Description:
**	Defines MO classes and client attach/detach functions for semaphores.
**
** Functions:
**
**	CS_sem_make_instance	- make an instance buffer for a sem.
**	CS_sem_attach		- make the sempahore known.
**	CS_detach_sem		- Make this semaphore unknown through MO.
**	CS_sem_type_get		- format sem type nicely
**	CS_sem_scribble_check_get - format scribble nicely.
**	CS_sem_name_get		- format sem name nicely.
**
** History:
**	29-Oct-1992 (daveb)
**	    documented.
**	2-Nov-1992 (daveb)
**	    make CS_sem_classes GLOBALCONSTDEF for VMS sharability.
**	9-Dec-1992 (daveb)
**	    make some things unsigned that should be.
**	    change .cl.clf to .clf.
**	13-Dec-1992 (daveb)
**	    Use == instead of '=' in scribble_check_get.  Caused horrible
**	    overwrite bugs.
**	13-Jan-1993 (daveb)
**	    Rename exp.clf.cs to exp.clf.vms.cs in all MO objects.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-Aug-93 (daveb)
**	    Turn off semaphore attaches for now.  We've got a leak to fix.
**	08-jan-95 (dougb)
**	    Cross integrate following Unix changes.  In particular, turn on
**	    semaphore attaches again.  (Rest of changes are mostly for
**	    consistency with Unix version.)
**		01-sep-93 (smc)
**	    Changed MOulongout to MOptrout to be portable to axp_osf.
**		10-Mar-1994 (daveb) 60514
**	    Use ptr get routines for pointers instead of int or
**	    unsigned int.
**		17-May-1994 (daveb) 59127
**	    Fixed semaphore leaks, named sems.  Move do_mo_sems object
**	    from csinterface.c to csmosem.c  Turn on semaphore objects
**	    by default.
**		02-Jun-1995 (jenjo02)
**	    Removed references to cs_cnt_sem, cs_long_cnt, cs_ms_long_et,
**	    cs_list, all of which are obsolete and due to be scrunched
**	    out of the structure. ==> only cs_list was referenced on VMS <==
**      06-feb-98 (kinte01)
**          Cross integrate change from VAX VMS CL to Alpha CL
**          10-Dec-1997 rosga02)
**            Added time stamp and sem name to trace data
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	15-jul-2003 (devjo01)
**	    Use MOsidget() to retreive sessions ID's.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-dec-2008 (joea)
**	    Fix above, per 2-nov-1992 comment.
**/

static i4		      cs_mo_sems = TRUE; /* attach sems? */


MO_INDEX_METHOD CS_sem_index;

MO_GET_METHOD CS_sem_type_get;
MO_GET_METHOD CS_sem_name_get;

MO_GET_METHOD CS_sem_scribble_check_get;

/* ---------------------------------------------------------------- */

static char index_class[] = "exp.clf.vms.cs.sem_index";

GLOBALCONSTDEF MO_CLASS_DEF CS_sem_classes[] =
{
  { 0, "exp.clf.vms.cs.do_mo_sems", sizeof(cs_mo_sems),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&cs_mo_sems, MOcdata_index },

    /* this is really attached, and uses the default index method */

  { MO_INDEX_CLASSID|MO_CDATA_INDEX, index_class,
	0, MO_READ, 0,
	0, MOstrget, MOnoset, 0, MOname_index },

   /* these have local methods for formatting */

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_addr", 
	0, MO_READ, index_class,
	0, MOpvget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_type", 
	0, MO_READ, index_class,
	0, CS_sem_type_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_scribble_check", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_scribble_check),
	MO_READ, index_class, 0,
	CS_sem_scribble_check_get, MOnoset, 0, MOidata_index },
	
   /* These are all builtin methods */

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_value",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_value), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_value), MOintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_count", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_count), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_count), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_owner", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_owner), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_owner), MOsidget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_pid", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_pid), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_pid), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_name", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_name),
	MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_name), CS_sem_name_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_init_pid", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_init_pid),
	MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_init_pid), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.vms.cs.sem_init_addr", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_init_addr),
	MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_init_addr), MOptrget, MOnoset,
	0, MOidata_index },

  { 0 }
};




/*{
** Name:	CS_sem_make_instance - make an instance buffer for a sem.
**
** Description:
**	Given the data in a semaphore, format up an appropriate
**	instance buffere using the saved init_pid and init_addr.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem
**
**
** Outputs:
**	buf
**
** Returns:
**	none.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	08-jan-95 (dougb)
**	    Cross integrate following Unix change:
**		01-sep-93 (smc)
**	    Changed MOulongout to MOptrout to be portable to axp_osf.
*/

void
CS_sem_make_instance( CS_SEMAPHORE *sem, char *buf )
{
    char pidbuf[ 80 ];
    char addrbuf[ 80 ];

    (void)MOulongout( 0, sem->cs_sem_init_pid, sizeof(pidbuf), pidbuf );
    (void)MOptrout( 0, (PTR)sem->cs_sem_init_addr,
		   sizeof(addrbuf), addrbuf );
    STpolycat( 3, pidbuf, ".", addrbuf, buf );
}

/* ---------------------------------------------------------------- */


/*{
** Name:	CS_sem_attach - make the sempahore known.
**
** Description:
**	Make this semaphore known.  Uses the data in the sempahore
**	to create an instance variable in a local buffer, which
**	is saved by the MOattach.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem	the sem to attach.
**
** Outputs:
**	none.
**
** Returns:
**	MOattach value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	08-jan-95 (dougb)
**	    Cross integrate following Unix change:
**		17-May-1994 (daveb) 59127
**	    Fixed semaphore leaks, named sems.  Move do_mo_sems object
**	    from csinterface.c to csmosem.c  Turn on semaphore objects
**	    by default.
*/

STATUS
CS_sem_attach( CS_SEMAPHORE *sem )
{
    char buf[ 80 ];
    STATUS stat = OK;

    if ( cs_mo_sems )
    {
	CS_sem_make_instance( sem, buf );
	stat =  MOattach( MO_INSTANCE_VAR, index_class, buf, (PTR)sem );
    }

    return ( stat );
}


/*{
** Name:	CS_detach_sem - Make this semaphore unknown through MO.
**
** Description:
**	Make this semaphore unknown through MO.
**
**	FIXME: This is not a safe approach, becuase you can't detach a
**	semaphore that has been corrupted.  (We rely on data in the
**	semaphore to provide the global name).  If it fails, we may need to
**	to an MO scan of the index class to locate it at it's current
**	attach point.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		the sem to detach.
**
** Outputs:
**	none.
**
** Returns:
**	MOdetach return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	08-jan-95 (dougb)
**	    Cross integrate following Unix change:
**		17-May-1994 (daveb) 59127
**	    Fixed semaphore leaks, named sems.  Move do_mo_sems object
**	    from csinterface.c to csmosem.c  Turn on semaphore objects
**	    by default.
*/

STATUS
CS_detach_sem( CS_SEMAPHORE *sem )
{
    char buf[ 80 ];
    STATUS stat = OK;

    if ( cs_mo_sems )
    {
	CS_sem_make_instance( sem, buf );
	if ( sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD )
	{
            TRdisplay( "@ CS detach sem:  sem %p (%s) looks bad (%x != %x)! %0.30s\n",
                      sem, buf, sem->cs_sem_scribble_check,
                      CS_SEM_LOOKS_GOOD, sem->cs_sem_name );
	}
	stat = MOdetach( index_class, buf );
    }

    return ( stat );
}


/*{
** Name:	CS_sem_type_get - format sem type nicely
**
** Description:
**	formats a sem type to the string "single" or "multi"
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to a CS_SEMAPHORE.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with "single" or "multi"
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
CS_sem_type_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    CS_SEMAPHORE *sem = (CS_SEMAPHORE *)object;
    return ( MOstrout( MO_VALUE_TRUNCATED,
		      sem->cs_type == 0 ? "single" : "multi",
		      luserbuf, userbuf ));
}



/*{
** Name:	CS_sem_scribble_check_get - format scribble nicely.
**
** Description:
**	formats a sem type to the strings "looks ok", "REMOVED"
**	or "CORRUPT".
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to a CS_SEMAPHORE.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with "single" or "multi"
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	13-Dec-1992 (daveb)
**	    Use == instead of '='.  Caused horrible overwrite bugs!
*/

STATUS
CS_sem_scribble_check_get(i4 offset,
			  i4 objsize,
			  PTR object,
			  i4 luserbuf,
			  char *userbuf)
{
    CS_SEMAPHORE *sem = (CS_SEMAPHORE *)object;
    char *str;

    if( sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD )
	str = "looks ok";
    else if( sem->cs_sem_scribble_check == CS_SEM_WAS_REMOVED )
	str = "REMOVED";
    else
	str = "CORRUPT";

    return ( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}

/*{
** Name:	CS_sem_name_get - format sem name nicely.
**
** Description:
**	string get, only changes any unlikely characters
**	that might be lurking into '#'.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored
**	object		point to a CS_SEMAPHORE.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with name.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
CS_sem_name_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    CS_SEMAPHORE *sem = (CS_SEMAPHORE *)object;

    char buf[ CS_SEM_NAME_LEN + 1 ]; /* buffer with printable name string */
    char *p;
    char *q;

    q = buf;
    p = sem->cs_sem_name;

    for( ; *p && p < &sem->cs_sem_name[ CS_SEM_NAME_LEN ];
	p = CMnext( p ), q++ )
    {
	if( CMprint( p ) )
	    *q = *p;		/* FIXME! Wrong on dbl byte */
	else
	    *q = '#';
    }
    *q = EOS;

    return ( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ));
}
