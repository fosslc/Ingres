/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <ddsmem.h>
#include <ci.h>

/* 
** Password decryption defines. Must match those in CI gcu and gcn
*/

#define DDS_CRYPT_SIZE  8
#define DDS_PASSWD_END  '0'
/**
**
**  Name: ddsdbuser.c - Data Dictionary Security Service Facility
**
**  Description:
**		This file contains all available functions to manage database user.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**  13-may-1998 (canor01)
**      Change DDS_PASSWORD_LENGTH to DDS_PASSWORD_LENGTH.
**      Replace gcn_encrypt with gcu_encode.
**  21-jul-1998 (marol01)
**      add alias property to dbuser structure (this alias has to be unique in 
**			the system rather than the name won't.
**  14-Dec-1998 (fanra01)
**      Add the function DDSGetDBUserInfo to return user information based
**      upon the the db user name.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**  17-Jan-2000 (chika01)
**      Password is stored not crypted when created new user.  Crypted password
**          should be stored instead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Oct-2002 (somsa01)
**	    Changed DDGInsert(), DDGUpdate(), DDGKeySelect() and DDGDelete()
**	    such that we pass the addresses of int and float arguments.
**	27-Jul-2004 (hanje04)
**	    Add DDSdecrypt(), DDSencode().
**	    Replace calls to gcn_decrypt() with static function DDSdecrypt()
**	    as gcn_decrypt is no longer visible outside of the gcn.
**	    Move DDSGetDBForUser() here from ddsdb.c so it can call new
**	    static DDSdecrypt().
**	    Relace calls to gcu_encode() with DDSencode();
**	    
**/

GLOBALREF SEMAPHORE dds_semaphore;
GLOBALREF DDG_SESSION dds_session;
static STATUS DDSdecrypt(char*, char*, char*);

/*
** Name: DDSCreateDBUser() - DB User creation in the Data Dictionary 
**							 and in memory
**
** Description:
**	
**
** Inputs:
**	char*		: user name
**	char*		: user password
**	char*		: user comment
**
** Outputs:
**	u_nat*		: user id
**
** Returns:
**	GSTATUS		: 
**				  E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**  17-Jan-2000 (chika01)
**      Password is stored not crypted when created new user.  Crypted password
**          should be stored instead.
*/
GSTATUS 
DDSCreateDBUser (
	char	*alias,
	char	*name, 
	char	*password, 
	char	*comment, 
	u_i4 *id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDSPerformDBUserId(id);
		if (err == GSTAT_OK) 
		{
			char crypt[DDS_PASSWORD_LENGTH];
			if (DDSencode(name, password, crypt) == OK)
			{
				err = DDGInsert(
						&dds_session, 
						DDS_OBJ_DBUSER, 
						"%d%s%s%s%s", 
						id, 
						alias,
						name, 
						crypt, 
						comment);
				if (err == GSTAT_OK)
					err = DDSMemCreateDBUser(*id, alias, name, crypt);

				if (err == GSTAT_OK)
					err = DDGCommit(&dds_session);
				else
					CLEAR_STATUS(DDGRollback(&dds_session));
			}
			else
				err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
		}
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSUpdateDBUser()	- DB User change in the Data Dictionary 
**							  and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	char*		: user name
**	char*		: user password
**	char*		: user comment
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
DDSUpdateDBUser (
	u_i4 id,
	char *alias,
	char *name, 
	char *password, 
	char *comment) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		bool same = FALSE;
		char crypt[DDS_PASSWORD_LENGTH];

		err = DDSMemVerifyDBUserPass(id, password, &same);
		if (err == GSTAT_OK)
		{
			if (same == FALSE)
			{
				if (DDSencode(name, password, crypt) != OK)
					err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
				password = crypt;
			}

			if (err == GSTAT_OK)
				err = DDGUpdate(
					&dds_session, 
					DDS_OBJ_DBUSER, 
					"%s%s%s%s%d", 
					alias,
					name, 
					password, 
					comment, 
					&id);

			if (err == GSTAT_OK)
				err = DDSMemUpdateDBUser(id, alias, name, password);

			if (err == GSTAT_OK)
				err = DDGCommit(&dds_session);
			else
				CLEAR_STATUS(DDGRollback(&dds_session));
		}
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSDeleteDBUser()	- DB User delete in the Data Dictionary 
**							  and in memory
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
DDSDeleteDBUser (
	u_i4 id) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGDelete(&dds_session, DDS_OBJ_DBUSER, "%d", &id);

		if (err == GSTAT_OK) 
			err = DDSMemDeleteDBUser(id);

		if (err == GSTAT_OK)
			err = DDGCommit(&dds_session);
		else
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSOpenDBUser()	- Open a cursor on the dbuser list
**
** Description:
**	
**
** Inputs:
**	PTR		*cursor
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
DDSOpenDBUser (PTR		*cursor) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
		err = DDGSelectAll(&dds_session, DDS_OBJ_DBUSER);
return(err);
}

