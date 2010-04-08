/*
** Copyright (c) 1992, 2009 Ingres Corporation
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
#include        <ci.h>
#include        <pc.h>
#include	<iplist.h>
#include	<erst.h>
#include        <uigdata.h>
#include	<stdprmpt.h>
#include	<gcccl.h>
#include	"nu.h"

/*
**  Name: nuedit.sc -- editing functions for netutil
**
**  Description:
** 	The functions in this module control the popup windows used for
**	editing detail information.  These functions are used exclusively
**	by the netutil utility.
**
**  Entry points:
**	nu_authedit() -- Edit or create login information
**	nu_connedit() -- Edit or create connection information
**	nu_attredit() -- Edit or create attribute information
** 	nu_locate_conn() -- Find a specified connection entry
** 	nu_locate_attr() -- Find a specified attribute entry
**	nu_is_private()  -- Distinguish between "Private" and "Global" words
**
**  History:
**	30-jun-92 (jonb)
**	    Created.
**	03-Oct-1993 (joplin)
**	    Fixed truncation of listen address problems by enlarging
**          the local character arrays used to hold the values,  Finalized
**          protocol selection list,  Got rid of "superuser" detection code.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	08-aug-95 (reijo01) #70384
**		Changed the procotols for the DESKTOP platform.
**      19-mar-96 (chech02)
**              deleted #include <locl.h>. LOCATION is redefined.
**              Added win 3.1 protocol drivers to protocol_listpick().
**	13-jun-96 (emmag)
**	    Add decnet to the list of protocols for NT.
**	21-Aug-97 (rajus01)
**	    Added nu_attredit(), nu_locate_attr() routines. Renamed 
**	    "logpass" to "logpassedit".
**      14-oct-97 (funka01)
**          Change hard-coded Netutil messages to depend on SystemNetutilName.
**          Modified to use correct form for JASMINE/INGRES.
**	23-Jan-98 (rajus01)
**	   Changed the order of field names so that the cursor is positioned
**	   at Login field.(logpassedit.frm). Fixed problems with the display
**	   of login type on the netutil form.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-sep-2003 (somsa01)
**	    Added tcp_ip to Windows.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	08-may-2008 (joea)
**	    Updated the list of protocols in protocol_listpick.
**	22-Sep-2008 (lunbr01) Sir 119985
**	    Move "tcp_ip" to the top of the ListChoices for Protocol
**	    and "wintcp" to the bottom for Windows (32-bit), since
**	    "tcp_ip" is now the preferred protocol.
**	24-Nov-2009 (frima01) Bug 122490
**	    Moved return types to separate lines.
*/

bool	nu_is_private();
CONN *	nu_locate_conn();
ATTR *	nu_locate_attr();
static char *protocol_listpick();

exec sql begin declare section;
GLOBALREF char global_word[];
GLOBALREF char private_word[];
GLOBALREF bool edit_global;
GLOBALREF char username[];
static char logpassFormName[ MAX_TXT ];
static char conneditFormName[ MAX_TXT ];
exec sql end declare section;

/*
**  nu_authedit() -- edit popup for login (authorization) entries
**
**  Inputs:
**
**    function -- AUTH_NEW ==> get new login
**                AUTH_EDIT ==> edit existing login
**                AUTH_EDITPW ==> edit password of existing login only
**    vnode -- the vnode name this entry is associated with (not edited)
**    login -- the login name.  May be changed by editing.
**    password -- points to location to store the password.  Edited but
**                not displayed, i.e. its input value doesn't matter.
**    prompt_word -- "Private" or "Global" depending on what kind of login
**		     this is.  Not Edited.
**
**  Returns:
**
**    EDIT_OK -- Edit approved by the user
**    EDIT_CANCEL -- Edit was cancelled by the user
**    EDIT_EMPTY -- User didn't change anything
**
**  Note that you can't change a private entry to a global by editing, or
**  vice versa.  Note also that a password must _always_ be entered.
*/

