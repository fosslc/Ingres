/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: gcs.h
**
** Description:
**	GCF security interface definition.
**
** History:
**	16-May-97 (gordy)
**	    Created.
**	 5-Aug-97 (gordy)
**	    Added support for dynamically loading mechanisms.
**	20-Aug-97 (gordy)
**	    Reworked the config strings.  Added server specific
**	    mechanism list (so encryption mechanisms will only
**	    be loaded by comm server).
**	22-Oct-97 (rajus01)
**	    Added mech_ovhd for encryption overhead.
**	20-Feb-98 (gordy)
**	    Defined MO objects.
**	17-Mar-98 (gordy)
**	    Use system prefix in config symbols.  Added config symbol
**	    to specify library path for a mechanism.
**	20-Mar-98 (gordy)
**	    Added mechanism encryption level.
**	14-May-98 (gordy)
**	    Added utility functions and host to remote authentication.
**	26-Jul-98 (gordy)
**	    Modified config parameters to work better with CBF.
**	21-Aug-98 (gordy)
**	    Added ability to delegate authorization from user and remote
**	    authentications (GCS_OP_VALIDATE) and generate new remote
**	    authentications for the validated user.
**	 4-Sep-98 (gordy)
**	    Added OP_RELEASE to free mechanism resources.
**	 4-May-99 (rajus01)
**	    Added KERBEROS mechanism.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Jun-04 (gordy)
**	    Added error codes GC100A and GC100B for expiration times.
**	 9-Jul-04 (gordy)
**	    Expanded mechanism configuration for individual auth mechanisms.
**	    Made IP Auth a distinct GCS operation.
**	 6-Dec-05 (gordy)
**	    Added E_GC100D_SRV_AUTH_FAIL.
**       03-Aug-2007 (Ralph Loen) SIR 118898
**          Added E_GC1200_KRB_GSS_API_ERR.
**      31-Oct-07 (Ralph Loen) Bug 119358
**         Added E_GC1027_GCF_MECH_DISABLED and E_GC1028_GCF_MECH_UNSUPPORTED.
**	28-Mar-08 (rajus01) SD issue 126704, Bug 120136
**	    Added E_GC1029_GCS_PASSWORD_LEN.
*/

#ifndef _GCS_H_INCLUDED_
#define _GCS_H_INCLUDED_

/*
** Security Mechanisms.
*/
typedef	u_i1	GCS_MECH;

#define	GCS_MECH_MAX		255	/* Max value and # of array entries */

typedef struct
{

    char	*mech_name;	/* Mechanism unique string identifier */
    GCS_MECH	mech_id;	/* Mechanism unique numeric identifier */

#define GCS_MECH_NULL		0	/* No security, validation disabled */
#define	GCS_MECH_SYSTEM		1	/* Original Ingres O.S. security */
#define	GCS_MECH_INGRES		2	/* Default Ingres security */
#define	GCS_MECH_KERBEROS	200	/* Public domain Kerberos security */
#define GCS_NO_MECH		255	/* Generic processing */

    i4		mech_ver;	/* GCS API version supported by mechanism */

#define	GCS_MECH_VERSION_1	1	/* Initial version */
#define	GCS_MECH_VERSION_2	2	/* IP Auth */

    u_i4	mech_caps;	/* Capabilities (defined wih GCS operations) */

#define	GCS_CAP_USR_AUTH	0x0001	/* User Authentication */
#define	GCS_CAP_PWD_AUTH	0x0002	/* Password Authentication */
#define	GCS_CAP_SRV_AUTH	0x0004	/* Server Authentication */
#define	GCS_CAP_IP_AUTH		0x0010	/* Installation Passwords */
#define	GCS_CAP_REM_AUTH	0x0020	/* Remote Authentication */
#define	GCS_CAP_ENCRYPT		0x0040	/* Encryption */

    i4		mech_status;	/* Accessibility status */

#define	GCS_DISABLED		0	/* Disabled */
#define	GCS_AVAIL		1	/* Available */

    i4		mech_enc_lvl;	/* Encryption Level */

#define	GCS_ENC_LVL_0		0	/* No encryption */
#define	GCS_ENC_LVL_1		1	/* Text scrambling */
#define	GCS_ENC_LVL_2		2	/* Encryption */

    i4		mech_ovhd;	/* Encryption Overhead */

} GCS_MECH_INFO;