/*
** Name: DDSRetrieveDBUser()	- Select a unique dbuser
**
** Description:
**	
**
** Inputs:
**	u_nat : dbuser id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
DDSRetrieveDBUser (
	u_i4	id,
	char	**alias,
	char	**name, 
	char	**password, 
	char	**comment) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, TRUE);
	if (err == GSTAT_OK) 
	{
		err = DDGKeySelect(
				&dds_session, 
				DDS_OBJ_DBUSER, 
				"%d%s%s%s%s", 
				&id,
				alias,
				name, 
				password, 
				comment);

		if (err == GSTAT_OK)
			err = DDGCommit(&dds_session);
		else
			CLEAR_STATUS(DDGRollback(&dds_session));

		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSGetDBUser()	- get the current dbuser
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GSTATUS 
DDSGetDBUser (
	PTR		*cursor,
	u_i4	**id, 
	char  **alias,
	char	**name, 
	char	**password, 
	char	**comment) 
{
	GSTATUS err = GSTAT_OK;
	bool	exist;

	err = DDGNext(&dds_session, &exist);
	if (err == GSTAT_OK && exist == TRUE)
		err = DDGProperties(
			   	&dds_session, 
					"%d%s%s%s%s", 
					id,
					alias, 
					name, 
					password, 
					comment);
	if (err != GSTAT_OK)
		exist = FALSE;
	*cursor = (char*) (SCALARP)exist;
return(err);
}

/*
** Name: DDSCloseDBUser()	- close the cursor on the dbuser list
**
** Description:
**
** Inputs:
**	PTR		*cursor
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
DDSCloseDBUser (PTR		*cursor) 
{
	GSTATUS err = GSTAT_OK;

	err = DDGClose(&dds_session);

	if (err == GSTAT_OK)
		err = DDGCommit(&dds_session);
	else
		CLEAR_STATUS(DDGRollback(&dds_session));
	CLEAR_STATUS(DDFSemClose(&dds_semaphore));
return(err);
}

/*
** Name: DDSGetDBUserName()	- return the dbuser name
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
DDSGetDBUserName(
	u_i4 id, 
	char** name)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetDBUserName(id, name);
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSGetDBUserPassword()	- return the dbuser password
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
DDSGetDBUserPassword(
	char*	alias,
	char* *dbuser,
	char*	*password)
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetDBUserPassword(alias, dbuser, password);
		if (err == GSTAT_OK)
		{
			char* tmp = NULL;
			err = G_ME_REQ_MEM(0, tmp, char, DDS_PASSWORD_LENGTH);
			if (err == GSTAT_OK)
				if (DDSdecrypt(*dbuser, *password, tmp) != OK)
				{
					MEfree((PTR)tmp);
					err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
				}
				else
					*password = tmp;
		}
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSGetDBUserInfo()
**
** Description:
**      Return the information held for the specified DBuser.
**
** Inputs:
**      alias       alias name.  If this is NULL dbuser is assumed.
**      dbuser      database user name.
**
** Outputs:
**      alias
**      dbuser
**      password
**
** Returns:
**    GSTATUS        :
**                  GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      14-Dec-1998 (fanra01)
**          Created.
*/
GSTATUS
DDSGetDBUserInfo(
    char* *dbuser,
    char* *alias,
    char* *password)
{
    GSTATUS err = GSTAT_OK;

    if ((*alias == NULL) && (*dbuser == NULL))
    {
        err = DDFStatusAlloc(E_DF0051_DDS_UNKNOWN_DBUSER);
    }
    else
    {
        err = DDFSemOpen(&dds_semaphore, FALSE);
        if (err == GSTAT_OK)
        {
            if (*alias != NULL)
            {
                err = DDSMemGetDBUserPassword(*alias, dbuser, password);
            }
            else
            {
                err = DDSMemGetDBUserInfo(*dbuser, alias, password);
            }
            if (err == GSTAT_OK)
            {
                char* tmp = NULL;
                err = G_ME_REQ_MEM(0, tmp, char, DDS_PASSWORD_LENGTH);
                if (err == GSTAT_OK)
                    if (DDSdecrypt(*dbuser, *password, tmp) != OK)
                    {
                        MEfree((PTR)tmp);
                        err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
                    }
                    else
                        *password = tmp;
            }
            CLEAR_STATUS(DDFSemClose(&dds_semaphore));
        }
    }
    return(err);
}

