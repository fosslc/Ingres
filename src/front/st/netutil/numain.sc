/*
** Copyright (c) 1992, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<si.h>
#include	<st.h>
#include	<me.h>
#include	<gc.h>
#include	<ex.h>
#include        <er.h>
#include	<cm.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include        <fe.h>
#include        <ug.h>
#include        <uf.h>
#include        <ui.h>
#include        <pc.h>
#include        <id.h>
#include	<iplist.h>
#include	<erst.h>
#include        <uigdata.h>
#include	<stdprmpt.h>
#include	<gca.h>
#include	<gcn.h>
#include	"nu.h"

/*
**  Name: netutil -- network configuration maintenance program
**
**  Description:
**	This utility provides users a facility for maintaining connection
**	and authorization and other attribute information to be used in
**	accessing Ingres servers via Ingres/NET.
**	Netutil may be run in one of two modes: forms-based interactive,
**	or non-interactively with one or more control files.  If the -file
**	command-line switch is present, netutil will operate non-interactively.
**
**  Entry points:
**	This file contains no externally visible functions
**
**  History:
**	30-jun-92 (jonb)
**	    Created.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      18-feb-93 (jonb)
**	    Added GLOBALREFs.
**	29-mar-93 (jonb)
**	    Added comments and cleaned up a few things.
**      02-sep-93 (joplin)
**          Added support for GCN_UID_FLAG request flag, fixed lost GCA_ERRORs,
**          got rid of "superuser" detection code, invoked nv_test_host() 
**          directly.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	12-may-94 (mikehan)
**	    Increased the size of etxt[] in test_connection() so that it
**	    is large enough to hold E_ST0035_connection_failed + the CL
**	    error message returned by ERreport().
**	15-jun-95 (emmag)
**	    Redefined main to ii_user_main and h_addr to ii_h_addr on NT.
**	08-aug-95 (reijo01)
**	    Changed the default protocol if platform is DESKTOP.
**	22-aug-95 (tutto01)
**	    Bring up the forms system a bit earlier, so that error reporting
**	    functions properly in all instances on Windows NT.
**	24-sep-95 (tutto01)
**	    Reverse previous change, and restore main, as netutil will now
**	    run as a console app on NT.
**	05-Dec-1995 (walro03/mosjo01)
**	    Positioned #ifdef and related compiler directives in column 1 to fix
**	    compile error on DG/UX.
**      03-jan-96 (emmag)
**          Calling netutil with an invalid argument causes a GPF in NT
**          as we haven't yet EXdeclare'ed the exception handler which
**          we attempt to EXdelete on exit.
**	07-Feb-96 (rajus01)
**	    Changed shutdown_comsvr(), display_comsvr_list() names to
**	    shutdown_svr(), display_svr_list() respectively. Now the 
**	    Control menu item of NETUTIL interface displays not only
**	    comm server, but also bridge server, if you have one running
**	    on host.
**	    Modified display_svr_list() to display bridge servers also.
**	10-apr-96 (chech02)
**	    Added function prototypes for win 3.1 port.
**	13-May-96 (gordy)
**	    Don't mix prototype on old style declarations.
**	15-jul-96 (thoda04)
**	    Added function prototypes for win 3.1 port.
**	26-feb-97 (mcgem01)
**	    Reintroduce NT change to ensure that exception handler has
**	    been declared before attempting to remove it upon hitting
**	    an error in the argument list.
**	21-Aug-97 (rajus01)
**	    Added support for specifying security mechanisms for interactive
**	    and non-interactive mode operations of netutil.
**	    Renamed "numainform" to "loginform". Added create_attr(),
**	    edit_attr(), destroy_attr(), attribute_frame(), 
**	    display_detail_attr() routines.
**	    Major work is done on redesign of the forms. The forms used by
**	    netutil are: loginform, attribform, logpassedit, connedit,
**	    attribedit, nusvrcntrl.
**	    The forms that will not be used any more are: numainform, logpass,
**	    getpass, nucmsvr. Added *** No longer Used ** comments to these
**	    forms.
**	    Other shortcomings in the old netutil are rectified:
**	    a. Giving the user ablity to provide choice to specify Private
**	       or Global in login/password edit.
**	    b. Cancel on "Choose type of Vnode" should not pop up other
**	       windows.
**	    c. If individual entries on the table is destroyed, and if 
**	       no other entries are there for the vnode, then the vnode
**	       must be removed from the nodetable.
**	28-aug-97 (hanch04)
**	    Change S_ST0020_choose_login_type to S_ST0020_choose_vnode_type
**	24-Sep-97( rajus01 )
**	    If vnode table is empty, avoid the error message being displayed
**	    when switching between Attibute and Login screens. Added the
**	    check 'if (vnode_rows > 0)'.
**	14-oct-97 (funka01)
**	    Change hard-coded Netutil messages to depend on SystemNetutilName.
**          Modified to use correct form for JASMINE/INGRES.
**	23-Jan-98 (rajus01)
**	    Made Private/Global field in the login table display only
**	   (logpassedit.frm). Fixed problems with display of login type,
**	   connection type, attribute type. Added call_from_vw, original_type
**	   global variables.
**	26-Jan-98 (rajus01)
**	   If individual entries on the table is destroyed, and if no other
**	   entries are there for the vnode then the vnode must be removed 
**	   from the nodetable. During this operation, the FE_Create, FE_Edit,
**	   FE_destroy  menu items are deactivated.
**	02-Mar-99 (matbe01)
**	   Added casting of (ATTR *) to nu_locate_attr to eliminate compile
**	   error on AIX (rs4_us5).
**	13-jun-2000 (somsa01)
**	    Updated MING hint to set up program name according to the product.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Mar-02 (gordy)
**	    Used GCN class definitions rather than GCN.
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. Added NEEDLIBSW hint which is 
**	    used for compiling on windows and compliments NEEDLIBS.
**	22-Sep-2008 (lunbr01) Sir 119985
**	    Change the default protocol from "wintcp" to "tcp_ip" for
**	    Windows (32-bit).
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of id.h to eliminate gcc 4.3 warnings.
*/

/*
PROGRAM = (PROG0PRFX)netutil
NEEDLIBS = NETUTILLIB INSTALLLIB COMPATLIB SHQLIB SHFRAMELIB 
NEEDLIBSW = SHGCFLIB SHEMBEDLIB 

UNDEFS =        II_copyright
*/
 
exec sql include sqlca;
exec sql include sqlda;
 
exec sql begin declare section;
static i4  nodet_row = 1;	/* Which row of the vnode table are we on? */
static i4  connt_row = 1;	/* Which row of the connect table are we on? */
static i4  autht_row = 1;	/* Which row of the login table are we on? */
static i4  attrib_row = 1;	/* Which row of the attribute table are we on?*/
static i4  whichtable;		/* Which table contains the cursor? */
GLOBALDEF char username[MAX_TXT]; /* Name of current user whom is maintained */
GLOBALDEF char *hostname = NULL;  /* Name of the host whose database we're
				     maintaining. */
static char local_host[MAX_TXT];	/* Name of the local host */
static char local_username[MAX_TXT]; /* Name of local user executing netutil */
static char *control_file_list = NULL;   /* List of control file names for
					    non-interactive mode. */
GLOBALDEF char private_word[MAX_TXT]; 		/* The word "Private" */
GLOBALDEF char global_word[MAX_TXT];		/* The word "Global" */
GLOBALDEF bool edit_global;			/* vnode type */
GLOBALDEF bool cancel_on_vnodetype;        	/* cancel vnode type*/
GLOBALDEF char *def_protocol;   /* Name of the default protocol to use */
static    char topFormName[ MAX_TXT ];
static    char controlFormName[ MAX_TXT ];
static char *control_menuitem;
static i4  nodet_offset = 0;
static i4  call_from_vw = 0;			/* Create from vnode window */
static bool original_type;			/* Original vnode type */
exec sql end declare section;

static i4  auth_rows;		/* Number of rows in the login table */
static i4  conn_rows;		/* Number of rows in the connect table */
static i4  vnode_rows;		/* Number of rows in the vnode table */
static i4  attrib_rows = 0;	/* Number of rows in the attribute table */

/*
**  Local functions...
*/

static VOID	top_frame();
static VOID	control_frame();
static VOID	attribute_frame();
static i4	adjust_control_menu();
static bool	shutdown_svr(bool, char *);
static VOID	display_detail(char *, char * );
static VOID	display_detail_auth(char *, char *);
static VOID	display_detail_conn(char *, char *);
static VOID	display_detail_attr(char *, char *);
static VOID	get_node_name(char *, char *, i4);
static VOID	display_vnode_list(char *, char *);
static STATUS	display_svr_list();
static STATUS  	create_auth(char *vnode, bool new);
static STATUS  	create_conn(char *);
static STATUS  	create_attr(char *);
static STATUS	edit_auth(char *, char *, char *);
static STATUS	edit_conn(char *, char *, char *, char *, char *);
static STATUS	edit_attr(char *, char *, char *, char *);
static STATUS	edit_vnode(char *, char *);
static STATUS	destroy_vnode(char *, char * );
static STATUS	destroy_auth(char *, char *, char *);
static STATUS	destroy_conn(char *, char *, char *, char *, char *);
static STATUS	destroy_attr(char *, char *, char *, char *);
static bool	enough_rows();
static VOID	test_connection(char *);
static STATUS	nu_getargs(i4, char *[]);


