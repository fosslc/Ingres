/*
**
** Copyright (c) 1999, 2005 Ingres Corporation
**
*/
# if defined(NT_GENERIC)
# include <windows.h>
# include <MsiQuery.h>
# endif /* NT_GENERIC */
# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <me.h>
# include <nm.h>
# include <er.h>
# include <qu.h>
# include <gc.h>
# include <gcccl.h>
# include <gca.h>
# include <gcn.h>
# include <gcm.h>
# include <gcu.h>
# include <gcnint.h>
# include <pc.h>
# include <pm.h>
# include <st.h>
# include <nm.h>
# include <util.h>
# include <cm.h>
# include "tngapi.h"
#if defined(UNIX)
# include <iplist.h>
# include <ip.h>
# include <cv.h>
# include <unistd.h>
# include <ipcl.h>
#endif  /* UNIX */

#define KEY1200      0
#define KEY1201      1
#define KEY209712    2
#define KEY209808    3
#define KEY209905    4
#define KEY200001    5
#define KEY250002    6
#define KEY250006    7
#define KEY250011    8
#define KEY260106    9
#define KEY260201   10
#define KEY260207   11
#define KEY260305   12
#define KEY300404   13
#define KEY301      14
#define KEY302      15
#define KEY303      16

#define MAXSTRING    255
#define MAX_REL      17
#define MAX_REL_SIZE 10

#if defined(UNIX)
#define _MAX_PATH    260
#define BUFFER_MAX  520
static void find_needed_packages(PKGBLK *); 
GLOBALDEF char *relID;
GLOBALDEF char *productName;
GLOBALDEF LIST distPkgList;
GLOBALDEF char releaseManifest[ _MAX_PATH ];
/* The following variables are defined for resolving the undefined */
/* symbols due to the inclusion of libinstall.a on Solaris */
GLOBALDEF i4 ver_err_cnt;
GLOBALDEF bool ignoreChecksum;
GLOBALDEF bool mkresponse = FALSE;
GLOBALDEF bool exresponse = FALSE;
GLOBALDEF LIST instPkgList;
GLOBALDEF bool pmode;
GLOBALDEF char *respfilename = NULL;
GLOBALDEF bool eXpress;
GLOBALDEF bool batchMode;
GLOBALDEF char installDir[ _MAX_PATH ];
#endif   /* UNIX */


/**
**
** Name:    tngapi.c
**
** Description:
**
** The routines in this file constitute an API being developed for the
** TNG development team and tells them things like whether the Ingres
** installation has been started, the value of configuration paramters
** etc.
**
** The following funtions are currently part of the TNG API:
**(E
**  II_PingServers          - determine the registered running server
**                            classes
**  II_GetResource          - get the value of a specified configuration
**                            param.
**  II_PingGCN              - determine if the Name Server is running.
**  II_TNG_Version          - is this a TNG version of Ingres?
**  II_ServiceStarted       - Is Ingres Started as a service? NT only.
**  II_StartService         - Start the Ingres Service.
**  II_StartServiceSync     - Start the Ingres Service and wait for it
**                            to start.
**  II_StopService          - Stop the Ingres Service.
**  II_StopServiceSync      - Issue a stop service command, await completion
**  II_GetEnvVariable       - Get the value of an Ingres environment
**                            variable.
**  II_IngresVersion        - Report the version of Ingres.
**  II_IngresVersionEx      - report the version of Ingres.
**  II_IngresServiceName    - Get the internal Ingres service name.
**  II_GetIngresInstallSize - Report the size needed for an Ingres
**                            install. This is +-2MB accurate.
**  II_GetIngresInstallSizeEx - Report the expected Ingres install size.
**)E
**
** History:
**    04-may-1999 (mcgem01)
**       Created.
**    02-aug-2000 (mcgem01)
**       Make consistent with Keiths CI changes which replace tng with
**       embed in the configuration parameter name.
**    02-aug-2000 (mcgem01)
**       Add a function which reports if Ingres has been started as
**       a service.
**    17-aug-2000 (mcgem01)
**       Add functions to start and stop the Ingres service and to get
**       the value of an Ingres environment variable.
**    21-sep-2000 (mcgem01)
**        More nat to i4 changes.
**    08-aug-2001 (rodjo04) Bug 105063
**        Added missing function II_IngresVersion(). This will have to be
**        updated as new versions are created.
**    25-oct-2001 (somsa01)
**        Changed "int_wnt" to "NT_GENERIC".
**    27-dec-2001 (somsa01)
**        In II_TNG_Version(), search for
**        "ii.<hostname>.setup.embed_installation" rather than
**        "ii.$.setup.embed_installation".
**    27-dec-2001 (somsa01)
**        Added II_IngresServiceName().
**    11-jan-2002 (somsa01)
**        Added II_StartServiceSync().
**    14-jan-2002 (somsa01)
**        The modifications to II_IngresVersion() have been migrated to
**        a new function, II_IngresVersionEx().
**    03-apr-2002 (somsa01)
**        In II_GetResource(), corrected setting of status variable.
**    03-apr-2002 (somsa01)
**        Modified II_GetIngresInstallSize() to take into account the
**        disk's allocation unit.
**    03-apr-2002 (somsa01)
**        In II_GetResource(), corrected copying of the value into the
**        return buffer. Also, corrected case statements for
**        II_IngresVersion() and II_IngresVersionEx().
**    23-sep-2002 (somsa01)
**        Added 2.6/0207 as a valid version.
**    03-Oct-2002 (fanra01)
**        Bug 108862
**        Moved the initial loading of parameters from config.dat after
**        the checks for existance of II_SYSTEM environment and the
**        existance of the config.dat file.
**    25-Apr-2003 (fanra01)
**        Bug 110152
**        Add call to reset GCA interface to allow multiple requests through
**        the II_PingGCN function.
**        Add II_PingServers to determine the registered servers created
**        based on ibr_load_server in iibrowse.
**    20-Aug-2003 (somsa01)
**        Added 2.6/0305 as a valid version.
**    12-Feb-2004 (hweho01)
**        Added supports for Unix platforms.
**    25-Mar-2004 (hweho01)
**        Modified GetPrivateProfileString() and II_StartServiceSync()
**        for UNIX platforms.
**    03-dev-2004 (penga03)
**        Corrected II_GetIngresInstallSize(). And added a new API
**        II_GetIngresInstallSizeEx(), which will get the required size
**        dynamically by checking IngresII.msi.
**    07-dec-2004 (penga03)
**        Add counting MDB size in II_GetIngresInstallSize() and
**        II_GetIngresInstallSizeEx().
**    07-dec-2004 (penga03)
**        Corrected the error while counting .NET Data Provider.
**    07-dec-2004 (penga03)
**        Corrected the error introduced by last change.
**    07-Feb-2005 (fanra01)
**        Sir 113881
**        Merge Windows and UNIX sources.
**    14-Feb-2005 (fanra01)
**        Sir 113888
**        Add a synchronous stop service function.
**    05-Aug-2005 (hweho01)
**        Star 14263892.
**        Modify the return code handling in II_PingGCN(), not to   
**        interpret the returned status GC0006_DUP_INIT from  
**        IIGCa_call(GCA_INITIATE, ...) as a fatal condition.  
**    24-Jan-2006 (drivi01)
**	  SIR 115615
**	  Replaced references to SystemProductName with SystemServiceName
**	  and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	  that we do not want changes to product name effecting service name.
**    12-Dec-2006 (hweho01)
**        Update the tngapi functions on Unix/Linux platforms.
**    24-Nov-2009 (frima01) Bug 122490
**        Added include of ipcl.h and updated IPCLsetEnv calls with third
**        parameter to eliminate gcc 4.3 warnings.
**/
# define ARRAY_SIZE( array )    (sizeof( array ) / sizeof( (array)[0] ))
# define MAX_NMSVR_PORT 15
# define MAX_TARGET_LEN  64

typedef struct
{

    char    *classid;
    char    value[ GCN_VAL_MAX_LEN + 1 ];

} GCM_INFO;

/*
** Name Server GCM information for registered servers.
*/
static    GCM_INFO    svr_info[] =
{
    { GCN_MIB_SERVER_CLASS, NULL }
};

static char*    iisvrclass[] =
{
    "NMSVR",
    "IUSVR",
    "INGRES",
    "COMSVR",
    "ICESVR",
    "RMCMD"
};

static STATUS
gca_fastselect( char *target, i4 *msg_type,
        i4 *msg_len, char *msg_buff, i4 buff_max )
{
    GCA_FS_PARMS    fs_parms;
    STATUS        status;
    STATUS        call_status;

    MEfill( sizeof( fs_parms ), 0, (PTR)&fs_parms );
    fs_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    fs_parms.gca_modifiers = GCA_RQ_GCM;
    fs_parms.gca_partner_name = target;
    fs_parms.gca_buffer = msg_buff;
    fs_parms.gca_b_length = buff_max;
    fs_parms.gca_message_type = *msg_type;
    fs_parms.gca_msg_length = *msg_len;

    call_status = IIGCa_call( GCA_FASTSELECT, (GCA_PARMLIST *)&fs_parms,
                  GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  fs_parms.gca_status != OK )
        status = fs_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
    {
        call_status = status;
    }
    else
    {
        *msg_type = fs_parms.gca_message_type;
        *msg_len = fs_parms.gca_msg_length;
    }

    return( call_status );
}

