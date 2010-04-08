/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <duucatdef.h>

/*{
** Name: dubdbmsmod.c -     Build the tables used by sysmod for dbms catalogs
**
** Description:
**      Sysmod is a table driven utility to modify system catalogs.  There is
**	a table to describe core catalogs (dub_00corecat_commands, 
**	Dub_01corecat_defs), dbms catalogs (Dub_10dbmscat_commands,
**	Dub_11dbmscat_defs), database database catalogs 
**	(Dub_30iidbdbcat_commands, Dub_31iidbdbcat_defs) and distributed (CDB) 
**	catalogs (Dub_40ddbcat_commands, Dub_41ddbcat_defs).
**
**	The first table for each group of catalogs is a structure containing
**	1) name of catalog and 2) ascii text of modify command for that 
**	catalog. 3) ascii text to create a secondary index (if appropriate).
**
**	The second table for each group of catalogs is populated from the
**	first table and contains:
**	    1) name of the catalog
**	    2) modify text string for the catalog
**	    3) text string to create a secondary index for this catalog
**	    4) count of number of secondary indexes for this catalog
**	    5) requested by name flag (initialized to false, but sysmod may
**		set flag, depending on input command line.
**	    6) exists flag. (always initialized to false).
**
** Inputs:
**	NONE
** Outputs:
**      None, but the following global variables are initialized:
**	    Dub_01corecat_defs, Dub_11dbmscat_defs, Dub_31iidbdbcat_defs and
**	    Dub_41ddbcat_defs
**      Returns:
**          NONE.
**      Exceptions:
**          None.
**
** Side Effects:
**    None.
**
** History:
**	    initially created.
**	15-jun-90 (teresa)
**	    added description section and added iidbms_comment and iisynonym.
**	31-may-92 (andre)
**	    added IIPRIV
**	31-may-92 (andre)
**	    added i_qid to IIDBDEPENDS' primary index
**	10-jun-92 (andre)
**	    added IIXPRIV
**	18-jun-92 (andre)
**	    added IIXPROCEDURE and IIXEVENT
**	01-oct-92 (robf)
**	    added IIGW06_RELATION and IIGW06_ATTRIBUTE for C2-Security Audit
**	08-feb-93 (jpk)
**	    added IIKEY, IIDEFAULT, IIPROCEDURE_PARAMETER, IISCHEMA,
**	    and IISECTYPE
**	22-mar-1993 (jpk)
**	    update to modify changed column names for iikey
**	    and iiprocedure_parameter.
**	27-apr-93 (anitap)
**	    iischema will keyed only on schema_name. also, index will be hash.
**	15-jun-93 (rickh)
**	    Added iixprotext, iiintegrityidx, and iiruleidx as indices
**	    which must be recreated at sysmod time.
**	08-jul-93 (shailaja)
**	    Fixed compiler warnings on trigraph sequences.
**	15-oct-93 (andre)
**	    Added text of query to create IIXSYNONYM to Dub_10dbmscat_commands[]
**      28-sep-96 (mcgem01)
**              Global data moved to dubdata.c
**	19-dec-96 (chech02)
**	    Removed READONLY attrib from GLOBALREFs.
**	20-may-1999 (nanpr01)
**	    Added new index for iirule.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      02-nov-2001 (stial01)
**          Added modify statements for iiprivlist, iirange, iiextended_relation
**      02-nov-2001 (stial01)
**          Added modify statement for iidbcapabilities
**	22-Jan-2003 (jenjo02)
**	    Added modify statement for iisequence
**	29-Dec-2004 (schka24)
**	    Remove B1 iidbdb catalogs.
*/

GLOBALREF   char	*Dub_00corecat_commands[];

GLOBALREF   DUU_CATDEF  Dub_01corecat_defs[DU_0MAXCORE_CATDEFS+1];


