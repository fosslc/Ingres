/*
** Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <bt.h>
#include    <ci.h>
#include    <cv.h>
#include    <qu.h>
#include    <st.h>
#include    <cm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <copy.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <sxf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psftrmwh.h>
#include    <uld.h>


/*
** Name: PSLWITHOPTS.C -- WITH-option semantic actions
**
** Description:
**
**	This file contains data and routines for handling the
**	various WITH-options that occur on various DDL statements:
**	create-like statements and MODIFY being the majority.
**	Many of the WITH-options allowed by these statements are
**	shared, and it makes sense to try to deal with them in
**	one place as much as possible.
**
**	This file is NOT about parsing WITH-options for DML (the
**	COPY statement being an exception), nor does it have anything
**	to do with the WITH-clause for common table expressions.
**
**	The general idea is to parse the various options into a
**	holding area in a form that is easy to cross-check at the
**	end.  The DMU_CHARACTERISTICS structure is defined specially
**	for this, with a bit-mask indicating which options we've
**	seen, plus holders for (most) options.  There's room for
**	some extra indicators that DMF doesn't care about, that
**	we use for tracking what WITH-options have occurred.
**	Thus, WITH-options set an indicator, plus maybe a value in
**	the DMU_CHARACTERISTICS, plus maybe some temporary values
**	in the PSS_YYVARS or occasionally in the DMU_CB itself.
**
**	Note that not all statements have a DMU_CB.  In particular,
**	COPY and constraint-with doesn't have one.  There's always
**	a DMU_CHARACTERISTICS available, but not always a DMU_CB.
**
**	Please note that (as of Oct 2010) we still don't have ALL of
**	the WITH-parsing consolidated here.  The REGISTER statement
**	has some funky hand-parsed options, as does the COPY statement.
**	Eventually, REGISTER should be consolidated here.  The COPY
**	options are a bit less of a concern, as most of them are
**	entirely unique to COPY and don't cause as much of a problem
**	as the options usable by multiple DDL statements.
**
** History:
**	7-Oct-2010 (kschendel) SIR 124544
**	    After years of pissing and moaning about the scattered and
**	    amorphous handling of DDL with-options for years, do something
**	    about it.  I didn't really expect this to expand into a
**	    replacement of the dmu_char_array too, but so it goes...
*/

/* Define the master tables of options.  There's one table for each
** general form:  option=string, option=hex, option=number, option,
** and option=(list).  The grammar has to discern these separately
** anyway, so it knows a priori which option table to look at.
**
** Each option table entry has:
**   - the option name (lowercase)
**   - An option name to use in the error message for a duplicate option
**     seen error.  Usually this is the UPPERCASE option name but not
**     always.
**   - a bit-mask indicating which statement types may use that
**     option.  (A bit-mask is used because many options are valid
**     for multiple statement types.)  "Statement type" roughly
**     translates to psq_qmode, but it may take additional info into
**     account (e.g. the various modify actions).
**   - The DMU indicator flag to set indicating that the option has
**     been seen and its value stored.
**   - A boolean saying whether the option is allowed with SQL
**   - A boolean saying whether the option is allowed with QUEL
**   - A boolean saying whether the option is allowed inside a
**     PARTITION=() clause (almost always NO).
**   - An action code saying how to store the option.  Many options
**     need special handling or special testing, but many other options
**     do a common action such as "set a flag bit" or "store an
**     integer value".
**   - The flag-bit to set if the action is PSL_ACT_FLAGSET.   Or,
**     the yyvars list_clause value to set for PSL_ACT_LIST.  Or, some
**     sort of value (e.g. on vs off) for some of the option specific
**     actions.  Irrelevant otherwise.
**   - The minimum and maximum values to test against if the action is
**     PSL_ACT_I4.  Irrelevant otherwise.
**   - The offset within a DMU_CHARACTERISTICS structure to store the (i4)
**     value if the action is PSL_ACT_I4.  Irrelevant otherwise.
*/

/* First we need some action codes.  Actions are:
**   - no-op:  Do nothing to "set" the option value (other than setting
**     the matching indicator bit, which is ALWAYS done).
**   - flag:  Set the given flag in DMU_CHARACTERISTICS's dmu_flags.  (There's
**     no "flag clear" because we assume that dmu_flags starts out zero.
**     The indicators distinguish between "no" and "not specified".)
**   - i4:  Check the value against the given minimum/maximum, and then
**     store it into the DMU_CHARACTERISTICS structure at the given offset
**   - list:  Set the "list_clause" type to the given value.
**   - something else:  all the other actions are specific to the
**     WITH-option, either because special additional validation is needed,
**     or because it's not plausible to handle the option value generically.
*/

enum psl_action_codes {
    PSL_ACT_NOOP,		/* Do nothing */
    PSL_ACT_FLAG,		/* Set a cur_chars dmu_flags flag */
    PSL_ACT_I4,			/* Set a cur_chars value after minmax check */
    PSL_ACT_LIST,		/* Store a list_clause type */

    /* These are the non-generic option specific actions */
    PSL_ACT_AESKEY,		/* aeskey = hex */
    PSL_ACT_COMPKWD,		/* compression (keyword alone) */
    PSL_ACT_ENCRYPTION,		/* encryption = string */
    PSL_ACT_EXTONLY,		/* [no]extensions only */
    PSL_ACT_INDEX,		/* index = string */
    PSL_ACT_JNL,		/* journaling */
    PSL_ACT_KEY,		/* key=() needs extra checking */
    PSL_ACT_LOCN, 		/* location=(), special setup check */
    PSL_ACT_NOJNLCHK,		/* nojournal_check special validation */
    PSL_ACT_NOPART,		/* nopartition */
    PSL_ACT_NPASSPHRASE,	/* new_passphrase = string */
    PSL_ACT_PAGESIZE,		/* page_size = NNN (special validation) */
    PSL_ACT_PARTITION,		/* partition = (), special setup */
    PSL_ACT_PASSPHRASE,		/* passphrase = string */
    PSL_ACT_STRUCT,		/* structure = string */
    PSL_ACT_USCOPE		/* unique_scope = string */
};


/* Define a bunch of "query bits" that correspond to query types (qmode),
** but include various sub-types for the MODIFY statement.
** Note 1: there are LOTS more MODIFY actions, including some fairly
** common ones like modify to truncated;  but they don't have any WITH-
** clause validation implications.
** Note 2: if you follow the grammar, you'll see that PSL_0_CRT_LINK can
** get to some of the WITH-clause grammar bits such as twith_name;  but
** it has to be a STAR session, and we divert to STAR text-hacking before
** any of the normal option stuff gets done.
*/

#define PSL_QBIT_CREATE		0x01	/* PSQ_CREATE */
#define PSL_QBIT_CTAS		0x02	/* PSQ_RETINTO */
#define PSL_QBIT_DGTT		0x04	/* PSQ_DGTT */
#define PSL_QBIT_DGTTAS		0x08	/* PSQ_DGTT_AS_SELECT */
#define PSL_QBIT_INDEX		0x10	/* PSQ_INDEX (non-vectorwise) */
#define PSL_QBIT_VWINDEX	0x20	/* PSQ_X100_CRINDEX (vectorwise index) */
#define PSL_QBIT_CONS		0x40	/* PSQ_CONS (constraint) */
#define PSL_QBIT_MODNONE	0x80	/* PSQ_MODIFY no action yet, e.g.
					** modify tablename to priority=3 */
#define PSL_QBIT_MODSTOR	0x100	/* PSQ_MODIFY to storage-structure */
#define PSL_QBIT_MODRELOC	0x200	/* PSQ_MODIFY to relocate */
#define PSL_QBIT_MODREORG	0x400	/* PSQ_MODIFY to reorganize */
#define PSL_QBIT_MODTRUNC	0x800	/* PSQ_MODIFY to truncated */
#define PSL_QBIT_MODADD		0x1000	/* PSQ_MODIFY to add_extend */
#define PSL_QBIT_MODENC		0x2000	/* PSQ_MODIFY to encrypt */
#define PSL_QBIT_MODDBG		0x4000	/* PSQ_MODIFY to table_debug */
#define PSL_QBIT_MODANY		0x8000	/* PSQ_MODIFY with ANY action
				** All of the above psl_qbit_modxxx will also
				** show PSL_QBIT_MODANY
				*/
#define PSL_QBIT_REG		0x10000	/* PSQ_REG_IMPORT */
#define PSL_QBIT_COPY		0x20000	/* PSQ_COPY (!) */
#define PSL_QBIT_OTHER		0x40000	/* Something unexpected */


/* Next we need a structure definition for the table */
typedef struct {
	char	*keyword;	/* The option name (lowercase) */
	char	*dupword;	/* Error string for duplicate-option error */
	u_i4	valid_qmodes;	/* The statement types allowing this option */
	i1	with_id;	/* The indicators bit # for this option */
	bool	sql_ok;		/* TRUE if option OK in SQL */
	bool	quel_ok;	/* TRUE if option OK in QUEL */
	bool	part_ok;	/* TRUE if option OK in PARTITION= */
	enum psl_action_codes action;  /* Action to check/store option */
	u_i4	flag;		/* Flag or list_clause type or option specific
				** value for option specific action */
	i4	minvalue;	/* Minimum value (I4 action) */
	i4	maxvalue;	/* Maximum value (I4 action) */
	u_i4	offset;		/* DMU_CHARACTERISTICS struct offset (I4 action) */
} PSL_WITH_TABLE;

/*    {"option-name", "NAME-FOR-DUPLICATE-OPTION-ERROR-MSG",
**	 valid_qmodes_bitmask,
**	 indicator-bit-to-check-and-set, sql-OK?, quel-OK?, part-OK?,
**	 action code, action-flag-or-type-or-value,
**	 i4-value-min, i4-value-max, i4-value-dmu-chars-offset
**    }
**
** Option tables are searched sequentially, so if you feel the need to
** shave off a few nanoseconds, you can put the most common options first.
*/


/* WITH-option table for option = string WITH-options.
** (psl_nm_eq_nm style.)
*/

static const PSL_WITH_TABLE nm_eq_nm_options[] =
{
    /* STRUCTURE=storage-structure-name */
    {"structure", "STRUCTURE",
	 PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	     PSL_QBIT_INDEX | PSL_QBIT_VWINDEX | PSL_QBIT_CONS,
	 DMU_STRUCTURE, TRUE, TRUE, FALSE,
	 PSL_ACT_STRUCT, 0,
	 0, 0, 0
    },

    /* UNIQUE_SCOPE = STATEMENT or ROW */
    {"unique_scope", "UNIQUE_SCOPE",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_MODNONE,
	DMU_STATEMENT_LEVEL_UNIQUE, TRUE, FALSE, FALSE,
	PSL_ACT_USCOPE, 0,
	0, 0, 0
    },

    /* INDEX = constraint-index-name */
    {"index", "INDEX",
	PSL_QBIT_CONS,
	PSS_WC_CONS_INDEX, TRUE, FALSE, FALSE,
	PSL_ACT_INDEX, 0,
	0, 0, 0
    },

    /* ENCRYPTION = encryption-type-bits */
    {"encryption", "ENCRYPTION",
	PSL_QBIT_CREATE | PSL_QBIT_MODSTOR,
	PSS_WC_ENCRYPT, TRUE, TRUE, FALSE,
	PSL_ACT_ENCRYPTION, 0,
	0, 0, 0
    },

    /* PASSPHRASE = encryption-unlocking-passphrase */
    {"passphrase", "PASSPHRASE",
	PSL_QBIT_CREATE | PSL_QBIT_MODENC,
	PSS_WC_PASSPHRASE, TRUE, TRUE, FALSE,
	PSL_ACT_PASSPHRASE, 0,
	0, 0, 0
    },

    /* NEW_PASSPHRASE = new-encryption-unlocking-passphrase */
    {"new_passphrase", "NEW_PASSPHRASE",
	PSL_QBIT_MODENC,
	PSS_WC_NPASSPHRASE, TRUE, TRUE, FALSE,
	PSL_ACT_NPASSPHRASE, 0,
	0, 0, 0
    },

    {NULL, NULL,
	0,
	0, FALSE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    }
};


/* WITH-option table for option = hexconstant WITH-options.
** (psl_nm_eq_hexconst style.)
*/

static const PSL_WITH_TABLE nm_eq_hex_options[] =
{
    /* AESKEY = xxxxxxx (encryption key in hex, atypical usage, for situations
    ** where multiple tables must coordinate encryption keys for binary
    ** compatibility)
    */
    {"aeskey", "AESKEY",
	PSL_QBIT_CREATE,
	PSS_WC_AESKEY, TRUE, TRUE, FALSE,
	PSL_ACT_AESKEY, 0,
	0, 0, 0
    },

    {NULL, NULL,
	0,
	0, FALSE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    }
};


/* WITH-option table for option = numeric-value WITH-options.
** (psl_nm_eq_no style.)
*/

static const PSL_WITH_TABLE nm_eq_no_options[] =
{
    /* PAGE_SIZE = power-of-two */
    {"page_size", "PAGE_SIZE",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_CONS,
	DMU_PAGE_SIZE, TRUE, TRUE, FALSE,
	PSL_ACT_PAGESIZE, 0,
	0, 0, 0
    },

    /* FILLFACTOR = data-page-fill-percent */
    {"fillfactor", "FILLFACTOR",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_DATAFILL, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, 100, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_fillfac)
    },

    /* NONLEAFFILL = btree-index-page-fill-percent */
    {"nonleaffill", "NONLEAFFILL",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_IFILL, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, 100, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_nonleaff)
    },

    /* LEAFFILL = btree-leaf-page-fill-percent */
    {"leaffill", "LEAFFILL",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_LEAFFILL, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, 100, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_leaff)
    },

    /* indexfill is a synonyn for leaffill */
    {"indexfill", "INDEXFILL",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_LEAFFILL, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, 100, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_leaff)
    },

    /* MINPAGES = min-number-of-main-hash-pages */
    {"minpages", "MINPAGES",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_MINPAGES, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, DB_MAX_PAGENO, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_minpgs)
    },

    /* MAXPAGES = max-number-of-main-hash-pages */
    {"maxpages", "MAXPAGES",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_MODSTOR |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_MAXPAGES, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, DB_MAX_PAGENO, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_maxpgs)
    },

    /* ALLOCATION = pages-to-initially-allocate-to-table
    ** Allocation is for the entire table if it's partitioned, so defer
    ** any checking until post-process checking.  (Otherwise we emit
    ** misleading bounds in the error message.)
    */
    {"allocation", "ALLOCATION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_MODTRUNC |
	    PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_ALLOCATION, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	MINI4, MAXI4, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_alloc)
    },

    /* EXTEND = pages-to-extend-table-by
    ** Extend is allowed for modify to add_extend as well as the usual.
    */
    {"extend", "EXTEND",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_MODADD |
	    PSL_QBIT_MODTRUNC | PSL_QBIT_CONS | PSL_QBIT_COPY,
	DMU_EXTEND, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, DB_MAX_PAGENO, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_extend)
    },

    /* BLOB_EXTEND = pages-to-extend-etab-by, only for modify to add_extend */
    {"blob_extend", "BLOB_EXTEND",
	PSL_QBIT_MODADD,
	DMU_BLOBEXTEND, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, DB_MAX_PAGENO, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_blobextend)
    },

    /* DIMENSION = rtree-index-dimension */
    {"dimension", "DIMENSION",
	PSL_QBIT_INDEX,
	DMU_DIMENSION, TRUE, FALSE, FALSE,
	PSL_ACT_I4, 0,
	2, DB_MAXRTREE_DIM, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_dimension)
    },

    /* PRIORITY = table-dmf-cache-priority */
    {"priority", "PRIORITY",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_MODTRUNC |
	    PSL_QBIT_MODNONE,
	DMU_TABLE_PRIORITY, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	0, DB_MAX_TABLEPRI, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_cache_priority)
    },

    /* TABLE_OPTION = n, controls modify to table_debug output */
    {"table_option", "TABLE_OPTION",
	PSL_QBIT_MODDBG,
	DMU_VOPTION, TRUE, TRUE, FALSE,
	PSL_ACT_I4, 0,
	1, 5, CL_OFFSETOF(DMU_CHARACTERISTICS, dmu_voption)
    },

    {NULL, NULL,
	0,
	0, FALSE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    }
};