static STATUS
gcm_load( char *target, i4 gcm_count, GCM_INFO *gcm_info, char *beg_inst,
      i4 msg_len, char *msg_buff, IISVRINFO* svrinfo )
{
    STATUS    status;
    char    *ptr = msg_buff;
    i4        i, msg_type, len;
    i4        data_count = 0;

    /*
    ** Build GCM request.
    */
    msg_type = GCM_GETNEXT;
    ptr += gcu_put_int( ptr, 0 );        /* error_status */
    ptr += gcu_put_int( ptr, 0 );        /* error_index */
    ptr += gcu_put_int( ptr, 0 );        /* future[2] */
    ptr += gcu_put_int( ptr, 0 );
    ptr += gcu_put_int( ptr, -1 );        /* client_perms: all */
    ptr += gcu_put_int( ptr, 0 );        /* row_count: no limit */
    ptr += gcu_put_int( ptr, gcm_count );

    for( i = 0; i < gcm_count; i++ )
    {
        ptr += gcu_put_str( ptr, gcm_info[i].classid );
        ptr += gcu_put_str( ptr, beg_inst );
        ptr += gcu_put_str( ptr, "" );
        ptr += gcu_put_int( ptr, 0 );
    }

    len = ptr - msg_buff;

    /*
    ** Connect to target, send GCM request and
    ** receive GCM response.
    */
    status = gca_fastselect( target, &msg_type, &len, msg_buff, msg_len );

    if ( status == OK )
    {
        char    classid[ GCN_OBJ_MAX_LEN ];
        char    instance[ GCN_OBJ_MAX_LEN ];
        char    cur_inst[ GCN_OBJ_MAX_LEN ];
        i4    len, err_index, junk, elements;

        /*
        ** Extract GCM info.
        */
        ptr = msg_buff;
        ptr += gcu_get_int( ptr, &status );
        ptr += gcu_get_int( ptr, &err_index );
        ptr += gcu_get_int( ptr, &junk );        /* skip future[2] */
        ptr += gcu_get_int( ptr, &junk );
        ptr += gcu_get_int( ptr, &junk );        /* skip client_perms */
        ptr += gcu_get_int( ptr, &junk );        /* skip row_count */
        ptr += gcu_get_int( ptr, &elements );

        if ( status )
        {
            SIprintf( "\nGCM error: 0x%x\n", status );
            elements = max( 0, err_index );
        }

        for(;;)
        {
            for( i = 0; i < gcm_count; i++ )
            {
                if ( ! elements-- )
                {
                    /*
                    ** Completed processing of current GCM response.
                    ** Return FAIL to indicate need for additional
                    ** request beginning with last valid instance
                    ** (make sure we actually loaded something from
                    ** the current message to avoid an infinite loop).
                    */
                    if ( data_count )  status = FAIL;
                        break;
                }
                else
                {
                    /*
                    ** Extract classid, instance, and value.
                    */
                    ptr += gcu_get_int( ptr, &len );
                    MEcopy( ptr, len, classid );
                    classid[ len ] = EOS;
                    ptr += len;

                    ptr += gcu_get_int( ptr, &len );
                    MEcopy( ptr, len, instance );
                    instance[ len ] = EOS;
                    ptr += len;

                    ptr += gcu_get_int( ptr, &len );
                    MEcopy( ptr, len, gcm_info[i].value );
                    gcm_info[i].value[ len ] = EOS;
                    ptr += len;

                    ptr += gcu_get_int( ptr, &junk );    /* skip perms */
                }

                /*
                ** A mis-match in classid indicates the end of
                ** the entries we requested.
                */
                if ( STcompare( classid, gcm_info[i].classid ) )
                    break;

                /*
                ** Instance value should be same for each
                ** classid in group.  Save for first classid,
                ** check all others.
                */
                if ( ! i )
                    STcopy( instance, cur_inst );    /* Save new instance */
                else  if ( STcompare( instance, cur_inst ) )
                {
                    /*
                    ** Invalid GCM instance
                    ** SIprintf( "\nFound invalid GCM instance: '%s' != '%s'\n",
                    ** instance, cur_inst );
                    */
                    break;
                }
            }

            if ( i < gcm_count )
                break;        /* Completed processing of GCM response info */
            else
            {
                data_count++;

                for( i = 0; i < ARRAY_SIZE(iisvrclass); i++ )
                {
                    if (!STcompare( gcm_info[0].value, iisvrclass[i] ))
                    {
                        svrinfo->servers |= (1<<i);
                        break;
                    }
                }
            }
        }
    }

    return( status );
}

/*{
** Name: II_PingServers     - determine the registered running server classes
**
** Description:
**  Return a list of running servers registered with an Ingres instance name
**  server.
**
**  A message requesting a list of running servers registered with the Ingres
**  name server is sent to the name server.  The name server in the instance
**  responds with a list of registered server classes.  Well known classes
**  are compared with the returned classes and compiled into the IISVRINFO
**  structure.
**
** Inputs:
**  srvinfo         .size       initialized to the size of srvinfo
**                              structure.
**
** Outputs:
**  srvinfo         .servers    bit field containing a list of registered
**                              servers.
**
** Returns:
**  II_SUCCESSFUL           The command completed successfully.
**  E_GC0000_OK             Request accepted for execution.
**  E_GC0002_INV_PARAM      Invalid parameter.
**  E_GC0003_INV_SVC_CODE   Invalid service code.
**  E_GC0004_INV_PLIST_PTR  Invalid parameter list pointer.
**  E_GC0006_DUP_INIT       Duplicate GCA_INITIATE request.
**  E_GC0007_NO_PREV_INIT   No previous GCA_INITIATE request.
**  Other non-zero          Command failed.
**
** Example:
**  # include "tngapi.h"
**
**  IISVRINFO   srvinfo;
**  II_INT4     status;
**
**  status = II_PingServers( &srvinfo );
**  if (status == II_SUCCESSFUL)
**  {
**      printf( "Name server %sregistered\n",
**          (srvinfo.servers & II_NMSVR) ? "" : " not " );
**      printf( "Recovery server %sregistered\n",
**          (srvinfo.servers & II_USVR) ? "" : "not " );
**      printf( "DBMS server %sregistered\n",
**          (srvinfo.servers & II_INGRES) ? "" : "not " );
**      printf( "Comms server %sregistered\n",
**          (srvinfo.servers & II_COMSVR) ? "" : "not " );
**  }
**  else
**  {
**      printf( "II_PingServers failed\n");
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_PingServers( IISVRINFO* svrinfo )
{
    STATUS          status = FAIL;
    STATUS          call_status;
    GCA_IN_PARMS    in_parms;
    GCA_TE_PARMS    te_parms;
    char*   var = NULL;
    char    ingnmsvr[MAX_NMSVR_PORT] = "\0";
    char    target[MAX_TARGET_LEN] = "\0";
    char    beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    i4       max_gca_msg;
    char*    msg_buff;

    NMgtAt( "II_INSTALLATION", &var );
    if (var && *var)
    {
        STprintf( ingnmsvr, "II_GCN%s_PORT", var );
        NMgtAt( ingnmsvr, &var );
        if (var && *var)
        {
            STprintf( target, "@::/@%s", var );
        }
    }
    if (*target && svrinfo && svrinfo->size == sizeof(IISVRINFO))
    {
        MEfill(sizeof(in_parms), (u_char)0, (PTR)&in_parms);

        in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
        in_parms.gca_api_version = GCA_API_LEVEL_5;
        in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

        call_status = IIGCa_call( GCA_INITIATE, (GCA_PARMLIST *)&in_parms,
              GCA_SYNC_FLAG, NULL, -1, &status );

        if ( call_status == OK  &&  in_parms.gca_status != OK )
            status = in_parms.gca_status, call_status = FAIL;

        if ( call_status == OK )
        {
            max_gca_msg = GCA_FS_MAX_DATA + in_parms.gca_header_length;

            if ((msg_buff=(char *)MEreqmem( 0, max_gca_msg, FALSE, &status )))
            {
                do
                {
                    status = gcm_load( target, ARRAY_SIZE( svr_info ), svr_info,
                       beg_inst, max_gca_msg, msg_buff, svrinfo );
                } while( status == FAIL );

                /*
                ** Shutdown communications.
                */
                call_status = IIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms,
                      GCA_SYNC_FLAG, NULL, -1, &status );

                if ( call_status == OK  &&  te_parms.gca_status != OK )
                    status = te_parms.gca_status, call_status = FAIL;
            }
        }
    }
    return(status);
}

/*{
** Name: II_GetResource     - get the value of a specified configuration param
**
** Description:
**  Return the value of a configuration parameter.  The parameter name
**  should match the name as found in the configuration registry file
**  config.dat.
**
** Inputs:
**  ResName             Parameter name; in the same format as the name found
**                      in config.dat.
**  ResValue            Pointer to memory area where the retrieved value
**                      will be returned.  There is no maximum length
**                      specified for a parameter value.  The memory area
**                      provided must be of sufficient size to receive
**                      the retrieved value.
**
** Outputs:
**  ResValue            The value of the configuration paramter in text form,
**                      returned as a string.
**
** Returns:
**  II_SUCCESSFUL       Value retrieved successfully.
**  Other non-zero      Command failed.
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 status;
**  II_CHAR parm_name[] = { "ii.hostname.ingstart.*.dbms" };
**  II_CHAR parm_value[128] = { '\0' };
**
**  status = II_GetResource( parm_name, parm_value );
**  if (status == II_SUCCESSFUL)
**  {
**      printf( "II_GetResource %s=%s\n", parm_name, parm_value );
**  }
**  else
**  {
**      printf( "II_GetResource failed\n" );
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_GetResource(char *ResName, char *ResValue)
{
    int        status;
    char    *value;

    PMinit();

    if ((status = PMload(NULL, PMerror)) != OK)
    return status;

    if ((status = PMget(ResName, &value)) == OK)
    {
    STcopy(value, ResValue);
    return OK;
    }
    else
    {
    return status;
    }
}

/*{
** Name: II_PingGCN         - determine if the name server is running
**
** Description:
**  Tests for the existance of a running name server.
**  The address for the name server is retrieved from the environment and
**  attempts to connect.  A successful connection indicates a running
**  name server.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   Name server is not running.
**  Other non-zero      Name server is running.
**
** Example:
**  # include "tngapi.h"
**
**  II_BOOL gcn_running;
**
**  gcn_running = II_PingGCN();
**  if (gcn_running)
**  {
**      printf( "GCN running\n" ):
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
II_BOOL
II_PingGCN()
{
    GCA_PARMLIST    parms;
    STATUS          status;
    CL_ERR_DESC     cl_err;
    i4             assoc_no;
    STATUS          tmp_stat;
    char            gcn_id[ GCN_VAL_MAX_LEN ];

    MEfill( sizeof(parms), 0, (PTR) &parms );

    (void) IIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0,
            -1L, &status);

    if( ( status != OK && status != E_GC0006_DUP_INIT ) || \
        ( ( status = parms.gca_in_parms.gca_status ) != OK ) )
            return( FALSE );

    status = GCnsid( GC_FIND_NSID, gcn_id, GCN_VAL_MAX_LEN, &cl_err );

    if( status == OK )
    {
        /* make GCA_REQUEST call */
        MEfill( sizeof(parms), 0, (PTR) &parms);
        parms.gca_rq_parms.gca_partner_name = gcn_id;
        parms.gca_rq_parms.gca_modifiers = GCA_CS_BEDCHECK |
                GCA_NO_XLATE;
        (void) IIGCa_call( GCA_REQUEST, &parms, GCA_SYNC_FLAG,
                0, GCN_RESTART_TIME, &status );
        if( status == OK )
                status = parms.gca_rq_parms.gca_status;

        /* make GCA_DISASSOC call */
        assoc_no = parms.gca_rq_parms.gca_assoc_id;
        MEfill( sizeof( parms ), 0, (PTR) &parms);
        parms.gca_da_parms.gca_association_id = assoc_no;
        (void) IIGCa_call( GCA_DISASSOC, &parms, GCA_SYNC_FLAG,
                0, -1L, &tmp_stat );
        /*
        ** Call to terminate to reset interface state
        ** Allows multiple calls to test gcn
        */
        IIGCa_call(   GCA_TERMINATE,
              &parms,
              (i4) GCA_SYNC_FLAG,    /* Synchronous */
              (PTR) 0,                /* async id */
              (i4) -1,           /* no timeout */
              &status);
        /* existing name servers answer OK, new ones NO_PEER */
        if( status == E_GC0000_OK || status == E_GC0032_NO_PEER )
                return( TRUE );
    }

    /*
    ** Terminate the connection because we will initiate it again on
    ** entry to this routine.
    */

    IIGCa_call(   GCA_TERMINATE,
                  &parms,
                  (i4) GCA_SYNC_FLAG,    /* Synchronous */
                  (PTR) 0,                /* async id */
                  (i4) -1,           /* no timeout */
                  &status);

    return( FALSE );
}