/*
** Configuration strings.
*/

#define	GCS_CNF_MECHANISM	"%s.%s.gcf.security_mechanism"	/* mech name */
#define	GCS_CNF_USR_MECH	"%s.%s.gcf.user_mechanism"	/* mech name */
#define	GCS_CNF_RSTRCT_USR_AUTH	"%s.%s.gcf.restrict_usr_auth"	/* true/false */
#define	GCS_CNF_PWD_MECH	"%s.%s.gcf.password_mechanism"	/* mech name */
#define	GCS_CNF_RSTRCT_PWD_AUTH	"%s.%s.gcf.restrict_pwd_auth"	/* true/false */
#define	GCS_CNF_SRV_MECH	"%s.%s.gcf.server_mechanism"	/* mech name */
#define	GCS_CNF_RSTRCT_SRV_AUTH	"%s.%s.gcf.restrict_srv_auth"	/* true/false */
#define	GCS_CNF_REM_MECH	"%s.%s.gcf.remote_mechanism"	/* mech name */
#define	GCS_CNF_RSTRCT_REM_AUTH	"%s.%s.gcf.restrict_rem_auth"	/* true/false */
#define GCS_CNF_PATH		"%s.%s.gcf.mechanism_location"	/* directory */
#define GCS_CNF_LOAD		"%s.%s.gcf.mechanisms"		/* mech list */
#define GCS_CNF_SRV_LOAD	"!.mechanisms"			/* mech list */
#define GCS_CNF_MECH_ENABLED	"%s.%s.gcf.mech.%s.enabled"	/* true,false */
#define	GCS_CNF_MECH_PATH	"%s.%s.gcf.mech.%s.location"	/* directory */
#define GCS_CNF_MECH_MODULE	"%s.%s.gcf.mech.%s.module"	/* library */
#define GCS_CNF_MECH_ENTRY	"%s.%s.gcf.mech.%s.entry"	/* function */


/*
** Name: GCS_OP_INIT
**
** Description:
**	Initialize GCS and all security mechanisms.  May be requested
**	more than once but must be paired with a matching GCS_OP_TERM 
**	request.  Input parameters are only referrenced on the initial
**	request.  Specify GCS_NO_MECH as mechanism.
**
**	Callback functions provide the ablitity for the GCS client to
**	extend and control GCS functionality.
**	
**	PTR mem_alloc_func( u_i4 length )
**
**	    Callback function to dynamically allocate memory.
**
**	    Input:	length	Amount of memory to allocate.
**	    Returns:	PTR	The memory allocated, NULL if error.
**
**	VOID mem_free_func( PTR addr )
**
**	    Callback function to free dynamically allocated memory.
**
**	    Input:	addr	Dynamically allocated memory to be freed.
**
**	VOID err_log_func( STATUS error_code, i4  parm_count, PTR parms )
**
**	    Callback function to log errors defined by an Ingres error 
**	    code and optional parameter values.
**
**	    Input:	error_code	Standard Ingres error code.
**			parm_count	Number of parameters.
**			parms		Array of ER_ARGUMENT values.
**
**	VOID msg_log_func( char * )
**
**	    Callback function to log message strings.
**
**	    Input:	message		Message text string.
**
** Input:
**	version		GCS API version used by GCS client.
**	mem_alloc_func	Function to allocate memory, may be NULL.
**	mem_free_func	Function to free allocated memory, may be NULL.
**	err_log_func	Function for logging errors, may be NULL.
**	msg_log_func	Function for logging messages, may be NULL.
**
** Output:
**	None.
*/

#define		GCS_OP_INIT		1	/* Initialize */