/*
**  DBMS CATALOG DEFINITIONS
**
**	History:
**
**	    I wouldn't claim credit for having written this either.
**
**	15-jun-93 (rickh)
**	    Added iixprotext, iiintegrityidx, and iiruleidx as indices
**	    which must be recreated at sysmod time.
**	25-aug-93 (robf)
**	    Added iilabelmap[idx], iilabelaudit for ES 2.0
**	15-oct-1993 (jpk)
**	    change keying; have text_sequence as
**	    part of unique key to allow comments
**	    longer than one line.
**	7-mar-94 (robf)
**          Added iirolegrant. 
**	21-mar-94 (teresa)
**	    fix bug 59075 -- change modify statement for iiprocedure from
**	    btree to btree unique.  (By the way rick, jrb **John Black** is
**	    the one who wrote this...)
*/

GLOBALREF   char	    *Dub_10dbmscat_commands[];

GLOBALREF   DUU_CATDEF	Dub_11dbmscat_defs[DU_1MAXDBMS_CATDEFS+1];

GLOBALREF   char	    *Dub_30iidbdbcat_commands[];

GLOBALREF   DUU_CATDEF	Dub_31iidbdbcat_defs[DU_3MAXIIDBDB_CATDEFS+1];


/* Distributed Catalog Definitions */

GLOBALREF   char	*Dub_40ddbcat_commands[];

GLOBALREF   DUU_CATDEF  Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS+1];


VOID
dub_corecats_def()
{
    /* iiattribute */
    Dub_01corecat_defs[0].du_relname	    = Dub_00corecat_commands[0];
    Dub_01corecat_defs[0].du_modify	    = Dub_00corecat_commands[1];
    Dub_01corecat_defs[0].du_index	    = NULL;
    Dub_01corecat_defs[0].du_index_cnt	    = 0;
    Dub_01corecat_defs[0].du_requested	    = FALSE;
    Dub_01corecat_defs[0].du_exists	    = FALSE;

    /* iidevices */
    Dub_01corecat_defs[1].du_relname	    = Dub_00corecat_commands[2];
    Dub_01corecat_defs[1].du_modify	    = Dub_00corecat_commands[3];
    Dub_01corecat_defs[1].du_index	    = NULL;
    Dub_01corecat_defs[1].du_index_cnt	    = 0;
    Dub_01corecat_defs[1].du_requested	    = FALSE;
    Dub_01corecat_defs[1].du_exists	    = FALSE;

    /* iiindex */
    Dub_01corecat_defs[2].du_relname	    = Dub_00corecat_commands[4];
    Dub_01corecat_defs[2].du_modify	    = Dub_00corecat_commands[5];
    Dub_01corecat_defs[2].du_index	    = NULL;
    Dub_01corecat_defs[2].du_index_cnt	    = 0;
    Dub_01corecat_defs[2].du_requested	    = FALSE;
    Dub_01corecat_defs[2].du_exists	    = FALSE;

    /* iirelation */
    Dub_01corecat_defs[3].du_relname	    = Dub_00corecat_commands[6];
    Dub_01corecat_defs[3].du_modify	    = Dub_00corecat_commands[7];
    Dub_01corecat_defs[3].du_index	    = NULL;
    Dub_01corecat_defs[3].du_index_cnt	    = 0;
    Dub_01corecat_defs[3].du_requested	    = FALSE;
    Dub_01corecat_defs[3].du_exists	    = FALSE;

    Dub_01corecat_defs[4].du_relname	    = NULL;
    Dub_01corecat_defs[4].du_modify	    = NULL;
    Dub_01corecat_defs[4].du_index	    = NULL;
    Dub_01corecat_defs[4].du_index_cnt	    = 0;
    Dub_01corecat_defs[4].du_requested	    = FALSE;
    Dub_01corecat_defs[4].du_exists	    = FALSE;
}


