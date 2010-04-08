/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: ULH.H - Control Blocks and Constants to Interface with ULH.
**
** Description:
**      This header file contains definitions of all data structures
**	and constants needed by a client program to interface to ULH.
**	This header file must be included in a client source program.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	18-sep-87 (puree)
**	    add user semaphores to hash objects.  ULH will no longer
**	    lock objects for users.
**	27-feb-88 (puree)
**	    remove unused functions.
**	05-apr-90 (andre)
**	    defined masks to be used when calling ulh_init().
**      14-aug-91 (jrb)
**          Changed ulh_usem to be CS_SEMAPHORE instead of SCF_SEMAPHORE in
**          ULH_OBJECT.
**	13-mar-92 (teresa)
**	    added external references for ulh_cre_class and ulh_dest_class.
**	31-aug-1992 (rog)
**	    Prototype ULF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**/

/*
**  Defines of constants.
*/
#define		ULH_MAXNAME	(sizeof(PTR) + DB_MAXNAME + DB_MAXNAME)
/*
**	Types of ULH Objects
*/
#define		ULH_FIXED	1
#define		ULH_DESTROYABLE	0
/*
**	ULH Information Code
*/
#define		ULH_TABLE	1	/* table attributes */
#define		ULH_ROSTER	2	/* class roster */
#define		ULH_MEMCOUNT	3	/* count of member in a class */
#define		ULH_STAT	4	/* table statistics */

/*}
** Name: ULH_OBJECT - ULH Object Header Block
**
** Description:
**      When access to an object is granted to a user, ULH returns the
**	pointer to the user-visible portion of the object.  The structure
**	of the user-visible object, ULH_OBJECT, is described below.
**
**	Since ULH_OBJECT is actually a portion of the ULH object, a user
**	must not alter any field in the structure other than the user-
**	defined pointer field.
**
** History:
**      08-jan-87 (puree)
**          initial version.
**	18-sep-87 (puree)
**	    add ulh_usem to be used by users for synchronizing accesses
**	    to hash objects since ULH no longer locks objects for users.
**      14-aug-91 (jrb)
**          Changed ulh_usem to be CS_SEMAPHORE instead of SCF_SEMAPHORE.
*/
typedef struct _ULH_OBJECT
{
    PTR		    ulh_id;		/* internal object id */
    char	    ulh_objname[ULH_MAXNAME]; /* object name */
    i4		    ulh_namelen;	/* length of name */
    CS_SEMAPHORE   ulh_usem;		/* user semaphore */
    PTR		    ulh_streamid;	/* associated memory stream id */
    PTR		    ulh_uptr;		/* user-defined pointer */
}   ULH_OBJECT;

/*}
** Name: ULH_LIST - List of Objects
**
** Description:
**      When multiple objects are given to ULH as in the ulh_mrelease
**      function, or when thay are returned by ULH as in the ulh_getclass
**      function, the object pointers are passed to/from ULH in a list
**      as descrribed by the ULH_LIST below.  The ULH_LIST must be in
**      user's data space.
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_LIST
{
    i4		ulh_mcount;		/* requested # of objects */
    i4		ulh_rcount;		/* actual # of objects returned */
    ULH_OBJECT	*ulh_mptr[1];		/* array of pointers to objects */ 
}   ULH_LIST;

/*}
** Name: ULH_RCB - ULH Request Control Block
**
** Description:
**      This structure is the primary interface to ULH.  It identifies the
**      hash table to work with.  It also contains output and error status
**      from the requested function.  The ULH_RCB must be allocated in
**      user's data space.  The structure of ULH_RCB is shown below:
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_RCB
{
    PTR		    ulh_hashid;		/* internal hash table id. */
    DB_ERROR	    ulh_error;		/* standard error code structure */
    ULH_OBJECT	    *ulh_object;	/* pointer to ULH OBJECT */
}   ULH_RCB;

/*}
** Name: ULH_ATTR - Hash Table Attributes
**
** Description:
**      The ULH_ATTR is used in the ulh_info function to obtain information
**      about a hash table.  The strucuture must be in user's data space
**      and will be filled with information by ULH.  It has the format
**      below:
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_ATTR
{
    PTR		    ulh_poolid;		/* memory pool id. */
    PTR		    ulh_streamid;	/* memory streamid */
    i4		    ulh_facility;	/* owner facility id. */
    i4		    ulh_bcount;		/* # of buckets in the object table */
    i4		    ulh_maxobj;		/* max. # of objects in the table */
    i4		    ulh_fcount;		/* # of fixed objects */
    i4		    ulh_dcount;		/* # of destroyable objects */
    i4		    ulh_cbuckets;	/* # of buckets in the class table */
    i4		    ulh_maxclass;	/* max. # of classes in the table */
    i4		    ulh_ccount;		/* # of classes in the table */
    i4		    ulh_l1count;	/* # of objects in LRU 1 queue */
    i4		    ulh_l2count;	/* # of objects in LRU 2 queue */
}   ULH_ATTR;