typedef struct _GCS_INIT_PARM
{

    i4		version;		/* Caller API version */

#define		GCS_API_VERSION_1	1	/* Initial version */
#define		GCS_API_VERSION_2	2	/* IP Auth */

    PTR		mem_alloc_func;		/* Dynamically allocate memory */
    PTR		mem_free_func;		/* Free allocated memory */
    PTR		err_log_func;		/* Log an error */
    PTR		msg_log_func;		/* Log a message */

} GCS_INIT_PARM;


/*
** Name: GCS_OP_TERM
**
** Description:
**	Terminate GCS and all security mechanisms.  Each GCS_OP_INIT
**	request requires a matching GCS_OP_TERM request.  Specify 
**	GCS_NO_MECH as mechanism.
**
** Input:
**	None.
**
** Output:
**	None.
*/

#define		GCS_OP_TERM		2	/* Terminate */


/*
** Name: GCS_OP_INFO
**
** Description:
**	Describes attributes of all security mechanisms.  Specify
**	GCS_NO_MECH as mechanism.
**
** Input:
**	None.
**
** Output:
**	mech_count	Number of security mechanisms.
**	mech_info	Array of mechanism information.
*/

#define		GCS_OP_INFO		3	/* Information */

typedef struct _GCS_INFO_PARM
{

    i4			mech_count;
    GCS_MECH_INFO	**mech_info;

} GCS_INFO_PARM;


/*
** Name: GCS_OP_SET
**
** Description:
**	Set GCS operational parameters.  Permits configuration
**	of GCS through definable parameter values.  Each parm
**	is identified by an ID and takes a single value (type
**	and size dependent on parm).
**
**	GET_KEY_FUNC: Function pointer (sizeof( PTR ))
**		
**	    Function to be called during server auth validation
**	    to obtain server key associated with a server ID.
**
**	USR_PWD_FUNC: Function pointer (sizeof( PTR ))
**
**	    Function to be called to validate a user ID and password.
**
**	IP_FUNC: Function pointer (sizeof( PTR ))
**
**	    Function to be called to validate installation password tickets.
**
**	Parameters are further described below.
**
** Input:
**	parm_id		Parameter identifier.
**	length		Length of parameter value.
**	buffer		Parameter value.
**
** Output:
**	None.
**
*/

#define		GCS_OP_SET		5	/* Set GCS parm value */

typedef struct _GCS_SET_PARM
{
    i4		parm_id;

#define GCS_PARM_GET_KEY_FUNC	1	/* Server key retrieval function */
#define GCS_PARM_USR_PWD_FUNC	2	/* Validate user/password function */
#define GCS_PARM_IP_FUNC	3	/* Validate inst. password tickets */

    i4		length;		/* Length of value */
    PTR		buffer;		/* Parameter value */

} GCS_SET_PARM;


/*
** Name: GCS_PARM_GET_KEY_FUNC 
**
** Description:
**	Defines a callback function to GCS which will be called 
**	during server auth validation to obtain the server key 
**	with which the authentication was created.  The parm
**	value is a function pointer for the client provided 
**	function having the following prototype:
**
**	STATUS get_key_func( GCS_GET_KEY_PARMS * )
**
**	Input:
**		server		Server ID assigned to server key.
**		length		Size of buffer.
**		buffer		Buffer to receive key.
**
** 	Output:
**		length		Length of key (not including null terminator).
**
** 	Returns:
**		STATUS		OK or FAIL.
**
**	The function parameters provide the server ID used to
**	identify the server key and a buffer to receive the key.
**	If the key for the indicated server ID is not available,
**	or the output buffer is too small for the key, a status
**	of FAIL should be returned.  Otherwise, OK should be
**	returned.
*/

typedef struct _GCS_GET_KEY_PARM
{

    char	*server;	/* Server ID */
    i4		length;		/* Size of buffer - length of key */
    char	*buffer;	/* Buffer to receive key */

} GCS_GET_KEY_PARM;


