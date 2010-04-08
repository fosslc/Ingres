/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <iiapi.h>
# include <sapiw.h>
# include <st.h>
# include <me.h>
# include <cs.h>
# include <mu.h>
# include <tm.h>
# include <ll.h>
# include <ba.h>
# include <hsh.h>
# include <ccmint.h>
# include <ccm.h>

/*
** Name:    acmsrc.c - The connection manager.
**
** Description:
**  The Connection Manager creates an drops connections to a data base for a
**  user.  By reusing dropped connections for the same data base & userid,
**  the connections are made much faster.
**
** History:
**      25-Feb-98 (shero03)
**          Created.
**	26-May-1998 (shero03)
**	    Assume the semaphores should always take place.
**	27-May-1998 (shero03)
**	    Simplify the code by replacing DB and USERID routines with a generic key
**	    routine.
**	29-Jun-1998 (shero03)
**	    Made the key routines into a generic hash facility
**	09-Jul-1998 (shero03)
**	    Moved the common code and structures to cuf!ccm
**	04-Mar-1999 (shero03)
**	    changed err to be pointer to API Error Info Block      
**	06-aug-1999 
**	    Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/* Global variables */

GLOBALREF CM_CB	*pCM_cb; /* Pointer to the Connection Manager Blk */

/*
**  Name: CM_GetAPIConnection      - Connects a client to a specified
**                                    database.
**
**  Description:
**      Connects a client to the specified database via API.
**
**  Inputs:
**      dbName          - Database name to connect to
**	flags		- Database connect options
**      user            - Registered client name
**
**  Outputs:
**     	phConn          - Pointer to connection handle
**      err		- Pointer to a error block
**
**  Returns:
**      OK if successful.
**
**  History:
**      25-Feb-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_GetAPIConnection (char *dbName, char *flags, char *user, PTR *phConn,
                  PTR err)
{
    STATUS      ret = IIAPI_ST_SUCCESS;
    char	luserid[DB_MAXNAME+1];
    char	lpassword[DB_MAXNAME+1];
    char	*pIn, *pOut;
    i4 		i;
    i4 		n;		/* Connection ID		    */
    i4 		lkey;		/* key length */
    CM_CONN_BLK wCommBlk;
    CM_CONN_BLK	*pConn;
    LNKDEF	*pentry;
    CM_KEY_BLK  *pkentry;
    II_PTR	hConn;		/* Connection handle		    */

    if (pCM_cb == NULL)
        return (IIAPI_ST_NOT_INITIALIZED);

    if (pCM_cb->cm_api_disconnect == NULL)
       pCM_cb->cm_api_disconnect = &IIcm_APIDisconnect;

    n = ++pCM_cb->cm_conn_ctr;

    /* Look for the DB name.  If it isn't found, then insert it */
    lkey = STlength(dbName);
    pentry = HSH_Find(pCM_cb->cm_db_index, dbName, lkey);
    if (pentry)
        wCommBlk.cm_conn_DB = (II_PTR)pentry;
    else
    {	
        pkentry = (CM_KEY_BLK*)HSH_Insert(pCM_cb->cm_db_index,
				dbName, lkey, sizeof(CM_KEY_BLK) + lkey); 
        pkentry->cm_key_lname = (II_INT2)lkey;
        wCommBlk.cm_conn_DB = (II_PTR)pkentry;
    }

    wCommBlk.cm_conn_flags = CM_CONN_API;

    /* Parse the optional password from the userid field */
    luserid[0] = 0x00;
    lpassword[0] = 0x00;
    pIn = user;
    if (pIn == NULL)
        wCommBlk.cm_conn_User = NULL;
    else
    {
        pOut = luserid;
        for (i = 0; i < DB_MAXNAME; i++, pIn++, pOut++)
        {
    	    if (*pIn == 0x00)
           {
               *pOut = 0x00;
               break;
           }
           if (*pIn == '/')
           {
               pIn++;
               *pOut = 0x00;
               pOut = lpassword;
               i = 0;  /* reset the index */
           }
           *pOut = *pIn;
        }

	/* Look for the Userid.  If it isn't found, then insert it */
        lkey = STlength(luserid);
        pentry = HSH_Find(pCM_cb->cm_userid_index, luserid, lkey); 
        if (pentry)
	    wCommBlk.cm_conn_User = (II_PTR)pentry;
	else    	
	{	
            pkentry = (CM_KEY_BLK*)HSH_Insert( pCM_cb->cm_userid_index,
				luserid, lkey, sizeof(CM_KEY_BLK) + lkey); 
            pkentry->cm_key_lname = (II_INT2)lkey;
            wCommBlk.cm_conn_DB = (II_PTR)pkentry;
	}
    }

    /* Look for and select an available connection */
    pentry = HSH_Select(pCM_cb->cm_free_conn_index, 
			(char*)&wCommBlk.cm_conn_DB, sizeof(II_PTR)*3);
    if (pentry == NULL)		/* have to create a connection 	*/
    {
    	/* FIXME use the flag value */
	hConn = NULL;
        ret = IIsw_connect (dbName, luserid, lpassword, &hConn, NULL, 
			    (IIAPI_GETEINFOPARM *) err);
    	if (hConn)
        {
            pConn = ccm_AllocConn(n, CM_CONN_API, wCommBlk.cm_conn_DB,
				 wCommBlk.cm_conn_User, hConn);
            HSH_Link(pCM_cb->cm_used_conn_index, 
		     (char *)&pConn->cm_conn_hndl,
		     sizeof(II_PTR),
		     &pConn->cm_conn_lnk);  /* add entry to chain in */
        }
	else
	    return ret; 
    }
    else
    {
        /* FIXME reset the connection   */
    	/* FIXME use the flag value */
        pConn = (CM_CONN_BLK*)LL_GetObject(pentry);
	HSH_Link(pCM_cb->cm_used_conn_index, 
		 (char *)&pConn->cm_conn_hndl,
		 sizeof(II_PTR),
		 &pConn->cm_conn_lnk);    /* add entry to index */        
        pCM_cb->cm_reuse_ctr++;
    }

    *phConn = pConn->cm_conn_hndl;	/* return the API connection handle  */
    if (TMsecs() > pCM_cb->cm_tm_next_purge)
    	ccm_PurgeFree();

    return ret;
}