/* WITH-option table for option (standalone keyword) WITH-options.
** For many of these, the flag word is used to distinguish OPT from NOOPT.
*/

static const PSL_WITH_TABLE kwd_options[] =
{
    {"compression", "COMPRESSION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_CONS,
	PSS_WC_COMPRESSION, TRUE, TRUE, FALSE,
	PSL_ACT_COMPKWD, 1,
	0, 0, 0
    },


    {"nocompression", "COMPRESSION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_CONS,
	PSS_WC_COMPRESSION, TRUE, TRUE, FALSE,
	PSL_ACT_COMPKWD, 0,
	0, 0, 0
    },

    {"duplicates", "DUPLICATES",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_REG,
	DMU_DUPLICATES, TRUE, TRUE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_DUPS,
	0, 0, 0
    },

    {"noduplicates", "DUPLICATES",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_REG,
	DMU_DUPLICATES, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {"journaling", "JOURNALING",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_REG,
	DMU_JOURNALED, TRUE, TRUE, FALSE,
	PSL_ACT_JNL, 1,
	0, 0, 0
    },

    /* Note, allow explicit nojournaling on DGTT's */
    {"nojournaling", "JOURNALING",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_REG,
	DMU_JOURNALED, TRUE, TRUE, FALSE,
	PSL_ACT_JNL, 0,
	0, 0, 0
    },

    /* There isn't a PARTITION standalone keyword, it's PARTITION=() */
    {"nopartition", "PARTITION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_MODSTOR,
	PSS_WC_PARTITION, TRUE, TRUE, FALSE,
	PSL_ACT_NOPART, 0,
	0, 0, 0
    },

    /* The apparent backwards-ness here is because NORECOVERY is the
    ** usual Ingres keyword.  RECOVERY is only recognized for gateway
    ** registers, and as best I can tell, nothing uses it!
    */
    {"recovery", "NORECOVERY",
	PSL_QBIT_REG,
	DMU_RECOVERY, TRUE, FALSE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_RECOVERY,
	0, 0, 0
    },

    {"norecovery", "NORECOVERY",
	PSL_QBIT_DGTT | PSL_QBIT_DGTTAS,
	DMU_RECOVERY, TRUE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {"autostruct", "AUTOSTRUCT",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS,
	DMU_AUTOSTRUCT, TRUE, FALSE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_AUTOSTRUCT,
	0, 0, 0
    },

    {"noautostruct", "AUTOSTRUCT",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS,
	DMU_AUTOSTRUCT, TRUE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    /* The inverse is SECURITY_AUDIT_KEY=(list) */
    {"nosecurity_audit_key", "SECURITY_AUDIT_KEY",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS,
	PSS_WC_ROW_SEC_KEY, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    /* quel variants on [no]journaling */
    {"logging", "LOGGING",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_REG,
	DMU_JOURNALED, FALSE, TRUE, FALSE,
	PSL_ACT_JNL, 1,
	0, 0, 0
    },

    {"nologging", "LOGGING",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_REG,
	DMU_JOURNALED, FALSE, TRUE, FALSE,
	PSL_ACT_JNL, 0,
	0, 0, 0
    },

    {"persistence", "PERSISTENCE",
	PSL_QBIT_INDEX | PSL_QBIT_MODSTOR,
	DMU_PERSISTS_OVER_MODIFIES, TRUE, TRUE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_PERSISTENCE,
	0, 0, 0
    },

    {"nopersistence", "PERSISTENCE",
	PSL_QBIT_INDEX | PSL_QBIT_MODSTOR,
	DMU_PERSISTS_OVER_MODIFIES, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {"concurrent_access", "CONCURRENT_ACCESS",
	PSL_QBIT_INDEX,
	DMU_CONCURRENT_ACCESS, TRUE, TRUE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_CONCUR_A,
	0, 0, 0
    },

    {"noconcurrent_access", "CONCURRENT_ACCESS",
	PSL_QBIT_INDEX,
	DMU_CONCURRENT_ACCESS, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    /* UNIQUE is generally handled with other syntax, e.g. modify foo
    ** to storage-structure UNIQUE on ..., but it's a useful standalone
    ** for ctas and dgtt-as with a structure= option.
    ** Don't need an actual flag, the indicator is enough.
    ** (there's no NOUNIQUE option.)
    */
    {"unique", "UNIQUE",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS,
	DMU_UNIQUE, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {"concurrent_updates", "CONCURRENT_UPDATES",
	PSL_QBIT_MODSTOR,
	DMU_CONCURRENT_UPDATES, TRUE, TRUE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_CONCUR_U,
	0, 0, 0
    },

    {"noconcurrent_updates", "CONCURRENT_UPDATES",
	PSL_QBIT_MODSTOR,
	DMU_CONCURRENT_UPDATES, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {"extensions_only", "EXTENSIONS_ONLY",
	PSL_QBIT_MODRELOC,	/* not reorganize? */
	PSS_WC_EXTONLY, TRUE, TRUE, FALSE,
	PSL_ACT_EXTONLY, 0,
	0, 0, 0
    },

    {"noextensions", "EXTENSIONS_ONLY",
	PSL_QBIT_MODRELOC,	/* not reorganize? */
	PSS_WC_EXTONLY, TRUE, TRUE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {"nodependency_check", "NODEPENDENCY_CHECK",
	PSL_QBIT_MODANY,
	PSS_WC_NODEP_CHECK, TRUE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    /* Needs some special checking */
    {"nojournal_check", "NOJOURNAL_CHECK",
	PSL_QBIT_CONS,
	PSS_WC_NOJNL_CHECK, TRUE, FALSE, FALSE,
	PSL_ACT_NOJNLCHK, 0,
	0, 0, 0
    },

    {"update", "UPDATE",
	PSL_QBIT_REG,
	DMU_GW_UPDT, TRUE, FALSE, FALSE,
	PSL_ACT_FLAG, DMU_FLAG_UPDATE,
	0, 0, 0
    },

    {"noupdate", "UPDATE",
	PSL_QBIT_REG,
	DMU_GW_UPDT, TRUE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    },

    {NULL, NULL,
	0,
	0, FALSE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    }
};

/* WITH-option table for option = (list)  WITH-options.
** (psl_lst_prefix style).
*/

static const PSL_WITH_TABLE list_options[] =
{
    {"compression", "COMPRESSION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_CONS,
	PSS_WC_COMPRESSION, TRUE, TRUE, FALSE,
	PSL_ACT_LIST, PSS_COMP_CLAUSE,
	0, 0, 0
    },

    {"location", "LOCATION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT | PSL_QBIT_DGTTAS |
	    PSL_QBIT_INDEX | PSL_QBIT_MODSTOR | PSL_QBIT_MODREORG |
	    PSL_QBIT_CONS | PSL_QBIT_REG,
	PSS_WC_LOCATION, TRUE, TRUE, TRUE,
	PSL_ACT_LOCN, PSS_NLOC_CLAUSE,
	0, 0, 0
    },

    {"oldlocation", "OLDLOCATION",
	PSL_QBIT_MODRELOC,
	PSS_WC_OLDLOCATION, TRUE, TRUE, FALSE,
	PSL_ACT_LIST, PSS_OLOC_CLAUSE,
	0, 0, 0
    },

    {"newlocation", "NEWLOCATION",
	PSL_QBIT_MODRELOC,
	PSS_WC_NEWLOCATION, TRUE, TRUE, FALSE,
	PSL_ACT_LIST, PSS_NLOC_CLAUSE,
	0, 0, 0
    },

    {"key", "KEY",
	PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_REG,
	PSS_WC_KEY, TRUE, TRUE, FALSE,
	PSL_ACT_KEY, PSS_KEY_CLAUSE,
	0, 0, 0
    },

    {"range", "RANGE",
	PSL_QBIT_INDEX,
	PSS_WC_RANGE, TRUE, FALSE, FALSE,
	PSL_ACT_LIST, PSS_RANGE_CLAUSE,
	0, 0, 0
    },

    {"partition", "PARTITION",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_MODSTOR,
	PSS_WC_PARTITION, TRUE, TRUE, FALSE,
	PSL_ACT_PARTITION, PSS_PARTITION_CLAUSE,
	0, 0, 0
    },

    {"security_audit", "SECURITY_AUDIT",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_REG,
	PSS_WC_SEC_AUDIT, TRUE, TRUE, FALSE,
	PSL_ACT_LIST, PSS_SECAUDIT_CLAUSE,
	0, 0, 0
    },

    {"security_audit_key", "SECURITY_AUDIT_KEY",
	PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_REG,
	PSS_WC_ROW_SEC_KEY, TRUE, TRUE, FALSE,
	PSL_ACT_LIST, PSS_SECAUDIT_KEY_CLAUSE,
	0, 0, 0
    },

    {NULL, NULL,
	0,
	0, FALSE, FALSE, FALSE,
	PSL_ACT_NOOP, 0,
	0, 0, 0
    }
};

/* Forward function declarations */

/* Handle various encryption-related actions, which are wild and
** wonderful enough to get a separate routine
*/
static DB_STATUS psl_act_encryptions(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DMU_CB *dmucb,
	char *valueptr,
	u_i2 valuelen,
	const PSL_WITH_TABLE *lookup);

/* Test for "gateway option" which gets passed through to somewhere else */
static bool psl_gw_option(
	char *name);

/* Check and store KEY=(list) list element */
static DB_STATUS psl_withlist_keyelem(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	DMU_CB *dmucb,
	char *value);

/* STAR option handler, passes text to star query text */
static DB_STATUS psl_withstar_text(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	char *option,
	char *value,
	PSS_Q_XLATE *xlated_qry);

/* Star list-option element handler, passes text to star query text */
static DB_STATUS psl_withstar_elem(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	char *value,
	PSS_Q_XLATE *xlated_qry);

static DB_STATUS psl_validate_rtree(
	PSS_SESBLK	*sess_cb,
	char		*qry,
	i4		qry_len,
	DB_ERROR	*err_blk);


/*
** Name: psl_withopt_init - Initialize for WITH-option scanning
**
** Description:
**
**	This routine is to be called when the grammar decides that a
**	WITH-clause might be on the way.  (So that proper defaulting
**	can occur, it should be called whether a WITH-clause is actually
**	seen, or not.)  We'll figure out the query mode (including
**	action if MODIFY) as a mask of PSL_QBIT_xxx bits, and figure
**	out where the DMU_CHARACTERISTICS is that we're working with.
**
** Inputs:
**	sess_cb		PSS_SESBLK session control block
**	psq_cb		Query parse control block
**
** Outputs:
**	None
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Clean up WITH-option parsing a bit more.
**	27-Oct-2010 (kschendel)
**	    Just return if a Star session.
*/

static struct PSL_QMODE_XLATE {
    i4 qmode;
    u_i4 bit;
} qmode_xlate[] = {
    {PSQ_CREATE, PSL_QBIT_CREATE},
    {PSQ_RETINTO, PSL_QBIT_CTAS},
    {PSQ_DGTT, PSL_QBIT_DGTT},
    {PSQ_DGTT_AS_SELECT, PSL_QBIT_DGTTAS},
    {PSQ_INDEX, PSL_QBIT_INDEX},
    {PSQ_X100_CRINDEX, PSL_QBIT_VWINDEX},
    {PSQ_CONS, PSL_QBIT_CONS},
    {PSQ_MODIFY, PSL_QBIT_MODANY},
    {PSQ_REG_IMPORT, PSL_QBIT_REG},
    {PSQ_COPY, PSL_QBIT_COPY},
    {0, PSL_QBIT_OTHER}
};


void
psl_withopt_init(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb)
{
    DMU_CHARACTERISTICS *dmuchars;	/* DMU chars to be filled in */
    i4 qmode;
    PSS_YYVARS *yyvars;	
    struct PSL_QMODE_XLATE *xlate;
    u_i4 qmode_bits;

    /* Star sessions will never use any of the WITH-parsing stuff.
    ** A Star session just pushes text around.  Return now if Star.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return;

    qmode = psq_cb->psq_mode;
    yyvars = sess_cb->pss_yyvars;
    yyvars->list_clause = 0;
    yyvars->secaudkey = NULL;
    yyvars->secaudit = FALSE;
    yyvars->withkey = NULL;
    yyvars->withkey_count = 0;
    yyvars->extonly = FALSE;
    if (qmode == PSQ_CONS)
	dmuchars = &sess_cb->pss_curcons->pss_dmu_chars;
    else if (qmode == PSQ_COPY)
    {
	QEF_RCB *rcb = (QEF_RCB *) sess_cb->pss_object;
	dmuchars = (DMU_CHARACTERISTICS *)
		rcb->qeu_copy->qeu_dmrcb->dmr_char_array.data_address;
    }
    else
    {
	QEU_CB *qeucb = (QEU_CB *) sess_cb->pss_object;
	DMU_CB *dmucb = (DMU_CB *) qeucb->qeu_d_cb;
	/* Everything else should be using a qeucb + dmucb */
	dmuchars = &dmucb->dmu_chars;
    }
	
    yyvars->cur_chars = dmuchars;
 
    /* Figure out the appropriate bit for the current query mode.
    ** Use a quickie table lookup for this.  Modify will need
    ** some additional attention.
    */
    for (xlate = &qmode_xlate[0]; ; ++xlate)
    {
	if (xlate->qmode == qmode || xlate->qmode == 0)
	{
	    qmode_bits = xlate->bit;
	    break;
	}
    }
    if (qmode == PSQ_MODIFY)
    {
	QEU_CB *qeucb = (QEU_CB *) sess_cb->pss_object;

	/* Relocate is in the qeu-cb, everything else is in the dmucb.
	** (relocate is different because it's a separate statement
	** in QUEL.)
	*/
	if (qeucb->qeu_d_op == DMU_RELOCATE_TABLE)
	{
	    qmode_bits |= PSL_QBIT_MODRELOC;
	}
	else
	{
	    DMU_CB *dmucb = (DMU_CB *) qeucb->qeu_d_cb;

	    switch (dmucb->dmu_action)
	    {
	    case DMU_ACT_NONE:
		qmode_bits |= PSL_QBIT_MODNONE;
		break;
	    case DMU_ACT_STORAGE:
		qmode_bits |= PSL_QBIT_MODSTOR;
		break;
	    case DMU_ACT_REORG:
		qmode_bits |= PSL_QBIT_MODREORG;
		break;
	    case DMU_ACT_TRUNC:
		qmode_bits |= PSL_QBIT_MODTRUNC;
		break;
	    case DMU_ACT_ADDEXTEND:
		qmode_bits |= PSL_QBIT_MODADD;
		break;
	    case DMU_ACT_ENCRYPT:
		qmode_bits |= PSL_QBIT_MODENC;
		break;
	    case DMU_ACT_VERIFY:
		if (dmucb->dmu_chars.dmu_vaction == DMU_V_DEBUG)
		    qmode_bits |= PSL_QBIT_MODDBG;
		break;
	    default:
		/* others don't change qmode-bits */
		break;
	    }
	}
    } /* if modify */
    yyvars->qmode_bits = qmode_bits;

    /* Set the yyvars query name string now so that everything here can
    ** use it.
    */
    psl_command_string(qmode, sess_cb, &yyvars->qry_name[0], &yyvars->qry_len);

} /* psl_withopt_init */

/*
** Name: psl_withopt_value -- Process a WITH-option and its value
**
** Description:
**
**	This routine is given a WITH-option table;  an option name;  and
**	(if relevant) an option value.  The option is looked up in the
**	table, and if it's valid, the value is processed using the
**	action code specified in the table.  The indicator bit
**	associated with the option is set, indicating that we've seen
**	that option.
**
**	The caller is responsible for dealing with a STAR session.
**	STAR sessions should never get here.
**
** Inputs:
**	sess_cb			PSS_SESBLK session control block
**	psq_cb			Query parse control block
**	with_table		PSL_WITH_TABLE to search
**	option			Option name (null terminated, lowercase)
**	valueptr		Pointer to value:  either string, or i4,
**				or NULL
**	valuelen		Only used for hex-string values at present
**	style			With-option "style" for error msgs, e.g.
**				"option=string", "keyword", etc.
**
** Outputs:
**	Sets indicator in dmu-indicators;  stores value as needed.
**	Returns E_DB_OK or error status with psf_error issued.
**	See below for E_DB_INFO return (gateway)
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Centralize DDL with-option handling.
*/

static DB_STATUS
psl_withopt_value(PSS_SESBLK * sess_cb, PSQ_CB *psq_cb,
	const PSL_WITH_TABLE *lookup, char *option,
	void *valueptr, u_i2 valuelen, char *style)
{
    bool in_partition_def;
    char *qry;
    DB_ERROR *err_blk;
    DB_STATUS status;
    DMU_CB *dmucb;
    DMU_CHARACTERISTICS *dmuchar;
    i4 err_code;
    i4 i4val;
    i4 qmode;
    i4 qry_len;
    PSS_YYVARS *yyvars;
    QEU_CB *qeucb;

    yyvars = sess_cb->pss_yyvars;
    err_blk = &psq_cb->psq_error;
    qmode = psq_cb->psq_mode;
    in_partition_def = (sess_cb->pss_stmt_flags & PSS_PARTITION_DEF) != 0;
    dmuchar = yyvars->cur_chars;
    qry = yyvars->qry_name;
    qry_len = yyvars->qry_len;

    /* Find the option, do basic validation.  If we get a NULL keyword
    ** entry in the table, the option wasn't found.
    */
    for (;;)
    {
	if (lookup->keyword == NULL
	  || STcompare(option, lookup->keyword) == 0)
	    break;
	++lookup;
    }

    if (lookup->keyword == NULL
      || (sess_cb->pss_lang == DB_SQL && !lookup->sql_ok)
      || (sess_cb->pss_lang == DB_QUEL && !lookup->quel_ok))
    {
	/* Special test for "gateway" options that are passed through. */
	if (lookup->keyword == NULL
	  && (qmode == PSQ_RETINTO || qmode == PSQ_CREATE)
	  && sess_cb->pss_lang == DB_SQL
	  && psl_gw_option(option))
	{
	    (VOID) psf_error(I_PS1002_CRTTBL_WITHOPT_IGNRD, 0L, PSF_USERERR,
		&err_code, err_blk, 1, 0, option);
	    /* psl_lst_prefix caller has to set "ignore me" list type,
	    ** return INFO to inform him
	    */
	    return (E_DB_INFO);
	}
	/* Nomatch, or match but wrong language which acts like nomatch */
	(void) psf_error(E_US07D7_2007_UNKNOWN_WITHOPT, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		qry_len, qry,
		0, option,
		0, style);
	return (E_DB_ERROR);
    }

    if ((yyvars->qmode_bits & lookup->valid_qmodes) == 0)
    {
	(void) psf_error(E_US07FE_2046_WITHOPT_USAGE, 0, PSF_USERERR,
		&err_code, err_blk, 2,
		qry_len, qry,
		0, option);
	return (E_DB_ERROR);
    }

    if (in_partition_def && !lookup->part_ok)
    {
	(void) psf_error(E_US1931_6449_PARTITION_BADOPT, 0, PSF_USERERR,
		&err_code, err_blk, 2,
		qry_len, qry, 0, option);
	return (E_DB_ERROR);
    }

    if (BTtest(lookup->with_id, dmuchar->dmu_indicators))
    {
	(void) psf_error(E_PS0BC9_DUPLICATE_WITH_CLAUSE,
		0L, PSF_USERERR, &err_code, err_blk, 3,
		qry_len, qry,
		(i4) sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
		0, lookup->dupword);
	return (E_DB_ERROR);    
    }

    /* Some actions may want to get at the DMU_CB being constructed,
    ** if any.
    */
    qeucb = NULL;
    dmucb = NULL;
    if (qmode != PSQ_CONS && qmode != PSQ_COPY)
    {
	qeucb = (QEU_CB *) sess_cb->pss_object;
	dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    }

    /* Apply the "action" to do any ad-hoc validation and store the
    ** value.  Some of these actions are standardized, and some are
    ** very ad-hoc;  that's just life.
    */
    status = E_DB_OK;
    switch (lookup->action)
    {
    case PSL_ACT_NOOP:
	/* Nothing to do!  setting the indicator bit (below) suffices. */
	break;

    case PSL_ACT_FLAG:
	/* Set a flag in dmu_flags in the current DMU_CHARACTERISTICS */
	dmuchar->dmu_flags |= lookup->flag;
	break;

    case PSL_ACT_I4:
	/* Min/max check on an i4, then store into the DMU_CHARACTERISTICS */
	i4val = *(i4 *)valueptr;
	if (i4val < lookup->minvalue || i4val > lookup->maxvalue)
	{
	    (void) psf_error(5341L, 0L, PSF_USERERR, &err_code, err_blk, 5,
		qry_len, qry,
		0, option,
		(i4) sizeof(i4), &i4val,
		(i4) sizeof(i4), &lookup->minvalue,
		(i4) sizeof(i4), &lookup->maxvalue);
	    return (E_DB_ERROR);	
	}
	*(i4 *)((char *) dmuchar + lookup->offset) = i4val;
	break;

    case PSL_ACT_LIST:
	/* List clause introducer, set the list type */
	yyvars->list_clause = lookup->flag;
	break;

    case PSL_ACT_AESKEY:
	status = psl_act_encryptions(sess_cb, psq_cb, dmucb,
		valueptr, valuelen, lookup);
	break;

    case PSL_ACT_COMPKWD:
	if (lookup->flag)
	{
	    dmuchar->dmu_dcompress = DMU_COMP_DFLT;
	    dmuchar->dmu_kcompress = DMU_COMP_DFLT;
	}
	break;

    case PSL_ACT_ENCRYPTION:
	status = psl_act_encryptions(sess_cb, psq_cb, dmucb,
		valueptr, valuelen, lookup);
	break;

    case PSL_ACT_EXTONLY:
	/* All we need to do is remember extensions_only in yyvars.
	** noextension just sets the indicator, noop otherwise.
	*/
	yyvars->extonly = TRUE;
	break;

    case PSL_ACT_INDEX:
	/* Only valid in a constraint-with context */
	STmove((char *)valueptr, ' ', sizeof(DB_TAB_NAME), 
		(char *)&sess_cb->pss_curcons->pss_restab.pst_resname);
	sess_cb->pss_curcons->pss_cons_flags |= PSS_CONS_IXNAME;
	break;

    case PSL_ACT_JNL:
	if (lookup->flag == 0)
	    dmuchar->dmu_journaled = DMU_JOURNAL_OFF;
	else if (sess_cb->pss_ses_flag & PSS_JOURNALED_DB)
	    dmuchar->dmu_journaled = DMU_JOURNAL_ON;
	else
	    dmuchar->dmu_journaled = DMU_JOURNAL_LATER;
	break;

    case PSL_ACT_KEY:
	yyvars->list_clause = PSS_KEY_CLAUSE;
	/* Extra restriction, you can't say KEY=() when it's a modify
	** to reconstruct.
	*/
	if (yyvars->qmode_bits & PSL_QBIT_MODSTOR && yyvars->md_reconstruct)
	{
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0, PSF_USERERR,
			&err_code, err_blk, 3,
			qry_len, qry,
			0, option,
			0, "RECONSTRUCT");
	    return (E_DB_ERROR);
	}
	break;

    case PSL_ACT_LOCN:
	yyvars->list_clause = PSS_NLOC_CLAUSE;
	/* For constraint with-clause, need to allocate the location
	** array, because it's not normally allocated unlike other
	** contexts that always allocate it.
	*/
	if (qmode == PSQ_CONS)
	{
	    PST_RESTAB *restab = &sess_cb->pss_curcons->pss_restab;
	    /*
	    ** Allocate the location entries.  Assume DM_LOC_MAX, although it's
	    ** probably fewer.  This is because we don't know how many locations
	    ** we have at this point, and it would be a big pain to allocate
	    ** less and then run into problems. This logic was scarfed from
	    ** psl_ct10_crt_tbl_kwd for use by ANSI constraints.
	    */
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		DM_LOC_MAX * sizeof(DB_LOC_NAME),
		&restab->pst_resloc.data_address,
		err_blk);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    restab->pst_resloc.data_in_size = 0; /* Start with 0 locns */
	}
	break;

    case PSL_ACT_NOJNLCHK:
	/* Only allowed in ALTER TABLE context (we already know that only
	** a constraint-with gets this far).
	*/
	if (yyvars->savemode != PSQ_ALTERTABLE)
	{
	    (void) psf_error(E_US1963_6499_X_ONLYWITH_Y, 0, PSF_USERERR,
			&err_code, err_blk, 3,
			qry_len, qry,
			0, option,
			0, "ALTER TABLE ADD CONSTRAINT");
	    return (E_DB_ERROR);
	}
	sess_cb->pss_curcons->pss_cons_flags |= PSS_NO_JOURNAL_CHECK;
	break;

    case PSL_ACT_NOPART:
	/* Action here depends on context; for create it's no-op */
	if (qmode == PSQ_MODIFY)
	{
	    DMT_TBL_ENTRY *showptr;

	    showptr = sess_cb->pss_resrng->pss_tabdesc;
	    if (showptr->tbl_status_mask & DMT_IS_PARTITIONED)
	    {
		/* Make sure that the entire table was listed for the modify,
		** and not just some partitions.
		** If just one partition was listed the DMU block will show
		** the partition table ID;  if some but not all were listed
		** there will be a ppchar array.
		*/
		if (dmucb->dmu_tbl_id.db_tab_index < 0
		  || dmucb->dmu_ppchar_array.data_in_size > 0)
		{
		    (void) psf_error(E_US1945_6463_LPART_NOTALLOWED, 0, PSF_USERERR,
			    &err_code, err_blk, 2,
			    0, "MODIFY",
			    0, "NOPARTITION");
		    return (E_DB_ERROR);
		}
		/* Ok, we are doing a repartitioning modify, repartitioning to
		** no partitions.  Set up some special stuff so that QEF and DMF
		** can figure out what's going on:  we need a partition def
		** with one physical partition and no dimensions.
		*/
		status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			    sizeof(DB_PART_DEF), &dmucb->dmu_part_def, err_blk);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		dmucb->dmu_partdef_size = sizeof(DB_PART_DEF);
		dmucb->dmu_part_def->nphys_parts = 1;
		dmucb->dmu_part_def->ndims = 0;
	    }
	}
	break;

    case PSL_ACT_NPASSPHRASE:
	status = psl_act_encryptions(sess_cb, psq_cb, dmucb,
		valueptr, valuelen, lookup);
	break;

    case PSL_ACT_PAGESIZE:
	/* Special validation for page size */
	i4val = *(i4 *)valueptr;
	if (i4val != 2048 && i4val != 4096 && i4val != 8192
	  && i4val != 16384 && i4val != 32768 && i4val != 65536)
	{
	    (void) psf_error(5356L, 0L, PSF_USERERR, &err_code, err_blk,
			3, qry_len, qry, 0, option,
			sizeof(i4), &i4val);
	    return (E_DB_ERROR);
	}
	dmuchar->dmu_page_size = i4val;
	break;

    case PSL_ACT_PARTITION:
	yyvars->list_clause = PSS_PARTITION_CLAUSE;
	status = psl_partdef_start(sess_cb, psq_cb, yyvars);
	break;

    case PSL_ACT_PASSPHRASE:
	status = psl_act_encryptions(sess_cb, psq_cb, dmucb,
		valueptr, valuelen, lookup);
	break;

    case PSL_ACT_STRUCT:
	/* Translate and store structure name.
	** Note, we do NOT get here for MODIFY of any sort.
	*/
	{
	    bool compressed;
	    char *ssname = (char *) valueptr;
	    i4 storestruct;

	    /* Decode storage structure */
	    compressed = (CMcmpcase(ssname, "c") == 0);
	    if (compressed)
	    {
		if (BTtest(PSS_WC_COMPRESSION, dmuchar->dmu_indicators))
		{
		    (void) psf_error(E_PS0BC4_COMPRESSION_TWICE,
			    0L, PSF_USERERR, &err_code, err_blk, 2,
			    qry_len, qry,
			    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno);
		    return (E_DB_ERROR);
		}

		BTset(PSS_WC_COMPRESSION, dmuchar->dmu_indicators);
		dmuchar->dmu_dcompress = DMU_COMP_DFLT;
		dmuchar->dmu_kcompress = DMU_COMP_DFLT;
		CMnext(ssname);
	    }

	    /* Note: this cannot return IVW structs in standard Ingres */
	    storestruct = uld_struct_xlate(ssname);
	    if (storestruct == 0)
	    {
		_VOID_ psf_error(2002L, 0L,
			PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			(i4) STtrmwhite((char *) valueptr), valueptr);
		return (E_DB_ERROR);    
	    }

	    /* Simple create table or DGTT only allows heap or IVW */
	    if ((qmode == PSQ_CREATE || qmode == PSQ_DGTT)
	      && storestruct != DB_HEAP_STORE && storestruct <= DB_STDING_STORE_MAX)
	    {
		(void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			    0, PSF_USERERR, &err_code, err_blk, 3,
			    qry_len, qry,
			    0, (char *) valueptr, qry_len, qry);
		return (E_DB_ERROR);
	    }

	    /* rtree only for index;  heap not allowed for index. */
	    if ((qmode != PSQ_INDEX && storestruct == DB_RTRE_STORE)
	      || ((qmode == PSQ_INDEX || qmode == PSQ_CONS)
		  && (storestruct == DB_HEAP_STORE || storestruct == DB_SORT_STORE)) )
	    {
		(void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			    0, PSF_USERERR, &err_code, err_blk, 3,
			    qry_len, qry,
			    0, (char *) valueptr, qry_len, qry);
		return (E_DB_ERROR);
	    }
	    if (qmode == PSQ_X100_CRINDEX && storestruct != DB_X100IX_STORE)
	    {
		(void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			    0, PSF_USERERR, &err_code, err_blk, 3,
			    qry_len, qry,
			    0, (char *) valueptr,
			    0, "a VECTORWISE base table");
		return (E_DB_ERROR);
	    }
	    dmuchar->dmu_struct = storestruct;
	}
	break;

    case PSL_ACT_USCOPE:
	/* Valid values are "row" and "statement", "statement" sets flag */
	if (STcasecmp((char *)valueptr, "statement") == 0)
	{
	    dmuchar->dmu_flags |= DMU_FLAG_UNIQUE_STMT;
	}
	else if (STcasecmp((char *)valueptr, "row") != 0)
	{
	    (void) psf_error(E_PS0BB3_INVALID_UNIQUE_SCOPE, 0L, PSF_USERERR,
			     &err_code, err_blk, 2,
			     qry_len, qry,
			     STtrmwhite((char *)valueptr), valueptr);
	    return (E_DB_ERROR);    
	}
	break;

    default:
	TRdisplay("%@ psl_withopt_value: Unknown lookup action code %d, option %s\n",
		lookup->action, (char *)valueptr);
	(void) psf_error(E_PS0002_INTERNAL_ERROR, 0, PSF_INTERR,
		&err_code, err_blk, 0);
	return (E_DB_ERROR);
    } /* end switch */

    /* Some actions set status, else it's OK from before the switch */
    if (status != E_DB_OK)
	return (status);

    /* Indicate that we've done this option */
    BTset(lookup->with_id, dmuchar->dmu_indicators);

    return (E_DB_OK);

} /* psl_withopt_value */

/*
** Name: psl_act_encryptions -- Handle encryption-relation actions
**
** Description:
**	The various actions for dealing with encryption-related WITH-
**	options are mildly inter-related, and rather than putting them
**	in-line in the Big Switch in psl_withopt_value, it seemed to
**	make sense to deal with them out-of-line here.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	dmucb		DMU_CB being constructed
**	valueptr	Pointer to the option value
**	valuelen	Length of the option value if hex constant
**	lookup		Pointer to relevant WITH-option table entry
**
** Outputs:
**	Sets various flags and fields in the DMU_CB
**	Returns E_DB_OK or error status with message issued
**
** History:
**	8-Oct-2010 (kschendel)
**	    Copied from originals in pslctbl.c.
*/

static DB_STATUS
psl_act_encryptions(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, DMU_CB *dmucb,
	char *valueptr, u_i2 valuelen, const PSL_WITH_TABLE *lookup)
{
    DB_ERROR *err_blk = &psq_cb->psq_error;
    i4 err_code;
    i4 qmode = psq_cb->psq_mode;

    switch (lookup->action)
    {
    case PSL_ACT_ENCRYPTION:
	/* Special check:  don't allow ENCRYPTION= if modify and a
	** secondary index.
	*/
	if (sess_cb->pss_yyvars->qmode_bits & PSL_QBIT_MODSTOR
	  && sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IDX)
	{
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			0, PSF_USERERR, &err_code, err_blk, 3,
			0, "MODIFY",
			0, "ENCRYPTION=",
			0, "a secondary index");
	    return (E_DB_ERROR);
	}
	if (STcasecmp(valueptr, "aes128") == 0)
	    dmucb->dmu_enc_flags |= (DMU_ENCRYPTED|DMU_AES128);
	else
	if (STcasecmp(valueptr, "aes192") == 0)
	    dmucb->dmu_enc_flags |= (DMU_ENCRYPTED|DMU_AES192);
	else
	if (STcasecmp(valueptr, "aes256") == 0)
	    dmucb->dmu_enc_flags |= (DMU_ENCRYPTED|DMU_AES256);
	else
	{
	    /* invalid ENCRYPTION= type */
	    (void) psf_error(9401L, 0L, PSF_USERERR, &err_code, err_blk, 1,
			     0, valueptr);
	    return (E_DB_ERROR);
	}
	break;

    case PSL_ACT_AESKEY:
	if (((dmucb->dmu_enc_flags & DMU_AES128) && (valuelen == 16)) ||
	    ((dmucb->dmu_enc_flags & DMU_AES192) && (valuelen == 24)) ||
	    ((dmucb->dmu_enc_flags & DMU_AES256) && (valuelen == 32)))
	{
	    MEcopy(valueptr, valuelen, (PTR)dmucb->dmu_enc_aeskey);
	    dmucb->dmu_enc_aeskeylen = valuelen;
	    dmucb->dmu_enc_flags2 |= DMU_AESKEY;
	}
	else
	{
	    /* AESKEY= hex value invalid for encryption type */
	    (void) psf_error(9406L, 0L, PSF_USERERR, &err_code, err_blk, 1,
			     sizeof(valuelen), &valuelen);
	    return (E_DB_ERROR);
	}
	break;

    case PSL_ACT_PASSPHRASE:
	{
	    i4 i;
	    u_char *p;
	    u_char *pend;
	    u_char *pass_addr[4];
	    i4	pass_offset[4];
	    i4	pass, pass_start, pass_end;

	    /* This is so fancy you just know it's ugly deep down. For CREATE
	    ** we know what type of AES encryption we are dealing with, but
	    ** with MODIFY we won't know until we access the relation record
	    ** for the table, so prep all AES flavors from the user string.
	    */
	    if (qmode == PSQ_CREATE)
	    {
		pass_addr[0] = dmucb->dmu_enc_passphrase;
		if (dmucb->dmu_enc_flags & DMU_AES128)
		    pass_offset[0] = AES_128_BYTES;
		else
		if (dmucb->dmu_enc_flags & DMU_AES192)
		    pass_offset[0] = AES_192_BYTES;
		else
		if (dmucb->dmu_enc_flags & DMU_AES256)
		    pass_offset[0] = AES_256_BYTES;
		else
		{
		    /* ENCRYPTION type has not been specified */
		    (void) psf_error(9402L, 0L, PSF_USERERR, &err_code, err_blk, 0);
		    return (E_DB_ERROR);
		}
		pass_start = 0;
		pass_end = 1;
	    }
	    else
	    {
		pass_addr[1] = dmucb->dmu_enc_old128pass;
		pass_addr[2] = dmucb->dmu_enc_old192pass;
		pass_addr[3] = dmucb->dmu_enc_old256pass;
		pass_offset[1] = AES_128_BYTES;
		pass_offset[2] = AES_192_BYTES;
		pass_offset[3] = AES_256_BYTES;
		pass_start = 1;
		pass_end = 4;
	    }

	    for ( pass = pass_start ; pass < pass_end ; pass++ )
	    {
		p = pend = pass_addr[pass];
		pend += pass_offset[pass];
		MEfill(pass_offset[pass],0,(PTR)pass_addr[pass]);
		for ( i = 0; valueptr[i] ; i++ )
		{
		    *p += (u_char) valueptr[i];
		    p++;
		    if (p == pend)
			p = pass_addr[pass];
		}
		if (qmode == PSQ_MODIFY && i == 0)
		{
		    /* NULL key turns off encryption */
		    dmucb->dmu_enc_flags2 |= DMU_NULLPASS;
		}
		else if ( i < ENCRYPT_PASSPHRASE_MINIMUM )
		{
		    /* passphrase is too short */
		    (void) psf_error(9403L, 0L, PSF_USERERR, &err_code, err_blk, 0);
		    return (E_DB_ERROR);
		}
	    }
	}
	break;

    case PSL_ACT_NPASSPHRASE:
	{
	    i4 i;
	    u_char *p;
	    u_char *pend;
	    u_char *pass_addr[3];
	    i4	pass_offset[3];
	    i4	pass, pass_start, pass_end;

	    /* remember we are modifying the passphrase */
	    dmucb->dmu_enc_flags2 |= DMU_NEWPASS;

	    /* Prep all AES flavors from the user string. */
	    pass_addr[0] = dmucb->dmu_enc_new128pass;
	    pass_addr[1] = dmucb->dmu_enc_new192pass;
	    pass_addr[2] = dmucb->dmu_enc_new256pass;
	    pass_offset[0] = AES_128_BYTES;
	    pass_offset[1] = AES_192_BYTES;
	    pass_offset[2] = AES_256_BYTES;
	    pass_start = 0;
	    pass_end = 3;

	    for ( pass = pass_start ; pass < pass_end ; pass++ )
	    {
		p = pend = pass_addr[pass];
		pend += pass_offset[pass];
		MEfill(pass_offset[pass],0,(PTR)pass_addr[pass]);
		for ( i = 0; valueptr[i] ; i++ )
		{
		    *p += (u_char) valueptr[i];
		    p++;
		    if (p == pend)
			p = pass_addr[pass];
		}
		if ( i < ENCRYPT_PASSPHRASE_MINIMUM )
		{
		    /* passphrase is too short */
		    (void) psf_error(9403L, 0L, PSF_USERERR, &err_code, err_blk, 0);
		    return (E_DB_ERROR);
		}
	    }
	}
	break;

    default:
	/* Nothing else can get here */
	break;

    } /* switch */

    return (E_DB_OK);
} /* psl_act_encryptions */

/*
** Name: psl_gw_option	- check if an option looks like a GW option
**
** Description:	    Examine an option name to determine if it could be a
**		    gateway-specific option.  Note that we don't have an
**		    exhaustive list of gateway db_id codes, so we will examine
**		    the first 4 chars of the name to determine if it looks like
**		    a gateway option (i.e. the first char must be alphabetic,
**		    the next two - alphanumeric, the fourth char must be '_'.
**
** Input:
**	name	    option name
**
** Output
**	none
**
** Returns:
**	TRUE if the option name looks like a valid gateway option name;
**	FALSE otherwise
**
** Side effects:
**	none
**
** History:
**
**	02-jul-91 (andre)
**	    written
*/
static bool
psl_gw_option(
	register char	    *name)
{
    register char	*c2, *c3, *c4;

    c2 = name + CMbytecnt(name);
    c3 = c2 + CMbytecnt(c2);
    c4 = c3 + CMbytecnt(c3);

    return
	(   CMalpha(name)		  /* first char must be alphabetic    */
         && (CMalpha(c2) || CMdigit(c2))  /* second char must be alphanumeric */
         && (CMalpha(c3) || CMdigit(c3))  /* second char must be alphanumeric */
	 && !CMcmpcase(c4, "_")		  /* fourth char must be '_'	      */
	);
}

/*
** Name: psl_nm_eq_nm -- Process option = string WITH-option
**
** Description:
**	Process a WITH-option that looks like option = string.
**	"string" is usually a single keyword, but it could also be
**	an arbitrary quoted string, depending on the exact grammar
**	rule that gets us here.
**
**	Thanks to the vagaries of the various grammar rules, it
**	happens that any STAR-type stuff will have been dealt with
**	by the caller.  We'll never have a STAR session at this point.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	option		Option name string (should be lowercase already)
**	value		Option value string
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Centralize all the WITH-stuff.
*/

DB_STATUS
psl_nm_eq_nm(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, char *option, char *value)
{
    DB_STATUS status;

    /* Purely defensive:  should never get a Star session, just return if
    ** we do ... caller has to figure it out.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    status = psl_withopt_value(sess_cb, psq_cb, nm_eq_nm_options,
		option, value, 0, "option=string");
    if (status == E_DB_INFO)
	status = E_DB_OK;	/* Don't care about goofy gw status */
    return (status);
} /* psl_nm_eq_nm */

/*
** Name: psl_nm_eq_hexconst -- Process option = hexconst WITH-option
**
** Description:
**	Process a WITH-option that looks like option = hexconst.
**	The caller passes the exact string length as well as the hex.
**
**	Thanks to the vagaries of the various grammar rules, it
**	happens that any STAR-type stuff will have been dealt with
**	by the caller.  We'll never have a STAR session at this point.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	option		Option name string (should be lowercase already)
**	valuelen	Option value length
**	value		Option value string
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Centralize all the WITH-stuff.
*/

DB_STATUS
psl_nm_eq_hexconst(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, char *option,
	u_i2 valuelen, char *value)
{
    DB_STATUS status;

    /* Purely defensive:  should never get a Star session, just return if
    ** we do ... caller has to figure it out.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    status = psl_withopt_value(sess_cb, psq_cb, nm_eq_hex_options,
		option, value, valuelen, "option=hex_constant");
    if (status == E_DB_INFO)
	status = E_DB_OK;	/* Don't care about goofy gw status */
    return (status);
} /* psl_nm_eq_hexconst */

/*
** Name: psl_nm_eq_no -- Process option = number WITH-option
**
** Description:
**	Process a WITH-option that looks like option = number.
**	The caller passes an i4 as the option value.
**
**	This variant DOES need to deal with Star sessions.  If
**	the session is a Star session, rather than doing any option
**	scanning at all, we'll append the option text to the Star
**	text being built.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	option		Option name string (should be lowercase already)
**	value		i4 option value
**	xlate		PSS_Q_XLATE * text stuff for Star queries
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Centralize all the WITH-stuff.
*/

DB_STATUS
psl_nm_eq_no(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, char *option,
	i4 value, PSS_Q_XLATE *xlate)
{
    DB_STATUS status;

    /*
    ** For distributed thread, do text buildup stuff
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	char	numbuf[25];	/* converted integer (need a CV define!) */

	CVla(value, numbuf);
	status = psl_withstar_text(sess_cb, psq_cb, option, numbuf, xlate);
	return (status);

    }
    status = psl_withopt_value(sess_cb, psq_cb, nm_eq_no_options,
		option, &value, 0, "option=number");
    if (status == E_DB_INFO)
	status = E_DB_OK;	/* Don't care about goofy gw status */
    return (status);
} /* psl_nm_eq_no */

/*
** Name: psl_with_kwd -- Process keyword option WITH-option
**
** Description:
**	Process a WITH-option that is just a keyword.
**
**	This variant DOES need to deal with Star sessions.  If
**	the session is a Star session, rather than doing any option
**	scanning at all, we'll append the option text to the Star
**	text being built.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	option		Option name string (should be lowercase already)
**	xlate		PSS_Q_XLATE * text stuff for Star queries
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Centralize all the WITH-stuff.
*/

DB_STATUS
psl_with_kwd(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	char *option, PSS_Q_XLATE *xlate)
{
    DB_STATUS status;

    /*
    ** For distributed thread, do text buildup stuff
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	status = psl_withstar_text(sess_cb, psq_cb, option, NULL, xlate);
	return (status);

    }
    status = psl_withopt_value(sess_cb, psq_cb, kwd_options,
		option, NULL, 0, "keyword");
    if (status == E_DB_INFO)
	status = E_DB_OK;	/* Don't care about goofy gw status */
    return (status);
} /* psl_with_kwd */

/*
** Name: psl_withlist_prefix -- Process option=(list) WITH-option
**
** Description:
**	Process a WITH-option that prefixes a list, option=(list).
**	Assuming that the option is valid, after dealing with this
**	prefix, the caller will call psl_withlist_elem for each
**	list element.
**
**	This variant DOES need to deal with Star sessions.  If
**	the session is a Star session, rather than doing any option
**	scanning at all, we'll append the option text to the Star
**	text being built.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	option		Option name string (should be lowercase already)
**	xlate		PSS_Q_XLATE * text stuff for Star queries
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Centralize all the WITH-stuff.
*/

DB_STATUS
psl_withlist_prefix(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	char *option, PSS_Q_XLATE *xlate)
{
    DB_STATUS status;

    /*
    ** For distributed thread, do text buildup stuff
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	status = psl_withstar_text(sess_cb, psq_cb, option, "( ", xlate);
	return (status);

    }
    status = psl_withopt_value(sess_cb, psq_cb, list_options,
		option, NULL, 0, "option=(list)");
    if (status == E_DB_INFO)
    {
	/* Was a goofy gateway option, set list-clause to "ignore" */
	sess_cb->pss_yyvars->list_clause = PSS_UNSUPPORTED_CRTTBL_WITHOPT;
	status = E_DB_OK;
    }
    return (status);
} /* psl_withlist_prefix */

/*
** Name: psl_withlist_elem: Handle one list-style option element
**
** Description:
**
**	This routine accepts and stores one list-style option value
**	for an option=(list) style WITH-option.  The option name
**	(prefix) was already validated, and the type of list clause
**	stored in yyvars list_clause.
**
**	All list-style options (except one) have string-valued
**	list elements.  The one exception (RANGE=) calls a different
**	routine to store values.
**
**	In a Star context, none of the ordinary stuff happens;
**	instead, we go off and append the element to a Star query text.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	value		String element value
**	xlate		PSS_Q_XLATE Star text tracker structure
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	9-Oct-2010 (kschendel) SIR 124544
**	    Centralize all (most?) of the WITH option handling
*/

DB_STATUS
psl_withlist_elem(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	char *value, PSS_Q_XLATE *xlate)
{

    char	*command;
    DB_ERROR	*err_blk;
    DB_STATUS	status;
    DMU_CB	*dmucb;
    DMU_CHARACTERISTICS *dmuchar;  /* Where we're putting option values */
    i4		err_code;
    i4		i4val;		/* For error messages */
    i4		length;		/* of "command" */
    PSS_YYVARS	*yyvars;
    QEU_CB	*qeucb;

    yyvars = sess_cb->pss_yyvars;
    err_blk = &psq_cb->psq_error;
    command = yyvars->qry_name;
    length = yyvars->qry_len;

    /* All existing list-options expect an identifier-type element,
    ** so reject quoted-string elements.  (The grammar rules allow
    ** quoted strings in case we want to add such a thing in the future.)
    */
    if (yyvars->id_type == PSS_ID_SCONST)
    {
	(void) psf_error(E_US1942_6460_EXPECTED_BUT, 0L, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "A name",
		0, "a string constant");
	return (E_DB_ERROR);
    }

    /* If a Star thread, divert off to store the text */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	if (psq_cb->psq_mode != PSQ_CREATE && psq_cb->psq_mode != PSQ_RETINTO)
	    return (E_DB_OK);
	return psl_withstar_elem(sess_cb, psq_cb, value, xlate);
    }

    dmuchar = yyvars->cur_chars;
    qeucb = NULL;
    dmucb = NULL;
    if ((yyvars->qmode_bits & (PSL_QBIT_CONS | PSL_QBIT_COPY)) == 0)
    {
	qeucb = (QEU_CB *) sess_cb->pss_object;
	dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    }

    status = E_DB_OK;
    switch (yyvars->list_clause)
    {
    case PSS_COMP_CLAUSE:
	/* [no]key, [no|hi]data.  We'll set additional indicator bits
	** to indicate which things have been seen;  this prevents
	** nonsense like compression=(key, nokey).
	*/
	{
	    i4 with_id;

	    if (STcompare(value, "key") == 0 || STcompare(value, "nokey") == 0)
		with_id = DMU_KCOMPRESSION;
	    else if (STcompare(value, "data") == 0
	      || STcompare(value, "nodata") == 0 || STcompare(value, "hidata") == 0)
		with_id = DMU_DCOMPRESSION;
	    else
	    {
		(void) psf_error(E_US07FF_2047_WITHOPT_VALUE, 0, PSF_USERERR,
			&err_code, err_blk, 3,
			length, command,
			0, value,
			0, "COMPRESSION=()");
		return (E_DB_ERROR);
	    }
	    if (BTtest(with_id, dmuchar->dmu_indicators))
	    {
		(void) psf_error(E_PS0BC4_COMPRESSION_TWICE,
			    0L, PSF_USERERR, &err_code, err_blk, 2,
			    length, command,
			    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno);
		return (E_DB_ERROR);
	    }
	    BTset(with_id, dmuchar->dmu_indicators);
	    if (with_id == DMU_KCOMPRESSION)
	    {
		if (STcompare(value, "key") == 0)
		    dmuchar->dmu_kcompress = DMU_COMP_ON;
		else
		    dmuchar->dmu_kcompress = DMU_COMP_OFF;
	    }
	    else
	    {
		if (STcompare(value, "data") == 0)
		    dmuchar->dmu_dcompress = DMU_COMP_ON;
		else if (STcompare(value, "hidata") == 0)
		    dmuchar->dmu_dcompress = DMU_COMP_HI;
		else
		    dmuchar->dmu_dcompress = DMU_COMP_OFF;
	    }
	}
	break;

    case PSS_KEY_CLAUSE:
	status = psl_withlist_keyelem(sess_cb, psq_cb, dmucb, value);
	break;

    case PSS_NLOC_CLAUSE:
    case PSS_OLOC_CLAUSE:
	{
	    DM_DATA	*dmdata_ptr;
	    DB_LOC_NAME	*loc, *lim;

	    if (sess_cb->pss_stmt_flags & PSS_PARTITION_DEF)
		dmdata_ptr = &yyvars->part_locs;
	    else if (psq_cb->psq_mode == PSQ_CONS)
		dmdata_ptr = &sess_cb->pss_curcons->pss_restab.pst_resloc;
	    else if (yyvars->list_clause == PSS_OLOC_CLAUSE)
		dmdata_ptr = &dmucb->dmu_olocation;
	    else
		dmdata_ptr = &dmucb->dmu_location;

	    /* See if there is space for 1 more */
	    if (dmdata_ptr->data_in_size/sizeof(DB_LOC_NAME) >= DM_LOC_MAX)
	    {
		/* Too many locations */
		i4val = DM_LOC_MAX;
		(VOID) psf_error(2115L, 0L, PSF_USERERR,
			&err_code, err_blk, 3,
			length, command,
			sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			sizeof(i4), &i4val);
		return (E_DB_ERROR);
	    }

	    lim = (DB_LOC_NAME *) (dmdata_ptr->data_address +
		    dmdata_ptr->data_in_size);

	    STmove(value, ' ', (u_i4) DB_LOC_MAXNAME, (char *) lim);
	    dmdata_ptr->data_in_size += sizeof(DB_LOC_NAME);

	    /* See if not a duplicate */
	    for (loc = (DB_LOC_NAME *) dmdata_ptr->data_address;
		 loc < lim;
		 loc++
		)
	    {
		if (!MEcmp((char *) loc, (char *) lim, sizeof(DB_LOC_NAME)))
		{
		    /* A duplicate was found */
		    (VOID) psf_error(2116L, 0L, PSF_USERERR,
			&err_code, err_blk, 3,
			length, command,
			sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
			0, value);
		    return (E_DB_ERROR);
		}
	    }
	}
	break;

    case PSS_PARTITION_CLAUSE:
	/* The only way to get here is if someone put gibberish in a
	** partition= clause, because the partition option is not really
	** a list.  (It has quite complicated syntax.)
	*/
	(VOID) psf_error(E_US1950_6468_NO_PARTRULE_SPEC, 0L, 
			PSF_USERERR, &err_code, err_blk, 1,
			length, command);
        return (E_DB_ERROR);

    case PSS_SECAUDIT_CLAUSE:
	/* security_audit=(table | [no]row)
	** "table" is ignored;  [no]row sets a flag.
	*/
	if (STcompare(value, "table") != 0)
	{
	    if (BTtest(PSS_WC_ROW_SEC_AUDIT, dmuchar->dmu_indicators))
	    {
		(void) psf_error(6351, 0, PSF_USERERR, &err_code, err_blk, 2,
			length, command,
			0, "SECURITY_AUDIT");
		return (E_DB_ERROR);
	    }
	    BTset(PSS_WC_ROW_SEC_AUDIT, dmuchar->dmu_indicators);
	    if (STcompare(value, "row") == 0)
		yyvars->secaudit = TRUE;
	    else if (STcompare(value, "norow") != 0)
	    {
		(void) psf_error(E_US07FF_2047_WITHOPT_VALUE, 0, PSF_USERERR,
			&err_code, err_blk, 3,
			length, command,
			0, value,
			0, "SECURITY_AUDIT=()");
		return (E_DB_ERROR);
	    }
	}
	break;

    case PSS_SECAUDIT_KEY_CLAUSE:
	{
	    char tempstr[DB_ATT_MAXNAME+1];
	    u_i4 ilen, olen;

	    /* Not really a list, it's just one column name. */
	    if (yyvars->secaudkey != NULL)
	    {
		(void) psf_error(6351, 0, PSF_USERERR, &err_code, err_blk, 2,
		    length, command,
		    0, "SECURITY_AUDIT_KEY");
		return (E_DB_ERROR);
	    }
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			sizeof(DB_ATT_NAME),
			&yyvars->secaudkey, err_blk);
	    if (status != E_DB_OK)
		return (status);

	    ilen = STlength(value);
	    olen = DB_ATT_MAXNAME;

	    status = cui_idxlate((u_char *) value, &ilen,
			      (u_char *)tempstr, &olen, 
			      *sess_cb->pss_dbxlate | CUI_ID_STRIP, 
			      (u_i4 *) NULL, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
	    	_VOID_ psf_error(6351, 0L, PSF_USERERR, &err_code, err_blk, 2,
			length, command,
			0, "SECURITY_AUDIT_KEY");
	       return (E_DB_ERROR);
	    }

	    MEmove(olen, (PTR)tempstr, ' ', DB_ATT_MAXNAME, 
			yyvars->secaudkey->db_att_name);
	}
	break;

    case PSS_UNSUPPORTED_CRTTBL_WITHOPT:
	/* Ignore these */
	break;

    default:
	TRdisplay("%@ psl_withlist_elem: Unknown list clause code %d, value %s\n",
		yyvars->list_clause, value);
	(void) psf_error(E_PS0002_INTERNAL_ERROR, 0, PSF_INTERR,
		&err_code, err_blk, 0);
	return (E_DB_ERROR);

    } /* switch */

    /* Some cases set status, else it's OK from before the switch */

    return (status);
} /* psl_withlist_elem */

/*
** Name: psl_withlist_keyelem -- Parse a KEY=() clause element
**
** Description:
**	KEY clause element parsing is not hard, but there are variations
**	based on statement context.  So, it's broken out into a separate
**	routine.
**
**	For the CREATE INDEX statement, the KEY list has to be an exact
**	prefix of the index column list.  It suffices to verify that
**	KEY= element N matches index column N, and then we fiddle around
**	with the DMU_CB to count the key element.  (Note that for CREATE
**	INDEX, the index attributes are in the dmu_key_attr array,
**	and the key attributes go into the dmu_attr_array array!
**	ie exactly backwards.)
**
**	For REGISTER and CTAS/DGTTAS, we allocate a PST_RESKEY
**	and hook it onto the end of the existing list, headed at
**	withkey in yyvars.  The new element goes on the end,
**	and while looking for the end, we can check for duplicates.
**	We'll also verify that the key column is a valid column.
**	Eventually, psl-ct3-crwith will hook up the key columns with
**	the create's DMU_CB.
**
** Inputs:
**	sess_cb			ptr to a PSF session CB
**	psq_cb			Query parse control block
**	dmucb			DMU_CB structure being built
**				(KEY= is not allowed in a PSQ_CONS context,
**				so we're assured that there is a DMU_CB.)
**	value			The KEY= column list element.
**
** Outputs:
**	Returns E_DB_OK or error status.
**
** History:
**	11-Oct-2010 (kschendel) SIR 124544
**	    Extract from existing key= scanners.
*/

static DB_STATUS
psl_withlist_keyelem(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, DMU_CB *dmucb,
	char *value)
{
    char	*command;
    DB_ATT_NAME	dbatt;		/* Supplied key column name, padded */
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    DB_STATUS	status;
    DMF_ATTR_ENTRY **attrs;
    i4		colno;
    i4		err_code;
    i4		length;
    i4		maxcol;
    PSS_YYVARS	*yyvars;
    PST_RESKEY	*cur, *prev;	/* Singly linked list chasers */
    PST_RESKEY	*newkey;

    yyvars = sess_cb->pss_yyvars;
    command = yyvars->qry_name;
    length = yyvars->qry_len;
    STmove(value, ' ', DB_ATT_MAXNAME, dbatt.db_att_name);

    /* Since CREATE INDEX is different from the others, do it and get
    ** out of the way.
    */
    if (psq_cb->psq_mode == PSQ_INDEX)
    {
	DMU_KEY_ENTRY	    **key;

	colno = dmucb->dmu_attr_array.ptr_in_count;

	/*
	** Number of keys must be <= number of columns.
	*/
	if ((colno + 1) > dmucb->dmu_key_array.ptr_in_count)
	{
	    (VOID) psf_error(5331L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);
	}

	key = (DMU_KEY_ENTRY **) dmucb->dmu_key_array.ptr_address;
	/*
	** Make sure the column exists in the column list in the right
	** order.
	*/
	if (MEcmp(dbatt.db_att_name, (key[colno])->key_attr_name.db_att_name,
	    DB_ATT_MAXNAME))
	{
	    (VOID) psf_error(5329L, 0L, PSF_USERERR, &err_code, err_blk, 2,
		psf_trmwhite((u_i4) sizeof(DB_TAB_NAME),
		(char *) &dmucb->dmu_index_name),
		(PTR) &dmucb->dmu_index_name,
		0, value);
	    return (E_DB_ERROR);
	}
	/* It does, so count off another key column.  The attr_array
	** pointer already points to the index columns, just adjust the
	** key count.  No, we're not adjusting the wrong one...
	*/
	dmucb->dmu_attr_array.ptr_in_count++;
	return (E_DB_OK);
    }

    /* Find end of the key list, checking dups as we go */
    prev = NULL;
    cur = yyvars->withkey;
    while (cur != NULL)
    {
	if (MEcmp(cur->pst_attname.db_att_name, dbatt.db_att_name, DB_ATT_MAXNAME) == 0)
	{
	    (void) psf_error(2025, 0, PSF_USERERR, &err_code, err_blk, 2,
		length, command, 0, value);
	    return (E_DB_ERROR);
	}
	prev = cur;
	cur = cur->pst_nxtkey;
    }

    /* Check that the column is valid. */

    attrs = (DMF_ATTR_ENTRY **) dmucb->dmu_attr_array.ptr_address;
    maxcol = dmucb->dmu_attr_array.ptr_in_count;
    for (colno = 0; colno < maxcol; colno++)
    {
	if (!MEcmp(dbatt.db_att_name,
		   (char *)&(attrs[colno])->attr_name,
		   DB_ATT_MAXNAME))
	{
	    break;    /* we have match, a valid key value */
	}
    }
    if (colno >= maxcol)
    {
	(VOID) psf_error(9312L, 0L, PSF_USERERR, &err_code,
			 err_blk, 1,
			 0, value);
	return (E_DB_ERROR);		
    }
    status = psl_check_key(sess_cb, &psq_cb->psq_error,
			    attrs[colno]->attr_type);
    if (status)
    {
	(VOID) psf_error(2179L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    0, value);
	return (E_DB_ERROR);
    }

    /* Store the key entry */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_RESKEY),
			&newkey, err_blk);
    if (status != E_DB_OK)
	return (status);
    if (prev == NULL)
	yyvars->withkey = newkey;
    else
	prev->pst_nxtkey = newkey;
    newkey->pst_nxtkey = NULL;
    MEcopy(dbatt.db_att_name, DB_ATT_MAXNAME, newkey->pst_attname.db_att_name);
    newkey->pst_descending = FALSE;
    /* Only REGISTER AS IMPORT has a key direction */
    if (psq_cb->psq_mode == PSQ_REG_IMPORT)
    {
	newkey->pst_descending = (yyvars->qry_mask & PSS_QMASK_REGKEY_DESC) != 0;
	yyvars->qry_mask &= ~PSS_QMASK_REGKEY_DESC;
    }
    ++ yyvars->withkey_count;

    return (E_DB_OK);
} /* pss_withlist_keyelem */