/*{
** Name: II_TNG_Version     - determine if an embedded version of Ingres.
**
** Description:
**  Tests for the existance of the configuration parameter specifically
**  introduced to identify embedded Ingres instances.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   Embedded instance value not found.
**  Other non-zero      Embedded instance value found.
**
** Example:
**  # include "tngapi.h"
**
**  II_BOOL embedded_instance;
**
**  embedded_instance = II_TNG_Version();
**  if (embedded_instance)
**  {
**      printf( "Embedded instance found\n" ):
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
II_BOOL
II_TNG_Version()
{
    char    value[16];
    char    key[64];
    STATUS    status;

    STprintf(key, "ii.%s.setup.embed_installation", PMhost());
    status = II_GetResource(key, value);

    if ((status == OK) && (value[0] != '\0') &&
    (STbcompare(value, 0, "on", 0, TRUE) == 0))
    {
    return TRUE;
    }
    else
    {
    return FALSE;
    }
}

/*{
** Name: II_GetEnvVariable  - get the value of an environment variable
**
** Description:
**  Retrieve the value of a named environment variable.  If the name does not
**  exist in the system environment and the name to be retrieved is not
**  II_SYSTEM then the value is retrieved from the Ingres environment area.
**
** Inputs:
**  name            Name of the environment variable to retrieve.
**  value           Pointer to receive the requested value.
**
** Outputs:
**  value           String pointer to the requested value.
**                  NULL is returned if the environment variable does not
**                  exist.
**                  Note: The returned pointer should be treated as read-only.
**
** Returns:
**  None.
**
** Example:
**  # include "tngapi.h"
**
**  II_CHAR*    env_var = { "II_TIMEZONE_NAME" };
**  II_CHAR*    env_val = NULL;
**
**  II_GetEnvVariable( env_var, &env_val );
**  if (env_val != NULL)
**  {
**      printf( "%s=%s\n", env_var, env_val );
**  }
**  else
**  {
**      printf( "%s not found\n", env_var );
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
VOID
II_GetEnvVariable(char *name, char **value)
{
    *value = getenv(name);

        if ( ( *value == (char *)NULL ) &&
             ( STcompare( SystemLocationVariable, name ) ) )
        {
            if (NMgtIngAt(name, value))
            {
                *value = (char *)NULL;
            }
        }
}

/*{
** Name: II_ServiceStarted  - determines if the Ingres instance is started
**
** Description:
**  On Windows, determines if the Ingres instance service has been started.
**
**  On Unix, determines if the Ingres instance has started.
**  
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   Embedded instance not started.
**  Other non-zero      Embedded instance started.
**
** Example:
**  # include "tngapi.h"
**
**  II_BOOL instance_running;
**
**  instance_running = II_ServiceStarted()
**  if (instance_running)
**  {
**      printf( "Embedded instance running\n" ):
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
II_BOOL
II_ServiceStarted()
{
#if defined(NT_GENERIC)
    CHAR               ServiceName[255];
    SERVICE_STATUS     ssServiceStatus;
    LPSERVICE_STATUS   lpssServiceStatus = &ssServiceStatus;
    SC_HANDLE          schSCManager, OpIngSvcHandle;
    bool               as_service = FALSE;
    bool               ServiceStarted = FALSE;

    /*
    ** Figure out if the service is started.
    */
    if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
        != NULL)
    {
        char        *ii_install;

        NMgtAt( ERx( "II_INSTALLATION" ), &ii_install );
        if( ii_install == NULL || *ii_install == EOS )
            ii_install = ERx( "" );
        else
            ii_install = STalloc( ii_install );

        STprintf(ServiceName, "%s_Database_%s", SystemServiceName,
                 ii_install);

        if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
                    SERVICE_QUERY_STATUS)) != NULL)
        {
            if (QueryServiceStatus(OpIngSvcHandle, lpssServiceStatus))
            {
                if (ssServiceStatus.dwCurrentState == SERVICE_RUNNING)
                    ServiceStarted = TRUE;
            }
            CloseServiceHandle(OpIngSvcHandle);
        }
        CloseServiceHandle(schSCManager);
    }
    return ServiceStarted;

#else   /* NT_GENERIC */

    /*  checking if the system is running on Unix/Linux platforms */

    char        *iisystem;
    CL_ERR_DESC cl_err;
    STATUS stat;
    char buf[MAXSTRING];
    II_BOOL ServiceStarted = FALSE;

    NMgtAt( ERx( "II_SYSTEM" ), &iisystem );
    if( ( iisystem != NULL ) && ( *iisystem != EOS ) )
    {
     STpolycat(2, ERx("iigcfid "), ERx("iigcn |grep . > /dev/null "), buf);
     ServiceStarted = ( PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err ) == OK ) ?
                TRUE: FALSE ;
    }
    else
    {
     ServiceStarted = FALSE;
     SIprintf( "\nEnvironment variable II_SYSTEM is not defined. \n" );
    }

    return (ServiceStarted);
#endif  /* NT_GENERIC */
}

/*{
** Name: II_StartService    - Start the Ingres Service.
**
** Description:
**  On Windows, commands the service control manager to start the Ingres
**  instance.
**
**  On Unix, issues the command to start the Ingres instance.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   The service command has been successfully issued.
**  Other non-zero      Service command failed.
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 status;
**
**  status = II_StartService()
**  if (status != 0)
**  {
**      printf( "Service start failed - %d\n", status ):
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_StartService()
{
#if defined(NT_GENERIC)
    CHAR                    ServiceName[255];
    SERVICE_STATUS          ssServiceStatus;
    LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
    SC_HANDLE               schSCManager, OpIngSvcHandle;
    int                     OsError = 0;

    /*
    ** Figure out if the service is started.
    */
    if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
        != NULL)
    {
        char        *ii_install;

        NMgtAt( ERx( "II_INSTALLATION" ), &ii_install );
        if( ii_install == NULL || *ii_install == EOS )
            ii_install = ERx( "" );
        else
            ii_install = STalloc( ii_install );

        STprintf(ServiceName, "%s_Database_%s", SystemServiceName,
            ii_install);

        if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
            SERVICE_START)) != NULL)
        {
            if (!StartService( OpIngSvcHandle, NULL, NULL ) )
            {
                OsError = GetLastError ();
            }
            CloseServiceHandle(OpIngSvcHandle);
        }
        CloseServiceHandle(schSCManager);
    }
    return OsError;

#else   /* NT_GENERIC */

    /*  Start up Ingres on Unix platforms */

    char        *iisystem;
    CL_ERR_DESC cl_err;
    STATUS stat;
    char buf[MAXSTRING];
    STATUS rtrn;

    NMgtAt( ERx( "II_SYSTEM" ), &iisystem );
    if( ( iisystem != NULL ) && ( *iisystem != EOS ) )
    {
        MEfill ( sizeof(buf), '\0', buf );
        STpolycat( 2, ERx( "iigcfid " ),
                    ERx( "iigcn |grep . > /dev/null " ), buf );
        if ( rtrn = PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err ) != OK )
        {
            MEfill ( sizeof(buf), '\0', buf );
            STpolycat( 4, iisystem, ERx( "/ingres/utility/iistartup -s " ),
                       iisystem, ERx( " > /dev/null " ), buf );
            rtrn = PCcmdline( NULL, buf, PC_NO_WAIT, NULL, &cl_err );
        }
        else
        {
          /*
            rtrn = II_SERVER_RUNNING;
            SIprintf( "\nIngres system is already running. \n" );
          */
        }
    }
    else
    {
        SIprintf( "\n Environment variable II_SYSTEM is not defined. \n" );
        rtrn = II_INGRES_NOT_FOUND;
    }
    return (rtrn );

#endif  /* NT_GENERIC */
}

/*{
** Name: II_StartServiceSync- Start the Ingres Service and wait for it
**
** Description:
**  On Windows, commands the service control manager to start the Ingres
**  instance continues to query the service control manager until the status
**  changes to either SERVICE_STARTED or SERVICE_STOPPED.
**
**  On Unix, issues the command to start the Ingres instance.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   The service started successfull.
**  Other non-zero      Service command failed.
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 status;
**
**  status = II_StartServiceSync()
**  if (status != 0)
**  {
**      printf( "Service start failed - %d\n", status ):
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_StartServiceSync()
{
#if defined(NT_GENERIC)

    CHAR        ServiceName[255];
    SERVICE_STATUS    ssServiceStatus;
    LPSERVICE_STATUS    lpssServiceStatus = &ssServiceStatus;
    SC_HANDLE        schSCManager, OpIngSvcHandle;
    int            OsError = 0;

    /*
    ** Figure out if the service is started.
    */
    if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)) != NULL)
    {
        char    *ii_install;

        NMgtAt (ERx( "II_INSTALLATION" ), &ii_install);
        if (ii_install == NULL || *ii_install == EOS)
            ii_install = ERx("");
        else
            ii_install = STalloc(ii_install);

        STprintf(ServiceName, "%s_Database_%s", SystemServiceName, ii_install);

        if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
            SERVICE_QUERY_STATUS | SERVICE_START)) != NULL)
        {
            if (!StartService(OpIngSvcHandle, NULL, NULL))
            {
                OsError = GetLastError();
            }
            else
            {
                while ( QueryServiceStatus(OpIngSvcHandle, lpssServiceStatus) &&
                    ssServiceStatus.dwCurrentState != SERVICE_RUNNING &&
                    ssServiceStatus.dwCurrentState != SERVICE_STOPPED)
                {
                    PCsleep(1000);
                }

                if (ssServiceStatus.dwCurrentState == SERVICE_STOPPED)
                    OsError = ssServiceStatus.dwWin32ExitCode;
            }

            CloseServiceHandle(OpIngSvcHandle);
        }

        CloseServiceHandle(schSCManager);
    }

    return OsError;

#else   /* NT_GENERIC */

    /* Start up services on Unix/Linux */
    char        *iisystem;
    CL_ERR_DESC cl_err;
    STATUS stat;
    char buf[MAXSTRING];
    STATUS rtrn;

    NMgtAt( ERx( "II_SYSTEM" ), &iisystem );
    if( ( iisystem != NULL ) && ( *iisystem != EOS ) )
    {
        MEfill ( sizeof(buf), '\0', buf );
        STpolycat( 2, ERx( "iigcfid " ),
                    ERx( "iigcn |grep . > /dev/null " ), buf );
        if ( rtrn = PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err ) != OK )
        {
            MEfill ( sizeof(buf), '\0', buf );
            STpolycat( 4, iisystem, ERx( "/ingres/utility/iistartup -s " ),
                       iisystem, ERx( " > /dev/null " ), buf );
            rtrn = PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err );
        }
    }
    else
    {
        SIprintf( "\n Environment variable II_SYSTEM is not defined. \n" );
        rtrn = II_INGRES_NOT_FOUND;
    }
    return (rtrn );

#endif  /* NT_GENERIC */
}

/*{
** Name: II_StopService     - Stop the Ingres Service
**
** Description:
**  On Windows, commands the service control manager to stop the Ingres
**  instance.
**
**  On Unix, issues the command to stop the Ingres instance.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   The service command has been successfully issued.
**  Other non-zero      Service command failed.
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 status;
**
**  status = II_StopService()
**  if (status != 0)
**  {
**      printf( "Service stop failed - %d\n", status ):
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_StopService()
{
#if defined(NT_GENERIC)

    CHAR                    ServiceName[255];
    SERVICE_STATUS          ssServiceStatus;
    LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
    SC_HANDLE               schSCManager, OpIngSvcHandle;
    int                     OsError = 0;

    /*
    ** Figure out if the service is started.
    */
    if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
        != NULL)
    {
        char        *ii_install;

        NMgtAt( ERx( "II_INSTALLATION" ), &ii_install );
        if( ii_install == NULL || *ii_install == EOS )
            ii_install = ERx( "" );
        else
            ii_install = STalloc( ii_install );

        STprintf(ServiceName, "%s_Database_%s", SystemServiceName,
            ii_install);

        if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
            SERVICE_STOP)) != NULL)
        {
            if (!ControlService( OpIngSvcHandle, SERVICE_CONTROL_STOP, lpssServiceStatus ) )
            {
                OsError = GetLastError ();
            }
            CloseServiceHandle(OpIngSvcHandle);
        }
        CloseServiceHandle(schSCManager);
    }
    return OsError;

