/*
**Copyright (c) 2004, 2008 Ingres Corporation
*/
/**
** Name: gcnmo
**
** Description:
**      Defines
**	    GVcnf_init		Add initialization of configuration parameter MIB objects
**	    GVcnf_term		Remove initialized version MIB objects
**          GVmo_init           Initialize version MIB objects
**          GVmo_term           Free version MIB objects
**          GVgetverstr         Return the version string
**          GVgetenvstr         Return the platform string
**	    GVgetversion	Return the build level
**          GVgetpatchlvl       Return the patch level
**          GVgettcpportaddr    Return the TCP port address
**          GVgetnetportaddr    Return the secondary  net driver address
**          GVgetconformance    Return the SQL92 conformance
**          GVgetlanguage       Return the language setting
**          GVgetcharset        Return the character set settting
**	    GVgetinstance	Return the instance identifier
**	    GVgetsystempath	Return the installation path
**
**  History:
**      29-Jan-2004 (fanra01)
**          SIR 111718
**          Created.
**      12-Feb-2004 (fanra01)
**          Replaced IIPTR with generic type.
**          Corrected comment syntax error.
**      01-Mar-2004 (fanra01)
**          SIR 111718
**          Add functions GVgetverstr and GVgetenvstr.
**      08-Mar-2004 (fanra01)
**          Made network driver string dependent on compile time flags.
**          Removed GVgetnetportaddr.
**          Renamed GVgetlanportaddr to GVgetnetportaddr.
**      09-Mar-2004 (fanra01)
**          Updated to use variable prefix.
**      23-Mar-2004 (fanra01)
**          Add GVgetinstance call.
**      24-Jun-2004 (fanra01)
**          Bug 122555
**          Setting of PM defaults to dbms may not be appropriate for the
**          calling process.  Add a save and restore of defaults.
**      18-Aug-2004 (hweho01)
**          Added MDB.MDB_DBNAME to cnf ( configuration parameter list ),  
**          so the mdb name will be included in the MO object. 
**      24-Feb-2005 (fanra01)
**          Sir 113975
**          Update parameters list to include all names containing an MDB
**          element.
**      01-Jun-2005 (fanra01)
**          SIR 114614
**          Add return of system path value and build level.
**	23-Jun-2008 (drivi01)
**	    Update comments for GV functions.
**	24-Sep-2008 (lunbr01)  Sir 119985
**	    Change primary_net[] value for Windows from "wintcp" to "tcp_ip".
**/
# include <compat.h>
# include <cm.h>
# include <cv.h>
# include <er.h>
# include <ex.h>
# include <gv.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <qu.h>
# include <st.h>

# include <gl.h>
# include <pm.h>
# include <sp.h>
# include <mo.h>

# include <gcccl.h>

# include "gvint.h"

# define MAXLINE        256
# define DEFAULT_FIELDS 4
# define TYPE_INT       1
# define TYPE_STR       2

# define ID_MAJOR       2

GLOBALREF char*         SystemCfgPrefix;
GLOBALREF MO_CLASS_DEF  gv_classes[];
GLOBALREF MO_CLASS_DEF  gv_prot_classes[];
GLOBALREF MO_CLASS_DEF  gv_cnf_classes[];
GLOBALREF MO_CLASS_DEF  gv_cnfdat_classes[];

static      i4          gv_cnf_init= FALSE;
static      i4          gv_mo_init = FALSE;

static      char*   GVcnf_index_name = { GV_MIB_CNF_INDEX };
static      char*   GVcnfdat_index_name = { GV_MIB_CNFDAT_INDEX };

# ifdef UNIX
static      char    primary_net[] = { ERx("TCP_IP") };
static      char    secondary_net[] = { ERx("TCP_IP") };
# else
static      char    primary_net[] = { ERx("TCP_IP") };
static      char    secondary_net[] = { ERx("LANMAN") };
# endif

typedef struct _cnf_list    CNF_LIST;