/*
** Name: psl_withlist_relem - store RANGE=() list element
**
** Description:
**	This routine performs the semantic actions for index_relem:
**	productions
**
**	index_relem:   	    signop I2CONST | signop I4CONST | signop DECCONST
**	                    signop F4CONST | signop F8CONST
**
**	The RANGE with-option only occurs on create index statements,
**	and is not relevant to Star.  So, we don't need to bother
**	with any Star checking.
**
** Inputs:
**	sess_cb			ptr to a PSF session CB
**	psq_cb			Query parse control block
**	element			The word which gives the with option.
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** History:
**	05-may-1995 (shero03)
**	    Created.
**	9-Oct-2010 (kschendel) SIR 124544
**	    Move to new with-handling module, tweak call sequence.
*/
DB_STATUS
psl_withlist_relem(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	f8  		*element)
{
    char	*command;
    DB_ERROR	*err_blk = &psq_cb->psq_error;
    DB_STATUS	status;
    DMU_CB	*dmu_cb;
    i4		err_code;
    i4		length;		/* of "command" */
    i4		rangeno;
    QEU_CB	*qeu_cb;

    /* There's no support for range / rtree stuff in Star.  If we get
    ** here by accident, just return rather than segv'ing.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    command = sess_cb->pss_yyvars->qry_name;
    length = sess_cb->pss_yyvars->qry_len;

    if (sess_cb->pss_yyvars->list_clause != PSS_RANGE_CLAUSE)
    {
	(void) psf_error(E_US1963_6499_X_ONLYWITH_Y, 
			0L, PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			0, "numeric option-list elements",
			0, "the RANGE=() option");
	return (E_DB_ERROR);
    }

    rangeno = dmu_cb->dmu_range_cnt;

    /*
    ** Number of ranges must be <= 2 * number of dimensions.
    */
    if ((rangeno + 1) > 2 * DB_MAXRTREE_DIM)
    {
	rangeno = 2 * DB_MAXRTREE_DIM;
	(void) psf_error(E_US14DC_5340_WITHOPT_TOOMANY, 0L, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "RANGE",
		sizeof(rangeno), &rangeno);
	return (E_DB_ERROR);
    }

    if (dmu_cb->dmu_range == NULL)
    {

	/*
	** Allocate the range entries.  Allocate enough space for
	** DB_MAXRTREE_DIM * 2, although it's probably
	** fewer.  This is because we don't know how many dimensions we
	** have at this point, and it would be a big pain to allocate less
	** and then run into problems.
	*/
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			    DB_MAXRTREE_DIM * 2 * sizeof(f8),
			    &dmu_cb->dmu_range, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	rangeno = 0;

    }

    dmu_cb->dmu_range[rangeno] = *element;
    dmu_cb->dmu_range_cnt++;

    return (E_DB_OK);
} /* psl_withlist_relem */

