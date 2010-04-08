/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <erglf.h>
#include    <gl.h>

#include    <er.h>
#include    <gc.h>
#include    <me.h>
#include    <qu.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <tm.h>
#include    <sp.h>
#include    <mo.h>

#include    <sl.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcm.h>

/*
** Name: IIAUTH.C - top loop for role authentication server API
**
** Contains:
**	IIrole_auth_server()
**	IIuser_auth_server()
**	IIuser2_auth_server()
**
** History:
**	9-mar-94 (robf)
**	    Created
**	15-mar-94 (robf)
**          Added user authentication entrypoint
**	23-apr-96 (chech02)
**          Added function prototypes for windows 3.1 port.
**	16-jul-96 (stephenb)
**	    Added IIuser2_auth_server() which grabs
**	    extra efective username info from GCA buffer and passes to
**	    callback function (fix primarily for ICE) 
**	15-sep-98 (mcgem01)
**	    Removed GCA_AUTH_SPECD which is now obsolete.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/	

static i4		auth_server(char *,i4 (fcn)(),i4,i4,i4);
static void		auth_complete();
static void		listen_complete();
static void		auth_log(char *, i4);
static i4  		getvalue();
static i4  		role_setvalue();
static i4  		user_setvalue();
static i4		auth_shutdown();
static i4  		(*user_callback)()=NULL;
static i4		(*user2_callback)()=NULL;
static i4  		(*role_callback)()=NULL;


static i4 role_success=0;
static i4 role_failure=0;
static i4 user_success=0;
static i4 user_failure=0;
static char    server_id[16];
static char    mechanism[21];

static MO_CLASS_DEF moclasses[] = {
	{
		0,
		"exp.scf.auth_server.server_id",
		sizeof(server_id),
		MO_READ,
		0, /* index */
		0, /* offset */
		MOstrget, /* get */
		MOnoset, /* set */
		(PTR)server_id, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.auth_mechanism",
		sizeof(mechanism),
		MO_READ,
		0, /* index */
		0, /* offset */
		MOstrget, /* get */
		MOnoset, /* set */
		(PTR)mechanism, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.role_authenticate",
		1,
		MO_READ|MO_WRITE,
		0, /* index */
		0, /* offset */
		MOzeroget, /* get */
		role_setvalue, /* set */
		(PTR)0, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.role_success",
		sizeof(role_success),
		MO_READ,
		0, /* index */
		0, /* offset */
		MOintget, /* get */
		MOnoset, /* set */
		(PTR)&role_success, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.role_failure",
		sizeof(role_failure),
		MO_READ,
		0, /* index */
		0, /* offset */
		MOintget, /* get */
		MOnoset, /* set */
		(PTR)&role_failure, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.shutdown",
		1,
		MO_READ|MO_SECURITY_WRITE,
		0, /* index */
		0, /* offset */
		MOzeroget, /* get */
		auth_shutdown, /* set */
		(PTR)0, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.user_authenticate",
		1,
		MO_READ|MO_WRITE,
		0, /* index */
		0, /* offset */
		MOzeroget, /* get */
		user_setvalue, /* set */
		(PTR)0, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.user_success",
		sizeof(user_success),
		MO_READ,
		0, /* index */
		0, /* offset */
		MOintget, /* get */
		MOnoset, /* set */
		(PTR)&user_success, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.user_failure",
		sizeof(user_failure),
		MO_READ,
		0, /* index */
		0, /* offset */
		MOintget, /* get */
		MOnoset, /* set */
		(PTR)&user_failure, /* global data */
		MOcdata_index, /* index method */
	},
	{
		0,
		"exp.scf.auth_server.shutdown",
		1,
		MO_READ|MO_SECURITY_WRITE,
		0, /* index */
		0, /* offset */
		MOzeroget, /* get */
		auth_shutdown, /* set */
		(PTR)0, /* global data */
		MOcdata_index, /* index method */
	},
	{0}
};

STATUS
IIrole_auth_server( 
	char *auth_mechanism,
	i4 (auth_func)(),
	i4 auth_sessions,
	i4 auth_flags)
{

	return auth_server(auth_mechanism,
			auth_func,
			auth_sessions,
			auth_flags,
			1);
}

STATUS
IIuser_auth_server( 
	char *auth_mechanism,
	i4 (auth_func)(),
	i4 auth_sessions,
	i4 auth_flags)
{

	return auth_server(auth_mechanism,
			auth_func,
			auth_sessions,
			auth_flags,
			0);
}

STATUS
IIuser2_auth_server( 
	char *auth_mechanism,
	i4 (auth_func)(),
	i4 auth_sessions,
	i4 auth_flags)
{

	return auth_server(auth_mechanism,
			auth_func,
			auth_sessions,
			auth_flags,
			2);
}

