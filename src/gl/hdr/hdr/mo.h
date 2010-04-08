/*
** mo.h - interface for the proposed mi (management information) module
**
** History:
**	25-Oct-91 (daveb)
**		Created from old experimental mi.h
**	14-Jan-92 (daveb)
**		Updated to MO.9 spec.
**	31-Jan-92 (daveb)
**		Updated to 1.1
**	4-Dec-1992 (daveb)
**	        Changed exp.gl.glf. to exp.glf for brevity.
**		Add MO_META_OFFSET_CLASS.  Would _really_ like something
**		to help determine the type, char or int, and something
**		for char buf size.
**	19-Aug-1993 (daveb)
**	    add missing externs for new MOptrget and MOpvget
**	17-sep-1993 (swm)
**		Add missing extern for MOptrout.
**	09-Sep-1996 (johna)
**		Add extern for new MOpidget(), MOtimestampget
**	14-aug-1998 (canor01)
**	    Removed dependency on compiled erglf.h file, since this header
**	    is now included in base CL files.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-jul-2003 (devjo01)
**	    Add MOptrxout(), MOsidget() & MOsidout().
**	 6-Apr-04 (gordy)
**	    Added MO_ANY_READ and MO_ANY_WRITE attributes to permit
**	    access by unprivileged clients.
**	24-Oct-05 (toumi01) BUG 115449
**	    We should not hold the MO mutex for some handler calls, else
**	    we can deadlock on ourselves.
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Amend headers for MOlongout and MOulongout functions for 
**          handling 8-byte integers. 
*/

# ifndef MO_INCLUDED
# define MO_INCLUDED

/*
** STATUS values of various functions, from <erglf.h>
*/
# define E_GL_MASK              0xD50000
# define E_MO_MASK              0x4000
# define MO_ALREADY_ATTACHED	(E_GL_MASK + E_MO_MASK + 0x01)
# define MO_BAD_MONITOR		(E_GL_MASK + E_MO_MASK + 0x03)
# define MO_BAD_MSG		(E_GL_MASK + E_MO_MASK + 0x04)
# define MO_BAD_SIZE		(E_GL_MASK + E_MO_MASK + 0x05)
# define MO_CLASSID_TRUNCATED	(E_GL_MASK + E_MO_MASK + 0x06)
# define MO_INSTANCE_TRUNCATED	(E_GL_MASK + E_MO_MASK + 0x08)
# define MO_NO_CLASSID		(E_GL_MASK + E_MO_MASK + 0x09)
# define MO_NO_DETACH		(E_GL_MASK + E_MO_MASK + 0x0A)
# define MO_NO_INSTANCE		(E_GL_MASK + E_MO_MASK + 0x0B)
# define MO_NO_NEXT		(E_GL_MASK + E_MO_MASK + 0x0C)
# define MO_NO_READ		(E_GL_MASK + E_MO_MASK + 0x0D)
# define MO_NO_STRING_SPACE	(E_GL_MASK + E_MO_MASK + 0x0E)
# define MO_NO_WRITE		(E_GL_MASK + E_MO_MASK + 0x0F)
# define MO_NULL_METHOD		(E_GL_MASK + E_MO_MASK + 0x10)
# define MO_VALUE_TRUNCATED	(E_GL_MASK + E_MO_MASK + 0x11)
# define MO_MEM_LIMIT_EXCEEDED  (E_GL_MASK + E_MO_MASK + 0x12)

/* MACRO for detemining member size, like CL_OFFSETOF */

/* this is ugly but avoids shared lib problems on VMS */

static ALIGN_RESTRICT        MO_trick_expect_unused_warning;

# define MO_SIZEOF_MEMBER(s_type, m_name) \
		(sizeof( ((s_type *)&MO_trick_expect_unused_warning)->m_name) )


/* "zero-points-to" isn't legal portable C */

/* # define MO_SIZEOF_MEMBER(struct, member) (sizeof((struct *)0)->member) */


/*
** Argument operations to MOon_off();
*/
# define MO_ENABLE	1
# define MO_DISABLE	2
# define MO_INQUIRE	4

/*
** Permissions on class definition.  These are octal constants,
** consistant with UNIX perm bit meanings, 2=read, 4=write, 1=?
*/
# define MO_PERM_NONE			0000000

# define MO_SES_READ			0000002
# define MO_SES_WRITE			0000004

# define MO_DBA_READ			0000020
# define MO_DBA_WRITE			0000040

# define MO_SERVER_READ			0000200
# define MO_SERVER_WRITE		0000400

# define MO_SYSTEM_READ			0002000
# define MO_SYSTEM_WRITE		0004000

# define MO_SECURITY_READ		0020000
# define MO_SECURITY_WRITE		0040000

/* mask for permission bits */

# define MO_PERMS_MASK			0066666

/* Modifiers to above permissions */

# define MO_PERMANENT			0100000

/* 
** Allow unprivileged access.  The following
** modifiers disable the permission bits and
** should only be used in limited cases when
** object access is completely unrestricted.
** MO_READ and MO_WRITE should generally be
** used instead.
*/