/*}
** Name: ULH_CSTAT - Information Record of a Class
**
** Description:
**      This structure is used in the ulh_info function to obtain information
**      about a class.  It must be in user's data space and has the format
**      below:
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_CSTAT
{
    i4		    ulh_count;		/* count of member in the class */
    i4		    ulh_namelen;	/* length of name */
    unsigned char   ulh_cname[ULH_MAXNAME]; /* name of the class */
}   ULH_CSTAT;


/*}
** Name: ULH_CLIST - List of Class Information Record
**
** Description:
**      The ULH_CLIST structure is used in the ulh_info function to obtain
**      information about multiple classes in a hash table.  It must be in
**      user's data space and has the format below:
**
** History:
**      08-jan-87 (puree)
**          initial version.
*/
typedef struct _ULH_CLIST
{
    i4		    ulh_ccount;		/* requested # of classes */
    i4		    ulh_rcount;		/* returned # of classes */
    ULH_CSTAT	    ulh_cstat[1];	/* array of classes */
}   ULH_CLIST;

/*
** values of masks to be used when calling ulh_init().
**
** History:
**	05-apr-90 (andre)
**	    defined ULH_ALIAS_OK
**
*/
#define	    ULH_ALIAS_OK    ((i4) 0x01)    /* aliases will be allowed */

/*
** size of alias structure; this has to be kept in sync with ULH_HALIAS
**
**  History:
**	05-apr-90 (andre)
**	    defined.
**	06-apr-90 (teg)
**	    DB_MAX_NAME -> DB_MAXNAME
*/
#define	    ULH_H_ALIAS_SIZE	(9*sizeof(PTR)+sizeof(i4)+2*DB_MAXNAME)


/*
**  External Function references
*/

FUNC_EXTERN DB_STATUS ulh_init( ULH_RCB *ulh_rcb, i4  facility, PTR pool_id,
				SIZE_TYPE *memleft, i4  max_obj, i4  max_class,
				i4 density, i4  info_mask, i4 max_alias );
FUNC_EXTERN DB_STATUS ulh_close( ULH_RCB *ulh_rcb );
FUNC_EXTERN DB_STATUS ulh_create( ULH_RCB *ulh_rcb, unsigned char *obj_name,
				  i4  name_len, i4  obj_type,
				  i4 block_size );
FUNC_EXTERN DB_STATUS ulh_destroy( ULH_RCB *ulh_rcb, unsigned char *obj_name,
				   i4  name_len );
FUNC_EXTERN DB_STATUS ulh_clean( ULH_RCB *ulh_rcb );
FUNC_EXTERN DB_STATUS ulh_rename( ULH_RCB *ulh_rcb, unsigned char *obj_name,
				  i4  name_len );
FUNC_EXTERN DB_STATUS ulh_access( ULH_RCB *ulh_rcb, unsigned char *obj_name,
				  i4  name_len );
FUNC_EXTERN DB_STATUS ulh_release( ULH_RCB *ulh_rcb );
FUNC_EXTERN DB_STATUS ulh_define_alias( ULH_RCB *ulh_rcb, unsigned char *alias,
					i4 alias_len );
FUNC_EXTERN DB_STATUS ulh_getalias( ULH_RCB *ulh_rcb, unsigned char *alias,
				    i4  alias_len );
FUNC_EXTERN DB_STATUS ulh_getmember( ULH_RCB *ulh_rcb,
				     unsigned char *class_name, i4  class_len,
				     unsigned char *obj_name, i4  name_len,
				     i4  obj_type, i4 block_size );
FUNC_EXTERN DB_STATUS ulh_cre_class( ULH_RCB *ulh_rcb,
				     unsigned char *class_name, i4  class_len,
				     unsigned char *obj_name, i4  name_len,
				     i4  obj_type, i4 block_size,
				     i4  uniq_id );
FUNC_EXTERN DB_STATUS ulh_dest_class( ULH_RCB *ulh_rcb,
				      unsigned char *class_name,
				      i4  class_len );
FUNC_EXTERN DB_STATUS ulh_info( ULH_RCB *ulh_rcb, i4  ulh_icode, PTR bufptr );
