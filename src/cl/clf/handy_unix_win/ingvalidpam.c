/*
** Copyright (c) 2006, 2008 Ingres Corporation 
*/

# include <sys/types.h>
# include <sys/stat.h>
# include <string.h>
# include <stdio.h>
# include <signal.h>
# include <errno.h>
# include <unistd.h>
# include <stdlib.h>
# include <ctype.h>
# include <security/pam_appl.h>
# include "ingpwutil.h"

#define DEFAULT_PAM_SERVICE "ingres"

/*
** Name: ingvalidpam.c
**
** Description: This program is used to validate a given userid/password
**		pair using PAM authentication API. 
**
** 
PROGRAM = ingvalidpam 
**
** History:
**      04-Sep-2005 (R.J external/pickr01)
**	   ORIGINAL CONTRIBUTION FROM THE COMMUNITY:
**         Added PAM authentication if requested(compiled with -DUSE_PAM)
**         Use the standard "login" authentication as defined in the
**         PAM configuration. (#define DEFAULT_PAM_SERVICE "login")
**         Changes:
**         The password check is now done in an additional function, which 
**         is either check_passwd(the old method) or check_pam_passwd(PAM)
**         Added Return values E_PAM_INIT (pam_start failed) and 
**         E_PAM_AUTH (pam authentication failed)
**         In DEBUG mode(II_INGVALIDPW_LOG) the errors returned by the
**         PAM functions are printed out.
**     21-May-2008 (Usha) SIR 120420, SD issue 116370
**	    Verified the community contribution and integrated it to the 
**	    Ingres source library. Did some code cleanup. 
**	
*/

int check_pam_passwd(char * username, char * password, FILE * fp);
int password_conversation(int num_msg, const struct pam_message **msg,
				 struct pam_response **resp, void *appdata_ptr);

/*
** The application data that gets passed to PAM conversation function.
*/
struct appdata {
	char * password;
	FILE * fp;
};

/*
** The PAM conversation function is specified using 'pam_conv' structure.
*/

static struct pam_conv conv =
{
    &password_conversation,
    (void *) NULL
};



main( argc, argv )
int	argc;
char	*argv[];
{
    char    user[USR_LEN+1];
    char    password[PWD_LEN+1];
    FILE    *fp = NULL;
    int	    status = 0;

    status = getDebugFile(&fp);
    if( status != E_OK )
       return( status );
    status = getUserAndPasswd(user, password, fp);
    if( status != E_OK )
       return( status );
    return ( check_pam_passwd (user, password, fp) );
}

/*
** Name: password_conversation - The PAM conversation function.
**
** Description: 
**	PAM library has a mechanism in the form of PAM conversation
**	function to send and receive text data non-interactively. 
**	This function receives a list of code/string pairs from PAM service 
**	module.	Among the list of codes received, the codes PAM_ECHO_ON and 
**	PAM_ECHO_OFF are used to get input from the user (the Ingres process 
**	in this case ). Typically the code PAM_ECHO_ON is used to get username
**	and PAM_ECHO_OFF is used to get password. The code PAM_ECHO_ON is not 
**	used in this conversation function because the username is directly 
**	passed to pam_start() function.
**
**	This convesation function is registered with PAM when the PAM session 
**	is initiated by the Ingres process by calling pam_start() function.
**
**	Input:
**	    num_msg   The number of messages being passed to this function 
**		      from PAM.(values range between 0 and PAM_MAX_NUM_MSG 
**		      inclusive) 
**	    msg       ptr to a buffer that holds messages to the user. 
**	    app_data  points a buffer containing application data. 
**		   
**	Output: 
**	    resp      ptr to a buffer that holds messages from the user. 
**
**	Returns:
**	    int       PAM_SUCCESS or error code.
**
**      Typically the PAM service modules configured for Ingres is responsible
**	for allocatiing and freeing the memory used by 'msg'. But the memory
**	for 'resp' is allocated by Ingres and freed by PAM service modules.	
**
**	PAM messages are stored in pam_message structure (code/string pair
**	referred above ).
**	struct pam_message
**	{
**          int msg_style;
**          char *msg;
**	};
**
**	The msg_style can be one of the following:
**
** 	PAM_PROMPT_ECHO_OFF: Prompt the user, disabling echoing of their response. 
**      PAM_PROMPT_ECHO_ON: Prompt the user, echoing their response. 
**	PAM_ERROR_MSG: Print an error message. 
**	PAM_TEXT_INFO: Print a general information message. 
**	
**	History: 
**	   21-May-2008 (Usha) SIR 120420, SD issue 116370
**	      Added the above function description. 
**    
*/
int
password_conversation(int num_msg, const struct pam_message **msg, 
		      struct pam_response **resp, void *appdata_ptr)
{
    struct appdata *data;
    FILE *fp;
    char *password;
    int i;