/*
** Name: psl_withstar_text - Output option text for Star session
**
** Description:
**	For Star DDL, with-options are passed through as-is.
**	We can get here for option=number, option, and option=(list)
**	type things.  A leading with or comma is output, then the
**	option, and if a value is given, the value.
**	LIST style options should pass "(" as the value, and
**	the list-element handler will stuff the list elements as
**	they arrive.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	option		Pointer to null-terminated option name string
**	value		Optional value string, or "(", or NULL for no value
**	xlated_qry	PSS_Q_XLATE text tracking data structure
**
** Outputs:
**	Text is appended to the query
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Extract from old with-option handlers
*/

static DB_STATUS
psl_withstar_text(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	char *option, char *value, PSS_Q_XLATE *xlated_qry)
{
    char	*str;
    DB_ERROR	*err_blk;
    DB_STATUS	status;
    i4		qmode = psq_cb->psq_mode;

    if (qmode != PSQ_CREATE && qmode != PSQ_RETINTO)
    {
	return (E_DB_OK);
    }
    err_blk = &psq_cb->psq_error;
    /* 
    ** Buffer with-clause parameter and pass (unverified) on to the LDB.
    ** LDB will report error if any.  For SELECT INTO, do not
    ** emit "with" -- all we are doing is giving OPC a with-clause
    ** to tack on to the statement it generates.
    */

    if (sess_cb->pss_distr_sflags & PSS_PRINT_WITH)
    {
	str = " with ";
	sess_cb->pss_distr_sflags &= ~PSS_PRINT_WITH;	
    }
    else
    {
	str = ", ";
    }

    /* Leadin plus option name */
    status = psq_x_add(sess_cb, str, &sess_cb->pss_ostream,
		xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		STlength(str),
		NULL, NULL, option, err_blk);
	
    if (value != NULL)
    {
	status = psq_x_add(sess_cb, value, &sess_cb->pss_ostream,
		xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		STlength(value),
		"=", " ", NULL, err_blk);
    }
    else
    {
	/* Ensure a space after a single-keyword option */
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		-1,
		NULL, NULL, " ", err_blk);
    }

    return(status);
} /* psl_withstar_text */