/*{
** Name: auth_server() - main loop for user/role authentication servers
**
** Description:
**	Does GCA housekeeping: GCA_INITIATE, GCA_REGISTER.
**	Posts GCA_LISTEN for incoming requests
**
**
** Inputs:
**	auth_mechanism - authentication mechanism type
**
**	auth_function  - user function to perform authentication
**	
**	auth_sessions  - number of allowed session
**
**	auth_flags     - zero currently
**
**	role_auth      - 1 if role auth server, 0 if user auth server
**
** Outputs:
**	none
**
** Called by:
**	main()
**
** History:
**	9-mar-94 (robf)
**	    Created
**	15-mar-94 (robf)
**          Split out from external interface, now handles user or role
**	    authentication. Note that a single server does one or the other,
**	    not both since we want to emphasize role and user authentication
**	    is different.
**	16-jul-96 (stephenb)
**	    Modified to handle IIuser2_auth_server
*/	
static i4
auth_server( 
	char *auth_mechanism,
	i4 (auth_func)(),
	i4 auth_sessions,
	i4 auth_flags,
	i4 role_auth)
{
	GCA_PARMLIST	parms;
	STATUS		status;
	CL_ERR_DESC	sys_err;
	i4	    	assoc_no;
	char		*authmech[1];
	STATUS		error;
	STATUS		local_stat;
	i4 		assoc_id;

	if (auth_mechanism==NULL ||
	    auth_func==NULL ||
	    auth_sessions!=1 ||
	    auth_flags!=0)
	{
		if(role_auth == 1)
			auth_log("Invalid parameters passed to IIrole_auth_server",0);
		else if(role_auth == 2)
			auth_log("Invalid parameters passed to IIrole2_auth_server",0);
		else
			auth_log("Invalid parameters passed to IIuser_auth_server",0);
		return 1;
	}
	MEadvise(ME_USER_ALLOC);
	if(role_auth == 1)
	{
		user_callback=NULL;
		user2_callback=NULL;
		role_callback=auth_func;
	}
	else if(role_auth == 2)
	{
		user_callback=NULL;
		user2_callback=auth_func;
		role_callback=NULL;
	}
	else
	{
		user_callback=auth_func;
		user2_callback=NULL;
		role_callback=NULL;
	}
	status=MOclassdef(MAXI2, moclasses);
	if(status!=OK)
	{
		auth_log("MOclassdef", status);
		return ;
	}
	/* initiate */

        MEfill( sizeof(parms), 0, (PTR) &parms);
	parms.gca_in_parms.gca_normal_completion_exit = auth_complete;
	parms.gca_in_parms.gca_expedited_completion_exit = auth_complete;
	parms.gca_in_parms.gca_modifiers = GCA_API_VERSION_SPECD ;
	parms.gca_in_parms.gca_api_version = GCA_API_LEVEL_4;
	parms.gca_in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_61;
	parms.gca_in_parms.gca_auth_user = NULL;

	(void)IIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 	
				(PTR)0, -1, &status);
			
	if( status != OK || ( status = parms.gca_in_parms.gca_status ) != OK )
	{
		auth_log("GCA_INITIATE failed", status);
		return FAIL;
	}

	/* GCA_REGISTER call. */

	MEfill( sizeof(parms), 0, (PTR) &parms);
	parms.gca_rg_parms.gca_l_so_vector = 1;
	authmech[0]=auth_mechanism;
	parms.gca_rg_parms.gca_no_connected = 1;
	parms.gca_rg_parms.gca_no_active = 1;
	parms.gca_rg_parms.gca_process_id = 0;
	parms.gca_rg_parms.gca_installn_id = 0;
	parms.gca_rg_parms.gca_served_object_vector = authmech;
	parms.gca_rg_parms.gca_modifiers = (GCA_RG_INSTALL);
	parms.gca_rg_parms.gca_srvr_class = "AUTHSVR";
	    
	(void)IIGCa_call( GCA_REGISTER, &parms, GCA_SYNC_FLAG, 
				(PTR)0, -1, &status );

	if( status != OK || ( status = parms.gca_rg_parms.gca_status ) != OK )
	{
		auth_log("GCA_REGISTER failed", status);
		return status;
	}
	fprintf(stderr,"Listen address %s\n",parms.gca_rg_parms.gca_listen_id);
	STlcopy(parms.gca_rg_parms.gca_listen_id, server_id, 
			sizeof(server_id)-1);
	STlcopy(auth_mechanism, mechanism, 
			sizeof(mechanism)-1);

	/* GCA_LISTEN until shutdown */
	for(;;)
	{
		status=IIGCa_call(GCA_LISTEN, &parms,
				GCA_SYNC_FLAG,
				(PTR)listen_complete,
				-1,
				&error);
		if(status)
		{
			auth_log("GCA_LISTEN failed", error);
			break;
		}
		assoc_id=parms.gca_ls_parms.gca_assoc_id;
		if(parms.gca_ls_parms.gca_assoc_id)
		{
			status=IIGCa_call(GCA_DISASSOC, &parms,
				GCA_SYNC_FLAG,
				(PTR)0,
				-1,
				&error);
			if(status)
			{
				auth_log("GCA_DISASSOC failed",error);
				break;
			}
		}
	}
	/*
	** Reset GCA
	*/
	local_stat=IIGCa_call(GCA_TERMINATE, &parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
	if(local_stat)
	{
		auth_log("GCA_TERMINATE", error);
		if(local_stat>status)
			status=local_stat;
	}
	return status;
}			