/*
** Name: GCS_PARM_USR_PWD_FUNC 
**
** Description:
**	Defines a callback function to GCS which will be called 
**	during password auth validation to validate a user ID
**	and password.  The parm value is a function pointer for 
**	the client provided function having the following prototype:
**
**	STATUS usr_pwd_func( GCS_USR_PWD_PARMS * )
**
**	Input:
**		user		User ID.
**		password	Password.
**
** 	Output:
**		None.
**
** 	Returns:
**		STATUS		OK, FAIL or error code.
**
**	The user ID and password should be validated and OK returned
**	if valid.  If the user ID/password pair is invalid, a GCF
**	error code, such as E_GC000B, should be returned.  If the
**	validity of the user ID and password cannot be determined,
**	FAIL should be returned and default GCS password validation
**	will be performed.
*/

typedef struct _GCS_USR_PWD_PARM
{

    char	*user;		/* User ID */
    char	*password;	/* Password */

} GCS_USR_PWD_PARM;

/*
** Name: GCS_PARM_IP_FUNC 
**
** Description:
**	Defines a callback function to GCS which will be called 
**	during user authentication to validate an installation
**	password ticket.  The parm value is a function pointer 
**	for the client provided function having the following 
**	prototype:
**
**	STATUS ip_func( GCS_IP_PARMS * )
**
**	Input:
**		user		User ID.
**		length		Length of ticket.
**		ticket		Ticket.
**
** 	Output:
**		None.
**
** 	Returns:
**		STATUS		OK, FAIL or error code.
*/

typedef struct _GCS_IP_PARM
{
    char	*user;		/* User ID */
    i4		length;		/* Length of ticket */
    PTR 	ticket; 	/* Ticket */

} GCS_IP_PARM;

/*
** Name: GCS_OP_VALIDATE
**
** Description:
**	Validates an authentication created by GCS_OP_USR_AUTH,
**	GCS_OP_PWD_AUTH, GCS_OP_SRV_AUTH or GCS_OP_REM_AUTH.  A
**	user alias must be provided.  If client does not request
**	a change of ID, alias should be same as client user ID.
**	Specifying GCS_NO_MECH as mechanism will result in the
**	validation request being passed to the mechanism which 
**	created the authentication.
**
**	Some security mechanisms can produce an authentication
**	token from a user or remote authentication which may be
**	used to generate a new remote authentication for the 
**	user validated by this routine.  This functionality is
**	optional and will be provided (when possible) if an
**	output token buffer is provided.
**
** Input:
**	user		User ID of requesting client process.
**	alias		User alias requested by client.
**	length		Length of authentication.
**	auth		Buffer containing authentication.
**	size		Size of output buffer.
**	buffer		Buffer to receive token.
**
** Output:
**	size		Length of token.
*/

#define		GCS_OP_VALIDATE		6	/* Validate authentication */

typedef struct _GCS_VALID_PARM
{

    char	*user;		/* Client user ID */
    char	*alias;		/* Requested user alias */
    i4		length;		/* Length of authentication */
    PTR		auth;		/* Authentication */
    i4		size;		/* Size of buffer - length of token */
    PTR		buffer;		/* Authentication token */

} GCS_VALID_PARM;


/*
** GCS_OP_USR_AUTH
**
** Description:
**	Creates a user authentication for the user ID associated
**	with the current process.  Specifying GCS_NO_MECH as
**	mechanism will pass request to the configured installation
**	or default mechanism.  Mechanism must have GCS_CAP_USR_AUTH
**	capability.
**
** Input:
**	length		Size of output buffer.
**	buffer		Buffer to receive authentication.
**
** Output:
**	length		Length of authentication.
*/

#define		GCS_OP_USR_AUTH		10	/* Generate user auth */

typedef struct
{

    i4		length;		/* Size of buffer - Length of auth */
    PTR		buffer;		/* Buffer to receive authentication */

} GCS_USR_PARM;


/*
** GCS_OP_PWD_AUTH
**
** Description:
**	Creates a password authentication for provided password.
**	Password will be validated along with the user alias when 
**	GCS_OP_VALIDATE is requested.  Specifying GCS_NO_MECH as 
**	mechanism will pass request to the configured installation
**	or default mechanism.  Mechanism must have GCS_CAP_PWD_AUTH
**	capability.
**
** Input:
**	user		User alias.
**	password	Password associated with user alias.
**	length		Size of output buffer.
**	buffer		Buffer to receive authentication.
**
** Output:
**	length		Length of authentication.
*/

