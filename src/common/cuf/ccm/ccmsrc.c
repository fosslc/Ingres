/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
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
** Name:    ccmsrc.c - The common connection manager.
**
** Description:
**  The Connection Manager creates and drops connections to a data base for a
**  user.  By reusing dropped connections for the same data base & userid,
**  the connections are made much faster.
**
** History:
**      09-Jul-98 (shero03)
**          Created.
**      18-Jan-1999 (fanra01)
**          Removed unused header.
**      06-aug-1999
**          Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* Global variables */

GLOBALDEF CM_CB	CM_cb; /* Structure which anchors all CM structures */
GLOBALDEF CM_CB	*pCM_cb = NULL; /* Pointer to the Connection Manager Blk */

/*
**  Name: CM_Initialize      - Starts the Connection Manager
**
**  Description:
**      Initialize the Connection Manager so it can accept Connect & Drop
**	During initialization the Connection Manager calls the HSH facility
**	HSH in turn allocates memory which will be freed when
**	CM_Termainte is called.  Otherwise a memory leak  will occur.
**
**  Inputs:
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
**	01-Apr-1999 (shero03)
**	    Fixed compiler errors after api change
*/
II_EXTERN STATUS II_EXPORT
IIcm_Initialize (PTR err)
{
    i4 		flag;
    CS_SCB	*scb;

    if (pCM_cb)
        return(E_AP0006_INVALID_SEQUENCE);

     pCM_cb = &CM_cb;

    CM_cb.cm_conn_ctr = 0;
    CM_cb.cm_reuse_ctr = 0;
    CM_cb.cm_drop_ctr = 0;
    CM_cb.cm_cleanup_ctr = 0;
    CM_cb.cm_flags = 0;
    CM_cb.cm_api_disconnect = NULL;
    CM_cb.cm_libq_disconnect = NULL;

    CSget_scb(&scb);
    if (scb && CS_is_mt())
       CM_cb.cm_flags |= CM_FLAG_MULTITHREADED;
    /* FIXME - make the time interval user modifiable */
    CM_cb.cm_tm_next_purge = TMsecs() + 5 * 60;	/* 5 min	*/

    /* Initialize the Hash Indexes */
    flag = HSH_SHARED | HSH_VARIABLE | HSH_STRKEY;
    CM_cb.cm_db_index = HSH_New(NUM_KEY_INDEX, CM_KEY_OFF,
    				sizeof(CM_KEY_BLK) + 20,
    			 	1, MAXI4, "CM_DB_SEM", flag);
    CM_cb.cm_userid_index = HSH_New(NUM_KEY_INDEX, CM_KEY_OFF,
    				sizeof(CM_KEY_BLK) + 8,
    			 	1, MAXI4, "CM_USERID_SEM", flag);
    flag = HSH_SHARED;
    CM_cb.cm_used_conn_index = HSH_New(NUM_CONN_INDEX, CM_CONN_OFF_HNDL,
    				sizeof(CM_CONN_BLK),
    			 	1, MAXI4, "CM_CONN_USED_SEM", flag);
    CM_cb.cm_free_conn_index = HSH_New(NUM_CONN_INDEX, CM_CONN_OFF_DB,
    				sizeof(CM_CONN_BLK),
    			 	0, 0, "CM_CONN_FREE_SEM", flag);

    return IIAPI_ST_SUCCESS;
}		/* proc  CM_Initialize */

/*
**  Name: CM_Terminate      - Stops the Connection Manager
**
**  Description:
**      Terminate the Connection Manager so it can free all allocated
**      storage.  Note that Connect and Drop requests are not valid after
**	this call.
**
**  Inputs:
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
IIcm_Terminate (PTR err)
{

    if (!pCM_cb)
    	return(IIAPI_ST_SUCCESS);

    ccm_PurgeUsed();	/* Drop all used connections w/rollback */
    ccm_PurgeFree();	/* Drop all timed-out free connections */
    ccm_PurgeFree();	/* Drop rest of free connections */

    /* Free all the Hash Indexes and their entries */
    HSH_Dispose(pCM_cb->cm_db_index);
    HSH_Dispose(pCM_cb->cm_userid_index);
    HSH_Dispose(pCM_cb->cm_used_conn_index);
    HSH_Dispose(pCM_cb->cm_free_conn_index);
    
    pCM_cb = NULL;

    return IIAPI_ST_SUCCESS;
}    	/* proc  CM_Terminate */