struct _cnf_list
{
    CNF_LIST*   next;
    CNF_LIST*   prev;
    i4          instance;
    ING_CONFIG  cnf;
};

CNF_LIST* cnfdathead ZERO_FILL;
CNF_LIST* cnfdattail ZERO_FILL;
CNF_LIST* cnfhead ZERO_FILL;
CNF_LIST* cnftail ZERO_FILL;

char*   cnf[] = 
{
    "^.*.\\.CONNECT_LIMIT$",
    "^.*.\\.OPF_MEMORY$",
    "^.*.\\.QSF_MEMORY$",
    "^.*.\\.QEF_MEMORY$",
    "^.*.\\.P[0-9]+K_STATUS$",
    "^.*.\\.P[0-9]+K\\.DMF_MEMORY$",
    "^.*.\\.RCP.FILE.KBYTES$",
    "^.*.\\.MDB\\.MDB_MDBNAME$",
    "^.*.\\.MDB.*",
    NULL
};

char*   prohibit[] =
{
    ".C2.",
    ".SECURE.",
    ".GCF.MECH",
    ".GCF.REMOTE_AUTH_ERROR",
    NULL
};

/*{
** Name: GVcnf_init
**
** Description:
**      Add initialization of configuration parameter MIB objects.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Return:
**      OK      command completed successfully.
**      FAIL
**
** History:
**      10-Mar-2004 (fanra01)
**          Created.
}*/
STATUS
GVcnf_init()
{
    STATUS  status = OK;
    i4      i;
    char*   remove;
    i4      count;
    PM_SCAN_REC state;
    PTR     pmname = NULL;
    PTR     pmval = NULL;
    char    buf[10];
    CNF_LIST*   tmp;
    char    pmexp[100];
    
    if(gv_cnf_init == FALSE)
    {
        char* p = buf;
        
        pmval = SystemCfgPrefix;
        for (; *pmval; CMnext( pmval ), CMnext( p ))
        {
            CMtoupper( pmval, p );
        }
        *p = '\0';
        STprintf( pmexp, "%s.*", buf );
        p = pmexp;
        if((status = MOclassdef( MAXI2, gv_cnfdat_classes )) == OK)
        {
            MEfill( sizeof(PM_SCAN_REC), 0, &state );
            for(status = OK, count=0; status == OK; count+=1)
            {
                if ((status = PMscan( p, &state, NULL, 
                    (char**)&pmname, (char**)&pmval )) == OK)
                {
                    for (i=0, remove=NULL; (prohibit[i] != NULL) &&
                        (remove == NULL); i+=1 )
                    {
                        remove = STstrindex( pmname, prohibit[i], 0,
                            TRUE );
                    }
                    if (remove == NULL)
                    {
                        tmp = (CNF_LIST*)MEreqmem(0, sizeof(CNF_LIST),
                            TRUE, NULL);
                        if (tmp != NULL)
                        {
                            tmp->cnf.param_name = STalloc(pmname);
                            tmp->cnf.param_val = STalloc((char*)pmval);
                            tmp->cnf.param_len = STlength((char*)pmval);
                            tmp->next = NULL;
                            if (cnfdathead != NULL)
                            {
                                tmp->prev = cnftail;
                                cnfdattail->next = tmp;
                                cnfdattail = tmp;
                            }
                            else
                            {
                                cnfdathead = tmp;
                                cnfdattail = tmp;
                                tmp->prev = NULL;
                            }
                        }
                        tmp->instance = count;
                        CVla( count, buf );
                        status = MOattach( MO_INSTANCE_VAR,
                            GVcnfdat_index_name, buf, (PTR)&tmp->cnf );
                    }
                    p = NULL;
                }
            }
        }
        if ((cnfdathead != NULL) &&
            ((status == OK) || (status == PM_NOT_FOUND)))            
        {
            gv_cnf_init = TRUE;
        }
    }
    return(status);
}

