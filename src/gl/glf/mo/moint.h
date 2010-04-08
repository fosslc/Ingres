/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

/**
** Name:	moint.h		- internal header for MO implementation.
**
** Description:
**	This file contains internal definitions, types, and function externs
**	used within the MO implementation.
**
**  History:
**	24-sep-92 (daveb)
**	    documented
**	4-Dec-1992 (daveb)
**	    Change exp.gl.glf to exp.glf for brevity
**	29-Apr-1993 (daveb)
**	    Increase MO_DEF_STR_LIMIT.  Ran out of space in a long
**	    running server, causing session registrations to fail.
**	    FIXME This is a potentially large problem.
**	5-May-1993 (daveb)
**	    Add "interp" field to the MO_CLASS.
**	10-Nov-1995 (reijo01)
**	    Increase MO_DEF_STR_LIMIT.  Ran out of space during a second
**      runall, causing the server to crash.
**	09-Feb-1996 (toumi01)
**	    Increase MO_DEF_MEM_LIMIT for axp_osf, since 64-bit pointers
**	    gobble up the allocation faster.
**      22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - cast #define symbols to i4
**	08-nov-1996 (canor01)
**	   Increase MO_DEF_MEM_LIMIT and MO_DEF_STR_LIMIT.  With higher
**	   granularity semaphores in logging, locking and buffer manager,
**	   we need to manage a lot more objects.
**	21-jan-1999 (canor01)
**	    Rename member name "class" to "mo_class" to prevent reserved-word
**	    conflict.
**      24-mar-1999 (hweho01)
**          Increase MO_DEF_MEM_LIMIT for ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Sep-2000 (hanje04)
**          Increase MO_DEF_MEM_LIMIT for axp_lnx) (Alpha Linux).
**/

/* some forward refs */

typedef struct mo_class MO_CLASS;

typedef struct mo_mon_block MO_MON_BLOCK;

# define MO_DEF_STR_LIMIT	((i4)1024 * 1024)

      /* allow for 64-bit pointers taking up more room */
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx)
# define MO_DEF_MEM_LIMIT	(1936 * 1024 * 2)
#else
# define MO_DEF_MEM_LIMIT	((i4)1936 * 1024)
#endif

int spptree();
VOID MO_dumpmem(char *,i4);

/* for possible 64 bit machines */

# define MO_MAX_NUM_BUF 30

# define MO_IS_TRUNCATED( stat ) \
	( stat == MO_CLASSID_TRUNCATED || \
	 stat == MO_INSTANCE_TRUNCATED || \
	 stat == MO_VALUE_TRUNCATED )

/*
** Some arbitrary limits, maybe these should not exist.
*/
# define MO_MAX_CLASSID 255
# define MO_MAX_INSTANCE 255
# define MO_MAX_INDEX	255
# define MO_MAX_VALUE	255

/* some internal classes */

# define MO_NUM_MUTEX		"exp.glf.mo.num.mutex"
# define MO_NUM_UNMUTEX		"exp.glf.mo.num.unmutex"
# define MO_NUM_DEL_MONITOR	"exp.glf.mo.num.del_monitor"

/*}
** Name:	MO_MON_BLOCK	- Block in the monitor table.
**
** Description:
**	
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

struct mo_mon_block
{
    SPBLK node;			/* node.key points to block base */
    MO_CLASS	*mo_class;	/* class of parent (not twin) */
    MO_MONITOR_FUNC *monitor;	/* the function */
    PTR		mon_data;	/* the cookie */
    char	*qual_regexp;	/* the regexp, or NULL for all instances,
				   always allocated from strings table */
};

/*}
** Name:	MO_CLASS	- node in the MO_classes tree
**
** Description:
**	node in the MO_classes tree
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

struct mo_class
{
    SPBLK	node;		/* classid in node.key */
    i4		cflags;		/* MO_{CLASS,INSTANCE}_CONST */
    i4		size;		/* size of object */
    i4		perms;		/* permissions on all instances */
    i4		interp;		/* suggested interpretation */
    char	*index;		/* classids forming the index,
				   comma separated */
    i4		offset;		/* value handed to get/set method */
    MO_GET_METHOD *get;		/* get conversion */
    MO_SET_METHOD *set;		/* set conversion */

    PTR		cdata;		/* global data for classid, passed
				   to index methods */
    MO_INDEX_METHOD *idx;	/* for get/getnext */

    MO_CLASS	*twin;
    MO_MON_BLOCK	*monitor; /* first monitor for the classid */
};