#else

   /* Stop the services on Unix/Linux */

    char        *iisystem;
    CL_ERR_DESC cl_err;
    STATUS status, rtrn;
    char buf[MAXSTRING];

    NMgtAt( ERx( "II_SYSTEM" ), &iisystem );
    if( ( iisystem != NULL ) && ( *iisystem != EOS ) )
    {
        /* Check if Ingres is running     */
        STpolycat( 2, ERx( "iigcfid " ), ERx( "iigcn |grep . > /dev/null " ), buf );
        status = PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err );

        if ( status == 0 )
        {
            /* Yes, Ingres is running, then stop it    */
            STpolycat( 2, ERx( "ingstop " ), ERx( " > /dev/null " ), buf );
            rtrn = PCcmdline( NULL, buf, PC_NO_WAIT, NULL, &cl_err );
        }
        else
        {
            SIprintf( "\nIngres DBMS system is not running. \n" );
            rtrn = 0;
        }
    }
    else
    {
        SIprintf( "\nEnvironment variable II_SYSTEM is not defined. \n" );
        rtrn = II_INGRES_NOT_FOUND;
    }

    return (rtrn );

#endif 
}

/*{
** Name: II_StopServiceSync - Issue a stop service command, await completion
**
** Description:
**  On Windows, commands the service control manager to stop the Ingres
**  instance and continues to query the service control manager until the
**  status changes to SERVICE_STOPPED.
**
**  On Unix, issues the command to stop the Ingres instance.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  0                   The service command has been successfully issued.
**  Other non-zero      Service command failed.
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 status;
**
**  status = II_StopServiceSync()
**  if (status != 0)
**  {
**      printf( "Service stop failed - %d\n", status ):
**  }
**
** History:
**      14-Feb-2005 (fanra01)
**          Created.
}*/
int
II_StopServiceSync()
{
#if defined(NT_GENERIC)

    CHAR                    ServiceName[255];
    SERVICE_STATUS          ssServiceStatus;
    LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
    SC_HANDLE               schSCManager, OpIngSvcHandle;
    int                     OsError = 0;

    /*
    ** Figure out if the service is started.
    */
    if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
        != NULL)
    {
        char        *ii_install;

        NMgtAt( ERx( "II_INSTALLATION" ), &ii_install );
        if( ii_install == NULL || *ii_install == EOS )
            ii_install = ERx( "" );
        else
            ii_install = STalloc( ii_install );

        STprintf(ServiceName, "%s_Database_%s", SystemServiceName,
            ii_install);

        if ((OpIngSvcHandle = OpenService(schSCManager, ServiceName,
            SERVICE_QUERY_STATUS | SERVICE_STOP)) != NULL)
        {
            if (!ControlService( OpIngSvcHandle, SERVICE_CONTROL_STOP, lpssServiceStatus ) )
            {
                OsError = GetLastError ();
            }
            else
            {
                while (QueryServiceStatus(OpIngSvcHandle, lpssServiceStatus))
                {
                    switch(ssServiceStatus.dwCurrentState)
                    {
                        case SERVICE_START_PENDING:
                        case SERVICE_STOP_PENDING:
                            PCsleep(1000);
                            break;
                        default:
                            break;
                    }
                    if (ssServiceStatus.dwCurrentState == SERVICE_STOPPED)
                    {
                        break;
                    }
                }

                if (ssServiceStatus.dwCurrentState == SERVICE_STOPPED)
                    OsError = ssServiceStatus.dwWin32ExitCode;
            }
            CloseServiceHandle(OpIngSvcHandle);
        }
        CloseServiceHandle(schSCManager);
    }
    return OsError;

#else   

   /* Stop the services on Unix/Linux */

    STATUS      status, rtrn ;
    char        *iisystem;
    CL_ERR_DESC cl_err;
    char check_cmd[MAXSTRING];
    char stop_cmd[MAXSTRING];
    bool stopped;

    rtrn = OK;
    NMgtAt( ERx( "II_SYSTEM" ), &iisystem );
    if( ( iisystem != NULL ) && ( *iisystem != EOS ) )
    {
        rtrn = OK;
        /* Check if Ingres is running     */
        STprintf( check_cmd, ERx( "iigcfid iigcn |grep . > /dev/null ") );
        status = PCcmdline( NULL, check_cmd, PC_WAIT, NULL, &cl_err );
        if ( status == 0  )
        {
            /* Yes, Ingres is running, need to stop it    */
            rtrn = II_UNSUCCESSFUL;
            STprintf( stop_cmd, ERx( "ingstop > /dev/null " ) );
            status = PCcmdline( NULL, stop_cmd, PC_WAIT, NULL, &cl_err );
            if( status == OK )
              {
               /* check if Ingres has stopped ?  */
               stopped = ( PCcmdline( NULL, check_cmd, PC_WAIT, NULL,
                                     &cl_err ) == OK ? FALSE: TRUE ) ;
               if(stopped == TRUE)
                 rtrn = OK;
              }
        }
    }
    else
    {
        rtrn = II_INGRES_NOT_FOUND;
    }

    return (rtrn);

#endif 
}

/*{
** Name: II_IngresVersion   - report the version of Ingres
**
** Description:
**  Scan config.dat and version.rel and extract the version number from
**  the release strings. Registry entry fields config.dbms and config.net
**  in config.dat are used to retrieve version strings.
**  All returned versions are compared with a list of known version numbers.
**
** Inputs:
**  None.
**
** Outputs:
**  None.
**
** Returns:
**  II_INGRES_NOT_FOUND     II_SYSTEM undefined.
**  II_CONFIG_NOT_FOUND     Error accessing config.dat.
**  II_CONFIG_BAD_PARAM     Unable to find version registry entries.
**  II_INGRES_1200
**  II_INGRES_1201
**  II_INGRES_209712
**  II_INGRES_209808
**  II_INGRES_209905
**  II_INGRES_200001
**  II_INGRES_250006
**  II_INGRES_250011
**  II_INGRES_260106
**  II_INGRES_260201
**  II_INGRES_260207
**  II_INGRES_260305
**  II_INGRES_300404
**  II_INGRES_301
**  II_INGRES_302
**  II_INGRES_303
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 version;
**
**  if ((version = II_IngresVersion()) > 0)
**  {
**      printf("II_IngresVersion returned %d \n", version);
**  }
**  else
**  {
**      printf("Bat return %d\n", version);
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_IngresVersion()
{
    PM_SCAN_REC    state1;
    STATUS    status;
    char    *ii_systempath;
    int        len = 0;
    char    keys[MAX_REL][MAXSTRING];
    char    versions[MAX_REL][MAX_REL_SIZE];
    char    versionRel[MAX_REL_SIZE];
    char    exeRel[MAX_REL_SIZE];
    int        latestversion;
    int        latestversionrel;
    int        latestversionexe;
    int        ret=0;
    int        i,j, totalnum = 0;
    char    *name;
    char*    className = NULL;
    char    *index;
    LOCATION    loc;
    FILE    *fptr;
    char    iidbmsFile[_MAX_PATH];
    char    mline[MAXSTRING];
#if defined(NT_GENERIC)
    DWORD    fileinfosize =0;
    LPVOID    lpData[50], lpBuffer;
#endif  /* NT_GENERIC */
    char    keyvalues[MAX_REL][MAX_REL_SIZE] = {"1200",
                            "1201",
                            "209712",
                            "209808",
                            "209905",
                            "200001",
                            "250002",
                            "250006",
                            "250011",
                            "260106",
                            "260201",
                            "260207",
                            "260305",
                            "300404",
                            "301",
                            "302",
                            "303"};

    latestversion = -1;
    latestversionrel = -1;
    latestversionexe = -1;

    /*
    ** Get II_SYSTEM from the environment.
    */
    NMgtAt(ERx( "II_SYSTEM" ), &ii_systempath);

    if (ii_systempath == NULL || *ii_systempath == EOS)
    return II_INGRES_NOT_FOUND;
    else
    ii_systempath = STalloc(ii_systempath);

    /*
    ** Test to see if the config.dat file can be opened
    */
    status = NMloc(UTILITY, FILENAME, "config.dat", &loc);
    if (status == OK)
    SIfopen(&loc, ERx("r"), SI_TXT, 0, &fptr) ;

    if (fptr == NULL)
    return II_CONFIG_NOT_FOUND;
    else
    SIclose(fptr);

    /*
    ** Moved loading of config data to after checking for system path.
    ** No II_SYSTEM or config file means its pointless to continue.
    */
    PMinit();
    status = PMload( NULL, (PM_ERR_FUNC *)NULL );
    /*
    ** A failed status here means that something went wrong with the load
    ** from config.dat.
    ** Any error from the PMload function is reported as II_CONFIG_BAD_PARAM
    ** to avoid returning a +ve integer for a failed status.
    */
    if (status != OK)
    return II_CONFIG_BAD_PARAM;

    /*
    ** First, look for DBMS entries.
    */
    for (status = PMscan("CONFIG.DBMS", &state1, NULL, &name, &className);
     status == OK;
     status = PMscan(NULL, &state1, NULL, &name, &className))
    {
    if (totalnum == 0)
        STcopy(name, keys[totalnum++]);
    else
    {
        for (i = 0; i <= totalnum; i++)
        {
        if (!STcompare(keys[i], name))
            break;
        }

        STcopy(name, keys[totalnum++]);
    }
    }

    /*
    ** Now, look for NET entries.
    */
    for (status = PMscan("CONFIG.NET", &state1, NULL, &name, &className);
     status == OK;
     status = PMscan(NULL, &state1, NULL, &name, &className))
    {
    if (totalnum == 0)
        STcopy(name, keys[totalnum++]);
    else
    {
        for (i = 0; i <= totalnum; i++)
        {
        if (!STcompare(keys[i], name))
            break;
        }

        STcopy(name, keys[totalnum++]);
    }
    }

    if (totalnum == 0)
    return II_CONFIG_BAD_PARAM;

    for (i=0; i < MAX_REL ; i++)
    for (j=0; j < MAX_REL_SIZE ; j++)
        versions[i][j] = '\0' ;

    for (j=0 ; j < totalnum ; j++)
    {
    index = STrindex(keys[j],".", 0);
    index++;

    while (!CMdigit(index))
        index++;

    i=0;

    while (!CMalpha(index))
    {
        versions[j][i] = *index;
        index++;
        i++;
    }
    }

    for (j=0; j < totalnum; j++)
    {
    for (i=0; i < MAX_REL ; i++)
    {
        if (STcompare(versions[j], keyvalues[i]) == 0)
        {
        if (latestversion <= i)
        {
            latestversion = i;
            break;
        }
        }
    }
    }

    status = NMloc(SUBDIR, FILENAME, "version.rel", &loc);
    if (status == OK)
    SIfopen(&loc, ERx("r"), SI_TXT, 0, &fptr);

    if (fptr != NULL)
    {
    SIgetrec (mline, MAXSTRING, fptr);
    index = mline;

    for (i=0; i < MAX_REL_SIZE ; i++)
        versionRel[i] = '\0' ;

           while (!CMdigit(index))
        index++;

    i = 0;
    while (!CMalpha(index))
    {
        if (*index != '/' && *index != '.' )
        {
        if (*index == ' ')
            break;
        versionRel[i] = *index;
        i++;
        }

        index++;
    }

    SIclose(fptr);

    for (i=0; i < MAX_REL ; i++)
    {
        if (STcompare(versionRel, keyvalues[i]) == 0)
        {
        if (latestversionrel <= i)
        {
            latestversionrel = i;
            break;
        }
        }
    }

    if (latestversionrel > latestversion)
        latestversion = latestversionrel;
    }    /* endif Version.rel */