# ifdef NT_GENERIC
# define h_addr ii_h_addr
# endif

/*
**  External functions...
*/
i4  nu_connedit(bool, char *cnode, bool *cpriv, 
                char *caddr, char *clist, char *cprot);


GLOBALDEF bool is_setuid = FALSE;     /* Are we impersonating another user? */

main(i4 argc, char *argv[])
{
    EX_CONTEXT	context;
    EX		FEhandler();
    STATUS      rtn;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC );  /* We'll use the Ingres malloc library */

    if ( EXdeclare(FEhandler, &context) != OK  ||  /* Any failure here... */
	 nu_getargs(argc,argv) == FAIL)		   /* ... or here ... */
    {
        EXdelete();				/* ... we're just out of luck */
        PCexit(FAIL);
    }

/*
**  Connect to the database and initialize internal data structures...
*/
    if (OK != (rtn = nu_data_init(hostname)))	
    {
	IIUGerr(rtn, 0, 0);
	if (hostname == NULL)
	    IIUGerr(E_ST003E_cant_connect, UG_ERR_FATAL, 1, local_host);
	else
	    IIUGerr(E_ST001F_cant_connect, UG_ERR_FATAL, 1, hostname);
    }


    STcopy(ERget(S_ST0003_global), global_word);   /* Just for convenience */
    STcopy(ERget(S_ST0004_private), private_word);

    if ( control_file_list != NULL )   /* Are we using control file(s)? */
    {
	char *filename, *next;
	for (filename = control_file_list; filename != NULL; filename = next)
	{
	    next = STindex(filename,ERx(","),0);
	    if (next != NULL)
	    {
		*next = EOS;
		CMnext(next);
	    }
	    if (*filename != EOS)
	        nu_file(filename);    /* Pass each filename to nu_file() */
	}
	EXdelete();
	PCexit(OK);		      /* That's all she wrote */
    }
/*
**  We'll be running in interactive mode; fire up the forms system...
*/

    if (FEforms() != OK)
    {
        SIflush(stderr);
        PCexit(FAIL);
    }

    /* Disable potentially dangerous features, and annoying "Out of Data" */
    exec frs set_frs frs ( 
        shell = 0,
        editor = 0,
        outofdatamessage = 0 );

    top_frame();    /* Main processing for interactive mode */
	 
    FEendforms();
#ifdef WIN16
    IIGCn_call(GCN_TERMINATE, NULL);
#endif
    EXdelete();
    PCexit(OK);
}

/*
**  top_frame()
**
**  Top-level processing for the main form.  (This was prototyped in ABF;
**  hence "frame".)  The form consists of three individual tablefields:
**  the master or "vnode" table, which is a list of all vnode names that 
**  have any information defined against them, and two detail fields, the 
**  "login" (a.k.a. authorization) and "connection" tables.  As the cursor
**  is moved through the master table, the detail tables are updated 
**  automatically to contain the information for the vnode name under the
**  cursor.  
**
**  Menu items apply to whichever field the cursor is located in when
**  the item is selected.  The functionality breaks down as follows:
**
**    Menu item    Tablefield    Function
**    --------------------------------------------------------------------
**    Create       Vnode         Get new virtual node name, bring up popup
**                               forms for login and connection entries.
**
**                 Login         Create new login entry for current vnode.  
**
**                 Connection    Create new connection entry for vnode.
**
**    Edit         Vnode         Rename the current vnode.  Login and
**                               connection data remains unchanged.
**
**                 Login         Change login name and/or password.
**
**                 Connection    Change any data item in current entry.
**
**    Destroy      Vnode         Destroy all entries, login and connection,
**                               for the current vnode name.
**
**                 Login         Destroy the single login entry under the
**                               cursor.
**
**                 Connection    Destroy the single connection entry under
**                               the cursor.
**
**  Other menu items:
**
**    Test:  Attempts to connect to the name server on the current vnode.
**           Intended to be a quick-and-dirty test of whether the information
**           is correct for the vnode.
**
**    Control:    (network admin) Brings up a screen that lets the user
**                quiesce or stop selected comm servers or bridge servers.
**
**    Attributes: (Other attributes info for the current vnode) Brings
**		  up a screen displaying the connection and other attributes
**		  info on the current vnode. The new screen will have all the
**		  menu items above and Login menuitem which enables the user
**		  to look up the login information of the vnode under cursor.
**
**    Help, Quit:  What you'd expect.
*/

static VOID
top_frame()
{
    exec sql begin declare section;
    char new_vnode[MAX_TXT];
    char vnode[MAX_TXT];
    char privglob[MAX_TXT];
    char login[MAX_TXT];
    char type[MAX_TXT];
    char prot[MAX_TXT];
    char list[MAX_TXT];
    char addr[MAX_TXT];
    char emptymsg[3];
    char *userinfo_text;
    i4   table_rows;
    STATUS estatus;
    char fldname[64];
    exec sql end declare section;
    
    *vnode = EOS;
    control_menuitem = ERget(F_ST0042_control_menuitem);

/*
**  Display main form...
*/
    STcopy(ERx("loginform"), topFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), topFormName) != OK)
        PCexit(FAIL);

/*
**  Fill in master table...
*/
    display_vnode_list(NULL, topFormName);

