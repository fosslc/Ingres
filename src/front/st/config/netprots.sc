/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**
**  Name: netprots.sc -- contains function which controls the Net server
**	protocol configuration form of the INGRES configuration utility.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	21-jun-93 (tyler)
**		Switched to new interface for multiple context PM
**		functions.
**	23-jul-93 (tyler)
**		Changed multiple context PM interfaces again. 
**	04-aug-93 (tyler)
**		Removed Restore menu item.
**	19-oct-93 (tyler)
**		Changed type of global variable "host" to (char *).
**	22-nov-93 (tyler)
**		Implemented SIRS for the Net group.  Removed dead
**		code.  Listen address changes are now include in the
**		configuration change log and this form now supports
**		the ChangeLog menu item.
**	01-dec-93 (tyler)
**		Fixed bad calls to IIUGerr().
**	22-feb-94 (tyler)
**		Fixed BUG 59434: asssign Help menu item to PF2.
**	30-Nov-94 (nanpr01)
**		Fixed BUG 60371: Added frskey mappings so that users can
**		have uniform key board mapping from screen to screen.
**		define frskey11-frskey19
**	07-Feb-96 (rajus01)
**		Added bridge_protocols_form() function for Protocol Bridge.
**	14-Feb-96 (rajus01)
**		Added '==>Vnode' field and additional processing to 
**		bridge_protocols_form.
**	10-oct-1996 (canor01)
**		Replace hard-coded 'ii' prefixes with SystemCfgPrefix.
**	20-dec-1996 (reijo01)
**		Initialize bridge_protocols form with globals 
**		ii_system_name and ii_installation_name
**	21-aug-97 (mcgem01)
**		Correct order of parameters when writing out the port
**		identifier.
**	 1-Oct-98 (gordy)
**		Generalized the net protocols form so that it can also
**		be used for installation registry protocols.
**      17-aug-2001 (wansh01) 
**              Deleted edit_vnode option. Added create vnode and
**              destroy vnode options for bridge configuration. 
**              Add add_bridge_vnode() and protocol_listpick() to
**              handle the create vnode option. 
**      26-Feb-2003 (wansh01) 
**              Added protocol configuration for DAS server. It used
** 		the generialized net protocols form.
**	07-04-2003 (wansh01)
**		Added gcd protocols help support. 
**	04-Oct-2004 (hweho01)
**              Added uf.h include file, the correct prototype
**              declarations for IIUFgtfGetForm and IIUFlcfLocateForm 
**              are needed.
*/

# include <compat.h>
# include <si.h>
# include <ci.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <te.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <pc.h>
# include <pm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
# include "cr.h"
# include "config.h"
# include <uf.h>

exec sql include sqlca;
exec sql include sqlda;

/*
** global variables
*/
exec sql begin declare section;
GLOBALREF char		*host;
GLOBALREF char		*ii_system;
GLOBALREF char		*ii_system_name;
GLOBALREF char		*ii_installation;
GLOBALREF char		*ii_installation_name;
exec sql end declare section;

GLOBALREF LOCATION	config_file;
GLOBALREF PM_CONTEXT	*config_data;
GLOBALREF PM_CONTEXT	*units_data;
GLOBALREF LOCATION	change_log_file;


static bool add_bridge_vnode(char *, char *, char *, char *);
static char *protocol_listpick(char *);
static bool     pm_initialized = FALSE;
/*
** local forms
*/
exec sql begin declare section;
static char *net_protocols = ERx( "net_protocols" );
static char *bridge_protocols = ERx( "bridge_protocols" );
static char *bridge_addvnode = ERx( "bridge_addvnode" );

exec sql end declare section;

