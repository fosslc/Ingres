/*
**Copyright (c) 2010 Ingres Corporation
*/

/**
** Name: DUUCATDEF.H - Definitions used in define System Catalog structures.
**
** Description:
**        This header contains definitions used to define the System
**	Catalog structures for the database utility 'sysmod'.
**
** History:
**      04-Jul-88 (ericj)
**          Initially commented.
**      04-Jul-88 (ericj)
**          Updated to support 'iiprocedure'.
**	29-Nov-88 (jrb)
**	    Changed DU_5MAXSCI_CATDEFS from 3 to 1.
**      07-mar-89 (neil)
**          Rules: Changed DU_1MAXDBMS_CATDEFS from 9 to 10.
**	12-May-89 (teg)
**	    incremented DU_0MAXCORE_CATDEFS to include iidevices,
**	    redefined DU_4MAXDDB_CATDEFS to 23 for TITAN.
**	17-May-89 (teg)
**	    merged titan/terminator lines
**	17-May-89 (ralph)
**	    GRANT Enhancements:
**	    bump DU_3MAXIIDBDB_CATDEFS from 5 to 8.
**      03-jan-90 (neil)
**          Alerters: Increased DU_1MAXDBMS_CATDEFS from 10 to 11.
**	9-may-90 (pete)
**	    Null out the Dui_21fecat_defs[] array. This will be handled
**	    by the new '-f' flag. Change DU_2MAXFE_CATDEFS from 14 to 0.
**	12-may-90 (pete)
**	    Remove: dui_fecats_def(), Dui_21fecat_defs[], DU_2MAXFE_CATDEFS
**	    and DU_FE_CATS. Renumber
**	    the other DU_.._CATS symbols and decrement DU_MAXCAT_DEFS to 5.
**	    Earlier (see 9-may history) FE cat stuff was just NULLed out rather
**	    than removed.
**	14-nov-91 (teresa)
**	    fix bug where someone added dbms catalogs to cmds array without
**	    incrementing DU_1MAXDBMS_CATDEFS.  As long as I'm here, fix 
**	    Dub_31iidbdbcat_defs as well.
**	31-may-92 (andre)
**	    increment DU_1MAXDBMS_CATDEFS to 14 to account for the new catalog -
**	    IIPRIV
**    3-dec-92 (jpk)
**        raise DU_1MAXDBMS_CATDEFS from 14 to 16 to support
**        gw06_attribute and gw06_relation
**    8-feb-93 (jpk)
**        raise DU_1MAXDBMS_CATDEFS from 16 to 21 to support
**        new system catalogs.
**    25-aug-93 (robf)
**	  raise DU_1MAXDBMS_CATDEFS from 21 to 23 to support Secure 2.0
**	  system catalogs
**        raise DU_3MAXIIDBDB_CATDEFS from 7 to 8 
**	9-dec-93 (robf)
**        raise DU_1MAXDBMS_CATDEFS from 23 to 26 for Secure 2.0
**	7-mar-94 (robf)
**	  raise DU_3MAXIIDBDB_CATDEFS from 9 to 10 for Secure 2.0 to
**	  support iirolegrant
**      2-nov-01 (stial01)
**        Increased DU_1MAXDBMS_CATDEFS
**	22-Jan-2003 (jenjo02)
**	  Increased DU_1MAXDBMS_CATDEFS to 29 for iisequence.
**	31-Dec-2003 (schka24)
**	    Increased MAXDBMS_CATDEFS to 34 for partition catalogs
**	29-Jan-2004 (schka24)
**	    Back the dratted thing off by one, not using iipartition.
**	    Actually we could just leave it alone but I don't want to
**	    confuse anyone.
**	29-Dec-2004 (schka24)
**	    Since iidbdb cats are still hard-coded, reduce dbdb catdefs
**	    by 1 (removed B1 iilabelaudit).
**	19-Apr-2007 (bonro01)
**	   Increase size of DU_1MAXDBMS_CATDEFS because of additional 
**	   command entries added by change 485383 for bug 117114.
**     24-oct-2008 (stial01)
**          Modularize create objects for increased ingres names (SIR 121123)
**     16-nov-2008 (stial01)
**          Added DUC_CATDEF size to verify catalog size = structure size
**     07-apr-2008 (stial01)
**          Added columns to DUC_CATDEF (for upgradedb)
**     10-Feb-2010 (maspa05) b122651
**          Added DUU_MANDFLAGS - a structure for a list of relstat,relstat2 
**          flags that must be set. The list itself dub_mand_relflags is 
**          defined in dubdata.c
**          Added DUC_CATEQV - a structure for a list of 'equivalent' catalogs
**          for the purposes of modify. See ducdata.c for list Duc_equivcats
**	30-Apr-2010 (kschendel)
**	    Added definitions for an iidbcapabilities table.
**	    Defined separate row-width and struct-width fields in the catalog
**	    definitions table.
**/
/*
[@forward_type_references@]
*/

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN VOID    dub_corecats_def();	    /* Setup a lookup table for the
					    ** core system catalog definitions.
					    */
FUNC_EXTERN STATUS	dub_dbmscats_def(); /* Setup a lookup table for the
					    ** dbms system catalog definitions.
					    */
FUNC_EXTERN VOID    dub_iidbdbcats_def();   /* Setup a lookup table for iidbdb
					    ** catalog definitions.
					    */

/*
**  Defines of other constants.
*/