static VOID
auth_log(char *mesg, i4  status)
{
	if(status)
		fprintf(stderr,"IIAUTH: %s: %s: error %d (x%x)\n",
				mechanism, mesg, status, status);
	else
		fprintf(stderr,"IIAUTH: %s: %s\n",
				mechanism, mesg);
}

static void auth_complete(i4 id)
{
	auth_log("auth_complete called: %d\n", id);
}

static void listen_complete(i4 id)
{

	auth_log("listen_complete called: %d \n", id);
}

/*
** Name: role_setvalue
**
** Description:
**	This routine is called by the GCM fastselect when setting the
**	MO role authentication resource. This checks its inputs
**	then calls the user callback to do the authentication
**
**	See the MO spec for information in parameters
**
** Returns:
**	OK - value set successfully
**
**	FAIL - value not set
**
** History:
**	10-mar-94 (robf)
**	    Created
**	15-mar-94 (robf)
**          Added  input check.
**	16-jul-96 (stephenb)
**	    Grab extra efective username info from GCA (not used here
*/
static i4
role_setvalue(
	int offset,
	int luserbuf,
	PTR userbuf,
	int objsize,
	PTR object)
{
	STATUS status=FAIL;
	char   user_name[33];
	char   e_user_name[33];
	char   role_name[33];
	char   role_password[25];
	char   *p;

	if(*userbuf!='0')
	{
	    /*
	    ** Check inputs
	    */
	    auth_log("Invalid/bogus value passed for role authentication",0);
	    return FAIL;
	}
	/*
	** Build information to do validation
	*/
	p=userbuf+4;
	MEcopy(p, 32, user_name);
	user_name[32]='\0';
	p+=32;
	MEcopy(p, 32, e_user_name);
	e_user_name[32]='\0';
	p+=32;
	MEcopy(p, 32, role_name);
	role_name[32]='\0';
	p+=32;
	MEcopy(p, 24, role_password);
	role_password[24]='\0';
	/*
	** Call user callback if set
	*/
	if(role_callback)
	{
		status=(*role_callback)(user_name,
				role_name,
				role_password
				);
		if(!status)
			status=FAIL;
		else
			status=OK;
	}
	else
		status=FAIL;
	if(status==OK)
	{
	    role_success++;
	    return OK;
	}
	else
	{
	    role_failure++;
	    return FAIL;
	}
}

/*
** Name: auth_shutdown
**
** Description:
**	Shutdown authentication server, called via IMA
**
** History
**	10-mar-94 (robf)
**         Created
**
*/
static i4
auth_shutdown()
{
	fprintf(stderr,"Authentication server shutting down\n");
	PCexit(0);
}
/*
** Name: user_setvalue
**
** Description:
**	This routine is called by the GCM fastselect when setting the
**	MO user authentication resource. This checks its inputs
**	then calls the user callback to do the authentication
**
**	See the MO spec for information in parameters
**
** Returns:
**	OK - value set successfully
**
**	FAIL - value not set
**
** History:
**	15-mar-94 (robf)
**	    Created
**	16-jul-96 (stephenb)
**	    Grab extra efective username info from GCA buffer and pass to
**	    alternative callback function for IIuser2_auth_server (fix 
**	    primarily for ICE but usefeul user info anyway).
*/
static i4
user_setvalue(
	int offset,
	int luserbuf,
	PTR userbuf,
	int objsize,
	PTR object)
{
	STATUS status=FAIL;
	char   user_name[33];
	char   e_user_name[33];
	char   password[25];
	char   *e_userbuf;
	char   *p;

	if(*userbuf!='1')
	{
	    /*
	    ** Check inputs
	    */
	    auth_log("Invalid/bogus value passed for user authentication",0);
	    return FAIL;
	}
	/*
	** Build information to do validation
	*/
	p=userbuf+4;
	MEcopy(p, 32, user_name);
	user_name[32]='\0';
	p+=32;
	MEcopy(p, 32, e_user_name);
	e_user_name[32]='\0';
	p+=32;
	MEcopy(p, 24, password);
	password[24]='\0';

	/*
	** Call user callback if set
	*/
	if(user_callback)
	{
		status=(*user_callback)(user_name, password);
		if(!status)
			status=FAIL;
		else
			status=OK;
	}
	else if(user2_callback)
	{
		if (e_user_name[0] == '\0' ||
		    STcompare(user_name, e_user_name) == 0)
			e_userbuf = NULL;
		else
			e_userbuf = e_user_name;
		status=(*user2_callback)(user_name, e_userbuf, password);
		if(!status)
			status=FAIL;
		else
			status=OK;
	}
	else
		status=FAIL;

	if(status==OK)
	{
	    user_success++;
	    return OK;
	}
	else
	{
	    user_failure++;
	    return FAIL;
	}
}