bool
net_protocols_form( char *net_id, char *instance )
{
	exec sql begin declare section;
	char *def_name;
	char in_buf[ BIG_ENOUGH ];
	char *name, *value, *title;
	char expbuf[ BIG_ENOUGH ];
	char description[ BIG_ENOUGH ];
	exec sql end declare section;

	STATUS status;
	PM_SCAN_REC state;
	bool changed = FALSE;
	char *regexp, *p;
	i4 len, i;
	char request[ BIG_ENOUGH ]; 
	char temp_buf[ BIG_ENOUGH ];

	if (IIUFgtfGetForm( IIUFlcfLocateForm(), net_protocols ) != OK)
 	 IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
			net_protocols );


	if ( !STcompare( net_id, ERx( "gcb" ) ) )
	{
	   title = STalloc( ERget( S_ST041D_configure_bridge_prot));
	   STcopy( ERget( S_ST041E_bridge_protocols_parm),description);
	}
        else
	if ( !STcompare( net_id, ERx( "gcd" ) ) )
	{
	   title = STalloc( ERget( S_ST069C_configure_das_prot));
	   STcopy( ERget( S_ST069D_das_protocols_parm),description);
	}
	else
	{
	   title = STalloc( ERget( S_ST044C_configure_net_prot) );
	   if ( !STcompare( net_id, ERx( "gcc" ) ) )
	      STcopy ( ERget( S_ST044D_net_protocols_parm),description);
           else
	      STcopy ( ERget( S_ST044E_registry_proto_parm),description);
	}

        /* center description (62 characters wide) */
	STcopy( description, temp_buf );
	len = (62 - STlength( temp_buf )) / 2;
	for( i = 0, p = description; i < len; i++,
			CMnext( p ) )
 	    STcopy( ERx( " " ), p );
	STcopy( temp_buf, p );

	if ( ! instance )  
	    def_name = STalloc( net_id );
	else  if ( ! STcompare( instance, ERx( "*" ) ) )
	    def_name = STalloc( ERget( S_ST030F_default_settings ) );
	else
	    def_name = STalloc( instance );

	exec frs inittable net_protocols protocols read;

	exec frs display net_protocols;

	exec frs initialize (
		host = :host,
		ii_installation = :ii_installation,
		ii_installation_name = :ii_installation_name,
		ii_system = :ii_system,
		ii_system_name = :ii_system_name,
		def_name = :def_name, 
		title    = :title,
		description     = :description  
	);

	exec frs begin;

	/* construct expbuf for gcc port id scan */
	if ( ! instance )
	    STprintf( expbuf, ERx( "%s.%s.%s.%%.%%" ), 
		      SystemCfgPrefix, host, net_id );
	else
	    STprintf( expbuf, ERx( "%s.%s.%s.%s.%%.%%" ), 
		      SystemCfgPrefix, host, net_id, instance );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		exec sql begin declare section;
		char *protocol;
		char *param;
		char *units;
		exec sql end declare section;

		/* extract protocol name */
		protocol = PMmGetElem( config_data, instance ? 4 : 3, name ); 

		/* extract parameter name */
		param = PMmGetElem( config_data, instance ? 5 : 4, name );

		/* get units */
		if ( PMmGet( units_data, name, &units ) != OK )
		    units = ERx( "" );

		exec frs loadtable net_protocols protocols (
			protocol = :protocol,
			name = :param,
			value = :value,
			units = :units
		);
	}

	exec frs end;

	exec frs activate menuitem :ERget( FE_Edit ),frskey12;
	exec frs begin;
		exec sql begin declare section;
		char protocol_id[ BIG_ENOUGH ];
		char parm[ BIG_ENOUGH ];
		exec sql end declare section;

		exec frs getrow net_protocols protocols (
			:protocol_id = protocol,
			:parm = name
		);

		if ( ! instance )
		    STprintf( request, ERx( "%s.%s.%s.%s.%s" ), 
			      SystemCfgPrefix, host, net_id, 
			      protocol_id, parm );
		else 
		    STprintf( request, ERx( "%s.%s.%s.%s.%s.%s" ), 
			      SystemCfgPrefix, host, net_id, 
			      instance, protocol_id, parm );

		if ( CRtype( CRidxFromPMsym( request ) ) == CR_BOOLEAN )
		{
		    exec frs getrow net_protocols protocols (
			:in_buf = value
		    );

		    if ( ValueIsBoolTrue( in_buf ) )
			STcopy( ERx( "OFF" ), in_buf );
		    else
			STcopy( ERx( "ON" ), in_buf );
		}
		else
		{
		    exec frs prompt (
			    :ERget( S_ST0328_value_prompt ),
			    :in_buf
		    ) with style = popup;
		}

		if ( set_resource( request, in_buf ) )
		{
		    changed = TRUE;

		    exec frs putrow net_protocols protocols (
			value = :in_buf
		    );
		}

	exec frs end;

	exec frs activate menuitem :ERget( S_ST035F_change_log_menu ),frskey18;
	exec frs begin;
		if( browse_file( &change_log_file,
			ERget( S_ST0347_change_log_title ), TRUE ) != OK )
		{
			exec frs message :ERget( S_ST0348_no_change_log )
				with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
             if ( !STcompare( net_id, ERx( "gcb" ) ) )
	     {
		if( !get_help( ERx( "gcb.protocols" ) ) )
			exec frs message 'No help available.'
				with style = popup;
             }
	     else 
             if ( !STcompare( net_id, ERx( "gcd" ) ) )
	     {
		if( !get_help( ERx( "gcd.protocols" ) ) )
			exec frs message 'No help available.'
				with style = popup;
             }
	     else 
	     {
		if( !get_help( ERx( "gcc.protocols" ) ) )
			exec frs message 'No help available.'
				with style = popup;
             }
	exec frs end;

	exec frs activate menuitem :ERget( FE_End ), frskey3;
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;

	MEfree( def_name );
	MEfree( title );
	return( changed );
}

