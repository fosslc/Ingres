/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <erglf.h>
#include <gl.h>
#include <er.h>
#include <gc.h>
#include <mu.h>
#include <st.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcs.h>
#include "gcsint.h"

/*
** Name: gcsutil.c
**
** Description:
**	General GCS utility functions.
**
** History:
**	15-May-97 (gordy)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 9-Jul-04 (gordy)
**	    Added gcs_default_mech() to return default mechanisms
**	    for specific GCS operations.
*/

#define	GCS_MECH_DEFAULT	ERx("default")
#define	GCS_MECH_NONE		ERx("none")

/*
** GCS globals
*/

GLOBALREF GCS_GLOBAL	*IIgcs_global;


/*
** Name: gcs_mech_id
**
** Description:
**	Returns the mechanism ID given the name of a mechanism.
**	The value "default" returns the ID of the configured
**	installation mechanism.  GCS_NO_MECH is returned if
**	name is "none" or if name cannot be resolved.
**
** Input:
**	name		Mechanism name, "none" or "default".
**
** Output:
**	None.
**
** Returns:
**	GCS_MECH	Mechanism ID, GCS_NO_MECH if not found.
**
** History:
**	15-May-98 (gordy)
**	    Created.
**	 9-Jul-04 (gordy)
**	    Support 'none' as alias for GCS_NO_MECH.
*/

GCS_MECH
gcs_mech_id( char *name )
{
    GCS_MECH	mech_id = GCS_NO_MECH;
    i4		i;

    if ( IIgcs_global  &&  IIgcs_global->initialized  &&
         STbcompare( name, 0, GCS_MECH_NONE, 0, TRUE ) )
	if ( ! STbcompare( name, 0, GCS_MECH_DEFAULT, 0, TRUE ) )
	    mech_id = IIgcs_global->install_mech;
	else
	{
	    for( i = 0; i < IIgcs_global->mech_cnt; i++ )
		if ( ! STbcompare( IIgcs_global->mechanisms[i]->mech_name, 0,
				   name, 0, TRUE ) )
		{
		    mech_id = IIgcs_global->mechanisms[i]->mech_id;
		    break;
		}
	}

    return( mech_id );
}


/*
** Name: gcs_default_mech
**
** Description:
**	Returns the default mechanism for a specific GCS operation.
**	GCS_NO_MECH is returned if the provided operation does not
**	have an associated default mechanism or if no mechanism has
**	been configured for the operation.
**
** Input:
**	op	GCS operation.
**
** Output:
**	None.
**
** Returns:
**	GCS_MECH	Mechanism ID.
**
** History:
**	 9-Jul-04 (gordy)
**	    Created.
*/

GCS_MECH
gcs_default_mech( i4 op )
{
    GCS_MECH mech_id = GCS_NO_MECH;

    if ( IIgcs_global  &&  IIgcs_global->initialized )
	switch( op )
	{
	case GCS_OP_USR_AUTH : mech_id = IIgcs_global->user_mech;	break;
	case GCS_OP_PWD_AUTH : mech_id = IIgcs_global->password_mech;	break;
	case GCS_OP_SRV_KEY  : /* fall through */
	case GCS_OP_SRV_AUTH : mech_id = IIgcs_global->server_mech;	break;
	case GCS_OP_IP_AUTH  : mech_id = GCS_MECH_INGRES;		break;
	case GCS_OP_REM_AUTH : mech_id = IIgcs_global->remote_mech;	break;
	}

    return( mech_id );
}