#if defined(NT_GENERIC)

    if (latestversion >= KEY250011)
    {
    STprintf(iidbmsFile, "%s\\ingres\\bin\\iidbms.exe", ii_systempath);

    fileinfosize = GetFileVersionInfoSize(iidbmsFile, NULL);

    if (fileinfosize == 0 && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        STprintf(iidbmsFile, "%s\\ingres\\bin\\iigcc.exe", ii_systempath);
        fileinfosize = GetFileVersionInfoSize(iidbmsFile, NULL);
    }

    if (fileinfosize == 0)
        goto final;

    GetFileVersionInfo(iidbmsFile, NULL, fileinfosize, lpData);

    VerQueryValue(lpData,
              TEXT("\\StringFileInfo\\040904b0\\FileDescription"),
              &lpBuffer,
              &fileinfosize);

    index = lpBuffer;

    for (i=0; i < MAX_REL ; i++)
        exeRel[i] = '\0' ;

    while (!CMdigit(index))
        index++;

    i = 0;
    while (!CMalpha(index))
    {
        if (*index != '/' && *index != '.' )
        {
        if (*index == ' ')
            break;
        exeRel[i] = *index;
        i++;
        }

        index++;
    }

    for (i=0; i < MAX_REL ; i++)
    {
        if (STcompare(exeRel,keyvalues[i]) == 0)
        {
        if (latestversionexe <= i)
        {
            latestversionexe = i;
            break;
        }
        }
    }

    if (latestversionexe > latestversion)
        latestversion = latestversionexe;
    }

#endif   /* if defined(NT_GENERIC)  */

final:

    switch (latestversion)
    {
    case KEY1200:
        return II_INGRES_1200;

    case KEY1201:
        return II_INGRES_1201;

    case KEY209712:
        return II_INGRES_209712;

    case KEY209808:
        return II_INGRES_209808;

    case KEY209905:
        return II_INGRES_209905;

    case KEY200001:
        return II_INGRES_200001;

    case KEY250002:
    case KEY250006:
        return II_INGRES_250006;    /* both keys are the same version */

    case KEY250011:
        return II_INGRES_250011;

    case KEY260106:
    case KEY260201:
        return II_INGRES_260201;    /* both keys are the same version */

    case KEY260207:
        return II_INGRES_260207;

    case KEY260305:
        return II_INGRES_260305;

    case KEY300404:
        return II_INGRES_300404;

    case KEY301:
        return II_INGRES_301;

    case KEY302:
        return II_INGRES_302;

    case KEY303:
        return II_INGRES_303;

    default:
        return -1;
    }
}

/*{
** Name: II_IngresVersionEx - report the version of Ingres
**
** Description:
**  Scan config.dat and version.rel and extract the version number from
**  the release strings. Registry entry fields config.dbms and config.net
**  in config.dat are used to retrieve version strings.
**  All returned versions are compared with a list of known version numbers.
**
** Inputs:
**  VersionString           Pointer to receive the version string.
**
** Outputs:
**  VersionString           Version string terminated with and end of string
**                          marker.
**                          The area is not updated if an error is returned.
**
** Returns:
**  II_INGRES_NOT_FOUND     II_SYSTEM undefined.
**  II_CONFIG_NOT_FOUND     Error accessing config.dat.
**  II_CONFIG_BAD_PARAM     Unable to find version registry entries.
**  II_INGRES_1200
**  II_INGRES_1201
**  II_INGRES_209712
**  II_INGRES_209808
**  II_INGRES_209905
**  II_INGRES_200001
**  II_INGRES_250006
**  II_INGRES_250011
**  II_INGRES_260106
**  II_INGRES_260201
**  II_INGRES_260207
**  II_INGRES_260305
**  II_INGRES_300404
**
** Example:
**  # include "tngapi.h"
**
**  II_INT4 version;
**  II_CHAR vers_string[10];
**
**  if ((version = II_IngresVersionEx( vers_string )) > 0)
**  {
**      printf("II_IngresVersion %s returned %d \n", vers_string, version);
**  }
**  else
**  {
**      printf("Bat return %d\n", version);
**  }
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
int
II_IngresVersionEx(char *VersionString)
{
    PM_SCAN_REC    state1;
    STATUS    status;
    char    *ii_systempath;
    int        len = 0;
    char    keys[MAX_REL][MAXSTRING];
    char    versions[MAX_REL][MAX_REL_SIZE];
    char    versionRel[MAX_REL_SIZE];
    char    exeRel[MAX_REL_SIZE];
    int        latestversion;
    int        latestversionrel;
    int        latestversionexe;
    int        ret=0;
    int        i,j, totalnum = 0;
    char    *name;
    char*    className = NULL;
    char    *index;
    LOCATION    loc;
    FILE    *fptr;
    char    iidbmsFile[_MAX_PATH];
    char    mline[MAXSTRING];
#if defined(NT_GENERIC)
    DWORD    fileinfosize =0;
    LPVOID    lpData[50], lpBuffer;
#endif  /* NT_GENERIC */
    char    keyvalues[MAX_REL][MAX_REL_SIZE] = {"1200",
                            "1201",
                            "209712",
                            "209808",
                            "209905",
                            "200001",
                            "250002",
                            "250006",
                            "250011",
                            "260106",
                            "260201",
                            "260207",
                            "260305",
                            "300404",
                            "301",
                            "302",
                            "303"};

    latestversion = -1;
    latestversionrel = -1;
    latestversionexe = -1;

    if (VersionString)
    STcopy("", VersionString);

    /*
    ** Get II_SYSTEM from the environment.
    */
    NMgtAt(ERx( "II_SYSTEM" ), &ii_systempath);

    if (ii_systempath == NULL || *ii_systempath == EOS)
    return II_INGRES_NOT_FOUND;
    else
    ii_systempath = STalloc(ii_systempath);

    /*
    ** Test to see if the config.dat file can be opened
    */
    status = NMloc(UTILITY, FILENAME, "config.dat", &loc);
    if (status == OK)
    SIfopen(&loc, ERx("r"), SI_TXT, 0, &fptr) ;

    if (fptr == NULL)
    return II_CONFIG_NOT_FOUND;
    else
    SIclose(fptr);

    /*
    ** Moved loading of config data to after checking for system path.
    ** No II_SYSTEM or config file means its pointless to continue.
    */
    PMinit();
    status = PMload( NULL, (PM_ERR_FUNC *)NULL );
    /*
    ** A failed status here means that something went wrong with the load
    ** from config.dat.
    ** Any error from the PMload function is reported as II_CONFIG_BAD_PARAM
    ** to avoid returning a +ve integer for a failed status.
    */
    if (status != OK)
    return II_CONFIG_BAD_PARAM;

    /*
    ** First, look for DBMS entries.
    */
    for (status = PMscan("CONFIG.DBMS", &state1, NULL, &name, &className);
     status == OK;
     status = PMscan(NULL, &state1, NULL, &name, &className))
    {
    if (totalnum == 0)
        STcopy(name, keys[totalnum++]);
    else
    {
        for (i = 0; i <= totalnum; i++)
        {
        if (!STcompare(keys[i], name))
            break;
        }

        STcopy(name, keys[totalnum++]);
    }
    }

    /*
    ** Now, look for NET entries.
    */
    for (status = PMscan("CONFIG.NET", &state1, NULL, &name, &className);
     status == OK;
     status = PMscan(NULL, &state1, NULL, &name, &className))
    {
    if (totalnum == 0)
        STcopy(name, keys[totalnum++]);
    else
    {
        for (i = 0; i <= totalnum; i++)
        {
        if (!STcompare(keys[i], name))
            break;
        }

        STcopy(name, keys[totalnum++]);
    }
    }

    if (totalnum == 0)
    return II_CONFIG_BAD_PARAM;

    for (i=0; i < MAX_REL ; i++)
    for (j=0; j < MAX_REL_SIZE ; j++)
        versions[i][j] = '\0' ;

    for (j=0 ; j < totalnum ; j++)
    {
    index = STrindex(keys[j],".", 0);
    index++;

    while (!CMdigit(index))
        index++;

    i=0;

    while (!CMalpha(index))
    {
        versions[j][i] = *index;
        index++;
        i++;
    }
    }

    for (j=0; j < totalnum; j++)
    {
    for (i=0; i < MAX_REL ; i++)
    {
        if (STcompare(versions[j], keyvalues[i]) == 0)
        {
        if (latestversion <= i)
        {
            latestversion = i;
            break;
        }
        }
    }
    }

    status = NMloc(SUBDIR, FILENAME, "version.rel", &loc);
    if (status == OK)
    SIfopen(&loc, ERx("r"), SI_TXT, 0, &fptr);

    if (fptr != NULL)
    {
    SIgetrec (mline, MAXSTRING, fptr);
    index = mline;

    for (i=0; i < MAX_REL_SIZE ; i++)
        versionRel[i] = '\0' ;

           while (!CMdigit(index))
        index++;

    i = 0;
    while (!CMalpha(index))
    {
        if (*index != '/' && *index != '.' )
        {
        if (*index == ' ')
            break;
        versionRel[i] = *index;
        i++;
        }

        index++;
    }

    SIclose(fptr);

    for (i=0; i < MAX_REL ; i++)
    {
        if (STcompare(versionRel, keyvalues[i]) == 0)
        {
        if (latestversionrel <= i)
        {
            latestversionrel = i;
            break;
        }
        }
    }

    if (latestversionrel > latestversion)
        latestversion = latestversionrel;
    }    /* endif Version.rel */

#if defined(NT_GENERIC)
    if (latestversion >= KEY250011)
    {
    STprintf(iidbmsFile, "%s\\ingres\\bin\\iidbms.exe", ii_systempath);

    fileinfosize = GetFileVersionInfoSize(iidbmsFile, NULL);

    if (fileinfosize == 0 && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        STprintf(iidbmsFile, "%s\\ingres\\bin\\iigcc.exe", ii_systempath);
        fileinfosize = GetFileVersionInfoSize(iidbmsFile, NULL);
    }

    if (fileinfosize == 0)
        goto final;

    GetFileVersionInfo(iidbmsFile, NULL, fileinfosize, lpData);

    VerQueryValue(lpData,
              TEXT("\\StringFileInfo\\040904b0\\FileDescription"),
              &lpBuffer,
              &fileinfosize);

    index = lpBuffer;

    for (i=0; i < MAX_REL ; i++)
        exeRel[i] = '\0' ;

    while (!CMdigit(index))
        index++;

    i = 0;
    while (!CMalpha(index))
    {
        if (*index != '/' && *index != '.' )
        {
        if (*index == ' ')
            break;
        exeRel[i] = *index;
        i++;
        }

        index++;
    }

    for (i=0; i < MAX_REL ; i++)
    {
        if (STcompare(exeRel,keyvalues[i]) == 0)
        {
        if (latestversionexe <= i)
        {
            latestversionexe = i;
            break;
        }
        }
    }

    if (latestversionexe > latestversion)
        latestversion = latestversionexe;
    }