bool
bridge_protocols_form( char *net_id )
{
	exec sql begin declare section;
	char *def_name;
	char in_buf[ BIG_ENOUGH ];
	char tmp_buf[ BIG_ENOUGH ];
	char *name, *value;
	char expbuf[ BIG_ENOUGH ];
	char protocol_id[ BIG_ENOUGH ], *protocol_status;
	char vnode_name[BIG_ENOUGH];
	char port_name[BIG_ENOUGH];
	exec sql end declare section;

	LOCATION *IIUFlcfLocateForm();
	STATUS IIUFgtfGetForm();
	STATUS status;
	PM_SCAN_REC state;
	bool changed = FALSE;
	char *regexp;
	char request[ BIG_ENOUGH ]; 
	i4   protocols_rows = 0;    /* # of protocols in the protocol table  */

	if( IIUFgtfGetForm( IIUFlcfLocateForm(), bridge_protocols ) != OK )
	{
		IIUGerr( S_ST031A_form_not_found, UG_ERR_FATAL, 1,
			bridge_protocols );
	}

	if( STcompare( net_id, ERx( "*" ) ) == 0 )
		def_name = STalloc( ERget( S_ST030F_default_settings ) );
	else
		def_name = STalloc( net_id );

	exec frs inittable bridge_protocols protocols read;

	exec frs display bridge_protocols;

	exec frs initialize (
		host = :host,
		ii_installation = :ii_installation,
		ii_installation_name = :ii_installation_name,
		ii_system = :ii_system,
		ii_system_name = :ii_system_name,
		def_name = :def_name
	);

	exec frs begin;


	/* construct expbuf for gcb port id scan */
	STprintf( expbuf, ERx( "%s.%s.gcb.%s.%%.port.%%" ), 
		  SystemCfgPrefix, host, net_id );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		exec sql begin declare section;
		char *vnode;
		char *protocol;
		exec sql end declare section;

		/* extract protocol name */
		protocol = PMmGetElem( config_data, 4, name ); 

		/* extract vnode name */
		if( PMnumElem( name ) == 7 )
		   vnode = PMmGetElem( config_data, 6, name );

		/* get status of scanned protocol and vnode*/ 
		STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ),
			  SystemCfgPrefix, host, net_id, protocol,vnode );

		if( PMmGet( config_data, request, &protocol_status )
			!= OK)	
                {
		    /* default to the status of scanned protocol */ 
		   STprintf( request, ERx( "%s.%s.gcb.%s.%s.status" ),
			  SystemCfgPrefix, host, net_id, protocol );

		   if( PMmGet( config_data, request, &protocol_status ))
			continue;
                }

		exec frs loadtable bridge_protocols protocols (
			name = :protocol,
			port = :value,
			status = :protocol_status,
			vnode = :vnode
		);
                protocols_rows++;

	}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST031E_edit_status_menu ),frskey12;
	exec frs begin;
		char request[ BIG_ENOUGH ]; 

		/* get protocol id */
		exec frs getrow bridge_protocols protocols (
			:protocol_id = name, 
			:vnode_name = vnode,
			:in_buf = status
		);


	 	    if ( ValueIsBoolTrue( in_buf ) )
			STcopy( ERx( "OFF" ), in_buf );
		    else
			STcopy( ERx( "ON" ), in_buf );

		/* compose resource request for insertion */
		STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ),
			  SystemCfgPrefix, host, net_id, protocol_id,
			  vnode_name);

		if( set_resource( request, in_buf ) )
		{
			/* update tablefield */
			exec frs putrow bridge_protocols protocols (
				status = :in_buf
			);
			changed = TRUE;
		}
	exec frs end;

	exec frs activate menuitem :ERget( S_ST041B_del_vnode_menu ),frskey13;
	exec frs begin;
		if (protocols_rows >0) 
		{
		/* get protocol id */
		exec frs getrow bridge_protocols protocols (
			:protocol_id = name,
			:vnode_name = vnode,
			:port_name = port
		);

		/* compose resource request for deletion */
		STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), 
			      SystemCfgPrefix, host,
			      net_id, protocol_id , vnode_name);
	        set_resource( request, NULL ); 

		STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ), 
			      SystemCfgPrefix, host,
			      net_id, protocol_id , vnode_name);
	        set_resource( request, NULL ); 

		/* update tablefield */

		exec frs deleterow bridge_protocols protocols;

		protocols_rows--;
		changed = TRUE;
		}


	exec frs end;

	exec frs activate menuitem :ERget(S_ST041A_add_vnode_menu ),frskey19;
            
	exec frs begin;
	     if (add_bridge_vnode(net_id,vnode_name,port_name,protocol_id))
	     {
	       /* compose resource request for insertion */
	       STprintf( request, ERx( "%s.%s.gcb.%s.%s.port.%s" ), 
		      SystemCfgPrefix, host, net_id, protocol_id, 
		      vnode_name );

	       set_resource( request, port_name  ); 

        	/*  default the status to ON  */ 	
	       STprintf( request, ERx( "%s.%s.gcb.%s.%s.status.%s" ), 
		      SystemCfgPrefix, host, net_id, protocol_id, 
		      vnode_name );
	       set_resource( request, ERx( "ON" ) ); 

		   /* insert the added nodes on top of the table */
	       exec frs insertrow bridge_protocols protocols  0 (
			    port = :port_name,
			    status = :ERx( "ON" ),
			    name = :protocol_id,
			    vnode = :vnode_name);

		
	       protocols_rows++; 
	       changed = TRUE;
             }
	exec frs end;

	exec frs activate menuitem :ERget( S_ST035F_change_log_menu ),frskey18;
	exec frs begin;
		if( browse_file( &change_log_file,
			ERget( S_ST0347_change_log_title ), TRUE ) != OK )
		{
			exec frs message :ERget( S_ST0348_no_change_log )
				with style = popup;
		}
	exec frs end;

	exec frs activate menuitem :ERget( FE_Help ), frskey1;
	exec frs begin;
		if( !get_help( ERx( "gcb.protocols" ) ) )
			exec frs message 'No help available.'
				with style = popup;
	exec frs end;

	exec frs activate menuitem :ERget( FE_End ), frskey3;
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;

    exec frs activate before column protocols all;

    exec frs begin;
	if (protocols_rows == 0 )
	{
	 exec frs set_frs menu '' (active(:ERget(S_ST041B_del_vnode_menu))=0);
	 exec frs set_frs menu '' (active(:ERget(S_ST031E_edit_status_menu))=0);
	 exec frs set_frs menu '' (active(:ERget(S_ST041A_add_vnode_menu))=1);
	}
	else
	{
	 exec frs set_frs menu '' (active(:ERget(S_ST041B_del_vnode_menu))=1);
	 exec frs set_frs menu '' (active(:ERget(S_ST031E_edit_status_menu))=1);
	 exec frs set_frs menu '' (active(:ERget(S_ST041A_add_vnode_menu))=1);
	}
	exec frs resume next;
    exec frs end;

	MEfree( def_name );
	return( changed );
}


