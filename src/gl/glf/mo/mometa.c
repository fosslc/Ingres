/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <er.h>
# include <lo.h>
# include <si.h>
# include <st.h>
# include <cm.h>
# include <nm.h>
# include <sp.h>
# include <tm.h>
# include <mo.h>
# include "moint.h"


/**
** Name:	mometa.c	- meta data management for MO
**
** Description:
**
**	Meta data management for MO.
**	This file defines:
**
**	MO_meta_classes[] 	- self management classes for MO classes.
**	MO_classid_index	-- index method for class management.
**	MO_class_get		-- get method for class name.
**	MO_oid_get		-- get method for oid.
**	MO_oid_set		-- set function for OID.
**
** History:
**	31-Jan-92 (daveb)
**		split from moclass.c
**	24-sep-92 (daveb)
**	    documented
**	13-Jan-1993 (daveb)
**	    Add guts of MO_oid_set, and add MO_oidmap_set.  The latter
**	    reads an SI file instance and uses it to drive MO_oid_set.
**	    Thus, clients can drive the loading of an oid map.
**      24-Mar-1993 (smc)
**          Cast parameter of SPfnext() to match prototype declaration.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	4-Aug-1993 (daveb) 58937
**	    cast PTRs before use.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - removed #define for SI_MAX_TXT_REC -
**	   symbol is defined in "sicl.h"
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**      24-sep-96 (mcgem01)
**          Global data moved to modata.c
**	30-jul-97 (teresak)
**	    Modified MO_GET_METHOD, MO_SET_METHOD, and MO_INDEX_METHOD to
**	    be FUNC_EXTERN instead of GLOBALREF. GLOBALREF has a specific 
**	    meaning to VMS and was generating compiler warnings for use in 
**	    this context.
**      21-jan-1999 (canor01)
**          Remove erglf.h. Rename "class" to "mo_class".
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

/* forward decls */

FUNC_EXTERN MO_GET_METHOD MO_classid_get;
FUNC_EXTERN MO_GET_METHOD MO_oid_get;
FUNC_EXTERN MO_GET_METHOD MO_class_get;
FUNC_EXTERN MO_SET_METHOD MO_oid_set;
FUNC_EXTERN MO_SET_METHOD MO_oidmap_set;
FUNC_EXTERN MO_INDEX_METHOD MO_classid_index;

GLOBALREF char MO_oid_map[ MAX_LOC ];
GLOBALREF SYSTIME MO_map_time;


/* data */

GLOBALREF MO_CLASS_DEF MO_meta_classes[];


