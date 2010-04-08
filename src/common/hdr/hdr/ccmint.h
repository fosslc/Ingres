/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:    ccm.h	- The connection manager internal structures
**
** Description:
**  The Connection Manager creates and drops connections to a data base for a user
**  By reusing dropped connections for the same data base & userid, the connections
**  are made much faster.
**
** History:
**      09-Jul-98 (shero03)
**          Created.
**	04-Mar-1999 (shero03)
**	    Use PTRs instead of II_PTR
**      06-aug-1999
**          Replace nat and longnat with i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#if !defined CCM_H
#define CCM_H

typedef void ccm_DiscFunc (PTR phConn, bool bCommit, PTR err);

/* Structure definitions */
typedef struct _CM_CB
{
	i4 		cm_flags;
	i4 		cm_conn_ctr;
	i4 		cm_reuse_ctr;
	i4 		cm_drop_ctr;
	i4 		cm_cleanup_ctr;
	i4		cm_tm_next_purge;
	HSHDEF		*cm_db_index;
	HSHDEF		*cm_userid_index;
	HSHDEF		*cm_used_conn_index;
	HSHDEF		*cm_free_conn_index;
	ccm_DiscFunc	*cm_api_disconnect;
	ccm_DiscFunc    *cm_libq_disconnect;
} CM_CB;

# define CM_FLAG_MULTITHREADED 1

/* Note that the number of indexes should be a prime number */
# define NUM_CONN_INDEX	251

typedef struct _CM_CONN_BLK
{
	LNKDEF		cm_conn_lnk;
	i4		cm_conn_id;
	CS_SID		cm_conn_SID;
	PTR		cm_conn_DB;	 /* DB, User & flags must be conseg */
	PTR		cm_conn_User;   /* They comprise the free key      */
	i4		cm_conn_flags;	/* defined below		*/
	PTR		cm_conn_hndl;
} CM_CONN_BLK;
typedef CM_CONN_BLK* P_CM_CONN_BLK;

# define CM_CONN_OFF_DB sizeof(LNKDEF)+sizeof(i4)+sizeof(CS_SID)
# define CM_CONN_OFF_HNDL CM_CONN_OFF_DB+sizeof(PTR)+sizeof(PTR)+sizeof(i4)

/*	cm_conn_flags values 	*/
# define CM_CONN_API	1
# define CM_CONN_LIBQ	2

# define NUM_KEY_INDEX	113
typedef struct _CM_KEY_BLK
{
	LNKDEF		cm_key_lnk;
	i2		cm_key_lname;
	char		cm_key_name[1];
} CM_KEY_BLK;
# define CM_KEY_OFF sizeof(LNKDEF)+sizeof(i2)

P_CM_CONN_BLK ccm_AllocConn( i4 conn_ID, i4 conn_flag,
				PTR conn_DB, PTR conn_Userid,
				PTR conn_hndl);
void ccm_PurgeFree(void);
void ccm_PurgeUsed(void);

#endif 
