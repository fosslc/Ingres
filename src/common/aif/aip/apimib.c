/*
** Copyright (c) 2004 Ingres Corporation
*/


#include <compat.h>
#include <gl.h>
#include <sp.h>
#include <mo.h>

#include <iicommon.h>
#include <iiapi.h>
#include <api.h>
#include <apichndl.h>

/*
** Name: apimib.c
**
** Description:
**	Defines API MIB classes.
**
** History:
**	 6-Jun-03 (gordy)
**	    Created.
**      21-Jan-2005 (wansh01)
**          INGNET 161 bug # 113737
**	    Changed api_class ch_taget to ch_target_display. 
*/


/*
** Name: api_classes
**
** Description:
**	MO definitions for API MIB objects.
**
** History:
**	 6-Jun-03 (gordy)
**	    Created.
*/

static bool		initialized = FALSE;

static MO_CLASS_DEF	api_classes[] =
{
    {
        0,
        IIAPI_MIB_TRACE_LEVEL,
       	MO_SIZEOF_MEMBER( IIAPI_STATIC, api_trace_level),
        MO_READ | MO_WRITE,
        0,
       	CL_OFFSETOF( IIAPI_STATIC, api_trace_level),
        MOintget,
        MOintset,
        (PTR) -1,
        MOcdata_index
    },
    {
	MO_CDATA_CLASSID | MO_INDEX_CLASSID,
	IIAPI_MIB_CONN,
	sizeof( PTR ),
	MO_READ,
	0,
	0,
	MOpvget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	IIAPI_MIB_CONN_GCA_ASSOC,
	MO_SIZEOF_MEMBER( IIAPI_CONNHNDL, ch_connID ),
	MO_READ,
	IIAPI_MIB_CONN,
	CL_OFFSETOF( IIAPI_CONNHNDL, ch_connID ),
	MOintget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	IIAPI_MIB_CONN_TARGET,
	MO_SIZEOF_MEMBER( IIAPI_CONNHNDL, ch_target_display ),
	MO_READ,
	IIAPI_MIB_CONN,
	CL_OFFSETOF( IIAPI_CONNHNDL, ch_target),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    },
    {
	MO_CDATA_INDEX,
	IIAPI_MIB_CONN_USERID,
	MO_SIZEOF_MEMBER( IIAPI_CONNHNDL, ch_username ),
	MO_READ,
	IIAPI_MIB_CONN,
	CL_OFFSETOF( IIAPI_CONNHNDL, ch_username ),
	MOstrpget,
	MOnoset,
	0,
	MOidata_index
    }
};


/*
** Name: api_init_mib()
**
** Description:
**      Issues MO calls to define all GCD MIB objects.
**
** Input:
**      None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Jun-03 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_init_mib( VOID )
{
    i4	i, class_cnt = sizeof(api_classes) / sizeof(api_classes[0]);

    if ( ! initialized )
    {
	initialized = TRUE;

	for( i = 0; i < class_cnt; i++ )
	    if ( api_classes[i].cdata == (PTR) -1 )
		api_classes[i].cdata = (PTR)IIapi_static;

	MOclassdef( i, api_classes );
    }

    return;
}