/*
**  DBMS CATALOG DEFINITIONS
**
**	History:
**
**	    I wouldn't claim credit for having written this either.
**
**	15-jun-93 (rickh)
**	    Added iixprotext, iiintegrityidx, and iiruleidx as indices
**	    which must be recreated at sysmod time.
**	25-aug-93 (robf)
**	    Added iilabelmap[idx], iilabelaudit for ES 2.0
**	9-dec-93 (robf)
**          Added iisecalarm for ES 2.0
**	31-Dec-2003 (schka24)
**	    Manual entries for the partition catalogs was just too much.
**	    Rewrite to operate in some reasonably sane manner.
**	19-Apr-2007 (bonro01)
**	    Add check to determine if the Dub_11dbmscat_defs[] array contains
**	    enough entries to contain the commands from the Dub_10dbmscat_commands
**	    array.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

STATUS
dub_dbmscats_def()
{
    char **commandsPtr;			/* Pointer to commands list */
    DUU_CATDEF *defPtr;			/* Pointer to dbmscat-defs list */
    i4 indexCount;			/* Indexes for this table */
    i4 dbmscatCount;			/* Available structure defs */

/* Map the command string list into a DUU_CATDEF structure */

    commandsPtr = &Dub_10dbmscat_commands[0];
    defPtr = &Dub_11dbmscat_defs[0];
    dbmscatCount = DU_1MAXDBMS_CATDEFS+1;
    do
    {
	/* First is always the catalog name */
	defPtr->du_relname = *commandsPtr++;
	/* Then the modify string */
	defPtr->du_modify = *commandsPtr++;
	/* See if there are index strings */
	indexCount = 0;
	while (*commandsPtr != NULL &&
	  (*commandsPtr)[0] == 'i' && (*commandsPtr)[1] == 'n')
	{
	    /* Looks like an index-on string;  store address of first one */
	    if (indexCount == 0) defPtr->du_index = commandsPtr;
	    ++ indexCount;
	    ++ commandsPtr;
	}
	defPtr->du_index_cnt = indexCount;
	/* Other members are cleared out */
	defPtr->du_requested = FALSE;
	defPtr->du_exists = FALSE;
	++ defPtr;
	-- dbmscatCount;
    } while (*commandsPtr != NULL && dbmscatCount > 0 );

    /* Dub_11dbmscat_def[] structure is too small */
    if( dbmscatCount == 0 )
	return(FAIL);

    /* Clear out the last one, rather unnecessarily */
    defPtr->du_relname = NULL;
    defPtr->du_modify = NULL;
    defPtr->du_index = NULL;
    defPtr->du_index_cnt = 0;
    defPtr->du_requested = FALSE;
    defPtr->du_exists = FALSE;

    return(OK);
}