/*
** Name: psl_withstar_elem -- Append list option element to Star text
**
** Description:
**	If a list-style option appears as part of a Star create (or ctas)
**	statement, we want to just append the value text and pass it
**	along to Star.  The caller verifies that the query is CREATE or CTAS.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	value		List-clause element (string)
**	xlated_qry	PSS_Q_XLATE text tracking data structure
**
** Outputs:
**	Text is appended to the query
**	Returns E_DB_OK or error status
**
** History:
**	8-Oct-2010 (kschendel) SIR 124544
**	    Extract from old with-option handlers
*/

static DB_STATUS
psl_withstar_elem(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	char *value, PSS_Q_XLATE *xlated_qry)
{
    DB_ERROR	*err_blk;
    DB_STATUS	status;
    u_char	*c1 = sess_cb->pss_nxtchar;

    err_blk = &psq_cb->psq_error;

    status = psq_x_add(sess_cb, value, &sess_cb->pss_ostream,
			xlated_qry->pss_buf_size,
			&xlated_qry->pss_q_list, STlength(value),
			" ", " ", NULL, err_blk);

    if (DB_FAILURE_MACRO(status))				
	return(status);

    while (CMspace(c1))
	CMnext(c1);

    if (!CMcmpcase(c1, ","))
    {
	CMprev(c1, 0);
	if (!CMcmpcase(c1, ")"))
	{
	    status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		    -1, NULL, NULL, ") ", err_blk);
	}
	else
	{
	    status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		    -1, NULL, NULL, ", ", err_blk);
	}
    }
    else if (CMnmstart(c1)) 
    {
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		    -1, NULL, NULL, ", ", err_blk);
    }
    else if (!CMcmpcase(c1, ")") || !CMcmpcase(c1, ""))
    {
	status = psq_x_add(sess_cb, "", &sess_cb->pss_ostream,
		    xlated_qry->pss_buf_size, &xlated_qry->pss_q_list,
		    -1, NULL, NULL, ") ", err_blk);
    }

    return (status);
} /* psl_withstar_elem */