#define		GCS_OP_PWD_AUTH		11	/* Generate password auth */

typedef struct
{

    char	*user;		/* User alias */
    char	*password;	/* Password */
    i4		length;		/* Size of buffer - Length of auth */
    PTR		buffer;		/* Buffer to receive authentication */

} GCS_PWD_PARM;


/*
** Name: GCS_OP_SRV_KEY
**
** Description:
**	Creates a random alpha-numeric key to be used for 
**	GCS_OP_SRV_AUTH requests.  Keys must be shared 
**	between servers which perform server authentication.  
**	The key is null terminated.  The returned key length
**	does not include the null terminator.  Specifying 
**	GCS_NO_MECH as mechanism will pass request to the 
**	configured installation or default mechanism.  
**	Mechanism must have GCS_CAP_SRV_AUTH capability.
**
** Input:
**	length		Size of output buffer.
**	buffer		Buffer to receive key.
**
** Output:
**	length		Length of key.
*/

#define		GCS_OP_SRV_KEY		12	/* Generate server key  */

typedef struct
{

    i4		length;		/* Size of buffer - Length of key */
    char	*buffer;	/* Buffer to receive key */

} GCS_KEY_PARM;


/*
** Name: GCS_OP_SRV_AUTH
**
** Description:
**	Creates a server authentication for the requested user and alias 
**	which requires the specified key for validation.  The server ID
**	specified here will be used during validation to obtain the key.
**	Specifying GCS_NO_MECH as mechanism will pass request to the 
**	configured installation or default mechanism.  Mechanism must 
**	have GCS_CAP_SRV_AUTH capability.
**
** Input:
**	user		User ID.
**	alias		User alias.
**	server		Server ID.
**	key		Server key.
**	length		Size of output buffer.
**	buffer		Buffer to receive authentication.
**
** Output:
**	length		Length of authentication.
*/

#define		GCS_OP_SRV_AUTH		13	/* Generate server auth */

typedef struct _GCS_SRV_PARM
{

    char	*user;		/* User ID */
    char	*alias;		/* User alias */
    char	*server;	/* Server ID */
    char	*key;		/* Server key */
    i4		length;		/* Size of buffer - Length of auth */
    PTR		buffer;		/* Buffer to receive authentication */

} GCS_SRV_PARM;

/*
** Name: GCS_OP_REM_AUTH
**
** Description:
**	Creates a remote authentication for the given host and
**	optional authentication token. Mechanism must have 
**	GCS_CAP_REM_AUTH.
**
** Input:
**	host		Remote target host ID.
**	length		Length of authentication token.
**	token		Authentication token.
**	size		Size of output buffer.
**	buffer		Buffer to receive authentication.
**
** Output:
**	size		Length of authentication.
*/

#define		GCS_OP_REM_AUTH		14	/* Generate remote auth */

typedef struct
{
    char	*host;		/* Remote host ID */
    i4		length;		/* Length of authentication token */
    PTR		token;		/* Optional authentication token */
    i4		size;		/* Size of buffer - length of authentication */
    PTR		buffer;		/* Buffer to receive authentication */

} GCS_REM_PARM;

/*
** Name: GCS_OP_RELEASE
**
** Description:
**	Free any mechanism resources associated with GCS object.
**	May be called for any authentication token and the token
**	output from GCS_OP_VALIDATE.  Should not be called for
**	server keys and encryption objects.
**
** Input:
**	length		Length of token.
**	token		Token.
**
** Output:
**	None.
*/

#define		GCS_OP_RELEASE		15	/* Release resources */

typedef struct
{
    i4		length;		/* Length of token */
    PTR		token;		/* Token */

} GCS_REL_PARM;