STATUS
nu_authedit(i4 function, char *vn, 
	    char *login, char *password, char *prompt_word)
{
	char lc_prompt_word[20];/* The prompt word in lowercase for messages */
	STATUS rtn;

	/* Make sure the sizes of these variables match form definitions */
	exec sql begin declare section;
	char sav_login[32 + 1];
	char lcl_login[32 + 1];
	char lcl_logintype[10 + 1];
	char lcl_pass[32 + 1];
	char lcl_pass_conf[32 + 1];
	char msg[40];
	i4 chflag, len;
	char *vnode = vn;
	char *type = prompt_word;
	exec sql end declare section;
	char msg1[200];

 	STprintf(msg1, "%s - Login/password data", SystemNetutilName); 

	*lcl_pass = *sav_login = *lcl_logintype = EOS;
	STcopy(prompt_word, lc_prompt_word);
	CMtolower(lc_prompt_word, lc_prompt_word);

	/* Find the form */

	STcopy(ERx("logpassedit"),logpassFormName);
	if (IIUFgtfGetForm(IIUFlcfLocateForm(), logpassFormName) != OK)
	{
    	    PCexit(FAIL);
	}

	exec frs set_frs frs 
	    ( validate(keys)=0, validate(menu)=0, validate(menuitem)=0,
	      validate(nextfield)=0, validate(previousfield)=0 ); 
	exec frs set_frs frs 
	    ( activate(keys)=1, activate(menu)=1, activate(menuitem)=1,
	      activate(nextfield)=1, activate(previousfield)=0 ); 

	/* Display the form.  Beginning of display loop. */

        exec frs display :logpassFormName with style=popup;

        exec frs initialize (f_node = :vnode);
	exec frs begin;
	    if (function == AUTH_NEW)
	    {
	        if (*login == EOS)
		{
		    STcopy(username,sav_login);
		    exec frs set_frs form (change(:logpassFormName)=1);
		}
	        exec frs putform :logpassFormName 
		    (infoline = :ERget(S_ST001B_define_new_auth));
	    }
	    else if (function == AUTH_EDIT)
	    {
	        exec frs putform :logpassFormName 
		    (infoline = :ERget(S_ST001C_edit_auth));
	    }
	    else
	    {
	        STprintf(msg,ERget(S_ST0030_enter_pw),lc_prompt_word );
		exec frs putform :logpassFormName (infoline=:msg);
		exec frs set_frs field :logpassFormName 
		    ( displayonly(f_login) = 1, underline(f_login) = 0 );
	    }

	    if (*sav_login == EOS)
	        STcopy(login,sav_login);

	    exec frs inquire_frs field :logpassFormName (:len=length(f_login_type));
	    MEfill(len,' ',msg);
	    STcopy(prompt_word,&msg[len-STlength(prompt_word)]);


	    exec frs putform :logpassFormName 
		( f_login = :sav_login, 
		  f_login_type = :type,
		  f_password = '',
		  f_password_conf = '' );
	exec frs end;

	exec frs activate menuitem :ERget(FE_Save), frskey4;
	exec frs begin;
	    exec frs inquire_frs form (:chflag = change(:logpassFormName));
	    if (!chflag)   /* Has anything changed on the form? */
	    { 
		rtn = EDIT_EMPTY;   /* Return a code that says so */
		exec frs breakdisplay;
	    } 
	    
	    /* Get all the potentially changed fields from the form */

	    exec frs getform :logpassFormName (:lcl_logintype=f_login_type,
					:lcl_login=f_login,
				       	:lcl_pass=f_password,
				       	:lcl_pass_conf=f_password_conf );
				       
	    if (*lcl_login != EOS) 
	    {
		if (*lcl_pass == EOS)  
		{
		    IIUGerr(E_ST002C_must_enter_pw,0,0);
		    exec frs resume field f_password;
		}
		if (*lcl_pass_conf == EOS)
		{
		    IIUGerr(E_ST002D_must_confirm_pw,0,0);
		    exec frs resume field f_password_conf;
		}
		if (!STequal(lcl_pass,lcl_pass_conf))
		{
		    IIUGerr(E_ST002F_password_mismatch,0,0);
		    exec frs putform :logpassFormName
		        (f_password = '', f_password_conf = '');
		    exec frs resume field f_password;
		}
	        if (*lcl_logintype != EOS) 
		{
	          if( STcompare( lcl_logintype, private_word ) == 0) 
		      edit_global = FALSE;
  		  else
		      edit_global = TRUE;
		}
                STcopy(lcl_login, login); 
	        STcopy(lcl_pass, password); 
            }
	    rtn = OK;
	    exec frs breakdisplay; 
	exec frs end;

	exec frs activate menuitem :ERget(FE_Cancel), frskey9 (activate=0);
	exec frs begin;
	    rtn = EDIT_CANCEL;
	    exec frs breakdisplay;
	exec frs end;

        exec frs activate menuitem :ERget(FE_Help), frskey1;
        exec frs begin;
            FEhelp(ERx("logpass.hlp"), msg1);
	exec frs end;
    	exec frs activate after field f_login_type;
    	exec frs begin;
	    exec sql begin declare section;
	    char privglob[12];
	    bool lcl_private = nu_is_private( lc_prompt_word );
	    exec sql end declare section;

            exec frs getform logpassedit (:privglob=f_login_type);

	    if (nu_is_private(privglob))
	    {
	    	exec frs putform logpassedit (f_login_type=:private_word);
	    	lcl_private = TRUE;
	    }
	    else if (!CMcmpnocase(privglob,global_word))
	    {
	    	lcl_private = FALSE;
	    	exec frs putform logpassedit (f_login_type=:global_word);
	    }
	    else
	    {
	    	IIUGerr(E_ST000B_p_or_g,0,0);
	    	exec frs resume field f_login_type;
	    }
	    exec frs resume next;
        exec frs end;

	exec frs activate before field f_password;
	exec frs begin;
	    exec frs getform :logpassFormName 
		(:lcl_logintype = f_login_type );
	    STcopy(lcl_logintype, lc_prompt_word);
	    CMtolower(lc_prompt_word, lc_prompt_word);
	    if (*lcl_logintype != EOS) 
	       STprintf( msg, STequal(lcl_login,INST_PWD_LOGIN)? 
	                 ERget(S_ST0031_enter_inst_pw):
	                 ERget(S_ST0030_enter_pw), lc_prompt_word );
	    exec frs putform :logpassFormName 
		  ( prompt_line = :msg,
		  noecho_reminder = :ERget(S_ST0033_password_wont_echo) );
	    exec frs resume next;
	exec frs end;

	exec frs activate after field f_password;
	exec frs begin;
	    exec frs putform :logpassFormName
		(prompt_line = :ERget(S_ST0029_confirm_pw));
	    exec frs resume next;
	exec frs end;

	exec frs activate before field f_password_conf;
	exec frs begin;
	    exec frs getform :logpassFormName 
		(:lcl_login = f_login, :lcl_pass = f_password);
	    if (*lcl_pass == EOS && *lcl_login != EOS)
		exec frs resume field f_password;
	    exec frs resume next;
	exec frs end;

	exec frs activate after field f_password_conf;
	exec frs begin;
	    exec frs putform :logpassFormName (noecho_reminder = '', prompt_line = '');
	   
	    exec frs resume next;
	exec frs end;

	exec frs activate before field f_login;
	exec frs begin;
	    exec frs putform :logpassFormName (noecho_reminder = '', prompt_line = '');
	    exec frs resume next;
	exec frs end;

	exec frs activate after field f_login;
	exec frs begin;
	    exec frs getform :logpassFormName (:lcl_login=f_login);
	    if (*lcl_login == EOS)
		*lcl_pass = EOS;
	    exec frs resume next;
	exec frs end;

	exec frs finalize;
	return rtn;
}


