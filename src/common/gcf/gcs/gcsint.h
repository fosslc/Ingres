/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

/*
** Name: gcsint.h
**
** Description:
**	GCS internal data object definitions.
**
** History:
**	19-May-97 (gordy)
**	    Created.
**	 5-Aug-97 (gordy)
**	    Added support for dynamic loading of mechanisms.
**	 5-Sep-97 (gordy)
**	    Added ID to GCS objects.
**	20-Jan-98 (gordy)
**	    Added configuration lookup function to global structure.
**	25-Feb-98 (gordy)
**	    Added GCS_E_DATA object for encrypted data.
**	 5-May-98 (gordy)
**	    Added versioning to GCS objects.
**	26-Aug-98 (gordy)
**	    Added GCS object ID GCS_DELEGATE for authentication delegation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-jan-2002 (loera01) SIR 106858
**          Added config_host to GCS global control block.
**	 5-May-03 (gordy)
**	    Added user ID lookup function to GCS global control block.
**	23-Jun-04 (gordy)
**	    Added additional version level for GCS objects.
**	 9-Jul-04 (gordy)
**	    Added individual authentication mechanisms to globals.
**	21-Jul-09 (gordy)
**	    Added GCS_NAME_MAX for lengths of standard names where
**	    appropriate.  Dynamically allocate global strings to
**	    remove length restrictions.
*/


#ifndef _GCSINT_H_INCLUDED_
#define _GCSINT_H_INCLUDED_

#include <tr.h>
#include <gcxdebug.h>

#define	GCS_MAX_DYNAMIC		10		/* Max dynamically load mechs */
#define	GCS_NAME_MAX		32		/* Max simple name length */
#define GCS_DYNAMIC_ENTRY	"gcs_mech_call"	/* Dynamic entry function */

/*
** GCS Data Object Header.
*/
typedef struct _GCS_OBJ_HDR
{

    u_i1	gcs_id[4];

#define GCS_OBJ_ID	0x4743534F	/* 'GCSO' (ascii big-endian) */

    GCS_MECH	mech_id;
    u_i1	obj_id;

#define	GCS_USR_AUTH		1
#define	GCS_PWD_AUTH		2
#define GCS_SRV_KEY		3
#define GCS_SRV_AUTH		4
#define	GCS_REM_AUTH		5
#define GCS_E_INIT		6
#define	GCS_E_CONFIRM		7
#define	GCS_E_DATA		8
#define GCS_DELEGATE		9

    u_i1	obj_ver;	/* Mechanism object version */

#define	GCS_OBJ_V0		0
#define	GCS_OBJ_V1		1
#define	GCS_OBJ_V2		2

    u_i1	obj_info;	/* For use by mechanism */
    u_i1	obj_len[2];	/* Length of following data */

} GCS_OBJ_HDR;

/*
** Macros for handling multi-byte integers in
** GCS_OBJ_HDR for heterogenous environments.
** Big-endian representation chosen arbitrarily.
**
** u_i4 GCS_PUT_I4( u_i1 *obj, u_i4 val )
** u_i4 GCS_GET_I4( u_i1 *obj )
** u_i2 GCS_PUT_I2( u_i1 *obj, u_i2 val )
** u_i2 GCS_GET_I2( u_i1 *obj )
*/

#define GCS_PUT_I4( obj, val ) \
	(*(obj) = ((val) >> 24) & 0xff, \
	 *((obj) + 1) = ((val) >> 16) & 0xff, \
	 *((obj) + 2) = ((val) >> 8) & 0xff, \
	 *((obj) + 3) = (val) & 0xff)

#define	GCS_GET_I4( obj ) \
	((*(obj) << 24) | (*((obj) + 1) << 16) | \
	 (*((obj) + 2) << 8) | *((obj) + 3))

#define	GCS_PUT_I2( obj, val )	\
	(*(obj) = ((val) >> 8) & 0xff, \
	 *((obj) + 1) = (val) & 0xff)

#define	GCS_GET_I2( obj ) \
	((*(obj) << 8) | *((obj) + 1))


/*
** Internal mechanism info.
*/

typedef STATUS (*MECH_FUNC)( i4, PTR );

typedef struct _MECH_INFO
{

    GCS_MECH_INFO	*info;		/* Mechanism information */
    MECH_FUNC		func;		/* Entry point */
    PTR			hndl;		/* Handle for dynamic libraries */

} MECH_INFO;


/*
** Name: GCS_GLOBAL
**
** Description:
**	GCS global control block.
**
**	Note: to support dynamic loading of mechanisms and
**	backward compatibility, new members may only be added
**	to the end of the structure and the structure version
**	must be bumped for each group of extensions.
**
** History:
**	19-May-97 (gordy)
**	    Created.
**	 5-Aug-97 (gordy)
**	    Added support for dynamic mechanisms.  Added tracing,
**	    memory management, and logging functions.
**	 4-Dec-97 (rajus01)
**	    Added ticket validation function.
**	20-Jan-98 (gordy)
**	    Added configuration lookup function.
**	 5-May-03 (gordy)
**	    Added user ID lookup function.  The initial user ID is still
**	    available as a member.
**	 9-Jul-04 (gordy)
**	    Added individual authentication mechanisms.  The default
**	    mechanism is now obsolete.
**	21-Jul-09 (gordy)
**	    Dynamically allocate host and username strings.
*/