/*
** Name: GCS_OP_IP_AUTH
**
** Description:
**	Creates an installation password authentication for 
**	the given host and installation password ticket. 
**	Mechanism must have GCS_CAP_IP_AUTH.
**
**	Note: uses GCS_REM_PARM for input.
**
** Input:
**	host		Remote target host ID.
**	length		Length of installation password ticket.
**	token		Installation password ticket.
**	size		Size of output buffer.
**	buffer		Buffer to receive authentication.
**
** Output:
**	size		Length of authentication.
*/

#define		GCS_OP_IP_AUTH		16	/* Generate IP auth */

/*
** Name: GCS_OP_E_INIT
**
** Description:
**	Initialize an encryption session.  This request must be followed
**	by a GCS_OP_E_CONFIRM or GCS_OP_E_TERM request.  The initiating
**	partner must provide an identification of the peer partner for
**	the encryption session.  An encryption session control block is
**	returned which must be used in all subsequent requests related
**	to this encryption session.  Do not specify GCS_NO_MECH as 
**	mechanism.  Mechanism must have GCS_CAP_ENCRYPT capability.
**
**	A token exchange between the communicating partners is required.  
**	The first token is provided as output from this request by the 
**	initiating partner, who must pass the token to the peer partner,
**	to be provided as input to the peer's GCS_OP_E_INIT request.  A 
**	second token, processed using GCS_OP_E_CONFIRM, confirming the 
**	session establishment must also be exchanged.
**
** Input:
**	initiator	TRUE if initiating partner, FALSE if peer.
**	peer		initiator: Peer ID (host name); peer None.
**	length		initiator: length of buffer; peer: length of token.
**	buffer		initiator: buffer for token; peer: token.
**
** Output:
**	length		initiator: length of token; peer: None.
**	escb		Encryption session control block.
*/

#define		GCS_OP_E_INIT		20	/* Initialize session */

typedef struct _GCS_EINIT_PARM
{

    PTR		escb;		/* Encryption session control block */
    bool	initiator;	/* TRUE if initiating partner, FALSE if peer */
    char	*peer;		/* Peer ID */
    i4		length;		/* Length of token/buffer */
    PTR		buffer;		/* Buffer for token */

} GCS_EINIT_PARM;


/*
** Name: GCS_OP_E_CONFIRM
**
** Description:
**	Confirm establishment of an encryption session.  This request
**	may be followed by GCS_OP_E_ENCODE and GCS_OP_E_DECODE requests.
**	It must eventually be followed by a GCS_OP_E_TERM request.  Do 
**	not specify GCS_NO_MECH as mechanism.  Mechanism must have 
**	GCS_CAP_ENCRYPT capability.
**
**	A token exchange between the communicating partners is required.  
**	The first token is processed using GCS_OP_E_INIT.  The second 
**	token is provided as output from this request by the peer partner, 
**	who must pass the token to the initiating partner, to be provided 
**	as input to the initiator's GCS_OP_E_CONFIRM request.
**
** Input:
**	escb		Encryption session control block.
**	initiator	TRUE if initiating partner, FALSE if peer.
**	length		initiator: length of token; peer: length of buffer.
**	buffer		initiator: token; peer: buffer for token.
**
** Output:
**	length		initiator: None; peer: length of token.
*/

#define		GCS_OP_E_CONFIRM	21	/* Peer session confirmation */

typedef struct _GCS_ECONF_PARM
{

    PTR		escb;		/* Encryption session control block */
    bool	initiator;	/* TRUE if initiating partner, FALSE if peer */
    i4		length;		/* Length of token/buffer */
    PTR		buffer;		/* Buffer for token */

} GCS_ECONF_PARM;


/*
** Name: GCS_OP_E_ENCODE
**
** Description:
**	Encrypt a data block to be exchanged in the context of an
**	encryption session.  The resulting length of the encrypted
**	data may be different than the unencrypted length.  Do not 
**	specify GCS_NO_MECH as mechanism.  Mechanism must have 
**	GCS_CAP_ENCRYPT capability.
**
** Input:
**	escb		Encryption session control block.
**	size		Length of buffer.
**	length		Length of unencrypted data.
**	buffer		Buffer to be encrypted.
**
** Output:
**	length		Length of encrypted data.
*/