# define MO_ANY_READ			0200000	/* Override read perm bits */
# define MO_ANY_WRITE			0400000	/* Override write perm bits */

/* all may read, or mask for someone may read */

# define MO_READ	( MO_SES_READ | MO_DBA_READ | MO_SERVER_READ | \
			  MO_SYSTEM_READ | MO_SECURITY_READ )

/* all may write, or mask for someone may write */

# define MO_WRITE	( MO_SES_WRITE | MO_DBA_WRITE | MO_SERVER_WRITE | \
			  MO_SYSTEM_WRITE | MO_SECURITY_WRITE )

/*
** Msg types for methods and monitors.
*/
# define MO_GET		1	/* access */
# define MO_GETNEXT	2	/* access */
# define MO_SET		3	/* access and monitor */
# define MO_DELETE	4	/* monitor */
# define MO_ATTACH	5	/* monitor */
# define MO_DETACH	6	/* monitor */
# define MO_SET_MON	7	/* monitor, for client use */
# define MO_DEL_MON	8	/* monitor, for client use */

/*
** Wired in classes
*/
# define MO_NUM_ATTACH		"exp.glf.mo.num.attach"
# define MO_NUM_CLASSDEF	"exp.glf.mo.num.classdef"
# define MO_NUM_DETACH		"exp.glf.mo.num.detach"
# define MO_NUM_GET		"exp.glf.mo.num.get"
# define MO_NUM_GETNEXT		"exp.glf.mo.num.getnext"
# define MO_NUM_SET_MONITOR	"exp.glf.mo.num.set_monitor"
# define MO_NUM_TELL_MONITOR	"exp.glf.mo.num.tell_monitor"

# define MO_NUM_ALLOC		"exp.glf.mo.num.alloc"
# define MO_NUM_FREE		"exp.glf.mo.num.free"

# define MO_MEM_LIMIT		"exp.glf.mo.mem.limit"
# define MO_MEM_BYTES		"exp.glf.mo.mem.bytes"

# define MO_STR_LIMIT		"exp.glf.mo.strings.limit"
# define MO_STR_BYTES		"exp.glf.mo.strings.bytes"
# define MO_STR_FREED		"exp.glf.mo.strings.freed"
# define MO_STR_DEFINED		"exp.glf.mo.strings.defined"
# define MO_STR_DELETED		"exp.glf.mo.strings.deleted"

# define MO_STRING_CLASS	"exp.glf.mo.strings.vals"
# define MO_STRING_REF_CLASS	"exp.glf.mo.strings.refs"

# define MO_META_CLASSID_CLASS	"exp.glf.mo.meta.classid"
# define MO_META_OID_CLASS	"exp.glf.mo.meta.oid"
# define MO_META_CLASS_CLASS	"exp.glf.mo.meta.class"
# define MO_META_SIZE_CLASS	"exp.glf.mo.meta.size"
# define MO_META_OFFSET_CLASS	"exp.glf.mo.meta.offset"
# define MO_META_PERMS_CLASS	"exp.glf.mo.meta.perms"
# define MO_META_INDEX_CLASS	"exp.glf.mo.meta.index"

/* currently unused */

# define MO_META_SYNTAX_CLASS	"exp.glf.mo.meta.syntax"
# define MO_META_ACCESS_CLASS	"exp.glf.mo.meta.access"
# define MO_META_DESC_CLASS	"exp.glf.mo.meta.desc"
# define MO_META_REF_CLASS	"exp.glf.mo.meta.ref"

/*
** typedefs for conversion and index methods.
*/

typedef STATUS MO_GET_METHOD( i4  offset,
			    i4  objsize,
			    PTR object,
			    i4  luserbuf,
			    char *userbuf );

typedef STATUS MO_SET_METHOD( i4  offset,
			    i4  luserbuf,
			    char *userbuf,
			    i4  objsize,
			    PTR object );

/* Msg is MO_GET or MO_GETNEXT */

typedef STATUS MO_INDEX_METHOD(i4 msg,
			       PTR cdata,
			       i4  linstance,
			       char *instance, 
			       PTR *instdata );

/* Msg is MO_ATTACH, MO_DETACH, etc, or any value given MOtell_monitor */

typedef STATUS MO_MONITOR_FUNC( char *classid,
				char *twinid,
				char *instance,
				char *value,
				PTR mon_data,
				i4 msg );
/*
** Flag values in MO_CLASS_DEF and MO_INSTANCE_DEF.
*/

/* classid string is not permanent -- save it */

# define	MO_CLASSID_VAR		(1 << 0)

/* instance string is not permanent -- save it */

# define	MO_INSTANCE_VAR		(1 << 1)

/* index string is not permanent -- save it */

# define	MO_INDEX_VAR		(1 << 2)

/* cdata isn't permanent -- save it */

# define	MO_CDATA_VAR		(1 << 3)

/* use classid string as index, ignore MO_INDEX_VAR */