/*{
** Name: GVcnf_term
**
** Description:
**      Remove initialized version MIB objects.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Return:
**      None
**
** History:
**      10-Mar-2004 (fanra01)
**          Created.
}*/
VOID
GVcnf_term()
{
    CNF_LIST*   tmp;
    char    instance[10];

    if(gv_cnf_init == TRUE)
    {
        for (tmp=cnfdathead; (tmp != NULL); tmp=cnfdathead)
        {
            CVla( tmp->instance, instance );
            if (MOdetach( GVcnfdat_index_name, instance ) == OK)
            {
                if (tmp->cnf.param_name)
                {
                    MEfree( tmp->cnf.param_name );
                }
                if (tmp->cnf.param_val)
                {
                    MEfree( tmp->cnf.param_val );
                }
            }
            cnfdathead = tmp->next;
            if (cnfdathead != NULL)
            {
                cnfdathead->prev = NULL;
            }
            else
            {
                cnfdattail = NULL;
            }
            MEfree( (PTR)tmp );
        }
    }
}

/*{
** Name: GVmo_init
**
** Description:
**      Add initialization of version MIB objects.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Return:
**      OK      command completed successfully.
**      FAIL	command failed
**
** History:
**      04-Feb-2004 (fanra01)
**          Created.
}*/
STATUS
GVmo_init()
{
    STATUS  status = OK;
    i4      i;
    i4      count;
    PM_SCAN_REC state;
    PTR     pmname = NULL;
    PTR     pmval = NULL;
    char    instance[10];
    char*   host;
    CNF_LIST*   tmp;
    char*   pmexp;
    char*   defval;
    char*   defaults[DEFAULT_FIELDS];

    if(gv_mo_init == FALSE)
    {
        host = PMhost();
        /*
        ** Save the PM defaults
        */
        for (i=0; i < DEFAULT_FIELDS; i+=1)
        {
            defaults[i] = NULL;
            if ((defval = PMgetDefault( i )) != NULL)
            {
                defaults[i] = STalloc( defval );
            }
        }
        PMsetDefault( 0, SystemCfgPrefix );
        PMsetDefault( 1, host );
        PMsetDefault( 2, ERx("dbms") );
        PMsetDefault( 3, "*" );

        if ((status = MOclassdef( MAXI2, gv_classes )) == OK)
        {
            if((status = MOclassdef( MAXI2, gv_cnf_classes )) == OK)
            {
                for(i=0, count=0, pmexp = cnf[i]; (pmexp != NULL); i+=1, pmexp = cnf[i])
                {
                    MEfill( sizeof(PM_SCAN_REC), 0, &state );
                    for( status = OK; status == OK; count+=1)
                    {
                        if ((status = PMscan( pmexp, &state, NULL, (char**)&pmname, (char**)&pmval )) == OK)
                        {
                            if ((tmp = (CNF_LIST*)MEreqmem(0, sizeof(CNF_LIST), TRUE, NULL)) != NULL)
                            {
                                tmp->cnf.param_name = STalloc(pmname);
                                tmp->cnf.param_val = STalloc((char*)pmval);
                                tmp->cnf.param_len = STlength((char*)pmval);
                                tmp->next = NULL;
                                if (cnfhead != NULL)
                                {
                                    tmp->prev = cnftail;
                                    cnftail->next = tmp;
                                    cnftail = tmp;
                                }
                                else
                                {
                                    cnfhead = tmp;
                                    cnftail = tmp;
                                    tmp->prev = NULL;
                                }
                            }
                            tmp->instance = count;
                            CVla( count, instance );
                            status = MOattach( MO_INSTANCE_VAR, GVcnf_index_name,
                                instance, (PTR)&tmp->cnf );
                            pmexp = NULL;
                        }
                    }
                }
            }
        }
        if ((cnfhead != NULL) &&
            ((status == OK) || (status == PM_NOT_FOUND)))            
        {
            gv_mo_init = TRUE;
        }
        /*
        ** Restore the defaults.
        */
        for (i=0; i < DEFAULT_FIELDS; i+=1)
        {
            PMsetDefault( i, defaults[i] );
            if (defaults[i] != NULL)
            {
                MEfree( defaults[i] );
            }
        }
    }
    return(status);
}