#define		GCS_OP_E_ENCODE		22	/* Encrypt data */

typedef struct _GCS_EENC_PARM
{

    PTR		escb;		/* Encryption session control block */
    i4		size;		/* Length of buffer */
    i4		length;		/* Length of unencrypted - encrypted data */
    PTR		buffer;		/* Buffer to be encrypted */

} GCS_EENC_PARM;


/*
** Name: GCS_OP_E_DECODE
**
** Description:
**	Decrypt a data block exchanged in the context of an encryption 
**	session.  The resulting length of the decrypted data may be 
**	different than the encrypted length.  Do not specify GCS_NO_MECH 
**	as mechanism.  Mechanism must have GCS_CAP_ENCRYPT capability.
**
** Input:
**	escb		Encryption session control block.
**	size		Length of buffer.
**	length		Length of encrypted data.
**	buffer		Buffer to be decrypted.
**
** Output:
**	length		Length of decrypted data.
*/

#define		GCS_OP_E_DECODE		23	/* Decrypt data */

typedef struct _GCS_EDEC_PARM
{

    PTR		escb;		/* Encryption session control block */
    i4		size;		/* Length of buffer */
    i4		length;		/* Length of encrypted - decrypted data */
    PTR		buffer;		/* Buffer to be decrypted */

} GCS_EDEC_PARM;


/*
** GCS_OP_E_TERM
**
** Description:
**	Terminate an encryption session and free resources.  Do not 
**	specify GCS_NO_MECH as mechanism.  Mechanism must have 
**	GCS_CAP_ENCRYPT capability.
**
** Input:
**	escb		Encryption session control block.
**
** Output:
**	None.
*/

#define		GCS_OP_E_TERM		24	/* Terminate session */

typedef struct _GCS_ETERM_PARM
{

    PTR		escb;		/* Encryption session control block */

} GCS_ETERM_PARM;


/*
** GCS Error codes.
*/

#define	E_GC1000_GCS_FAILURE		(STATUS)(E_GCF_MASK + 0x1000)
#define E_GC1001_GCS_NOT_INITIALIZED	(STATUS)(E_GCF_MASK + 0x1001)
#define E_GC1002_GCS_OP_UNKNOWN		(STATUS)(E_GCF_MASK + 0x1002)
#define E_GC1003_GCS_OP_UNSUPPORTED	(STATUS)(E_GCF_MASK + 0x1003)
#define E_GC1004_SEC_MECH_UNKNOWN	(STATUS)(E_GCF_MASK + 0x1004)
#define E_GC1005_SEC_MECH_DISABLED	(STATUS)(E_GCF_MASK + 0x1005)
#define E_GC1006_SEC_MECH_FAIL		(STATUS)(E_GCF_MASK + 0x1006)
#define E_GC1007_NO_AUTHENTICATION	(STATUS)(E_GCF_MASK + 0x1007)
#define E_GC1008_INVALID_USER		(STATUS)(E_GCF_MASK + 0x1008)
#define E_GC1009_INVALID_SERVER		(STATUS)(E_GCF_MASK + 0x1009)
#define E_GC100A_AUTH_EXPIRED		(STATUS)(E_GCF_MASK + 0x100A)
#define E_GC100B_NO_EXPIRATION		(STATUS)(E_GCF_MASK + 0x100B)
#define	E_GC100C_RESTRICTED_AUTH	(STATUS)(E_GCF_MASK + 0x100C)
#define	E_GC100D_SRV_AUTH_FAIL		(STATUS)(E_GCF_MASK + 0x100D)
#define E_GC1010_INSUFFICIENT_BUFFER	(STATUS)(E_GCF_MASK + 0x1010)
#define E_GC1011_INVALID_DATA_OBJ	(STATUS)(E_GCF_MASK + 0x1011)
#define E_GC1012_INVALID_PARM_VALUE	(STATUS)(E_GCF_MASK + 0x1012)
#define	E_GC1013_NO_MEMORY		(STATUS)(E_GCF_MASK + 0x1013)
#define E_GC1020_GCF_MECH_INIT		(STATUS)(E_GCF_MASK + 0x1020)
#define E_GC1021_GCF_MECH_INFO		(STATUS)(E_GCF_MASK + 0x1021)
#define E_GC1022_GCF_MECH_INSTALL	(STATUS)(E_GCF_MASK + 0x1022)
#define E_GC1023_GCF_MECH_LOAD		(STATUS)(E_GCF_MASK + 0x1023)
#define E_GC1024_GCF_MECH_BIND		(STATUS)(E_GCF_MASK + 0x1024)
#define E_GC1025_GCF_MECH_DUP_ID	(STATUS)(E_GCF_MASK + 0x1025)
#define	E_GC1026_GCF_RESTRICTED_AUTH	(STATUS)(E_GCF_MASK + 0x1026)
#define E_GC1027_GCF_MECH_DISABLED      (STATUS)(E_GCF_MASK + 0x1027)
#define E_GC1028_GCF_MECH_UNSUPPORTED   (STATUS)(E_GCF_MASK + 0x1028)
#define E_GC1029_GCS_PASSWORD_LEN	(STATUS)(E_GCF_MASK + 0x1029)

