/*
** Copyright (c) 2003, 2003 Computer Associates Intl. Inc. All Rights Reserved.
**
** This is an unpublished work containing confidential and proprietary
** information of Computer Associates.  Use, disclosure, reproduction,
** or transfer of this work without the express written consent of
** Computer Associates is prohibited.
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <sp.h>
#include <mo.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcadm.h>
#include <gcadmint.h>

/*
** Name: gcadmutl.c   
**
** Description:
**	GCADM utility routines. 
**
**	gcadm_keyword	convert tokens to token_ids.	
**	gcadm_command 	convert token_ids to command. 
**
** History:
**	 03-10-2004 (wansh01) 
**	    Created.
*/



/*
** Name: gcadm_keyword
**
** Description:
**	Convert the input array of tokens to corresponding token ids
**	using the pre-defined keyword tables. 
**
**
** Input:
**	token_count	Numbers of tokens.
**	pToken		ptr to array of ptrs to char tokens. 
**	keyword_count	Numbers of keywords.
**	pKeywords	pointer to array of defined keywords. 
**	default_id	default token id 
**
** Output:
**	token_ids       array of token_ids.
**
** Returns:
**	void
**
** History:
**	 18-Mar-04 (wansh01)
**	    Created.
*/
	
void  gcadm_keyword ( u_i2 token_count, char ** pToken, 
	u_i2 keyword_count, ADM_KEYWORD * pKeywords, ADM_ID *token_ids, 
	ADM_ID default_id )
{
    bool 	keyword_found; 
    i4		i,j;
    ADM_KEYWORD *keyEntry;  


    for ( i =0; i < token_count; i++, token_ids++, pToken++)
    {
	keyword_found = FALSE; 
	keyEntry = pKeywords;  

	for ( j =0; j< keyword_count; j++, keyEntry++ )
	{
	    if ( STcasecmp ( *pToken, keyEntry->token ) == 0)
	    {
		token_ids[ 0 ] = keyEntry->id;
		keyword_found = TRUE; 
		break;
	    }
	}
	if ( ! keyword_found ) 
	{
	    if ( GCADM_global.gcadm_trace_level >= 5 )
		TRdisplay( "       GCADMUTL keyword not found\n");
	    token_ids[ 0] = default_id; 
	}  
    }
    return;
}

/*
** Name: gcadm_command
**
** Description:
**	Convert the input array of tokens_ids to corresponding command
**	using the pre-defined commands table. 
**
**
** Input:
**	token_count	Numbers of tokens_ids.
**	token_ids	ptr to array of token_ids. 
**	command_count	Numbers of commands.
**	commands	pointer to array of defined commands. 
**
** Output:
**	rqst_cmd 	ptr to requested command struct.
**
** Returns:
**	bool  		request command found indicator 
**
** History:
**	 18-Mar-04 (wansh01)
**	    Created.
*/
	
bool gcadm_command ( u_i2 token_count, ADM_ID *token_ids, 
	u_i2 command_count, ADM_COMMAND * commands,  ADM_COMMAND ** rqst_cmd)
{
    bool 	command_found; 
    i4		i,j;
    u_i2        tokenid_found; 
    ADM_ID  	*pCmdKeyword,  *pTokenId;  


    command_found = FALSE; 

    for ( i =0; i < command_count; i++, commands++)
    {
	tokenid_found =0; 

	if ( token_count == commands->count ) 
	{
	    pCmdKeyword = commands->keywords; 
	    pTokenId = token_ids; 

	    for ( j =0; j< token_count; j++, pTokenId++, pCmdKeyword++ )
	    {
		if ( *pTokenId == *pCmdKeyword )
		  tokenid_found ++;  
		else 
		    break;
	    }

	    if ( token_count == tokenid_found )
	    {
		command_found = TRUE; 
		*rqst_cmd = commands; 		    
		break;
	    }

	}
    }

	if ( ( ! command_found ) 
	 &&  ( GCADM_global.gcadm_trace_level >= 4 ) )
	    TRdisplay( "       GCADMUTL command not found\n");

   return ( command_found );
}