/*
**  Beginning of display loop.
*/
    exec frs display :topFormName;

    exec frs initialize;
    exec frs begin;
	{
	    exec sql begin declare section;
	    char userinfo_txt[MAX_TXT];
	    exec sql end declare section;
	    STprintf(userinfo_txt, "%s: %s", 
		ERget(S_ST0037_user_tag), username);
            exec frs putform :topFormName (userinfo=:userinfo_txt);
	    if (hostname == NULL)
	    {
	        exec frs set_frs field :topFormName 
		    (invisible(remote_host_name)=1);
	    }
	    else
	    {
		exec frs putform :topFormName (remote_host_name = :hostname);
	        exec frs set_frs field :topFormName 
		    (invisible(remote_host_name)=0);
	    }
	}
    exec frs end;

    exec frs activate menuitem :ERget(FE_Create);
    exec frs begin;
        switch (whichtable)
        {
            case NODE_TBL: 
		cancel_on_vnodetype = 0;
                get_node_name(NULL,new_vnode,sizeof(new_vnode));
                if (*new_vnode != EOS)
                {
		    call_from_vw = 1;
                    estatus = create_auth(new_vnode, TRUE);
		    if( cancel_on_vnodetype )
			break;

		     /* Keep the original vnode type, no matter
		     ** what the login type is changed to. 
		     */
		     if( original_type )
		        edit_global = FALSE;
		     else
		        edit_global = TRUE;
                    if( OK == create_conn(new_vnode) || OK == estatus )
                    {
                    	exec frs clear field nodetable; 
                        display_vnode_list(new_vnode, topFormName);  
                    }
		    call_from_vw = 0;
                }
                break;
            case CONN_TBL:
	    	if (vnode_rows > 0)
		{
		    if (OK == create_conn(vnode))  
			display_detail_conn(vnode, topFormName);
		}
		else
		{
		    IIUGerr(E_ST0018_no_vnode_rows, 0, 0);
		}
                break;
            case AUTH_TBL:
	    	if (vnode_rows > 0)
		{
		    if (OK == create_auth(vnode, FALSE))
			display_detail_auth(vnode, topFormName);
		}
		else
		{
		    IIUGerr(E_ST0018_no_vnode_rows, 0, 0);
		}
                break;
        }
    exec frs end;

    exec frs activate menuitem :ERget(FE_Destroy);
    exec frs begin;
        if (enough_rows())   
            switch (whichtable)
            {
                case NODE_TBL:
		    if (OK == destroy_vnode( vnode, topFormName ) ) 
		    {
			if (nodet_row >= vnode_rows)
			    nodet_row--;
                        display_vnode_list(NULL, topFormName);
                    }
                    break;
                case CONN_TBL:
                    exec frs getrow :topFormName conntable
                        (:type=connt_type, :prot=connt_protocol, 
                         :list=connt_listen, :addr=connt_address);
                    if ( NULL == nu_locate_conn(vnode, nu_is_private(type), 
                                                addr, list, prot, NULL) )
		    {
                        IIUGerr(E_ST0017_no_conn,0,0);
		    }
		    else if (OK == destroy_conn(vnode,type,addr,list,prot))
                    {
			if (connt_row >= conn_rows)
			    connt_row--;
                        display_detail_conn(vnode, topFormName);
                    }
                    break;
                case AUTH_TBL:
                    exec frs getrow :topFormName authtable 
                        (:privglob=type, :login=login);
                    if ( OK == destroy_auth(vnode,privglob,login) )
                    {
			autht_row = 1;
                        display_detail_auth(vnode, topFormName );
                    }
                    break;
            }
	    if( auth_rows == 0 && conn_rows == 0 && attrib_rows == 0)
	    {
		nu_destroy_vnode( vnode );
                display_vnode_list(NULL, topFormName);

		/* Deactivate the following menu items during this operation */
	        exec frs set_frs menu '' (active(:ERget(FE_Create))=0);
	        exec frs set_frs menu '' (active(:ERget(FE_Edit))=0);
	        exec frs set_frs menu '' (active(:ERget(FE_Destroy))=0);
	    }
    exec frs end;

    exec frs activate menuitem :ERget(FE_Edit);
    exec frs begin;
	if (enough_rows())  
            switch (whichtable)
            {
                case NODE_TBL:
                    get_node_name(vnode,new_vnode,sizeof(new_vnode));
		    if (*new_vnode != EOS)
		    {
			if (OK == edit_vnode(vnode,new_vnode))
			    display_vnode_list(new_vnode, topFormName);
		    }
                    break;
                case CONN_TBL:
                    exec frs getrow :topFormName conntable
                        (:type=connt_type, :prot=connt_protocol, 
                         :list=connt_listen, :addr=connt_address);
                    if (OK == edit_conn(vnode,type,addr,list,prot))
                        display_detail_conn(vnode, topFormName);
                    break;
                case AUTH_TBL:
                    exec frs getrow :topFormName authtable 
			(:login=login, :privglob=type);
                    if (OK == edit_auth(vnode,login,privglob))
                        display_detail_auth(vnode, topFormName );
                    break;
            }
    exec frs end;

    exec frs activate menuitem :ERget(FE_Attributes) (activate = 0);
    exec frs begin;
	exec sql begin declare section;
	    i4  cnt, row_no;
	exec sql end declare section;
	if( vnode_rows > 0 )
	{
	    exec frs inquire_frs table :topFormName (:cnt = lastrow(nodetable));
	    exec frs inquire_frs table :topFormName(:row_no = rowno(nodetable));
	    exec frs getrow :topFormName nodetable (:nodet_row=_record);
	    nodet_offset = nodet_row + (cnt - row_no);
	}
	attribute_frame();
    exec frs end;

    exec frs activate menuitem :ERget(FE_Test) (activate = 0);
    exec frs begin;
	if (!vnode_rows)
	    IIUGerr(E_ST0018_no_vnode_rows, 0, 0);
	else
	    test_connection(vnode);
    exec frs end;

    exec frs activate menuitem :control_menuitem;
    exec frs begin;
	control_frame();
    exec frs end;

    exec frs activate menuitem :ERget(FE_Help), frskey1;
    exec frs begin;
	FEhelp(ERx("numain.hlp"), SystemNetutilName); 
    exec frs end;

    exec frs activate menuitem :ERget(FE_End), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;

    exec frs activate before column nodetable all;
    exec frs begin;
        whichtable = NODE_TBL;  

	/* Activate the follwing menu items back on the node_table */
	exec frs set_frs menu '' (active(:ERget(FE_Create))=1);
	exec frs set_frs menu '' (active(:ERget(FE_Edit))=1);
	exec frs set_frs menu '' (active(:ERget(FE_Destroy))=1);
	if (vnode_rows > 0)     
	{
            exec frs getrow :topFormName nodetable 
		(:new_vnode=name, :nodet_row=_record);

	    if (!STequal(new_vnode,vnode))
	    {
	        STcopy(new_vnode,vnode);  
	        autht_row = connt_row = 1;
	    }
            display_detail( vnode, topFormName );  
	}
	else
	{
	    autht_row = connt_row = 1;
	    STcopy(ERx("''"), emptymsg);
	    exec frs putform :topFormName 
		(auth_vnode=:emptymsg, conn_vnode=:emptymsg);
	    exec frs inittable :topFormName authtable read;
	    exec frs inittable :topFormName conntable read;
	}
	exec frs set_frs menu '' (active(:ERget(FE_Test))=1);
	exec frs set_frs menu '' (active(:ERget(FE_Attributes))=1);
    exec frs end;

    exec frs activate before column authtable all;
    exec frs begin;
        whichtable = AUTH_TBL;
        exec frs scroll :topFormName nodetable to :nodet_row;
	if (auth_rows > 1)
	    exec frs getrow :topFormName authtable (:autht_row=_record);
	else
	    autht_row = 1;
	
	exec frs set_frs menu '' (active(:ERget(FE_Test))=0);
	exec frs set_frs menu '' (active(:ERget(FE_Attributes))=0);
    exec frs end;

    exec frs activate before column conntable all;
    exec frs begin;
        whichtable = CONN_TBL;
        exec frs scroll :topFormName nodetable to :nodet_row;
	if (conn_rows > 1)
	    exec frs getrow :topFormName conntable (:connt_row=_record);
	else
	    connt_row = 1;

	exec frs set_frs menu '' (active(:ERget(FE_Test))=0);
	exec frs set_frs menu '' (active(:ERget(FE_Attributes))=0);
    exec frs end;

    exec frs activate before column attribtable all;
    exec frs begin;
        whichtable = ATTR_TBL;
        exec frs scroll :topFormName nodetable to :nodet_row;
	if (attrib_rows > 1)
	    exec frs getrow :topFormName attribtable (:attrib_row=_record);
	else
	    connt_row = 1;

	exec frs set_frs menu '' (active(:ERget(FE_Test))=0);
	exec frs set_frs menu '' (active(:ERget(FE_Attributes))=0);
    exec frs end;

}

/*
**  control_frame -- handle form for shutdown/quiesce.
**  
**  This form is redesigned to include bridge server also. This has two
**  columns called "Network Server ID", "Network Server Type". The former
**  shows local server id, the later shows whether it is comm server or bridge
**  server.
**
**  This form consists of a single tablefield, which lists each communication
**  server and bridge server currently operating on the host.  There are four
**  menu items, besides the predictable "End" and "Help": "Quiesce",
**  "QuiesceAll", "Stop", and "StopAll".  Stopping a comm server does just
**  that: kills It. Quiescing a comm server stops it when the last active
**  connection goes away.  The "...All" menuitems do the same thing, but they
**  operate on every comm server in the tablefield, as opposed to just the
**  one the cursor is sitting on. 
**  The "...All" menuitems are applicable ONLY to the comm server NOT bridge
**  server.
**
**  The "...All" items will be set to inactive if there's only one
**  server either comm server or bridge server displayed in the tablefield.
*/

static VOID
control_frame()
{
    i4  svr_cnt, i;
    exec sql begin declare section;
    char csid[MAX_TXT];
    char type[MAX_TXT];
    exec sql end declare section;
    char msg[200];

    STprintf(msg, "%s - Server control", SystemNetutilName); 
    STcopy(ERx("nusvrcntrl"), controlFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), controlFormName) != OK)
        return;

    if (display_svr_list() != OK)
    {
	IIUGerr(E_ST0047_no_net_servers, 0, 0);
	return;
    }

/*
**  Beginning of display loop.
*/

    exec frs display :controlFormName;

    exec frs initialize;
    exec frs begin;
	adjust_control_menu();  /* Set "...All" items active or inactive. */
    exec frs end;

    exec frs activate menuitem :ERget(F_ST0043_quiesce_menuitem);
    exec frs begin;
	exec frs getrow :controlFormName svr_tbl (:csid = svr_id,
					    :type = svr_type);
	if (shutdown_svr( TRUE, csid ))
	{
	    exec frs deleterow :controlFormName svr_tbl;
	    if (!adjust_control_menu())
		exec frs breakdisplay;
	}
    exec frs end;


    exec frs activate menuitem :ERget(F_ST0044_quiesce_all_menuitem);
    exec frs begin;
	if (shutdown_svr(TRUE, NULL ))
	{
            svr_cnt = adjust_control_menu();
            for (i=1; i <= svr_cnt; i++)
            {
                exec frs getrow :controlFormName svr_tbl 1 (:csid = svr_id,
						    :type = svr_type);
                   if (OK == nu_shutdown( TRUE, csid ))
                   {
                      exec frs deleterow :controlFormName svr_tbl 1;
                      adjust_control_menu();
                   }
            }
            exec frs breakdisplay;
        }
        exec frs resume next;
    exec frs end;

    exec frs activate menuitem :ERget(F_ST0045_stop_menuitem);
    exec frs begin;
	exec frs getrow :controlFormName svr_tbl (:csid = svr_id,
						:type=svr_type);
	if (shutdown_svr(FALSE, csid ))
	{
	    exec frs deleterow :controlFormName svr_tbl;
	    if (!adjust_control_menu())
		exec frs breakdisplay;
	}
    exec frs end;

    exec frs activate menuitem :ERget(F_ST0046_stop_all_menuitem);
    exec frs begin;
	if (shutdown_svr(FALSE, NULL ))
        {
            svr_cnt = adjust_control_menu();
            for (i=1; i <= svr_cnt; i++)
            {
                exec frs getrow :controlFormName svr_tbl 1 (:csid = svr_id,
							:type = svr_type);
		if( ! STcompare(GCA_COMSVR_CLASS, type ) )
		{
                    if (OK == nu_shutdown( TRUE, csid ))
                    {
                        exec frs deleterow :controlFormName svr_tbl 1;
                        adjust_control_menu();
                    }
		}
            }
            exec frs breakdisplay;
        }
        exec frs resume next;    
    exec frs end;

    exec frs activate menuitem :ERget(FE_Help), frskey1;
    exec frs begin;
	FEhelp(ERx("nucomsvr.hlp"), msg); 
    exec frs end;

    exec frs activate menuitem :ERget(FE_End), frskey3;
    exec frs begin;
        exec frs breakdisplay;
    exec frs end;
}