#define E_GC1100_KRB_FQDNHOST           (STATUS)(E_GCF_MASK + 0x1100)
#define E_GC1101_KRB_WINSOCK_INIT       (STATUS)(E_GCF_MASK + 0x1101)
#define E_GC1102_KRB_GSS_ACQUIRE_CRED   (STATUS)(E_GCF_MASK + 0x1102)
#define E_GC1103_KRB_GSS_INIT_SEC_CTX   (STATUS)(E_GCF_MASK + 0x1103)
#define E_GC1104_KRB_GSS_ACCEPT_SEC_CTX (STATUS)(E_GCF_MASK + 0x1104)
#define E_GC1105_KRB_GSS_IMPORT_NAME    (STATUS)(E_GCF_MASK + 0x1105)
#define E_GC1106_KRB_GSS_DISPLAY_NAME   (STATUS)(E_GCF_MASK + 0x1106)
#define E_GC1107_KRB_GSS_ENCRYPT        (STATUS)(E_GCF_MASK + 0x1107)
#define E_GC1108_KRB_GSS_DECRYPT        (STATUS)(E_GCF_MASK + 0x1108)
#define E_GC1109_KRB_GSS_INQUIRE_CRED   (STATUS)(E_GCF_MASK + 0x1109)

#define E_GC1200_KRB_GSS_API_ERR        (STATUS)(E_GCF_MASK + 0x1200)


/*
** GCS MIB Variable Names
*/

#define	GCS_MIB_VERSION			"exp.gcf.gcs.version"
#define	GCS_MIB_INSTALL_MECH		"exp.gcf.gcs.installation_mechanism"
#define	GCS_MIB_DEFAULT_MECH		"exp.gcf.gcs.default_mechanism"
#define	GCS_MIB_TRACE_LEVEL		"exp.gcf.gcs.trace_level"
#define	GCS_MIB_MECHANISM		"exp.gcf.gcs.mechanism"
#define	GCS_MIB_MECH_NAME		"exp.gcf.gcs.mechanism.name"
#define	GCS_MIB_MECH_VERSION		"exp.gcf.gcs.mechanism.version"
#define	GCS_MIB_MECH_CAPS		"exp.gcf.gcs.mechanism.capabilities"
#define GCS_MIB_MECH_STATUS		"exp.gcf.gcs.mechanism.status"
#define	GCS_MIB_MECH_ELVL		"exp.gcf.gcs.mechanism.encrypt_level"
#define GCS_MIB_MECH_OVHD		"exp.gcf.gcs.mechanism.overhead"


/*
** Main GCS Service Function.
*/

FUNC_EXTERN STATUS	IIgcs_call( i4  op, GCS_MECH mech, PTR parms );

/*
** GCS utility functions.
*/

FUNC_EXTERN GCS_MECH	gcs_mech_id( char *name );
FUNC_EXTERN GCS_MECH	gcs_default_mech( i4 op );

#endif /* _GCS_H_INCLUDED_ */

