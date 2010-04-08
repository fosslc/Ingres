/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <si.h>
# include <st.h>
# include <tr.h>
# include <sp.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	modebug.c	- file of debug only routines for MO
**
** Description:
**	This file defines:
**
**	MO_showclasses		-- TRdisplay all the classes.
**	MO_dumpinstance		-- TRdisplay an instance.
**	MO_showinstances	-- TRdisplay all the instances.
**	MO_dumpmem		-- dump memory using TRdisplay
**	MO_tr_set		-- set the trace file, for debugging.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p in MO_showclasses(void), MO_dumpinstance(), 
**	    MO_dumpmem() and MO_showclass().
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/

/* # define xDEBUG */


/*{
** Name:	MO_showclasses	-- TRdisplay all the classes.
**
** Description:
**	Prints the whole classes tree.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	none.	
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p.
*/

VOID
MO_showclasses(void)
{
    SPBLK *sp;
    MO_CLASS *cp;

    for( sp = SPfhead( MO_classes ); sp != NULL; sp = SPfnext( sp ) )
    {
	cp = (MO_CLASS *)sp;
	TRdisplay("cp %p, node.key %s\n", cp, cp->node.key );
    }
}

/*{
** Name:	MO_dumpinstance	-- TRdisplay an instance.
**
** Description:
**	Prints an instance.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	ip	the instance to dump
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x's to %p's.
*/

VOID
MO_dumpinstance( MO_INSTANCE *ip )
{
    TRdisplay("ip %p cp %p   %s:%s   idata %p\n",
	     ip,
	     ip->classdef,
	     ip->classdef->node.key,
	     ip->instance ? ip->instance : "<nil>" ,
	     ip->idata );
}

/*{
** Name:	MO_showinstances	-- TRdisplay all the instances.
**
** Description:
**	Prints all the instances.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

VOID
MO_showinstances(void)
{
    SPBLK *sp;

    MO_once();
    for( sp = SPfhead( MO_instances ); sp != NULL; sp = SPfnext( sp ) )
	MO_dumpinstance( (MO_INSTANCE *)sp );

    SIflush( stdout );
}

/*{
** Name:	MO_dumpmem	-- dump memory using TRdisplay
**
** Description:
**	Dumps memory in hex and char format using TRdisplay.  For
**	calling inside a debugger.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	mem	pointer to first byte to dump.
**	len	number of bytes to dump.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x's to %p's.
*/

VOID
MO_dumpmem( char *mem, i4  len )
{
    i4  i, j;
    i4  c;
    char hbuf[ 12 ];

    /* print a line */
    TRdisplay("dumping %d bytes at %p\n", len, mem );
    for( i = 0; i < len; i+= 16 )
    {
	TRdisplay("%p ", mem + i );
	for( j = 0; j < 16 && (i+j) < len; j++ )
	{
	    c =  mem[ i + j ];
	    STprintf(hbuf, "%02p ", c );
	    TRdisplay("%s ", hbuf );
	}
	TRdisplay("\n         ");
	for( j = 0; j < 16 && (i+j) < len; j++ )
	{
	    c =  mem[ i + j ];
	    STprintf(hbuf, "%c  ", CMprint(&c) ? c : ' ' );
	    TRdisplay("%s ", hbuf );
	}
	TRdisplay("\n");
    }
}

/*{
** Name:	MO_tr_set	-- set the trace file, for debugging.
**
** Description:
**	Set the trace file output to the input string.  For calling
**	inside a debugger.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	dev	the place to open.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

void
MO_tr_set( char *dev )
{
    CL_SYS_ERR sys_err;

    if( *dev )
	TRset_file(TR_T_OPEN, dev, STlength(dev), &sys_err);
}



/*{
** Name: MO_showclass	- SIprintf info about class.
**
** Description:
**	SIprintf some stuff for a class, for debugging; never called.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	cp	class pointer.
**
** Outputs:
**	none.
**
** Returns:
**	OK
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x's to %p's.
*/


STATUS
MO_showclass( MO_CLASS *cp )
{
    SIprintf("class cp %p classid %s\n", cp, cp->node.key );
    SIprintf("\tcflags %x\n", cp->cflags );
    SIprintf("\tsize %d\n", cp->size );
    SIprintf("\tperms %o %d\n", cp->perms, cp->perms );
    SIprintf("\tindex %p %s\n", cp->index, cp->index ? cp->index : "null" );
    SIprintf("\toffset %d\n", cp->offset );
    SIprintf("\tget %p\n", cp->get );
    SIprintf("\tset %p\n", cp->set );
    SIprintf("\tcdata %p\n", cp->cdata );
    SIprintf("\tidx %p\n", cp->idx );
    SIprintf("\ttwin %p\n", cp->twin );
    SIprintf("\tmonitor %p\n", cp->monitor );
    return(OK);
}