/*
**  nu_connedit() -- Popup form for editing or entering connection data
**
**  Inputs:
**
**    new -- boolean, TRUE if this is a new entry, FALSE if we're editing
**           an existing entry.
**    cnode -- the virtual node name.  Not edited.
**    cpriv -- pointer to boolean, TRUE for private, FALSE for global.  May
**             be changed by editing.
**    caddr -- the network address.  May be changed by editing.
**    clist -- the listen address.  May be changed by editing.
**    cprot -- the protocol name.  May be changed by editing.
**
**  Returns:
**
**    EDIT_OK -- Edit approved by the user
**    EDIT_CANCEL -- Edit was cancelled by the user
**    EDIT_EMPTY -- User didn't change anything
**    < 0 -- This is a duplicate entry.  The value returned is the negative
**           of the row number for the entry that was duplicated.
** History:
**	23-Jan-98 (rajus01)
**	   Changed the order of field names so that the cursor is positioned
**	   at Network address field.(connedit.frm). Fixed problems with the
**	   display of connection type on the netutil form.
**	06-Aug-2009 (rajus01) QA issue: SD 138409, Bug 122417.
**	   Don't display the "Duplicate connection Entry" message when the
**	   values being edited are the same as old values. It doesn't seem
**	   correct from usability view point.
**	06-Aug-09 (rajus01) QA issue: SD 138409, Bug 122417.
**	   Revised code changes based on Gordy's feedback about 
**	   my prior change 499298.
*/