/*
** Name: psl_withopt_post -- Post-processing for the WITH-clause
**
** Description:
**
**	After all WITH-clause options (if any) are parsed, this
**	routine is called to cross-check everything and generate
**	the desired output data structures.  Most of the work has
**	been done in-place into a DMU_CHARACTERISTICS, but there
**	may be a bit more to do (setting additional flags and such).
**
**	There's a lot of code here that is somewhat unavoidably
**	statement type specific.  Modify and create-index statements
**	have an existing (base) table storage structure that can be
**	relevant.  Constraint-WITH and COPY don't have a DMU_CB.
**	Modify has to deal with the possibility that the with-option
**	(singular) is actually the modify "action", as in for
**	example: modify table to priority=3.  This isn't really a
**	WITH-clause, but the grammar uses the same productions
**	(twith_num in this case), so we need to set the modify
**	action code and return to finish parsing.
**
**	Yet another special MODIFY action deals with "modify to
**	reconstruct", which is defined to re-issue all of the existing
**	table characteristics unless overridden by the query options.
**	In order to properly check things we have to fill in those
**	existing options before checking.  Consider a hash table
**	with minpages=100, and "modify h to reconstruct with maxpages=50".
**	That's an illegal combination that we won't catch until we
**	pull in all the existing non-overridden table characteristics.
**
** Inputs:
**	sess_cb			PSS_SESBLK parser session control block
**	  ->pss_yyvars		cur-cons and related info filled in
**	psq_cb			Query parse control block
**
** Outputs:
**	Returns E_DB_OK or error status
**
** History:
**	11-Oct-2010 (kschendel) SIR 124544
**	    Centralize all the WITH-parsing stuff
**	27-Oct-2010 (kschendel)
**	    Just return if a Star session.
*/