/* adjust_control_menu -- set the "...All" menuitems active or inactive,
** depending on whether more than one  server is listed in the
** tablefield.
*/

static i4
adjust_control_menu()
{
    exec sql begin declare section;
    i4  cnt, act;
    exec sql end declare section;

    exec frs inquire_frs table :controlFormName (:cnt = lastrow(svr_tbl));

    if (cnt)
    {
	act = (cnt > 1);

	exec frs set_frs menu :controlFormName 
	    (active(:ERget(F_ST0046_stop_all_menuitem)) = :act);
	exec frs set_frs menu :controlFormName 
	    (active(:ERget(F_ST0044_quiesce_all_menuitem)) = :act);
    }

    return cnt;
}

/*
** nu_shutdown -- quiesce or shutdown a comm server or bridge server.
**
** The "qflag" parameter tells us whether this is a quiesce (TRUE) or a
** shutdown (FALSE) operation.
*/

STATUS
nu_shutdown(bool qflag, char *csid )
{
    STATUS rtn = nv_shutdown(qflag, csid);
    if (rtn != OK)
    {
	   IIUGerr(E_ST004E_shutdown_err,0,2,
		ERget(qflag? F_ST004C_quiesce_word: F_ST004D_stop_word), csid);
    }

    return rtn;
}

/*  shutdown_svr -- wrapper around "nu_shutdown" with user confirmation. */

static bool
shutdown_svr( bool qflag, char *csid )
{
    char lptitle[MAX_TXT], lpchoice[MAX_TXT];


    STprintf( lptitle, 
	      ERget(csid==NULL? S_ST0049_really_do_all: S_ST0048_really_do_it), 
	      ERget(qflag? F_ST004C_quiesce_word: F_ST004D_stop_word), csid );

    STpolycat(8, ERget(F_ST0103_yes), ERx(";"),
		 ERget(qflag?F_ST0043_quiesce_menuitem:F_ST0045_stop_menuitem),
		 ERx("\n"), ERget(F_ST0104_no), ERx(";"),
		 ERget(S_ST004A_dont_do_it),
		 ERget(qflag? F_ST004C_quiesce_word: F_ST004D_stop_word),
	      lpchoice);

    if (0 == IIFDlpListPick(lptitle, lpchoice, 0,-1,-1,NULL,NULL,NULL,NULL))
	return csid==NULL? TRUE: (OK == nu_shutdown(qflag, csid ));

    return FALSE;
}

/*
**  display_detail_auth()
**
**  Update the login detail table.  The table contains a field headed "Scope",
**  which doesn't correspond to any actual data, but just contains some
**  text explaining who gets to use the login represented by that row of the
**  table.  The login table can contain a maximum of two rows: one private
**  entry for the current user, and one global entry for the host.
*/

static VOID
display_detail_auth( char *vnode, char *formname )
{
    exec sql begin declare section;
    char msg[MAX_TXT+3];
    char *type;
    char *form = formname;
    char *glogin, *plogin, *uid;
    exec sql end declare section;

    if( ! STcompare( form, topFormName ) )
    {
        STpolycat(3, ERx("'"), vnode, ERx("'"), msg);
        exec frs putform :form (auth_vnode=:msg, conn_vnode=:msg);
    }
    exec frs inittable :form authtable read;
    nu_vnode_auth(vnode,&glogin,&plogin,&uid);

    auth_rows = 0;

    if (glogin != NULL)   /* Is there a global login? */
    {
	STprintf( msg, ERget(S_ST0005_any_user),
		  hostname==NULL? local_host: hostname );
	exec frs loadtable :form authtable 
	    (login=:glogin, type=:global_word, scope=:msg);
	auth_rows++;
    }

    if (plogin != NULL)   /* Is there a private login? */
    {
	STprintf(msg,ERget(S_ST0006_user_only),uid);
	exec frs loadtable :form authtable 
	    (login=:plogin, type=:private_word, scope=:msg);
	auth_rows++;
    }

    /* Leave the cursor where it was... */
    exec frs scroll :form authtable to :autht_row;
}


/*
**  display_detail_conn()
**
**  Update the connection detail table.  Load it with every connection
**  entry for the current vnode.  There is no practical limit to the number
**  of connection entries that can be associated with a vnode name; conse-
**  quently, this tablefield scrolls.
*/

static VOID
display_detail_conn(char *vnode, char *formname)
{
    exec sql begin declare section;
    char *type;
    char *address, *listen, *protocol;
    char *form;
    exec sql end declare section;
    i4  private;
    bool flag;

    form = formname;
    exec frs inittable :form conntable read;

    for ( flag=TRUE, conn_rows = 0;  /* "flag" just says to rewind the list */
	  NULL != nu_vnode_conn(flag,&private,vnode,&address,&listen,&protocol);
	  flag=FALSE, conn_rows++ )
    {
	type = private==1? private_word: global_word;
        exec frs loadtable :form conntable
	    ( connt_address=:address, 
	      connt_type=:type,
	      connt_protocol=:protocol,
	      connt_listen=:listen );
    }

    /* Leave the cursor where it was... */
    exec frs scroll :form conntable to :connt_row;
}

/*
**  display_detail() -- display all the info for given vnode on the 
**			form being displayed.
*/

static VOID
display_detail(char *vnode, char *formname)
{
     display_detail_auth( vnode, formname );
     display_detail_conn( vnode, formname );
     display_detail_attr( vnode, formname );
}

/*
**  display_detail_attr()
**
**  Update the attribute information table.  Load it with every attribute
**  entry for the current vnode.  There is no practical limit to the number
**  of attribute entries that can be associated with a vnode name; conse-
**  quently, this tablefield scrolls.
*/
static VOID
display_detail_attr( char *vnode, char *formname )
{
    exec sql begin declare section;
    char 	*type;
    char 	*form = formname;
    char 	*attr_name, *attr_value;
    exec sql end declare section;
    i4  	private;
    bool 	flag;

    exec frs inittable :form attribtable read;

    for ( flag=TRUE, attrib_rows = 0;  /* "flag" just says to rewind the list */
	  NULL != nu_vnode_attr(flag,&private,vnode,&attr_name, &attr_value);
	  flag=FALSE, attrib_rows++ )
    {
	type = private==1? private_word: global_word;
        exec frs loadtable :form attribtable
	    ( type=:type,
	      name=:attr_name,
	      value=:attr_value );
    }

    /* Leave the cursor where it was... */
    exec frs scroll :form attribtable to :attrib_row;

}

/*
**  get_node_name()
**
**  Using the standard FE prompt box, prompt the user for a new vnode
**  name.  This is used when creating a new vnode or when renaming an
**  existing one.
**
**  Inputs:
**    
**    vnode -- current name of vnode, or NULL if this is a new vnode
**    name -- location to receive name entered by user
**    namelen -- size of the location for new name
*/

static VOID
get_node_name(char *vnode, char *name, i4  namelen)
{
    char *vnm;
    i4  rown;
    bool flag;
    BOXPROMPT bp;
    char prmsg[MAX_TXT];
    char text[MAX_TXT];
    if (vnode != NULL)  /* Is there a current name? */
    {
	/* Include it in the prompt... */
	STprintf(text,ERget(S_ST001D_new_name_text),vnode);
	bp.text = text;
	STcopy(ERget(S_ST001E_new_name_prompt),prmsg);
    }
    else
    {
	/* Prompt for brand-new name... */
        STcopy(ERget(S_ST0002_enter_vnode),prmsg);
        bp.text=ERx("");
    }

    /* Set up magic data structure for IIUFbpBoxPrompt ... */

    bp.short_text = prmsg;
    bp.x = bp.y = 0;
    bp.buffer = name;
    bp.len = namelen - 1;
    bp.displen = 20;
    bp.initbuf = NULL;
    bp.htitle = bp.hfile = NULL;
    bp.lfn = bp.vfn = NULL;
    if (!IIUFbpBoxPrompt(&bp))
    {
	/* User cancelled, or it didn't work for some other reason... */
	*name = EOS;
	return;
    }

    /* Make sure the name doesn't duplicate an existing one... */

    for ( rown=1, flag=TRUE; 
	  NULL != (vnm = nu_vnode_list(flag)); flag=FALSE, rown++ )
    {
	if (0 == STbcompare(name, 0, vnm, 0, FALSE))
	{
	    IIUGerr(E_ST0001_vnode_exists,0,1,name);
	    nodet_row = rown;
	    exec frs scroll :topFormName nodetable to :nodet_row;
	    *name = EOS;
	}
    }
}

/*
**  edit_vnode()
**
**  Rename a virtual node.  The inputs are the current name and the new
**  name.
**
**  This process involves re-writing every detail entry -- authorization
**  and connection -- into the database, because the vnode name is
**  embedded in each entry.  In the case of authorization entries, 
**  re-writing the entry necessitates prompting the user for the
**  password associated with the user name, because this is essentially
**  write-only information; the name server must be given a password when
**  the record is written, but won't return it, for obvious reasons.  In
**  effect, this means that a user can't rename a vnode if without knowing
**  all associated passwords.
*/