typedef PTR	(*MEM_ALLOC_FUNC)( u_i4 );
typedef VOID	(*MEM_FREE_FUNC)( PTR );
typedef VOID	(*ERR_LOG_FUNC)( STATUS, i4, PTR );
typedef VOID	(*MSG_LOG_FUNC)( char * );
typedef STATUS	(*GET_KEY_FUNC)( GCS_GET_KEY_PARM * );
typedef STATUS	(*USR_PWD_FUNC)( GCS_USR_PWD_PARM * );
typedef STATUS	(*IP_FUNC)( GCS_IP_PARM * );
typedef VOID    (*USR_FUNC)( char *, i4 );

typedef VOID	(*TR_TRACE_FUNC)();
typedef char *	(*TR_LOOK_FUNC)( GCXLIST *, i4  );
typedef STATUS	(*CONFIG_FUNC)( char *, char ** );

typedef struct _GCS_GLOBAL
{
    i4			version;			/* Structure version */

#define	GCS_CB_VERSION_1	1	/* Initial version */
#define	GCS_CB_VERSION_2	2	/* Added usr_func member */
#define	GCS_CB_VERSION_3	3	/* Added individual auth mechanisms */

    u_i4		initialized;			/* Init count */

    i4			gcs_trace_level;		/* Tracing level */
    TR_TRACE_FUNC	tr_func;			/* Trace output */
    TR_LOOK_FUNC	tr_lookup;			/* Symbol lookup */
    GCXLIST		*tr_ops;			/* OP code symbols */
    GCXLIST		*tr_objs;			/* Object symbols */
    GCXLIST		*tr_parms;			/* Set parm symbols */
    CONFIG_FUNC		config_func;			/* Config lookup */

    i4			mech_cnt;			/* # of mechanisms */
    GCS_MECH_INFO	*mechanisms[ 256 ];		/* Known mechanisms */
    MECH_INFO		mech_array[ 256 ];		/* Info for mech ID */
    GCS_MECH		install_mech;			/* Configured mech */
    GCS_MECH		default_mech;			/* obsolete! */

    char		*host;				/* Current host name */
    char		*config_host;			/* PM host name */
    char		*user;				/* Current user name */

    MEM_ALLOC_FUNC	mem_alloc_func;	/* Memory allocation function */
    MEM_FREE_FUNC	mem_free_func;	/* Memory free function */
    ERR_LOG_FUNC	err_log_func;	/* Error logging, may be NULL */
    MSG_LOG_FUNC	msg_log_func;	/* Message logging, may be NULL */
    GET_KEY_FUNC	get_key_func;	/* Server key lookup, may be NULL */
    USR_PWD_FUNC	usr_pwd_func;	/* Password validation, may be NULL */
    IP_FUNC		ip_func;	/* Ticket validation, may be NULL */
    USR_FUNC		usr_func;	/* User ID */

    GCS_MECH		user_mech;	/* User authentication mechanism */
    bool		restrict_usr;	/* Restrict user authentication */
    GCS_MECH		password_mech;	/* Password authentication mechnism */
    bool		restrict_pwd;	/* Restrict password authentication */
    GCS_MECH		server_mech;	/* Server authentication mechnism */
    bool		restrict_srv;	/* Restrict server authentication */
    GCS_MECH		remote_mech;	/* Remote authentication mechnism */
    bool		restrict_rem;	/* Restrict remote authentication */

} GCS_GLOBAL;


/*
** Tracing.
**
**	Level 1: Errors.
**	Level 2: General.
**	Level 3: Mechanism.
**	Level 4: Details.
*/

#define	GCS_TRACE( level ) \
	if ( IIgcs_global  &&  IIgcs_global->gcs_trace_level >= (level) ) \
	    (*IIgcs_global->tr_func)

#define GCS_TR_MECH( id ) \
	( IIgcs_global \
	  ? ( IIgcs_global->mech_array[ (GCS_MECH)(id) ].info \
		? IIgcs_global->mech_array[ (GCS_MECH)(id) ].info->mech_name \
		: "<unknown>" ) \
	  : "(uninitialized)" )


/*
** Static GCS mechanism entry points.
*/

FUNC_EXTERN STATUS	gcs_null( i4, PTR );
FUNC_EXTERN STATUS	gcs_system( i4, PTR );
FUNC_EXTERN STATUS	gcs_ingres( i4, PTR );

#endif /* _GCSINT_H_INCLUDED_ */