/*
**  add_bridge_vnode  
**  output:
**
**    protocol -- the protocol name.  
**    port  -- the listen address. Not edited.  
**    vnode -- the virtual node name.  Not edited.
**
**  Returns:
**
**    TRUE -- Add ok 
**    FALSE -- Add error/cancel
** History:
**	10-Aug-2001 (wansh01) 
**	   created 
*/


bool  add_bridge_vnode(char *net_id, char *addvnode, char *addport, char *addprotocol) 
{ 
    bool  rtn; 

    /* Make sure the sizes of these variables match form definitions */
    exec sql begin declare section; 
    char fldname[64];
    char vnode[BIG_ENOUGH]; 
    char port[BIG_ENOUGH]; 
    char protocol[BIG_ENOUGH]; 
    bool changed;
    exec sql end declare section;
    char msg[200];
    char                *pm_val;
    char                pmsym[128];


    if (IIUFgtfGetForm(IIUFlcfLocateForm(), bridge_addvnode) != OK)
        PCexit(FAIL);

    /* ...and display it.  Top of display loop. */
    
    exec frs display :bridge_addvnode with style=popup;

    exec frs initialize;
    exec frs begin;

        exec frs set_frs frs 
            ( validate(keys)=0, validate(menu)=0, validate(menuitem)=0,
              validate(nextfield)=0, validate(previousfield)=0 ); 
        exec frs set_frs frs 
            ( activate(keys)=1, activate(menu)=1, activate(menuitem)=1,
              activate(nextfield)=1, activate(previousfield)=0 ); 

	exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices)) = 0);
    
    
    exec frs end;

    exec frs activate menuitem :ERget(FE_Save), frskey4;
    exec frs begin;

        exec frs inquire_frs form (:changed = change(:bridge_addvnode));
	if (!changed)
        { 
	    addvnode=addport=addprotocol=EOS;
	    rtn = FALSE;
	    exec frs breakdisplay;
	}

        exec frs getform :bridge_addvnode 
	    ( :protocol = protocol, 
	      :port = port, 
	      :vnode = vnode );


	
	if (*protocol == EOS) 
	{
            IIUGerr(S_ST031F_edit_protocol_prompt,0,0);
	    exec frs resume field protocol;
        } 
	else
	{
         if (!pm_initialized)
         {
	     PMinit();
	     if (PMload(NULL, (PM_ERR_FUNC *)NULL) == OK)
		 pm_initialized = TRUE;
         }

	 STprintf( pmsym, ERx( "%s.%s.gcb.%s.%s.status" ), 
		  SystemCfgPrefix, host, net_id, protocol );
         if ( PMget( pmsym, &pm_val ) != OK ) 
         {
            IIUGerr(S_ST031F_edit_protocol_prompt,0,0);
	    exec frs resume field protocol;
	 }
        }

	if (*port == EOS)  
	{ 
	    IIUGerr(E_ST000E_listen_req,0,0);
	    exec frs resume field port;
	} 

	if (*vnode == EOS)  
	{
            IIUGerr(S_ST0530_vnode_prompt,0,0);
	    exec frs resume field vnode;
	} 

        STcopy(vnode, addvnode);
        STcopy(port, addport);
        STcopy(protocol,addprotocol);

        rtn = TRUE; 
	exec frs breakdisplay; 
    exec frs end;

    exec frs activate menuitem :ERget(FE_Cancel), frskey9 (activate=0);
    exec frs begin;
        rtn = FALSE;
        exec frs breakdisplay;
    exec frs end;

    exec frs activate menuitem :ERget(F_ST0001_list_choices),frskey13 
				(activate =0);    
				 
    exec frs begin;
	exec sql begin declare section;
	char *prot;
	exec sql end declare section;

	if (NULL != (prot = protocol_listpick( net_id )))
	{
	    exec frs putform :bridge_addvnode (protocol=:prot);
	    exec frs set_frs form (change(:bridge_addvnode) = 1);
	    exec frs set_frs field :bridge_addvnode (change(protocol) = 1);
	    exec frs redisplay;
	}

    exec frs end;

    exec frs activate menuitem :ERget(FE_Help), frskey1;
    exec frs begin;
	if( !get_help( ERx( "gcb.crtvnodes" ) ) )
		exec frs message 'No help available.'
			with style = popup;
    exec frs end;

    exec frs activate before field all;
    exec frs begin;
        exec frs inquire_frs form (:fldname = field);
	if (STcompare(fldname, ERx("protocol")) == 0) 
	{
	    exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices))=1);
	}
	else
	{
	    exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices))=0); 
	}
	exec frs resume next;
    exec frs end;


    exec frs finalize;
    return rtn;
}