/*
** Name: DDSGetDBForUser()	- Retrieve the associated user for a specific Database
**
** Description:
**	
**
** Inputs:
**	u_nat		: user id
**	char*		: dbname name
**
** Outputs:
**	char*		: user name can be NULL if no relation was defined
**	char*		: user password can be NULL (is allocated, so it has to be deleted)
**
** Returns:
**	GSTATUS		: 
**				  E_OI0030_WSM_UNKNOWN_USER
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
DDSGetDBForUser (
	u_i4	user_id, 
	char	*name, 
	char	**dbname,
	char	**user, 
	char	**password,
	i4 *flag) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemOpen(&dds_semaphore, FALSE);
	if (err == GSTAT_OK) 
	{
		err = DDSMemGetDBForUser(user_id, name, dbname, user, password, flag);

		if (err == GSTAT_OK)
		{
			char* tmp = NULL;
			err = G_ME_REQ_MEM(0, tmp, char, DDS_PASSWORD_LENGTH);
			if (err == GSTAT_OK)
				if (DDSdecrypt(*user, *password, tmp) != OK)
				{
					MEfree((PTR)tmp);
					err = DDFStatusAlloc(E_DF0054_DDS_CRYPT);
				}
				else
					*password = tmp;
		}
		CLEAR_STATUS(DDFSemClose(&dds_semaphore));
	}
return(err);
}

/*
** Name: DDSencode
**
** Description:
**	This routine encrypts a string.  Uses the inverse algorithm
**	of DDSDecrypt() so that passwords encrypted using this
**	routine are acceptable for ICE Server usage.
**
** Inputs:
**	key		Encryption key.  Max length DDS_USER_LENGTH.
**	str		String to encrypt.  Max length (DDS_PASSWORD_LENGTH/2) - 1.
**
** Outputs:
**	buff		Encrypted string.  Max length DDS_PASSWORD_LENGTH
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	28-Jul-2004 (hanje04)
**	    Created from gcu_encode.
*/

STATUS
DDSencode( char *key, char *str, char *buff )
{
    CI_KS	ksch;
    i4		len;
    char	kbuff[ DDS_USER_LENGTH ];
    char	ibuff[ DDS_PASSWORD_LENGTH ];
    char	tmp[ DDS_PASSWORD_LENGTH ];

    if ( ! key  ||  ! buff )  return( FAIL );

    if ( ! str  ||  ! *str )
    {
    	*buff = EOS;
	return( OK );
    }

    /* 
    ** Coerce key to length DDS_CRYPT_SIZE.
    */
    if ( (len = (i4)STlength( key )) > (DDS_USER_LENGTH - 1) )
	return(FAIL);

    STcopy( key, kbuff );
    for ( ; len < DDS_CRYPT_SIZE; len++ )  kbuff[ len ] = ' ';
    kbuff[ DDS_CRYPT_SIZE ] = EOS;

    /*
    ** Coerce encryption string to multiple of DDS_CRYPT_SIZE.
    */
    if ( (len = (i4)STlength( str )) > (DDS_PASSWORD_LENGTH - 1) )
	return( FAIL );
    
    if ( len > (DDS_PASSWORD_LENGTH/2 - 1) )
	return( FAIL );

    /* Add trailing marker to handle strings with trailing SPACE */
    STcopy( str, ibuff );
    ibuff[ len++ ] = DDS_PASSWD_END;
	
    /* Pad string with spaces to modulo DDS_CRYPT_SIZE length. */
    while( len % DDS_CRYPT_SIZE )  ibuff[ len++ ] = ' ';
    ibuff[ len ] = EOS;

    /*
    ** Now encrypt the string.
    */
    CIsetkey( kbuff, ksch );
    CIencode( ibuff, len, ksch, tmp );
    CItotext( (u_char *)tmp, len, buff );

    return( OK );
}

/*
** Name: DDSdecrypt
**
** Description:
**	Decrypt password encrypted by DDSencode().
**
** Input:
**	key	Encryption key.
**	pbuff	Encrypted password.
**
** Output:
**	passwd	Decrypted password.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	27-Jul-04 (hanje04)
**	    Copied here from gcn_decrypt() in gcnencry.c
*/

static STATUS
DDSdecrypt( char *key, char *pbuff, char *passwd )
{
    i4		klen, plen;
    CI_KS	ksch;
    char	keystr[ DDS_USER_LENGTH + 1 ];
    char	buff[ DDS_PASSWORD_LENGTH + 1 ];

    if ( key == NULL  ||  passwd == NULL )  return( FAIL );

    if ( pbuff == NULL  || *pbuff == EOS )  
    {
	*passwd = EOS;
	return( OK );
    }

    klen = STlength( key );
    plen = STlength( pbuff );

    if ( klen > DDS_USER_LENGTH )  return( FAIL );
    if ( plen > DDS_PASSWORD_LENGTH )  return( FAIL );

    /*
    ** Adjust key to DDS_CRYPT_SIZE by padding/truncation.
    */
    STcopy( key, keystr );
    for( ; klen < DDS_CRYPT_SIZE; klen++ )  keystr[ klen ] = ' ';
    keystr[ DDS_CRYPT_SIZE ] = EOS;
    CIsetkey( keystr, ksch );

    /*
    ** Password is passed as text, convert back to encoded 
    ** binary.  Encoded password should be of correct size.
    ** Decrypt password.
    */
    CItobin( pbuff, &plen, (u_char *)buff );
    if ( plen % DDS_CRYPT_SIZE )  return( FAIL );
    CIdecode( buff, plen, ksch, passwd );

    /*
    ** Cleanup password by removing trailing
    ** spaces and the end marker.
    */
    passwd[ plen ] = EOS;
    plen = STtrmwhite( passwd );
    if ( passwd[ --plen ] != DDS_PASSWD_END )  return( FAIL );
    passwd[ plen ] = EOS;

    return( OK );
}