DB_STATUS
psl_withopt_post(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb)
{

    char *command;
    char *ssname;
    char *dmu_indicators;	/* Convenience indicators pointer */
    DB_ERROR *err_blk;
    DB_STATUS status;
    DMT_TBL_ENTRY *showptr;	/* Existing table info for MODIFY, existing
				** base table info for CREATE INDEX */
    DMU_CB *dmucb;
    DMU_CHARACTERISTICS *dmuchar;
    i4 err_code;
    i4 length;
    i4 sstruct;
    PSS_YYVARS *yyvars;
    QEU_CB *qeucb;
    u_i4 qbits;			/* Query mode bits */

    /* Star sessions never use any of the real WITH-parsing stuff.
    ** A Star session just pushes text around.  Return now if Star.
    */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
	return (E_DB_OK);

    yyvars = sess_cb->pss_yyvars;
    err_blk = &psq_cb->psq_error;
    qbits = yyvars->qmode_bits;
    dmuchar = yyvars->cur_chars;
    dmu_indicators = dmuchar->dmu_indicators;
    dmucb = NULL;

    command = yyvars->qry_name;
    length = yyvars->qry_len;

    if ((qbits & (PSL_QBIT_CONS|PSL_QBIT_COPY)) == 0)
    {
	qeucb = (QEU_CB *) sess_cb->pss_object;
	dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    }

    showptr = NULL;
    if (qbits & (PSL_QBIT_INDEX | PSL_QBIT_MODANY | PSL_QBIT_COPY))
	showptr = sess_cb->pss_resrng->pss_tabdesc;

    /* Before doing anything else, see if we are doing a modify action.
    ** If so, set the DMU CB action and get out.
    */
    if (qbits & PSL_QBIT_MODNONE)
    {
	/* The two possibilities are PRIORITY=n and UNIQUE_SCOPE=xxx.
	** The way the grammar is arranged, we have to have one or the other.
	** The actual value is already set and indicated, just set the action.
	*/
	if (BTtest(DMU_TABLE_PRIORITY, dmu_indicators))
	{
	    dmucb->dmu_action = DMU_ACT_PRIORITY;
	}
	else
	{
	    /* Must be unique-scope.  Verify that the table is indeed
	    ** unique, else it's meaningless.
	    */
	    if ((showptr->tbl_status_mask & DMT_UNIQUEKEYS) == 0)
	    {
		(void) psf_error(E_PS0BB2_UNIQUE_REQUIRED, 0L, 
		    PSF_USERERR, &err_code, err_blk, 1, 0, "MODIFY");
		return (E_DB_ERROR);
	    }
	    dmucb->dmu_action = DMU_ACT_USCOPE;
	}
	/* Note that after returning, the grammar will redo the "with init"
	** to pick up the new modify action context.
	*/
	return (E_DB_OK);
    }

    /* Since many checks depend on storage structure, figure out what
    ** the storage structure is, or should be.
    */
    sstruct = 0;
    if (qbits & PSL_QBIT_MODSTOR || BTtest(DMU_STRUCTURE, dmu_indicators))
    {
	sstruct = dmuchar->dmu_struct;
	/* Special check for "modify to reconstruct", fill in any
	** existing table characteristics that aren't overridden.
	*/
	if (yyvars->md_reconstruct)
	{
	    status = psl_md_reconstruct(sess_cb, dmucb, err_blk);
	    if (status != E_DB_OK)
		return (status);
	}
    }
    else if (qbits & PSL_QBIT_COPY)
    {
	sstruct = showptr->tbl_storage_type;
    }

    /* If something that uses a storage structure, default and validate it. */
    /* Note that the gateway REGISTER statements sortof-kindof use a
    ** storage structure, but in a different way, and we won't deal with
    ** REGISTER here.
    */
    if (qbits & (PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT |
		  PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_VWINDEX |
		  PSL_QBIT_MODSTOR | PSL_QBIT_CONS) )
    {
	if (sstruct == 0)
	{
	    i4 dcompress, kcompress;

	    dcompress = kcompress = DMU_COMP_OFF;
	    if (qbits & PSL_QBIT_VWINDEX)
		sstruct = DB_X100IX_STORE;
	    else if (qbits & PSL_QBIT_INDEX)
	    {
		/* This pss_idxstruct thing is a little-known variable that is
		** DB_ISAM_STORE by default.  Apparently, it can also be
		** set via an equally unknown session modifier (-nxxx) in the
		** session connect message.  The outside world will verify
		** that pss_idxstruct is HASH, ISAM, or BTREE.
		** A negative value means "with compression".
		** FYI, there's also a -rxxx that sets the result-structure
		** at session startup.  Fascinating...
		*/
		sstruct = abs(sess_cb->pss_idxstruct);
		if (sess_cb->pss_idxstruct < 0
		  && ! BTtest(PSS_WC_COMPRESSION, dmu_indicators))
		{
		    dcompress = kcompress = DMU_COMP_DFLT;
		}
	    }
	    else if (qbits & (PSL_QBIT_CTAS | PSL_QBIT_DGTTAS))
	    {
		sstruct = sess_cb->pss_result_struct;
		/* Don't attempt to do vectorwise catalogs! */
		if (sstruct > DB_STDING_STORE_MAX
		  && MEcmp((PTR) &dmucb->dmu_owner, (PTR) sess_cb->pss_cat_owner, sizeof(DB_OWN_NAME)) == 0)
		    sstruct = DB_HEAP_STORE;
		if (sess_cb->pss_result_compression)
		{
		    dcompress = kcompress = DMU_COMP_DFLT;
		}
	    }
	    else if (qbits & (PSL_QBIT_CREATE | PSL_QBIT_DGTT))
	    {
		sstruct = sess_cb->pss_result_struct;
		/* All Ingres style structures devolve to HEAP for
		** simple create table.
		*/
		if (sstruct <= DB_STDING_STORE_MAX)
		    sstruct = DB_HEAP_STORE;
		dcompress = sess_cb->pss_create_compression;
		/* Don't mess with catalogs! */
		if (MEcmp((PTR) &dmucb->dmu_owner, (PTR) sess_cb->pss_cat_owner, sizeof(DB_OWN_NAME)) == 0)
		{
		    sstruct = DB_HEAP_STORE;
		    kcompress = dcompress = DMU_COMP_OFF;
		}
	    }
	    else if (qbits & PSL_QBIT_CONS)
	    {
		sstruct = DB_BTRE_STORE;
		/* If we extend create_compression to affect constraint
		** indexes also, here's where it would go.
		*/
	    }
	    dmuchar->dmu_struct = sstruct;
	    if (! BTtest(PSS_WC_COMPRESSION, dmu_indicators))
	    {
		dmuchar->dmu_dcompress = dcompress;
		dmuchar->dmu_kcompress = kcompress;
	    }
	} /* sstruct == 0 */

	/* Apply compression defaulting if needed */
	if (dmuchar->dmu_kcompress == DMU_COMP_DFLT)
	{
	    if (sstruct == DB_BTRE_STORE)
		dmuchar->dmu_kcompress = DMU_COMP_ON;
	    else
		dmuchar->dmu_kcompress = DMU_COMP_OFF;
	}
	if (dmuchar->dmu_dcompress == DMU_COMP_DFLT)
	{
	    /* Data compression default is ON unless btree secondary index */
	    if (sstruct > DB_STDING_STORE_MAX
	      || (sstruct == DB_BTRE_STORE
		  && ( (qbits & PSL_QBIT_INDEX)
		   || ( (qbits & PSL_QBIT_MODSTOR) && (showptr->tbl_status_mask & DMT_IDX) ))) )
		dmuchar->dmu_dcompress = DMU_COMP_OFF;
	    else
		dmuchar->dmu_dcompress = DMU_COMP_ON;
	}

	ssname = uld_struct_name(sstruct);

	/* Now do structure= related checks. */
	/* Simple CREATE / DGTT only allows HEAP or vectorwise */
	if (sstruct != DB_HEAP_STORE && sstruct <= DB_STDING_STORE_MAX
	  && qbits & (PSL_QBIT_CREATE | PSL_QBIT_DGTT))
	{
	    char msg[50];	/* structname + ' structure' */

	    STcopy(ssname, msg);
	    STcat(msg, " structure");
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			0, PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			0, msg,
			length, command);
	    return (E_DB_ERROR);
	}

	/* Key compression only allowed with btree. */
	if (sstruct != DB_BTRE_STORE
	  && BTtest(DMU_KCOMPRESSION, dmu_indicators)
	  && dmuchar->dmu_kcompress != DMU_COMP_OFF)
	{
	    (void) psf_error(E_US1963_6499_X_ONLYWITH_Y,
			0, PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			0, "KEY compression",
			0, "the BTREE structure");
	    return (E_DB_ERROR);
	}

	/* Explicit data compression not allowed with btree secondary index */
	if (BTtest(DMU_DCOMPRESSION, dmu_indicators)
	  && dmuchar->dmu_dcompress != DMU_COMP_OFF
	  && ((qbits & PSL_QBIT_INDEX)
		|| (qbits & PSL_QBIT_MODSTOR && showptr->tbl_status_mask & DMT_IDX))
	  && (sstruct == DB_BTRE_STORE || sstruct == DB_RTRE_STORE) )
	{
	    i4 local_err;
	    if (sstruct == DB_BTRE_STORE)
		local_err = E_PS0BC0_BTREE_INDEX_NO_DCOMP;
	    else
		local_err = E_PS0BCA_RTREE_INDEX_NO_COMP;
	    /* Note, DMU_CB ref here is OK, this is INDEX or MODIFY */
	    _VOID_ psf_error(local_err, 0L, PSF_USERERR,
			&err_code, err_blk, 2,
			length, command,
			psf_trmwhite(sizeof(DB_TAB_NAME), dmucb->dmu_table_name.db_tab_name),
				dmucb->dmu_table_name.db_tab_name);
	    return (E_DB_ERROR);
	}

	/* Rtree goes thru some additional validation */
	if (sstruct == DB_RTRE_STORE)
	{
	    status = psl_validate_rtree(sess_cb,
			command, length, err_blk);
	    if (status != E_DB_OK)
		return (status);
	}

	/* Heapsort is only allowed on ctas / dgttas.  (MODIFY allows it too,
	** but for modify it's already stored as HEAP.)
	** Change the visible structure to HEAP, and set a pst_restab flag
	** to tell OPF about sorting the result.
	*/
	if (sstruct == DB_SORT_STORE)
	{
	    if ((qbits & (PSL_QBIT_CTAS | PSL_QBIT_DGTTAS)) == 0)
	    {
		(void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			0, PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			0, "HEAPSORT",
			length, command);
		return (E_DB_ERROR);
	    }
	    sess_cb->pss_restab.pst_heapsort = TRUE;
	    dmuchar->dmu_struct = sstruct = DB_HEAP_STORE;
	    ssname = "heap";
	}

	/* UNIQUE not allowed on HEAP */
	if (sstruct == DB_HEAP_STORE
	  && BTtest(DMU_UNIQUE, dmu_indicators))
	{
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			0, PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			0, "UNIQUE",
			0, "the HEAP structure");
	    return (E_DB_ERROR);
	}

	/* Vectorwise structure checks... */
	if (sstruct > DB_STDING_STORE_MAX)
	{
	    /* No other WITH-options are allowed if an explicit vectorwise
	    ** structure was given.  (Allow WITH-options for an implicit
	    ** VW structure, mostly because the QA tests are chock full of
	    ** page_size and other WITH-options.  It's too much of a pain
	    ** to keep a parallel set of QA tests with the only difference
	    ** being no WITH-options on DDL.)
	    */
	    if (BTtest(DMU_STRUCTURE, dmu_indicators))
	    {
		char temp_ind[(DMU_ALLIND_LAST+BITSPERBYTE)/BITSPERBYTE];

		MEcopy(dmu_indicators, sizeof(temp_ind), temp_ind);
		BTclear(DMU_STRUCTURE, temp_ind);
		if (BTcount(temp_ind, PSS_WC_LAST))
		{
		    (void) psf_error(E_PS04F0_NOWITH_X100, 0, PSF_USERERR,
				&err_code, err_blk, 1,
				length, command);
		    return (E_DB_ERROR);
		}
	    }

	    /* PARTITION is not allowed with an (implicit) vectorwise structure.
	    ** As noted above, we'll let other options slide, but PARTITION
	    ** is not available with VW.
	    ** Go ahead and allow NOPARTITION, though.
	    */
	    if (BTtest(PSS_WC_PARTITION, dmu_indicators)
	      && dmucb->dmu_part_def != NULL && dmucb->dmu_part_def->ndims > 0)
	    {
		(void) psf_error(E_PS04F0_NOWITH_X100, 0, PSF_USERERR,
			&err_code, err_blk, 1,
			length, command);
		return (E_DB_ERROR);
	    }

	    /* VW structure not allowed in a constraint-with, or an index on
	    ** a non-VW base table.
	    */
	    if (qbits & (PSL_QBIT_CONS | PSL_QBIT_INDEX))
	    {
		(void) psf_error(E_US1947_6465_X_NOTWITH_Y,
			0, PSF_USERERR, &err_code, err_blk, 3,
			length, command,
			0, ssname,
			length, command);
		return (E_DB_ERROR);
	    }

	    /* Make all of the WITH-options go away except for structure=.
	    ** This is to ignore options on an implicit vectorwise struct.
	    */
	    MEfill(sizeof(dmuchar->dmu_indicators), 0, dmu_indicators);
	    BTset(DMU_STRUCTURE, dmu_indicators);
	} /* vectorwise checks */
    } /* Storage-structure checks */

    /* Heap, vectorwise not allowed with KEY=.
    ** This test done outside of the main structure tests because
    ** REGISTER does want this check.
    */
    if (BTtest(PSS_WC_KEY, dmu_indicators)
      && ((sstruct == DB_HEAP_STORE && ! sess_cb->pss_restab.pst_heapsort)
	    || sstruct == DB_X100_STORE || sstruct == DB_X100RW_STORE) )
    {
	ssname = uld_struct_name(sstruct);
	(void) psf_error(E_US1947_6465_X_NOTWITH_Y,
		    0, PSF_USERERR, &err_code, err_blk, 3,
		    length, command,
		    0, "KEY= option",
		    0, ssname);
	return (E_DB_ERROR);
    }

    /* MODIFY: if index or table supports a UNIQUE constraint, it must be
    ** specified with PERSISTENCE, and UNIQUE_SCOPE=statement.
    */
    if ((qbits & PSL_QBIT_MODSTOR)
      && (showptr->tbl_2_status_mask & DMT_SUPPORTS_CONSTRAINT) )
    {
	u_i4 local_err = 0;

	if (showptr->tbl_2_status_mask & DMT_NOT_UNIQUE)
        {
	   if ( !(BTtest(DMU_PERSISTS_OVER_MODIFIES, dmu_indicators)))
	   {
	      local_err = E_PS0BB6_PERSISTENT_CONSTRAINT;
           }
        }
	else
	{
	   /*
	   ** You must specify WITH options UNIQUE, UNIQUE_SCOPE and PERSISTENCE
	   ** for an index that supports a unique constraint
	   */
	   if (!BTtest(DMU_UNIQUE, dmu_indicators) ||
		!BTtest(DMU_STATEMENT_LEVEL_UNIQUE, dmu_indicators) ||
		!BTtest(DMU_PERSISTS_OVER_MODIFIES, dmu_indicators))
	   {
	      local_err = E_PS0BB4_CANT_CHANGE_UNIQUE_CONS;
           }
        }

	if (local_err)
	{
	    char	*tabname, *typename;

	    tabname = (char *)  dmucb->dmu_table_name.db_tab_name;

	    if (showptr->tbl_status_mask & DMT_IDX)
		typename = "index";
	    else
		typename = "table";

	    (void) psf_error(local_err, 0L, PSF_USERERR,
			     &err_code, err_blk, 3,
			     length, command,
			     5, typename,
			     psf_trmwhite(DB_TAB_MAXNAME, tabname), tabname);
	    return (E_DB_ERROR);
	}
    }

    /* UNIQUE_SCOPE requires UNIQUE */
    if (BTtest(DMU_STATEMENT_LEVEL_UNIQUE, dmu_indicators)
      && !BTtest(DMU_UNIQUE, dmu_indicators))
    {
	(void) psf_error(E_PS0BB2_UNIQUE_REQUIRED, 0L, PSF_USERERR,
		     &err_code, err_blk, 1,
		     length, command);
	return (E_DB_ERROR);
    }

    /* Do some encryption related checks:
    ** encryption= requires passphrase=, and new_passphrase is not allowed
    ** if the (old) passphrase= is empty.
    */
    if (BTtest(PSS_WC_ENCRYPT, dmu_indicators)
      && ! BTtest(PSS_WC_PASSPHRASE, dmu_indicators))
    {
	(void) psf_error(E_PS0C88_ENCRYPT_NOPASS, 0L, PSF_USERERR,
		&err_code, err_blk, 1,
		psf_trmwhite(DB_TAB_MAXNAME,
			dmucb->dmu_table_name.db_tab_name),
		dmucb->dmu_table_name.db_tab_name);
	return (E_DB_ERROR);
    }
    if (BTtest(PSS_WC_NPASSPHRASE, dmu_indicators)
      && dmucb->dmu_enc_flags2 & DMU_NULLPASS)
    {
	(void) psf_error(9404L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	return (E_DB_ERROR);
    }

    /* Numeric-value checks */
    /* FILLFACTOR not allowed with HEAP */
    if (BTtest(DMU_DATAFILL, dmu_indicators) && sstruct == DB_HEAP_STORE)
    {
	(void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "FILLFACTOR",
		0, "the HEAP structure");
	return (E_DB_ERROR);
    }
    /* [NON]LEAFFILL only allowed with BTREE */
    if (sstruct != DB_BTRE_STORE
      && (BTtest(DMU_IFILL, dmu_indicators)
	  || BTtest(DMU_LEAFFILL, dmu_indicators) ) )
    {
	char *str = "LEAFFILL";

	if (!BTtest(DMU_LEAFFILL, dmu_indicators))
	    str = "NONLEAFFILL";
	(void) psf_error(E_US1963_6499_X_ONLYWITH_Y, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, str,
		0, "the BTREE structure");
	return (E_DB_ERROR);
    }
    /* minpages/maxpages only with HASH */
    if (sstruct != DB_HASH_STORE
      && (BTtest(DMU_MINPAGES, dmu_indicators)
	  || BTtest(DMU_MAXPAGES, dmu_indicators) ) )
    {
	char *str = "MINPAGES";

	if (!BTtest(DMU_MINPAGES, dmu_indicators))
	    str = "MAXPAGES";
	(void) psf_error(E_US1963_6499_X_ONLYWITH_Y, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, str,
		0, "the HASH structure");
	return (E_DB_ERROR);
    }
    /* Minpages <= maxpages */
    if (BTtest(DMU_MINPAGES, dmu_indicators)
      && BTtest(DMU_MAXPAGES, dmu_indicators)
      && dmuchar->dmu_minpgs > dmuchar->dmu_maxpgs)
    {
	(void) psf_error(5338L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	    length, command, sizeof(i4), &dmuchar->dmu_minpgs);
	return (E_DB_ERROR);
    }
    /* There had been a test prohibiting ALLOCATION with a heap COPY,
    ** but I'm removing it (kschendel) -- DMF may in fact do a recreate
    ** load on a heap, and if it doesn't, it just ignores the allocation.
    */
    /* ALLOCATION divided by number of partitions has to be reasonable */
    if (BTtest(DMU_ALLOCATION, dmu_indicators))
    {
	i4 nparts;
	u_i4 allocation;

	nparts = 1;
	if (qbits & (PSL_QBIT_CTAS | PSL_QBIT_DGTTAS | PSL_QBIT_MODSTOR))
	{
	    /* If there's a part-def, it's the NEW number of partitions.
	    ** If there's no part-def, either it's nopartition, or a
	    ** modify to reconstruct that isn't changing the partitioning;
	    ** for the latter, take the current number of partitions.
	    */
	    if (dmucb->dmu_part_def != NULL)
		nparts = dmucb->dmu_part_def->nphys_parts;
	    else if (qbits & PSL_QBIT_MODSTOR && yyvars->md_reconstruct)
		nparts = showptr->tbl_nparts;
	    if (nparts == 0)
		nparts = 1;
	}
	/* The idea here is that a whole-table allocation below 4 is
	** an error.  Above 4, we divide by # of partitions, and it's OK
	** to divide the entire allocation away ... DMF will do the
	** right thing, and at least the user had some clue.
	*/
	allocation = (u_i4) dmuchar->dmu_alloc;
	if (allocation > 4 * nparts)
	    allocation = allocation / nparts;
	if (allocation < 4 || allocation > DB_MAX_PAGENO)
	{
	    i4 lo = 4, hi = DB_MAX_PAGENO;
	    (void) psf_error(5341, 0, PSF_USERERR, &err_code, err_blk, 5,
				length, command, 0, "ALLOCATION",
				sizeof(i4), &allocation,
				sizeof(i4), &lo, sizeof(i4), &hi);
	    return (E_DB_ERROR);
	}
    }

    /* DIMENSION only with RTREE */
    if (BTtest(DMU_DIMENSION, dmu_indicators) && sstruct != DB_RTRE_STORE)
    {
	(void) psf_error(E_US1963_6499_X_ONLYWITH_Y, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "DIMENSION",
		0, "the RTREE structure");
	return (E_DB_ERROR);
    }

    /* DGTT requires the NORECOVERY option unless it's vectorwise. */
    if (qbits & (PSL_QBIT_DGTT | PSL_QBIT_DGTTAS)
      && sstruct <= DB_STDING_STORE_MAX
      && (!BTtest(DMU_RECOVERY, dmu_indicators)
	  || dmuchar->dmu_flags & DMU_FLAG_RECOVERY) )
    {
	(VOID) psf_error(E_PS0BD0_MUST_USE_NORECOV, 0L, PSF_USERERR,
		&err_code, err_blk, 1,
		length, command);
	return (E_DB_ERROR);
    }

    /* AUTOSTRUCT and STRUCTURE= are mutually exclusive. */
    if (BTtest(DMU_AUTOSTRUCT, dmu_indicators)
      && BTtest(DMU_STRUCTURE, dmu_indicators))
    {
	(void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "AUTOSTRUCT",
		0, "STRUCTURE=");
	return (E_DB_ERROR);
    }

    /* Partitioning is (at present) not allowed with temp tables.
    ** The scanner doesn't allow PARTITION= with dgtt/dgttas, but it
    ** can still slip through if we're doing a MODIFY.
    ** Similarly with partitioning when modifying a secondary index.
    */
    if (BTtest(PSS_WC_PARTITION, dmu_indicators)
      && (qbits & PSL_QBIT_MODSTOR)
      && dmucb->dmu_part_def != NULL && dmucb->dmu_part_def->nphys_parts > 0
      && (showptr->tbl_temporary || showptr->tbl_status_mask & DMT_IDX) )
    {
	char *str = "a secondary index";
	if (showptr->tbl_temporary)
	    str = "a temporary table";
	(void) psf_error(E_US1932_6450_PARTITION_NOTALLOWED,
		0L, PSF_USERERR, &err_code, err_blk,
		2, length, command, 0, str);
	return (E_DB_ERROR);
    }

    /* modify .. with persistence is only allowed with secondary indexes */
    if (BTtest(DMU_PERSISTS_OVER_MODIFIES, dmu_indicators)
      && (qbits & PSL_QBIT_MODSTOR)
      && dmuchar->dmu_flags & DMU_FLAG_PERSISTENCE
      && (showptr->tbl_status_mask & DMT_IDX) == 0)
    {
	(void) psf_error(E_US1963_6499_X_ONLYWITH_Y, 0, PSF_USERERR,
		&err_code, err_blk, 3,
		length, command,
		0, "PERSISTENCE",
		0, "a secondary index");
	return (E_DB_ERROR);
    }

    /* Done with post-checks */

    /* Extensions-only option deals in flag hacking, not dmu chars.
    ** Normal is EXTTOO.  Extensions-only sets extonly.
    ** Both extonly and noext clear exttoo.
    */
    if (BTtest(PSS_WC_EXTONLY, dmu_indicators))
    {
	if (yyvars->extonly)
	    dmucb->dmu_flags_mask ^= (DMU_EXTONLY_MASK | DMU_EXTTOO_MASK);
	else
	    dmucb->dmu_flags_mask &= ~(DMU_EXTONLY_MASK | DMU_EXTTOO_MASK);
    }

    /* NODEPENDENCY_CHECK sets a bit in the DMU flags mask. */
    if (BTtest(PSS_WC_NODEP_CHECK, dmu_indicators))
	dmucb->dmu_flags_mask |= DMU_NODEPENDENCY_CHECK;

    /* NOJOURNAL_CHECK sets a constraint flag */
    if (BTtest(PSS_WC_NOJNL_CHECK, dmu_indicators))
	sess_cb->pss_curcons->pss_cons_flags |= PSS_NO_JOURNAL_CHECK;

    /* QUEL parsing apparently needs to know whether an explicit
    ** [NO]DUPLICATES was given.
    */
    if (BTtest(DMU_DUPLICATES, dmu_indicators))
    {
	if (dmucb->dmu_chars.dmu_flags & DMU_FLAG_DUPS)
	    yyvars->with_dups = PST_ALLDUPS;
	else
	    yyvars->with_dups = PST_NODUPS;
    }

    /* For statements that require a storage structure, make sure that
    ** the STRUCTURE and COMPRESSION indicators are turned on.
    ** We may have defaulted structure and compression above.  The
    ** indicators were left untouched until now, so that post-checking
    ** could distinguish implicit from explicit storage structure,
    ** but the checking is done and we need to make sure DMF notices
    ** the final choices.
    ** MODIFY normally has the indicator set, but reconstruct doesn't.
    ** so go ahead and let structure modify thru here too.
    ** Don't need to do this for constraint-WITH, storage structures
    ** are handled by opf / qef.
    */
    if (qbits & (PSL_QBIT_CREATE | PSL_QBIT_CTAS | PSL_QBIT_DGTT |
		 PSL_QBIT_DGTTAS | PSL_QBIT_INDEX | PSL_QBIT_VWINDEX |
		 PSL_QBIT_MODSTOR) )
    {
	BTset(DMU_STRUCTURE, dmuchar->dmu_indicators);
	if (dmuchar->dmu_dcompress != DMU_COMP_OFF)
	    BTset(DMU_DCOMPRESSION, dmuchar->dmu_indicators);
	if (dmuchar->dmu_kcompress != DMU_COMP_OFF)
	    BTset(DMU_KCOMPRESSION, dmuchar->dmu_indicators);
    }

    /* Looks good! */
    return (E_DB_OK);

} /* psl_withopt_post */