static STATUS
edit_vnode(char *vnode, char *new_vnode)
{
    exec sql begin declare section;
    char log[MAX_TXT];
    char typ[MAX_TXT];
    char adr[MAX_TXT];
    char pro[MAX_TXT];
    char lis[128];
    exec sql end declare section;
    STATUS rtn = OK;
    char ppassw[MAX_TXT];
    char gpassw[MAX_TXT];
    bool has_global = FALSE;

    /* Grab all login information, and note whether there's a global
       login entry.  */

    exec frs unloadtable :topFormName authtable (:log=login, :typ=type);
    exec frs begin;
	if (OK != (rtn = nu_authedit(AUTH_EDITPW, vnode, log, gpassw, typ)))
	    exec frs endloop;
	if (nu_is_private(typ))
	    STcopy(gpassw,ppassw);
	else
	    has_global = TRUE;
    exec frs end;

    if (rtn != OK)  /* Everything ok so far? */
	return(rtn);

    /* Change the vnode name in all the authorization entries on d/b... */

    exec frs unloadtable :topFormName authtable (:log=login, :typ=type);
    exec frs begin;
    {
	bool priv = nu_is_private(typ);
	if (OK == (rtn = nv_del_auth(priv, vnode, log)))
	    rtn = nv_add_auth(priv, new_vnode, log, priv? ppassw: gpassw);
	if (rtn != OK)
	    exec frs endloop;
    }
    exec frs end;

    if (rtn != OK)     /* Did that work? */
    {
	IIUGerr(rtn, 0, 0);
	return(rtn);
    }

    /* Change the vnode name in all the connection entries on d/b... */

    exec frs unloadtable :topFormName conntable 
	(:typ = connt_type, :pro = connt_protocol, 
	 :adr = connt_address, :lis = connt_listen);
    exec frs begin;
    {
	bool priv = nu_is_private(typ);
	if (OK == (rtn = nv_del_node(priv, vnode, pro, adr, lis)))
	    rtn = nv_merge_node(priv, new_vnode, pro, adr, lis);
	if (rtn != OK)
	    exec frs endloop;
    }
    exec frs end;

    if (rtn != OK)    /* Did that work? */
	IIUGerr(rtn, 0, 0);
    else
	/* Change the vnode name in the internal data structure... */
        nu_rename_vnode(vnode,new_vnode);

    return rtn;
}

/*
**  destroy_vnode() 
**
**  Destroy all entries associated with the vnode name passed as an argument.
**  The user is prompted once for confirmation before anything is destroyed.
*/

static STATUS
destroy_vnode(char *vnode, char *formname )
{
    exec sql begin declare section;
    char login[MAX_TXT];
    char type[MAX_TXT];
    char addr[MAX_TXT];
    char prot[MAX_TXT];
    char list[128];
    char attr_name[MAX_TXT];
    char attr_value[MAX_TXT];
    char *form = formname;
    exec sql end declare section;
    bool has_global = FALSE;
    char msg[MAX_TXT];
    STATUS rtn = OK;

    /* Make user confirm that s/he really wants to destroy it... */

    STcopy(ERget(S_ST0010_all_data),msg);
    if ( CONFCH_YES != 
	 IIUFccConfirmChoice(CONF_DESTROY, vnode, msg, ERx(""), ERx("")) ) 
    {
	return 1;
    }

   /* Unload all logins from the tablefield and delete them from the d/b */

   exec frs unloadtable :form authtable (:type=type, :login=login);
   exec frs begin;
   	if (*login != EOS)
   	{
    	    if (OK != (rtn = nv_del_auth(nu_is_private(type), vnode, login)))
    	    {
	   	IIUGerr(rtn, 0, 0);
	   	exec frs endloop;
	    }
	}
    exec frs end;
     
    if (rtn != OK)  /* Did that work? */
	return rtn;

    /* Unload all attributes and delete from the d/b ... */

    exec frs unloadtable :form attribtable 
	( :type = type, :attr_name = name, :attr_value = value );
    exec frs begin;
	if ( OK != 
	     	(rtn = nv_del_attr( nu_is_private(type), vnode, attr_name,
							attr_value ) ) )
	    {
	    	IIUGerr(rtn, 0, 0);
	    	exec frs endloop;
	    }
    exec frs end;
    if (rtn != OK)  /* Did that work? */
	return rtn;

    /* Unload all connections and delete from the d/b ... */

    exec frs unloadtable :form conntable 
		(:type = connt_type, :prot = connt_protocol, 
		:addr = connt_address, :list = connt_listen);
    exec frs begin;
	if ( OK != (rtn = nv_del_node(nu_is_private(type),
				vnode, prot, addr, list)) )
	{
	    IIUGerr(rtn, 0, 0);
	    exec frs endloop;
	}
    exec frs end;
     
    if (rtn != OK)  /* Did that work? */
	return rtn;

    /* Update the internal data structure and return... */

    nu_destroy_attr( vnode );
    nu_destroy_conn( vnode );
    nu_destroy_vnode( vnode );
    return OK;
}

/*
**  destroy_auth()
**
**  Destroy a single login entry.  Inputs:
**
**    vnode -- name of the vnode associated with the entry
**    pg -- the word "Private" or "Global"
**    login -- the login name
**
**  The user is asked to confirm before the entry is destroyed.
*/

static STATUS
destroy_auth(char *vnode, char *pg, char *login)
{
    STATUS rtn;
    char msg[MAX_TXT];
    char *object;

    STprintf(msg,ERx("%s "),pg);
    CMtolower(msg,msg);

    /* If it's an installation password, reflect that in the prompt... */

    if ( !STequal(login,INST_PWD_LOGIN) )
    {
	STcat(msg, ERget(S_ST0012_auth_entry));
	object = login;
    }
    else
    {
        STcat(msg, ERget(S_ST0013_inst_pwd));
	object = vnode;
    }

    /* Request confirmation from the user... */

    if ( CONFCH_YES != 
	 IIUFccConfirmChoice(CONF_DESTROY, object, msg, ERx(""), ERx("")) )
    {
	return 1;
    }

    /* Delete the entry from the name server d/b... */

    if (OK != (rtn = nv_del_auth(nu_is_private(pg), vnode, login)))
	IIUGerr(rtn, 0, 0);
    else
	/* If that worked, delete it from the internal data structure */
        nu_change_auth(vnode, nu_is_private(pg), NULL);
    return rtn;
}

/*
**  destroy_conn()
**
**  Destroy a single connection entry.  Inputs:
**
**    vnode -- name of the vnode associated with the entry
**    priv -- the word "Private" or "Global"
**    addr -- the network address \
**    list -- the listen address   > of the entry to be destroyed
**    prot -- the protocol name   /
**
**  The user is asked to confirm before the entry is destroyed.
*/

static STATUS
destroy_conn(char *vnode, char *priv, char *addr, char *list, char *prot)
{
    STATUS rtn;
    char msg[MAX_TXT];

    /* Ask user for confirmation */

    STcopy(ERget(S_ST0011_conn_entry),msg);
    if ( CONFCH_YES != 
	 IIUFccConfirmChoice(CONF_DESTROY, ERx(""), msg, ERx(""), ERx("")) )
    {
        return 1;
    }

    /* Delete from the d/b, and if that works, remove from internal data */

    if (OK != (rtn = nv_del_node(nu_is_private(priv), vnode, prot, addr, list)))
	IIUGerr(rtn, 0, 0);
    else
        nu_remove_conn(vnode);
    return rtn;
}

/*
**  create_auth()
**
**  Create a new authorization entry for the indicated vnode.  This is
**  called as part of creating an entire vnode entry ("Create" menuitem
**  in the vnode table) and in response to the "Create" menuitem in the
**  login table.  The "new" parameter tells us which case this is.
**  In the case of a whole new vnode, we'll default to creating a
**  global entry if the user is a network manager and a private entry
**  otherwise.  If we're adding an entry to an existing vnode, we create
**  whichever kind of entry isn't there already; that is, if there's
**  currently a private entry we must be creating a global one, and vice
**  versa.  If there's no current entry in this case, we assume private if
**  the user isn't a net manager, and ask for a choice if he is.
*/