/*}
** Name:	MO_INSTANCE	- node in MO_instances tree of attached objects
**
** Description:
**	node in MO_instances tree of attached objects
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/
typedef struct
{
    SPBLK	node;		/* key points to self */
    i4		iflags;		/* MO_INSTANCE_CONST */
    char	*instance;
    MO_CLASS	*classdef;	/* the class definition, not null */
    PTR		idata;		/* instance-specific data, handed
				   to get/set methods */
} MO_INSTANCE;


/*}
** Name:	MO_STRING	- block in the strings table.
**
** Description:
**	Block in the strings table.  These are really allocated
**	as (sizeof SPBLK + sizeof(i4) + strlen( string ) + 1)
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/
typedef struct
{
    SPBLK node;			/* node.key points to buf */
    i4	refs;			/* for garbage collection */
    char buf[1];		/* really allocated to length */
} MO_STRING;


/* My trees */

GLOBALREF SPTREE *MO_classes;
GLOBALREF SPTREE *MO_instances;
GLOBALREF SPTREE *MO_strings;
GLOBALREF SPTREE *MO_monitors;

/* My state */

GLOBALREF bool	MO_disabled;	/* affected by MOon_off */

/* My statistics */

GLOBALREF i4 MO_nattach;
GLOBALREF i4 MO_ndetach;
GLOBALREF i4 MO_nclassdef;
GLOBALREF i4 MO_ndel_monitor;
GLOBALREF i4 MO_nset_monitor;
GLOBALREF i4 MO_ntell_monitor;

GLOBALREF i4 MO_nget;
GLOBALREF i4 MO_ngetnext;
GLOBALREF i4 MO_nset;

GLOBALREF i4 MO_nmutex;
GLOBALREF i4 MO_nunmutex;

GLOBALREF MO_CLASS_DEF MO_mem_classes[];
GLOBALREF MO_CLASS_DEF MO_meta_classes[];
GLOBALREF MO_CLASS_DEF MO_mon_classes[];
GLOBALREF MO_CLASS_DEF MO_str_classes[];
GLOBALREF MO_CLASS_DEF MO_tree_classes[];

/* moattach.c */

FUNC_EXTERN MO_INSTANCE *MO_getinstance( char *classid, char *instance );
FUNC_EXTERN MO_INDEX_METHOD MO_index;

/* moclass.c */

FUNC_EXTERN STATUS MO_getclass( char *classid, MO_CLASS **cp );
FUNC_EXTERN STATUS MO_nxtclass( char *classid, MO_CLASS **cp );

/* moget.c */

FUNC_EXTERN STATUS MO_get( i4  valid_perms,
			 char *classid,
			 char *instance,
			 i4  *lsbufp,
			 char *sbuf,
			 i4  *operms );

/* mogetnext.c */

FUNC_EXTERN STATUS MO_getnext(i4 valperms,
			      i4  lclassid,
			      i4  linstance,
			      char *classid,
			      char *instance,
			      i4  *lsbufp,
			      char *sbuf,
			      i4  *operms );

/* momem.c */

FUNC_EXTERN PTR MO_alloc( i4  size, STATUS *stat );
FUNC_EXTERN VOID MO_free( PTR obj, i4  size );

/* momon.c */

FUNC_EXTERN STATUS MO_tell_class( MO_CLASS *cp, char *instance,
				 char *value, i4  op );

/* momutex.c */

FUNC_EXTERN STATUS MO_mutex(void);
FUNC_EXTERN STATUS MO_unmutex(void);

/* mostr.c */

FUNC_EXTERN char *MO_defstring( char *s, STATUS *stat );
FUNC_EXTERN VOID MO_delstring( char *s );

/* moutil.c */


FUNC_EXTERN VOID MO_once(void);
FUNC_EXTERN VOID MO_breakpoint(void);
FUNC_EXTERN void MO_str_to_uns( char *a, u_i4 *u);

/* end of moint.h */