#endif  /* NT_GENERIC */

final:

    if (VersionString)
    STprintf(VersionString, "%s", keyvalues[latestversion]);

    switch (latestversion)
    {
    case KEY1200:
        return II_INGRES_1200;

    case KEY1201:
        return II_INGRES_1201;

    case KEY209712:
        return II_INGRES_209712;

    case KEY209808:
        return II_INGRES_209808;

    case KEY209905:
        return II_INGRES_209905;

    case KEY200001:
        return II_INGRES_200001;

    case KEY250002:
    case KEY250006:
        return II_INGRES_250006;    /* both keys are the same version */

    case KEY250011:
        return II_INGRES_250011;

    case KEY260106:
    case KEY260201:
        return II_INGRES_260201;    /* both keys are the same version */

    case KEY260207:
        return II_INGRES_260207;

    case KEY260305:
        return II_INGRES_260305;

    case KEY300404:
        return II_INGRES_300404;

    case KEY301:
        return II_INGRES_301;

    case KEY302:
        return II_INGRES_302;

    case KEY303:
        return II_INGRES_303;

    default:
        return -1;
    }
}

/*{
** Name: II_IngresServiceName - get the internal Ingres service name
**
** Description:
**  Returns the service string used to identify the Ingres instance within
**  the service control manager.
**
** Inputs:
**  ServiceName             Pointer to receive the service name string.
**
** Outputs:
**  ServiceName             Service name string terminated with an end of
**                          string marker.
**
** Returns:
**  None.
**
** Example:
**  # include "tngapi.h"
**
**  II_CHAR service_name[30];
**
**  II_IngresServiceName( service_name );
**  printf( "II_IngresServiceName %s\n", service_name );
**
** History:
**      07-Feb-2005 (fanra01)
**          Added History.
}*/
void
II_IngresServiceName(char *ServiceName)
{
    char    *ii_install;

    NMgtAt(ERx("II_INSTALLATION"), &ii_install);
    if (ii_install == NULL || *ii_install == EOS)
        ii_install = ERx( "" );
    else
        ii_install = STalloc(ii_install);

    STprintf(ServiceName, "%s_Database_%s", SystemServiceName, ii_install);
}

/*{
** Name: II_GetIngresInstallSize - Report the expected Ingres install size
**
** Description:
**  Calculate the expected disk space that the installation is expected to
**  occupy using the packages described in the response file.
**
** Inputs:
**	response_file	Path to the Embedded Ingres response file
**                  For more information concerning response file options
**                  please see the Ingres Embedded Edition User Guide.
**
** Outputs:
**  InstallSize     Estimated filespace the installation is expected to
**                  occupy.
**
** Returns:
**  None.
**
** Example:
**  # include "tngapi.h"
**
**  # if defined(UNIX)
**  II_CHAR reponsefile[] = {"./ingrsp.rsp"};
**  # else
**  II_CHAR reponsefile[] = {".\\ingres.rsp"};
**  # endif
**  double  InstallSize = 0;
**
**  II_GetIngresInstallSize( responsefile, &InstallSize );
**  printf( "II_GetIngresInstallSize = %f\n", InstallSize );
**
**  Note:
**  On Unix/Linux platforms, the routine needs to retrieve package size 
**  from manifest file ( release.dat ), so it will perform the following  
**  steps to search the file :  
**  1) The location pointed by $II_MANIFEST_DIR if the environment  
**     variable is set.
**  2) $II_SYSTEM/ingres/install  
**  If the manifest file is not found, error message will be sent out  
**  to errlog.log and return -1 as value in InstallSize.
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
}*/
void
II_GetIngresInstallSize (char *ResponseFile, double *InstallSize)
{
    char    szBuffer[512];
    double    logfilesize = SIZE_TRANSACTION_LOG_DEFAULT;
    bool    DBMSSelected = FALSE, NETSelected = FALSE;
    bool    ESQLCSelected = FALSE, TOOLSSelected = FALSE;
    longlong    SectorsPerCluster = 0, BytesPerSector = 0, ClusterSize = 0;
    longlong    NumberOfFreeClusters = 0, TotalNumberOfClusters = 0;
    char    drive[16];
    i4      i ; 

#if defined(NT_GENERIC)
    if (GetFileAttributes(ResponseFile) == -1)
    {
        *InstallSize = -1;
        return;
    }

    /*
    ** Get the transaction log file size, if set.
    */
    if (GetPrivateProfileString("Ingres Configuration", "logfilesize", "",
                szBuffer, sizeof(szBuffer), ResponseFile))
    {
    logfilesize = (f4)atoi(szBuffer);
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres DBMS Server",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_DBMS + SIZE_DBMS_ICE + SIZE_DBMS_NET +
               SIZE_DBMS_NET_ICE + SIZE_DBMS_NET_TOOLS +
               SIZE_DBMS_NET_VISION + SIZE_DBMS_REPLICATOR +
               SIZE_DBMS_TOOLS + SIZE_DBMS_VDBA +
               SIZE_DBMS_VISION +
               SIZE_ODBC + SIZE_DBMS_EXTRAS + logfilesize;

        DBMSSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Net",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        if (DBMSSelected)
        *InstallSize += SIZE_NET + SIZE_NET_TOOLS;
        else
        {
        *InstallSize += SIZE_NET + SIZE_DBMS_NET + SIZE_NET_TOOLS +
                   SIZE_DBMS_NET_ICE + SIZE_DBMS_NET_TOOLS +
                   SIZE_DBMS_NET_VISION + SIZE_ODBC;
        }

        NETSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Star",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_STAR;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres Embedded SQL/C",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_ESQLC + SIZE_ESQLC_ESQLCOB;

        ESQLCSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Embedded SQL/COBOL",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        if (ESQLCSelected)
        *InstallSize += SIZE_ESQLCOB;
        else
        *InstallSize += SIZE_ESQLCOB + SIZE_ESQLC_ESQLCOB;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Embedded SQL/Fortran",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_ESQLFORTRAN;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Querying and Reporting Tools",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_TOOLS + SIZE_TOOLS_VISION;

        if (!DBMSSelected)
        *InstallSize += SIZE_DBMS_TOOLS;
        if (!NETSelected)
        *InstallSize += SIZE_NET_TOOLS;
        if (!DBMSSelected && !NETSelected)
        *InstallSize += SIZE_DBMS_NET_TOOLS;

        TOOLSSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Vision",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_VISION;

        if (!DBMSSelected)
        *InstallSize += SIZE_DBMS_VISION;
        if (!DBMSSelected && !NETSelected)
        *InstallSize += SIZE_DBMS_NET_VISION;
        if (!TOOLSSelected)
        *InstallSize += SIZE_TOOLS_VISION;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Replicator",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_REPLICATOR;

        if (!DBMSSelected)
        *InstallSize += SIZE_DBMS_REPLICATOR;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/ICE",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_ICE + SIZE_ICE_EXTRAS;

        if (!DBMSSelected)
        *InstallSize += SIZE_DBMS_ICE;
        if (!DBMSSelected && !NETSelected)
        *InstallSize += SIZE_DBMS_NET_ICE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Online Documentation",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_DOC;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres Visual DBA",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_VDBA;

        if (!DBMSSelected)
        *InstallSize += SIZE_DBMS_VDBA;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "JDBC Server",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_JDBC;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres .NET Data Provider",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_DOTNETDP;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "II_MDB_INSTALL",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_MDB;
    }
    }

    /*
    ** Adjust the disk space to account for the allocation units of the disk.
    ** the pre-defined values are based upon a 512 allocation unit, 8 sectors
    ** per cluster.
    */
    if (GetPrivateProfileString("Ingres Configuration", "II_SYSTEM",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
        longlong    NumClusters;

        strncpy(drive, szBuffer, 3);
        drive[3] = '\0';

        /*
        ** First, break up our size into number of clusters.
        */
        NumClusters = (*InstallSize * 1024 * 1024) / (8 * 512);

        GetDiskFreeSpace(drive, &SectorsPerCluster, &BytesPerSector,
             &NumberOfFreeClusters, &TotalNumberOfClusters);

        ClusterSize = SectorsPerCluster * BytesPerSector;

        /*
        ** Now, convert our clusters into the localized size, in MB.
        */
        *InstallSize = NumClusters * ClusterSize / (1024 * 1024);
    }

#else  /* if defined(NT_GENERIC) */

      /* The section for Unix/Linux */

     LISTELE     *lp;
     PKGBLK      *pkg; 
     char        *cp;
     char        pkg_name_set[100]; 
     char        rel_path_name[ MAX_LOC ]; 
     double      TotalPkgsSize; 
     char        err_msg[ MAX_LOC + 50 ]; 
     CL_ERR_DESC errdesc;

     if (access(ResponseFile,  R_OK) == -1 )
     {
        *InstallSize = -1;
        return;
     }

    STcopy( ERx("release.dat") , releaseManifest );

    /* check if release.dat exists */
    NMgtAt( ERx( "II_MANIFEST_DIR" ), &cp );
    if( cp != NULL && *cp != EOS )
      STprintf( rel_path_name, ERx("%s/%s"), cp, releaseManifest ); 
    else 
      { 
        NMgtAt( ERx( "II_SYSTEM" ), &cp ); 
        STprintf( rel_path_name, ERx("%s%s%s"), cp,  
                                    "/ingres/install/",  releaseManifest ); 
      }
    if( access( rel_path_name, R_OK ) == -1 )   
      {
        STprintf(err_msg, ERx("%s : %s is not found.\n"),  
                          "II_GetIngresInstallSize()",  rel_path_name ); 
        SIfprintf( stderr,  err_msg );
        *InstallSize = -1;
        return; 
      }

    /*
    ** Get the transaction log file size, if it is specified.
    */
    logfilesize = SIZE_TRANSACTION_LOG_DEFAULT * ( 1024 * 1024 ) ;
    if (GetPrivateProfileString("Ingres Configuration", "LOG_KBYTES", "",
                szBuffer, sizeof(szBuffer), ResponseFile))
    {
      if( STcompare( szBuffer, "<default>" ) != 0 )
        logfilesize = (f4)atoi(szBuffer) * 1024 ;
    }

    /* Setup package list by reading the data from release manifest file */
    if( ip_parse( DISTRIBUTION, &distPkgList, &relID,
                  &productName, TRUE ) != OK )
     {
      *InstallSize = -1 ;  
      return ; 
     }

    /*
    ** Initailized all packages in the list to NOT_SELECTED first.  
    */
    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
     {
      pkg = (PKGBLK *) lp->car;
      pkg->selected = NOT_SELECTED;
     }

    /*
    ** Scan thru the package list, mark down the SELECTED and INCLUDED
    ** packages if they are specified in the response file.  
    */
    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
     {
      pkg = (PKGBLK *) lp->car;
      STprintf( pkg_name_set, ERx( "%s_set" ), pkg->feature );
      if (GetPrivateProfileString("Ingres Configuration",
          pkg_name_set, "", szBuffer, sizeof(szBuffer), ResponseFile))
       {
        if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
            !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
          {
            pkg->selected = SELECTED;
            /* also check the included packages */
            find_needed_packages( pkg );
          }
       }
     }

   /*  Calculate the required disk space size for the selected packages */
    TotalPkgsSize = 0 ;
    *InstallSize = 0 ;
    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
     {
      pkg = (PKGBLK *) lp->car;
      if( pkg->selected == SELECTED )
       TotalPkgsSize +=  pkg->actual_size;
      else if ( pkg->selected ==  INCLUDED )
       TotalPkgsSize +=  pkg->actual_size;
     }

    *InstallSize = ( TotalPkgsSize + logfilesize ) / (1024 * 1024) ;

#endif   /* if defined(NT_GENERIC) */

}