VOID
dub_iidbdbcats_def()
{                                                                      
    /* iidatabase definition */
    Dub_31iidbdbcat_defs[0].du_relname	    = Dub_30iidbdbcat_commands[0];
    Dub_31iidbdbcat_defs[0].du_modify	    = Dub_30iidbdbcat_commands[1];
    Dub_31iidbdbcat_defs[0].du_index	    = &Dub_30iidbdbcat_commands[2];
    Dub_31iidbdbcat_defs[0].du_index_cnt    = 1;
    Dub_31iidbdbcat_defs[0].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[0].du_exists	    = FALSE;

    /* iiuser definition */
    Dub_31iidbdbcat_defs[1].du_relname	    = Dub_30iidbdbcat_commands[3];
    Dub_31iidbdbcat_defs[1].du_modify	    = Dub_30iidbdbcat_commands[4];
    Dub_31iidbdbcat_defs[1].du_index	    = NULL;
    Dub_31iidbdbcat_defs[1].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[1].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[1].du_exists	    = FALSE;

    /* iilocations definition */
    Dub_31iidbdbcat_defs[2].du_relname	    = Dub_30iidbdbcat_commands[5];
    Dub_31iidbdbcat_defs[2].du_modify	    = Dub_30iidbdbcat_commands[6];
    Dub_31iidbdbcat_defs[2].du_index	    = NULL;
    Dub_31iidbdbcat_defs[2].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[2].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[2].du_exists	    = FALSE;

    /* iiextend definition */
    Dub_31iidbdbcat_defs[3].du_relname	    = Dub_30iidbdbcat_commands[7];
    Dub_31iidbdbcat_defs[3].du_modify	    = Dub_30iidbdbcat_commands[8];
    Dub_31iidbdbcat_defs[3].du_index	    = NULL;
    Dub_31iidbdbcat_defs[3].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[3].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[3].du_exists	    = FALSE;

    /* iiusergroup definition */
    Dub_31iidbdbcat_defs[4].du_relname	    = Dub_30iidbdbcat_commands[9];
    Dub_31iidbdbcat_defs[4].du_modify	    = Dub_30iidbdbcat_commands[10];
    Dub_31iidbdbcat_defs[4].du_index	    = NULL;
    Dub_31iidbdbcat_defs[4].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[4].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[4].du_exists	    = FALSE;

    /* iirole definition */
    Dub_31iidbdbcat_defs[5].du_relname	    = Dub_30iidbdbcat_commands[11];
    Dub_31iidbdbcat_defs[5].du_modify	    = Dub_30iidbdbcat_commands[12];
    Dub_31iidbdbcat_defs[5].du_index	    = NULL;
    Dub_31iidbdbcat_defs[5].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[5].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[5].du_exists	    = FALSE;

    /* iidbpriv definition */
    Dub_31iidbdbcat_defs[6].du_relname	    = Dub_30iidbdbcat_commands[13];
    Dub_31iidbdbcat_defs[6].du_modify	    = Dub_30iidbdbcat_commands[14];
    Dub_31iidbdbcat_defs[6].du_index	    = NULL;
    Dub_31iidbdbcat_defs[6].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[6].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[6].du_exists	    = FALSE;

    /* iiprofile definition */
    Dub_31iidbdbcat_defs[7].du_relname	    = Dub_30iidbdbcat_commands[15];
    Dub_31iidbdbcat_defs[7].du_modify	    = Dub_30iidbdbcat_commands[16];
    Dub_31iidbdbcat_defs[7].du_index	    = NULL;
    Dub_31iidbdbcat_defs[7].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[7].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[7].du_exists	    = FALSE;

    /* iirolegrant definition */
    Dub_31iidbdbcat_defs[8].du_relname	    = Dub_30iidbdbcat_commands[17];
    Dub_31iidbdbcat_defs[8].du_modify	    = Dub_30iidbdbcat_commands[18];
    Dub_31iidbdbcat_defs[8].du_index	    = NULL;
    Dub_31iidbdbcat_defs[8].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[8].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[8].du_exists	    = FALSE;

    Dub_31iidbdbcat_defs[9].du_relname	    = NULL;
    Dub_31iidbdbcat_defs[9].du_modify	    = NULL;
    Dub_31iidbdbcat_defs[9].du_index	    = NULL;
    Dub_31iidbdbcat_defs[9].du_index_cnt    = 0;
    Dub_31iidbdbcat_defs[9].du_requested    = FALSE;
    Dub_31iidbdbcat_defs[9].du_exists	    = FALSE;
}


VOID
dub_ddbcats_def()
{

    i4	    cnt;

    for(cnt = 0; cnt < DU_4MAXDDB_CATDEFS; cnt++)
    {
	Dub_41ddbcat_defs[cnt].du_relname    = Dub_40ddbcat_commands[cnt * 2];
	Dub_41ddbcat_defs[cnt].du_modify     = Dub_40ddbcat_commands[cnt*2 + 1];
	Dub_41ddbcat_defs[cnt].du_index	     = NULL;
	Dub_41ddbcat_defs[cnt].du_index_cnt  = 0;
	Dub_41ddbcat_defs[cnt].du_requested  = FALSE;
	Dub_41ddbcat_defs[cnt].du_exists     = FALSE;
    }

    Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS].du_relname	    = NULL;
    Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS].du_modify	    = NULL;
    Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS].du_index	    = NULL;
    Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS].du_index_cnt	    = 0;
    Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS].du_requested	    = FALSE;
    Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS].du_exists	    = FALSE;

    return;
}