/*{
** Name:	MO_classid_index	-- index method for class management.
**
** Description:
**	A standard index method for use with the class tree.  The output
**	instance pointer is to the MO_CLASS block.  Instances are the
**	actual string value.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT
**	cdata		ignored.
**	linstance	length of instance buffer.
**	instance	to lookup
**	instdata	to stuff.
**
** Outputs:
**	instance	the instance string, on getnext.
**	instdata	filled with the MO_CLASSID location.
**
** Returns:
**	OK
**	MO_BAD_MSG
**	MO_NO_INSTANCE
**	MO_NO_NEXT
**	MO_INSTANCE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_classid_index(i4 msg,
		 PTR cdata,
		 i4  linstance,
		 char *instance, 
		 PTR *instdata )
{
    STATUS stat = OK;
    MO_CLASS *cp;

    switch( msg )
    {
    case MO_GET:
	if( OK == (stat = MO_getclass( instance, &cp ) ) )
	    *instdata = (PTR)cp;
	break;

    case MO_GETNEXT:
	if( OK == ( stat = MO_nxtclass( instance, &cp ) ) )
	{
	    *instdata = (PTR)cp;
	    stat = MOstrout( MO_INSTANCE_TRUNCATED,
			     cp->node.key, linstance, instance );
	}
	break;

    default:
	stat = MO_BAD_MSG;
	break;
    }
    return( stat );
}



/*{
** Name:	MO_class_get	-- get method for class name.
**
** Description:
**	Get method for class name.  Needed because of handling twins,
**	otherwise we'd use MOstrout directly.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the MO_CLASS
**	luserbuf	out buffer length.
**	userbuf		out buffer.
**
** Outputs:
**	userbuf		written with name
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

STATUS
MO_class_get(i4 offset,
	     i4  objsize,
	     PTR object,
	     i4  luserbuf,
	     char *userbuf)
{
    MO_CLASS *cp = (MO_CLASS *)object;
    char *src;

    if( CMdigit( (char *)cp->node.key ) )
	src = cp->twin->node.key;
    else
	src = cp->node.key;

    return( MOstrout( MO_VALUE_TRUNCATED, src, luserbuf, userbuf ) );
}


/*{
** Name:	MO_oid_get	-- get method for oid.
**
** Description:
**	Get method for oid name.  Needed because of handling twins,
**	otherwise we'd use MOstrout directly.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the MO_CLASS
**	luserbuf	out buffer length.
**	userbuf		out buffer.
**
** Outputs:
**	userbuf		written with oid
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

STATUS
MO_oid_get(i4 offset,
	   i4  objsize,
	   PTR object,
	   i4  luserbuf,
	   char *userbuf)
{
    MO_CLASS *cp = (MO_CLASS *)object;
    char *src;

    if( CMdigit( (char *)cp->node.key ) )
	src = cp->node.key;
    else if ( cp->twin != NULL )
	src = cp->twin->node.key;
    else
	src = "";
    return( MOstrout( MO_VALUE_TRUNCATED, src, luserbuf, userbuf ) );
}


/*{
** Name:	MO_oid_set	-- set function for OID.
**
** Description:
**
**	This function mapp classname to OID twin.
**	First, set up the twin, then lookup all the
**	existing attached objects for the class and attach them
**	again under the oid.
**
**	Error if there is an OID attached to the classid.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	offset		ignored.
**	luserbuf	length of the user input buffer.
**	userbuf		the input user buf with the OID string.
**	objsize		ignored.
**	object		the MO_CLASSID for the class name.
**
** Outputs:
**	object		twin is written.
**
** Returns:
**	OK
**	MO_NO_WRITE.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_oid_set(i4 offset,
	   i4  luserbuf,
	   char *userbuf,
	   i4  objsize,
	   PTR object )
{
    STATUS	cl_stat = OK;
    MO_CLASS	*cp = (MO_CLASS *)object;
    MO_CLASS	*twin_cp;
    MO_INSTANCE	*ip;
    MO_INSTANCE	*next_ip;

    MO_CLASS_DEF	mo_class;

    do
    {
	/* if it has a twin already, error */
	if( cp->twin != NULL )
	{
	    cl_stat = MO_NO_WRITE;
	    break;
	}

	mo_class.flags = MO_CLASSID_VAR | cp->cflags;
	mo_class.classid = userbuf;
	mo_class.size = cp->size;
	mo_class.perms = cp->perms;
	mo_class.index = cp->index;
	mo_class.get = cp->get;
	mo_class.set = cp->set;
	mo_class.cdata = cp->cdata;
	mo_class.idx = cp->idx;
	if( OK != (cl_stat = MOclassdef( 1, &mo_class ) ) )
	    break;

	if( OK != (cl_stat = MO_getclass( userbuf, &twin_cp ) ) )
	    break;

	cp->twin = twin_cp;
	twin_cp->twin = cp;
	twin_cp->monitor = cp->monitor;

	/* now attach all objects of the class as twins */

	for( ip = (MO_INSTANCE *)SPfhead( MO_instances ); ip != NULL ; ip = next_ip )
	{
	    next_ip = (MO_INSTANCE *)SPfnext( (SPBLK *)ip );
	    if( ip->classdef == cp )
	    {
		/* create one for the twin. */
		cl_stat = MOattach( 0, twin_cp->node.key, ip->instance, ip->idata );
		if( cl_stat != OK )
		    break;

		if( next_ip->classdef != cp )
		    break;
	    }
	}
    } while( FALSE );
    return( cl_stat );
}


/*{
** Name:	MO_oidmap_set	-- set function for OID MAP.
**
** Description:
**
**	This function takes as an instance the string name of
**	something in the FILES directory that maps class names
**	to OID strings.  The file is read, and the mappings
**	are installed.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	offset		ignored.
**	luserbuf	length of the user input buffer.
**	userbuf		the input user buf with the OID string.
**	objsize		ignored.
**	object		the MO_CLASSID for the class name.
**
** Outputs:
**	object		twin is written.
**
** Returns:
**	OK
**	MO_NO_WRITE.
**
** History:
**	13-Jan-1993 (daveb)
**	    Created.
*/

STATUS
MO_oidmap_set(i4 offset,
	   i4  luserbuf,
	   char *userbuf,
	   i4  objsize,
	   PTR object )
{
    FILE	*fp;
    STATUS	cl_stat;
    STATUS	set_stat;
    LOCATION	loc;
    char	loc_buf[ MAX_LOC ];
    char	line[ SI_MAX_TXT_REC ];
    char	*words[ 2 ];
    i4		wordcount;
    SYSTIME	new_time;
    
    NMloc( FILES, FILENAME, userbuf, &loc );
    LOcopy( &loc, loc_buf, &loc );
    cl_stat = LOlast( &loc, &new_time );

    /* If it's a new file, or old one has changed, read it */

    if( OK == cl_stat && (STcompare( userbuf, MO_oid_map ) ||
			  new_time.TM_secs > MO_map_time.TM_secs ))
    {
	STcopy( userbuf, MO_oid_map );
	MO_map_time = new_time;
	    
	cl_stat = SIfopen( &loc , ERx( "r" ), SI_TXT, (i4) SI_MAX_TXT_REC, &fp );
	if( cl_stat == OK )
	{
	    while( SIgetrec( line, (i4) SI_MAX_TXT_REC, fp ) == OK ) 
	    {
		wordcount = 2;
		STgetwords( line, &wordcount, words );
		if( words[0][0] != '#' && wordcount >= 2 )
		    cl_stat = MOset( ~0, MO_META_OID_CLASS, words[0], words[1] );

	    }
	    SIclose( fp );
	}
    }
    return( cl_stat );
}