#if defined(UNIX)
/*
** Name :  GetPrivateProfileString()
**
** Description : This is an emulation of the same function in Window API,
**               It retrieves the string associated with the specified
**               key (parameter) in a given response file.
** Parameters  : 1) app_str  - pointer to a null-terminated string
**               2) key_name - pointer to a null-terminated string
**               3) def_str  - pointer to a null-terminated default string
**               4) ret_buf  - pointer to the buffer containing the return
**                             data.
**               5) buf_size - the size of the buffer pointed to by buf_ptr.
**               6) res_file - pointer to a null-terminated string that
**                             names the response file.
** Note : Including app_str and def_str in the parameter list is for the
**        compatibility  (with Window) purpose only, these two parameters
**        will not be used or referenced.
**
** Input  :   key_name   name of the parameter in the response file
**            res_file   name of the response file
** Output :   ret_buf    string retrieved from the response file
**
** Returns:   ret_buf_len -  contains the number of characters in the
**                           buffer pointed by ret_buf if it succeed
**                        -  contains zero if it failed
** History:
**      10-Feb-2004 (hweho01)
**          Wrote for remote installation API (Unicenter), emulated
**          the existing Window API.
*/
int
GetPrivateProfileString( char *app_str, char *key_name, char *def_str,
                             char *ret_buf,  int buf_size, char *res_file )
{
    char  res_line[BUFFER_MAX], data_buf[BUFFER_MAX];
    char  *data_ptr;
    FILE  *res_fdesc;
    LOCATION    loc;
    int         ret_buf_len = 0;

    LOfroms( PATH & FILENAME, res_file, &loc );
    /* read the entries one by one in the response file */
    if( (SIopen( &loc, "r", &res_fdesc)) != OK )
    {
        SIprintf("\nUnable to open response file %s. \n", res_file);
        return(ret_buf_len);
    }
    else
    {
        while( ( SIgetrec(res_line, sizeof(res_line), res_fdesc ) ) == OK )
        {
            if((data_ptr=STstrindex(res_line,key_name,sizeof(res_line),TRUE))
                != NULL)
            {
                if((data_ptr = STindex( res_line, "=", sizeof(res_line)))
                    != NULL )
                {
                    data_ptr++;     /* point to the value in the entry */
                    STzapblank (data_ptr, data_buf);
                    if ( ( ret_buf_len = STlength( data_buf ) ) > buf_size )
                    {
                        SIprintf("\nInsufficent buffer for data from response file.\n");
                        ret_buf_len = 0;
                        break;
                    }
                    STcopy( data_buf, ret_buf );
                    break;
                }
                else
                    break;
            }
        }
        SIclose(res_fdesc);
    }

    return ( ret_buf_len );

}

#endif  /* if defined(UNIX)  */

#if defined(NT_GENERIC)
void
GetMergeModuleSize(MSIHANDLE hDb, char *MMName, double *MMSize)
{
    MSIHANDLE hView, hView02, hRecord, hRecord02;
    char szQuery[512], szValue[512], szValue02[512];
    DWORD dwSize, dwSize02;

    *MMSize=0;
    sprintf(szQuery, "SELECT Component FROM ModuleComponents WHERE ModuleID='%s'", MMName);
    MsiDatabaseOpenView(hDb, szQuery, &hView);
    if (hView && !MsiViewExecute(hView, 0))
    {
    while (!MsiViewFetch(hView, &hRecord))
    {
        dwSize=sizeof(szValue);
        if (!MsiRecordGetString(hRecord, 1, szValue, &dwSize))
        {
        sprintf(szQuery, "SELECT FileSize FROM File WHERE Component_='%s'", szValue);
        MsiDatabaseOpenView(hDb, szQuery, &hView02);
        if (hView && !MsiViewExecute(hView02, 0))
        {
            while (!MsiViewFetch(hView02, &hRecord02))
            {
            dwSize02=sizeof(szValue02);
            if (!MsiRecordGetString(hRecord02, 1, szValue02, &dwSize02))
                *MMSize+=atof(szValue02);
            }
        }
        }
    }/* while */
    }
    *MMSize=*MMSize/(1024*1024);
    if (hRecord) MsiCloseHandle(hRecord);
    if (hRecord02) MsiCloseHandle(hRecord02);
    if (hView) MsiCloseHandle(hView);
    if (hView02) MsiCloseHandle(hView02);
    return;
}
#endif  /* NT_GENERIC */

/*{
** Name: II_GetIngresInstallSizeEx - Report the expected Ingres install size
**
** Description:
**  Calculate the expected disk space that the installation is expected to
**  occupy using the packages described in the response file.
**
** Inputs:
** 1) response_file Path to the Embedded Ingres response file
**                  For more information concerning response file options
**                  please see the Ingres Embedded Edition User Guide.
**
** 2)  For Window platform : 
**     MSIPath      Specifies the full path or relative path to the Microsoft
**                  installer database file.
**
**     For Unix/Linux platforms :
**     RelManifestPath Specifies the full path or relative path to the  
**                     release manifest file.
**
** Outputs:
**  InstallSize     Estimated filespace the installation is expected to
**                  occupy.
**
** Returns:
**  None.
**
** Example:
**  # include "tngapi.h"
**
**  # if defined(NT_GENERIC)
**  II_CHAR reponsefile[] = {".\\ingres.rsp"};
**  # else
**  II_CHAR reponsefile[] = {"./ingrsp.rsp"};
**  # endif
**  double  InstallSize = 0;
**
**  # if defined(NT_GENERIC) 
**  II_GetIngresInstallSizeEx( responsefile, MSIPath, &InstallSize );
**  printf( "II_GetIngresInstallSize = %f\n", InstallSize );
**  # else  
**  II_GetIngresInstallSizeEx( responsefile, RelManifestPath, &InstallSize );
**  printf( "II_GetIngresInstallSize = %f\n", InstallSize );
**  #endif 
**
** History:
**      07-Feb-2005 (fanra01)
**          Added history.
**      15-Oct-2006 (hweho01)
**          Added support for Unix/Linux platforms.
}*/
#if defined(NT_GENERIC)
void
II_GetIngresInstallSizeEx (char *ResponseFile, char *MSIPath, double *InstallSize)
{
    char szBuffer[1024];
    double logfilesize=0;
    bool DBMSSelected=FALSE, NETSelected=FALSE;
    bool ESQLCSelected=FALSE, TOOLSSelected=FALSE;
    longlong SectorsPerCluster=0, BytesPerSector=0, ClusterSize=0;
    longlong NumberOfFreeClusters=0, TotalNumberOfClusters=0;
    char drive[16];
    MSIHANDLE hDb;
    double size_dbms;
    double size_dbms_ice;
    double size_dbms_net;
    double size_dbms_net_ice;
    double size_dbms_net_tools;
    double size_dbms_net_vision;
    double size_dbms_replicator;
    double size_dbms_tools;
    double size_dbms_vdba;
    double size_dbms_vision;
    double size_doc;
    double size_dotnetdp;
    double size_esqlc;
    double size_esqlc_esqlcob;
    double size_esqlcob;
    double size_esqlfortran;
    double size_ice;
    double size_jdbc;
    double size_net;
    double size_net_tools;
    double size_odbc;
    double size_replicator;
    double size_star;
    double size_tools;
    double size_tools_vision;
    double size_vdba;
    double size_vision;
    double size_dbms_extras;
    double size_ice_extras;
    double size_transaction_log_default;

    size_dbms_extras=12.867;
    size_ice_extras=14.700;
    size_transaction_log_default=32.000;

    if ((GetFileAttributes(ResponseFile) == -1) || (GetFileAttributes(MSIPath) == -1))
    {
    *InstallSize = -1;
    return;
    }

    if (MsiOpenDatabase(MSIPath, MSIDBOPEN_READONLY, &hDb))
    {
    *InstallSize = -1;
    return;
    }

    GetMergeModuleSize(hDb, "IngresIIDBMS.870341B5_2D6D_11D5_BDFC_00B0D0AD4485", &size_dbms);
    GetMergeModuleSize(hDb, "IngresIIDBMSIce.13772035_2E42_11D5_BDFC_00B0D0AD4485", &size_dbms_ice);
    GetMergeModuleSize(hDb, "IngresIIDBMSNet.96721983_F981_11D4_819B_00C04F189633", &size_dbms_net);
    GetMergeModuleSize(hDb, "IngresIIDBMSNetIce.11E96427_2E33_11D5_BDFC_00B0D0AD4485", &size_dbms_net_ice);
    GetMergeModuleSize(hDb, "IngresIIDBMSNetTools.F20BFF46_2E2E_11D5_BDFC_00B0D0AD4485", &size_dbms_net_tools);
    GetMergeModuleSize(hDb, "IngresIIDBMSNetVision.C3CD8D55_2E42_11D5_BDFC_00B0D0AD4485", &size_dbms_net_vision);
    GetMergeModuleSize(hDb, "IngresIIDBMSReplicator.9A8FCFE6_2E40_11D5_BDFC_00B0D0AD4485", &size_dbms_replicator);
    GetMergeModuleSize(hDb, "IngresIIDBMSTools.9BD3B8D5_2E34_11D5_BDFC_00B0D0AD4485", &size_dbms_tools);
    GetMergeModuleSize(hDb, "IngresIIDBMSVdba.AC0D8A36_2EC4_11D5_BDFC_00B0D0AD4485", &size_dbms_vdba);
    GetMergeModuleSize(hDb, "IngresIIDBMSVision.64519E46_2E30_11D5_BDFC_00B0D0AD4485", &size_dbms_vision);
    GetMergeModuleSize(hDb, "IngresIIDoc.027C6035_32EC_11D5_BDFD_00B0D0AD4485", &size_doc);
    GetMergeModuleSize(hDb, "IngresIIDotNETDataProvider.2ACEFE22_2A2F_42FC_85D7_C51687E9E15C", &size_dotnetdp);
    GetMergeModuleSize(hDb, "IngresIIEsqlC.EFB4AAA4_2ECF_11D5_BDFC_00B0D0AD4485", &size_esqlc);
    GetMergeModuleSize(hDb, "IngresIIEsqlCEsqlCobol.348475A5_2ED1_11D5_BDFC_00B0D0AD4485", &size_esqlc_esqlcob);
    GetMergeModuleSize(hDb, "IngresIIEsqlCobol.9D6ADEC5_2ED1_11D5_BDFC_00B0D0AD4485", &size_esqlcob);
    GetMergeModuleSize(hDb, "IngresIIEsqlFortran.8361FFCD_6FE1_45FB_BCB9_480438EF57FB", &size_esqlfortran);
    GetMergeModuleSize(hDb, "IngresIIIce.95BA2255_2F07_11D5_BDFC_00B0D0AD4485", &size_ice);
    GetMergeModuleSize(hDb, "IngresIIJdbc.4925A2E6_3383_11D5_BDFD_00B0D0AD4485", &size_jdbc);
    GetMergeModuleSize(hDb, "IngresIINet.89BE49FA_2EC5_11D5_BDFC_00B0D0AD4485", &size_net);
    GetMergeModuleSize(hDb, "IngresIINetTools.87B765B5_2ECA_11D5_BDFC_00B0D0AD4485", &size_net_tools);
    GetMergeModuleSize(hDb, "IngresIIODBC.70DC58B6_2D77_11D5_BDFC_00B0D0AD4485", &size_odbc);
    GetMergeModuleSize(hDb, "IngresIIReplicator.531C8AD6_2F05_11D5_BDFC_00B0D0AD4485", &size_replicator);
    GetMergeModuleSize(hDb, "IngresIIStar.97446DA5_2ECD_11D5_BDFC_00B0D0AD4485", &size_star);
    GetMergeModuleSize(hDb, "IngresIITools.A6479506_2ED9_11D5_BDFC_00B0D0AD4485", &size_tools);
    GetMergeModuleSize(hDb, "IngresIIToolsVision.191D1AA6_2EDD_11D5_BDFC_00B0D0AD4485", &size_tools_vision);
    GetMergeModuleSize(hDb, "IngresIIVdba.21C4C978_3382_11D5_BDFD_00B0D0AD4485", &size_vdba);
    GetMergeModuleSize(hDb, "IngresIIVision.395BC475_2F00_11D5_BDFC_00B0D0AD4485", &size_vision);
    MsiCloseHandle(hDb);

    *InstallSize=0;
    /*
    ** Get the transaction log file size, if set.
    */
    if (GetPrivateProfileString("Ingres Configuration", "logfilesize", "",
                szBuffer, sizeof(szBuffer), ResponseFile))
    {
    logfilesize = (double)atoi(szBuffer);
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres DBMS Server",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_dbms + size_dbms_ice + size_dbms_net +
               size_dbms_net_ice + size_dbms_net_tools +
               size_dbms_net_vision + size_dbms_replicator +
               size_dbms_tools + size_dbms_vdba +
               size_dbms_vision +
               size_odbc + size_dbms_extras + logfilesize;

        DBMSSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Net",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        if (DBMSSelected)
        *InstallSize += size_net + size_net_tools;
        else
        {
        *InstallSize += size_net + size_dbms_net + size_net_tools +
                   size_dbms_net_ice + size_dbms_net_tools +
                   size_dbms_net_vision + size_odbc;
        }

        NETSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Star",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_star;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres Embedded SQL/C",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_esqlc + size_esqlc_esqlcob;

        ESQLCSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Embedded SQL/COBOL",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        if (ESQLCSelected)
        *InstallSize += size_esqlcob;
        else
        *InstallSize += size_esqlcob + size_esqlc_esqlcob;

    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Embedded SQL/Fortran",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_esqlfortran;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Querying and Reporting Tools",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_tools + size_tools_vision;

        if (!DBMSSelected)
        *InstallSize += size_dbms_tools;
        if (!NETSelected)
        *InstallSize += size_net_tools;
        if (!DBMSSelected && !NETSelected)
        *InstallSize += size_dbms_net_tools;

        TOOLSSelected = TRUE;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Vision",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_vision;

        if (!DBMSSelected)
        *InstallSize += size_dbms_vision;
        if (!DBMSSelected && !NETSelected)
        *InstallSize += size_dbms_net_vision;
        if (!TOOLSSelected)
        *InstallSize += size_tools_vision;

    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/Replicator",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_replicator;

        if (!DBMSSelected)
        *InstallSize += size_dbms_replicator;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres/ICE",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_ice + size_ice_extras;

        if (!DBMSSelected)
        *InstallSize += size_dbms_ice;

        if (!DBMSSelected && !NETSelected)
        *InstallSize += size_dbms_net_ice;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration",
                "Ingres Online Documentation",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_doc;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres Visual DBA",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_vdba;

        if (!DBMSSelected)
        *InstallSize += size_dbms_vdba;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "JDBC Server",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_jdbc;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "Ingres .NET Data Provider",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += size_dotnetdp;
    }
    }

    if (GetPrivateProfileString("Ingres Configuration", "II_MDB_INSTALL",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
        !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
    {
        *InstallSize += SIZE_MDB;
    }
    }

    /*
    ** Adjust the disk space to account for the allocation units of the disk.
    ** the pre-defined values are based upon a 512 allocation unit, 8 sectors
    ** per cluster.
    */
    if (GetPrivateProfileString("Ingres Configuration", "II_SYSTEM",
                "", szBuffer, sizeof(szBuffer), ResponseFile))
    {
    double    NumClusters;

    strncpy(drive, szBuffer, 3);
    drive[3] = '\0';

    /*
    ** First, break up our size into number of clusters.
    */
    NumClusters = (*InstallSize * 1024 * 1024) / (8 * 512);

    GetDiskFreeSpace(drive, &SectorsPerCluster, &BytesPerSector,
             &NumberOfFreeClusters, &TotalNumberOfClusters);

    ClusterSize = SectorsPerCluster * BytesPerSector;

    /*
    ** Now, convert our clusters into the localized size, in MB.
    */
    *InstallSize = NumClusters * ClusterSize / (1024 * 1024);
    }
}