# define	MO_INDEX_CLASSID	(1 << 4)

/* use classid string for cdata, ignore MO_CDATA_VAR */

# define	MO_CDATA_CLASSID	(1 << 5)

/* use index string for cdata, ignore MO_CDATA_VAR */

# define	MO_CDATA_INDEX		(1 << 6)

/* don't hold MO mutex when calling handler for this class */
# define	MO_NO_MUTEX		(1 << 7)

/*
** Element in a table passed to MOclassdef
*/

typedef struct
{
    i4		flags;		/* MO_CLASSID_VAR | MO_INDEX_VAR */
    char	*classid;	/* name of the class */
    i4		size;		/* size of object */
    i4		perms;		/* permissions on all instances */
    char	*index;		/* classids forming the index,
				   comma separated */
    i4		offset;		/* value handed to get/set method */
    MO_GET_METHOD *get;		/* get conversion */
    MO_SET_METHOD *set;		/* set conversion */

    PTR		cdata;		/* global data for classid, passed
				   to index methods */
    MO_INDEX_METHOD *idx;	/* for get/getnext.  NULL means
				   use attached instances */
} MO_CLASS_DEF;

/*
** Function Declarations
*/

FUNC_EXTERN STATUS MOattach( i4  flags, char *classid, char *instance, PTR idata );

FUNC_EXTERN STATUS MOclassdef( i4  nelem, MO_CLASS_DEF *cdp );

FUNC_EXTERN STATUS MOdetach( char *classid, char *instance );

FUNC_EXTERN STATUS MOget( i4  valid_perms,
			 char *classid,
			 char *instance,
			 i4  *lsbufp,
			 char *sbuf,
			 i4  *operms );

FUNC_EXTERN STATUS MOgetnext( i4  valid_perms,
			     i4  lclassid,
			     i4  linstance,
			     char *classid,
			     char *instance,
			     i4  *lsbufp,
			     char *sbuf,
			     i4  *operms );

FUNC_EXTERN STATUS MOon_off( i4  operation, i4  *old_state );

FUNC_EXTERN STATUS MOset( i4  valid_perms,
			 char *classid,
			 char *instance,
			 char *sval );

FUNC_EXTERN STATUS MOset_monitor( char *classid,
				 PTR mon_data,
				 char *qual_regexp,
				 MO_MONITOR_FUNC *monitor,
				 MO_MONITOR_FUNC **old_monitor );

FUNC_EXTERN STATUS MOtell_monitor( char *classid,
				  char *instance,
				  char *value,
				  i4  msg );

/* public utilities */

FUNC_EXTERN STATUS MOsptree_attach( SPTREE *t );

FUNC_EXTERN STATUS MOlongout(STATUS errstat,  i8 val,
			      i4  destlen, char *dest );

FUNC_EXTERN STATUS MOulongout(STATUS errstat,  u_i8 val,
			      i4  destlen, char *dest );

FUNC_EXTERN STATUS MOptrout(STATUS errstat,  PTR ptr,
			      i4  destlen, char *dest );

FUNC_EXTERN STATUS MOptrxout(STATUS errstat,  PTR ptr,
			      i4  destlen, char *dest );

FUNC_EXTERN STATUS MOsidout(STATUS errstat,  PTR ptr,
			      i4  destlen, char *dest );

FUNC_EXTERN STATUS MOstrout( STATUS errstat, char *src,
			     i4  destlen, char *dest );

FUNC_EXTERN STATUS MOlstrout( STATUS errstat,
			     i4  srclen, char *src,
			     i4  destlen, char *dest );

/* public methods */

FUNC_EXTERN MO_GET_METHOD MOblankget;
FUNC_EXTERN MO_GET_METHOD MOintget;
FUNC_EXTERN MO_GET_METHOD MOuivget;
FUNC_EXTERN MO_GET_METHOD MOuintget;
FUNC_EXTERN MO_GET_METHOD MOivget;
FUNC_EXTERN MO_GET_METHOD MOpidget;
FUNC_EXTERN MO_GET_METHOD MOptrget;
FUNC_EXTERN MO_GET_METHOD MOpvget;
FUNC_EXTERN MO_GET_METHOD MOstrget;
FUNC_EXTERN MO_GET_METHOD MOstrpget;
FUNC_EXTERN MO_GET_METHOD MOtimestampget;
FUNC_EXTERN MO_GET_METHOD MOzeroget;
FUNC_EXTERN MO_GET_METHOD MOsidget;

FUNC_EXTERN MO_SET_METHOD MOnoset;
FUNC_EXTERN MO_SET_METHOD MOintset;
FUNC_EXTERN MO_SET_METHOD MOuintset;
FUNC_EXTERN MO_SET_METHOD MOstrset;

FUNC_EXTERN MO_INDEX_METHOD MOcdata_index;
FUNC_EXTERN MO_INDEX_METHOD MOidata_index;
FUNC_EXTERN MO_INDEX_METHOD MOname_index;

# endif				/* multiple inclusion gate */