/*
**  Name: CM_DropAPIConnection      - Drops a client connection to a specified
**                                    database.
**
**  Description:
**      Drops a client connection to a specified database via API.
**
**  Inputs:
**	phConn		- Pointer to the Connection handle.
**	bCommit		- True -> commit the connection
**			  False -> rollback the connection
**
**  Outputs:
**      err		- Pointer to a error block
**
**  Returns:
**      OK if successful.
**
**  History:
**      25-Feb-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_DropAPIConnection (PTR phConn, bool bCommit, PTR err)
{
    CM_CONN_BLK	*pConn;
    LNKDEF	*pentry;

    if (pCM_cb == NULL)
        return (IIAPI_ST_NOT_INITIALIZED);

    pCM_cb->cm_drop_ctr++;

    pentry = HSH_Select(pCM_cb->cm_used_conn_index, (char*)&phConn,
    			 sizeof(II_PTR));
    if (pentry == NULL)
    {
    	return IIAPI_ST_INVALID_HANDLE;
    }

    pConn = (CM_CONN_BLK*)LL_GetObject(pentry);

    if (bCommit == TRUE)
    {
    	/* Need the hTran for the API Commit call	*/
    }
    else
    {
    	/* Need the hTran for the API Rollback call	*/
    }

    HSH_Link(pCM_cb->cm_free_conn_index, 
	     (char *)&pConn->cm_conn_DB,
	     sizeof(II_PTR)*3,
	     &pConn->cm_conn_lnk);  /* add entry to index */

    if (TMsecs() > pCM_cb->cm_tm_next_purge)
    	ccm_PurgeFree();

    return IIAPI_ST_SUCCESS;
}

/*
**  Name: iicm_APIDisconnection - Disconnect a client's connection
**
**  Description:
**      Unconnects a client from a server via API.
**
**  Inputs:
**	phConn		- -> to the Connection handle.
**	bCommit		- True -> commit the connection
**			  False -> rollback the connection
**
**  Outputs:
**      *err		- error code 
**
**  Returns:
**      OK if successful.
**
**  History:
**      01-Jul-98 (shero03)
**          Created.
*/
II_EXTERN void II_EXPORT
IIcm_APIDisconnect (PTR phConn, bool bCommit, PTR err)
{

    if (bCommit)
    {
    	/*  Commit the transaction */
    }
    else
    {
    	/* Cancel any statement */
    	/* Rollback any transaction */
    }				       

    IIsw_disconnect ((II_PTR)phConn, (IIAPI_GETEINFOPARM *)err);

    return;
}