/*{
** Name: GVmo_term
**
** Description:
**      Remove initialized version MIB objects.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Return:
**      None
**
** History:
**      10-Feb-2004 (fanra01)
**          Created.
}*/
VOID
GVmo_term()
{
    CNF_LIST*   tmp;
    char    instance[10];

    if(gv_mo_init == TRUE)
    {
        for (tmp=cnfhead; (tmp != NULL); tmp=cnfhead)
        {
            CVla( tmp->instance, instance );
            if (MOdetach( GVcnf_index_name, instance ) == OK)
            {
                if (tmp->cnf.param_name)
                {
                    MEfree( tmp->cnf.param_name );
                }
                if (tmp->cnf.param_val)
                {
                    MEfree( tmp->cnf.param_val );
                }
            }
            cnfhead = tmp->next;
            if (cnfhead != NULL)
            {
                cnfhead->prev = NULL;
            }
            else
            {
                cnftail = NULL;
            }
            MEfree( (PTR)tmp );
        }
    }
}

/*{
** Name: GVgetverstr
**
** Description:
**      Returns the current version string.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      01-Mar-2004 (fanra01)
**          Created.
}*/
STATUS
GVgetverstr( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    status = MOstrout( MO_VALUE_TRUNCATED, GV_VER, luserbuf, userbuf );
    return(status);
}

/*{
** Name: GVgetenvstr
**
** Description:
**      Returns the current platform string.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      01-Mar-2004 (fanra01)
**          Created.
}*/
STATUS
GVgetenvstr( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    status = MOstrout( MO_VALUE_TRUNCATED, GV_ENV, luserbuf, userbuf );
    return(status);
}

/*{
** Name: GVgetversion
**
** Description:
**      Returns the current build level.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
**      01-Jun-2005 (fanra01)
**          Add return of bldlevel.
}*/
STATUS
GVgetversion( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    ING_VERSION v;
    i4          ival;
    
    if ((status = GVver( offset, &v )) == OK)
    {
        switch(offset)
        {
            case GV_M_MAJOR:
                ival = v.major;
                break;
            case GV_M_MINOR:
                ival = v.minor;
                break;
            case GV_M_GENLVL:
                ival = v.genlevel;
                break;
            case GV_M_BLDLVL:
                ival = v.bldlevel;
                break;
            case GV_M_BYTE:
                ival = v.byte_type;
                break;
            case GV_M_HW:
                ival = v.hardware;
                break;
            case GV_M_OS:
                ival = v.os;
                break;
            case GV_M_BLDINC:
                ival = v.bldinc;
                break;
        }
        status = MOlongout( MO_VALUE_TRUNCATED, ival, luserbuf, userbuf );
    }
    else
    {
        *userbuf = '\0';
    }
    return(status);
}

/*{
** Name: GVgetpatchlvl
**
** Description:
**      Returns the current patch level.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
}*/
STATUS
GVgetpatchlvl( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    
    *userbuf = '\0';
    return(status);
}

/*{
** Name: GVgettcpportaddr
**
** Description:
**      Returns the port address for the TCP protocol.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
**	02-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**	    The portmapper routine takes additional parameter to return the
**	    actual symbolic port matching the numeric port. The actual 
**	    symbolic port info is being ignored here...
**
}*/
STATUS
GVgettcpportaddr( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    char actual_symbolic_port[ GCC_L_PORT + 1 ];
    
    if ((status = GCpportaddr( primary_net, luserbuf, userbuf, 
				actual_symbolic_port )) != OK)
    {
        *userbuf = '\0';
        status = OK;
    }
    return(status);
}