/*
**  Name: CM_Cleanup      - Remove any connections for this process
**
**  Description:
**      Drop any connections that this process may have established.
**
**  Inputs:
**	SID	Session ID
**
**  Outputs:
**      *err		- error code
**
**  Returns:
**      OK if successful.
**
**  History:
**      03-Mar-98 (shero03)
**          Created.
*/
II_EXTERN STATUS II_EXPORT
IIcm_Cleanup (CS_SID sid, PTR err)
{
    LNKDEF	**list;
    LNKDEF	*entry = NULL;
    LNKDEF      *nentry;
    CM_CONN_BLK *pConn;
    IIAPI_GETEINFOPARM *lerr;

    if (pCM_cb == NULL)
        return (IIAPI_ST_NOT_INITIALIZED);

    if (HSH_AreEntries(pCM_cb->cm_used_conn_index))
    {
        HSH_GetLock(pCM_cb->cm_used_conn_index);/* get an excl lock  */

    	while (TRUE)
        {
            HSH_Next(pCM_cb->cm_used_conn_index, &list, &entry, &nentry);
            if (entry == NULL)      /* At end ? */
                break;              /* Y. leave */
            pConn = LL_GetObject(entry);

            if (pConn->cm_conn_SID == sid)
	    {	
    	    	HSH_Unlink(pCM_cb->cm_used_conn_index, 
	    		  (char *)&pConn->cm_conn_hndl,
	     		  sizeof(PTR),
	     		  &pConn->cm_conn_lnk);     /* remove it from used */
    	        if (pConn->cm_conn_flags == CM_CONN_API)
                    pCM_cb->cm_api_disconnect(pConn->cm_conn_hndl, FALSE, 
		    				(PTR)lerr);
    		else
                    pCM_cb->cm_libq_disconnect(pConn->cm_conn_hndl, FALSE,
		    				 (PTR)lerr);
		/* Free the Entry */
    		HSH_DeallocEntry(pCM_cb->cm_used_conn_index, &pConn->cm_conn_lnk);
	    }
        }
        HSH_ReleaseLock(pCM_cb->cm_used_conn_index);/* release the lock  */
    }

    return IIAPI_ST_SUCCESS;

}		/* proc - CM_cleanup	*/

/*
**  Name: AllocConn      - Allocate a new connection block
**
**  Description:
**      Allocate a piece of ME storage to hold the new CM_CONN_BLK
**
**  Inputs:
**      conn_db		- DataBase handle
**	conn_userid	- Userid handle
**	conn_hndl	_ Connection handle
**
**  Outputs:
**
**  Returns:
**      The addr(Connection entry)
**
**  History:
**      27-Feb-98 (shero03)
**          Created.
*/
P_CM_CONN_BLK ccm_AllocConn(i4  conn_id, i4  conn_flag,
		   	PTR conn_DB, PTR conn_Userid, PTR conn_hndl)
{
    CM_CONN_BLK	*entry;

    entry = (CM_CONN_BLK *)HSH_AllocEntry(pCM_cb->cm_used_conn_index,
    					 NULL, 0, sizeof(CM_CONN_BLK));
    if (entry)
    {
        entry->cm_conn_id = conn_id;         /* initialize the ID field     */
        if (pCM_cb->cm_flags & CM_FLAG_MULTITHREADED)
            CSget_sid(&entry->cm_conn_SID);      /* Fill in the SID         */
        else
            entry->cm_conn_SID = 1;
        entry->cm_conn_flags = conn_flag;
        entry->cm_conn_DB = conn_DB;         /* Fill in the DB handle       */
        entry->cm_conn_User = conn_Userid;   /* Fill in the Userid Handle   */
        entry->cm_conn_hndl = conn_hndl;     /* Fill in the Conn Handle     */
    }

    return entry;			/* return the addr(new entry)	     */
}		/* proc - AllocConn */