static STATUS
create_auth(char *vnode, bool new)
{
    char login[MAX_TXT];
    char password[MAX_TXT];
    STATUS rtn;
    char lpstring[MAX_TXT], temp1[MAX_TXT], temp2[MAX_TXT];
    i4  choice;
    i4  nrows = 0;
    i4  currtypes = 0;

    *login = *password = EOS;

    if (!new)  /* If this is an existing vnode that we're adding to... */
    {
        exec sql begin declare section;
        char temptype[MAX_TXT];
        exec sql end declare section;

	exec frs unloadtable :topFormName authtable (:temptype=type);
	exec frs begin;
	    currtypes += nu_is_private(temptype)? 1: 2;
	    nrows++;
	exec frs end;
    }

    /* At this point, currtypes == 0  ==>  No current logins
  			        == 1  ==>  There's a private login
				== 2  ==>  There's a global login
				== 3  ==>  There are both  */
    switch (currtypes)
    {
	case 0:
	    /* No login entries at the moment */

	    /* Put up a list pick to allow user to choose type */
	    STprintf( temp1,ERget(S_ST0005_any_user),
		      hostname==NULL? local_host: hostname );
	    STprintf(temp2,ERget(S_ST0006_user_only),username);
	    STprintf(lpstring,"%s;%s\n%s;%s",
	       global_word,temp1,private_word,temp2);
	    if( call_from_vw )
	        choice = IIFDlpListPick(ERget(S_ST0020_choose_vnode_type),
		       lpstring, 0, -1, -1, NULL, NULL, NULL, NULL);
	    else
	        choice = IIFDlpListPick(ERget(S_ST053D_choose_login_type),
		       lpstring, 0, -1, -1, NULL, NULL, NULL, NULL);
	    if (choice < 0)
	    {
		cancel_on_vnodetype = 1;
		return choice;
	    }
	    edit_global = (choice == 0);

	    /* Store the vnode type. The user has the option of
	    ** changing the login type. 
	    */
	    if( !edit_global )
	       original_type = TRUE;
	    else
	       original_type = FALSE;
	    break;
	case 1:
	    /* There's a private, so we'll create a global */
	    edit_global = TRUE;
	    break;
	case 2:
	    /* There's a global, so we'll create a private.  No problemo. */
	    edit_global = FALSE;
	    break;
	case 3:
	    /* Why are we here? */
	    IIUGerr(E_ST0016_cannot_create, 0, 1, vnode);
	    return 1;
    }

    /* Pop up the authorization edit window and let the user enter one. */

    switch (nu_authedit(AUTH_NEW, vnode, login, password,
			edit_global? global_word: private_word))
    {
	case EDIT_OK:   
	case EDIT_EMPTY:
	    rtn = OK;
	    if (*password != EOS)
	    {
		rtn = nv_add_auth(!edit_global, vnode, login, password);
	        if (rtn != OK)
		    IIUGerr(rtn, 0, 0);
	        else
                    nu_change_auth( vnode, !edit_global, login);
	    }
	    break;
	default:
	    rtn = 1;
    }

    /* Position cursor to what we just created.  Global is always first. */

    if (rtn == OK)
	autht_row = nrows == 0? 1: ( edit_global? 1: 2 );
    return rtn;
}

/*
**  edit_auth() -- Edit an existing login entry.  
**
**  Inputs:
**
**    vnode -- virtual node name associated with the entry
**    login -- the login name
**    privglob -- the word "Private" or "Global"
**    
*/

static STATUS
edit_auth(char *vnode, char *login, char *privglob)
{
    STATUS rtn;
    char e_login[MAX_TXT];
    char e_password[MAX_TXT];
    bool priv_flag;

    priv_flag = nu_is_private(privglob);

    STcopy(login, e_login);  /* Make a copy for the edit function to massage */
    switch (nu_authedit(AUTH_EDIT, vnode, e_login, e_password,
			priv_flag? private_word: global_word))
    {
	case EDIT_OK:  /* The user accepted the edit, so... */
	
	    /* Delete old entry from d/b and add new one */
	    if (OK == (rtn = nv_del_auth(priv_flag, vnode, login)))
		rtn = nv_add_auth(priv_flag, vnode, e_login, e_password);

	    /* If that worked, modify the internal data structure to match */
	    if (rtn != OK)
		IIUGerr(rtn, 0, 0);
	    else
                nu_change_auth(vnode, priv_flag, e_login);
	    break;

	default:
	    rtn = 1;
    }
    return rtn;
}

/*
**  create_conn() -- Create a new connection entry for the specified vnode.
*/

static STATUS
create_conn(char *vnode)
{
    char protocol[MAX_TXT];
    char address[128];
    char listen[MAX_TXT];
    bool private;
    STATUS rtn;

    /* Pop up the edit window for connections and accept a new one... */

    switch (nu_connedit(TRUE, vnode, &private, address, listen, protocol))
    {
	case EDIT_OK:  /* The user accepted the edit, so... */
	    
	    /* Add the new data to the name server database ... */
	    rtn = nv_merge_node(private,vnode,protocol,address,listen);

	    /* If that worked, add it to the internal data structure ... */
	    if (rtn != OK)
		IIUGerr(rtn, 0, 0);
	    else
	    {
                nu_new_conn(vnode,private,address,listen,protocol);
		/* Make new entry the current row... */
	        nu_locate_conn(vnode,private,address,listen,protocol,
			       &connt_row);
	    }
	    break;

	case EDIT_EMPTY:
	    rtn = OK;
	    break;

	default:
	    rtn = 1;
    }
    return rtn;
}

/*
**  edit_conn() -- Edit an existing connection entry
**
**  Inputs:
**
**    vnode -- virtual node name
**    type -- Private or Global
**    addr, list, prot -- network address, listen address, and protocol name
*/

static STATUS
edit_conn(char *vnode, char *type, char *addr, char *list, char *prot)
{
    i4  rtn;
    CONN *conn;
    char h_prot[MAX_TXT];
    char h_addr[MAX_TXT];
    char h_list[128];
    bool h_private;
    bool private = nu_is_private(type);

    /* Locate the entry in the internal data structure... */

    if (NULL == (conn = nu_locate_conn(vnode, private, addr, list, prot, NULL)))
	return 1;

    /* Make copies of everything that editing might change ... */

    h_private = private;
    STcopy(addr,h_addr);
    STcopy(list,h_list);
    STcopy(prot,h_prot);

    /* Pop up the edit window on the selected entry... */

    rtn = nu_connedit(FALSE, vnode, &private, addr, list, prot);

    /* If the user accepts the edit, delete the old entry from the name
       server d/b, add the new one, and update the internal data structure.  */

    if (rtn == EDIT_OK)
    {
	if (OK == (rtn = nv_del_node(h_private,vnode,h_prot,h_addr,h_list)))
	    rtn = nv_merge_node(private, vnode, prot, addr, list);
	if (rtn != OK)
	    IIUGerr(rtn, 0, 0);
	else
	    nu_change_conn(conn, private, addr, list, prot);
	return rtn;
    }

    /* If nu_connedit returned a negative value, the user has tried to
       create a duplicate entry by editing.  We'll position the cursor
       on the one his edit was a duplicate of, just to prove it's
       really there... */

    if (rtn < 0)
    {
	connt_row = -rtn;
	return OK;
    }

    return 1;
}

/*
**  display_vnode_list() -- fill vnode table from internal data structure
**
**  The "selected_vnode" argument may be NULL.  If it's not, it points
**  to a string which represents a vnode which will end up under the cursor.
*/

static VOID
display_vnode_list(char *selected_vnode, char *formname )
{
	bool flag;
	exec sql begin declare section;
	char *vnode_name;
	char *form;
	exec sql end declare section;

        form = formname; 
	exec frs inittable :form nodetable read;
        for ( flag=TRUE, vnode_rows=0; 
	      NULL != (vnode_name = nu_vnode_list(flag)); 
	      flag=FALSE, vnode_rows++ )
	{
	    exec frs loadtable :form nodetable (name=:vnode_name);
	    if ( selected_vnode != NULL && 
		 !STbcompare(vnode_name,0,selected_vnode,0,FALSE) )
	    {
	            nodet_row = vnode_rows+1;
	    }
	}
	
	exec frs scroll :form nodetable to :nodet_row;
}

/*
**  display_svr_list() -- fill svr table from name-server data
*/

static STATUS
display_svr_list()
{
    bool flag;
    i4  n_comm_svr;
    i4  n_brdg_svr;
    exec sql begin declare section;
    char *svrnam;
    char comsvr[6];
    char brgsvr[6];
    exec sql end declare section;

    exec frs inittable :controlFormName svr_tbl read;

    STcopy(GCA_COMSVR_CLASS, comsvr);
    STcopy(GCA_BRIDGESVR_CLASS, brgsvr);
    for ( flag = TRUE, n_comm_svr = 0; 
	  NULL != (svrnam = nu_comsvr_list(flag)); 
	  flag = FALSE, n_comm_svr++ )
    {
	STtrmwhite(svrnam);
	exec frs loadtable :controlFormName svr_tbl (svr_id = :svrnam,
						svr_type = :comsvr);
    }

    for ( flag = TRUE, n_brdg_svr = 0; 
	  NULL != (svrnam = nu_brgsvr_list(flag)); 
	  flag = FALSE, n_brdg_svr++ )
    {
	STtrmwhite(svrnam);
	exec frs loadtable :controlFormName svr_tbl (svr_id = :svrnam,
						svr_type = :brgsvr);
    }

    if( !  n_comm_svr ) 
        if( ! n_brdg_svr )
	  return FAIL;

    exec frs scroll :controlFormName svr_tbl to 1;
    return OK;
}

/*
**  enough_rows() 
**
**  Does the current table have enough rows to justify doing a destroy
**  or edit operation?  If not, display an appropriate error message and
**  return FALSE.
*/

static bool
enough_rows()
{
    switch (whichtable)
    {
	case NODE_TBL:
	    if (vnode_rows > 0) 
		return TRUE;
	    IIUGerr(E_ST0018_no_vnode_rows, 0, 0);
	    return FALSE;
	case CONN_TBL:
	    if (conn_rows > 0) 
		return TRUE;
	    IIUGerr(E_ST0019_no_conn_rows, 0, 0);
	    return FALSE;
	case AUTH_TBL:
	    if (auth_rows > 0) 
		return TRUE;
	    IIUGerr(E_ST001A_no_auth_rows, 0, 0);
	    return FALSE;
	case ATTR_TBL:
	    if ( attrib_rows > 0 ) 
		return TRUE;
	    IIUGerr(E_ST0136_no_attrib_rows, 0, 0);
	    return FALSE;
    }
}