#else    /* #if deined(NT_GENERIC) */

       /* The section for Unix/Linux  */ 
void
II_GetIngresInstallSizeEx (char *ResponseFile, 
                           char *RelManifestPath, 
                           double *InstallSize)
{

  i4        i; 
  double    logfilesize, TotalPkgsSize; 
  char      szBuffer[512];
  char      *cp, *dir_ptr; 
  char      manifest_dir[MAX_LOC + 1];  
  PKGBLK    *pkg;
  LISTELE   *lp;
  char      pkg_name_set[100];
  char      err_msg[ MAX_LOC + 50]; 
  CL_ERR_DESC errdesc;

    if (access(ResponseFile,  R_OK) == -1 )
     {
        *InstallSize = -1;
        return;
     }

    if (access(RelManifestPath,  R_OK) == -1 )
     {
      STprintf(err_msg, ERx("%s : %s is not found. \n"),  
                       "II_GetIngresInstallSizeEx()", RelManifestPath ); 
      SIfprintf( stderr,  err_msg );
      *InstallSize = -1;
      return;
     }

    /*
    ** Get the transaction log file size, if it is specified.
    */
    logfilesize = SIZE_TRANSACTION_LOG_DEFAULT * ( 1024 * 1024 ) ;
    if (GetPrivateProfileString("Ingres Configuration", "LOG_KBYTES", "",
                szBuffer, sizeof(szBuffer), ResponseFile))
    {
      if( STcompare( szBuffer, "<default>" ) != 0 )
         logfilesize = (f4)atoi(szBuffer) * 1024 ;
    }

    STcopy( RelManifestPath, manifest_dir );
    if (( dir_ptr = STrindex( manifest_dir, "/", 0)) != NULL )
     {
       *dir_ptr = EOS;
       STcopy( (++dir_ptr), releaseManifest );
     }
    else
     {
       STcopy( "./",  manifest_dir );
       STcopy( RelManifestPath, releaseManifest );
     }

     /* check if manifest variable is set   */
     NMgtAt( ERx( "II_MANIFEST_DIR" ), &cp );
     if( cp != NULL && *cp != EOS )
        { 
          if( STcompare( cp, manifest_dir ) != 0 ) 
             IPCLsetEnv( "II_MANIFEST_DIR", manifest_dir, FALSE );  
       }   
    
   /* Setup package list by reading the data from release manifest file */
   if( ip_parse( DISTRIBUTION, &distPkgList, &relID,
                 &productName, TRUE ) != OK )
      *InstallSize = -1 ;  

   if ( cp != NULL) 
        IPCLsetEnv( "II_MANIFEST_DIR", cp , FALSE);  
    
    if( *InstallSize == -1 ) 
        return ; 

    /*
    ** Initailized all packages in the list to NOT_SELECTED first.  
    */
    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
     {
      pkg = (PKGBLK *) lp->car;
      pkg->selected = NOT_SELECTED;
     }

    /*
    ** Scan thru the package list, mark down the SELECTED and INCLUDED
    ** packages if they are specified in the response file.  
    */
    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
     {
      pkg = (PKGBLK *) lp->car;
      STprintf( pkg_name_set, ERx( "%s_set" ), pkg->feature );
      if (GetPrivateProfileString("Ingres Configuration",
          pkg_name_set, "", szBuffer, sizeof(szBuffer), ResponseFile))
       {
        if (!STncasecmp(szBuffer, "y", 1) || !STncasecmp(szBuffer, "yes", 3) ||
            !STncasecmp(szBuffer, "1", 1) || !STncasecmp(szBuffer, "true", 4))
          {
            pkg->selected = SELECTED;
            /* also check the included packages */
            find_needed_packages( pkg );
          }
       }
     }


   /*  Calculate the required disk space size for the selected packages */
    TotalPkgsSize = 0 ; 
    *InstallSize = 0 ; 
    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
     {
      pkg = (PKGBLK *) lp->car;
      if( pkg->selected == SELECTED ) 
       TotalPkgsSize +=  pkg->actual_size; 
      else if ( pkg->selected ==  INCLUDED )
       TotalPkgsSize +=  pkg->actual_size; 
     }

    *InstallSize = ( TotalPkgsSize + logfilesize ) / (1024 * 1024) ;  

} 
#endif    /* if defined(NT_GENERIC) */

#if defined(UNIX)
/*
**   Function : find_needed_patckages( PKGBLD *)
**
**              It is called by II_GetIngresInstallSize()
**              and II_GetIngresInstallSizeEx() routines, 
**              will find out the all the packages required 
**              for the selected components.
**
*/
static void
find_needed_packages(PKGBLK *pkg)
{
    PKGBLK *sub_pkg;
    PRTBLK *prt;
    FILBLK *fil;
    LISTELE *lp;
    REFBLK *ref;

    /* find all needed packages */ 
    for( lp = pkg->refList.head; lp != NULL ; lp = lp->cdr )
    {
        ref = (REFBLK *) lp->car;

        if( ((sub_pkg = ip_find_package( ref->name, DISTRIBUTION ))
                != NULL ) && (ref->type != PREFER ) )
        {
            find_needed_packages(sub_pkg);
        }
    }
    pkg->selected = INCLUDED;
}

/*
** Dummy ip_forms routine.  
** Resolv the undefined symbol in some Unix platforms such as Solaris 
** due to the inclusion of libinstall.a  
*/ 
void
ip_forms( bool yesno )
{
	return;
}

/*
** Dummy ip_display_msg routine.  
** Resolv the undefined symbol in some Unix platforms such as Solaris 
** due to the inclusion of libinstall.a  
*/ 
void ip_display_msg( char *msg )
{
  return; 
}

/*
** Dummy ip_display_transient_msg routine.   
** Resolv the undefined symbol in some Unix platforms such as Solaris 
** due to the inclusion of libinstall.a  
*/ 
void ip_display_transient_msg( char *msg )
{
	return; 
}

/*
** Dummy ip_dep_popup routine.   
** Resolv the undefined symbol in some Unix platforms such as Solaris 
** due to the inclusion of libinstall.a  
*/ 
bool
ip_dep_popup( PKGBLK *pkg1, PKGBLK *pkg2, char *oldvers )
{
	return( TRUE );
}

#endif   /* if defined(UNIX) */