/*
**  Name: PurgeFree      - Purge old free connections
**
**  Description:
**      Scan the free index.
**	Mark all entries.
**	if an entry is marked twice drop the connection and dealloc the entry.
**
**  Inputs:
**
**  Outputs:
**
**  Returns:
**
**  History:
**      02-Mar-98 (shero03)
**          Created.
*/
void ccm_PurgeFree(void)
{
    LNKDEF	**list;
    LNKDEF	*entry = NULL;
    LNKDEF      *nentry;
    IIAPI_GETEINFOPARM lerr;
    CM_CONN_BLK *pConn;

    if (HSH_AreEntries(pCM_cb->cm_free_conn_index))
    {
        HSH_GetLock(pCM_cb->cm_free_conn_index); /* Lock the Index */

    	while (TRUE)
        {
            HSH_Next(pCM_cb->cm_free_conn_index, &list, &entry, &nentry);
            if (entry == NULL)      /* At end ? */
                break;             /* Y. leave */
            pConn = LL_GetObject(entry);
            if (pConn->cm_conn_hndl == NULL)
                pConn->cm_conn_hndl = (PTR)1;
            else
            {
        	HSH_Unlink(pCM_cb->cm_free_conn_index, 
			(char *)&pConn->cm_conn_DB,
			sizeof(PTR)*3,
			&pConn->cm_conn_lnk);     /* remove it from free */

                if (pConn->cm_conn_flags == CM_CONN_API)
                    pCM_cb->cm_api_disconnect((PTR)&pConn->cm_conn_hndl, TRUE, 
		    				(PTR)&lerr);
                else
                    pCM_cb->cm_libq_disconnect((PTR)&pConn->cm_conn_hndl, TRUE, 
		    				(PTR)&lerr);

			/* Note that the entry is being freed with the used list */
			/* Only the used list does the allocates and deallocates */
			/* The entry in fact, isn't in either index! */
    		HSH_DeallocEntry(pCM_cb->cm_used_conn_index, &pConn->cm_conn_lnk);
            }
        }

        HSH_ReleaseLock(pCM_cb->cm_free_conn_index); /* release the lock */
    }

    /* FIXME - make the time interval user modifiable */
    pCM_cb->cm_tm_next_purge = TMsecs() + 5 * 60;	/* 5 min	*/

    return;
}		/* proc - PurgeFree */

/*
**  Name: PurgeUsed      - Purge all used connections
**
**  Description:
**      Scan the used index.
**	Drop the connection and dealloc the entry.
**
**  Inputs:
**
**  Outputs:
**
**  Returns:
**
**  History:
**      03-Mar-98 (shero03)
**          Created.
*/
void ccm_PurgeUsed(void)
{
    LNKDEF	**list;
    LNKDEF	*entry = NULL;
    LNKDEF      *nentry;
    IIAPI_GETEINFOPARM lerr;
    CM_CONN_BLK *pConn;

    if (HSH_AreEntries(pCM_cb->cm_used_conn_index))
    {
        HSH_GetLock(pCM_cb->cm_used_conn_index); /* Lock the Index */

    	while (TRUE)
        {
            HSH_Next(pCM_cb->cm_used_conn_index, &list, &entry, &nentry);
            if (entry == NULL)      /* At end ? */
                break;                  /* Y. leave */
            pConn = LL_GetObject(entry);

    		HSH_Unlink(pCM_cb->cm_used_conn_index, 
			  (char *)&pConn->cm_conn_hndl,
			  sizeof(PTR),
			  &pConn->cm_conn_lnk);     /* remove it from used */
    		if (pConn->cm_conn_flags == CM_CONN_API)
                    pCM_cb->cm_api_disconnect((PTR)&pConn->cm_conn_hndl, FALSE,
		    				 (PTR)&lerr);
    		else
                    pCM_cb->cm_libq_disconnect((PTR)&pConn->cm_conn_hndl, FALSE,
		    				 (PTR)&lerr);
    		HSH_DeallocEntry(pCM_cb->cm_used_conn_index, &pConn->cm_conn_lnk);
        }

        HSH_ReleaseLock(pCM_cb->cm_used_conn_index); /* release the lock */
    }

    return;
}		/* proc - PurgeUsed */