/*
**  test_connection() -- connect to a remote name server
**
**  The "vnode" argument indicates which name server to connect to.  If
**  the connection is successful, we display a box message saying so.  If
**  it isn't, we display a box message saying it wasn't, along with the
**  error text explaining why it didn't work.
*/

static VOID
test_connection(char *vnode)
{
    char connid[MAX_TXT];
    STATUS stat;
    exec sql begin declare section;
    char etxt[512];
    exec sql end declare section;

    /*  If we're talking to a remote name server, the test connection will
	be from the local node to the remote to the destination.  If we're
	talking to our own name server, it's just from here to there.  */

    if (hostname != NULL) 
	STpolycat(3,hostname,ERx("::"),vnode,connid);
    else
	STcopy(vnode,connid);
    
    /*  Try the connection and set up the text field to say what happened. */

    if (OK == (stat = nv_test_host(connid)))
    {
	STprintf(etxt,ERget(S_ST0034_connection_ok), vnode);
    }
    else
    {
	STprintf(etxt, ERget(E_ST0035_connection_failed), vnode);
	if (OK != ERreport(stat, &etxt[STlength(etxt)]))
	    STprintf(&etxt[STlength(etxt)],
		 ERget(S_ST0036_reason_unknown), stat);
    }

    /* Display the result. */

    exec frs message :etxt with style=popup;
}
 

/*
**  nu_getargs() -- get and parse command line arguments.
*/

static ARG_DESC
args[] =
{
    { ERx("vnode"),	DB_CHR_TYPE, FARG_FAIL, NULL },
    { ERx("user"),	DB_CHR_TYPE, FARG_FAIL, NULL },
    { ERx("file"),	DB_CHR_TYPE, FARG_FAIL, NULL },
    NULL
};

static STATUS
nu_getargs(i4 argc, char *argv[])
{
    char *user;

/*
**  Get the current user id and hostname.
*/
    user = local_username;
    IDname(&user);
    GChostname(local_host,sizeof(local_host));

    user = NULL;
    args[0]._ref = (PTR)&hostname;
    args[1]._ref = (PTR)&user;
    args[2]._ref = (PTR)&control_file_list;

    if (IIUGuapParse(argc, argv, ERx("netutil"), args) != OK)
        return FAIL;
/*
**  If a hostname was specified then save the name of the node 
**  which is running the remote name server.  Otherwise the local
**  host's name server is to be operated on.
*/
    if (hostname != NULL && STequal(hostname,local_host))
        hostname = NULL;

/*
**  Figure out the default procotol to use, when creating
**  new connection entries.
*/
    if (hostname == NULL)      /* Only try to figure it out on local hosts */
    {
# ifdef DESKTOP
#  ifdef WIN16
	def_protocol = ERx("winsock");
#  else
	def_protocol = ERx("tcp_ip");
#  endif
# else
# ifdef VMS
	def_protocol = ERx("decnet");
# else
	def_protocol = ERx("tcp_ip");
# endif
# endif
    }
    else
    {
	def_protocol = ERx("");
    }

/*
**  If a username was specified then we're trying to impersonate 
**  another user to maintain their vnode definitions.  Otherwise
**  use the current username.
*/
    if (user != NULL) 
    {
        STcopy(user, username);
        is_setuid = TRUE;
    }
    else
        STcopy(local_username, username);

    return OK;
}

/*
**  attribute_frame -- Form for other attributes information.
**  
**  History:
**     21-Aug-97 (rajus01)
*/  
static VOID
attribute_frame()
{
    exec sql begin declare section;
    char 	new_vnode[MAX_TXT];
    char 	vnode[MAX_TXT];
    char 	emptymsg[3];
    char 	type[MAX_TXT];
    char 	prot[MAX_TXT];
    char 	list[MAX_TXT];
    char 	addr[MAX_TXT];
    char 	attr_name[MAX_TXT];
    char 	attr_value[MAX_TXT];
    char 	privglob[MAX_TXT];
    exec sql end declare section;
    STATUS 	estatus_conn;
    STATUS 	estatus_auth;

/*
**  Display Attributes form...
*/
    if( IIUFgtfGetForm( IIUFlcfLocateForm(), ERx( "attribform" ) ) != OK )
        PCexit( FAIL );
/*
**  Fill in master table...
*/
    display_vnode_list( NULL, "attribform" );

/*
**  Beginning of display loop.
*/
    exec frs display attribform;
    exec frs initialize;
    exec frs begin;
	{
	    exec sql begin declare section;
	    char 	userinfo_txt[MAX_TXT];
	    exec sql end declare section;

	    STprintf( userinfo_txt, "%s: %s", 
		ERget( S_ST0037_user_tag ), username );
            exec frs putform attribform( userinfo = :userinfo_txt );

	    /* Maintain the scroll position when you go from
	    ** loginform to attribform.
	    */
	    exec frs scroll attribform nodetable to :nodet_offset;
            exec frs scroll attribform nodetable to :nodet_row;

	    if ( hostname == NULL )
	    {
	        exec frs set_frs field attribform 
		    (invisible(remote_host_name)=1);
	    }
	    else
	    {
		exec frs putform attribform( remote_host_name = :hostname );
	        exec frs set_frs field attribform 
		    ( invisible( remote_host_name )=0 );
	    }
		   
	}
    exec frs end;
    exec frs activate menuitem :ERget( FE_Create );
    exec frs begin;
        switch( whichtable )
        {
            case NODE_TBL: 
		cancel_on_vnodetype = 0;
                get_node_name( NULL, new_vnode, sizeof( new_vnode ) );
                if ( *new_vnode != EOS )
                {
		    call_from_vw = 1;
                    estatus_auth = create_auth( new_vnode, TRUE );
		    if( cancel_on_vnodetype )
			break;
		     /* Keep the original vnode type, no matter
		     ** what the login type is changed to. 
		     */
		    if( original_type )
		        edit_global = FALSE;
		    else
		        edit_global = TRUE;
                    estatus_conn = create_conn( new_vnode );
                    if ( create_attr( new_vnode ) == OK || 
			 estatus_conn == OK || estatus_auth == OK )
                    {
                        display_vnode_list( new_vnode, "attribform" );  
                        display_vnode_list( new_vnode, topFormName );  
                    }
                }
                break;
            case CONN_TBL:
	    	if (vnode_rows > 0)
		{
		    if( create_conn( vnode ) == OK )  
		    {
			display_detail_conn( vnode, "attribform" );
                        display_detail_conn( vnode, topFormName );  
		    }
		}
		else
		    IIUGerr( E_ST0018_no_vnode_rows, 0, 0 );
                break;
            case ATTR_TBL:
	    	if ( vnode_rows > 0 )
		{
		    if( create_attr( vnode ) == OK )
		    {
			display_detail_attr( vnode, "attribform" );
			display_detail_attr( vnode, topFormName );
		    }
		}
		else
		    IIUGerr( E_ST0018_no_vnode_rows, 0, 0 );
                break;
        }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Destroy );
    exec frs begin;
        if ( enough_rows() )   
            switch( whichtable )
            {
                case NODE_TBL:
		    if( destroy_vnode( vnode, "attribform" ) == OK ) 
		    {
			if( nodet_row >= vnode_rows )
			    nodet_row--;
			/* Update loginform also . */
                        display_vnode_list( NULL, "attribform" );
                        display_vnode_list( NULL, topFormName );
                    }
                    break;
                case CONN_TBL:
                    exec frs getrow attribform conntable
                        ( :type=connt_type, :prot=connt_protocol, 
                          :list=connt_listen, :addr=connt_address );

                    if( nu_locate_conn( vnode, nu_is_private( type ), 
                                       addr, list, prot, NULL ) == NULL )
                        IIUGerr( E_ST0017_no_conn, 0, 0 );
		    else if( destroy_conn( vnode, type, addr,
					   list, prot ) == OK )
                    {
			if( connt_row >= conn_rows )
			    connt_row--;

			/* Update loginform also . */
                        display_detail_conn( vnode, "attribform" );
                        display_detail_conn( vnode, topFormName );
                    }
                    break;
                case ATTR_TBL:
                    exec frs getrow attribform attribtable 
                        ( :privglob=type, :attr_name=name, :attr_value=value );

                    if( nu_locate_attr( vnode, nu_is_private( privglob ), 
                                       attr_name, attr_value, NULL ) == NULL )
                        IIUGerr( E_ST020E_no_attr, 0, 0 );
		    else if( destroy_attr( vnode, privglob, attr_name,
					 attr_value ) == OK )
                    {
			if ( attrib_row >= attrib_rows )
			    attrib_row--;
			/* Update loginform also . */
                        display_detail_attr( vnode, "attribform" );
                        display_detail_attr( vnode, topFormName );
                    }
                    break;
            }
	    if( auth_rows == 0 && conn_rows == 0 && attrib_rows == 0)
	    {
		nu_destroy_vnode( vnode );
		/* Update loginform also . */
                display_vnode_list( NULL, "attribform" );
                display_vnode_list( NULL, topFormName );
		/* Deactivate the following menu items during this operation */
	        exec frs set_frs menu '' (active(:ERget(FE_Create))=0);
	        exec frs set_frs menu '' (active(:ERget(FE_Edit))=0);
	        exec frs set_frs menu '' (active(:ERget(FE_Destroy))=0);
	    }
    exec frs end;
    exec frs activate menuitem :ERget( FE_Edit );
    exec frs begin;
	if( enough_rows() )  
            switch( whichtable )
            {
                case NODE_TBL:
                    get_node_name( vnode, new_vnode, sizeof( new_vnode ) );
		    if( *new_vnode != EOS )
			if( edit_vnode( vnode, new_vnode ) == OK )
			{
			    display_vnode_list( new_vnode, "attribform" );
			    display_vnode_list( new_vnode, topFormName );
			}
                    break;
                case CONN_TBL:
                    exec frs getrow attribform conntable
                        ( :type=connt_type, :prot=connt_protocol, 
                         :list=connt_listen, :addr=connt_address );
                    if( edit_conn( vnode, type, addr, list, prot ) == OK )
		    {
                        display_detail_conn( vnode, "attribform" );
                        display_detail_conn( vnode, topFormName );
		    }
                    break;
                case ATTR_TBL:
                    exec frs getrow attribform attribtable 
			( :attr_name=name, :privglob=type, :attr_value=value );
                    if( edit_attr( vnode, privglob, attr_name,
					attr_value ) == OK )
		    {
                        display_detail_attr( vnode, "attribform" );
                        display_detail_attr( vnode, topFormName );
		    }
                    break;
            }
    exec frs end;

    exec frs activate menuitem :ERget( FE_Login ) ( activate = 0 );
    exec frs begin;
	exec sql begin declare section;
	    i4  cnt, row_no;
	exec sql end declare section;
	if( vnode_rows > 0 )
	{
	    exec frs inquire_frs table attribform ( :cnt = lastrow(nodetable) );
	    exec frs inquire_frs table attribform( :row_no = rowno(nodetable) );
	    exec frs getrow attribform nodetable ( :nodet_row=_record );
	    nodet_offset = nodet_row + ( cnt - row_no );
	    exec frs scroll :topFormName nodetable to :nodet_offset;
	    exec frs scroll :topFormName nodetable to :nodet_row;
	}
	exec frs breakdisplay;
    exec frs end;

    exec frs activate menuitem :ERget( FE_Test ) ( activate = 0 );
    exec frs begin;
	if ( !vnode_rows )
	    IIUGerr( E_ST0018_no_vnode_rows, 0, 0 );
	else
	    test_connection( vnode );
    exec frs end;

    exec frs activate menuitem :control_menuitem;
    exec frs begin;
	control_frame();
    exec frs end;

    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
	FEhelp( ERx( "nuattr.hlp" ), ERx( "Netutil" ) ); 
    exec frs end;

    exec frs activate menuitem :ERget( FE_End ), frskey3;
    exec frs begin;
	exec frs clear screen;
    	FEendforms();
	PCexit(OK);
    exec frs end;

    exec frs activate before column nodetable all;
    exec frs begin;
        whichtable = NODE_TBL;   

	/* Activate the following menu items back on the node_table */
	exec frs set_frs menu '' (active(:ERget(FE_Create))=1);
	exec frs set_frs menu '' (active(:ERget(FE_Edit))=1);
	exec frs set_frs menu '' (active(:ERget(FE_Destroy))=1);
	if ( vnode_rows > 0 )     
	{
   	    char 	msg[ MAX_TXT+3 ];
            exec frs getrow attribform nodetable
			( :new_vnode=name, :nodet_row = _record );
	    if( !STequal( new_vnode, vnode ) )
	    {
	        STcopy( new_vnode, vnode );  
	        connt_row = attrib_row = 1;
	    }
	    STpolycat( 3, ERx("'"), vnode, ERx("'"), msg );
	    exec frs putform attribform ( attr_connvnode=:msg,
					  attr_vnode=:msg );
            display_detail( vnode, "attribform" );  
	}
	else
	{
	    connt_row = attrib_row = 1;
	    STcopy( ERx("''"), emptymsg );
	    exec frs putform attribform 
		( attr_connvnode=:emptymsg, attr_vnode=:emptymsg );
	    exec frs inittable attribform conntable read;
	    exec frs inittable attribform attribtable read;
	}
	exec frs set_frs menu '' ( active( :ERget( FE_Test ) ) = 1 );
	exec frs set_frs menu '' ( active( :ERget( FE_Login ) ) = 1 );
    exec frs end;

    exec frs activate before column attribtable all;
    exec frs begin;
        whichtable = ATTR_TBL;
        exec frs scroll attribform nodetable to :nodet_row;
	if ( attrib_rows > 1 )
	    exec frs getrow attribform attribtable ( :attrib_row=_record );
	else
	    attrib_row = 1;
	exec frs set_frs menu '' ( active( :ERget( FE_Test ) ) = 0 );
	exec frs set_frs menu '' ( active( :ERget( FE_Login) ) = 0 );
    exec frs end;

    exec frs activate before column conntable all;
    exec frs begin;
        whichtable = CONN_TBL;
        exec frs scroll attribform nodetable to :nodet_row;
	if( conn_rows > 1 )
	    exec frs getrow attribform conntable( :connt_row = _record );
	else
	    connt_row = 1;
	exec frs set_frs menu '' ( active( :ERget( FE_Test ) ) = 0 );
	exec frs set_frs menu '' ( active( :ERget( FE_Login ) ) = 0 );
    exec frs end;

}