/*{
** Name: GVgetnetportaddr
**
** Description:
**	Returns the secondary net driver address.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
**	02-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**	    The portmapper routine takes additional parameter to return the
**	    actual symbolic port matching the numeric port. The actual 
**	    symbolic port info is being ignored here...
}*/
STATUS
GVgetnetportaddr( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    
    char actual_symbolic_port[ GCC_L_PORT + 1 ];
    if ((status = GCpportaddr( secondary_net, luserbuf, userbuf, 
					actual_symbolic_port  )) != OK)
    {
        *userbuf = '\0';
        status = OK;
    }
    return(status);
}

/*{
** Name: GVgetconformance
**
** Description:
**	Returns the SQL92 conformance.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
}*/
STATUS
GVgetconformance( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    char *confstr = ERx("%s.%s.fixed_prefs.iso_entry_sql-92");
    char  configstr[256];
    char* valuestr = NULL;
    
    STprintf( configstr, confstr, SystemCfgPrefix, PMhost());
    if ((status = PMget( configstr, &valuestr )) == OK)
    {
        STlcopy( valuestr, userbuf,luserbuf );
    }
    return(status);
}

/*{
** Name: GVgetlanguage
**
** Description:
**	Returns the language setting.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
**      09-Mar-2004 (fanra01)
**          Updated to use variable prefix.
}*/
STATUS
GVgetlanguage( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    char        envstr[MAXLINE];
    char  *valuestr = NULL;
    
    STprintf( envstr, ERx("%s_LANGUAGE"), SystemVarPrefix );
    NMgtAt(envstr, &valuestr);
    if (valuestr && *valuestr)
    {
        STlcopy( valuestr, userbuf, luserbuf );
    }
    return(status);
}

/*{
** Name: GVgetcharset
**
** Description:
**	Returns the character set setting
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      29-Jan-2004 (fanra01)
**          Created.
**      09-Mar-2004 (fanra01)
**          Updated to use variable prefix.
}*/
STATUS
GVgetcharset( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    char        envstr[MAXLINE];
    char        idstr[MAXLINE];
    char*       valuestr = NULL;
    
    STprintf( envstr, ERx("%s_INSTALLATION"), SystemVarPrefix );
    NMgtAt( envstr, &valuestr);
    if (valuestr && *valuestr)
    {
        /*
        ** Ensure that the subport is not used in the symbol
        ** generation.
        */
        STlcopy( valuestr, idstr, ID_MAJOR );
        STprintf( envstr, ERx("%s_CHARSET%s"), SystemVarPrefix, idstr );
        NMgtAt(envstr, &valuestr);        
        if (valuestr && *valuestr)
        {
            STlcopy( valuestr, userbuf, luserbuf );
        }
    }
    return(status);
}

/*{
** Name: GVgetinstance
**
** Description:
**	Returns the instance identifier.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      23-Mar-2004 (fanra01)
**          Created.
}*/
STATUS
GVgetinstance( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    char        envstr[MAXLINE];
    char*       valuestr = NULL;
    
    STprintf( envstr, ERx("%s_INSTALLATION"), SystemVarPrefix );
    NMgtAt( envstr, &valuestr);
    if (valuestr && *valuestr)
    {
        STlcopy( valuestr, userbuf, luserbuf );
    }
    return(status);
}

/*{
** Name: GVgetsystempath
**
** Description:
**	Returns the installation path to II_SYSTEM.
**      
** Inputs:
**      offset      unused
**      objsize     unused
**      object      unused
**      luserbuf    length of buffer for return
**      
** Outputs:
**      userbuf     buffer for return value
**
** Return:
**      OK          command completed successfully
**      
** History:
**      01-Jun-2005 (fanra01)
**          Created.
}*/
STATUS
GVgetsystempath( i4  offset,
    i4  objsize,
    PTR object,
    i4  luserbuf,
    char *userbuf )
{
    STATUS      status = OK;
    char        envstr[MAXLINE];
    char*       valuestr = NULL;
    
    STprintf( envstr, ERx("%s_SYSTEM"), SystemVarPrefix );
    NMgtAt( envstr, &valuestr);
    if (valuestr && *valuestr)
    {
        STlcopy( valuestr, userbuf, luserbuf );
    }
    return(status);
}