/*
** Name: psl_validate_rtree - verify that the specified combination of options
**				is legal for an RTREE structure
**
** Description:
**	Examine options associated with an RTREE structure to
**	make sure they are valid.  At present, RTREE is only
**	available from a CREATE INDEX statement.
**
** Inputs:
**	sess_cb			PSF session CB
**	qry			The name of the statement being parsed
**	qry_len			Length of qry
**
** Output:
**	err_blk			filled in if an illegal combnination of options
**				is discovered
**
** Side effects:
**	none
**
** Returns:
**	E_DB_{OK,ERROR}
**
** History
**
**	28-jun-1996 (shero03)
**	    written
**	16-apr-1997 (shero03)
**	    Use adi_dt_rtree to verify that all the proper functions exist
**	26-sep-1997 (shero03)
**	    B85904: Validate the range with the data type.  
**	29-Jan-2004 (schka24)
**	    No partitioned RTREE's.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Minor changes for centralized with-options.
*/
static DB_STATUS
psl_validate_rtree(
	PSS_SESBLK	*sess_cb,
	char		*qry,
	i4		qry_len,
	DB_ERROR	*err_blk)
{
    QEU_CB	    	*qeu_cb;
    DMU_CB	    	*dmu_cb;
    char	    	*tabname;
    DMT_ATT_ENTRY   	*attr;
    DMU_KEY_ENTRY	**key;
    ADI_DT_RTREE	rtree_blk;
    i4		err_code;
    i4			i, j;
    DB_STATUS		status;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    tabname = (char *) &dmu_cb->dmu_table_name;
	
    if ( (sess_cb->pss_rsdmno == 0) ||
         (sess_cb->pss_rsdmno > 1) )
    {
       (VOID) psf_error(E_PS0BCC_RTREE_INDEX_ONE_KEY,
		0L, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_table_name),
		&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }

    /* Check for PARTITION, but allow NOPARTITION. */
    if (BTtest(PSS_WC_PARTITION, dmu_cb->dmu_chars.dmu_indicators)
      && dmu_cb->dmu_part_def != NULL)
    {
	(void) psf_error(E_US1932_6450_PARTITION_NOTALLOWED,
		0, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry, 0, "the RTREE structure");
	return (E_DB_ERROR);
    }
	
    if (BTtest(DMU_UNIQUE, dmu_cb->dmu_chars.dmu_indicators))
    {
       (VOID) psf_error(E_PS0BCB_RTREE_INDEX_NO_UNIQUE,
		0L, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_table_name),
		&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }
	
    if (!(BTtest(PSS_WC_RANGE, dmu_cb->dmu_chars.dmu_indicators)))
    {
       (VOID) psf_error(E_PS0BCD_RTREE_INDEX_NEED_RANGE,
		0L, PSF_USERERR, &err_code, err_blk, 2,
		qry_len, qry,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &dmu_cb->dmu_table_name),
		&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }
	
    /* Ensure that the key's data type has all appropiate functions */
    /* First, find the key's attribute by locating its name */
    key = (DMU_KEY_ENTRY **)dmu_cb->dmu_key_array.ptr_address;
    attr = pst_coldesc(sess_cb->pss_resrng,
	    (*key)->key_attr_name.db_att_name, DB_ATT_MAXNAME);
    status = adi_dt_rtree(sess_cb->pss_adfcb, attr->att_type, &rtree_blk);
    if (DB_FAILURE_MACRO(status))
    {
       	(VOID) psf_error(E_PS0BCE_RTREE_INDEX_BAD_KEY,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &dmu_cb->dmu_table_name),
			&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }

    if (dmu_cb->dmu_range_cnt != rtree_blk.adi_dimension * 2)
    {
       	(VOID) psf_error(E_PS0BCF_RTREE_INDEX_BAD_RANGE,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &dmu_cb->dmu_table_name),
			&dmu_cb->dmu_table_name);
       return (E_DB_ERROR);
    }

    for (i = 0, j = rtree_blk.adi_dimension; i < rtree_blk.adi_dimension; i++, j++)
    {
    	if (dmu_cb->dmu_range[i] >= dmu_cb->dmu_range[j])
        {
       	   (VOID) psf_error(E_PS0BCF_RTREE_INDEX_BAD_RANGE,
			0L, PSF_USERERR, &err_code, err_blk, 2,
			qry_len, qry,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &dmu_cb->dmu_table_name),
			&dmu_cb->dmu_table_name);
          return (E_DB_ERROR);
        }
    }

    return (E_DB_OK);

}  /* end psl_validate_rtree */