/*
**  create_attr() -- Create a new attribute entry for the specified vnode.
**
**  History:
**     21-Aug-97 (rajus01)
*/
static STATUS
create_attr( char *vnode )
{
    char 	attr_name[64 + 1];
    char 	attr_value[64 + 1];
    bool 	private;
    STATUS 	rtn;

    /* Pop up the edit window for attributes and accept a new one... */

    switch( nu_attredit( TRUE, vnode, &private, attr_name, attr_value ) )
    {
	case EDIT_OK:  /* The user accepted the edit, so... */
	    /* Add the new data to the name server database ... */
	    rtn = nv_merge_attr( private, vnode, attr_name, attr_value );

	    /* If that worked, add it to the internal data structure ... */
	    if ( rtn == OK )
	    {
                nu_new_attr( vnode, private, attr_name, attr_value );
		/* Make new entry the current row... */
	        nu_locate_attr( vnode, private, attr_name, attr_value,
				&attrib_row );
	    }
	    else
		IIUGerr( rtn, 0, 0 );
	    break;
	case EDIT_EMPTY:
	    rtn = OK;
	    break;
	default:
	    rtn = 1;
    }

    return rtn;
}

/*
**  edit_attr( ) -- Edit an existing attribute entry
**
**  Inputs:
**
**    vnode 	-- virtual node name
**    type 	-- Private or Global
**    nm, val 	-- attribute name, attribute value.
**
**  History:
**     21-Aug-97 (rajus01)
*/
static STATUS
edit_attr( char *vnode, char *type, char *name, char *value )
{
    i4  	rtn;
    ATTR 	*attr;
    char 	nm[MAX_TXT];
    char 	val[MAX_TXT];
    bool 	pg;
    bool 	private = nu_is_private( type );

    /* Locate the entry in the internal data structure... */

    if( ( attr = (ATTR *)nu_locate_attr( vnode, private, name,value, NULL ) ) == NULL )
	return 1;

    /* Make copies of everything that editing might change ... */
    pg = private;
    STcopy( name, nm );
    STcopy( value, val );

    /* Pop up the edit window on the selected entry... */

    rtn = nu_attredit( FALSE, vnode, &private, name, value );

    /* If the user accepts the edit, delete the old entry from the name
       server d/b, add the new one, and update the internal data structure.  */

    if( rtn == EDIT_OK )
    {
	if( ( rtn = nv_del_attr( pg, vnode, nm, val ) ) == OK )
	    rtn = nv_merge_attr( private, vnode, name, value );

	if ( rtn == OK )
	    nu_change_attr( attr, private, name,value );
	else
	    IIUGerr( rtn, 0, 0 );

	return rtn;
    }

    /* If nu_attredit returned a negative value, the user has tried to
       create a duplicate entry by editing.  We'll position the cursor
       on the one his edit was a duplicate of, just to prove it's
       really there... */

    if( rtn < 0 )
    {
	attrib_row = -rtn;
	return OK;
    }

    return 1;
}

/*
**  destroy_attr()
**
**  Destroy a single attribute entry.  Inputs:
**
**    vnode -- name of the vnode associated with the entry
**    priv  -- the word "Private" or "Global"
**    name  -- the attribute name  }-> of the entry to be destroyed
**    value -- the attribute value }
**
**  The user is asked to confirm before the entry is destroyed.
**
**  History:
**     21-Aug-97 (rajus01)
*/
static STATUS
destroy_attr( char *vnode, char *priv, char *name, char *value )
{
    STATUS 	rtn;
    char 	msg[ MAX_TXT ];

    /* Ask user for confirmation */

    STcopy( ERget(S_ST0537_attr_entry), msg );
    if ( CONFCH_YES != 
	 IIUFccConfirmChoice(CONF_DESTROY, ERx(""), msg, ERx(""), ERx("")) )
    {
        return 1;
    }

    /* Delete from the d/b, and if that works, remove from internal data */

    if( ( rtn = nv_del_attr( nu_is_private(priv), vnode, name, value ) ) == OK )
        nu_remove_attr( vnode );
    else
	IIUGerr( rtn, 0, 0 );

    return rtn;
}
