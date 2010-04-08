/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <clconfig.h>
# include <cs.h>
# include <mo.h>
# include <cm.h>

# include <csinternal.h>
# include "cslocal.h"

# include "csmgmt.h"

#ifdef USE_MO_WITH_SEMS

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
**	5-Dec-1992 (daveb)
**	    Make a bunch of things unsigned output; some were coming out
**	    with minus signs.
**	13-Dec-1992 (daveb)
**	    Use == instead of '=' in scribble_check_get.  Caused horrible
**	    overwrite bugs.
**	13-Jan-1993 (daveb)
**	    Rename MO objects to include a .unix component.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**      01-sep-93 (smc)
**          Changed MOulongout to MOptrout to be portable to axp_osf.
**	20-sep-93 (mikem)
**	    Added new entries to the CS_sem_classes[] table to support new 
**	    statistics on semaphore contention.
**      10-Mar-1994 (daveb) 60514
**          use ptr get routines for pointers inseted of int or
**          unsigned int.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  Move do_mo_sems object
**  	    from csinterface.c to csmosem.c  Turn on semaphore objects
**  	    by default.
**	02-Jun-1995 (jenjo02)
**	    Removed references to cs_cnt_sem, cs_long_cnt, cs_ms_long_et,
**	    cs_list, all of which are obsolete and due to be scrunched
**	    out of the structure.
**      13-Aug-1996 (jenjo02)
**          Modified to be in accord with thread's version of CS_SEMAPHORE.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**	    added csmt mo defs
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	15-Nov-1999 (jenjo02)
**	    ifdef'd out all the code with USE_MO_WITH_SEMS. It's been
**	    a long time since MO has been used with semaphores, so this
**	    modules really isn't needed any more.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-jul-2003 (devjo01)
**	    Use MOsidget to retreive session ID values.
**/

static i4		      cs_mo_sems = TRUE; /* attach sems? */


MO_INDEX_METHOD CS_sem_index;

MO_GET_METHOD CS_sem_type_get;
MO_GET_METHOD CS_sem_name_get;

MO_GET_METHOD CS_sem_scribble_check_get;

/* ---------------------------------------------------------------- */

static char index_class[] = "exp.clf.unix.cs.sem_index";

GLOBALDEF MO_CLASS_DEF CS_sem_classes[] =
{
  { 0, "exp.clf.unix.cs.do_mo_sems", sizeof(cs_mo_sems),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&cs_mo_sems, MOcdata_index },

    /* this is really attached, and uses the default index method */

  { MO_INDEX_CLASSID|MO_CDATA_INDEX, index_class,
	0, MO_READ, 0,
	0, MOstrget, MOnoset, 0, MOname_index },

   /* these have local methods for formatting */

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_addr", 
	0, MO_READ, index_class,
	0, MOpvget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_type", 
	0, MO_READ, index_class,
	0, CS_sem_type_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_scribble_check", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_scribble_check),
	MO_READ, index_class, 0,
	CS_sem_scribble_check_get, MOnoset, 0, MOidata_index },
	
   /* These are all builtin methods */

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_value",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_value), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_value), MOintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_count", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_count), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_count), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_owner", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_owner), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_owner), MOsidget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_pid", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_pid), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_pid), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_name", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_name),
	MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_name), CS_sem_name_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_init_pid", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_init_pid),
	MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_init_pid), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.smsx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smsx_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smsx_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.smxx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smxx_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smxx_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.smcx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smcx_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smcx_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.smx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smx_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smx_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.sms_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_sms_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_sms_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.smc_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smc_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smc_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.smcl_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smcl_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smcl_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.cs_smsp_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smsp_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smsp_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.cs_smmp_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smmp_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smmp_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.cs_smnonserver_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smnonserver_count),
        MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smnonserver_count), 
        MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_init_addr", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_init_addr),
	MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_init_addr), MOptrget, MOnoset,
	0, MOidata_index },

# ifdef OS_THREADS_USED
  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_sid",
        MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sid), MO_READ, index_class,
        CL_OFFSETOF(CS_SEMAPHORE, cs_sid), MOsidget, MOnoset,
        0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.unix.cs.sem_stats.sms_hwm",
        MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_sms_hwm),
        MO_READ, index_class,
        CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_sms_hwm),
        MOuintget, MOnoset, 0, MOidata_index },
# endif /* OS_THREADS_USED */

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
*/

void
CS_sem_make_instance( CS_SEMAPHORE *sem, char *buf )
{
    char pidbuf[ 80 ];
    char addrbuf[ 80 ];

    (void) MOulongout( 0, sem->cs_sem_init_pid, sizeof(pidbuf), pidbuf );
    (void) MOptrout( 0, (PTR)sem->cs_sem_init_addr,
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
**	11-Aug-1993 (daveb)
**	    turn off for now.
*/

STATUS
CS_sem_attach( CS_SEMAPHORE *sem )
{
    char buf[ 80 ];
    STATUS stat = OK;

    if( cs_mo_sems )
    {
	CS_sem_make_instance( sem, buf );
	stat =  MOattach( MO_INSTANCE_VAR, index_class, buf, (PTR)sem );
    }
    return( stat );
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
*/

STATUS
CS_detach_sem( CS_SEMAPHORE *sem )
{
    char buf[ 80 ];
    STATUS stat = OK;

    if( cs_mo_sems )
    {
	CS_sem_make_instance( sem, buf );
	if( sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD )
	{
	    TRdisplay("CS detach sem:  sem %p (%s) looks bad (%x != %x)!\n",
		      sem, buf, sem->cs_sem_scribble_check, CS_SEM_LOOKS_GOOD );
	}
	stat =  MOdetach( index_class, buf );
    }
    return( stat );
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
    return( MOstrout( MO_VALUE_TRUNCATED,
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
**	    Use == instead of '=' in scribble_check_get.  Caused horrible
**	    overwrite bugs.
*/

STATUS
CS_sem_scribble_check_get(i4 offset,
			  i4  objsize,
			  PTR object,
			  i4  luserbuf,
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

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
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

    return( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ));
}


#endif /* USE_MO_WITH_SEMS */