/*
**  protocol_listpick() -- Gets a ListChoices for possible protocols
**
**  Returns a character string that contains the chosen item out of a
**  a list of possible protocols.
**  use PMmscan to find protocols that are already defined in the
**  config.dat 
**  <prefix>.<host>.gcb.<net_id>.<protocols>.status
**
**  Inputs:
**
**    net_id  -- net_id so it can scan the config.dat  
**
**  Returns:
**
**    A Pointer to a character string representing a protocal.
**
*/


static char *
protocol_listpick(char * net_id)
{
    i4  lpx;
    static char lpstr[256];
    char *sp, *rtn;
    char *protocol;
    char *regexp;
    STATUS status;
    PM_SCAN_REC state;
    char *name, *value;
    char expbuf[ BIG_ENOUGH ];

    *lpstr = EOS;
	/* construct expbuf for gcb port id scan */
	STprintf( expbuf, ERx( "%s.%s.gcb.%s.%%.status" ), 
		  SystemCfgPrefix, host, net_id );

	regexp = PMmExpToRegExp( config_data, expbuf );

	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{

		/* extract protocol name */
		protocol = PMmGetElem( config_data, 4, name ); 

                STcat(lpstr, protocol);
                STcat(lpstr, ERx("\n"));
        }

     if (*lpstr == EOS)
       return NULL;

    /* Display the ListChoices for selection */
    lpx = IIFDlpListPick( ERget(S_ST0041_protocol_prompt), lpstr, 
			  0, -1, -1, NULL, NULL, NULL, NULL );
    
    /* If nothing was picked just return an empty string */
    if (lpx < 0)
	return NULL;

    /* Find the selection in the delimitted character string */
    for (rtn = lpstr; lpx; lpx--)
    {
	rtn = STindex(rtn, ERx("\n"), 0);
	if (rtn == NULL)
	    return NULL;
	CMnext(rtn);
    }

    /* return the results of the string search */
    if (NULL != (sp = STindex(rtn, ERx("\n"), 0)))
	*sp = EOS;
    return rtn;
}
