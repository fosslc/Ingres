/*
** Copyright (C) 1987, 2004 Ingres Corporation
*/

# include <compat.h>
# include <sys\types.h>
# include <sp.h>
# include <st.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <cm.h>
# include <csinternal.h>
# include <tr.h>

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
**	03-mar-1997 (canor01)
**	    Move definitions of global MO objects to csdata.c.
**	14-may-97 (mcgem01)
**	    Clean up compiler warning by including tr.h
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	24-jul-2000 (somsa01 for jenjo02)
**	    ifdef'd out all the code with USE_MO_WITH_SEMS. It's been
**	    a long time since MO has been used with semaphores, so this
**	    modules really isn't needed any more.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**/

GLOBALREF i4    IIcs_mo_sems;  /* attach sems? */

GLOBALREF char  CSsem_index_name[];

/* ---------------------------------------------------------------- */

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
CS_sem_make_instance(CS_SEMAPHORE * sem, char *buf)
{
    char pidbuf[80];
    char addrbuf[80];

    (void) MOulongout( 0, sem->cs_sem_init_pid, sizeof(pidbuf), pidbuf);
    (void) MOptrout( 0, (PTR)sem->cs_sem_init_addr,
    		sizeof(addrbuf), addrbuf);
    STpolycat( 3, pidbuf, ".", addrbuf, buf);
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
*/

STATUS
CS_sem_attach(CS_SEMAPHORE * sem)
{
	char            buf[80];
	STATUS          stat = OK;

	if (IIcs_mo_sems)
	{
	    CS_sem_make_instance(sem, buf);
 	    stat = MOattach(MO_INSTANCE_VAR, CSsem_index_name, buf, (PTR) sem);
	}

	return (stat);
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
CS_detach_sem(CS_SEMAPHORE * sem)
{
	char            buf[80];
	STATUS          stat;

	CS_sem_make_instance(sem, buf);

	if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD )
	{
	    TRdisplay("CS detach sem:  sem %p (%s) looks bad (%x != %x)!\n",
	    	sem, buf, sem->cs_sem_scribble_check, CS_SEM_LOOKS_GOOD );
	}

	stat = MOdetach(CSsem_index_name, buf);
	return (stat);
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
CS_sem_type_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
	CS_SEMAPHORE   *sem = (CS_SEMAPHORE *) object;
	return (MOstrout(MO_VALUE_TRUNCATED,
			 sem->cs_type == 0 ? "single" : "multi",
			 luserbuf, userbuf));
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
CS_sem_scribble_check_get(i4  offset,
			  i4  objsize,
			  PTR object,
			  i4  luserbuf,
			  char *userbuf)
{
	CS_SEMAPHORE   *sem = (CS_SEMAPHORE *) object;
	char           *str;

	if (sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD)
	    str = "looks ok";
	else if (sem->cs_sem_scribble_check == CS_SEM_WAS_REMOVED)
	    str = "REMOVED";
	else
	    str = "CORRUPT";

	return (MOstrout(MO_VALUE_TRUNCATED, str, luserbuf, userbuf));
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
CS_sem_name_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
	CS_SEMAPHORE   *sem = (CS_SEMAPHORE *) object;

	char            buf[CS_SEM_NAME_LEN + 1];	/* buffer with printable
							 * name string */
	char           *p;
	char           *q;

	q = buf;
	p = sem->cs_sem_name;

	for (; *p && p < &sem->cs_sem_name[CS_SEM_NAME_LEN];
	     p = CMnext(p), q++) {
		if (CMprint(p))
			*q = *p;/* FIXME! Wrong on dbl byte */
		else
			*q = '#';
	}
	*q = EOS;

	return (MOstrout(MO_VALUE_TRUNCATED, buf, luserbuf, userbuf));
}

#endif /* USE_MO_WITH_SEMS */