/*
**      Define catalog grouping types.
*/
#define                 DU_CORE_CATS    0
#define			DU_DBMS_CATS	1
#define			DU_IIDBDB_CATS	2
#define			DU_DDB_CATS	3
#define			DU_SCI_CATS	4

#define			DU_MAXCAT_DEFS	5
/*
[@defined_constants@]...
*/


#define	    	DU_0MAXCORE_CATDEFS	4   /* # elements in Dub_01corecat_defs array */
#define		DU_1MAXDBMS_CATDEFS	34  /* # elements in Dub_11dbmscat_defs array */
#define		DU_3MAXIIDBDB_CATDEFS	9   /* # elements in Dub_31iidbdbcat_defs array */
#define		DU_4MAXDDB_CATDEFS	23  /* # elements in Dub_41ddbcat_defs array */
#define		DU_5MAXSCI_CATDEFS	1   

/*
[@group_of_defined_constants@]...
/*
[@global_variable_references@]
*/

/*}
** Name: DUU_CATDEF - System catalog description for database utilities.
**
** Description:
**	DUU_CATDEF defines a modify and/or index commands for the
**	system catalogs, for sysmod.  The table data is in dubdata.c.
**
**	DUC_CATDEF sounds similar, but it's very different;  it
**	contains the actual table create definition as well as the
**	modify / create index statements.  It's used by createdb and
**	upgradedb, and the data is in ducdata.c.
**
**	The two could probably be combined, which sounds like a useful
**	project for someone.
**
** History:
[@history_template@]...
*/
typedef struct _DUU_CATDEF
{
    char	    *du_relname;	/* Catalog name in commands array. */
    char	    *du_modify;		/* Catalog's modify command. */
    char	    **du_index;		/* secondary index commands. */
    i4		    du_index_cnt;	/* Count of number of index commands
					** on this catalog.
					*/
    i4		    du_requested;	/* Has this catalog been requested to
					** be sysmoded?
					*/
    i4		    du_exists;		/* Does this catalog exist in the
					** database?
					*/
}   DUU_CATDEF;


typedef struct _DUC_CATDEF
{
    i4 rowsize;			/* Table row size.  This must be
					** the exact row size.
					*/
    i4 structsize;		/* Equivalent structure size, for double-
				** checking.  Zero means don't check.
				** A "no check" might mean that the structure
				** is inaccessible to duc (e.g. it's in a
				** private header, say in dmf), or it might
				** mean that the structure is known to have
				** a different size for a valid reason such
				** as end-padding.
				*/
    char *catname;
    i4   columns; /* number of columns */
    char *create;	/* Create table statement, NULL means do via quel */
    char *modify;	/* Modify table statement, NULL means none */
    char *index1;	/* Create index #1, NULL means none */
    char *index2;	/* Create index #2, NULL means none */
} DUC_CATDEF;

typedef struct _DUC_PROCDEF
{
    char *procname;
    char *procdef;
} DUC_PROCDEF;

GLOBALREF DUU_CATDEF	Dub_01corecat_defs[];
GLOBALREF DUU_CATDEF	Dub_11dbmscat_defs[];
GLOBALREF DUU_CATDEF	Dub_31iidbdbcat_defs[];
GLOBALREF DUU_CATDEF	Dub_41ddbcat_defs[];
GLOBALREF DUU_CATDEF	Dui_51scicat_defs[];

/*
** Name: DUC_CATEQV - 'equivalent' catalogs for the purposes of modify
**
** Description:
**     A list of pairs of catalog and equivalent catalog where you can modify
**     the equivalent to indirectly modify the intended e.g. modify iirelation
**     to modify iirel_idx
**
** History:
**     10-Feb-2010 (maspa05)
**          Created.
**
*/
typedef struct _DUC_CATEQV
{
	char *catname;
	char *equiv_catname;
} DUC_CATEQV;

GLOBALREF DUC_CATEQV Duc_equivcats[];


/*
** Name: DUC_DBCAPS - List of "constant" iidbcapabilities entries.
**
** Description:
**	This structure describes a list of iidbcapabilities entries
**	which are constant in the sense that they are either hardcoded
**	literals or system #define constants.
**
**	Also defined is a (somewhat goofy looking) macro for quoting
**	numeric #defines.
**
**	Usage examples:
**	   ... {DU_DB1_QUEL_LEVEL, DU_DB1_CUR_QUEL_LEVEL}, ...
**
**	The #defines used above are defined as strings (e.g. "QUEL_LEVEL"),
**	so they get used as-is.
**
**	   ... DUC_DBCAP("SQL_MAX_TABLE_NAME_LEN", DB_TAB_MAXNAME), ...
**
**	Here, DB_TAB_MAXNAME is a number (256), so the DUC_DBCAP macro
**	is needing to string-ize things.
**
** History:
**	27-Apr-2010 (kschendel) SIR 123639
**	    Put the hammer down on iidbcapabilities once and for all.  I hope.
*/

typedef struct _DUC_DBCAPS
{
    char	duc_capability[32+1];	/* Catalog is currently hardwired to 32 */
    char	duc_value[32+1];
} DUC_DBCAPS;

GLOBALREF DUC_DBCAPS Duc_dbcaps[];
GLOBALREF i4 Duc_num_dbcaps;

/* This stuff is all ansi C and even the Microsoft compiler had
** better implement it!  The extra level of macro expansion serves
** to expand (say) DB_TAB_MAXNAME into 256.
*/
#define DUC_DBCAP_STR(x) #x
#define DUC_DBCAP(c,v) {c, DUC_DBCAP_STR(v)}