i4
nu_connedit(new, cnode, cpriv, caddr, clist, cprot) 
bool new, *cpriv;
exec sql begin declare section;
char *cnode, *caddr, *clist, *cprot; 
exec sql end declare section; 
{ 
    i4  rtn; 

    /* Make sure the sizes of these variables match form definitions */
    exec sql begin declare section; 
    char fldname[64];
    bool lcl_private = *cpriv;
    char lcl_privglob[12];
    char lcl_address[128 + 1]; 
    char lcl_listen[64 + 1]; 
    char lcl_protocol[64 + 1]; 
    bool changed;
    bool data_changed = TRUE;
    char *disp; 
    GLOBALREF char *def_protocol;
    exec sql end declare section;
    char msg[200];

    STprintf(msg, "%s - Connection data", SystemNetutilName); 
    /* Find the form... */
    STcopy(ERx("connedit"), conneditFormName);
    if (IIUFgtfGetForm(IIUFlcfLocateForm(), conneditFormName) != OK)
        PCexit(FAIL);

    /* ...and display it.  Top of display loop. */
    
    exec frs display :conneditFormName with style=popup;

    exec frs initialize ( f_node = :cnode );
    exec frs begin;

        exec frs set_frs frs 
            ( validate(keys)=0, validate(menu)=0, validate(menuitem)=0,
              validate(nextfield)=0, validate(previousfield)=0 ); 
        exec frs set_frs frs 
            ( activate(keys)=1, activate(menu)=1, activate(menuitem)=1,
              activate(nextfield)=1, activate(previousfield)=0 ); 

	exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices)) = 0);
    
        if (new)
        {
	    disp = ERget(S_ST0014_define_new_conn);
	    exec frs putform :conneditFormName 
		(f_address=:cnode, f_protocol = :def_protocol);
        }
        else
        {
	    disp = ERget(S_ST0015_edit_conn);
	    exec frs putform :conneditFormName 
	      ( f_listen = :clist, 
	        f_protocol = :cprot,
	        f_address = :caddr );
        }

	exec frs putform :conneditFormName (infoline=:disp);
    
	 if( new )
            disp = edit_global ? global_word: private_word;
	 else
            disp = *cpriv ? private_word: global_word;
        exec frs putform :conneditFormName (f_privglob=:disp);
    exec frs end;

    exec frs activate menuitem :ERget(FE_Save), frskey4;
    exec frs begin;

        exec frs inquire_frs form (:changed = change(:conneditFormName));
	if (!changed)
        { 
	    *caddr = *clist = *cprot = EOS;
	    rtn = EDIT_EMPTY;
	    exec frs breakdisplay;
	}

        exec frs getform :conneditFormName 
	    ( :lcl_privglob = f_privglob, 
	      :lcl_address = f_address, 
	      :lcl_protocol = f_protocol, 
	      :lcl_listen = f_listen );

	if (*lcl_privglob == EOS)  
	{ 
	    IIUGerr(E_ST000B_p_or_g,0,0);
	    exec frs resume field f_privglob; 
	} 

	if (*lcl_address == EOS)  
	{
            IIUGerr(E_ST000C_net_address_req,0,0);
	    exec frs resume field f_address;
	} 
	
	if (*lcl_protocol == EOS) 
	{
            IIUGerr(E_ST000D_protocol_req,0,0);
	    exec frs resume field f_protocol;
        } 

	if (*lcl_listen == EOS)  
	{ 
	    IIUGerr(E_ST000E_listen_req,0,0);
	    exec frs resume field f_listen;
	} 

	if( changed )
	{
	    if( *cpriv == lcl_private && 
		(STcompare(caddr, lcl_address ) == 0) && 
		(STcompare(clist, lcl_listen ) == 0) &&
		(STcompare(cprot, lcl_protocol ) == 0) ) 
		data_changed = FALSE;
	}

	*cpriv = lcl_private;
        STcopy(lcl_address,caddr);
	STcopy(lcl_listen,clist);
        STcopy(lcl_protocol,cprot);

	if (NULL != nu_locate_conn(cnode,lcl_private,caddr,clist,cprot,&rtn))
        { 
	    /* If the connection entry is found and if it occurs 
	    ** when creating a new connection from a connEdit pop up
	    ** frame, then display the warning message that it is a 
	    ** "Duplicate connection Entry" to the user. 
	    ** Also, if more than one record is present and a change
	    ** is made to match another existing entry the duplicate error
	    ** message must be displayed. Otherwise don't.
	    **
	    ** Example test scenario provided by Gordy.
	    **
	    ** Let us say one entry is already present, and
	    ** 
	    ** 1. Adding a second identical entry should get a duplicate error 
	    **	  and no new entry should be made.
	    ** 2. Adding a second different entry should have two different 
	    **    entries.
	    ** 3. Changing one entry to be the same as the other should produce
	    **    a duplicate error and no change should occur and cursor 
	    **	  should not be moved.
	    ** 4. Changing one entry without making any real change should 
	    **	  simply end up with the same two entries.
	    */
	    if( new )
	    {
	       rtn = -rtn; 
	       IIUGerr(E_ST000F_duplicate_conn,0,0);
	    }
	    else /* edit */
	    {
		/* Multiple connection entries present */
	        if( rtn >=1 && data_changed )  
	        {
	           rtn = -rtn; 
	           IIUGerr(E_ST000F_duplicate_conn,0,0);
	        }
	    }
	} 
	else 
	    rtn = EDIT_OK; 
	exec frs breakdisplay; 
    exec frs end;

    exec frs activate menuitem :ERget(FE_Cancel), frskey9 (activate=0);
    exec frs begin;
        rtn = EDIT_CANCEL;
        exec frs breakdisplay;
    exec frs end;

    exec frs activate menuitem :ERget(F_ST0001_list_choices) (activate = 0);
    exec frs begin;
	exec sql begin declare section;
	char *prot;
	exec sql end declare section;

	if (NULL != (prot = protocol_listpick(TRUE)))
	{
	    exec frs putform :conneditFormName (f_protocol=:prot);
	    exec frs set_frs form (change(:conneditFormName) = 1);
	    exec frs set_frs field :conneditFormName (change(f_protocol) = 1);
	    exec frs redisplay;
	}

    exec frs end;

    exec frs activate menuitem :ERget(FE_Help), frskey1;
    exec frs begin;
        FEhelp(ERx("connedit.hlp"), msg);
    exec frs end;

    exec frs activate before field all;
    exec frs begin;
        exec frs inquire_frs form (:fldname = field);
	if (STcompare(fldname, ERx("f_protocol")) == 0) 
	{
	    exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices))=1);
	}
	else
	{
	    exec frs set_frs menu '' (active(:ERget(F_ST0001_list_choices))=0); 
	}
	exec frs resume next;
    exec frs end;

    exec frs activate after field f_privglob;
    exec frs begin;
	exec sql begin declare section;
	char privglob[12];
	exec sql end declare section;

        exec frs getform :conneditFormName (:privglob=f_privglob);

	if (nu_is_private(privglob))
	{
	    exec frs putform :conneditFormName (f_privglob=:private_word);
	    lcl_private = TRUE;
	}
	else if (!CMcmpnocase(privglob,global_word))
	{
	    lcl_private = FALSE;
	    exec frs putform :conneditFormName (f_privglob=:global_word);
	}
	else
	{
	    IIUGerr(E_ST000B_p_or_g,0,0);
	    exec frs resume field f_privglob;
	}
	exec frs resume next;
    exec frs end;

    exec frs activate before field f_address;
    exec frs begin;
	exec sql begin declare section;
	char privglob[12];
	exec sql end declare section;

        exec frs getform :conneditFormName (:privglob=f_privglob);

	if (nu_is_private(privglob))
	{
	    exec frs putform :conneditFormName (f_privglob=:private_word);
	    lcl_private = TRUE;
	}
	else if (!CMcmpnocase(privglob,global_word))
	{
	    lcl_private = FALSE;
	    exec frs putform :conneditFormName (f_privglob=:global_word);
	}
	else
	{
	    IIUGerr(E_ST000B_p_or_g,0,0);
	    exec frs resume field f_privglob;
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
** 
**  (joplin) Note:  There were several methods that were attempted by
**  the original author (jonb).  However, what I finally settled on
**  was a simple regurgatation of the protocol values listed in the
**  documentation for NETU,NETUTIL.  Please check out the older 
**  revisions of this file to see the programatic methods used.
**
**  Inputs:
**
**    really  -- flag to tell us if we *really* want to do this
**
**  Returns:
**
**    A Pointer to a character string representing a protocal.
**
*/


static char *
protocol_listpick(really)
bool really;
{
    i4  lpx;
    static char lpstr[256];
    char *sp, *rtn;

/*
**  Straight from the original documentation, here is the 
**  List of Protocols and the amazing dancing bear, Checkers.
*/
    *lpstr = EOS;
#  ifdef WIN16
    STcat(lpstr, ERx("winsock\n"));
    STcat(lpstr, ERx("netbios\n"));
    STcat(lpstr, ERx("decnet\n"));
    STcat(lpstr, ERx("nvl_spx\n"));
    STcat(lpstr, ERx("tcp_dec\n"));
    STcat(lpstr, ERx("tcp_ftp\n"));
    STcat(lpstr, ERx("tcp_lmx\n"));
    STcat(lpstr, ERx("tcp_nfs\n"));
    STcat(lpstr, ERx("tcp_wol\n"));
    STcat(lpstr, ERx("lan_workplace\n"));
#  else
#if defined(NT_GENERIC)
    STcat(lpstr, ERx("tcp_ip\n"));
    STcat(lpstr, ERx("lanman\n"));
    STcat(lpstr, ERx("nvlspx\n"));
    STcat(lpstr, ERx("decnet\n"));
    STcat(lpstr, ERx("wintcp\n"));
#elif defined(VMS)
    STcat(lpstr, ERx("decnet\n"));
    STcat(lpstr, ERx("sna_lu0\n"));
    STcat(lpstr, ERx("tcp_dec\n"));
    STcat(lpstr, ERx("tcp_wol\n"));
#else /* UNIX, etc. */
    STcat(lpstr, ERx("decnet\n"));
    STcat(lpstr, ERx("sna_lu62\n"));
    STcat(lpstr, ERx("tcp_ibm\n"));
    STcat(lpstr, ERx("tcp_ip\n"));
    STcat(lpstr, ERx("tcp_knet\n"));
#endif
#  endif /* WIN16 */

    if (*lpstr == EOS)
	return NULL;

    if (!really)
	return lpstr;  /* It's ok, it won't be used.  Any non-null will do. */

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



/*
**  nu_locate_conn() -- Locate a connection entry in the internal data
**
**  Loops through the internal data structure looking for a connection
**  entry that matches the criteria specified by the input arguments.
**  This has the effect of making the corresponding member of the internal
**  list the "current" member, which is convenient if it needs to be
**  deleted or modified.  This function can also be used as a simple
**  predicate to see if the specified entry currently exists or not.
**  
**  Inputs:
**
**    vnode -- the virtual node name
**    private -- TRUE for a private entry, FALSE for a global
**    address -- the network address \
**    listen -- the listen address    > of the entry being searched for
**    protocol -- the protocol name  /
**    row -- may be NULL; if not, points to i4  which receives the row
**           number of the match, if any.
**
**  Returns:
**
**    Pointer to connection entry in internal data structure.
**    
*/

CONN *
nu_locate_conn(char *vnode, i4  private, 
	       char *address, char *listen, char *protocol, i4  *row)
{
    bool flag;
    char *ad, *li, *pr;
    i4  pv, lrow;
    CONN *rtn;

    /* Loop through the data structure looking for a match... */

    for ( flag=TRUE, lrow=1; /* "flag" just says to rewind list the 1st time */
	  NULL != (rtn = nu_vnode_conn(flag, &pv, vnode, &ad, &li, &pr)); 
          flag=FALSE, lrow++ )
    {
        if ( pv == private  &&
             !STbcompare(address,  0, ad, 0, FALSE) &&
	     !STbcompare(listen,   0, li, 0, FALSE) &&
	     !STbcompare(protocol, 0, pr, 0, FALSE) ) 
	{
	    /* Found one. */
	    if (row != NULL) 
		*row = lrow;
	    return rtn;
	}
    }
    return NULL;
}

/*
**  nu_is_private() 
**
**  Converts the words "Private" and "Global" to a boolean.  Input argument
**  is the word to be examined.  Returns TRUE for private, FALSE for global.
*/

bool
nu_is_private(char *pg)
{
    return (!CMcmpnocase(pg,private_word));
}

/*
**  Name: nu_attredit() 
**
**  Description:
**	Edit popup for attribute information
**
**  Input:
**    new		boolean for new attribute or edit existing attribute.
**    vnode 		vnode name this entry is associated with (not edited)
**    privglob		"Private or Global"
**    name		attribute name
**    value		attribute value
**
**  Output:
**	None.
** 
**  Returns:
**    EDIT_OK 		Edit approved by the user
**    EDIT_CANCEL 	Edit was cancelled by the user
**    EDIT_EMPTY 	User didn't change anything
**
** History:
**	08-Aug-97 (rajus01)
**	    Created.
**	23-Jan-98 (rajus01)
**	   Changed the order of field names so that the cursor is positioned
**	   at attribute value field.(attribedit.frm). Fixed problems with the
**	   display of attribute type on the netutil form.
*/
i4
nu_attredit( new, vnode, privglob, name, value ) 
bool 	new, *privglob;
exec sql begin declare section;
char 	*vnode, *name, *value; 
exec sql end declare section; 
{ 
    i4  rtn; 

    /* Make sure the sizes of these variables match form definitions */
    exec sql begin declare section; 
    bool 	lcl_type = *privglob;
    char 	lcl_privglob[12];
    char 	lcl_name[64 + 1]; 
    char 	lcl_value[64 + 1]; 
    bool 	changed;
    char 	*disp; 
    exec sql end declare section;

    /* Find the form... */

    if( IIUFgtfGetForm( IIUFlcfLocateForm(), ERx( "attribedit" ) ) != OK )
        PCexit(FAIL);

    /* ...and display it.  Top of display loop. */
    
    exec frs display attribedit with style = popup;

    exec frs initialize ( f_node = :vnode );
    exec frs begin;

        exec frs set_frs frs 
            ( validate(keys)=0, validate(menu)=0, validate(menuitem)=0,
              validate(nextfield)=0, validate(previousfield)=0 ); 
        exec frs set_frs frs 
            ( activate(keys)=1, activate(menu)=1, activate(menuitem)=1,
              activate(nextfield)=1, activate(previousfield)=0 ); 

        if( new )
        {
	    disp = ERget( S_ST0535_define_new_attr );
	    exec frs putform attribedit 
		( attributename='', attributevalue='' );
        }
        else
        {
	    disp = ERget( S_ST0536_edit_attr );
	    exec frs putform attribedit 
	      ( attributename = :name, 
	        attributevalue = :value );
        }

	exec frs putform attribedit( infoline=:disp );
    
	if( new )
           disp = edit_global ? global_word: private_word;
	else
           disp = *privglob ? private_word: global_word;
        exec frs putform attribedit( f_privglob = :disp );
    exec frs end;

    exec frs activate menuitem :ERget( FE_Save ), frskey4;
    exec frs begin;
        exec frs inquire_frs form ( :changed = change( attribedit ) );
	if( !changed )
        { 
	    *name = *value = EOS;
	    rtn   = EDIT_EMPTY;
	    exec frs breakdisplay;
	}

        exec frs getform attribedit 
	    ( :lcl_privglob = f_privglob, 
	      :lcl_name     = attributename, 
	      :lcl_value    = attributevalue ); 

	if( *lcl_privglob == EOS )  
	{ 
	    IIUGerr( E_ST000B_p_or_g, 0, 0 );
	    exec frs resume field f_privglob; 
	} 

	if( *lcl_name == EOS )  
	{
            IIUGerr( E_ST0137_attribname_req, 0, 0 );
	    exec frs resume field attributename;
	} 
	
	if( *lcl_value == EOS ) 
	{
            IIUGerr( E_ST0138_attribval_req, 0, 0 );
	    exec frs resume field attributevalue;
        } 

	*privglob = lcl_type;
        STcopy( lcl_name, name );
	STcopy( lcl_value, value );

	if( nu_locate_attr( vnode, lcl_type, name, value, &rtn ) != NULL )
        { 
	    rtn = -rtn; 
	    IIUGerr( E_ST020C_duplicate_attr, 0, 0 );
	} 
	else 
	    rtn = EDIT_OK; 
        exec frs breakdisplay;
    exec frs end;

    exec frs activate menuitem :ERget( FE_Cancel ), frskey9 ( activate = 0 );
    exec frs begin;
        rtn = EDIT_CANCEL;
        exec frs breakdisplay;
    exec frs end;

    exec frs activate menuitem :ERget( FE_Help ), frskey1;
    exec frs begin;
        FEhelp( ERx( "attredit.hlp" ), ERx( "Netutil - Attribute data" ) );
    exec frs end;

    exec frs activate after field f_privglob;
    exec frs begin;
	exec sql begin declare section;
	char privglob[12];
	exec sql end declare section;

        exec frs getform attribedit ( :privglob = f_privglob );

	if( nu_is_private( privglob ) )
	{
	    exec frs putform attribedit( f_privglob = :private_word );
	    lcl_type = TRUE;
	}
	else if( !CMcmpnocase( privglob, global_word ) )
	{
	    lcl_type = FALSE;
	    exec frs putform attribedit( f_privglob = :global_word );
	}
	else
	{
	    IIUGerr( E_ST000B_p_or_g, 0, 0 );
	    exec frs resume field f_privglob;
	}
	exec frs resume next;
    exec frs end;

    exec frs activate before field attributename;
    exec frs begin;
	exec sql begin declare section;
	char privglob[12];
	exec sql end declare section;

        exec frs getform attribedit ( :privglob = f_privglob );

	if( nu_is_private( privglob ) )
	{
	    exec frs putform attribedit( f_privglob = :private_word );
	    lcl_type = TRUE;
	}
	else if( !CMcmpnocase( privglob, global_word ) )
	{
	    lcl_type = FALSE;
	    exec frs putform attribedit( f_privglob = :global_word );
	}
	else
	{
	    IIUGerr( E_ST000B_p_or_g, 0, 0 );
	    exec frs resume field f_privglob;
	}
	exec frs resume next;
    exec frs end;

    exec frs finalize;
    return rtn;
}

/*
**  Name: nu_locate_attr() 
**
**  Description:
**  	Loops through the internal data structure looking for an attribute
**  	entry that matches the criteria specified by the input arguments.
**  	This has the effect of making the corresponding member of the internal
**  	list the "current" member, which is convenient if it needs to be
**  	deleted or modified.  This function can also be used as a simple
**  	predicate to see if the specified entry currently exists or not.
**  
**  Input:
**    vnode 		The virtual node name
**    private 		TRUE for a private entry, FALSE for a global
**    name 		the attr_name }
**    value		the attr_value}--> of the entry being searched for
**    row 		may be NULL; if not, points to i4  which receives 
**			the row number of the match, if any.
**
**  Output:
**	None.
**
**  Returns:
**    Pointer to attribute entry in internal data structure.
**    
** History:
**	08-Aug-97 (rajus01)
**	    Created.
*/
ATTR *
nu_locate_attr( char *vnode, i4  private, 
	       char *name, char *value, i4  *row )
{
    bool 	flag;
    char 	*nam, *val;
    i4  	pv, lrow;
    ATTR 	*rtn;

    /* Loop through the data structure looking for a match... */

    for( flag=TRUE, lrow=1; /* "flag" just says to rewind list the 1st time */
	  NULL != ( rtn = nu_vnode_attr( flag, &pv, vnode, &nam, &val ) ); 
          flag=FALSE, lrow++ )
    {
        if ( pv == private  &&
             !STbcompare( name,  0, nam, 0, FALSE ) &&
	     !STbcompare( value,   0, val, 0, FALSE ) ) 
	{
	    /* Found one. */
	    if( row != NULL ) 
		*row = lrow;
	    return rtn;
	}
    }
    return NULL;
}