    if (!appdata_ptr)
	return PAM_CONV_ERR;
    data = (struct appdata *) appdata_ptr;
    fp = (*data).fp;
    password = data->password;

    if (num_msg != 1 || msg[0]->msg_style != PAM_PROMPT_ECHO_OFF)
    {
#ifdef xDEBUG
        for (i = 0; i < num_msg; i++) 
             INGAUTH_DEBUG_MACRO(fp,0, "Message %d : %s\n", i, msg[i]->msg);
#endif
	return PAM_CONV_ERR;
    }
    *resp = calloc(num_msg, sizeof(struct pam_response));
    if (!*resp)
    {
        INGAUTH_DEBUG_MACRO(fp, 1, "Out of memory error\n");
	return PAM_CONV_ERR;
    }
    (*resp)[0].resp = strdup((char *) password);
    (*resp)[0].resp_retcode = 0;

    return ((*resp)[0].resp ? PAM_SUCCESS : PAM_CONV_ERR);
}

/*
** Name: check_pam_passwd - Makes calls to PAM service to authenticate the user.
**
**	The PAM APIs called by Ingres process are:
**
**	pam_start  Initiates PAM authentication transaction for the service and
**		   the user specified by the service and user respectively. The
**		   'conv' argument specifies the conversation function to be 
**		   used. On success, it returns PAM handle 'pamh' which must 
**	 	   be used with subsequent PAM functions.
**
**	pam_authenticate   Authenticates the current user specified in 
**			   pam_start().
**	pam_acct_mgmt      Performs account validity checks.
**	pam_end 	   Terminates PAM authentication transaction.
**
**	Input:
**	   username   User to be authenticated.
**	   password   User password.
**	   fp	      File pointer to log file for debugging.
**
**	Output:
**	   None.
**
**	Returns: E_OK or PAM error code.
**	
**	History:
**	   21-May-2008 (Usha) SIR 120420, SD issue 16370
**	      Added the above description.
*/
int
check_pam_passwd(char * username, char * password, FILE * fp)
{
    pam_handle_t	*pamh = NULL;
    int 		retval = PAM_SUCCESS;
    char		*service = DEFAULT_PAM_SERVICE;
    struct appdata 	data;

    data.password = password;
    data.fp = fp;

    conv.appdata_ptr = (void *) &data;

    /* Create PAM connection */
    retval = pam_start(service, username, &conv, &pamh);
    if (retval != PAM_SUCCESS)
    {
	INGAUTH_DEBUG_MACRO(fp, 1,
		"Error: failed to create PAM authenticator (%s), service %s\n",
		 pam_strerror(pamh, retval), DEFAULT_PAM_SERVICE);
	return E_INIT;
    }

    retval = pam_authenticate(pamh, 0);
    if (retval != PAM_SUCCESS)
    {
	INGAUTH_DEBUG_MACRO(fp, 1,
		 "Error: pam_authenticate returned %d (%s), service %s\n",
		 retval, pam_strerror(pamh, retval), DEFAULT_PAM_SERVICE);
        return E_AUTH;
    }

    retval = pam_acct_mgmt(pamh, 0);
    if (retval != PAM_SUCCESS)
    {
	INGAUTH_DEBUG_MACRO(fp, 1,
			 "Error: pam_acct_mgmt returned %d (%s), service %s\n",
			 retval, pam_strerror(pamh, retval),
			 DEFAULT_PAM_SERVICE);
        return E_EXPIRED;
    }

    pam_end(pamh, retval);
    INGAUTH_DEBUG_MACRO(fp,1,"return E_OK (PAM, service %s)\n",
			DEFAULT_PAM_SERVICE);
    return E_OK;
}
