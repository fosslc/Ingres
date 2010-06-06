/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**    Message file for Facility code:   _DU
**
**    History :
**
**      29-jul-1991 (fpang)
**	   Changed text of W_DU1816_SV_CANT_REGISTER_TABLE.
**      01-dec-1992 (robf)
**	   Added E_DU3516_DBDB_NOSECURITY. Also updated SUPERUSER
**	   descriptions in  various messages.
**	21-dec-1992 (robf)
**	   Added W_DU1831_SUPUSR_OBSOLETE for Destroydb & Sysmod no more
**	   -s (SUPERUSER) flag.
**	10-Jan-1993 (jpk)
**	   Added W_DU1085_SORTFLAG_DEPRECATED, W_DU1086_CANT_LOAD_PM_FILE
**	   W_DU1087_CANT_GET_HOSTNAME, and W_DU1088_PMSETDEFAULT_FAILED
**      29-Jan-1993 (jpk)
**	   Added I_DU00D1_IIQEF_ADD_EXTENSIONS, I_DU00D2_IIPROCPARAM_CRE,
**	   I_DU00D3_IIDEFAULT_CRE, I_DU00D4_IIKEY_CRE, W_DU1869_NOMODIFY,
**	   W_DU1869_NOMODIFY, W_DU186A_MISSING_SYSCAT, W_DU186B_REGISTER_FAILED,
**	   E_DU2416_NOCOPY_SYSCAT, E_DU2417_NODROP_SYSCAT,
**	   E_DU2418_NOCREATE_SYSCAT, E_DU2419_NOREPOP_SYSCAT,
**	   E_DU241A_SYSCAT_UPGRADE_FAILED, E_DU241B_NODROP_TEMP.
**	16-jun-93 (anitap)
**	    Added the following error messages: S_DU168D_SCHEMA_MISSING,
**	    S_DU168E_UNREFERENCED_DEFTUPLE, S_DU168F_ILLEGAL_KEY, 
**	    S_DU1690_MISSING_KEYTUP, S_DU1691_MISSING_RULETUP,
**	    S_DU1692_MISSING_DBPROCTUP, S_DU1693_MISSING_INDEXTUP,
**	    S_DU1694_INVALID_DEFAULT, S_DU034B_DELETE_IIDEFAULT.
**	23-july-1993 (jpk)
**	   Added E_DU3519_LEGAL_REG_DELIM_ID, I_DU0160_BAD_CASE_FLAGS,
**	   I_DU0161_USERID_CASE, I_DU0162_INSTALLATION_VALUE,
**	   I_DU0163_CMD_LINE_OVERRIDE for creating a FIPS database.
**	30-jul-93 (stephenb)
**	    Added errors for verifydb dbms_catalogs checks of iigw06_relation
**	    and iigw06_attribute.
**	7-sep-93 (robf)
**	    Added errors for B1 label garbage collection starting S_DU1750
**	4-oct-93 (dianeh)
**	    Cleanup of text destined for messages.txt.
**	10-oct-93 (anitap)
**	    Added S_DU16A8_IIDBDEPENDS_MISSING, S_DU0245_CREATE_SCHEMA,
**	    S_DU0445_CREATE_SCHEMA, S_DU0246_CREATE_CONSTRAINT &
**	    S_DU0446_CREATE_CONSTRAINT.
**	10-nov-93 (rblumer)
**	    defined new error message I_DU0164_BAD_CASE_FLAGS;
**	    changed I_DU0160_BAD_CASE_FLAGS to I_DU0160_BAD_USERID_CASE.
**	07-dec-93 (dianeh)
**	    Continue refining text destined for messages.txt file.
**	15-dec-93 (andre)
**	    Added messages DU16C0 and DU16C1
**	20-dec-93 (robf)
**          Added errors for new verifydb dbms checks
**	06-jan-94 (anitap)
**	    Corrected typo in S_DU16A8_IIDBDEPENDS_MISSING. Removed message
**	    S_DU16A9. Not needed.
**	    Added error messages E_DU503B_EMPTY_IIPROCEDURE and 
**	    E_DU503C_EMPTY_IIINTEGRITY.
**	07-jan-94 (anitap)
**	    Removed E_DU503C_EMPTY_IIINTEGRITY. Not needed as of last
**	    integration as integrity tuples will not be entered for NOT NULL
**	    constraints. So iiintegrity catalog could be empty.
**	11-jan-94 (andre)
**	    defined msgs 16C2 and 16C3
**	18-jan-94 (andre)
**	    defined msgs 0247, 0447, 16C4-16C6
**	19-jan-94 (andre)
**	    defined msgs 0248-024F, 0448-044F, 16C7-16CE
**	20-jan-94 (andre)
**	    defined msgs 16CF, 16E0-16E4, 0250-0251, 0450-0451
**	21-jan-94 (andre)
**	    defined msgs 16E5-16ED, 0252-0254, 0452-0454
**	23-jan-94 (andre)
**	    defined msgs 16EE
**	02-feb-94 (anitap)
**	    Enhanced message S_DU168F_ILLEGAL_KEY to include the words 
**	    "constraint id". Corrected spelling of 
**	    S_DU0370_DROP_FROM_IIINTERGRITY to S_DU0370_DROP_FROM_IIINTEGRITY.
**	    Same with S_DU0320_DROP_FROM_IIINTERGRITY.
**	10-feb-94 (andre)
**	    added quotes around constraint name in 0247
**	20-Feb-1994 (jpk)
**	    Added messages corresponding to robf's duerr.h definitions
**	    for I_DU00CD_C2SXA_CRE and I_DU00CE_IIAUDIT.
**	11-April-1994 (jpk)
**	    Change II_AUTHORIZATION reference to II_AUTH_STRING.
**	23-may-94 (jpk/kirke)
**	    Added messages for I_DU00D9_IIPRIV - I_DU00EF_UPG_SCHEMA.
**	11-jul-94 (robf)
**          Added messages for 1.1 ES upgradedb:
**		I_DU00F0_IILABELMAP_CRE
**		I_DU00F1_IISECALARM_CRE
**		I_DU00F2_IISESSION_PRIVILEGES_CRE
**		I_DU00F3_IIPHYSICAL_COLUMNS_CRE
**		I_DU00F4_IIROLEGRANT_CRE
**		I_DU00F5_IIPROFILE_CRE
**		I_DU00F6_IILABELAUDIT_CRE
**		I_DU00F7_IIPROFILES_CRE
**		I_DU00F8_IIROLES_CRE
**		I_DU00F9_IILEVELAUDIT_CRE
**		I_DU00FA_IIROLEGRANTS_CRE
**		I_DU00FB_IIDATABASE_INFO_CRE
**		I_DU00FC_IIEXTEND_INFO_CRE
**	12-jul-94 (robf)
**           Added more messages for 1.1 ES upgradedb:
**	15-September-1994 (Bin)
**	    Fixed a typo in S_DU17FD_CATALOG_PATCH_WARNING message text.
**      20-sep-1994 (wolf) 
**	    Fix typo in last change that caused ERCOMPILE failure.
**      03-mar-1995 (newca01)       
**          Add error message DU181F for starview.
**      21-mar-1995 (chech02)
**          Add E_DU6019_CANT_UPGRADESQL92 for upgradedb.
**      07-jun-1995 (chech02)
**          Add E_DU503C_TOO_MANY_VNODES for verifydb.
**	16-aug-1995 (harpa06)
**	    Added E_DU351A_MULTI_DB_DESTRY of destroydb.
**	19-jun-1996 (sweeney)
**	    Fix typos.
**	24-jun-1996 (somsa01)
**	    Changed "Ingres Corporation" to "Computer Associates" for
**	    S_DU17FC_TABLE_PATCH_WARNING and S_DU17FD_CATALOG_PATCH_WARNING.
**	16-oct-96 (mcgem01)
**	    Remove hardcoded ingres references from the comments which will
**	    wind up in the messages.txt which is to be shared by ingres
**	    and jasmine.   Since Jasmine does not use any of this code,
**	    I will not change the message text.
**	15-apr-97 (stephenb)
**	    add message E_DU351B_INCOMPATIBLE_VERSION.
**	10-jul-97 (nanpr01)
**	    System generated indexes cannot be dropped using drop index
**	    command. So message is changed accordingly. Also added
**	    E_DU503D_RUNMODE_INVALID message. Also added 
**	    S_DU0422_DROP_UNIQUE_CONSTRAINT,S_DU0423_DROP_UNIQUE_CONSTRAINT,
**	    S_DU0424_DROP_CHECK_CONSTRAINT, S_DU0425_DROP_CHECK_CONSTRAINT,
**	    S_DU0426_DROP_REF_CONSTRAINT, S_DU0427_DROP_REF_CONSTRAINT,
**	    S_DU0428_DROP_PRIMARY_CONSTRAINT, S_DU0429_DROP_PRIMARY_CONSTRAINT,
**	    S_DU1711_UNIQUE_CONSTRAINT, S_DU1712_CHECK_CONSTRAINT,
**	    S_DU1713_REFERENCES_CONSTRAINT, S_DU1714_PRIMARY_CONSTRAINT.
**	20-feb-1998 (rigka01)
**	    bug #89032
**	    change RUN_INTERACTIVE to RUNINTERACTIVE and RUN_SILENT to
**	    RUNSILENT in message E_DU503D and any others (e.g. E_DU5033) 
**	10-apr-98 (mcgem01)
**	    Change product name from OpenIngres to Ingres.
**	05-Nov-1998 (carsu07)
**	    Removed reference to CONVTO60 in message E_DU3150. (Bug 91643)
**	21-jun-1999 (nanpr01)
**	    Fixed the grammmatical error in error message. Bug 97567.
**      19-jan-2000 (stial01)
**          Added I_DU00FE_VERIFYING_TBL
**	23-May-2001 (jenjo02)
**	    Added I_DU00FF_IIQEF_CHECK_LOC_AREA
**	15-Oct-2001 (jenjo02)
**	    Deleted short-lived I_DU00FF_IIQEF_CHECK_LOC_AREA. Area directories
**	    are now made by CREATE/ALTER LOCATION.
**      07-nov-2001 (stial01)
**          Added E_DU3303_BAD_PAGESIZE
**      09-jan-2002 (stial01)
**          New upgradedb messages for SIR 106745
**	11-jun-2004 (gupsh01)
**	    Added error messages for loadmdb utility.
**	30-Aug-2004 (schka24)
**	    New upgradedb messages for r3 upgrade.
**	    Change 1617 (no dbdepends) message for verifydb.
**	30-Dec-2004 (schka24)
**	    Remove iixprotect verifydb messages.
**	15-Feb-2005 (gupsh01)
**	    Added error message E_DU6521_UNORM_NFC_WITH_NFD.
**	15-Feb-2005 (gupsh01)
**	    Remove unneccessary comment in the last line.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**      8-jul-2005 (stial01)
**          Updated text of S_DU17FE_FORCE_CONSISTENT_WARN
**      2-mar-2006 (stial01)
**          Added new messages for upgradedb
**	25=Jul-2007 (kibro01) b101500
**	    Downgraded DU168E to make clear it is not serious
**	06-Aug-2007 (smeke01) b118877
**	    Added error message E_DU503E_INVALID_LOG_FILE
**	31-Jan-2008 (smeke01) b118877
**	    Improved error message E_DU5023_CANT_OPEN_LOG_FILE
**	28-Feb-2008 (kibro01) b119427
**	    Added S_DU16F6_INVALID_IIPROTECT
**	17-Oct-2008 (kibro01) b121096
**	    Fix DU5009 to including database_name
**      11-dec-2008 (stial01)
**          Added E_DU351C_INCOMPATIBLE_VERSION
**	12-Nov-2009 (kschendel) SIR 122882
**	    compat level is now an integer, fix trace message.
**      07-dec-2009 (joea)
**          Add E_DU241C_SELECT_ERR_UPGR_IIDEFAULT and
**          E_DU241D_INSERT_ERR_UPGR_IIDEFAULT.
**      10-Feb-2010 (maspa05) b122651
**          Add S_DU025A_SET_MAND_RELFLAGS, S_DU025B_MAKE_PHYSL_TBL_HASH,
**          S_DU045A_MAND_RELFLAGS_SET, S_DU045B_PHYSL_TBL_MADE_HASH,
**          S_DU1760_MAND_FLAGS_INCORRECT, S_DU1761_PHYSL_TBL_NOT_HASH,
**          E_DU601B_CANT_SET_RELFLAGS and E_DU601C_CANT_MAKE_CATS_HASH
**	3-May-2010 (kschendel) SIR 123639
**	    Drop some unused information msgs, change DU00A1.
**	26-May-2010 (frima01) Bug 123753
**	    Add W_DU1874_UPGR_NOT_STAR_OWNER.
*/


#define _DU_CLASS   11


I_DU0010_UTIL_INTRO
"%0c database '%1c' . . .\n"
I_DU0011_UTIL_SUCCESS
"\n%0c of database '%1c' completed successfully.\n"
I_DU0020_FOUND_DB_FI
"  Database '%0c' was found in area '%1c'."
I_DU0021_FINDDBS_MODE_FI
"Running FINDDBS in %0c mode."
I_DU0022_FINDDBS_INTRO_FI
"Beginning search for databases . . ."
I_DU0023_END_SEARCH_FI
"End search for databases."
I_DU0024_START_UPDATE_FI
"\nStarting to update system catalog 'iidatabase'."
I_DU0025_END_UPDATE_FI
"Finished updating system catalog 'iidatabase'."
I_DU0026_START_ANALYZE_FI
"\nStarting FINDDBS analysis."
I_DU0027_END_ANALYZE_FI
"\nFINDDBS analysis is finished."
I_DU0028_PROMPT_FORLOC_FI
"\nEnter another database LOCATIONNAME to scan:"
I_DU0029_SCAN_AREA_FI
"\nScanning '%0c' for databases:"
I_DU002A_SAME_DBNAME_FI
"Two databases with the same name '%0c' have been found."
I_DU002B_ALL_REALDBS_FOUND_FI
"All real databases are catalogued in the database database."
I_DU002C_ALL_DBTUPS_FOUND_FI
"All 'iidatabase' tuples correspond to real databases."
I_DU002D_DBTUP_APPEND_FI
"  Inserting database '%0c' into IIDBDB."
I_DU002E_USAGE_FI
"  Usage:  FINDDBS [-a | -r] [-p] [-l] [-v]"
I_DU002F_SUCCESS_FI
"\nRebuilding of the IIDBDB completed successfully."
I_DU0030_ANLZ_SUCCESS_FI
"\nAnalysis of the IIDBDB completed successfully."
I_DU0031_NEWLOC_PROMPT_FI
"Enter any new location names you wish to scan/define. (RETURN to exit)>\n
NOTE: location names will only be added to installation in replace mode, but
      may be scanned in analyze mode."
/*%
** Parameters:
**	None.
**
** Explanation:
**	finddbs gives the user the opportunity to specify any extended data
**	locations that are not in the iilocation catalog. If the user specifies
**	a location name, that user will also be prompted for the physical
**	mapping of that location.
**
**	finddbs will search that location for databases whether in replace or
**	report mode.
**
**	If finddbs is running in replace mode, finddbs will also update the
**	iilocations catalog with this new location.
**
**	NOTE: It is recommended that the user use accessdb to define new
**	locations rather than specifying them via finddbs.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	Enter <CR> if they do not wish to specify any locations.
**	Otherwise, enter the name of the location.  The name must be
**	a valid location name and must not exceed 24 characters.
*/
I_DU0032_LOCAREA_PROMPT_FI
"Enter the physical area that the new location will map to.>
(RETURN to abort defining this location)>\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has chosen to specify a location to search that is not in
**	the iilocations catalog. [NOTE: The recommended way is to define the
**	location via accessdb and only run finddbs on locations defined in
**	the iilocation catalog].  The user has already been prompted for the
**	location name and has provided it.  Now the user is being prompted
**	for the physical mapping of that location.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	Enter a valid location mapping; the length may not exceed 128
**	characters.
*/
I_DU0061_CONVDB_INTRO
"%0c 6.0 conversion of database '%1c's' DBMS objects (6.0 BE Phase) ...\n\n"
I_DU0062_DBMS60_OK
"Conversion of the database '%0c's' DBMS objects completed successfully."
I_DU0080_UPGRADEDB_TERM
"\nUPGRADEDB is terminating.\n
Be sure to check the upgradedb log file for errors."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb is terminating its execution.  This is usually an error exit.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required by this message, but actions are probably required from
**	previous messages.
*/
I_DU0081_UPGRADEDB_DONE
"\n
*********************\n
  UPGRADEDB complete.\n
*********************\n\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb is terminating its execution.  This is displayed whether the
**	execution is successful or an error exit is being taken.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0082_UPGRADEDB_BEGIN
"\n
*************************\n
  -- UPGRADEDB Startup --\n
*************************\n
Parameter specified is '%0c'\n\n"
/*%
** Parameters:
**	%0c - upgradedb input parameter ("dbname" or "-all")
**
** Explanation:
**	upgradedb is beginning its execution.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0083_COMPAT_INFO
"Compat Level = %0d"
/*%
** Parameters:
**	%0d - compatibility level
**
** Explanation:
**	This is the output of upgradedb trace 5 and will not be output unless
**	upgradedb trace 5 is enabled.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0084_COMPAT_INFO
"return value from set_cmptlvl = %0d"
/*%
** Parameters:
**	%0d - internal upgradedb compatibility level
**
** Explanation:
**	This is the output of upgradedb trace 5 and will not be output unless
**	upgradedb trace 5 is enabled.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0085_IIDBDB_CONNECT
"\n
****************************\n
-- Connecting to 'iidbdb' --\n
****************************\n
\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb connects to the iidbdb and to individual user databases. This
**	is an informational message that upgradedb is connecting to the iidbdb.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0086_SUPERUSER
"User name is '%0c'\n
You are a super-user"
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	Identifies who the user is and that user is a super-user.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0087_NOTSUPERUSER
"User name is '%0c'\n
You are not a super-user"
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	Identifies who the user is and that the user is not a super-user.
**	If the user is also not the DBA, they will not be able to run the
**	upgrade.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0088_UPGRADE_ID
"Converting database %0c to %1c level..."
/*%
** Parameters:
**	%0c - database name
**	%1c - server version number
**
** Explanation:
**	Identifies what database is being upgraded and what level it is being
**	upgraded to.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU0089_DB_UPGRADED
"\n
*************************************************************************\n
Processing of database '%0c' completed successfully.\n
Database will be promoted to '%1c' status\n
*************************************************************************\n"
/*%
** Parameters:
**	%0c - database name
**	%1c - server version number
**
** Explanation:
**	Identifies that the database was successfully upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU008A_DB_NOT_UPGRADED
"\n
*************************************************************************\n
Processing of database '%0c' completed unsuccessfully.\n
Database will be NOT promoted to '%1c' status\n
*************************************************************************\n"
/*%
** Parameters:
**	%0c - database name
**	%1c - server version number
**
** Explanation:
**	Identifies that the database was not successfully upgraded.
**	Previous messages will indicate why.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required.
*/
I_DU008B_UPGRADEDB_BYPASSED
"Conversion bypassed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb is refusing to start any upgrades and is terminating its
**	execution.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None required by this message, but actions are probably required from
**	previous messages.
*/
I_DU008C_COLLECT_DBS
"Extracting database names from iidatabase..."
/*%
**Parameters:
**	None.
**
** Explanation:
**	upgradedb is refusing to start any upgrades and is terminating its
**	execution.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU008D_UPGRADE_TARGET
"Database '%0c' selected for conversion by UPGRADEDB."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb is going to convert this database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU008E_USER_DB_CONNECT
"\n
*****************************************************************************\n
Connecting to database '%0c' owned by user '%1c'\n
*****************************************************************************\n
\n
Converting core catalogs ..."
/*%
** Parameters:
**	%0c - database name
**	%1c - DBA
**
** Explanation:
**	upgradedb is now connecting to the target database.
**	The DBMS server will convert the core catalogs during this first
**	connection.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU008F_NOT_UPGRADED
"\n
****************************************************************\n
Processing of database '%0c' completed\n
\t\t %0c will NOT be promoted to %1c status.\n
****************************************************************\n"
/*%
** Parameters:
**	%0c - database name
**	%1c - server version number
**
** Explanation:
**	upgradedb informs the user that the database was not converted.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0090_IS_UPGRADED
"\n
****************************************************************\n
Processing of database '%0c' completed\n
\t\t %0c will be promoted to %1c status.\n
****************************************************************\n"
/*%
** Parameters:
**	%0c - database name
**	%1c - server version number
**
** Explanation:
**	upgradedb informs the user that the database was converted.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0091_IIUSER_CATALOG
"The iiuser catalog is currently %0c format"
/*%
** Parameters:
**	%0c - server version number
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0092_CONVERTING_IIUSER
"Converting iiuser..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0093_IIUSERGROUP_EXISTS
"The iiusergroup catalog currently exists"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0094_CRE_IIUSERGROUP
"Creating iiusergroup..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0095_IIROLE_EXISTS
"The iirole catalog currently exists"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0096_CRE_IIROLE
"Creating iirole..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0097_IISECRYSTATE_EXISTS
"The iisecuritystate catalog currently exists"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0098_CRE_IISECURITYSTATE
"Creating iisecuritystate..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU009A_IIDBPRIV_EXISTS
"The iidbpriv catalog currently exists"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU009B_CRE_IIDBPRIV
"Creating iidbpriv..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU009C_CRE_IIDBPRIVILEGES
"Creating view iidbprivileges..."
/*%
**Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU009D_IIDATABASE_CURRENT
"The iidatabase catalog is currently %0c format."
/*%
** Parameters:
**	%0c - server version number
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU009E_UPG_IIDATABASE
"Upgrading iidatabase..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU009F_APPEND_DUMP_LOC
"Appending default dump location"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that it is making the II_DUMP location
**	for this installation.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00A0_UPDATING_CAPS
"Updating iidbcapabilities entries......"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00A1_SERVER_CATS
"Checking and converting essential DBMS server catalogs..."
/*%
** Parameters:
**	None
**
** Explanation:
**	Informational upgradedb progress message.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00A2_UPG_IIPROTECT
"Converting iiprotect..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb informs the user that this catalog is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00B3_TRACE_01
"UPGRADEDB trace point 1. Call to UPGRADEFE skipped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb trace point 1 was specified, so upgradedb is skipping the
**	spawn of upgradefe.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00B4_FE_UPGRADE
"Upgrading Front End objects..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb is getting ready to do work on front-end objects.
**	This will translate to spawning upgradefe unless "-f nofeclients"
**	was specified.  Also, if no client list was specified, it will use
**	the authorization string assigned to II_AUTH_STRING to determine
**	which clients to specify.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00B5_TRACE_POINT
"---upgradedb trace point %0d---"
/*%
** Parameters:
**	%0d - trace point number
**
** Explanation:
**	This identifies that a trace point was hit.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00B6_DB_IDENTIFIER
"Database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	This identifies which database is being upgraded.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00C7_UPDT_IIDBPRIV
"Updating iidbpriv with users who may access private databases"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Access to private databases used to reside in catalog iidbaccess.  Now
**	that catalog is obsolete and the information is placed into iidbpriv.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00C8_PRIV_DB_CURRENT
"iidbpriv correctly contains definitions of users with access to private dbs."
I_DU00D0_IISCHEMA_CRE
"Creating iischema..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is converting to the
**	current format.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D1_IIQEF_ADD_EXTENSIONS
"Creating internal procedure iiqef_add_extensions ..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies an internal database procedure that upgradedb is adding
**	to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D2_IIPROCPARAM_CRE
"Creating iiprocedure_parameter..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D3_IIDEFAULT_CRE
"Creating iidefault..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D4_IIKEY_CRE
"Creating iikey..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D5_IIKEYS
"Creating iikeys..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D6_IICONSTRAINTS
"Creating iiconstraints..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D7_IIREF_CONSTRAINTS
"Creating iiref_constraints..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D8_IICONSTRAINT_INDEXES
"Creating iiconstraint_indexes..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00D9_IIPRIV
"iipriv..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00DA_MODIFY_CORE
"Modifying core catalogs..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This tells the user that upgradedb is modifying the core DBMS catalogs.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00DB_PREPARE_QT
"Preparing to upgrade views, permits, rules, integrities..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is squirreling away copies of system catalogs that
**	will later be needed to rebuild views, permits, rules, and integrities.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00DC_UPGR_BOOT_CAT
"Upgrading DBMS catalogs..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the "bootstrap" system catalogs -- those
**	needed to create and drop all the other DB objects.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00DD_IIINTEGRITY
"iiintegrity..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the system catalog iiintegrity.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00DE_IIDBDEPENDS
"iidbdepends..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the system catalog iidbdepends.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00DF_IIDEVICES
"iidevices..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the system catalog iidevices.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E0_IIPROCEDURE
"iiprocedure..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the system catalog iiprocedure.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E1_IISTATISTICS
"iistatistics..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the system catalog iistatistics.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E2_FILE_EXTEND_INGRES
"Performing file extend conversion for INGRES tables..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is performing file extend conversion for tables owned
**	by the system catalog owner -- the system and front end catalogs, 
**	usually.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E3_FILE_EXTEND_TBL
"   file extend converting %0c..."
/*%
** Parameters:
**	%0c - table being converted
**
** Explanation:
**	Upgradedb is performing file extend conversion for named table.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E4_MAKE_UDT_DEFAULTS
"Converting user defined datatype default values..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is setting up system catalog values to allow
**	user defined data types to have default values.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E5_FILE_EXTEND_USER
"Performing file extend conversion for user tables..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is performing file extend conversion for user tables.
**	This may take the longest time of any step in upgradedb.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E6_FILE_EXTEND_TBL_ERR
"   error %0d occurred when file extend converting %1c..."
/*%
** Parameters:
**	%0d - table being converted
**	%1c - table being converted
**
** Explanation:
**	Upgradedb encountered an error while performing file extend
**	conversion for named table.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E7_CONVERTING_TBL
"Converting %0c ..."
/*%
** Parameters:
**	%0c - table being converted
**
** Explanation:
**	This identifies a 6.4 catalog that upgradedb is converting to the
**	current release 1.x format.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E8_UPGRADING_VIEWS
"Converting views..."
/*%
** Parameters:
**	None
**
** Explanation:
**	This informs the user that upgradedb is about to upgrade views.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00E9_UPG_INGRES_VIEW
"Converting INGRES view %0c ..."
/*%
** Parameters:
**	%0c - view being converted
**
** Explanation:
**	This identifies a release 6.4 view (probably a standard catalog)
**	that upgradedb is rebuilding to fix the query trees.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00EA_UPG_USER_VIEW
"Converting view %0c ..."
/*%
** Parameters:
**	%0c - view being converted
**
** Explanation:
**	This identifies a 6.4 user view
**	that upgradedb is rebuilding to fix the query trees.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00EB_UPGRADING_PERMITS
"Converting permits..."
/*%
** Parameters:
**	None
**
** Explanation:
**	This informs the user that upgradedb is about to upgrade permits.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00EC_UPGRADING_INTEGRITIES
"Converting integrities..."
/*%
** Parameters:
**	None
**
** Explanation:
**	This informs the user that upgradedb is about to upgrade integrities.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00ED_UPGRADING_RULES
"Converting rules..."
/*%
** Parameters:
**	None
**
** Explanation:
**	This informs the user that upgradedb is about to upgrade integrities.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00EF_UPG_SCHEMA
"Adding schema..."
/*%
** Parameters:
**	None
**
** Explanation:
**	This informs the user that upgradedb is about to add schema.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F0_IILABELMAP_CRE
"Creating iilabelmap..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F1_IISECALARM_CRE
"Creating iisecalarm..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F2_IISESSION_PRIVILEGES_CRE
"Creating iisession_privileges..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F3_IIPHYSICAL_COLUMNS_CRE
"Creating iiphysical_columns..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F4_IIROLEGRANT_CRE
"Creating iirolegrant..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F5_IIPROFILE_CRE
"Creating iiprofile..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F6_IILABELAUDIT_CRE
"Creating iilabelaudit..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F7_IIPROFILES_CRE
"Creating iiprofiles..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F8_IIROLES_CRE
"Creating iiroles..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00F9_IILEVELAUDIT_CRE
"Creating iilevelaudit..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00FA_IIROLEGRANTS_CRE
"Creating iirolegrants..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00FB_IIDATABASE_INFO_CRE
"Creating iidatabase_info..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00FC_IIEXTEND_INFO_CRE
"Creating iiextend_info..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This identifies a catalog that upgradedb is adding to the database.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00FD_IIEVENT_CATALOG
"The iievent catalog is currently %0c format"
/*%
** Parameters:
**	%0c - server version number
**
** Explanation:
**	upgradedb informs the user that this catalog does not require upgrading.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00FE_VERIFYING_TBL
"Verifying %0c ..."
/*%
** Parameters:
**	%0c - table being verified
**
** Explanation:
**      This identifies that a table is being verified during upgrade
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU00FF_IISEQUENCE
"iisequence..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is upgrading the system catalog iisequence.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	None.
*/
I_DU0100_MODIFYING_SY
"  Modifying '%0c' . . ."
I_DU0101_PARTITION_CRE
"Creating partitioning catalogs ..."
/*%
** Parameters:
**
** Explanation:
**      This identifies that catalogs are being created during upgrade.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0102_DROPPING_STDCAT_VIEWS
"Dropping old standard catalog views ..."
I_DU0103_RECREATING_STDCAT_VIEWS
"Recreating standard catalog views ..."
I_DU0104_UNLOADING_TREES
"Running unloaddb to unload query-tree objects (views, procedures, rules) ..."
/*%
** Parameters:
**	None
**
** Explanation:
**	Many non-table database objects are stored as "query trees".
**	These include views, DB procedures, rules, and some QUEL permits
**	and integrities.  Upgradedb has determined that the query tree
**	format in this database is obsolete.  It regenerates the query
**	trees by dropping and redefining all tree objects, and it uses
**	the UNLOADDB utility to do much of the work.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0105_RELOADING_TREES
"Reloading query-tree objects ..."
I_DU0106_FIXING_64_GRANTS
"Fixing 6.4-style all-to-public and select-to-public grants ..."
/*%
** Parameters:
**	None
**
** Explanation:
**	Databases created by version 6.4 or earlier stored all-to-public
**	and select-to-public in a different format from current databases.
**	Upgradedb is converting these grants by reissuing them.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0107_FIXING_CAT_GRANTS
"Redoing system catalog grants ..."
I_DU0108_MODIFYING_PGSIZE
"Modifying converted catalogs back to %0d page size ..."
I_DU0109_USER_DB_RECONNECT
"Reconnecting to database %0c owned by user %1c."
/*%
** Parameters:
**	%0c - database name
**	%1c - DBA
**
** Explanation:
**	upgradedb has disconnected from the target database and is now
**	reconnecting.  This is done several times during the upgrade
**	process, to flush cached database information or to modify
**	the connection characteristics.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0110_DROPPING_REF_CONS
"Dropping cross-user referential constraints ..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A foreign key constraint that references a table owned by a different
**	user creates a system generated rule, in a form that cannot be
**	upgraded directly by upgradedb.  Upgradedb must upgrade these
**	rules by dropping the referential constraint.  The constraint
**	will be regenerated later in the upgrade process.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0111_RECREATING_REF_CONS
"Recreating cross-user referential constraints ...\n
(This may take a while for large tables.)"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is recreating foreign key constraints that referenced
**	a different user's table.  This will recreate the associated
**	system generated rules, in the format required by the current
**	Ingres version.
**
**	Recreating a referential constraint requires a pass through the
**	data to ensure that the constraint is satisfied initially.
**	If the referencing table is large, this could take some time.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0112_DROPPING_CONS
"Dropping OpenIngres 1.2 referential and check constraints ..."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Referential and check constraints create system generated rules.
**	OpenIngres 1.2 did not create these rules in a form that can be
**	upgraded directly by upgradedb.  Upgradedb must upgrade these
**	rules by dropping the constraint.  The constraints will be
**	regenerated later in the upgrade process.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0113_RECREATING_CONS
"Recreating referential and check constraints ...\n
(This may take a while for large tables.)"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is recreating constraints that were dropped earlier.
**	This will recreate the associated system generated rules, in the
**	format required by the current Ingres version.
**
**	Recreating a constraint requires a pass through the data to
**	ensure that the constraint is satisfied initially.  If the table
**	is large, this could take some time.
**
** System Status:
**	This is an informational message.
**
** Recommendation:
**	No action is required.
*/
I_DU0114_CONVERTING_STAR
"Converting Distributed Option (Star) catalogs ..."
I_DU0160_BAD_USERID_CASE
" %0c case userids aren't permitted with %1c case\n
regular identifiers and %2c case delimited identifiers."
/*%
** Parameters:
**	%0c - real_user_case value (UPPER, LOWER, or MIXED)
**	%1c - reg_id_case value (UPPER, LOWER, or MIXED)
**	%2c - delim_id_case value (UPPER, LOWER, or MIXED)
**
** Explanation:
**	Some case combinations are illegal; createdb displays this message
**	when it detects an illegal combination.
**
** System Status:
**	The createdb process aborts; the database is not created.
**
** Recommendation:
**	Check the createdb command-line switches and/or configuration settings.
*/
I_DU0161_USERID_CASE
"  Userid case must be the same as either regular identifier case\n
or delimited identifier case."
I_DU0162_INSTALLATION_VALUE
"  %0c case for %1c was taken from your installation defaults."
/*%
** Parameters:
**	%0c - case value: UPPER, LOWER or MIXED
**	%1c - parameter name: regular or delimited identifier, or user ID
**
** Explanation:
**	When values are taken from command line and/or installation
**	defaults, this message clarifies which came from where. Presumably
**	the user can see the ones on the command line.  This message
**	explains which ones came from installation defaults.
**
** System Status:
**	The createdb process aborts; the database is not created.
**
** Recommendation:
**	Check the createdb command line switches and/or configuration settings.
*/
I_DU0163_CMD_LINE_OVERRIDE
"  You can override an installation default by specifying a different\n
value on the createdb command line."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is provided to help the user with a list of
**	legal combinations.
**
** System Status:
**	The createdb process aborts; the database is not created.
**
** Recommendation:
**	Check the createdb command line switches and/or configuration settings.
*/
I_DU0164_BAD_CASE_FLAGS
" %0c case regular identifiers are not permitted\n
with %1c case delimited identifiers."
/*%
** Parameters:
**	%0c - reg_id_case value (UPPER or LOWER)
**	%1c - delim_id_case value (UPPER, LOWER, or MIXED)
**
** Explanation:
**	The reg_id_case and delim_id_case flags must be the same,
**	or delim_id_case must be mixed.  Any other combinations return
**	this error.  See error DU3519 for a list of legal combinations.
**
** System Status:
**	The createdb process aborts; the database is not created.
**
** Recommendation:
**	Check the createdb command-line switches and/or configuration settings.
*/
I_DU0200_CR_CORE_CATS
"  Creating DBMS Core System Catalogs . . ."
I_DU0201_CR_DBMS_CATS
"  Creating DBMS System Catalogs . . ."
I_DU0202_CR_DBDB_CATS
"  Creating Database Database System Catalogs . . ."
I_DU0203_MOD_DBMS_CATS
"  Modifying DBMS System Catalogs . . ."
I_DU0204_MOD_DBDB_CATS
"  Modifying Database Database System Catalogs . . ."
I_DU0205_CR_FE_CATS
"  Creating Front-end System Catalogs . . ."
I_DU0206_CR_SCI_CATS
"  Creating Standard Catalog Interface . . ."
I_DU0207_MOD_FE_CATS
"  Modifying Front-end System Catalogs . . ."
/*
** messages S_DU240 to S_DU02FF are reserved for verifydb informative msgs
*/
S_DU0240_SET_COMMENT
"The recommended action is to indicate that a comment is defined for this table/view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found that the table does not indicate that there is a comment
**	defined for this table/view.  The recommended solution is to change
**	iirelation.relstat for the table.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
**	This is an informational message, so processing continues.  However,
**	this type of message requires input from the user.  The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0241_DROP_THIS_COMMENT
"The recommended action is to delete this comment."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a corrupted comment and wants to drop it from the
**	database.  If verifydb is running in "runinteractive" mode, the user
**	will be prompted for yes/no permission.
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0242_SET_DELIM_MIXED
"The recommended action is to set Mixed Case for Delimited Identifiers.\n\n
WARNING:\n
VERIFYDB can only guess at what is correct for your database.  You may wish to\n
visually inspect the database, selecting ii_columns.table_name,\n
iicolumns.owner_name and iicolumns.column_name to determine correct name case\n
semantics for the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found illegal delimited identifier semantics.  If verifydb
**	is running in "runinteractive" mode, the user will be prompted for
**	yes/no permission.
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0243_CLEAR_DELIM_UPPER
"The recommended action is to remove Upper Case for Delimited Identifiers.\n\n
WARNING:\n
VERIFYDB can only guess at what is correct for your database.  You may wish to\n
visually inspect the database, selecting ii_columns.table_name,\n
iicolumns.owner_name and iicolumns.column_name to determine correct name case\n
semantics for the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found illegal delimited identifier semantics.  If verifydb
**	is running in "runinteractive" mode, the user will be prompted for
**	yes/no permission.
**
** System Status:
**	This is an informative message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0244_CLEAR_REALUSER_MIXED
"The recommended action is to remove Mixed Case for Real User Identifiers.\n\n
WARNING:\n
VERIFYDB can only guess at what is correct for your database.  You may wish to\n
visually inspect the database, selecting ii_columns.table_name,\n
iicolumns.owner_name and iicolumns.column_name to determine correct name case\n
semantics for the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found illegal delimited identifier semantics.  (Real user
**	identifiers are the user's login ID.) If verifydb is running in
**	"runinteractive" mode, the user will be prompted for yes/no
**	permission.
**
** System status
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0245_CREATE_SCHEMA
"The recommended action is to create the schema."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an orphaned object and wants to create the schema for
**	the object. If verifydb is running in "runinteractive" mode, the user
**	will be prompted for yes/no permission.
**
** System Status:
**	This is an informational message only; processing continues.
**
** Recommendation:
**	Enter Y(es) or N(o).
*/
S_DU0246_CREATE_CONSTRAINT
"The recommended action is to create the constraint."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a constraint with no corresponding index/rule/procedure
**	to support it. So verifydb dropped the constraint and wants to create
**	the constraint for the table. If verifydb is running in "runinteractive"
**	mode, the user will prompted for yes/no permission.
**
** System Status:
**	This is an informational message only; processing continues.
**
** Recommendation:
**	Enter Y(es) or N(o).
*/
S_DU0247_DROP_CONSTRAINT
"The recommended action is to drop constraint '%0c'.'%1c'
\nfrom this database."
/*%
** Parameters:
**	%0c - schema in which the constraint resides
**	%1c - name of the constraint
**
** Explanation:
**	verifydb found that the object on which this constraint depends does not
** 	exist
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU0248_DELETE_IIXPRIV
"The recommended action is to delete the offending tuple from iixpriv."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPRIV test #1 found an entry in iixpriv that does not have
**	a corresponding entry in iipriv.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**      None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**      case, enter Y(ES) or N(O).
*/
S_DU0249_INSERT_IIXPRIV_TUPLE
"The recommended action is to append missing tuple to iixpriv."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPRIV test #2 found a tuple in iipriv that does not have a
**	corresponding tuple in iixpriv.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU024A_DELETE_IIXEVENT
"The recommended action is to delete the offending tuple from IIXEVENT."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXEVENT test #1 found an entry in iixevent that does not have
**	a corresponding entry in iievent.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**      None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**      case, enter Y(ES) or N(O).
*/
S_DU024B_INSERT_IIXEVENT_TUPLE
"The recommended action is to append missing tuple to iixevent."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXEVENT test #2 found a tuple in iievent that does not have a
**	corresponding tuple in iixevent.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU024C_DELETE_IIXPROC
"The recommended action is to delete the offending tuple from IIXPROCEDURE."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPROCEDURE test #1 found an entry in iixprocedure that 
**	does not have a corresponding entry in iiprocedure.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**      None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**      case, enter Y(ES) or N(O).
*/
S_DU024D_INSERT_IIXPROC_TUPLE
"The recommended action is to append missing tuple to iixprocedure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPROCEDURE test #2 found a tuple in iiprocedure that 
**	does not have a corresponding tuple in iixprocedure.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0250_DELETE_IIPRIV_TUPLE
"The recommended action is to delete all tuples with\n
d_obj_id = %0d, d_obj_idx = %1d, d_priv_number = %2d and d_obj_type = %3d\n
from iipriv."
/*%
** Parameters:
**	%0d - iipriv.d_obj_id
**	%1d - iipriv.d_obj_idx
**	%2d - iipriv.d_priv_number
**	%3d - iipriv.d_obj_type
**
** Explanation:
**	verifydb IIPRIV test #1 found an entry in iipriv that describes a
**	privilege descriptor for a non-existent object or an object of 
**	unexpected type
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0251_MARK_DBP_DORMANT
"Recommended action is to mark the database procedure 
\n(and all database procedures dependent on it) dormant.
\nThis will also result in destruction of permits on newly dormant 
\ndatabase procedures."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a database procedures s.t. some object or privilege on 
**	which it depends no longer exists - rather than dropping the database 
**	procedure we will mark it dormant which is exactly what should have 
**	happened when the object or privilege on which the database procedure 
**	depends was destroyed
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0252_SET_DB_DBP_INDEP_LIST
"The recommended action is to indicate that a database procedure depends on
\nsome objects and/or privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROCEDURE test #6 discovered that while the IIPROCEDURE
**	tuple describing a database procedure indicates that it does not depend
**	on any objects and/or privileges, record(s) describing such dependencies
**	were found by examining IIDBDEPENDS and/or IIPRIV.  
**	The recommended action is record the fact that the database procedure 
**	depends on some objects and/or privileges
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0253_UBT_IS_A_DBMS_CATALOG
"The recommended action is to zero out the database procedure's
\nunderlying base table id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROCEDURE test #7 discovered that the database procedure's
**	underlying base table is a DBMS catalog. 
**	The recommended action is to zero out the database procedure's 
**	underlying base table id.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0254_FIX_OBJNAME_IN_IIPROT
"The recommended action is to update schema and object name from the catalog
\ndescribing the object itself."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #5 found that the name of the schema and/or the
**	object stored in IIPROTECT tuple do not match those in the catalog 
**	describing the object itself (IIRELATION for tables and view, 
**	IIPROCEDURE for database procedures, or IIEVENT for dbevents.)
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0255_SET_PERM_LANG_TO_QUEL
"The recommended action is to update proflags to indicate that the 
\npermit was created using QUEL."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #8 found that proflags indicates that the 
**	permit was created using SQL, but IIQRYTEXT indicates otherwise.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0256_SET_PERM_LANG_TO_SQL
"The recommended action is to update proflags to indicate that the 
\npermit was created using SQL."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #9 found that proflags indicates that the 
**	permit was created using QUEL, but IIQRYTEXT indicates otherwise.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0257_PERM_CREATED_BEFORE_10
"The recommended action is to update proflags to indicate that the 
\npermit was created prior to OpenIngres 1.0."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #10 found that proflags indicates that the 
**	permit was created after 6.4, but the permit conveys multiple privileges
**	which suggests that it was created in 6.4 or before
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	      user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0258_DROP_DBEVENT
"Recommended action is to mark the dbevent for destruction." 
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #12 found a permit on a dbevent issued by a 
**	user who does not own it and no longer possesses GRANT OPTION FOR
**	privileges specified in the permit.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0259_CLEAR_VGRANT_OK
"Recommended action is to set the bit in iirelation.relstat
\nwhich indicates that the view depends on some privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIRELATION test #30 discovered that while the IIRELATION
**	tuple describing a view indicates that it does not depend
**	on any privileges, record(s) describing such dependencies
**	were found by examining IIPRIV.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU025A_SET_MAND_RELFLAGS
"Recommended action is to set the flags in IIRELATION.relstat and 
\nrelstat2 which are mandatory for %0c."
/*%
** Parameters:
**	%0c - system catalog name
**
** Explanation:
**	verifydb discovered that a system catalog defined to have mandatory
**	flags for relstat and/or relstat2 does not have the correct flags.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU025B_MAKE_PHYSL_TBL_HASH
"Recommended action is to modify %0c back to its intended structure."
/*%
** Parameters:
**	%0c - system catalog name
**
** Explanation:
**	verifydb discovered that a system catalog with TCB2_PHYSLOCK_CONCUR
**	was not hash.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
/*
** Messages S_DU0300 through S_DU034f are used only in
** RUNINTERACTIVE mode
*/
S_DU0300_PROMPT
" Enter Y for yes, N for no. >"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This messages requests input from the user -- used with the
**	"runinteractive" mode of verifydb.  This prompt message always
**	follows an informative message indicating the action that verifydb
**	will take if the user gives permission.  It is automatically output
**	by duve_talk() when message_type (duve_mtype) = ASK and
**	mode (duve_cb->duve_mode) = RUNINTERACTIVE.  It is also output
**	when duve_mtype = INTERACTIVE and mode = RUNINTERACTIVE.  It is
**	never output with any other message type and mode combinations.
**
** System Status:
**	Wait until input; loop back to prompt if input not valid.
**
** Recommendation:
**	Enter Y(ES) or N(O) and hit carriage return.
*/
S_DU0301_DROP_IIRELATION_TUPLE
"The recommended action is to drop this tuple from iirelation\n
(which effectively drops table %0c (owner %1c) from the database)"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #1 found a tuple with an invalid table specifier.
**	Since the table specifier is used to join iirelation tuples to tuples
**	in other system catalogs, this tuple is useless.  If we know the
**	correct table ID for this table, we should manually replace it in
**	iirelation.  Otherwise, this tuple should be deleted from iirelation.
**	The recommended corrective action to make the system catalogs
**	consistent is to drop this table from the database by removing it
**	from the iirelation catalog.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the user
**	will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0302_DROP_TABLE
"The recommended action is to drop table %0c (owner %1c)\n
from the database."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	A verifydb test found a condition (which it reported) that leaves
**	the system catalogs inconsistent.  The recommended corrective action
**	to make the system catalogs consistent is to drop this table from
**	the database via an SQL drop.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	(This message is displayed in conjunction with verifydb test #2a.)
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0303_REPLACE_RELWID
"The recommended action is correct the value in iirelation\n
of (%0d) with (%1d)"
/*%
** Parameters:
**	%0d - table width from iirelation (iirelation.relwid)
**	%1d - sum of width of each attribute (from each iiattribute.attfrml)
** Explanation:
**	verifydb test #2b found a mismatch in table width between iirelation
**	and the sum of each column's width.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0304_REPLACE_RELKEYS
"The recommended action is to correct the value in iirelation\n
of (%0d) with (%1d)"
/*%
** Parameters:
**	%0d - number of keys iirelation thinks there are (iirelation.relkeys)
**	%1d - count of each key in iiattribute
**
** Explanation:
**	verifydb test #3 found a mismatch between the number of keys that should
**	be in the table's primary key and the number that are actually in the
**	key.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0305_CLEAR_PRTUPS
"Recommended action is to clear protection\n
information from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #8 found that iirelation specified there are protections
**	on this table, but none are defined in iiprotect.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0306_CLEAR_INTEGS
"Recommended action is to clear integrity\n
information from iirelation."
/*%
** Parameters:
**	None.
** Explanation:
**	verifydb test #9 found that iirelation specified there are integrities
**	on this table, but none are defined in iiintegrity.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0307_CLEAR_CONCUR
"Recommended action is to clear this locking\n
specification from iirelation."
/*%
** Parameters:
**	None.
** Explanation:
**	verifydb test #10 found that the CONCUR bit was set for a table that
**	was not a DBMS core system catalog.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0308_SET_CONCUR
"Recommended action is to set this locking\n
specification from iirelation."
/*%
** Parameters:
**	None.
** Explanation:
**	verifydb test #11 found that one of the DBMS core system catalogs did
**	not have the special locking (CONCUR) bit set in iirelation.relstat.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0309_MAKE_TABLE
"Recommended action is to alter iirelation to\n
indicate that this is a table, not a view"
/*%
** Parameters:
**	None.
** Explanation:
**	verifydb test #12 found that there was a physical disk file associated
**	with this view.  This often indicates that it's really a table that
**	has been mismarked as a view.  However, it is possible that this is
**	really a view and someone has renamed files or invalidated a backup
**	somehow.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU030A_CLEAR_CATALOG
"Recommended action is to alter iirelation to\n
stop indicating that this table is a system catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #5a found that a table was mismarked as a system
**	catalog when it did not start with "ii" or when the owner was not
**	the system catalog owner.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU030B_DROP_VIEW
"The recommended action is to drop view\n
%0c (owner %1c)\n
from the database."
/*%
** Parameters:
**	%0c - view name (null terminated string)
**	%1c - view owner (null terminated string)
**
** Explanation:
**	verifydb test #12 found that the specified view was not defined in
**	iitree.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU030C_CLEAR_VBASE
"Recommended action is to clear view base\n
specification from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #15 found that iirelation believed there was a view
**	based on this table, but none could be found in iidbdepends.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU030D_SET_IDXD
"The recommended action is to set a bit in iirelation.relstat\n
for base table %0c (owner %1c)\n
to indicate that the table has a secondary index."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	The IDXD bit in iirelation.relstat for the base table indicates that
**	there is a secondary index table for this table.  verifydb will 
**	set this bit automatically, unless verifydb is in RUNINTERACTIVE mode.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	This is the prompt for verifydb test #17 and #30.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU030E_FIX_RELIDXCOUNT
"The recommended action is to set iirelation.relidxcount to\n
%0d (from %1d)"
/*%
** Parameters:
**	%0d - number of secondary indexes defined in iiindexes
**	%1d - number of secondary indexes iirelation thinks there are
**  	      (from iirelation.relidxcount)
**
** Explanation:
**	verifydb test #18 found a discrepancy between the number of secondary
**	indexes that iirelation thinks there are and the number of secondary
**	indexes defined in iiindex.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Give yes/no permission to set iirelation.relidxcount to the
**	number of index tables defined in iiindex.
*/
S_DU030F_CLEAR_RELIDXCOUNT
"The recommended action is to modify iirelation to indicate that\n
there are no secondary indexes for table\n
%0c (owner %1c)"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #18 found that iirelation thinks there are secondary
**	indexes defined on this table and there AREN'T any.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Give yes/no permission to clear iirelation.relidxcount and 
**	clear IDXD bit from iirelation.relstat.
*/
S_DU0310_CLEAR_OPTSTAT
"The recommended action is to modify iirelation to\n
stop indicating that there are optimizer statistics\n
for this table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #19 found that iirelation thinks there are optimizer
**	statistics for this table, but there aren't any.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0311_RESET_RELLOCOUNT
"The recommended action is to modify the number of locations\n
in iirelation from %0d to %1d."
/*%
** Parameters:
**	%0d - number of locations iirelation thinks there are
**	      (from iirelation.relloccount)
**	%1d - number of locations defined in iidevices
**
** Explanation:
**	verifydb test #20 found a discrepancy between the number of locations
**	that iirelation thinks there are and the number of locations defined
**	in iidevices.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0312_FIX_RELDFILL
"The recommended action is to replace the invalid data fill factor with\n
the default data fill factor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The data page fill factor must be specified for all table types.  If
**	it is not, then a default value will be used.  This is detected by
**	verifydb tests #22 through #25.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0313_FIX_RELLFILL
"The recommended action is to replace the invalid leaf fill factor with\n
the default leaf fill factor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The leaf page fill factor must be specified for all table types.  If
**	it is not, then a default value will be used.  This is detected by
**	verifydb test #25, and applies only to Btrees.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0314_FIX_RELIFILL
"The recommended action is to replace the invalid index fill factor with\n
the default index fill factor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The index page fill factor must be specified for all table types.  If
**	it is not, then a default value will be used.  This is detected by
**	verifydb test #25, and applies only to Btrees.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0315_CREATE_IIRELIDX
"The recommended action is to create a tuple in iirel_idx for this table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #26 found a tuple in iirelation that does not have a
**	corresponding tuple in iirel_idx.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0316_CLEAR_IIRELIDX
"The recommended action is to drop this tuple from system catalog iirel_idx."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #27 found an entry in iirel_idx that does not have
**	a corresponding entry in iirelation.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0317_DROP_ATTRIBUTES
"The recommended action is to delete tuples with\n
attrelid = %0d, attrelidx = %1d\n
from iiattributes."
/*%
** Parameters:
**	%0c - attrelid (from iiattributes)
**	%1c - attrelidx (from iiattributes)
**
** Explanation:
**	verifydb test #28 found some tuples in iiattributes that have table id
**	and index table id values that do not match any entries in iirelation.
**	this means that they are useless, and should be deleted.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0318_DROP_INDEX
"The recommended action is to delete this tuple\n
(baseid = %0d, indexid = %1d)\n
from iiindex."
/*%
** Parameters:
**	%0d - baseid (from iiindex)
**	%1d - indexid (from iiindex)
**
** Explanation:
**	verifydb test #30 found some tuples in iiindex that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	This means that they are useless, and should be deleted.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0319_SET_INDEX
"The recommended action is to modify iirelation to indicate that\n
this is an index table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #31 found that this table is a secondary index table, but
**	it is not marked as such in iirelation.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU031A_DROP_DEVRELID
"The recommended action is to delete tuples with\n
devrelid = %0d, devrelidx = %1d\n
from iidevices."
/*%
** Parameters:
**  %0d - devrelid (from iidevices)
**  %1d - devrelidx (from iidevices)
**
** Explanation:
**	verifydb test #33 found some tuples in iidevices that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	This means that they are useless, and should be deleted.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU031B_SET_MULTIPLE_LOC
"The recommended action is to modify iirelation to indicate that this table
has multiple locations defined."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #34 found that iirelation did not have the MULTIPLE_LOC
**	bit set in relstat, but there are actually locations defined for this
**	table in iidevices.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
**/
S_DU031C_DEL_TREE
"The recommended action is to delete all tuples\n
with base table id = %0d, index table id = %1d\n
tree id = %2d %3d from iitree."
/*%
** Parameters:
**	%0d - base table ID associated with tree (from iitree.treetabbase)
**	%1d - base table index ID associated with tree (from iitree.treetabidx)
**	%2d - high portion of tree ID (iitree.treeid1)
**	%3d - low portion of tree ID (iitree.treeid2)
**
** Explanation:
**	verifydb test #35 found tuples in iitree with invalid table ID, or
**	verifydb test #36 found tuples in iitree with an invalid tree mode,
**	or verifydb test #37 found the tree ID was invalid.
**    
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE:  this message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU031D_SET_VIEW
"The recommended action is to set the VIEW bit in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the VIEW bit should have been set in
**	iirelation.relstat (for this view) but was not.
**    
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
**
**	NOTE: The user will usually want to enter yes.  The query text for this
**	view can be retrieved from iiqrytext, and the DBMS can rebuild the view
**	if the query text is rekeyed.
*/
S_DU031E_SET_PRTUPS
"The recommended action is to set the PRTUPS bit in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the PRTUPS bit should have been set in
**	iirelation.relstat (for this table) but was not.
**    
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
**
**	NOTE: The user will usually want to enter yes.  The query text for this
**	permit can be retrieved from iiqrytext, and the DBMS can rebuild the
**	permit if the query text is rekeyed.
*/
S_DU031F_SET_INTEGS
"The recommended action is to set the INTEGS bit in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the INTEGS bit should have been set in
**	iirelation.relstat (for this table) but was not.
**    
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
**
**	NOTE: The user will usually want to enter yes.  The query text for this
**	integrity can be retrieved from iiqrytext, and the DBMS can rebuild the 
**	integrity if the query text is rekeyed.
*/
S_DU0320_DROP_FROM_IIINTEGRITY
"The recommended action is to drop all tuples from iiintegrity\n
with inttabbase = %0d and inttabidx = %1d."
/*%
** Parameters:
**	%0d - base table ID (from iiintegrity.inttabbase)
**	%1d - index table ID (from iiintegrity.inttabidx)
**
** Explanation:
**	verifydb test #40 found that the table that is supposed to receive
**	this integrity doesn't exist (at least it's not listed in iirelation);
**	or, verifydb test #42 found the tree that defines the integrity is
**	missing from iitree, which means the DBMS cannot enforce the integrity.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0321_DROP_FROM_IIPROTECT
"The recommended action is to drop all tuples from iiprotect\n
with protabbase = %0d and protabidx = %1d."
/*%
** Parameters:
**	%0d - base table ID (from iiprotect.protabbase)
**	%1d - index table ID (from iiprotect.protabidx)
**
** Explanation:
**	verifydb test #44 found that the table that is supposed to receive
**	this integrity doesn't exist (at least it's not listed in iirelation);
**	or, verifydb test #46 found the tree that defines the permit is missing
**	from iitree, which means the DBMS cannot enforce the permit.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0322_DROP_IIDBDEPENDS_TUPLE
"The recommended action is to delete all tuples with\n
inid1 = %0d, inid2 = %1d, deid1 = %2d and deid2 = %3d\n
from iidbdepends."
/*%
** Parameters:
**	%0d - iidbdepends.inid1
**	%1d - iidbdepends.inid2
**	%2d - iidbdepends.deid1
**	%3d - iidbdepends.deid2
**
** Explanation:
**	verifydb test #54 found an entry in iidbdepends that describes a
**	view or table with permit as dependent on another table.  However,
**	the dependent table/view does not exist (or is not defined in 
**	iirelation). Or, verifydb test #55 found that the independent table
**	does not exist, so the dependency described by this entry in
**	iidbdepends is not valid.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0323_SET_VBASE
"The recommended action is to indicate that\n
a view is defined against this table/view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #56 found that the independent table does not indicate
**	that there is a view defined against this table/view.  The recommended
**	solution is to change iirelation.relstat (set VBASE) for the 
**	independent table.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0324_CREATE_IIXDBDEPENDS
"The recommended action is to create a tuple in iixdbdepends for this table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #58 found a tuple in iidbdepends that does not have a
**	corresponding tuple in iixdbdepends.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0325_REDO_IIXDBDEPENDS
"The recommended action is to drop this index table and recreate it."
/*%
** Parameters:
**	None.
** Explanation:
**	verifydb test #59 found an entry in iixdbdepends that does not have
**	a corresponding entry in iidbdepends.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user.  The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0326_DROP_FROM_IISTATISTICS
"The recommended action is to drop all tuples from iistatistics\n
with stabbase = %0d and stabindex = %1d."
/*%
** Parameters:
**	%0d - base table ID (from iistatistics.stabbase)
**	%1d - index table ID (from iistatistics.stabindex)
**
** Explanation:
**	verifydb test #60 found that the table that is supposed to receive
**	these statistics doesn't exist (at least it's not listed in iirelation);
**	verifydb test #61 found that the column number receiving the statistics
**	was invalid; or, verifydb test #65 found the statistics were performed
**	on an index table.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0327_SET_OPTSTAT
"The recommended action is to indicate that\n
there are optimizer statistics for this table/view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #62 found that the iirelation entry for this table does
**	not indicate there are optimizer statistics defined for this table.
**	The recommended solution is to change iirelation.relstat (set OPTSTAT).
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0328_FIX_SNUMCELLS
"The recommended action is to correct snumcells\n
(in iistatistics) from %0d to %1d for\n
table %2c (owner %3c) attribute# %4d"
/*%
** Parameters:
**	%0d - current value of snumcells
**	%1d - actual count of cells in iihistogram
**	%2c - table name
**	%3c - table owner
**	%4d - attribute number
**  
** Explanation:
**	verifydb test #64 found a mismatch between iistatistics and
**	iihistogram in the number of cells in the histogram.  If the
**	real count is zero, the whole histogram is missing.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0329_DROP_FROM_IIHISTOGRAM
"The recommended action is to drop all tuples from iihistogram\n
with htabbase = %0d and htabindex = %1d for attribute # %2d."
/*%
** Parameters:
**	%0d - base table ID (from iihistogram.htabbase)
**	%1d - index table ID (from iihistogram.htabindex)
**	%2d - attribute number (from iihistogram.hatno)
**
** Explanation:
**  verifydb test #65 found that the statistics associated with this
**	histogram doesn't exist; or, verifydb test #66 found some of
**	the tuples that comprise the histogram are missing from iihistogram.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU032A_SET_RELSPEC_TO_HASH
"The recommended action is to set the default storage structure for table\n
%0c to HASH."
/*%
** Parameters:
**	%0c - DBMS system catalog name
**
** Explanation:
**	verifydb test #4 found a DBMS catalog with an invalid relspec;
**	the relspec for that table should be TCB_HASH.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU032B_SET_RELSPEC_TO_BTREE
"The recommended action is to set the default storage structure for table\n
%0c to BTREE."
/*%
** Parameters:
**	%0c - DBMS system catalog name
**
** Explanation:
**	verifydb test #4 found a DBMS catalog with an invalid relspec;
**	the relspec for that table should be TCB_BTREE.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU032C_EMPTY_IIHISTOGRAM
"The recommended action is to empty the iihistogram table\n
by deleting all tuples from it."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found tuples in iihistogram but iistatistics is empty.
**	The iihistogram tuples are used in conjunction with the iistatistics
**	tuples, so these tuples are meaningless and should be dropped.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	verifydb will skip the checks for each tuple in iihistogram, since the
**	iistatistics cache was not built.
**
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU032D_DROP_TABLE
"The recommended action is to drop table\n
%0c (owner %1c) from\n
database %2c."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**	%2c - database name
**
** Explanation:
**	verifydb's drop_table action will prompt the user for permission to
**	drop the table if running in INTERACTIVE mode.  Otherwise, it will
**	just go ahead with the drop. If the feature is evoked in REPORT mode,
**	verifydb will log that the table should be dropped and exit
**	without dropping it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If prompted, the user must enter Yes/No permission to drop this table.
*/
S_DU032E_DROP_USER
"The recommended action is to drop user '%0c'."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	verifydb will drop the tuple from the iiuser table.  This will
**	effectively drop the user from the installation and make any database
**	objects that the user owns orphans (since the user name has been
**	corrupted, the objects will be orphans anyway).
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If prompted, the user must enter Yes/No permission to drop this table.
**
**	It might be a good idea for the user to answer NO to all subsequent
**	verifydb prompts, go in via accessdb and assure that all users are
**	defined (and if not, define them), then re-run verifydb.
*/
S_DU032F_CLEAR_BADBITS
"The recommended action is to remove invalid privileges for user '%0c'."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	iiuser.status is a bitmap of user privileges; some of the bits set
**	in the bitmap are invalid, so verifydb wants to zero the invalid bits.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If prompted, the user must enter Yes/No permission to drop this table.
**
**	There is a good chance that the user's permissions may not be right
**	if there were invalid bits in the permission bitmap.  The system
**	administrator should run accessdb to assure that the user has the
**	correct permissions.
*/
S_DU0330_DROP_IIDATABASE_TUPLE
"The recommended action is to drop the tuple from iidatabase\n
(which effectively removes the database from the installation.)\n
If you permit this drop, run\n
     finddbs -r\n
to attempt to recover the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #69 found a tuple with an invalid database name specifier.
**	The recommended corrective action to make the database database
**	catalogs consistent is to drop information about this database from
**	the installation by removing it from the iidatabase catalog.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take recommended action.  If no is given, verifydb will
**	proceed on to next text.
**
**	NOTE: This message will also be displayed in REPORT mode, but the user
**	will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0331_CREATE_IIEXTEND_ENTRY
"The recommended action is to put a tuple in iiextend for\n
database %0c at location %1c."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - database default data location (from iidatabase.dbdev)
**
** Explanation:
**	verifydb test #72 found no iiextend entry for this database.  All
**	databases should have at least one entry in iiextend for their default
**	data location.  Additionally, databases extended via accessdb will have
**	an entry for each extended location, but we cannot test for that here.
**	verifydb will build a iiextend tuple, since it has all the information
**	that it needs.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode, then
**	the user must give Yes/No answer to the prompt.
*/
S_DU0332_CREATE_DBACCESS_ENTRY
"The recommended action is to put a tuple in iidbaccess for\n
dba %1c to access database %0c."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - database owner (from iidatabase.own)
**
** Explanation:
**	verifydb test #76 found no iidbaccess entry for the DBA of this
**	private database.  It is possible that this database should really
**	be marked public and iidatabase.access is corrupted.  However, it is
**	also possible that this database is really a private database, and
**	somehow the DBA no longer has access to it.  This fix will force the
**	database to be private and will give access to the DBA.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode, then
**	the user must give Yes/No answer to the prompt.
*/
S_DU0333_MARK_DB_OPERATIVE
"The recommended action is to mark the database as operative."
/*%
** Parameters:
**	None.
** Explanation:
**	verifydb test #77 found that the database was not marked operative,
**	but there was no reason that the database should not be marked as such.
**	However, verifydb will not take this corrective action unless it is in 
**	RUNINTERACTIVE mode.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user must give Yes/No answer to the prompt.
*/
S_DU0334_UPDATE_IIDBID_IDX
"The recommended action is to make an entry in secondary index iidbid_idx\n
with db_id = %0d and tidp = %1d."
/*%
** Parameters:
**	%0d - database ID (from iidatabase.db_id)
**	%1d - TID
**
** Explanation:
**	verifydb test #82 found that the secondary index tuple for this 
**	database was missing or incorrect.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No answer to the 
**	prompt; otherwise, no action is required.
*/
S_DU0335_DROP_IIDBIDIDX
"The recommended action is to remove the tuple from secondary index iidbid_idx."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #83 found a secondary index tuple with no corresponding
**	tuple in the base table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No answer to the 
**	prompt; otherwise, no action is required.
*/
S_DU0336_UPDATE_IIDBIDIDX
"The recommended action is to update iidbid_idx.tidp to %0d."
/*%
** Parameters:
**	%0d - TID pointer value
**
** Explanation:
**	verifydb test #84 found a secondary index tuple with the wrong TID
**	pointer value.  The TID pointer should be updated to the correct
**	value.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode, then give Yes/No permission.
*/
S_DU0337_DROP_DBACCESS
"The recommended action is to remove access to private database\n
'%1c' for user '%0c'."
/*%
** Parameters:
**	%0c - user name (from iidbaccess.usrname)
**	%1c - private database name (from iidbaccess.dbname)
**
** Explanation:
**	verifydb test #85 or #86 found an invalid entry in iidbaccess.  This
**	entry is being removed from the catalog.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode, then give Yes/No permission.
*/
S_DU0338_MARK_DB_PRIVATE
"The recommended action is to mark database '%0c'\n
as a private database."
/*%
** Parameters:
**	%0c - private database name (from iidbaccess.dbname)
**
** Explanation:
**	verifydb test #87 found a private database that was marked as a global
**	database.  verifydb knows it is a private database, because there is a
**	valid entry in table iidbaccess, giving the DBA access to the database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode, then give Yes/No permission.
*/
S_DU0339_DROP_LOCATION
"The recommended action is to drop location with area =\n
'%0c'\n
from the iilocations catalog."
/*%
** Parameters:
**	%0c - location physical name (from iilocations.area)
**
** Explanation:
**	verifydb test #88 found an invalid location logical name.
**	(See message S_DU166B_INVALID_LOCNAME for a full
**	explanation.)
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, give Yes/No permission.
**
**	After the corrective action is taken, the user should use accessdb to
**	redefine the location. NOTE: The user must use the same logical/physical
**	names that the location was previously defined at, since the location
**	information is in the config file.  If the user does not know how the
**	location should be defined, do an ASCII dump on the config file to
**	find out.  The config file resides in the database's default data
**	location and is file aaaaaaaa.cnf.
*/
S_DU033A_FIX_LOC_STAT
"The recommended action is to clear invalid bits in location status."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #89 found invalid bits in the location status (used to
**	define what the location may be used for).
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.  After the corrective action is taken, the user
**	should use accessdb to assure that the location is defined for the
**	correct uses.
*/
S_DU033B_DROP_IIEXTEND
"The recommended action is to drop this tuple from iiextend."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #90 or #91 found an iiextend tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.  After the corrective action is taken, the user
**	should use accessdb to assure that the database is correctly defined.
*/
S_DU033C_DROP_IISYNONYM
"The recommended action is to drop this tuple from iisynonym."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #92 or #93 found an invalid iisynonym tuple.  The test
**	will have already printed a message describing why the tuple is
**	invalid.  Next, this message is printed as the recommended corrective
**	action.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.
*/
S_DU033D_CLEAR_TCBINDEX
"The recommended action is to clear the iirelation.relstat.TCB_INDEX bit."
/*%  S_DU033C_CLEAR_TCBINDEX
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #16a found a tuple with reltidx=0 but relstat.TCB_INDEX
**	set.  Since the table's index_id is zero, this can't really be a
**	secondary index.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.
*/
S_DU033E_SET_RULES
"The recommended action is to set the RULE bit in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the TCB_RULE bit should have been set in
**	iirelation.relstat (for this table) but was not.
**    
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
**
**	NOTE: The user will usually want to enter yes.  The query text for this
**	integrity can be retrieved from iiqrytext, and the DBMS can rebuild the 
**	integrity if the query text is rekeyed.
*/
S_DU033F_REMOVE_STARCDBS
"The recommended action is to delete the iistar_cdbs entry with\n
DDB name ='%0c'\n
CDB name ='%1c'"
/*%
** Parameters:
**	%0c - distributed database name
**	%1c - coordinator database name
**
** Explanation:
**	verifydb will remove a tuple from the table that associates a
**	coordinator database and a distributed database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.
*/
S_DU0340_UPDATE_STARCDBS
"The corrective action is to update catalog iistar_cdbs with the correct value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Update one or more fields in catalog iistar_cdbs with the correct value.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.
*/
S_DU0341_WARN_IICDBID_IDX
"WARNING: secondary index iicdbid_idx out of synch with base table iistar_cdbs.\n
Recommend you drop index and recreate as:\n
\t'create index iicdbid_idx on iistar_cdbs(cdb_id)'"
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb iicdbid_idx checks found an inconsistency between the secondary
**	index and the base table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0342_CLEAR_RULE
"Recommended action is to clear RULE information from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #27 found that iirelation specified there are rules
**	on this table, but none are defined in iirules.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0343_DROP_EVENT
"Recommended action is to drop this event from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an event with no corresponding query text or with a
**	non-unique identifier.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0344_DROP_DBP
"Recommended action is to drop this database procedure from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a dbp with no corresponding query text or with a
**	non-unique identifier.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0345_DROP_FROM_IIRULE
"The recommended action is to drop rule %0c (owned by %1c) from this database."
/*%
** Parameters:
**	%0c - rule name
**	%1c - rule owner
**
** Explanation:
**	verifydb found that there is a problem with this rule, such as the table
**	that is supposed to receive this rule doesn't exist.  So, remove this
**	tuple from iirule to drop the rule from the database.
**
** System Status:
**	This is an informational message; processing continues.  If the user
**	is in RUNINTERACTIVE mode, then input will be required from the user.
**	In this case, the S_DU0300_PROMPT message will be displayed following
**	this message, and processing will simply wait until some input is given.
**	If the input is not a valid response, the message will be repeated until
**	a valid response is given.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU0346_SET_RULE
"The recommended action is to set the RULE bit in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found that the RULE bit should have been set in
**	iirelation.relstat (for this table) but was not.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU0347_CLEAR_DEFLT_GRP
"The recommended action is to clear the default group for user '%0c'."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	verifydb found a default group that does not exist, so it wants to
**	remove the default group by setting it to the same value as though it
**	had never been initialized.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU0348_DROP_USR_FROM_GRP
"The recommended action is drop invalid user from specified user group."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a default group that contains a non-existent user.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU0349_DROP_FROM_IIDBPRIV
"The recommended action is drop privileges for this database/user."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an invalid user/database pair in iidbpriv.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU034A_CLEAR_INVALID_PRIV
"The recommended action is to clear invalid privileges for\n
user '%0c' to database '%1c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - database name
**
** Explanation:
**	verifydb found invalid bits set in iidbpriv.flags.  It clears all bits
**	that are not also set in iidbpriv.control.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	None, unless verifydb is being run in RUNINTERACTIVE mode. In that
**	case, enter Y(ES) or N(O).
*/
S_DU034B_DELETE_IIDEFAULT
"the recommended action is to delete the default tuple having\n
id1 %0d and id2 %1d from iidefault."
/*%
** Parameters:
**	%0d - default id1 
**	%1d - default id2
**
** Explanation:
**	verifydb test in ckdefault() found a default tuple with no reference
**	to any attribute. An informational message will have already been give
**	to the user telling why the default tuple is invalid.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If running in RUNINTERACTIVE mode, the user should answer yes/no.
*/
S_DU034C_SET_INDEX
"The recommended action is to set the bit in iirelation.relstat, indicating\n
that this catalog is a secondary index."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a secondary index which did not have the TCB_INDEX bit
**	set in iirelation.relstat.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If running in RUNINTERACTIVE mode, the user should answer yes/no.
*/
S_DU034D_DROP_IIGW06_RELATION
"The recommended action is to drop the registration from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an entry in iigw06_relation that does not have a
**	corresponding entry in iirelation.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If running in RUNINTERACTIVE mode, the user should answer yes/no.
*/
S_DU034E_DROP_IIGW06_ATTRIBUTE
"The recommended action is to drop the attribute from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an entry in iigw06_attribute that does not have a
**	corresponding entry in iigw06_relation; i.e., this attribute does
**	not belong to any security audit gateway table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If running in RUNINTERACTIVE mode, the user should answer yes/no.
*/
S_DU034F_DROP_IIDBMS_COMMENT
"The recommended action is to delete all tuples with\n
comtabbase = %0d, comtabidx = %1d from iidbms_comments."
/*%
** Parameters:
**	%0d - iidbms_comment.comtabbase
**	%1d - iidbms_comment.comtabidx
**
** Explanation:
**	verifydb found an entry in iidbms_comments that describes a
**	view or table with a comment.  However, that table/view does not
**	exist (or is not defined in iirelation).
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0351_DROP_IIRELATION_TUPLE
"VERIFYDB dropped this tuple from iirelation\n
(which effectively drops table %0c (owner %1c) from the database)"
/*%
** Parameters:
**	%0c -- table name (from iirelation.relid)
**	%1c -- table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #1 found a tuple with an invalid table specifier.
**	Since the table specifier is used to join iirelation tuples to tuples
**	in other system catalogs, this tuple is useless.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0352_DROP_TABLE
"table %0c (owner %1c)\n
dropped from the database."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	A verifydb test found a condition (which it reported) that leaves
**	the system catalogs inconsistent.  The recommended corrective action
**	to make the system catalogs consistent is to drop this table from
**	the database via an SQL drop.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0353_REPLACE_RELWID
"the value in iirelation of (%0d) replaced with (%1d)"
/*%
** Parameters:
**	%0d - table width from iirelation (iirelation.relwid)
**	%1d - sum of width of each attribute (from each iiattribute.attfrml)
**
** Explanation:
**	verifydb test #2b found a mismatch in table width between iirelation
**	and the sum of each column's width.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0354_REPLACE_RELKEYS
"value in iirelation of (%0d) replaced with (%1d)"
/*%
** Parameters:
**	%0d - number of keys iirelation thinks there are (iirelation.relkeys)
**	%1d - count of each key in iiattribute
**
** Explanation:
**	verifydb test #3 found a mismatch between the number of keys that should
**	be in the table's primary key and the number that are actually in the
**	key.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0355_CLEAR_PRTUPS
"protection information cleared from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #8 found that iirelation specified there are protections
**	on this table, but none are defined in iiprotect.
**
** System Status:
**	This is an informational message; processing continues.
** Recommendation:
**	None.
*/
S_DU0356_CLEAR_INTEGS
"integrity information cleared from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #9 found that iirelation specified there are integrities
**	on this table, but none are defined in iiintegrity.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0357_CLEAR_CONCUR
"CONCUR locking specification cleared from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #10 found that the CONCUR bit was set for a table that
**	was not a DBMS core system catalog.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0358_SET_CONCUR
"CONCUR locking specification set in iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #11 found that one of the DBMS core system catalogs did
**	not have the special locking (CONCUR) bit set in iirelation.relstat.
**
**	The user will be prompted to give yes/no permission.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0359_MAKE_TABLE
"iirelation altered to indicate that this is a table, not a view"
/*%
** Parameters:
**	None.
**
** Explanation:
**      verifydb test #12 found that there was a physical disk file associated
**      with this view.  This often indicates that it's really a table that
**      has been mismarked as a view.  However, it is possible that this is
**      really a view and someone has renamed files or invalidated a backup
**      somehow.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU035A_CLEAR_CATALOG
"iirelation altered to stop indicating that this table is a system catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #5a found that a table was mismarked as a system
**	catalog when it did not start with "ii" or when the owner was not
**	the system catalog owner.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU035B_DROP_VIEW
"view %0c (owner %1c)\n
dropped from the database."
/*%
** Parameters:
**	%0c - view name (null terminated string)
**	%1c - view owner (null terminated string)
**
** Explanation:
**	verifydb test #12 found that the specified view was not defined in
**	iitree.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU035C_CLEAR_VBASE
"view base specification cleared from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #15 found that iirelation believed there was a view
**	based on this table, but none could be found in iidbdepends.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU035D_SET_IDXD
"iirelation.relstat INDEXED bit set\n
for base table %0c (owner %1c)"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	The IDXD bit in iirelation.relstat for the base table indicates that
**	there is a secondary index table for this table.  verifydb will 
**	set this bit automatically, unless verifydb is in RUNINTERACTIVE mode.
**	This is the correction for verifydb test #17 and #30.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU035E_FIX_RELIDXCOUNT
"iirelation.relidxcount set to %0d (from %1d)"
/*%
** Parameters:
**	%0d - number of secondary indexes defined in iiindexes
**	%1d - number of secondary indexes iirelation thinks there are
**	      (from iirelation.relidxcount)
**
** Explanation:
**	verifydb test #18 found a discrepancy between the number of secondary
**	indexes that iirelation thinks there are and the number of secondary
**	indexes defined in iiindex.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU035F_CLEAR_RELIDXCOUNT
"iirelation modified to indicate that\n
there are no secondary indexes for table\n
%0c (owner %1c)"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #18 found that iirelation thinks there are secondary
**	indexes defined on this table but there aren't any.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0360_CLEAR_OPTSTAT
"iirelation modified to stop indicating that there are optimizer statistics\n
for this table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #19 found that iirelation thinks there are optimizer
**	statistics for this table, but there aren't any.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	none.
*/
S_DU0361_RESET_RELLOCOUNT
"number of locations in iirelation modified from %0d to %1d."
/*%
** Parameters:
**	%0d - number of locations iirelation thinks there are
**	      (from iirelation.relloccount)
**	%1d - number of locations defined in iidevices
**
** Explanation:
**	verifydb test #20 found a discrepancy between the number of locations
**	that iirelation thinks there are and the number of locations defined
**	in iidevices.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0362_FIX_RELDFILL
"invalid data fill factor replaced with the default data fill factor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The data page fill factor must be specified for all table types.  If
**	it is not, then a default value will be used.  This is detected by
**	verifydb tests #22 through #25.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0363_FIX_RELLFILL
"invalid leaf fill factor replaced with the default leaf fill factor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The leaf page fill factor must be specified for all table types.  If
**	it is not, then a default value will be used.  This is detected by
**	verifydb test #25, and applies only to Btrees.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	Enter Y(ES) or N(O).
*/
S_DU0364_FIX_RELIFILL
"invalid index fill factor replaced with the default index fill factor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The index page fill factor must be specified for all table types.  If
**	it is not, then a default value will be used.  This is detected by
**	verifydb test #25, and applies only to Btrees.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0365_CREATE_IIRELIDX
"iirel_idx tuple created."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #26 found a tuple in iirelation that does not have a
**	corresponding tuple in iirel_idx.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0366_CLEAR_IIRELIDX
"tuple dropped from system catalog iirel_idx."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #27 found an entry in iirel_idx that does not have
**	a corresponding entry in iirelation.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0367_DROP_ATTRIBUTES
"tuples with attrelid = %0d, attrelidx = %1d\n
deleted from iiattributes."
/*%
** Parameters:
**	%0c - attrelid from iiattributes
**	%1c - attrelidx from iiattributes
**
** Explanation:
**	verifydb test #28 found some tuples in iiattributes that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	This means that they are useless, and should be deleted.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0368_DROP_INDEX
"this tuple (with baseid = %0d, indexid = %1d)\n
deleted from iiindex."
/*%
** Parameters:
**	%0d - baseid from iiindex
**	%1d - indexid from iiindex
**
** Explanation:
**	verifydb test #30 found some tuples in iiindex that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	this means that they are useless, and should be deleted.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0369_SET_INDEX
"iirelation modified to indicate that this is an index table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #31 found that this table is a secondary index table, but
**	is not marked as such in iirelation.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU036A_DROP_INDEX
"tuples with devrelid = %0d, devrelidx = %1d\n
deleted from iidevices."
/*%
** Parameters:
**	%0d - devrelid from iidevices
**	%1d - devrelidx from iidevices
**
** Explanation:
**	verifydb test #33 found some tuples in iidevices that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	this means that they are useless, and should be deleted.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU036B_SET_MULTIPLE_LOC
"iirelation modified to indicate that this table has multiple\n
locations defined."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #34 found that iirelation did not have the MULTIPLE_LOC
**	bit set in relstat, but there are actually locations defined for this
**	table in iidevices.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
**/
S_DU036C_DEL_TREE
"tuples with base table id = %0d, index table id = %1d\n
tree id = %2d %3d deleted from iitree."
/*%
** Parameters:
**	%0d - base table ID associated with tree (from iitree.treetabbase)
**	%1d - base table index ID associated with tree (from iitree.treetabidx)
**	%2d - high portion of tree ID (iitree.treeid1)
**	%3d - low portion of tree ID (iitree.treeid2)
**
** Explanation:
**	verifydb test #35 found tuples in iitree with an invalid tree mode.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU036D_SET_VIEW
"iirelation.relstat VIEW bit flag set."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the VIEW bit should have been set in
**	iirelation.relstat (for this view) but was not.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU036E_SET_PRTUPS
"iirelation.relstat PRTUPS bit flag set."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the PRTUPS bit should have been set in
**	iirelation.relstat (for this table) but was not.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU036F_SET_INTEGS
"iirelation.relstat INTEGS bit flag set."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the INTEGS bit should have been set in
**	iirelation.relstat (for this table) but was not.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0370_DROP_FROM_IIINTEGRITY
"tuples with inttabbase = %0d and inttabidx = %1d\n
dropped from iiintegrity"
/*%
** Parameters:
**	%0d - base table ID (from iiintegrity.inttabbase)
**	%1d - index table ID (from iiintegrity.inttabidx)
**
** Explanation:
**	verifydb test #40 found that the table that is supposed to receive
**	this integrity doesn't exist (at least it's not listed in iirelation);
**	or, verifydb test #42 found the tree that defines the integrity is
**	missing from iitree, which means the DBMS cannot enforce the integrity.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0371_DROP_FROM_IIPROTECT
"tuples with protabbase = %0d and protabidx = %1d dropped from iiprotect"
/*%
** Parameters:
**	%0d - base table id (from iiprotect.protabbase)
**	%1d - index table id (from iiprotect.protabidx)
**
** Explanation:
**	verifydb test #44 found that the table that is supposed to receive
**	this integrity doesn't exist (at least it's not listed in iirelation);
**	or, verifydb test #46 found the tree that defines the permit is missing
**	from iitree, which means the DBMS cannot enforce the permit.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0372_DROP_IIDBDEPENDS_TUPLE
"tuple with inid1 = %0d, inid2 = %1d, deid1 = %2d and deid2 = %3d\n
deleted from iidbdepends."
/*%
** Parameters:
**	%0d - iidbdepends.inid1
**	%1d - iidbdepends.inid2
**	%2d - iidbdepends.deid1
**	%3d - iidbdepends.deid2
**
** Explanation:
**	verifydb test #54 found an entry in iidbdepends that describes a
**	view or table with permit as dependent on another table.  However,
**	the dependent table/view does not exist (or is not defined in 
**	iirelation). Or, verifydb test #55 found that the independent table
**	does not exist, so the dependency described by this entry in
**	iidbdepends is not valid.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0373_SET_VBASE
"iirelation modified to indicate that a view is defined against\n
this table/view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #56 found that the independent table does not indicate
**	that there is a view defined against this table/view.  The recommended
**	solution is to change iirelation.relstat (set VBASE) for the 
**	independent table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0374_CREATE_IIXDBDEPENDS
"iixdbdepends tuple created."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #58 found a tuple in iidbdepends that does not have a
**	corresponding tuple in iixdbdepends.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0375_REDO_IIXDBDEPENDS
"iixdbdepends dropped and recreated."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #59 found an entry in iixdbdepends that does not have
**	a corresponding entry in iidbdepends.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0376_DROP_FROM_IISTATISTICS
"tuples with stabbase = %0d, stabindex = %1d dropped from iistatistics"
/*%
** Parameters:
**	%0d - base table ID (from iistatistics.stabbase)
**	%1d - index table ID (from iistatistics.stabindex)
**
** Explanation:
**	verifydb test #60 found that the table that is supposed to receive
**	these statistics doesn't exist (at least it's not listed in iirelation);
**	verifydb test #63 found that the column number receiving the statistics
**	was invalid; or, verifydb test #61 found the statistics were performed
**	on an index table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0377_SET_OPTSTAT
"iirelation.relstat modified to indicate that\n
there are optimizer statistics for this table/view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #62 found that the iirelation entry for this table does
**	not indicate there are optimizer statistics defined for this table.
**	The recommended	solution is to change iirelation.relstat (set OPTSTAT) .
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0378_FIX_SNUMCELLS
"snumcells (in iistatistics)\n
corrected from %0d to %1d for\n
table %2c (owner %3c) attribute# %4d"
/*%
** Parameters:
**	%0d - current value of snumcells
**	%1d - actual count of cells in iihistogram
**	%2c - table name
**	%3c - table owner
**	%4d - attribute number
**	
** Explanation:
**	verifydb test #64 found a mismatch in the number of tuples in
**	the histogram for this attribute.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0379_DROP_FROM_IIHISTOGRAM
"tuples with htabbase = %0d, htabindex = %1d dropped from iihistogram\n
 for attribute #%2d."
/*%
** Parameters:
**	%0d - base table ID (from iihistogram.htabbase)
**	%1d - index table ID (from iihistogram.htabindex)
**	%2d - attribute number (from iihistogram.hatno)
**
** Explanation:
**	verifydb test #65 found that the statistics associated with this
**	histogram doesn't exist; or, verifydb test #66 found some of the
**	tuples that comprise the histogram are missing from iihistogram.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU037A_SET_RELSPEC_TO_HASH
"The default storage structure for table\n
%0c set to HASH."
/*%
** Parameters:
**	%0c - DBMS system catalog name
**
** Explanation:
**	verifydb test #4 found a DBMS catalog with an invalid relspec;
**	the relspec for that table should be TCB_HASH.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU037B_SET_RELSPEC_TO_BTREE
"The the default storage structure for table\n
%0c set to BTREE."
/*%
** Parameters:
**	%0c - DBMS system catalog name
**
** Explanation:
**	verifydb test #4 found a DBMS catalog with an invalid relspec;
**	the relspec for that table should be TCB_BTREE.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU037C_EMPTY_IIHISTOGRAM
"All tuples cleared from iihistogram."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found tuples in iihistogram, but iistatistics is empty.
**	The iihistogram tuples are used in conjunction with the iistatistics
**	tuples, so these tuples are meaningless and should be dropped.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU037D_DROP_TABLE
"Table %0c (owner %1c)\n
dropped from database %2c."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**	%2c - database name
**
** Explanation:
**      verifydb's drop_table action will prompt the user for permission to
**      drop the table if running in RUNINTERACTIVE mode. Otherwise, it will
**      just go ahead with the drop. If the feature is evoked in REPORT mode,
**      verifydb will log that the table should be dropped and exit
**      without dropping it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If prompted, the user must enter Yes/No permission to drop this table.
**	Otherwise, no action is required.
*/
S_DU037E_DROP_USER
"User '%0c' dropped from installation."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	verifydb deleted the tuple from the iiuser table.  This effectively
**	dropped the user from the installation (and made any database objects
**	that user owns orphans; however, since the user name has been corrupted,
**	the objects will be orphans anyway).
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If the user sees this message, they should use accessdb to add anyone
**	back who was dropped and should not have been.
*/
S_DU037F_CLEAR_BADBITS
"invalid privileges for user '%0c' were removed."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	iiuser.status is a bitmap of user privileges.  Some of the bits set
**	in the bitmap are invalid; verifydb zeroed the invalid bits.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	There is a good chance that the user's permissions may not be right
**	if there were invalid bits in the permission bitmap.  The system
**	administrator should run accessdb to assure that the user has the
**	correct permissions.
*/
S_DU0380_DROP_IIDATABASE_TUPLE
"The tuple has been dropped from iidatabase\n
(which effectively removeed the database from the installation.)"
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #69 found a tuple with an invalid database name specifier.
**	verifydb removed the tuple from the iidatabase table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user should run finddbs to attempt to recover the database.
*/
S_DU0381_CREATE_IIEXTEND_ENTRY
"iiextend tuple build with\n
dname = %0c, lname= %1c."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - default data location (from iidatabase.dbdev)
**
** Explanation:
**	verifydb test #72 found no iiextend entry for this database.  All
**	databases should have at least one entry in iiextend for their default
**	data location.  Additionally, databases extended via accessdb will have
**	an entry for each extended location, but we cannot test for that here.
**	verifydb will build a iiextend tuple, since it has all the information
**	it needs.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0382_CREATE_DBACCESS_ENTRY
"DBA %0c now has access to\n
private database %1c."
/*%
** Parameters:
**	%0c - database owner (from iidatabase.own)
**	%1c - database name (from iidatabase.name)
**
** Explanation:
**	verifydb test #76 found no iidbaccess entry for the DBA of this
**	private database.  It is possible that this database should really
**	be marked public and iidatabase.access is corrupted.  However, it is
**	also possible that this database is really a private database, and
**	somehow the DBA no longer has access to it.  verifydb has now forced
**	the database to be private, and has given access to the DBA.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user may wish to use accessdb to give other users access
**	to this database.
*/
S_DU0383_MARK_DB_OPERATIVE
"The database was marked as operative."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #77 found that the database was not marked operative,
**	but there was no reason that the database should not be marked as such.
**	So, verifydb marked the database operative.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0384_UPDATE_IIDBID_IDX
"secondary index iidbid_idx entry created\n
with db_id = %0d and tidp = %1d."
/*%
** Parameters:
**	%0d - database ID (from iidatabase.db_id)
**	%1d - TID
**
** Explanation:
**	verifydb test #82 found that the secondary index tuple for this 
**	database was missing or incorrect.  The fix is to update iidatabase's
**	secondary index table by placing a valid entry for this database in it.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give a Yes/No answer to the 
**	verifydb prompt.  Otherwise, no action is required.
*/
S_DU0385_DROP_IIDBIDIDX
"Tuple dropped from secondary index iidbid_idx."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #83 found a secondary index tuple with no corresponding
**	tuple in the base table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0386_UPDATE_IIDBIDIDX
"iidbid_idx.tidp updated to %0d."
/*%
** Parameters:
**	%0d - TID pointer value
**
** Explanation:
**	verifydb test #84 found a secondary index tuple with the wrong
**	TID pointer value.  The TID pointer has been updated to the correct
**	value.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0387_DROP_DBACCESS
"Access to private database\n
'%1c' for user '%0c' is removed."
/*%
** Parameters:
**	%0c - user name (from iidbaccess.usrname)
**	%1c - private database name (from iidbaccess.dbname)
**
** Explanation:
**	verifydb test #85 or #86 found an invalid entry in iidbaccess.  This
**	entry is being removed from the catalog.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0388_MARK_DB_PRIVATE
"Private database '%0c'\n
now marked as a private database."
/*%
** Parameters:
**	%0c - private database name (from iidbaccess.dbname)
**
** Explanation:
**	verifydb test #87 found a private database that was marked as a global
**	database.  verifydb knows it is a private database, because there is a
**	valid entry in table iidbaccess, giving the DBA access to the database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0389_DROP_LOCATION
"location dropped from the iilocations catalog.\n
Use ACCESSDB to redefine the location for area =
'%0c'."
/*%
** Parameters:
**	%0c - location physical name (from iilocations.area)
**
** Explanation:
**	verifydb test #88 found an invalid location logical name.
**	(See message S_DU166B_INVALID_LOCNAME for a full
**	explanation.)
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	After the corrective action is taken, the user should use accessdb to
**	redefine the location. NOTE: The user must use the same logical/physical
**	names that the location was previously defined at, since the location
**	information is in the config file.  If the user does not know how the
**	location should be defined, do an ASCII dump on the config file to
**	find out.  The config file resides in the database's default data
**	location and is file aaaaaaaa.cnf.
*/
S_DU038A_FIX_LOC_STAT
"Invalid bits cleared from location status word."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #89 found invalid bits in the location status (used to
**	define what the location may be used for).
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	After the corrective action is taken, the user should use accessdb to
**	assure that the location is defined for the correct uses.
*/
S_DU038B_DROP_IIEXTEND
"The tuple is removed from iiextend."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #90 or #91 found an iiextend tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	After the corrective action is taken, the user should use accessdb to
**	assure that the database is correctly defined.
*/
S_DU038C_DROP_IISYNONYM
"The tuple is removed from iisynonym."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #92 or #93 found an invalid iisynonym tuple. verifydb has
**	taken the recommended corrective action and is notifying the user that
**	it has deleted the tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user may want to correctly redefine the synonym that was dropped.
**	No user action is required.
*/
S_DU038D_CLEAR_TCBINDEX
"The iirelation.relstat.TCB_INDEX bit is cleared."
/*%  S_DU033C_CLEAR_TCBINDEX
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #16a found a tuple with reltidx=0 but relstat.TCB_INDEX
**	set.  Since the table's index_id is zero, this can't really be a
**	secondary index.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU038E_SET_RULES
"The RULE bit has been set in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #38 found that the TCB_RULE bit should have been set in
**	iirelation.relstat (for this table) but was not.
**	  
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message requires input from the user. The S_DU0300_PROMPT
**	message will be displayed following this message, and processing will
**	simply wait until some input is given.  If the input is not a valid
**	response, the message will be repeated until a valid response is given.
**
** Recommendation:
**	None.
*/
S_DU038F_REMOVE_STARCDBS
"Verifydb deleted the iistar_cdbs entry with DDB name ='%0c'\n
CDB name ='%1c'"
/*%
** Parameters:
**	%0c - distributed database name
**	%1c - coordinator database name
**
** Explanation:
**	verifydb will remove a tuple from the table that associates a
**	coordinator database and a distributed database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer prompt with Yes/No.
*/
S_DU0390_UPDATE_STARCDBS
"VERIFYDB has updated catalog iistar_cdbs with the correct value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Update one or more fields in catalog iistar_cdbs with the correct value.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must answer the prompt with
**	either yes or no.
*/
S_DU0392_CLEAR_RULE
"rule information cleared from iirelation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #27 found that iirelation specified there are rules
**	on this table, but none are defined in iirules.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0393_DROP_EVENT
"Verifydb dropped this event from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an event with no corresponding query text or with a
**	non-unique identifier.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0394_DROP_DBP
"Verifydb dropped this Database Procedure from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a dbp with no corresponding query text or with a
**	non-unique identifier.
**
**	The user will be prompted to give yes/no permission.  If yes is given,
**	verifydb will take the recommended action.  If no is given, verifydb
**	will proceed to the next test.
**
**	NOTE: This message will also be displayed in REPORT mode, but the
**	user will not be given the opportunity to input yes/no permission.
** 
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0395_DROP_FROM_IIRULE
"Verifydb dropped drop rule %0c (owned by %1c) from this database."
/*%
** Parameters:
**	%0c - rule name
**	%1c - rule owner
**
** Explanation:
**	verifydb found that there is a problem with this rule (e.g., the table
**	that is supposed to receive this rule doesn't exist), so remove this
**	tuple from iirule to drop the rule from the database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0396_SET_RULE
"Verifydb set the the RULE bit in iirelation.relstat."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found that the RULE bit should have been set in
**	iirelation.relstat (for this table) but was not.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0397_CLEAR_DEFLT_GRP
"Verifydb cleared the default group for user '%0c'."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	verifydb found a default group that does not exist, so it wants to
**	remove the default group by setting it to the same value as though it
**	had never been initialized.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0398_DROP_USR_FROM_GRP
"Verifydb dropped invalid user from specified user group."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a default group that contains a non-existent user.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0399_DROP_FROM_IIDBPRIV
"Verifydb dropped privileges for this database/user."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an invalid user/database pair in iidbpriv.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU039A_CLEAR_INVALID_PRIV
"Verifydb cleared invalid privileges for user '%0c' to database '%1c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - database name
**
** Explanation:
**	verifydb found invalid bits set in iidbpriv.flags.  It clears all bits
**	that are not also set in iidbpriv.control.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU039B_DELETE_IIDEFAULT
"Verifydb deleted the default tuple having\n
id1 %0d and id2 %1d from iidefault."
/*%
** Parameters:
**	%0d - default id1 
**	%1d - default id2
**
** Explanation:
**	verifydb test in ckdefault() found a default tuple with no reference
**	to any attribute. An informational message will have already been given
**	to the user telling why the default tuple is invalid.
**
** System Status:
**	This is an informational message only; processing continues.
**
** Recommendation:
**	None.
*/

S_DU039C_SET_INDEX
"Verifydb set the iirelation.relstat bit to indicate that this catalog is a\n
secondary index."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a secondary index which did not have the TCB_INDEX bit
**	set in iirelation.relstat.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	If running in interactive mode, the user should answer yes/no.
*/
S_DU039D_DROP_IIGW06_RELATION
"Verifydb removed this registration from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a tuple in iigw06_relation that did not have a
**	corresponding entry in either iirelation or iigw06_attribute or had a
**	non-zero reltidx value, and it removed that tuple and all related 
**	information regarding the registration from the database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU039E_DROP_IIGW06_ATTRIBUTE
"Verifydb removed this security audit gateway attribute from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a tuple in iigw06_attribute that did not have a
**	corresponding entry in either iigw06_relation or iiattribute or had a
**	non-zero reltidx value, and it removed that tuple and all related 
**	information regarding the attribute from the database.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU039F_DROP_IIDBMS_COMMENT
"Tuples with comtabbase = %0d, comtabidx = %1d deleted from iidbms_comments."
/*%
** Parameters:
**	%0d - iidbms_comment.comtabbase
**	%1d - iidbms_comment.comtabidx
**
** Explanation:
**	verifydb found stale comments where the underlying table/view no longer
**	existed, so it deleted the comment from iidbms_comment.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0400_DEL_TEMP_FILE
"The recommended action is to delete temporary file\n
%0c from location %1c"
/*%
** Parameters:
**	%0c - temporary file name
**	%1c - temporary file location
**
** Explanation:
**	verifydb's purge operation found a temporary or non-DBMS file in
**	a database directory.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message may require input from the user.  The
**	S_DU0300_PROMPT message will be displayed following this message,
**	and processing will simply wait until some input is given.  If the
**	input is not a valid response, the message will be repeated until a
**	valid response is given.
**
** Recommendation:
**	In RUNINTERACTIVE mode, the user must answer the Yes/No prompt.
**	If the user does not want to delete this file, s/he should move it to
**	some location other than a database directory.
*/
S_DU0401_DROP_EXPIRED_RELATION
"The recommended action is to destroy expired table\n
%0c (owner %1c)."
/*%
** Parameters:
**	%0c - expired table name
**	%1c - expired table owner
**
** Explanation:
**	verifydb's purge operation found an expired relation in the database.
**
** System Status:
**	This is an informational message; processing continues.  However,
**	this type of message may require input from the user.  The
**	S_DU0300_PROMPT message will be displayed following this message,
**	and processing will simply wait until some input is given.  If the
**	input is not a valid response, the message will be repeated until a
**	valid response is given.
**
** Recommendation:
**	In RUNINTERACTIVE mode, the user must answer the Yes/No prompt.
**	If user does not want to destroy this table, they should change the
**	expiration date in iirelation.relsave.  If they set it to zero, then
**	the relation becomes permanent, or cannot expire.
*/
S_DU0410_DEL_TEMP_FILE
"Temporary file %0c\n
deleted from location %1c."
/*%
** Parameters:
**	%0c - temporary file name
**	%1c - temporary file location
**
** Explanation:
**	verifydb's purge operation found a temporary or non-DBMS file in
**	a database directory and deleted it.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0411_DROP_EXPIRED_RELATION
"Expired table %0c\n
owner %1c) is destroyed."
/*%
** Parameters:
**	%0c - expired table name
**	%1c - expired table owner
**
** Explanation:
**	verifydb's purge operation found an expired relation in the database and
**	destroyed it.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0420_DROP_INDEX
"The recommended action is to drop the secondary index."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb's TABLE or XTABLE in PATCH mode (RUN, RUNINTERACTIVE, or
**	RUNSILENT) cannot allow secondary indexes to exist on the target
**	table. The patch operation may cause the secondary index to get out
**	of sync with the base table, which would create an inconsistent
**	condition.
**
**	Therefore, verifydb deletes the secondary indexes, and it is the
**	user's responsibility to recreate them after they are satisfied with
**	the patch operation.
**
**	This message indicates that verifydb has detected the secondary index.
**	In RUN or RUNSILENT mode, verifydb will automatically drop it.  In
**	RUNINTERACTIVE mode, verifydb will prompt the user for permission to
**	drop the table.
**
** System Status:
**	verifydb will continue running.  However, if verifydb is in
**	RUNINTERACTIVE mode and the user withholds permission, verifydb
**	processing will terminate.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must enter YES/NO permission
**	in response to the prompt.
*/
S_DU0421_DROP_INDEX
"The secondary index has been dropped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb's TABLE or XTABLE in PATCH mode (RUN, RUNINTERACTIVE, or
**	RUNSILENT) cannot allow secondary indexes to exist on the target
**	table. The patch operation may cause the secondary index to get out
**	of sync with the base table, which would create an inconsistent
**	condition.
**
**	Therefore, verifydb deletes the secondary indexes, and it is the
**	user's responsibility to recreate them after they are satisfied with
**	the patch operation.
**
**	This message indicates that verifydb has dropped the secondary index.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0422_DROP_UNIQUE_CONSTRAINT
"The recommended action is to drop the unique constraint."
S_DU0423_DROP_UNIQUE_CONSTRAINT
"The unique constraint has been dropped."
S_DU0424_DROP_CHECK_CONSTRAINT
"The recommended action is to drop the check constraint."
S_DU0425_DROP_CHECK_CONSTRAINT
"The check constraint has been dropped."
S_DU0426_DROP_REF_CONSTRAINT
"The recommended action is to drop the referential constraint."
S_DU0427_DROP_REF_CONSTRAINT
"The referential constraint has been dropped."
S_DU0428_DROP_PRIMARY_CONSTRAINT
"The recommended action is to drop the primary constraint."
S_DU0429_DROP_PRIMARY_CONSTRAINT
"The primary constraint has been dropped."
S_DU0430_INSERT_CAP
"The recommended action is to insert capability '%0c' (value '%1c') into\n
iiddb_ldb_dbcaps"
/*%
** Parameters:
**	%0c - capability name
**	%1c - capability value
**
** Explanation:
**	verifydb's REFRESH_LDBS operation wants to insert a new capability that
**	is found in the LDB into the catalog that Star uses to keep track of
**	LDBMS capabilities.
**
** System Status:
**	verifydb will continue running.  However, if verifydb is in
**	RUNINTERACTIVE mode and the user withholds permission, verifydb
**	processing will not perform this action.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must enter YES/NO permission
**	in response to the prompt.
*/
S_DU0431_CAP_INSERTED
"Capability '%0c' (value '%1c') inserted into iiddb_ldb_dbcaps"
/*%
** Parameters:
**	%0c - capability name
**	%1c - capability value
**
** Explanation:
**	verifydb's REFRESH_LDBS operation wants to insert a new capability that
**	is found in the LDB into the catalog that Star uses to keep track of
**	LDBMS capabilities.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.  This is an informational message.
*/
S_DU0432_UPDATE_CAP
"The recommended action is to update iiddb_ldb_dbcaps capability '%0c' to '%1c'"
/*%
** Parameters:
**	%0c - capability name
**	%1c - capability value
**
** Explanation:
**	verifydb's REFRESH_LDBS operation wants to update the value of a
**	capability in the catalog that Star uses to keep track of LDBMS
**	capabilities, because the value of that capability in the LDBMS has
**	changed.
**
** System Status:
**	The verifydb process continues.  However, if verifydb is in
**	RUNINTERACTIVE mode and the user withholds permission, verifydb
**	processing will not perform this action.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must enter YES/NO permission
**	in response to the prompt.
*/
S_DU0433_CAP_UPDATED
"iiddb_ldb_dbcaps capability '%0c' is updated to '%1c'"
/*%
** Parameters:
**	%0c - capability name
**	%1c - capability value
**
** Explanation:
**	verifydb's REFRESH_LDBS operation wants to update the value of a
**	capability in the catalog that Star uses to keep track of LDBMS
**	capabilities, because the value of that capability in the LDBMS has
**	changed.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.  This is an informational message.
*/
S_DU0440_SET_COMMENT
"Verifydb indicated that a comment is defined for this table/view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found that the table does not indicate that there is a comment
**	defined for this table/view.  The recommended solution is to change
**	iirelation.relstat (set COMMENT) for the table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0441_DROP_THIS_COMMENT
"Offending comment was deleted from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found a corrupted comment and wants to drop it from the
**	database.
**
** System status
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0442_SET_DELIM_MIXED
"Verifydb set Mixed Case for Delimited Identifiers for this DB."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found illegal delimited identifier semantics.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user may want to check that correct semantics were set by
**	visually inspecting the database, selecting ii_columns.table_name
**	iicolumns.owner_name and iicolumns.column_name to determine correct name
**	case.
*/
S_DU0443_CLEAR_DELIM_UPPER
"Verifydb removed Upper Case for Delimited Identifiers for this DB."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found illegal delimited identifier semantics.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user may want to check that correct semantics were set by
**	visually inspecting the database, selecting ii_columns.table_name
**	iicolumns.owner_name and iicolumns.column_name to determine correct name
**	case.
*/
S_DU0444_CLEAR_REALUSER_MIXED
"Verifydb removed Mixed Case for Real User Identifiers for this DB."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found illegal delimited identifier semantics.  (Real user
**	identifiers are the user's login ID.)
**
** System status
**	This is an informational message; processing continues.
**
** Recommendation:
**	The user may want to check that correct semantics were set by
**	visually inspecting the database, selecting ii_columns.table_name
**	iicolumns.owner_name and iicolumns.column_name to determine correct name
**	case.
*/
S_DU0445_CREATE_SCHEMA
"Verifydb created schema for the orphaned object."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb found an orphaned object.
**
** System status
**	This is an informational message only; processing continues.
**
** Recommendation:
**	The user may want to check that the schema has been created.
*/
S_DU0446_CREATE_CONSTRAINT
"verifydb created constraint after dropping it."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb dropped the constraint.
**
** System Status:
**	This is an informational message only; processing continues.
**
** Recommendation:
**	The user may want to check that the constraint has been created.
*/
S_DU0447_DROP_CONSTRAINT
"Verifydb dropped constraint '%0c'.'%1c' from this database."
/*%
** Parameters:
**      %0c - schema in which the constraint resides
**      %1c - name of the constraint
**
** Explanation:
**      verifydb found that the object on which this constraint depends does not
**      exist
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0448_DELETED_IIXPRIV
"deleted offending IIXPRIV tuple."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPRIV test #1 found an entry in iixpriv that does not have
**	a corresponding entry in iipriv.  User was asked for and has given 
**	permission to delete the offending IIXPRIV tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0449_INSERTED_IIXPRIV_TUPLE
"iixpriv tuple created."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPRIV test #2 found a tuple in iipriv that does not have a
**	corresponding tuple in iixpriv.  User was asked for and has given
**      permission to insert a corresponding iixpriv tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU044A_DELETED_IIXEVENT
"Deleted the offending tuple from IIXEVENT."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXEVENT test #1 found an entry in iixevent that does not have
**	a corresponding entry in iievent.  User was asked for and has given 
**	permission to delete the offending tuple from IIXEVENT.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU044B_INSERTED_IIXEVENT_TUPLE
"iixevent tuple created."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXEVENT test #2 found a tuple in iievent that does not have a
**	corresponding tuple in iixevent.  User was asked for and has given
**      permission to insert a corresponding iixevent tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU044C_DELETED_IIXPROC
"Deleted the offending tuple from IIXPROCEDURE."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPROCEDURE test #1 found an entry in iixprocedure that does 
**	not have a corresponding entry in iiprocedure.  User was asked for and 
**	has given permission to delete the offending tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU044D_INSERTED_IIXPROC_TUPLE
"iixprocedure tuple created."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIXPROCEDURE test #2 found a tuple in iiprocedure that does 
**	not have a corresponding tuple in iixprocedure.  User was asked for and
**	has given permission to insert a corresponding iixprocedure tuple.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0450_DELETED_IIPRIV_TUPLE
"tuple with d_obj_id = %0d, d_obj_idx = %1d, d_priv_number = %2d 
\nand d_obj_type = %3d was deleted from iipriv."
/*%
** Parameters:
**	%0d - iipriv.d_obj_id
**	%1d - iipriv.d_obj_idx
**	%2d - iipriv.d_priv_number
**	%3d - iipriv.d_obj_type
**
** Explanation:
**	verifydb IIPRIV test #1 found an entry in iipriv that describes a
**	privilege descriptor for a non-existent object or an object of 
**	unexpected type
** 
** System Status:
**	This is an informational message; processing continues.  
**
** Recommendation:
**	none
*/
S_DU0451_MARKED_DBP_DORMANT
"Verifydb marked this database procedure (and all database 
\nprocedures dependent on it) dormant."
/*%
** Parameters:
**	None.
**
** Explanation:
**      verifydb found a database procedures s.t. some object or privilege on
**      which it depends no longer exists - rather than dropping the database
**      procedure we will mark it dormant which is exactly what should have
**      happened when the object or privilege on which the database procedure
**      depends was destroyed.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0452_SET_DB_DBP_INDEP_LIST
"Verifydb updated IIPROCEDURE to indicate that this database procedure
\ndepends on existence of some objects and/or privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROCEDURE test #6 discovered that while the IIPROCEDURE
**	tuple describing a database procedure indicates that it does not depend
**	on any objects and/or privileges, record(s) describing such dependencies
**	were found by examining IIDBDEPENDS and/or IIPRIV.
**	The recommended action is record the fact that the database procedure 
**	depends on some objects and/or privileges
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0453_CLEARED_DBP_UBT_ID
"Verifydb cleared underlying object id for this database procedure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROCEDURE test #7 discovered that the database procedure's 
**	underlying base table is a DBMS catalog.
**	The recommended action is clear the database procedure's underlying 
**	base table id.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0454_FIXED_OBJNAME_IN_IIPROT
"Verifydb copied name of the schema and object from the catalog 
\ndescribing the object into the description of this permit or 
\nsecurity_alarm."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #5 found that the name of the schema and/or the
**	object stored in IIPROTECT tuple do not match those in the catalog 
**	describing the object itself (IIRELATION for tables and view, 
**	IIPROCEDURE for database procedures, or IIEVENT for dbevents.)
**
** 	The recommended action is to overwrite schema/object name in IIPROTECT 
**	tuple with those found in the catalog describing the object
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	none
*/
S_DU0455_SET_PERM_LANG_TO_QUEL
"Verifydb updated proflags to indicate that the permit was created using QUEL."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #8 found that proflags indicates that the 
**	permit was created using SQL, but IIQRYTEXT indicates otherwise.
**
**	The recommended action is to update proflags to indicate that the permit
**	was created using QUEL.
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	none
*/
S_DU0456_SET_PERM_LANG_TO_SQL
"Verifydb updated proflags to indicate that the permit was created using SQL."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #9 found that proflags indicates that the 
**	permit was created using QUEL, but IIQRYTEXT indicates otherwise.
**
**	The recommended action is to update proflags to indicate that the permit
**	was created using SQL.
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	none
*/
S_DU0457_PERM_CREATED_BEFORE_10
"Verifydb updated proflags to indicate that the permit was created 
\nprior to OpenIngres 1.0."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #10 found that proflags indicates that the 
**	permit was created after 6.4, but the permit conveys multiple privileges
**	which suggests that it was created in 6.4 or before
**
**	The recommended action is to update proflags to indicate that the permit
**	was created prior to release 1.0.
**
** System Status:
**	This is an informational message.  Processing continues.
**
** Recommendation:
**	none
*/
S_DU0458_DROPPED_DBEVENT
"Verifydb has dropped this dbevent."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #12 found a permit on a dbevent issued by a 
**	user who does not own it and no longer possesses GRANT OPTION FOR
**	privileges specified in the permit.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU0459_CLEARED_VGRANT_OK
"Bit in IIRELATION.relstat indicating that the view depends
\non some privileges has been set."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIRELATION test #30 discovered that while the IIRELATION
**	tuple describing a view indicates that it does not depend
**	on any privileges, record(s) describing such dependencies
**	were found by examining IIPRIV.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU045A_MAND_RELFLAGS_SET
"IIRELATION.relstat and IIRELATION.relstat2 have been updated to
\ninclude mandatory flags."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb discovered that a system catalog defined to have mandatory
**	flags for relstat and/or relstat2 does not have the correct flags.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU045B_PHYSL_TBL_MADE_HASH
"The catalog %0c has been modified to its intended hash table structure."
/*%
** Parameters:
**	%0c - system catalog name
**
** Explanation:
**	verifydb discovered that a system catalog with TCB2_PHYSLOCK_CONCUR
**	was not hash.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU04A0_SHOW_MODE
" VERIFYDB SELECTED MODE IS %0d"
/*%
** Parameters:
**	%0d - decimal integer -- value corresponding to keyword supplied
**	      with "-m" flag:
**	        1=RUN, 2=RUNSILENT, 3=RUNINTERACTIVE, 4=REPORT,
**	        -1=INVALID, -2=NOTSPECIFIED
** Explanation:
**	Debugging information; shows what mode verifydb was evoked with.
**
** System Status:
**	For development purposes only.
**
** Recommendation:
**	None.
*/
S_DU04A1_SHOW_SCOPE
" VERIFYDB SELECTED SCOPE IS %0d"
/*%
** Parameters:
**	%0d - decimal integer -- value corresponding to keyword supplied with
**	      "-s" flag:
**	        1=SPECIFIED_DB, 2=DBA, 3=INSTALLATION
**	        -1=INVALID, -2=NOTSPECIFIED
**
** Explanation:
**	Debugging information; shows what mode verifydb was evoked with.
**
** System Status:
**	For development purposes only.
**
** Recommendation:
**	None.
*/
S_DU04A2_SHOW_SCOPE_NAME
" VERIFYDB SPECIFIED DATABASE: %0c"
/*%
** Parameters:
**	%0c - first database name specified with "-sdbname" flag
**	      (null-terminated string)
**
** Explanation:
**	Debugging information; name of user-specified database.
**
** System Status:
**	For development purposes only.
**
** Recommendation:
**	None.
*/
S_DU04A3_SHOW_OPERATION
"    VERIFYDB SELECTED OPERATION IS %0d"
/*%
** Parameters:
**	%0c - decimal integer -- value corresponding to keyword supplied
**	      with "-o" flag:
**	        1=DBMS_CATALOGS, 2=FORCE_CONSISTENT, 3=PURGE,
**	        4=TEMP_PURGE, 5=EXPIRED_PURGE, 6=TABLE
**	        -1=INVALID, -2=NOTSPECIFIED
**
** Explanation:
**	Debugging information; shows what operation verifydb was evoked with.
**
** System Status:
**	For development purposes only.
**
** Recommendation:
**	None.
*/
S_DU04A4_SHOW_TABLE_NAME
"    VERIFYDB SPECIFIED TABLE NAME IS %0c"
/*%
** Parameters:
**	%0c - table specified with "-o<table>" flag (null-terminated string)
**
** Explanation:
**	Debugging information; shows the name of the table that table
**	operation is to be performed on.
**
** System Status:
**	For development purposes only.
**
** Recommendation:
**	None.
*/
S_DU04A5_SHOW_USER
"    VERIFYDB SPECIFIED USER IS %0c"
/*%
** Parameters:
**	%0c - user name specified with "-u" flag (null-terminated string)
**
** Explanation:
**	Debugging information; shows the name of the user specified with
**	the "-u" option.
**
** System Status:
**	For development purposes only.
**
** Recommendation:
**	None.
*/
S_DU04C0_CKING_CATALOGS
"VERIFYDB: beginning check of DBMS catalogs\n
for database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Informational only -- used with dbms_catalogs verifydb operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04C1_CATALOGS_CHECKED
"VERIFYDB: catalog check for database %0c complete."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Informational only -- used with dbms_catalogs verifydb operation.
**
** System Status:
**	The verifydb process will exit successfully -- its done.
**
** Recommendation:
**	None.
*/
S_DU04C2_FORCING_CONSISTENT
"VERIFYDB: beginning attempt to patch\n
database %0c consistent"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Informational only -- used with force_consistent verifydb operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04C3_DATABASE_PATCHED
"VERIFYDB: database %0c patched consistent\n"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Informational only -- used with force_consistent verifydb operation.
**
** System Status:
**	The verifydb process will exit successfully -- its done.
**
** Recommendation:
**	None.
*/
S_DU04C4_DROPPING_TABLE
"VERIFYDB: beginning the drop of table\n
%0c from database %1c."
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	Informational only -- used with drop_table verifydb operation.
**
** System Status:
**	The verifydb process is starting to drop the table.
**
** Recommendation:
**	None.
*/
S_DU04C5_TABLE_DROPPED
"VERIFYDB: table %0c successfully\n
dropped from database %1c."
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	Informational only -- used with drop_table verifydb operation.
**
** System Status:
**	The verifydb process will exit successfully -- its done.
**
** Recommendation:
**	None.
*/
S_DU04C6_USER_ABORT
"VERIFYDB: execution terminated at user request."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Informational only -- displayed when the user opts not to do verifydb
**	after reading a warning message.
**
** System Status:
**	The verifydb process will exit without doing anything.
**
** Recommendation:
**	None.
*/
S_DU04C7_PURGE_START
"VERIFYDB: beginning PURGE of temporary and/or expired tables\n
for database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Informational only -- used with purge, temp_purge, or expired_purge
**	verifydb operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04C8_TEMPPURGE_START
"VERIFYDB will remove all temporary tables from database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Informational only -- used with purge or temp_purge verifydb operation.
**	Identifies the type of purge being performed.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04C9_EXPPURGE_START
"VERIFYDB will remove all expired relations tables from the database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Informational only -- used with purge or expired_purge verifydb
**	operation. Identifies the type of purge being performed.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04CA_PURGE_DONE
"VERIFYDB: PURGE of database %0c\n
is complete."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Informational only -- used with purge, temp_purge, or expired_purge
**	 verifydb operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04CB_START_TABLE_OP
"VERIFYDB: beginning TABLE/XTABLE operation on table\n
%0c from database %1c."
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	Informational only -- used with table or xtable verifydb operation.
**
** System Status:
**	The verifydb process is starting to operate on the specified table.
**
** Recommendation:
**	None.
*/
S_DU04CC_TABLE_OP_DONE
"VERIFYDB: TABLE/XTABLE operation on table\n
%0c, database %1c is complete."
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	Informational only -- used with table or xtable verifydb operation.
**
** System Status:
**	The verifydb process is starting to operate on specified table.
**
** Recommendation:
**	None.
*/
S_DU04CD_ACCESSCHECK_START
"................................................................
\nVERIFYDB: Checking access to database '%0c' as user '%1c'"
/*%
** Parameters:
**	%0c - database name
**	%1c - user name
**
** Explanation:
**	Informational only -- indicates verifydb is about to attempt to
**	connect to a database as a specified user.
**
** System Status:
**	The verifydb process is about to connect to the specified database
**	as the specified user.
**
** Recommendation:
**	None.
*/
S_DU04CE_ACCESSIBLE
"VERIFYDB is able to connect to database '%0c' as user '%1c'"
/*%
** Parameters:
**	%0c - database name
**	%1c - user name
**
** Explanation:
**	Informational only -- used with table or xtable verifydb operation.
**
** System Status:
**	The verifydb process was able to connect to the specified database
**	as the specified user.
**
** Recommendation:
**	None.
*/
S_DU04CF_INACCESSIBLE
"VERIFYDB is NOT able to connect to database '%0c' as user '%1c'"
/*%
** Parameters:
**	%0c - database name
**	%1c - user name
**
** Explanation:
**	Informational only -- used with table or xtable verifydb operation.
**
** System Status:
**	The verifydb process is unable to connect to the specified database
**	as the specified user.
**
** Recommendation:
**	None.
*/
S_DU04D0_DDB_WITH_INGRES_CDB
"Database %0c is a Distributed Database.\n
Verifydb does not operate on Distributed Databases at this time.\n
You may respecify command on CDB '%1c'"
/*%
** Parameters:
**	%0c - distributed database name
**	%1c - coordinator database name
**
** Explanation:
**	Informational only -- used with table or xtable verifydb operation.
**
** System Status:
**	The verifydb process is unwilling to connect to the specified
**	distributed database.
**
** Recommendation:
**	The user may reissue the command on the coordinator database, if
**	desired.
*/
S_DU04D1_DDB_CDB_NOT_INGRES
"Database %0c is a Distributed Database.\n
Verifydb does not operate on Distributed Databases at this time.\n
(The CDB is NOT an INGRES Database)"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Informational only -- used with table or xtable verifydb operation.
**
** System Status:
**	The verifydb process is unwilling to connect to the specified
**	distributed database.
**
** Recommendation:
**	The user may reissue the command on the coordinator database, if
**	desired.
*/
S_DU04D2_REFRESH_START
"VERIFYDB will refresh capability level for all LDBS resistered to %0c"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Informational only -- used with refresh_ldbs verifydb operation.
**	Indicates that verifydb is starting to perform this operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04D3_REFRESH_DONE
"VERIFYDB refresh_ldbs operation on database %0c is complete."
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Informational only -- used with refresh_ldbs verifydb operation.
**	Indicates that verifydb has completed this operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU04FF_CONTINUE_PROMPT
"Do you wish to continue? (Y = yes, N = no)>"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Requests input from the user -- used with RUNINTERACTIVE mode of
**	verifydb.  This prompt message always follows an informational
**	message indicating the risk of running verifydb's DBMS_CATALOGS
**	operation in RUNINTERACTIVE mode.
**
** System Status:
**	The verifydb process waits for input, looping back to prompt if
**	the input is not valid.
**
** Recommendation:
**	Enter Y(ES)/N(O).
*/
I_DU0500_STAR_UTIL_INTRO
"%0c distributed database '%1c' . . .\n"
/*%
** Parameters:
**	%0c - text ("Destruction" or "Creation")
**	%1c - distributed database name
**
** Explanation:
**	This message identifies the function of the utility and which 
**	database its operating on.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0501_STAR_UTIL_SUCCESS
"%0c of distributed database '%1c' successfully completed.\n"
/*%
**Parameters:
**	%0c - text ("Destruction" or "Creation")
**	%1c - distributed database name
**
**Explanation:
**	This message identifies the successful completion of the function 
**	of the utility and which database its operating on.
**
**System Status:
**	Informational only; processing continues.
**
**User Action:
**	None.
*/
I_DU0502_STAR_UTIL_DDB_ENTRY
"  Remove distributed database entry from 'iidatabase' and 'iistar_cdbs' . . .\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message informs what distributed createdb/destroydb is doing.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0503_STAR_UTIL_GENERAL
"  %0c STAR %1c . . ."
/*%
** Parameters:
**	%0c - text ("Destroy" or "Create")
**	%1c - information about the current process
**
** Explanation:
**	This message identifies what createdb is doing.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0504_ALL_DDBTUPS_FOUND_FI
"All distributed databases in IIDATABASE are catalogued in IISTAR_CDBS"
/*%
**Parameters:
**	None.
**
**Explanation:
**	This message informs user that the CDB is catalogued in IISTAR_CDBS.
**
**System Status:
**	Informational only; processing continues.
**
**User Action:
**	None.
*/
I_DU0505_ALL_CDBTUPS_FOUND_FI
"All coordinator databases in IIDATABASE are catalogued in IISTAR_CDBS"
/*%
** Parameters:
**	None.
**
** Explanation:
**	All IISTAR_CDBS tuples are catalogued in IIDATABASE.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
/*********** MESSAGE DOCUMENTATION APPEARS TO PRECEDE MESSAGE STARTING HERE...
*/
I_DU0506_ALL_STARTUPS_FOUND_FI
"All IISTAR_CDBS tuples are catalogued in IIDATABASE"
/*%
** Parameters:
**	%0c- distributed database name
**
** Explanation:
**	This message informs the user that finddbs is inserting the DDB.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0507_DDBTUP_APPEND_FI
"  Inserting distributed database '%0c' into IIDBDB."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Inform the user that the distributed database has a matching
**	CDB to go with it.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0508_ALL_DDBCDB_FOUND_FI
"All distributed databases in IIDATABASE have matching coordinator database"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	The distributed database was found.
**
** System Status:
**	Informational only ; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0509_DDB_FOUND_FI
"  Distributed database '%0c' was found."
/*%
** Parameters:
**	    None.
**
** Explanation:
**	    Inform the user finddbs is now looking for distributed databases.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0510_SEARCH_DDB_FI
"Beginning to search for distributed databases"
/*%
** Parameters:
**	    None.
**
** Explanation:
**	    finddbs starts to restore distributed databases.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0511_RESTORE_DDB_FI
"Beginning to restore distributed databases"
I_DU0520_GW_UTIL_INTRO
"%0c %1c Gateway database '%2c' . . .\n"
/*%
** Parameters:
**	%0c - text ("Destruction" or "Creation")
**	%1c - gateway name (e.g., RMS)
**	%2c - gateway database name
**
** Explanation:
**	This message identifies the function of the utility (createdb or
**	destroydb), which gateway is involved, and which database it is
**	operating on.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
I_DU0521_GW_UTIL_GENERAL
"  %0c Gateway %1c . . ."
/*%
** Parameters:
**	%0c - text ("Destroy" or "Create")
**	%1c - information on current process
**
** Explanation:
**	This message identifies what createdb is doing.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1000_DB_NOT_FOUND
"The database '%0c' could not be found in the IIDBDB database\n
system catalog."
W_DU1001_NO_IIEXTEND_ENTRIES
"There were no entries in the IIextend catalog of IIDBDB for this\n
database."
W_DU1010_UTIL_ABORT
"%0c of database '%1c' abnormally terminated."
W_DU1011_INTERRUPT
"  Utility interrupt terminated."
W_DU1012_UNKNOWN_EX
"Unexpected exception occurred in utility."
W_DU1013_UTIL_ABORT
"%0c of database abnormally terminated."
W_DU1020_DEL_DIRNOTFOUND
"The directory '%1c' in the path '%0c'\n
was not found."
W_DU1021_DEL_FILENOTFOUND
"The file '%1c' in the path '%0c'\n
was not found."
W_DU1030_UNKNOWN_CATALOG_SY
"The system catalog '%0c' is unknown to the SYSMOD utility.\n
Please notify Ingres Corporation technical support."
W_DU1031_NOMOD_FE_CAT
"Warning: SYSMOD could not modify the front-end catalogs.\n
An error occurred when attempting to call the internal utility program\n
'%0c' to modify the front-end system catalogs.  Continuing . . ."
/*%
** Explanation:
**	    SYSMOD just tried to call an internal utility program called "modifyfe"
**	    to modify the front-end system catalogs. Modifyfe was not able to
**	    successfully modify the front-end catalogs.
**
** System Status:
**	    Any other catalogs specified on the SYSMOD command line will be
**	    modified. The front-end catalogs were not completely modified
**	    (some of them may have been modified before the error occurred).
**
** Recommendation:
**	    Check for problems in your environment and DBMS installation
**	    (disk space, subprocess and other quotas, is the installation
**	    complete with executables present in the "bin" subdirectory, etc.).
**	    VMS users can run the "checkenv" utility.
*/
W_DU1032_FECAT_IGNORE
"WARNING: '%0c' ignored.\n
Do not specify front-end catalog names on the SYSMOD command line.\n
Specify which front-end catalogs to modify by specifying the -f flag,\n
followed by the name of a front-end dictionary client (for example\n
-f INGRES, or  -f WINDOWS_4GL); that will cause all front-end catalogs\n
used by the specified dictionary client(s) to be modified.\n
SYSMOD will continue and '%0c' will be ignored . . ."
/*%
** Parameters:
**	%0c - frontend catalog name
**
** Explanation:
**	You specified the name of a frontend catalog on the sysmod command
**	line.  Frontend catalogs are modified in groups by specifying the 
**	name of a frontend "dictionary client" on the sysmod command
**	line; then all frontend catalogs used by that dictionary client will be
**	modified.
**
** System Status:
**	The frontend catalog name is ignored. It is as if it were not
**	specified on the sysmod command line.
**
** Recommendation:
**	Check which frontend client you want to modify catalogs for
**	and specify that client name on the sysmod command line (for example:
**	"sysmod <database_name> vision").
*/
W_DU1040_FOUND_EXTDB_FI
"Found extended database '%0c' in the area '%1c'."
W_DU1041_NOLOC_FI
"'%0c' has not been defined as a locationname in the IILOCATIONS catalog."
W_DU1042_NO_CNFLOC_FI
"The locationname '%0c' found in the '%1c'\n
database's configuration file has not been defined in this installation's\n
IILOCATIONS catalog."
W_DU1043_SAME_DBNAME_FI
"Second occurrence of '%0c' ignored."
W_DU1044_REALDBS_NOT_FOUND_FI
"\nReal databases not in the IIDATABASE catalog:"
W_DU1045_DBTUPS_NOT_FOUND_FI
"'iidatabase' tuples not corresponding to real databases:"
W_DU1046_DUP_DBTUP_FI
"The IIDBDB contains more than one entry for the database %0c."
W_DU1047_SAME_DB_FI
"Database '%0c' already located."
W_DU1048_ABORT_FI
"Rebuilding of IIDBDB abnormally terminated."
W_DU1049_LOCNAME_TOO_LONG_FI
"ERROR: Location names may only be 24 characters long.  Please re_enter."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered a location name that is more than 24 characters long in
**	response to I_DU0031_NEWLOC_PROMPT_FI.  Location names are limited to
**	24 characters.
**
** System Status:
**	finddbs "throws away" this invalid input and displays the
**	I_DU0031_NEWLOC_PROMPT_FI prompt.
**
** Recommendation:
**	The user is given another chance to correctly specify the location name.
**	The user may break from the prompt by entering <CR>.  In that case,
**	finddbs will stop its search phase and start its analyze phase.
*/
W_DU104A_AREANAME_TOO_LONG_FI
"ERROR: Location names may not exceed 128 characters.\n
Please reenter location definition."
/*%
** Parameters:
**	None.
** Explanation:
**	The user entered the physical mapping for a location that is more than
**	128 characters long in response to I_DU0032_LOCAREA_PROMPT_FI prompt.
**	Location mappings are limited to 128 characters.
**
** System Status:
**	finddbs "throws away" this invalid input, along with the location name
**	specified, and displays the I_DU0031_NEWLOC_PROMPT_FI prompt.
**
** Recommendation:
**	The user is given another chance to correctly specify the location name
**	and physical mapping. The user may break from the prompt by entering
**	<CR>.  In that case, finddbs will stop its search phase and start its
**	analyze phase.
*/
W_DU104B_DUP_LOCATION_FI
"ERROR: Location %0c is already defined in this installation"
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	The user is asking to define a new location, but that location is
**	already known to the installation.  Therefore, finddbs refuses to
**	operate on this user-defined location.  finddbs does not verify that
**	the physical mapping the user specifies matches the mapping in the
**	iilocation catalog.  This is done intentionally because finddbs does
**	not support remapping/relocation of existing locations.
**
** System Status:
**	finddbs stops operating on the user-specified location and returns to
**	the I_DU0031_NEWLOC_PROMPT_FI prompt
**
** Recommendation:
**	The user may specify a different location or may enter <CR> to terminate
**	the interactive portion of finddbs.
*/
W_DU104C_INVALID_LOC_FI
"ERROR: User defined location %0c is invalid."
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	The user defined an invalid location.  This can be caused by either
**	using a non-existent mapping or by using a mapping to a valid location
**	that does not contain a data directory.
**
** System Status:
**	finddbs stops operating on the user-specified location and returns to
**	the I_DU0031_NEWLOC_PROMPT_FI prompt
**
** Recommendation:
**	The user may specify a different location or may enter <CR> to terminate
**	the interactive portion of finddbs.
*/
W_DU104D_DILISTDIR_FAIL_FI
"  An unexpected error was returned from the CL routine 'DIlistdir'\n
when called to search directory '%0c'\n
FINDDBS is unable to search location '%1c' for databases.."
/*%
** Parameters:
**	%0c - pathname that DIlistdir() failed on
**	%1c - location name that maps to %0c
**
** Explanation:
**	finddbs was unable to search this location because of an unexpected
**	error returned from DIlistdir().
**
** System Status:
**	finddbs stops operating on this location and goes on to another.
**
** Recommendation:
**	The user should determine what the problem is (usually a VMS system
**	logical is not defined, is incorrectly defined, or protections on the
**	directory are incorrect).  The user should correct the problem and
**	re-run finddbs.
*/
W_DU1062_DBMS60_FAIL
"Conversion of the database '%0c's' DBMS objects failed."
W_DU1080_RUN_DESTROYDB
"Run DESTROYDB to delete the partially created database '%0c'."
W_DU1081_CANT_READ_PRODS_LST
"WARNING: unable to read file 'prods.lst'.\n
An error occurred while trying to read the file: 'prods.lst' in the\n
'dictfiles' subdirectory of the 'ingres', 'files' directory. This\n
indicates that there is a problem with your Ingres installation;\n
see your Site Technical Contact for assistance.\n
CREATEDB or UPGRADEDB will continue, but only front-end catalogs for the basic\n
Ingres products (dictionary client 'INGRES') will be created/upgraded.\n
Continuing . . ."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while trying to open and read the file "prods.lst"
**	in the $II_CONFIG/dictfiles directory. Probably there are either 
**	missing files in the directory, or protections on "prods.lst" do 
**	not allow world READ access; either way, this signals an installation 
**	problem.
**
** System Status:
**	This is a warning only; createdb will continue without error, but
**	it will only create the front-end catalogs for dictionary client
**	'INGRES' (i.e., catalogs for other dictionary clients you are authorized
**	to run will not be created).
**
** Recommendation:
**	Check your installation for missing or unreadable files
**	in the directory mentioned above.
*/
W_DU1082_STAR_FECAT_WARN
"WARNING: No FE catalogs are created for the DISTRIBUTED Database.\n
You must run UPGRADEFE on a STAR server to create the appropriate FE catalogs."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified frontend clients via the "-f" flag on the createdb
**	command line.  However, the user was creating a distributed database,
**	so the parameters following the "-f" flag are ignored.
**
** System Status:
**	This is a warning only; createdb will continue without error, but
**	it will NOT create the frontend catalogs for any FE clients.
**
** Recommendation:
**	The user should run upgradefe with the same "-f" flag parameters that
**	were used with createdb.
*/
W_DU1083_BAD_STARCONNECT
"Unable to connect to the STAR server.  Please check your installation and see
if a STAR server is up."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate
**	(i.e., create or upgrade) a distributed database.  However, the
**	utility could not connect to the Star server.
**
** System Status:
**	Execution continues, but FE catalog operations will be skipped.
**
** Recommendation:
**	The user should verify that the Star server is up and operative in their
**	installation.  The user should run upgradefe on their distributed
**	database to create/upgrade the FE catalogs.
*/
W_DU1084_NO_FECAT_CRE
"Warning:  Front-End catalog creation is skipped.\n
You may use UPGRADEFE to create the Front-End catalogs for database %0c."
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate
**	(i.e., create or upgrade) a distributed database.  However, the
**	utility could not connect to the Star server.
**
** System Status:
**	Execution continues, but FE catalog creation is skipped.
**
** Recommendation:
**	The user should verify that the Star server is up and operative in their
**	installation.  The user should run upgradefe on their distributed
**	database to create/upgrade the FE catalogs.
*/
W_DU1085_SORTFLAG_DEPRECATED
"The -w flag replaces the -s flag.  In future please specify -wlocationname.\n
 '%0c' will be the work location for this database."
/*%
** Parameters:
**	%0c - location name
** Explanation:
**	Now that work locations replace and subsume sort locations, the "-w"
**	flag replaces and subsumes the "-s" flag.  But we still accept the
**	"-s" flag for backward compatibility.  However, we issue this warning
**	so the user can change scripts, etc.
**
** System Status:
**	Warning message only.
** Recommendation:
**	If the createdb command was issued from a script, update the script to
**	use the "-w" flag instead of a "-s" flag.
*/
W_DU1086_CANT_LOAD_PM_FILE
"Unable to load INGRES parameter file II_CONFIG/config.dat"
W_DU1087_CANT_GET_HOSTNAME
"Unable to get host name"
W_DU1088_PMSETDEFAULT_FAILED
"Unable to set up defaults for PM parameter lookup"
W_DU1520_NO_UCODE50_FILE
"The 6.0 INGRES utility file '%0c' doesn't\n
exist.  This file is needed to create the 6.0 INGRES utility table 'iicodemap'.\n
'iicodemap' is used to map 5.0 user codes to their corresponding user names.\n
Please consult DUF annotated error messages for suggested solutions to this\n
problem."
W_DU1521_BAD_UCODE
"The user code '%0c' could not be found in the 'iicodemap' table.\n
This code was encountered in a 5.0 admin file when trying to rebuild a 5.0\n
iidatabase tuple for FINDDBS.\n
Please consult DUF annotated error messages for suggested solutions to this\n
problem."
W_DU1522_NO_50AREA
"The 5.0 area '%0c' could not be found in the 'iilocations' table.\n
This area was encountered in a 5.0 admin file when trying to rebuild a 5.0\n
iidatabase tuple.\n
Please consult DUF annotated error messages for suggested solutions to this\n
problem."
W_DU1523_BAD_50ADMIN_READ
"Can't read 5.0 admin file '%0c' for database '%1c'\n
on '%2c'."
W_DU1524_TOO_MANY_LOCS
"The 5.0 area '%0c' is referenced by two or more locations in the\n
iilocations catalog.  FINDDBS is unable to determine what location this\n
database belongs on.  Because of this no iidatabase tuple can be built for\n
this 5.0 database.\n
Please consult DUF annotated error messages for suggested solutions to this\n
problem."
S_DU1600_INVALID_RELID
"Table %0c (owner %1c) has invalid table identifier"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #1 found an entry in iirelation where reltid=0.  Since
**	reltid is used to join the iirelation catalog to other system catalogs
**	that are required to retrieve any data from the table (such as
**	iiattribute), this tuple is useless.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Recommended action can only be performed in RUNINTERACTIVE mode. The
**	recommended action is to delete this tuple from iirelation.  If the
**	user does not opt to delete this tuple, s/he can manually modify
**	iirelation.reltid to the correct value (if known).  (See DU0301.)
*/
S_DU1601_INVALID_ATTID
"Table %0c (owner %1c)\n
has a mismatch in number of columns.\n
iirelation indicates there are %2d\n
but iiattribute contains %3d."
/*%
** Parameters:
**	 %0d - attribute number (from iiattribute.attno)
**	 %1c - table name (from iirelation.relid)
**	 %2c - table owner (from iirelation.relowner)
**
** Explanation:
**	 verifydb test #2a found an invalid attno in iiattribute.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Recommended action can only be performed in RUNINTERACTIVE mode. The
**	recommended action is to delete this table from the database.  If the
**	user does not opt to drop this table, s/he can manually reconstruct
**	the iiattribute tuples. (See DU0302.)
*/
S_DU1602_DUPLICATE_ATTIDS
"Table %0c (owner %1c) has\n
too many column #%2d)"
/*%
** Parameters:
**	 %0c - table name (from iirelation.relid)
**	 %1c - table owner (from iirelation.relowner)
**	 %2d - attribute number (from iiattribute.attno)
**
** Explanation:
**	 verifydb test #2a found more than one tuple with the specified attno.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Recommended action can only be performed in RUNINTERACTIVE mode. The
**	recommended action is to delete this table from the database.  If the
**	user does not opt to drop this table, s/he can manually reconstruct the
**	iiattribute tuples. (See DU0302.)
*/
S_DU1603_MISSING_ATTIDS
"Attribute #%0d is missing from\n
table %1c (owner %2c)."
/*%
** Parameters:
**	 %0d - attribute number (from iiattribute.attno)
**	 %1c - table name (from iirelation.relid)
**	 %2c - table owner (from iirelation.relowner)
**
** Explanation:
**	 verifydb test #2a found a missing attribute definition for the table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Recommended action can only be performed in RUNINTERACTIVE mode. The
**	recommended action is to delete this table from the database.  If the
**	user does not opt to drop this table, s/he can manually reconstruct the
**	iiattribute tuples. (See DU0302.)
*/
S_DU1604_ATTID_MISMATCH
"Mismatch on number of columns in\n
table %0c (owner %1c) -- \n
iirelation indicates it has %2d columns and\n
there are %3d columns defined in iiattributes."
/*%
** Parameters:
**	 %0c - table name (from iirelation.relid)
**	 %1c - table owner (from iirelation.relowner)
**	 %2d - number of attributes (from iirelation.relatts)
**	 %3d - largest value of attribute number (from iiattribute.attno)
**
** Explanation:
**	There is a mismatch between the number of columns that iirelation
**	indicates the table has and the actual number of columns defined in
**	iiattributes.  This is detected by verifydb test #2a.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Recommended action can only be performed in RUNINTERACTIVE mode. The
**	recommended action is to delete this table from the database.  If the
**	user does not opt to drop this table, s/he can manually reconstruct the
**	iiattribute tuples. (See DU0302.)
*/
S_DU1605_INCORRECT_RELWID
"There is a mismatch between the table width\n
defined in iirelation as %0d\n
and the sum of each column's width in iiattributes\n
(which is %1d)\n
for table %2c (owner %3c)"
/*%
** Parameters:
**	 %0d - table width from iirelation (iirelation.relwid)
**	 %1d - sum of width of each attribute (from each iiattribute.attfrml)
**	 %2c - table name (from iirelation.relid)
**	 %3c - table owner (from iirelation.relowner)
** Explanation:
**	verifydb test #2b found a mismatch between how large (number of bytes
**	of storage) a tuple in the table is and the actual columns defined in
**	iiattributes.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Recommended action can only be performed in RUNINTERACTIVE mode. The
**	recommended action is to replace iirelation.relatts with the sum of
**	each attfrml.  If the user does not opt to do so, s/he can manually
**	reconstruct the iiattribute tuples, or drop the table from the database.
**	(See DU0303.)
*/
S_DU1606_KEYMISMATCH
"Mismatch between iirelation and iiattributes\n
concerning #keys in table.\n
iirelation indicates there are %0d\n
but iiattribute indicates there are %1d\n
for table %2c (owner %3c)"
/*%
** Parameters:
**	%0d - columns iirelation indicates there are (iirelation.relkeys)
**	%1d - count of number of keys in iiattributes for this table
**	      (count(iiattributes.attkey == ISKEY)
**	%2c - table name (from iirelation.relid)
**	%3c - tableowner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #3 found a mismatch between the number of columns marked
**	as keys in iiattributes and the number of keys that iirelation
**	indicates there should be.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Replace iirelation.relkeys with the count of keys defined in
**	iiattribute.  If the user is in RUNINTERACTIVE mode and does not
**	opt for the recommended action, s/he can manually repair or reconstruct
**	the iiattribute tuples, or drop the table from the database.
**	No correction is made if verifydb is in REPORT mode.  (See DU0304.)
*/
S_DU1607_INVALID_RELSPEC
"Table %0c (owner %1c)\n
has invalid storage structure specified\n
Run VERIFYDB with -otable option for this table."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #4 found an invalid relspec in iirelation for this table.
**
** System Status:
**	The verifydb process continues..
**
** Recommendation:
**	Run verifydb's patchlink function on this table, or drop this table from
**	the database.
*/
S_DU1608_MARKED_AS_CATALOG
"Table %0c (owner %1c) incorrectly marked as system catalog."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #5a found a table marked as a system catalog that does
**	not start with "ii", or is not owned by $ingres, or both.
**
** System Status
**	The verifydb process continues.
**
** Recommendation:
**	In RUNINTERACTIVE mode, the user can grant yes/no permission to
**	clear the bit in iirelation.  No correction is made if verifydb is
**	in REPORT mode.  (See DU030A.)
*/
S_DU1609_NOT_CATALOG
"Table %0c (owner %1c) is not a system catalog"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #5b found a catalog that starts with "ii" but does not
**	have iirelation.relstat==CATALOG.
**
** System Status:
**	The verifydb process continues..
**
** Recommendation:
**	Some tables beginning with ii are permitted from convto60.  The user
**	must decide if this is one.  If not, the user may want to manually
**	modify iirelation.relstat.
*/
S_DU1610_NO_TABLE_FILE
"Warning:  Physical File for table\n
%0c (owner %1c)\n
does not exist."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #6 found a file was missing.
**
** System Status:
**	The verifydb process continues, as long as it can before it crashes.
**	It should not crash if the missing file is for a user table, but it
**	will crash if the missing file is for a core system catalog.
**
** Recommendation:
**	Try to find a backup tape with the disk file if the table should
**	have a file (or files, if there are multiple locations) associated with
**	it.  If the table is a core catalog and the file cannot be restored or
**	recreated, the user has lost all the data in the database.
**
**	If the user cannot recover the disk file for a user table, then only
**	the user table is lost, not the whole database.  If verifydb is being
**	run in RUNINTERACTIVE mode, it will prompt for permission to drop the
**	table from the system catalogs.  Such permission should NOT be granted
**	until the user is sure that he does not have a backup of the file(s).
**	(See DU0302.)
*/
S_DU1611_NO_PROTECTS
"iirelation indicates that there are protections for\n
table %0c (owner %1c),\n
but none are defined."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #8 found that there were not really protections defined
**	for this table, but iirelation thought that there were.
**
** System Status:
**	The verifydb process continues..
**
** Recommendation:
**	None required unless running in RUNINTERACTIVE mode, then enter yes/no
**	permission for request to clear PRTUPS bit from iirelation.relstat after
**	being prompted for permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0305.)
*/
S_DU1612_NO_INTEGS
"iirelation indicates that there are integrities for\n
table %0c (owner %1c),\n
but none are defined."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #9 found that there were not really integrities defined
**	for this table, but iirelation thought that there were.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required, unless running in RUNINTERACTIVE mode.  Then
**	enter yes/no permission for request to clear INTEGS bit from
**	iirelation.relstat after being prompted for permission.  
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0306.)
*/
S_DU1613_BAD_CONCUR
"iirelation specifies special core catalog locking\n
for non core catalog table:\n
Table %0c  (owner %1c)"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #10 found that a table that is not a DBMS core catalog
**	was marked for special core-catalog locking, which blocks out
**	no_read_lock users while the catalog is being updated.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required, unless in RUNINTERACTIVE mode.  Then enter
**	yes/no permission to clear the CONCUR bit from iirelation.relstat --
**	the answer should be yes, unless the table is one of: iirelation;
**	iirel_idx; iiattribute; iiindex; iidevices.  If the table is one of
**	these, and verifydb outputs this message, then there is a serious
**	problem.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0307.)
*/
S_DU1614_MISSING_CONCUR
"iirelation does not specify special core catalog locking\n
for core catalog %0c"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**
** Explanation:
**	verifydb test #11 found that one of the DBMS core catalogs is not
**	marked for the special locking that keeps no_read_lock users from
**	accessing the catalog while it is being updated.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required, unless in RUNINTERACTIVE mode.  Then enter
**	yes/no permission to set the CONCUR bit from iirelation.relstat --
**	the answer should be yes, if the table is one of: iirelation;
**	iirel_idx; iiattribute; iiindex; iidevices.  If the table is not
**	one of these, and verifydb outputs this message, then there is a
**	serious problem.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0308.)
*/
S_DU1615_NOT_VIEW
"iirelation indicates that\n
%0c (owner %1c)\n
is a view,\n
but a physical disk file exists for it,
which indicates that it's really a table." 
/*%
** Parameters:
**	%0c - view/table name (from iirelation.relid)
**	%1c - view/table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #12 found that there is a physical disk file associated
**	with this view's reltid -- and views never have physical tables
**	associated with them.  Besides, there is no entry in iitree for
**	this view.  Therefore, it is either a table that has been mismarked
**	as a view, or someone has inadvertently changed file names and
**	iitree has lost some of its tuples.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If the user is not running in RUNINTERACTIVE mode, verifydb
**	automatically changes iirelation to assume this is a table,
**	not a view.  If verifydb is in RUNINTERACTIVE mode, then the
**	user must grant yes/no permission to redefine the view to be a table.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0309.)
*/
S_DU1616_MISSING_VIEW_DEF
"there is no view definition (in iitree) for\n
%0c (owner %1c)"
/*%
** Parameters:
**	%0c - view name (from iirelation.relid)
**	%1c - view owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #12 could not find the view definition for this view.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If verifydb is not in RUNINTERACTIVE mode, it automatically drops this
**	view from the system.  Otherwise, the user must give yes/no permission
**	to drop this view from the system.
**
**	No correction is made if verifydb is in REPORT mode.  (See DU030B.)
*/
S_DU1617_NO_DBDEPENDS
"Missing information in iidbdepends for\n
view %0c (owner %1c).\n
This may be normal if the the view is a constant view (depends on no
tables).  If the view is not a constant view, the recommended action is
to drop this view.  Verifydb will not perform this action automatically."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #13 found that the view was not shown in iidbdepends;
**	therefore, the DBMS can't trace the view back to the table(s) it's based
**	on.  This means there is no way to verify that the base table for the
**	view really exists.
**	If the view really does not depend on any tables, this situation
**	is normal and correct.  However, if the view is defined on one
**	or more tables, the view definition is damaged.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Verifydb will not correct this warning automatically, since it
**	cannot determine whether the view is supposed to be defined on
**	base table(s).  The DBA should examine the view definition and
**	drop/recreate the view, unless it is a constant view.
*/
S_DU1618_NO_QUERYTEXT
"Warning: there is no entry in iiqrytext\n
to define view %0c (owner %1c)."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #14 found that the query text (which defined the view)
**	is not in iiqrytext table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb only prints a warning message, so there is no user action
**	required.  However, the user may want to drop this view and redefine
**	it, so that the query test is defined.
*/
S_DU1619_NO_VIEW	
"iirelation indicates that there is a view defined\n
for table %0c (owner %1c),\n
but none exists."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #15 found that iirelation believes this is a base table
**	for a view, but there is no view defined that uses this table as part
**	(or all) of the view base.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If verifydb is not in RUNINTERACTIVE mode, it automatically clears the
**	VBASE bit from iirelation.relstat.  Otherwise, it prompts the user for
**	permission to do so.  The user must give yes/no permission -- usually
**	s/he should give yes in this case.  If the user wants to recreate
**	the view, s/he can get the view definition from iiqrytext.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU030C.)
*/
S_DU161A_NO_BASE_FOR_INDEX
"The base table for index table\n
%0c (owner %1c)\n
does not exist."
/*%
** Parameters:
**	%0c - index table name (from iirelation.relid)
**	%1c - index table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #16 found that this index table is useless, because
**	the base table that it describes does not exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop the index table unless in
**	RUNINTERACTIVE mode, where it will prompt the user for permission
**	to drop the table.  In this case, the user must give yes/no permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0302.)
*/
S_DU161B_NO_2NDARY
"iirelation does not indicate that there is\n
a secondary index table for base table\n
%0c (owner %1c).\n
However, index table\n
%2c (owner %3c) exists."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid for base table)
**	%1c - table owner (from iirelation.relowner for base table)
**	%2c - index table name (from iirelation.relid for index table)
**	%3c - index table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #17 found that the iirelation entry for a base table did
**	not indicate there is a secondary index table for this table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless running in RUNINTERACTIVE mode -- then give yes/no
**	permission to set IDXD bit in iirelation.relstat for base table.
**
**	No correction can be  made if verifydb is in REPORT mode. (See DU030D.)
*/
S_DU161C_WRONG_NUM_INDEXES
"iirelation believes there are %0d indexes on\n
table %1c (owner %2c)\n
but %3d are defined."
/*%
** Parameters:
**	%0d - number of index tables iirelation thinks there are (from
**		iirelation.relidxcount)
**	%1c - base table name (from iirelation.relid)
**	%2c - table owner (from iirelation.relowner)
**	%3d - actual number of index tables defined in iiindex
**
** Explanation:
**	verifydb test #18 found that iirelation thinks there are more secondary
**	indexes defined for this table than really exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give yes/no
**	permission to change the value of iirelation.relidxcount.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU030E.)
*/
S_DU161D_NO_INDEXS
"iirelation believes there are indexes on\n
table %0c (owner %1c)\n
but none are defined."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid for base table)
**	%1c - table owner (from iirelation.relowner for base table)
**
** Explanation:
**	verifydb test #18 found that iirelation thinks there are secondary
**	indexes defined for this table, but none exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give yes/no
**	permission to clear iirelation.relidxcount and clear IDXD bit from
**	iirelation.relstat.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU030F.)
*/
S_DU161E_NO_STATISTICS
"iirelation believes there are optimizer statistics for\n
table %0c (owner %1c)\n
but none are defined."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid for base table)
**	%1c - table owner (from iirelation.relowner for base table)
**
** Explanation:
**	verifydb test #19 found that iirelation thinks there are optimizer
**	statistics defined for this table, but none exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give yes/no
**	permission to clear OPTSTAT bit from iirelation.relstat.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0310.)
*/
S_DU161F_WRONG_NUM_"iirelation believes there are %0d locations for\n
table %1c (owner %2c)\n
but %3d are defined."
/*%
** Parameters:
**	%0d - number of locations iirelation thinks there are (from
**		iirelation.relloccount)
**	%1c - table name (from iirelation.relid)
**	%2c - table owner (from iirelation.relowner)
**	%3d - actual number of locations defined in iidevices
**
** Explanation:
**	verifydb test #20 found that iirelation thinks there are more locations
**	defined for this table than are defined in iidevices.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give yes/no
**	permission to change the value of iirelation.relloccount, if there are
**	some locations defined in iidevices.  Or, if there are no locations
**	defined in iidevices, then give yes/no permission to reset the 
**	MULTIPLE_LOC bit of iirelation.relstat and to clear
**	iirelation.relloccount.  NOTE: verifydb will not take corrective
**	action unless it is running in RUNINTERACTIVE mode and yes permission
**	has been granted. (See DU0311.)
*/
S_DU1620_INVALID_LOC
"location %0c\n
for table %1c (owner %2c)\n
is invalid"
/*%
** Parameters:
**	%0c - location name (from iirelation.relloc)
**	%1c - table name (from iirelation.relid)
**	%2c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #21 found that the default location for this table is
**	not valid, or one of the extended locations for this table is not
**	valid.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will not permit any user action. However, the user will want
**	to manually fix the location to be valid.
*/
S_DU1621_INVALID_FILLFACTOR
"Table %0c (owner %1c)\n
has an invalid data page fill factor of %2d"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**	%2d - fill factor (from iirelation.reldfill)
**
** Explanation:
**	verifydb test #22, #23, #24, or #25 found an invalid data page
**	fill factor.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give
**	yes/no permission to put the default fill factor for this table
**	type into iirelation.reldfill.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0312.)
*/
S_DU1622_INVALID_LEAF_FILLFTR
"Table %0c (owner %1c)\n
has an invalid leaf page fill factor of %2d"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**	%2d - fill factor (from iirelation.rellfill)
**
** Explanation:
**	verifydb test #25 found an invalid leaf page fill factor.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give
**	yes/no permission to put the default fill factor for this table
**	type into iirelation.reldfill.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0313.)
*/
S_DU1623_INVALID_INDEX_FILLFTR
"Table %0c (owner %1c)\n
has an invalid index page fill factor of %2d"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**	%2d - fill factor (from iirelation.relifill)
**
** Explanation:
**	verifydb test #25 found an invalid index page fill factor.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless verifydb is in RUNINTERACTIVE mode -- then give
**	yes/no permission to put the default fill factor for this table
**	type into iirelation.reldfill.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0314.)
*/
S_DU1624_MISSING_IIRELIDX
"system catalog iirel_idx is missing the entry for\n
table %0c (owner %1c)"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #26 found a table definition in iirelation that
**	does not have a corresponding entry in iirel_idx.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode. Then
**	the user must give yes/no permission to create a tuple in iirel_idx
**	for this table.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU1625_MISSING_IIRELATION
"Table %0c (owner %1c)\n
is defined in iirel_idx, but is missing from iirelation.\n
This means that the database does not recognize this table."
/*%
** Parameters:
**	%0c - table name (from iirel_idx.relid)
**	%1c - table owner (from iirel_idx.relowner)
**
** Explanation:
**	verifydb test #27 found an entry in the index table for a
**	nonexistent table -- i.e., there is no corresponding entry in
**	iirelation for this table.
**
** System Status:
*The verifydb process continues.
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode. Then
**	the user must give yes/no permission to drop this tuple from iirel_idx.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU1626_INVALID_ATTRELID
"Table id of %0d, Index table id of %1d\n
in iiattribute does not match any table
listed in iirelation."
/*%
** Parameters:
**	%0c - attrelid from iiattributes
**	%1c - attrelidx from iiattributes
**
** Explanation:
**	verifydb test #28 found some tuples in iiattributes that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	this means that they are useless, and should be deleted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user must give yes/no permission to drop these tuples from
**	iiattribute.  The user may want to construct an iirelation tuple
**	manually, rather than to delete these tuples from iiattribute.
**	NOTE: verifydb will not take any corrective action for this situation
**	unless it is in RUNINTERACTIVE mode, and "yes" permission has been
**	granted. (See DU0317.)
*/
S_DU1627_INVALID_ATTFMT
"Table %0c (owner %1c)\n
has invalid attribute format for column %2d of %3d."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**	%2d - column number (from iiattribute.attid)
**	%3d - numeric value of iiattribute.attfrmt
**
** Explanation:
**	verifydb test #29 found an invalid attribute format specification for
**	one of the columns.  It must be one of: INTEGER; FLOAT; C; TEXT; DATA;
**	MONEY; CHAR; or VCHAR.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If verifydb is in RUNINTERACTIVE mode, the user must give yes/no
**	permission to drop table from database.  If the user withholds
**	permission or verifydb is being run in another mode, the user may
**	want to manually update iiattribute.attfrmt.  Valid values are:
**                          Nullable     Not Nullable
**	    INTEGER           -30             30
**	    FLOAT             -31             31
**	    C                 -32             32
**	    TEXT              -37             37
**	    DATE               -3              3
**	    MONEY              -5              5
**	    CHAR              -20             20
**	    VCHAR             -21             21
**
**	(See DU0302.)
*/
S_DU1628_INVALID_BASEID
"Table id of %0d, Index table id of %1d\n
in iiindex does not match any table
listed in iirelation."
/*%
** Parameters:
**	%0c - baseid from iiindex
**	%1c - indexid from iiindex
**
** Explanation:
**	verifydb test #30 found some tuples in iiindex that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	this means that they are useless, and should be deleted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user must give yes/no permission to drop these tuples from iiindex.
**	The user may want to construct an iirelation tuple manually, rather than
**	to delete these tuples from iiindex.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0318.)
*/
S_DU1629_IS_INDEX
"iirelation does not indicate that table
%0c (owner %1c)\n
is a secondary index table."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid for base table)
**	%1c - table owner (from iirelation.relowner for base table)
**
** Explanation:
**	verifydb test #31 found that the iirelation entry did not mark this
**	as a secondary index table, but it is one.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless running in RUNINTERACTIVE mode -- then give yes/no
**	permission to set INDEX bit in iirelation.relstat for this index table.
**	(See DU0319.)
*/
S_DU162A_INVALID_INDEXKEYS
"Secondary index table %0c (owner %1c)\n
has invalid key column of %2d"
/*%
** Parameters:
**	%0c - table name (from iirelation.relid for base table)
**	%1c - table owner (from iirelation.relowner for base table)
**	%2d - invalid idom element in iiindex table
**	      (where value is > #columns in base table).
**
** Explanation:
**	verifydb test #32 found that one of the table columns marked as a key
**	is invalid, because its value is larger than the number of columns in 
**	the table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless running in RUNINTERACTIVE mode -- then give yes/no
**	permission to drop this index table. (See DU0302.)
*/
S_DU162B_INVALID_DEVRELIDX
"Table id of %0d, Index table id of %1d\n
in iidevices does not match any table
listed in iirelation."
/*%
** Parameters:
**	%0c - devrelid from iidevices
**	%1c - devrelidx from iidevices
**
** Explanation:
**	verifydb test #33 found some tuples in iidevices that have table ID
**	and index table ID values that do not match any entries in iirelation.
**	this means that they are useless, and should be deleted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user must give yes/no permission to drop these tuples from
**	iidevices.  The user may want to construct an iirelation tuple
**	manually, rather than to delete these tuples from iidevices.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031A.)
*/
S_DU162C_LOCATIONS_EXIST
"Locations have been defined for table\n
%0c (owner %1c)\n
but iirelation does not indicate that they exist."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #34 found locations for this table defined in iidevices,
**	but iirelation.relstat.MUTLIPLE_LOC was set to false.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically set the MULTIPLE_LOC bit in iirelation's
**	relstat, unless verifydb is in RUNINTERACTIVE mode -- then the user
**	must grant yes/no permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031B.)
*/
S_DU162D_UNKNOWN_TREEMODE
"iitree table id= %0d, index id= %1d\n
for table/view %2c (owner %3c)\n
has unknown tree modes of %4d.\n
Valid values are:  view=17, protection=19, integrity=20,\n
                   register link = 101, create link = 100"
/*%
** Parameters:
**	%0d - base table ID associated with tree (from iitree.treetabbase)
**	%1d - base table index ID associated with tree (from iitree.treetabidx)
**	%2c - table/view name (from iirelation.relid)
**	%3c - table/view owner (from iirelation.relowner)
**	%4d - mode value (from iitree.treemode)
**
** Explanation:
**	verifydb found an entry in iitree that has a mode unknown to
**	the DBMS.  Of course, it is possible that new tree entries will be
**	created in later versions of the database (greater than 6.1) and
**	an older version of verifydb is being run.  Therefore, the correction
**	of deleting these tuples from the tree will only be performed in
**	interactive mode.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Give yes/no permission to delete these tuples from iitree.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031C.)
*/
S_DU162E_INVALID_TREEID
"iitree table id= %0d, index id= %1d \n
for %2c (owner %3c)\n
has invalid treeid.\n
Tree cannot be referenced by other system catalog tables."
/*%
** Parameters:
**	%0d - base table ID associated with tree (from iitree.treetabbase)
**	%1d - base table index ID associated with tree (from iitree.treetabidx)
**	%2c - table/view name (from iirelation.relid)
**	%3c - table/view name (from iirelation.relowner)
**
** Explanation:
**	verifydb found a tree ID that is invalid.  Since treeid is how
**	other system catalogs reference this tree, this tree cannot be used by
**	the DBMS.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If the user knows the correct tree ID value, s/he can replace in it the
**	effected iitree tuples.  In such a case, s/he should give no permission
**	to delete the tuples (RUNINTERACTIVE mode only).  Otherwise, the
**	tuples will be removed from iitree, and the user can get the original
**	query text from iiqrytext. (See DU031C.)
*/
S_DU162F_NO_TREEBASE
"The base table for tree entry does not exist.\n
(table id = %0d, index id = %1d"
/*%
** Parameters:
**	%0d - base table ID associated with tree (from iitree.treetabbase)
**	%1d - base table index ID associated with tree (from iitree.treetabidx)
**
** Explanation:
**	verifydb test #35 could not find an entry in iirelation with the same
**	(base, index) ID.  Views, permits, and integrities are tied to
**	an entry in iirelation, so if this tree defines a view, an integrity,
**	or a permit, it is useless and should be dropped.  Stored procedures are
**	not tied to an entry in iirelation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the tuples from iitree, unless
**	verifydb is in RUNINTERACTIVE mode, in which case it will prompt
**	the user for yes/no permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031C.)
*/
S_DU1630_NO_VIEW_RELSTAT
"iirelation does not indicate that\n
table %0c (owner %1c) is a view."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #38 found the iirelation tuple for the iitree entry did
**	not indicate that this is a view.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is in RUNINTERACTIVE mode, the correction of setting
**	the VIEW bit in iirelation.relstat is automatically performed.  If not,
**	then the user must give yes/no permission to make this correction when
**	prompted.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031D.)
*/
S_DU1631_NO_PROTECT_RELSTAT
"iirelation does not indicate that protections are defined for table\n
%0c (owner %1c)."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1s - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #38 found the iirelation tuple for the iitree entry did
**	not indicate that this table has protections defined for it, or
**	verifydb test #45 found the iirelation tuple for this permission did not
**	indicate that this table has permissions defined for it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is in RUNINTERACTIVE mode, the correction of setting
**	the PRTUPS bit in iirelation.relstat is automatically performed. If not,**	then the user must give yes/no permission to make this correction when
**	prompted.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031E.)
*/
S_DU1632_NO_INTEGS_RELSTAT
"iirelation does not indicate that integrities are defined for table\n
%0c (owner %1c)."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #38 found the iirelation tuple for the iitree entry did
**	not indicate that this table has integrities defined for it, or
**	verifydb test #41 found the iirelation tuple for this integrity did not
**	indicate that this table has integrities defined for it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is in RUNINTERACTIVE mode, the correction of setting
**	the INTEGS bit in iirelation.relstat is automatically performed. If not,**	then the user must give yes/no permission to make this correction when
**	prompted.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031F.)
*/
S_DU1633_MISSING_TREE_SEQ
"There is a sequence error in iitree for tree %0d %1d:\n
for table/view %2c (owner %3c)\n
Missing tuple for sequence #%4d"
/*%
** Parameters:
**	%0d - high portion of tree ID (from iitree.treeid1)
**	%1d - low portion of tree ID (from iitree.treeid2)
**	%2c - table/view name (from iirelation.relid)
**	%3c - table/view owner (from iirelation.relowner)
**	%4d - missing sequence number
**
** Explanation:
**	A tree entry may be comprised of more than one tuple in iitree. In such
**	a case, each tree entry has identical treetabbase, treetabidx, treeid1
**	and treeid2 values, but has unique (and sequential) treeseq values.
**	verifydb test #39 determined that one (or more) of the expected treeseq
**	values are missing.  NOTE: verifydb cannot determine if the last (or
**	last N) tuples are missing from the tree, as there is no place in iitree
**	that gives a count of the number of tree tuples expected.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop the tree tuples from iitree unless in
**	RUNINTERACTIVE mode, where user must give yes/no permission for such
**	a fix.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031C.)
*/
S_DU1634_NO_BASE_FOR_INTEG
"Base table id %0d, index id %1d\n
that integrity is defined for is not\n
listed in iirelation."
/*%
** Parameters:
**	%0d - base table ID (from iiintegrity.inttabbase)
**	%1d - index table ID (from iiintegrity.inttabidx)
**
** Explanation:
**	verifydb test #40 found that the table that is supposed to receive
**	this integrity doesn't exist (at least it's not listed in iirelation.)
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iiintegrity unless in RUNINTERACTIVE mode -- then the
**	user must enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0320.)
*/
S_DU1635_NO_TREE_FOR_INTEG
"WARNING: No tree exists for integrity %0d %1d\n
on table %2c (owner %3c).\n
The DBMS cannot enforce this integrity."
/*%
** Parameters:
**	%0d - high portion of treeid (from iiintegrity.inttreeid1)
**	%1d - low portion of treeid (from iiintegrity.inttreeid2)
**	%2c - table name (from iirelation.relid)
**	%3c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #42 found that the tree that defines this integrity
**	does not exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iiintegrity unless in RUNINTERACTIVE mode -- then the user
**	must enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0320.)
*/
S_DU1636_NO_INTEG_QRYTEXT
"The query text for integrity %0d %1d\n
(table %2c, owner %3c) does not exist."
/*%
** Parameters:
**	%0d - high portion of qryid (from iiintegrity.intqryid1)
**	%1d - low portion of qryid (from iiintegrity.intqryid2)
**	%2c - table name (from iirelation.relid)
**	%3c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #43 found that the query text used to define this 
**	integrity does not exist.  This does not hurt DBMS functioning, but it
**	affects some utility routines.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user may want to drop this integrity and redefine it, if s/he knows
**	what the definition should be.
*/
S_DU1637_NO_BASE_FOR_PERMIT
"Base table id %0d, index id %1d\n
that permit is defined for is not\n
listed in iirelation."
/*%
** Parameters:
**	%0d - base table ID (from iiprotect.protabbase)
**	%1d - index table ID (from iiprotect.protabidx)
**
** Explanation:
**	verifydb test #44 found that the table that is supposed to receive
**	this permit doesn't exist (at least it's not listed in iirelation.)
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iipermit unless in RUNINTERACTIVE mode -- then the user must
**	enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0321.)
*/
S_DU1638_NO_TREE_FOR_PERMIT
"No tree exists for permit %0d %1d\n
on table %2c (owner %3c).\n
The DBMS cannot enforce this integrity."
/*%
** Parameters:
**	%0d - high portion of treeid (from iipermit.protreeid1)
**	%1d - low portion of treeid (from iipermit.protreeid2)
**	%2c - table name (from iirelation.relid)
**	%3c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #46 found that the tree that defines this permit
**	does not exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iipermit unless in RUNINTERACTIVE mode -- then the user must
**	enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0321.)
*/
S_DU1639_NO_PERMIT_QRYTEXT
"The query text for permit %0d %1d\n
(table %2c, owner %3c) does not exist."
/*%
** Parameters:
**	%0d - high portion of qryid (from iiprotect.proqryid1)
**	%1d - low portion of qryid (from iiprotect.proqryid2)
**	%2c - table name (from iirelation.relid)
**	%3c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #44 found that the query text used to define this 
**	permit does not exist.  This does not hurt DBMS functioning, but it
**	affects some utility routines.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user may want to drop this permit and redefine it, if s/he knows
**	what the definition should be.
*/
S_DU163A_UNKNOWN_QRYMODE
"iiqrytext id %0d %1d has unknown modes of %2d.\n
Valid values are:  view=17, protection=19, integrity=20 stored_procedure=87\n
                   createlink=100, registerlink=101, \n
		   register_import=103, rule=110,security_alarm=131\n
"
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**	%2d - query mode (from iiqrytext.mode)
**
** Explanation:
**	verifydb test #48 found an entry in iiqrytext that has a mode unknown 
**	to the DBMS.  This is informational only, as the DBMS server does not
**	act on iiqrytext.  However, some DBMS utilities do act on it.  If 
**	possible, it is best to drop the view, permit, integrity, or stored
**	procedure and recreate it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU163B_INVALID_QRYTEXT_ID
"iiqrytext has entry in it with invalid query id of 0 0."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb test #49 found an entry in iiqrytext that has an invalid
**	query id of txtid1=0 and txtid2=0.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required; however, the user may want to manually drop these tuples
**	from iiqrytext.  The user may want to examine the txt field and
**	determine what txtid1 and txtid2 should be and manually correct them.
*/
S_DU163C_QRYTEXT_VIEW_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a view.  However, this view is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	verifydb test #50 was unable to find the view definition in iirelation
**	that corresponds to this view querytext. This is informational only, 
**	as the DBMS Server does not act on iiqrytext.  However, the user may
**	want to redefine the view to the database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU163D_QRYTEXT_PERMIT_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a permit.  However, this permit is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	verifydb test #50 was unable to find the permit definition in iirelation
**	that corresponds to this permit querytext. This is informational only, 
**	as the DBMS server does not act on iiqrytext.  However, the user may
**	want to redefine the permit to the database.  It is possible that the
**	table the permit is for does not exist in the database, but verifydb
**	is not able to determine this, as iiqrytext does not have the
**	equivalent of reltid and reltidx.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU163E_QRYTEXT_INTEG_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define an integrity.  However, this integrity is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	verifydb test #50 was unable to find the integrity definition in 
**	iirelation that corresponds to this integrity querytext. This is 
**	informational only, as the DBMS server does not act on iiqrytext.  
**	However, the user may want to redefine the integrity to the database.  
**	It is possible that the table the integrity is for does not exist in 
**	the database, but verifydb is not able to determine this, as iiqrytext 
**	does not have the equivalent of reltid and reltidx.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU163F_QRYTEXT_DBP_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a database procedure.  However, this DBP is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	verifydb test #52b was unable to find the database procedure definition
**	in iitree that corresponds to this DBP querytext. This is 
**	informational only, as the DBMS server does not act on iiqrytext.  
**	However, the user may want to redefine the DBP to the database.  
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1640_NONEXISTENT_DEID
"The table/view described in iidbdepends as\n
deid1 = %0d, deid2 = %1d\n
does not exist."
/*%
** Parameters:
**	%0d - dependent table id (from iidbdepends.deid1)
**	%1d - dependent table index id (from iidbdepends.deid2)
**
** Explanation:
**	verifydb test #54 found an entry in iidbdepends that describes a
**	view or table with permit as dependent on another table.  However,
**	the dependent table/view does not exist (or is not defined in
**	iirelation).
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iidbdepends tuple, unless it is
**	in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0322.)
*/
S_DU1641_NONEXISTENT_INDEP_TBL
"The table that %0c (owner %1c)\n
is dependent on doesn't exist."
/*%
** Parameters:
**	%0c - table name from iirelation.relid (of iirelation that matches
**		on deid1 and deid2)
**	%1c - table owner from iirelation.relowner (of iirelation that matches
**		on deid1 and deid2)
**
** Explanation:
**	verifydb test #55 found an entry in iidbdepends that describes a
**	view or table with permit as dependent on another table.  However,
**	the table/view that it is dependent on does not exist (or is not 
**	defined in iirelation)
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iidbdepends tuple, unless it is
**	in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0322.)
*/
S_DU1642_INID_NO_VBASE
"The independent table/view\n
%0c (owner %1c)\n
does not indicate a view is based on it."
/*%
** Parameters:
**	%0c - table name from iirelation.relid (of iirelation 
**		that matches on inid1 and inid2)
**	%1c - table owner from iirelation.relid (of 
**		iirelation that matches on inid1 and inid2)
**
** Explanation:
**	verifydb test #56 found an entry in iidbdepends that describes a
**	view as dependent on another table.  However, the table/view that it 
**	is dependent does not indicate that there is a view based on it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically set the VBASE bit in iirelation/relstat of
**	the independent table/view, unless it is in RUNINTERACTIVE mode. Then
**	it will prompt the user for yes/no permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0323.)
*/
S_DU1643_DEPENDENT_NOT_VIEW
"The dependent table %0c (owner %1c)\n
should be marked as a view, and it is not"
/*%
** Parameters:
**	%0c - table name from iirelation.relid (of iirelation that matches
**		on deid1 and deid2)
**	%1c - table owner from iirelation.relowner (of iirelation that matches
**		on deid1 and deid2)
**
** Explanation:
**	verifydb test #57 found an entry in iidbdepends that describes a
**	view as dependent on another table.  However, the iirelation entry for
**	this dependent view does not indicate that this is a view.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically set the VBASE bit in iirelation/relstat of
**	the independent table/view, unless it is in RUNINTERACTIVE mode. Then
**	it will prompt the user for yes/no permission.
**
**	No correction can be made if verifydb is in REPORT mode.  (See DU031D.)
*/
S_DU1644_MISSING_IIXDBDEPENDS
"system catalog iixdbdepends missing the entry for\n
Independent table:  %0c (owner %1c)\n
Dependent table:    %2c (owner %3c)."
/*%
** Parameters:
**	%0c - independent table name
**	%1c - independent table owner
**	%2c - dependent table name
**	%3c - dependent table owner
**
** Explanation:
**	verifydb test #58 found a table definition in iidbdepends that
**	does not have a corresponding entry in iixdbdepends.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	user must give yes/no permission to create a tuple in iixdbdepends for
**	this table.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0324.)
*/
S_DU1645_MISSING_IIDBDEPENDS
"Table with deid1 = %0d deid2=%1d\n
is defined in iixdbdepends, but is missing from iidbdepends."
/*%
** Parameters:
**	%0d -- dependent table ID (from iidbdpends.deid1)
**	%1d -- dependent table index ID (from iidbdpends.deid2)
**
** Explanation:
**	verifydb test #59 found an entry in the index table for a
**	nonexistent table -- i.e., there is no corresponding entry
**	in iidbdepends for this table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to drop this tuple from
**	iixdbdepends.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0325.)
*/
S_DU1646_INVALID_STATISTIC_COL
"Statistics for %0c (owner %1c)\n
are for column #%2d, but this table\n
only has %3d columns"
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	verifydb test #64 found that the statistics for this table were
**	performed on an invalid column number.  Since the utility that gathers
**	these statistics would not gather statistics on a non-existent column,
**	this indicates that the iistatistics table has probably been 
**	corrupted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The best user action is to re-run the optimizedb utility.  However,
**	verifydb will attempt to patch up the iistatistics table by deleting the
**	appropriate tuple(s) from iistatistics, unless it is in RUNINTERACTIVE
**	mode.  Then the user must supply yes/no permission to perform the fix.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0326.)
*/
S_DU1647_NO_OPSTAT_IN_BASE
"Table %0c (owner %1c)\n
does not indicate that it has optimizer statistics"
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	verifydb test #62 found that the iirelation entry for the table
**	that these statistics are based on does not indicate that there
**	are statistics for this table -- (i.e., iirelation.relstat's OPTSTAT
**	bit flag is not set).
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is in RUNINTERACTIVE mode, it will automatically
**	set the iirelation.relstat's OPTSTAT bit.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0327.)
*/
S_DU1648_MISSING_HISTOGRAM
"Mismatch in histogram cell count for table\n
%0c (owner %1c) on attribute #%2d\n
iistatistics expects histogram cells, but there are not any"
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**	%2d - attribute number
**	%3d - number of cells iistatistics thinks there are 
**		(from iistatistics.snumcells)
**	%4d - number of cells iihistogram has (actual count of tuples)
**
** Explanation:
**	verifydb test #64 found that table iistatistics indicate there is a
**	histogram for this table, but no histogram can be found in iihistogram.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is in RUNINTERACTIVE mode, it will automatically
**	modify the tuple in iistatistics to stop indicating that there is a
**	histogram.
**
**	No correction can be made if verifydb is in REPORT mode.  (See DU0328.)
*/
S_DU1649_STATS_ON_INDEX
"there are statistics for index table\n
%0c (owner %1c)\n
table id %2d, %3d.\n
Statistics on index tables are not permitted."
/*%
** Parameters:
**	%0c - index table name 
**	%1c - index table owner
**	%2d - base table ID (from iihistogram.htabbase)
**	%3d - index table ID (from iihistogram.htabindex)
**
** Explanation:
**	verifydb test #61 found statistics defined on an index table.  This
**	is not allowed.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iistatistics, unless it is in RUNINTERACTIVE mode. Then
**	the user must enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0326.)
*/
S_DU164A_NO_STATS_FOR_HISTOGRAM
"There are no statistics associated with this histogram:\n
htabbase = %0d, htabindex = %1d"
/*%
** Parameters:
**	%0d - base table ID (from iihistogram.htabbase)
**	%1d - index table ID (from iihistogram.htabindex)
**
** Explanation:
**	verifydb test #65 could not find the entry in iistatistics for this
**	histogram.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iihistogram, unless it is in RUNINTERACTIVE mode.  Then
**	the user must enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode.  (See DU0329.)
*/
S_DU164B_HISTOGRAM_SEQUENCE_ERR
"There is a sequence error in iihistogram for table\n
%0c (owner %1c) on attribute %2d.\n
Missing tuple for sequence #%3d.\n
This means that part of the histogram is missing."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner 
**	%2d - attribute number
**	%3d - missing sequence number
**
** Explanation:
**	A histogram entry may be comprised of more than one tuple in
**	iihistogram. In such a case, each histogram entry has unique
**	(and sequential) hsequence values.
**
**	verifydb test #66 determined that one (or more) of the expected
**	sequence values are missing.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user should probably rerun the optimizedb utility to regenerate this
**	histogram.  verifydb will delete the histogram tuples from iihistogram,
**	unless it is being run in RUNINTERACTIVE mode. Then the user must
**	input Yes/No.
**
**	No correction can be made if verifydb is in REPORT mode.  (See DU0329.)
*/
S_DU164C_NO_BASE_FOR_STATS
"Base table id %0d, index id %1d\n
that statistics are for is not\n
listed in iirelation."
/*%
** Parameters:
**	%0d - base table ID (from iistatistics.stabbase)
**	%1d - index table ID (from iistatistics.stabindex)
**
** Explanation:
**	verifydb test #60 found that the table that these statistics are
**	compiled for doesn't exist (at least it's not listed in iirelation.)
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iistatistics, unless it is in RUNINTERACTIVE mode. Then
**	the user must enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0326.)
*/
S_DU164D_INVAL_SYS_CAT_RELSPEC
"DBMS system catalog %0c has an invalid relspec of %1d"
/*%
** Parameters:
**	%0c - DBMS system catalog name
**	%1d - invalid relspec value (from iirelation.relspec)
**
** Explanation:
**	verifydb test #4 found a system catalog with an invalid relspec.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically set the relspec to the correct value,
**	unless it is in RUNINTERACTIVE mode.  Then the user must enter
**	YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU032A
**	and DU032B.)
*/
S_DU164E_DUPLICATE_TREE_SEQ
"There is a sequence error in iitree for tree %0d %1d:\n
for table/view %2c (owner %3c)\n
Duplicate tuple for sequence #%4d"
/*%
** Parameters:
**	%0d - high portion of tree ID (from iitree.treeid1)
**	%1d - low portion of tree ID (from iitree.treeid2)
**	%2c - table/view name (from iirelation.relid)
**	%3c - table/view owner (from iirelation.relowner)
**	%4d - duplicate sequence number
**
** Explanation:
**	A tree entry may be comprised of more than one tuple in iitree. In such
**	a case, each tree entry has identical treetabbase, treetabidx, treeid1
**	and treeid2 values, but has unique (and sequential) treeseq values.
**
**	verifydb test #39 determined that one (or more) of the expected treeseq
**	values are duplicated.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop the tree tuples from iitree, unless
**	it is in RUNINTERACTIVE mode, where the user must give yes/no
**	permission for such a fix.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031C.)
*/
S_DU164F_QRYTEXT_SEQUENCE_ERR
"There is a sequence error in iiqrytext for query id %0d %1d:\n
Missing tuple for sequence #%2d"
/*%
** Parameters:
**	%0d - high portion of query ID (from iiqrytext.txtid1)
**	%1d - low portion of query ID (from iiqrytext.txtid2)
**	%2d - missing sequence number
**
** Explanation:
**	A query entry may be comprised of more than one tuple in iiqrytext.  
**	In such a case, each query entry has identical txtid1, txtid2, and
**	mode values, but has unique (and sequential) seq values.
**
**	verifydb test #53 determined that one (or more) of the expected sequence
**	values are missing.  NOTE: verifydb cannot determine if the last (or
**	last N) tuples are missing from the query, as there is no place in 
**	iiqrytext that gives a count of the number of query tuples expected.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required; however, the user may want to manually fix this, if the
**	user knows how the query should read.  The user may want to delete the
**	other tuples for this query, if the user does not know how query should
**	read.
*/
S_DU1650_QRYTEXT_DUP_ERR
"There is a sequence duplication error in iiqrytext for\n
query id %0d %1d:\n
Duplicate tuple for sequence #%2d"
/*%
** Parameters:
**	%0d - high portion of query ID (from iiqrytext.txtid1)
**	%1d - low portion of query ID (from iiqrytext.txtid2)
**	%2d - missing sequence number
**
** Explanation:
**	A query entry may be comprised of more than one tuple in iiqrytext.  
**	In such a case, each query entry has identical txtid1, txtid2, and
**	mode values, but has unique (and sequential) sequence values.
**
**	verifydb test #53 determined that one (or more) of the expected seq
**	values are duplicated.  NOTE: verifydb cannot determine if the last (or
**	last N) tuples are missing from the query, as there is no place in 
**	iiqrytext that gives a count of the number of query tuples expected.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required; however, the user may want to fix manually, if the user
**	knows how the query should read.  The user may want to delete the other
**	tuples for this query, if the user does not know how the query should
**	read.
*/
S_DU1651_INVALID_IIHISTOGRAM
"There are no statistics for any of the tables in\n
this database.  However, there are histograms."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The tuples in iihistogram are used in conjunction with tuples in
**	iistatistics.  If there are no tuples in iistatistics, then all of
**	the tuples in iihistogram are meaningless.
**
** System Status:
**	The verifydb process continues. However, it is unable to check each
**	tuple in iihistogram, because the iihistogram logic requires the
**	iistatistics cache, and there is no cache for iistatistics. Therefore,
**	verifydb does not check the status of each tuple in iihistogram.
**
** Recommendation:
**	verifydb will automatically delete all tuples from iihistogram
**	in RUN or RUNSILENT modes.  It will prompt the user for permission
**	to drop all iihistogram tuples in RUNINTERACTIVE mode.  The user
**	must give or withhold permission.
*/
S_DU1652_HISTOGRAM_DUP_ERR
"There is a sequence error in iihistogram for table\n
%0c (owner %1c) on attribute %2d.\n
Duplicate tuple for sequence #%3d."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner 
**	%2d - attribute number
**	%3d - missing sequence number
**
** Explanation:
**	A histogram entry may be comprised of more than one tuple in
**	iihistogram. In such a case, each histogram entry has unique
**	(and sequential) hsequence values.
**
**	verifydb test #66 determined that one (or more) of the expected sequence
**	values are missing.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user should probably rerun the optimizedb utility to regenerate this
**	histogram.  verifydb will delete the histogram tuples from iihistogram
**	unless it is in RUNINTERACTIVE mode.  Then the user must input Yes/No.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0329.)
*/
S_DU1653_RUN_VERIFYDB
"There are some dependencies on table %0c\n
 (which is being dropped).\n
 To clean them up, run:\n
	VERIFYDB -mruni -odbms -sdbn %1c "
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	verifydb is dropping this table.  This will leave the database in an
**	inconsistent state, because there will be a dependency on this table
**	in the system.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user must run verifydb to clean up any inconsistencies left over
**	after this table is dropped.
*/
S_DU1654_INVALID_USERNAME
"INGRES user name '%0c' is invalid."
/*%
** Parameters:
**	%0c -- user name
**
** Explanation:
**	The user name has characters in it which are not legal for a
**	user name.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The system administrator should run accessdb to see if any users have
**	been dropped.  If so, use accessdb to redefine them.
*/
S_DU1655_INVALID_USERSTAT
"Permissions for INGRES user '%0c' are corrupted."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	iiuser.status has invalid bits set.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will clear invalid bits.  However, there is a good chance that
**	the user's permissions may not be right if there were invalid bits in
**	the permission bitmap.  The system administrator should run accessdb to
**	assure that the user has correct permissions.
*/
S_DU1656_INVALID_DBNAME
"Database Name '%0c'
(owned by '%1c') is invalid."
/*%
** Parameters:
**	%0c - database name
**	%0c - DBA
**
** Explanation:
**	The name of the database has been corrupted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	In this case, the user should run finddbs to get the database back.
**	However, the user must know which locations to tell finddbs to search
**	and may also need to use accessdb to assign correct privileges to the 
**	database.
*/
S_DU1657_INVALID_DBDEV
"The default location name (%2c)\n
for database '%0c'\n
(owned by '%1c') is invalid."
/*%
** Parameters:
**	%0c - database name
**	%1c - DBA
**	%2c - default location name (from iidatabase.dbdev)
**
** Explanation:
**	The database's default data location is not known to the installation.
**	This means that the server will not be able to find the database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	If the user thinks that the location is valid, they should withhold
**	permission to drop this database.  Then they should add the location
**	via accessdb.
*/
S_DU1658_INVALID_DBDEV
"Database '%0c' is owned by non-existent\n
INGRES user: %1c."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	The database's owner is not a valid user.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	If the user thinks that the owner should exist, they should not give
**	permission to delete the database and should use accessdb to define
**	the owner as a valid user.
*/
S_DU1659_MISSING_IIEXTEND
"The default data location of %2c\n
is not defined in the iiextend catalog for
database %0c (owner %1c)."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**	%2c - default location
**
** Explanation:
**	Every database has at least one entry in iiextend for its default
**	location.  There should be an entry for this database (its default
**	location has already been validated), but there is not.  The correct
**	fix is to put the default location (from iidatabase.dbdev) into the
**	iiextend table.  verifydb will automatically do this for the user 
**	unless it is being run in RUNINTERACTIVE mode.  In that case, the
**	user will be prompted for yes/no permission.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	No action is required unless verifydb is in RUNINTERACTIVE mode. In
**	that case, the user must enter Yes/No permission before verifydb will
**	take any action.
*/
S_DU165A_INVALID_CKPDEV
"The checkpoint location name (%2c)\n
for database '%0c'\n
(owned by '%1c') is invalid."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - DBA (from iidatabase.own)
**	%2c - checkpoint location name (from iidatabase.ckpdev)
**
** Explanation:
**	The database's checkpoint location in not known to the installation.
**	This means that the server will not be able to checkpoint this database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	If the user thinks that the location is valid, they should withhold
**	permission to drop this database.  Then they should go in via
**	accessdb and add the location.
*/
S_DU165B_INVALID_JNLDEV
"The journal location name (%2c)\n
for database '%0c'\n
(owned by '%1c') is invalid."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - DBA (from iidatabase.own)
**	%2c - journal location name (from iidatabase.jnldev)
**
** Explanation:
**	The database's journal location in not known to the installation.
**	This means that the server will not be able to keep a journaling log
**	for this database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	If the user thinks that the location is valid, they should withhold
**	permission to drop this database.  Then they should go in via
**	accessdb and add the location.
*/
S_DU165C_INVALID_SORTDEV
"The work location name (%2c)\n
for database '%0c'\n
(owned by '%1c') is invalid."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - DBA (from iidatabase.own)
**	%2c - work location name (from iidatabase.sortdev)
**
** Explanation:
**	The database's work location in not known to the installation.
**	This means that the server will not be able to use temporary storage
**      for temp tables and sorts for this database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	If the user thinks that the location is valid, they should withhold
**	permission to drop this database.  Then they should go in via
**	accessdb and add the location.
*/
S_DU165D_DBA_HAS_NO_ACCESS
"DBA %0c does not have access to\n
private DB %1c."
/*%
** Parameters:
**	%0c - database owner
**	%1c - database name
**
** Explanation:
**	The only way to access a private database is to be explicitly
**	permitted to access the database via a tuple in iidbaccess. createdb 
**	puts a tuple in iidbaccess for the DBA of a private database at
**	creation time.  Any other users are given access to that database via
**	the accessdb utility.
**
**	It is not reasonable for the owner of the private database to be
**	denied access to that database. Therefore, the missing iidbaccess tuple
**	must be an error, and verifydb will automatically correct that,
**	unless in RUNINTERACTIVE mode.  In that case, the user will be 
**	prompted for yes/no permission.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	No action is required unless verifydb is in RUNINTERACTIVE mode. In
**	that case, the user must enter Yes/No permission before verifydb will
**	take any action.
*/
S_DU165E_BAD_DB
"WARNING: Database %0c (owner %1c)\n
is inoperative.  Use DESTROYDB to remove it from the installation."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	The database is not usable.  Either createdb failed, or destroydb
**	failed.  In either case, the user should run destroydb to remove
**	it from the installation.  If they want a database by that name, they
**	can use createdb to re-create it after they have destroyed the
**	inoperative version.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	No action is required.  However, the user may want to remove the
**	inoperative database from the installation with destroydb.
*/
S_DU165F_UNCONVERTED_DB
"WARNING: Database %0c (owner %1c)\n
has not been converted yet.  Use the proper conversion utility\n
to make it operative."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	The database is not compatible with the installation's server.
**	The user may want to convert the database, or they may want to
**	leave it at its current level.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	No action is required.
*/
S_DU1660_INCOMPATIBLE_DB
"WARNING: Database %0c (owner %1c)\n
is not compatible with the INGRES server."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	The database is not marked as operative, or as a conversion candidate,
**	and the database compatibility level is not valid.  This is a very
**	serious corruption.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Try running "finddbs -r".  If that does not pick up the database
**	correctly, get the database back from a checkpoint/rollforward.
**	If there was not any checkpoint/journaling on the database, the
**	user should call Ingres Corporation Technical Support for help. 
**	They may be able to manipulate the iidbdb catalogs to permit 
**	the server to access the database.  IMPORTANT: In this case it 
**	is a must to unload the database, destroy it, recreate it, and 
**	reload it.
*/
S_DU1661_INOPERATIVE_DB
"Database %0c (owner %1c)\n
is not marked operative."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	The database is not marked operative, but there is not any apparent
**	reason why it should not be marked operative.  However, a database may
**	only be marked operative in RUNINTERACTIVE mode if the user gives YES
**	permission.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user must give Yes/No permission in response to the prompt.
*/
S_DU1662_INCOMPAT_MINOR
"WARNING: Database %0c (owner %1c)\n
has invalid minor compatibility level of %2d."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**	%2d - minor compatibility level
**
** Explanation:
**	The database has an unreasonable minor compatibility level.  The minor
**	compatibility level is how the server keeps track of what the latest 
**	changes in the database are.
**
**	This can be a serious or trivial problem, depending on the reason the
**	compatibility level is invalid.  The first thing to verify is that
**	the user is running with the correct version of verifydb.  This is the
**	most likely reason for this type of error, if there is not any other
**	corruption to the iidatabase table.
**
**	If the user is running with the correct version of verifydb, then
**	the user should delete this tuple from iidatabase and run "finddbs -r"
**	to get the information back from the config file.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Try running "finddbs -r".  If that does not pick up the database
**	correctly, get the database back from a checkpoint/rollforward.
**	If there was not any checkpoint/journaling on the database, the
**	user should call Ingres Corporation Technical Support for assistance. 
*/
S_DU1663_ZERO_DBID
"WARNING: Database %0c (owner %1c)\n
has invalid database id of 0."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	Each database should have a unique, nonzero database ID.  The server
**	uses the database ID to keep track of which database is associated with
**	a specific processing thread.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Try running "finddbs -r".  If that does not pick up the database
**	correctly, get the database back from a checkpoint/rollforward.
*/
S_DU1664_DUPLICATE_DBID
"Database %0c (owned by %1c)\n
has a database id of of %2d.\n
This value should be unique but is not.  Run\n
  finddbs -r\n
to fix that problem."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**	%2d - database ID
**
** Explanation:
**	Each database should have a unique, nonzero database ID.  The server
**	uses the database ID to keep track of which database is associated with
**	a specific processing thread.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Try running "finddbs -r".  If that does not pick up the database
**	correctly, get the database back from a checkpoint/rollforward.
*/
S_DU1665_NO_2NDARY_INDEX
"Database %0c (owned by %1c)\n
does not have an entry in secondary index dbid_idx."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	The iidatabase table has a secondary index named iidbid_idx.  Each tuple
**	in iidatabase must have a corresponding entry in iidbid_idx.  Either the
**	entry is missing, or the tidp for the entry is incorrect.  In either
**	case, iidbid_idx must be updated to match iidatabase.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then then user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1666_NO_IIDATABASE_ENTRY
"There is no entry in iidatabase to correspond to the iidbid_idx\n
tuple for db_id = %0d"
/*%
** Parameters:
**	%0d - database ID (from iidbid_idx.db_id)
**
** Explanation:
**	The iidatabase table has a secondary index named iidbid_idx.  Each tuple
**	in iidatabase must have a corresponding entry in iidbid_idx.  Each tuple
**	in iidbid_idx must have a corresponding entry in iidatabase.  In this
**	case, there was a tuple in iidbid_idx with no corresponding tuple in
**	iidatabase; verifydb will remove the entry form iidbid_idx.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1667_WRONG_IIDBIDIDX_TID
"The secondary index iidbid_idx (indexes iidatabase) has the wrong tid pointer\n
for database %0c (owner %1c).\n
The tidp should be %2d, but is %3d."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - database owner (from iidatabase.own)
**	%1d - TID associated with iidatabase tuple
**	%2d - iidbid_idx.tidp 
**
** Explanation:
**	The iidatabase table has a secondary index named iidbid_idx.  Each tuple
**	in iidatabase must have a corresponding entry in iidbid_idx.  Each tuple
**	in iidbid_idx must have a corresponding entry in iidatabase.  In this
**	case, there was a tuple in iidbid_idx had the wrong value for the TID
**	pointer; verifydb will update iidbid_idx.tidp to the correct value.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1668_NONUSER_WITH_ACCESS
"User '%0c' has access to private database '%1c'.\n
However, this user is not a valid user."
/*%
** Parameters:
**	%1c - private database name (from iidbaccess.dbname)
**	%0c - user (from iidbaccess.usrname)
**
** Explanation:
**	Table iidbaccess is used to specify which users should have
**	access to a private database.  In this case, one of the users specified
**	is not a valid user in this installation.  verifydb will
**	automatically remove access for this nonexistent user from this
**	database, unless it is being run in RUNINTERACTIVE mode.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1669_NOSUCH_DB
"User '%0c' has access to private database '%1c'.\n
However, there is no such database in this installation."
/*%
** Parameters:
**	%0c - user (from iidbaccess.usrname)
**	%1c - private database name (from iidbaccess.dbname)
**
** Explanation:
**	Table iidbaccess is used to specify which users should have
**	access to a private database.  In this case, the specified database
**	does not exist in this installation. verifydb will automatically
**	remove access for this user to the nonexistent database, unless it
**	is being run in RUNINTERACTIVE mode.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU166A_PRIV_NOT_GLOBAL
"Private database '%1c'\n
(owned by '%0c').\n
Is not marked as a private database."
/*%
** Parameters:
**	%0c - user (from iidbaccess.usrname)
**	%1c - private database name (from iidbaccess.dbname)
**
** Explanation:
**	Table iidbaccess is used to specify which users should have
**	access to a private database.  In this case, there is an entry in
**	iidbaccess for the owner of the database, which indicates a private
**	database.  However, the iidatabase has this database marked as GLOBAL
**	(public).  The database should be private if there is a valid entry for
**	it in iidbaccess.
**
**	verifydb will automatically mark the database as a private database,
**	unless it is being run in RUNINTERACTIVE mode.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU166B_INVALID_LOCNAME
"'%0c' is not a valid location name.  (Associated area = \n
'%1c')"
/*%
** Parameters:
**	%0c - location logical name (from iilocations.lname)
**	%1c - physical translation of logical location name
**		(from iilocations.area)
**
** Explanation:
**	Table iilocation keeps a list of location names and what
**	these names translate to.  The user never refers to the translated
**	name, they always use the logical name to refer to a location. accessdb
**	is the only utility that permits a user to associate logical and
**	physical names.  accessdb puts the logical/physical pair into the
**	config file when a database is extended to a specific location.  It
**	uses the iilocations tuple to get logical/physical values for this
**	location.
**
**	In this case, the logical name has been corrupted to the point that
**	the DBMS cannot use it to look up the physical area associated with the
**	logical.
**
**	verifydb will automatically remove the tuple from iilocations, unless
**	it is being run in RUNINTERACTIVE mode.  It will print the physical
**	name (or area) associated with the logical in the verifydb log.  The
**	user should use accessdb to redefine the location.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission
**	before verifydb can take corrective action.
**
**	After the corrective action is taken, the user should use accessdb to
**	redefine the location.  NOTE: Because the location information is in
**	the config file, the user must use the same logical/physical
**	names that the location was previously defined at.  If the user
**	does not know how the location should be defined, they should do
**	an ASCII dump on the config file to find out.  The config file resides
**	in the database's default data location and is file aaaaaaaa.cnf.
*/
S_DU166C_BAD_LOC_STAT
"Location '%0c' has spurious bits\n
set in the location status word.  Status = %1d."
/*%
** Parameters:
**	%0c - logical name of the location (from iilocations.lname)
**	%1d - value of location status (from iilocations.status)
**
** Explanation:
**	The status field indicates the type of location (DATA, WORK, CKP, etc).
**	Some of the bits that are not used have been set.  This means that the
**	entire status for the location may be invalid, and the user should
**	use accessdb to look at the location and ensure that it is defined
**	correctly.
**
**	verifydb will clear unused bits (to avoid confusing accessdb), but this
**	does not ensure that the location status is correct.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU166D_NOSUCH_DB
"iiextend indicates that database '%0c'\n
is at location '%1c'.\n
However, that database does not exist."
/*%
** Parameters:
**	%0c - database name (from iiextend.dname)
**	%1c - location name (from iiextend.lname)
**
** Explanation:
**	There is a tuple in iiextend for each location of each database in
**	the installation.  If the database resides in only the default
**	location, there will be only one entry.  If the database has been
**	extended to three locations, there will be 4 entries (one for the
**	default location, and one for each of the three extended locations).
**	Each entry consists of a database name and a location name.  The
**	location name must also be in iilocations to be meaningful.  The
**	database name must be in iidatabase to be meaningful.
**
**	Each location in iiextend for a database also resides in the database
**	configuration file (in the database's default location.)  The config
**	file is what the server uses to find extended locations, but table
**	iiextend is what accessdb uses to define/display them.  Also, the
**	server uses the entry for the default location from iidatabase.dbdev
**	to find the config file.  This table must be in sync with the
**	config file and with table iidatabase.
**
**	In this case, the database name is not in iidatabase, or has been
**	dropped by verifydb.
**
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission before
**	verifydb can take corrective action.
**
**	After verifydb has taken corrective action, the user should run accessdb
**	to ensure the database is defined correctly.
*/
S_DU166E_NOSUCH_LOC
"iiextend indicates that database '%0c'\n
is at location '%1c'.\n
However, that location does not exist."
/*%
** Parameters:
**	%0c - database name (from iiextend.dname)
**	%1c - location name (from iiextend.lname)
**
** Explanation:
**	There is a tuple in iiextend for each location of each database in
**	the installation.  If the database resides in only the default
**	location, there will be only one entry.  If the database has been
**	extended to three locations, there will be 4 entries (one for the
**	default location, and one for each of the three extended locations).
**	Each entry consists of a database name and a location name.  The
**	location name must also be in iilocations to be meaningful.  The
**	database name must be in iidatabase to be meaningful.
**
**	Each location in iiextend for a database also resides in the database
**	configuration file (in the database's default location.)  The config
**	file is what the server uses to find extended locations, but table
**	iiextend is what accessdb uses to define/display them.  Also, the
**	server uses the entry for the default location from iidatabase.dbdev
**	to find the config file.  This table must be in sync with the
**	config file and with table iidatabase.
**
**	In this case, the location name is not in iilocations, or has been
**	dropped by verifydb.
**
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission before
**	verifydb can take corrective action.
**
**	After verifydb has taken corrective action, the user should run accessdb
**	to ensure the database is defined correctly.
*/
S_DU166F_NONUNIQUE_SYNONYM
"WARNING: invalid synonym name '%0c' (owner %1c).\n
This synonym name is invalid because there is a table, view or index
with the same name that is owned by the same user."
/*%
** Parameters:
**	%0c - synonym name (from iisynonym.synonym_name)
**	%1c - synonym owner (from iisynonym.synonym_owner)
**
** Explanation:
**	The synonym name, owner name pair must be a unique identifier that
**	is an alias to a table, index, or view name.  In this case, it is not
**	a unique identifier, because there is a table, index, or view with the
**	same name and same owner.
**
**	verifydb will recommend dropping the tuple from the iisynonym catalog
**	if this condition is encountered.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission before
**	verifydb can take corrective action.
*/
S_DU1670_INVALID_SYNONYM
"WARNING: synonym '%0c' (owned by '%1c')\n
resolves to a table that does not exist.\n
Table base id: %2d, table index id: %3d."
/*%
** Parameters:
**	%0c - synonym name (from iisynonym.synonym_name)
**	%1c - synonym owner (from iisynonym.synonym_owner)
**	%2d - table base ID (from iisynonym.syntabbase)
**	%3d - table index ID (from iisynonym.syntabindex)
**
** Explanation:
**	There was not an entry in iirelation for a table with reltid=syntabbase
**	and reltidx=syntabindex.  This means that the synonym resolves to a
**	nonexistent table.
**
**	verifydb will recommend dropping the tuple from the iisynonym catalog
**	if this condition is encountered.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission before
**	verifydb can take corrective action.
*/
S_DU1671_NOT_INDEX
"WARNING:  table '%0c' (owner %1c)\n
is marked as a secondary index, but is is not."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	The iirelation.relstat bit, TCB_INDEX, was set, but iirelation.reltidx
**	was zero. iirelation.reltidx is always nonzero for secondary indexes,
**	so this must not really be a secondary index.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission before
**	verifydb can take corrective action.  Otherwise, no action is required.
*/
S_DU1672_MISSING_IIINDEX
"Warning:  secondary index '%0c' (owner %1c)\n
is not entered in system catalog iiindex."
/*%
** Parameters:
**	%0c - secondary index name
**	%1c - secondary index owner
**
** Explanation:
**	All secondary indexes need to be entered in iiindex. Since this tuple
**	is not in iiindex, this secondary index should be dropped.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must give Yes/No permission before
**	verifydb can take corrective action.  Otherwise, no action is required.
*/
S_DU1673_NO_RULES_RELSTAT
"iirelation does not indicate that rules are defined for table\n
%0c (owner %1c)."
/*%
** Parameters:
**	%0s - table name (from iirelation.relid)
**	%1s - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #38 found the iirelation tuple for the iitree entry did
**	not indicate that this table has rules defined on it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting PRTUPS bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU031E.)
*/
S_DU1674_QRYTEXT_RULE_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a rule.  However, this rule is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	A verifydb test was unable to find the rule definition in 
**	iirule that corresponds to this rule querytext. This is an
**	informational message only, since the DBMS server does not act
**	on iiqrytext.  
**
**	However, the user may want to redefine the rule to the database.  
**	It is possible that the table the rule is for does not exist in 
**	the database, but verifydb is not able to determine this, as iiqrytext 
**	does not have the equivalent of reltid and reltidx.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1675_QRYTEXT_EVENT_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a dbevent.  However, this dbevent is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	verifydb was unable to find the dbevent definition in 
**	iievent that corresponds to this dbevent querytext. This is 
**	an informational message only, since the DBMS server does not
**	act on iiqrytext.  
**
**	However, the user may want to redefine the dbevent to the database.  
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1676_NOSUCH_DDB
"Distributed database '%0c' is known to Star but not to this installation."
/*%
** Parameters:
**	%0c - distributed database name (from iistar_cdbs.ddb_name)
**
** Explanation:
**	Table iistar_cdbs is used to specify a distributed database to its
**	coordinator database.  In this case, the specified distributed database
**	does not exist in this installation.
**
**	verifydb will automatically remove the entry from iistar_cdbs, unless
**	it is being run in RUNINTERACTIVE mode.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1677_NOSUCH_CDB
"Coordinator database '%0c' is known to Star but not to this installation."
/*%
** Parameters:
**	%0c - coordinator database name (from iistar_cdbs.cdb_name)
**
** Explanation:
**	Table iistar_cdbs is used to specify a distributed database to its
**	coordinator database.  In this case, the specified coordinator database
**	does not exist in this installation.
**
**	verifydb will automatically remove the entry from iistar_cdbs, unless
**	it is being run in RUNINTERACTIVE mode.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1678_WRONG_DDB_OWNER
"Distributed database '%0c' should be owned by '%1c' but is owned by '%2c'."
/*%
** Parameters:
**	%0c - distributed database name (from iistar_cdbs.ddb_name)
**	%1c - distributed database owner (as per iidatabase)
**	%2c - distributed database owner (as per iistar_cdbs)
**
** Explanation:
**	Table iistar_cdbs is used to specify a distributed database to its
**	coordinator database.  In this case, the DBA of the distributed
**	database in iistar_cdbs does not agree with the DBA name from the
**	iidatabase entry.
**
**	Unless it is being run in RUNINTERACTIVE mode, verifydb will
**	automatically replace the erroneous value in iistar_cdbs with the
**	value from iidatabase,
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU1679_WRONG_CDB_OWNER
"Coordinator database '%0c' should be owned by '%1c' but is owned by '%2c'."
/*%
** Parameters:
**	%0c - coordinator database name (from iistar_cdbs.ddb_name)
**	%1c - coordinator database owner (as per iidatabase)
**	%2c - coordinator database owner (as per iistar_cdbs)
**
** Explanation:
**	Table iistar_cdbs is used to specify a distributed database to its
**	coordinator database.  In this case, the DBA of the coordinator
**	database in iistar_cdbs does not agree with the DBA name from the
**	iidatabase entry.
**
**	Unless it is being run in RUNINTERACTIVE mode, verifydb will
**	automatically replace the erroneous value in iistar_cdbs with the
**	with the value from iidatabase.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU167A_WRONG_CDB_ID
"Coordinator database '%0c' should have unique id of '%1d' but has '%2d'."
/*%
** Parameters:
**	%0c - coordinator database name (from iistar_cdbs.ddb_name)
**	%1c - coordinator database unique ID (as per iidatabase)
**	%2c - coordinator database unique ID (as per iistar_cdbs)
**
** Explanation:
**	Table iistar_cdbs is used to specify a distributed database to its
**	coordinator database.  In this case, the unique ID for the coordinator
**	database in iistar_cdbs does not agree with the unique ID from the
**	iidatabase entry.
**
**	Unless it is being run in RUNINTERACTIVE mode, verifydb will
**	automatically replace the erroneous value in iistar_cdbs with the
**	with the value from iidatabase.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU167B_NO_IISTARCDBS_ENTRY
"There is no entry in iistar_cdbs corresponding to the iicdbid_idx secondary \n
index for a cdb_id of '%0d'"
/*%
** Parameters:
**	%0c - coordinator database unique identifier (from iicdbid_idx)
**
** Explanation:
**	Table iistar_cdbs has a secondary index, iicdbid_idx, on cdb_id.
**	The value of cdb_id in the secondary index does not correspond to any
**	tuples in base table iistar_cdbs.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None, unless in RUNINTERACTIVE mode.  Then the user must give Yes/No
**	permission before verifydb can take corrective action.
*/
S_DU167C_QRYTEXT_DBP_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a database procedure.  However, this DBP is missing from the database."
/*%
** Parameters:
**	%0d - low portion of query ID (from iiqrytext.txtid1)
**	%1d - high portion of query ID (from iiqrytext.txtid2)
**
** Explanation:
**	A verifydb test was unable to find the database procedure definition in 
**	iiprocedure that corresponds to this procedure querytext. This is an
**	informational message only, since the DBMS server does not act on
**	iiqrytext.  
**
**	However, the user may want to redefine the procedure to the database.  
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU167D_NO_RULES
"iirelation indicates that there are rules for\n
table %0c (owner %1c),\n
but none are defined."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb test #27 found that there were not really rules defined
**	for this table, but iirelation thought that there were.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None required, unless running in RUNINTERACTIVE mode -- then enter
**	yes/no permission for request to clear RULES bit from iirelation.relstat
**	after being prompted for permission.  
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0342.)
*/
S_DU167E_NO_TXT_FOR_EVENT
"There is no query text definition for event %0c (owner %1c)"
/*%
** Parameters:
**	%0c - event name
**	%1c - event owner 
**
** Explanation:
**	verifydb found that there was no entry in iiqrytext for this event.
**
** System Status:
**	verifydb wants to drop the event, but may only do so in RUNINTERACTIVE
**	mode.  If not in RUNINTERACTIVE mode, verifydb will not take any
**	corrective action.  If in RUNINTERACTIVE mode, verifydb will prompt
**	the user for permission to drop this rule.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU167F_EVENTID_NOT_UNIQUE
"The unique identifier for event %0c (owner %1c)\n
is not unique, and has a value of ['%2d','%3d]"
/*%
** Parameters:
**	%0c - event name
**	%1c - event owner 
**	%2d - event identifier base value
**	%2d - event identifier index value
**
** Explanation:
**	verifydb found a tuple in iirelation with the same unique identifier
**	that iievent has for this event.  This means that the event identifier
**	is not actually unique.
**
** System Status:
**	verifydb wants to drop the event, but may only do so in RUNINTERACTIVE
**	mode.  If not in RUNINTERACTIVE mode, verifydb will not take any
**	corrective action.  If in RUNINTERACTIVE mode, verifydb will prompt
**	the user for permission to drop this rule.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1680_NO_TXT_FOR_DBP
"There is no query text definition for database procedure %0c (owner %1c)"
/*%
** Parameters:
**	%0c - procedure name
**	%1c - procedure owner 
**
** Explanation:
**	verifydb found that there was no entry in iiqrytext for this
**	database procedure.
**
** System Status:
**	verifydb wants to drop the procedure, but may only do so in
**	RUNINTERACTIVE mode.  If not in RUNINTERACTIVE mode, verifydb
**	will not take any corrective action.  If in RUNINTERACTIVE mode,
**	verifydb will prompt the user for permission to drop this procedure.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1681_DBPID_NOT_UNIQUE
"The unique identifier for DB Procedure %0c (owner %1c)\n
is not unique, and has a value of ['%2d','%3d]"
/*%
** Parameters:
**	%0c - database procedure name
**	%1c - database procedure owner
**	%2d - event identifier base value
**	%2d - event identifier index value
**
** Explanation:
**	verifydb found a tuple in iirelation with the same unique identifier
**	that iiprocedure has for this database procedure.  This means that the
**	procedure identifier is not actually unique.
**
** System Status:
**	verifydb wants to drop the procedure, but may only do so in
**	RUNINTERACTIVE mode.  If not in RUNINTERACTIVE mode, verifydb
**	will not take any corrective action.  If in RUNINTERACTIVE mode,
**	verifydb will prompt the user for permission to drop this procedure.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1682_NO_BASE_FOR_RULE
"Rule %0c (owned by %1c) references a table that does not exist."
/*%
** Parameters:
**	%0c - rule name
**	%1c - rule owner
**
** Explanation:
**	verifydb found that the table that is supposed to receive this rule
**	doesn't exist (at least it's not listed in iirelation.)
**
** System Status:
**	verifydb wants to drop the rule, but may only do so in
**	RUNINTERACTIVE mode.  If not in RUNINTERACTIVE mode, verifydb
**	will not take any corrective action.  If in RUNINTERACTIVE mode,
**	verifydb will prompt the user for permission to drop this rule.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1683_NO_RULE_RELSTAT
"iirelation does not indicate that rules are defined for table\n
%0c (owner %1c)."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb found the iirelation tuple for the rule did not indicate that
**	this table has rules defined for it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU1684_NO_RULE_QRYTEXT
"The query text %0d %1d\n
for rule %2c (owner %3c) does not exist."
/*%
** Parameters:
**	%0d - high portion of qryid
**	%1d - low portion of qryid
**	%2c - rule name
**	%3c - rule owner
**
** Explanation:
**	verifydb found that the query text used to define this rule does not
**	exist.
**
** System Status:
**	verifydb wants to drop the rule, but may only do so in
**	RUNINTERACTIVE mode.  If not in RUNINTERACTIVE mode, verifydb
**	will not take any corrective action.  If in RUNINTERACTIVE mode,
**	verifydb will prompt the user for permission to drop this rule.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1685_NO_DBP_FOR_RULE
"Procedure %0c (owner %1c) referenced by \n
rule %2c (owner %3c) does not exist."
/*%
** Parameters:
**	%0c - procedure name
**	%1c - procedure owner
**	%2c - rule name
**	%3c - rule owner
**
** Explanation:
**	verifydb test #43 found that the query text used to define this 
**	integrity does not exist.  This does not hurt DBMS functioning, but it
**	effects some utility routines.
**
** System Status:
**	verifydb wants to drop the rule, but may only do so in
**	RUNINTERACTIVE mode.  If not in RUNINTERACTIVE mode, verifydb
**	will not take any corrective action.  If in RUNINTERACTIVE mode,
**	verifydb will prompt the user for permission to drop this rule.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1686_NO_TIME_FOR_RULE
"Rule %0c (owner %1c) is a time based rule.\n
However, no time or interval was specified defining when to fire this rule."
/*%
** Parameters:
**	%0c - rule name
**	%1c - rule owner
**
** Explanation:
**	verifydb found a time-based rule with no time or time interval
**	specified, which means that this rule will never file.
**
** System Status:
**	verifydb wants to drop the rule, but may only do so in
**	RUNINTERACTIVE mode.  If not in RUNINTERACTIVE mode, verifydb
**	will not take any corrective action.  If in RUNINTERACTIVE mode,
**	verifydb will prompt the user for permission to drop this rule.
**
** Recommendation:
**	The user must give Yes/No permission when prompted.
*/
S_DU1687_INVALID_DMPDEV
"The dump location name (%2c)\n
for database '%0c'\n
(owned by '%1c') is invalid."
/*%
** Parameters:
**	%0c - database name (from iidatabase.name)
**	%1c - DBS (from iidatabase.own)
**	%2c - dmpdev location name (from iidatabase.dmpdev)
**
** Explanation:
**	The database's dump (online checkpoint) location in not known to the
**	installation. This means that the server will not be able to checkpoint
**	this database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will ask for permission to remove this tuple from iidatabase.
**	If the user thinks that the location is valid, they should withhold
**	permission to drop this database.  Then they should go in via
**	accessdb and add the location.
*/
S_DU1688_INVL_DEFAULT_GRP
"The user group (%1c)\n
for user '%0c' is invalid."
/*%
** Parameters:
**	%0c - user name
**	%1c - default group name
**
** Explanation:
**	The specified default group name does not exist in the iidefault
**	catalog.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU1689_NOSUCH_USER
"User '%0c' defined in group '%1c' does not exist."
/*%
** Parameters:
**	%0c - user name
**	%1c - group name
**
** Explanation:
**	The default group specifies a user name that does not exist in the
**	iiuser catalog.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU168A_NOSUCH_DB
"User '%0c' was granted access or privileges to database '%1c'.\n
However, database '%1c' does not exist."
/*%
** Parameters:
**	%0c - user name
**	%1c - group name
**
** Explanation:
**	The iidbpriv catalog specifies a database that does not exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU168B_NOSUCH_USER
"User '%0c' was granted access or privileges to database '%1c'.\n
However, user '%0c' does not exist."
/*%
** Parameters:
**	%0c - user name
**	%1c - group name
**
** Explanation:
**	The iidbpriv catalog specifies a user that does not exist.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU168C_INVALID_PRIVS
"User '%0c' was granted an invalid access or privileges to database '%1c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - group name
**
** Explanation:
**	The iidbpriv catalog specifies an undefined privilege.  Only privileges
**	granted may be turned on, and here a privilege is turned on that was not
**	granted.  The fix is to turn that privilege back off.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU168D_SCHEMA_MISSING
"schema tuple missing for %0c."
/*%
** Parameters:
**	%0c - schema name
**
** Explanation:
**	verifydb tests in cksynonym/ckrel/ckevent/ckproc could not find a
**	corresponding entry for the schema tuple in iischema for the database
**	object. This is an informational message only, since the DBMS server
**	will not create the schema. However, the user may want to define the
**	schema.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required; however, the user could issue the CREATE SCHEMA
**	statement after a SET SESSION AUTHORIZATION statement, if he knows
**	the owner of the database object.
*/
S_DU168E_UNREFERENCED_DEFTUPLE
"non-canonical default tuple having id1 %0d and id2 %1d not referenced \n
by any attribute.  This is an informational message, and may be safely ignored."
/*%
** Parameters:
**	%0d - id1 of default tuple
**	%1d - id2 of default tuple
**
** Explanation:
**	A verifydb test in ckdefault() found a non-canonical default tuple
**	with no reference to any attribute.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user will be given a choice whether to delete the default
**	tuple. (See DU034B.)
*/
S_DU168F_ILLEGAL_KEY
"Attribute with constraint id %0d is not a legal column for the key"
/*%
** Parameters:
**	%0d - constraint ID for the attribute
**
** Explanation:
**	A verifydb test in ckkey() found an illegal column for the key for the
**	constraint ID.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1690_MISSING_KEYTUP
"Warning: There is a missing key tuple in iikey for constraint %0c"
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	verifydb found an integrity which has a constraint that does not
**	have enough tuples in iikey.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1691_MISSING_RULETUP
"WARNING: Verifydb found an integrity tuple having constraint name %0c \n
without an entry in iirule table."
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	verifydb found a referential constraint with no corresponding entry
**	in the iirule table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1692_MISSING_DBPROCTUP
"WARNING: Verifydb found an integrity tuple having naame %0c without an entry \n
in iiprocedure table."
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	verifydb found a referential constraint with no corresponding entry 
**	in iiprocedure table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1693_MISSING_INDEXTUP
"WARNING: Verifydb found an integrity tuple having constraint name %0c \n
without an entry in iiindex table."
/*%
** Parameters:
**	%0c - constraint name
**
** Explanation:
**	verifydb found a referential constraint with no corresponding entry
**	in iiindex table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1694_INVALID_DEFAULT
"attribute %0c has no valid default."
/*%
** Parameters:
**	%0c - attribute name
**
** Explanation:
**	A verifydb test in ckatt() found an attribute tuple with no valid
**	default.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1695_NOT_MARKED_AS_IDX
"Table %0c (owner '%1c') is a secondary index but is not marked as one."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	verifydb found a secondary index which was missing the relstat bit
**	(TCB_INDEX) indicating this is a secondary index.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1696_IIINDEX_MISSING
"Table %0c (owner '%1c') is a secondary index\n
but the iiindex catalog does not know about it."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	verifydb found a secondary index in iirelation that was not in
**	iiindex.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1697_MISSING_RELTUP
"Registered table for SAL file %0c\n
is defined in iigw06_relation, but is missing from iirelation.\n
This means that the database does not recognize this table."
/*%
** Parameters:
**	%0c - SAL file name (from iigw06_relation.audit_log)
**
** Explanation:
**	verifydb test #1 for iigw06_relation found an entry in the 
**	catalog for a nonexistent table -- i.e., there is no corresponding
**	entry in iirelation for this table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is being run in RUNINTERACTIVE
**	mode.  Then the user must give yes/no permission to drop this tuple
**	from iigw06_relation.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU1698_MISSING_GW06ATT
"Registered table %0c for SAL file %1c\n
is defined in iigw06_relation, but has no attributes."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - SAL file name (from iigw06_relation.audit_log)
**
** Explanation:
**	verifydb test #2 for iigw06_relation found an entry in the 
**	catalog that does not have any attributes; i.e., there is no
**	corresponding value in iigw06_attribute.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is being run in RUNINTERACTIVE
**	mode.  Then the user must give yes/no permission to drop this tuple
**	from iigw06_relation.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU1699_NONZERO_GW06IDX
"Registered table %0c for SAL file %1c\n
has a non-zero index field indicating that it is a registered index.\n
Indexes are not supported for the security audit gateway."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - SAL file name (from iigw06_relation.audit_log)
**
** Explanation:
**	verifydb test #3 for iigw06_relation found that the index attribute
**	(reltidx) was non-zero, indicating that the registration is an index.
**	Indexes are not supported for the security audit gateway.
**
** System Status:
**	The verifydb process continues.
**
** User action:
**	None is required, unless verifydb is being run in RUNINTERACTIVE
**	mode.  Then the user must give yes/no permission to drop this tuple
**	from iigw06_relation.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU169A_MISSING_GW06REL
"Attribute named '%0c'\n
is registered as an attribute in the security audit gateway,\n
but is not in any security audit gateway table,\n
this attribute will not be recognized by the gateway."
/*%
** Parameters:
**	%0c - attribute name (from iigw06_attribute.attname)
**
** Explanation:
**	verifydb test #1 for iigw06_attribute found that a security audit
**	gateway attribute did not belong to any security audit gateway table;
**	i.e., it had no corresponding value in iigw06_relation.
**
** System Status:
**	The verifydb process continues.
**
** User action:
**	None is required, unless verifydb is being run in RUNINTERACTIVE
**	mode.  Then the user must give yes/no permission to drop this tuple
**	from iigw06_relation.
*/
S_DU169B_MISSING_IIATTRIBUTE
"Attribute named '%0c'\n
is registered as an attribute in the security audit gateway,\n
but does not have a corresponding entry in iiattribute.\n
This attribute will not be recognized by ingres." 
/*% 
** Parameters
**	%0c - attribute name (from iigw06_attribute.attname)
**
** Explanation:
**	verifydb test #2 for iigw06_attribute found that a security audit
**	gateway attribute did not have a corresponding entry in iiattribute;
**	without this entry, the attribute will not be recognized by 
**	the DBMS as a valid database attribute.
**
** System Status:
**	The verifydb process continues.
**
** User action:
**	None is required, unless verifydb is being run in RUNINTERACTIVE
**	mode.  Then the user must give yes/no permission to drop this tuple
**	from iigw06_relation.
**
**	No correction is made if verifydb is in REPORT mode.
*/
S_DU169C_NONZERO_GW06ATTIDX
"Attribute named '%0c',\n
registered as a security audit gateway attribute has a non-zero index field,\n
indicating that the table it belongs to is a registered index.\n
Indexes are not supported for the security audit gateway."
/*%
** Parameters:
**	%0c - attribute name (from iigw06_attribute.attname)
**
** Explanation:
**	verifydb test #3 for iigw06_attribute found that the index field
**	(reltidx) was non-zero, indicating that the attribute belongs to a
**	registered index. Indexes are not supported for the security audit 
**	gateway.
**
** System Status:
**	The verifydb process continues.
**
** User action:
**	None is required, unless verifydb is being run in RUNINTERACTIVE
**	mode.  Then the user must give yes/no permission to drop this tuple
**	from iigw06_relation.
**
**	No correction is made if verifydb is in REPORT mode.
*/
S_DU169D_MISSING_IIGW06REL
"Table %0c\n
is indicated as a security audit gateway table in iirelation,\n
but does not have a corresponding row in iigw06_relation.\n
The security audit gateway will not be able to use this table."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**
** Explanation:
**	verifydb test #29 for iirelation found a table indicated to be a
**	security audit gateway table (relgwid=6), but could not find
**	a row in iigw06_relation that matched; therefore, the gateway will
**	not be able to use this table.
**
** System Status:
**	The verifydb process continues.
**
** User action:
**	None is required while verifydb is running, but the DBA is expected
**	to fix the problem offline.
*/
S_DU169E_MISSING_COM_BASE
"The table/view described in iidbms_comments as comtabbase = %0d,\n
comtabidx = %1d does not exist."
/*%
** Parameters:
**	%0d - base table ID
**	%1d - index table ID
**
** Explanation:
**	A verifydb test found an entry in iidbms_comment that describes a
**	view or table with comments.  However, that table/view does not exist
**	(or is not defined in iirelation)
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iidbms_comment tuple unless in
**	RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU169F_NO_COMMENT_RELSTAT
"iirelation does not indicate that comments are defined for table\n
%0c (owner %1c)."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb found the iirelation tuple for the comment did not indicate
**	that this table has rules defined for it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU16A0_BAD_COL_COMMENT
"Invalid comment for column %0d of table %1c (owner %2c)."
/*%
** Parameters:
**	%0d - attribute ID (from iidbms_comment.comattid)
**	%1c - table name (from iirelation.relid)
**	%2c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb found that the iidbms_comment tuple was corrupted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU16A1_BAD_TBL_COMMENT
"Invalid table comment for %0c (owner %1c)."
/*%
** Parameters:
**	%0c - table name (from iirelation.relid)
**	%1c - table owner (from iirelation.relowner)
**
** Explanation:
**	verifydb found that the iidbms_comment tuple was corrupted.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Unless verifydb is being run in RUNINTERACTIVE mode, correction of
**	setting RULE bit in iirelation.relstat is automatically performed.
**	Otherwise, the user must give yes/no permission to make this correction
**	when the prompt is given.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU16A2_COMMENT_DUP_ERR
"Duplicate sequence number (%3d) on comment for col %2d, table %0c (owner %1c)"
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**	%2d - column ID (0 implies a table-level comment)
**	%3d - duplicated sequence number
**
** Explanation:
**	verifydb found a sequence error on DBMS comments that span more than a
**	single tuple.
**
** System Status:
**	The verifydb process continues.  If verifydb is being run in
**	RUNINTERACTIVE mode, it will prompt the user for yes/no
**	permission to delete the comment. In any of the other run modes,
**	it automatically deletes it.
**
** Recommendation:
**	Answer YES/NO if prompted.
*/ 
S_DU16A3_COMMENT_SEQ_ERR
"Missing sequence number (%3d) on comment for col %2d, table %0c (owner %1c)"
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**	%2d - column ID (0 implies a table level comment)
**	%3d - missing sequence number.
**
** Explanation:
**	verifydb found a sequence error on DBMS comments that span more than a
**	single tuple.
**
** System Status:
**	The verifydb process continues.  If verifydb is being run in
**	RUNINTERACTIVE mode, it will prompt the user for yes/no
**	permission to delete the comment. In any of the other run modes,
**	it automatically deletes it.
**
** Recommendation:
**	Answer YES/NO if prompted.
*/ 
S_DU16A4_NONDELIM_UPPER_ERR
"Database '%0c' (owner %1c) breaks semantics rules for delimited identifiers.\n
If non-delimited IDs are upper case, then delimited IDs must be either upper\n
case or mixed.\n
But here non-delimited IDs are upper case and delimited IDs are lower case"
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	verifydb found a violation of delimited identifier semantics.
**
** System Status:
**	The verifydb process continues.  If verifydb is being run in
**	RUNINTERACTIVE mode, it will prompt the user for yes/no
**	permission to delete the comment. In any of the other run modes,
**	it automatically deletes it.
**
** Recommendation:
**	Answer YES/NO if prompted.
*/ 
S_DU16A5_NONDELIM_LOWER_ERR
"Database '%0c' (owner %1c) breaks semantics rules for delimited identifiers.\n
If non-delimited IDs are lower case, then delimited IDs may not be upper case.\n
But here non-delimited IDs are lower case and delimited IDs are upper case"
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	verifydb found a violation of delimited identifier semantics.
**
** System Status:
**	The verifydb process continues.  If verifydb is being run in
**	RUNINTERACTIVE mode, it will prompt the user for yes/no
**	permission to delete the comment. In any of the other run modes,
**	it automatically deletes it.
**
** Recommendation:
**	Answer YES/NO if prompted.
*/ 
S_DU16A6_SCHIZOID_DELIM_ID
"Database '%0c' (owner %1c) breaks semantics rules for delimited identifiers.\n
Delimited Identifiers may be lower, upper, or mixed case, but only one of these.
Here both mixed and upper case are specified for delimited identifiers."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	verifydb found a violation of delimited identifier semantics.
**
** System Status:
**	The verifydb process continues.  If verifydb is being run in
**	RUNINTERACTIVE mode, it will prompt the user for yes/no
**	permission to delete the comment. In any of the other run modes,
**	it automatically deletes it.
**
** Recommendation:
**	Answer YES/NO if prompted.
*/ 
S_DU16A7_REALUSER_CASE_ERR
"Database '%0c' (owner %1c) breaks semantics rules for delimited identifiers.\n
If the real user login is mixed case, then delimited IDs must also be mixed.\n
Here the real user login is specified as mixed but delimited IDs are not."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	verifydb found a violation of delimited identifier semantics.
**
** System Status:
**	The verifydb process continues.  If verifydb is being run in
**	RUNINTERACTIVE mode, it will prompt the user for yes/no
**	permission to delete the comment. In any of the other run modes,
**	it automatically deletes it.
**
** Recommendation:
**	Answer YES/NO if prompted.
*/ 
S_DU16A8_IIDBDEPENDS_MISSING
"Database object '%0c' (owner %1c) does not have corresponding entry in \n
IIDBDEPENDS catalog."
/*%
** Parameters:
**	%0c - database object name
**	%1c - database object owner
**
** Explanation:
**	verifydb found a dependent database object with no entry in the
**	iidbdepends catalog.
**
** System Status:
**	The verifydb process continues. If verifydb is being run in 
**	"runinteractive" mode, it will prompt the user for yes/no permission
**	to delete the database object. In any of the other run modes, it
**	automatically deletes it.
**
** Recommendation:
**	Answer Y(es)/N(o), if prompted.
*/
S_DU16AA_RESET_DEFLT_PROF
"User '%0c' has been set to the default profile."
/*%
    ** Parameters:
    **  %0c - name of user
    ** Explanation:
    **	Verifydb has reset the user profile name to the default profile.
    ** System Status:
    **	VERIFYDB will continue processing.
*/
S_DU16AB_INVALID_PROFNAME
"Profile '%0c' has an invalid name."
/*%
    ** Parameters:
    **	%0c - profile name
    ** Explanation:
    **	Verifydb found a profile name which is illegal when checking iiprofile
    **  tuples
    ** System Status:
    **  Verifydb continues processing.  If in interactive mode it will prompt
    **	the user for yes/no permission to delete the profile. In any of the
    **	other run modes, it simply deletes it.
    ** User Action:
    **  Answer YES/NO if prompted.
*/ 
S_DU16AC_DROP_PROFILE
"The recommended action is to drop profile '%0c'."
/*%
    ** Parameters:
    **  %0c - name of profile
    ** Explanation:
    **	Verifydb will drop the tuple from the iiprofile table.  This will
    **  effectively drop the profile from the installation 
    ** System Status:
    **	VERIFYDB will continue processing.
    ** User Action:
    **	If prompted, user must enter Yes/No permission to drop this profile.
    **  Otherwise, no action is required.
    **  It might be a good idea for the user to answer NO to all subsequent
    **  VERIFYDB prompts, go in via ACCESSDB and assure that all users are
    **  defined (if not define them), then run VERIFYDB.
*/
S_DU16AD_PROFILE_DROPPED
"Profile '%0c' has been dropped."
/*%
    ** Parameters:
    **  %0c - name of profile
    ** Explanation:
    **	Verifydb has dropped the profile from the iiprofile catalog.
    ** System Status:
    **	VERIFYDB will continue processing.
*/
S_DU16AE_INVALID_PROFSTAT
"Profile '%0c' has invalid status bits."
/*%
    ** Parameters:
    **  %0c -- profile name
    ** Explanation:
    **  iiprofile.status has invalid bits set.
    ** System Status:
    **  VERIFYDB keeps running.
    ** User Action:
    **  Verifydb will clear invalid bits.  However, there is a good chance that
    **  the profile's permissions may not be right if there were invalid bits in
    **  the permission bitmap.  SYSTEM Administrator should run ACCESSDB to
    **  assure that profile has correct permissions.
*/
S_DU16AF_CLEAR_PROFILE_BITS
"The recommended action is to remove invalid status bits for profile '%0c'."
/*%
    ** Parameters:
    **  %0c - name of profile
    ** Explanation:
    **	iiprofile.status is a bitmap of profile attributes.  Some of the 
    **  bits set in the bitmap are invalid.  Verifydb wants to zero the 
    ** invalid bits.
    ** System Status:
    **	VERIFYDB will continue processing.
    ** User Action:
    **	If prompted, user must enter Yes/No permission to reset the bits
    **  Otherwise, no action is required.
    **  There is a good chance that the profile's permissions may not be right
    **	if there were invalid bits in the permission bitmap.  SYSTEM
    **	Administrator should run ACCESSDB to assure that user has correct
    **	permissions.
*/
S_DU16B0_PROFILE_BITS_CLEARED
"Invalid status bits for profile '%0c' were removed."
/*%
    ** Parameters:
    **  %0c - name of profile
    ** Explanation:
    **	iiprofile.status is a bitmap of profile attributes.  Some of the bits 
    **  set in the bitmap are invalid.  Verifydb zeroed the invalid bits.
    ** System Status:
    **	VERIFYDB will continue processing.
    ** User Action:
    **  There is a good chance that the profile's permissions may not be right
    **	if there were invalid bits in the permission bitmap.  SYSTEM
    **	Administrator should run ACCESSDB to assure that user has correct
    **	permissions.
*/
S_DU16B1_NO_DEFAULT_PROFILE
"The default profile tuple was not found in iiprofile."
/*
** Parameter:
**	None
** Explanation
**	The default profile tuple seems to be missing from the iiprofile
**	catalog.
** System Status:
**	VERIFYDB will continue processing.
**/
S_DU16B2_ADD_DEFAULT_PROF
"The recommendation is to recreate a new default profile tuple in iiprofile"
/*
** Parameters:
**   None
** Explanation:
**	The default profile tuple is missing from the iiprofile catalog.
**	Verifydb wants to add a new tuple.
** System Status:
**	VERIFYDB will continue processing.
** User Action:
**	If prompted, user must enter Yes/No permission to add the new
**	tuple.
**	Note that not adding a default profile may cause subsequent 
**	problems when processing the iiuser catalog.
*/
S_DU16B3_DEFAULT_PROF_ADDED
"A new default profile tuple has been added"
/*
** Parameter:
**	None
** Explanation
**	VERIFYDB has added a default profile tuple to the iiprofile 
**	catalog.
** System Status:
**	VERIFYDB will continue processing.
**/
S_DU16B4_INVLD_USER_PROFILE
"User '%0c' has an invalid or unknown profile '%1c'"
/*%
    ** Parameters:
    **	%0c - table name
    **	%1c - profile name
    ** Explanation:
    **	Verifydb found a profile name which is illegal when checking user
    **  tuples
    ** System Status:
    **  Verifydb continues processing.  If in interactive mode it will prompt
    **	the user for yes/no permission to delete the profile. In any of the
    **	other run modes, it simply deletes it.
    ** User Action:
    **  Answer YES/NO if prompted.
*/ 
S_DU16B5_USER_PROF_RESET
"The recommended action is to reset user '%0c' to the default profile"
/*%
    ** Parameters:
    **  %0c - name of user
    ** Explanation:
    **	Verifydb will reset the user profile to the default profile
    ** System Status:
    **	VERIFYDB will continue processing.
    ** User Action:
    **	If prompted, user must enter Yes/No permission to reset the profile
    **  Otherwise, no action is required.
    **  It might be a good idea for the user to answer NO to all subsequent
    **  VERIFYDB prompts, go in via ACCESSDB and assure that all users are
    **  defined (if not define them), then run VERIFYDB.
*/
S_DU16C0_NONEXISTENT_DEP_OBJ
"The %0c described in iidbdepends as\n
deid1 = %1d, deid2 = %2d, dtype = %3d, qid = %4d\n
does not exist."
/*%
** Parameters:
**	%0c - object type
**	%1d - dependent object id (from iidbdepends.deid1)
**	%2d - dependent object index id (from iidbdpends.deid2)
**	%3d - dependent object type (from iidbdpends.dtype)
**      %4d - dependent object secondary id (from iidbdpends.qid)
**
** Explanation:
**	verifydb test #54 found a tuple in iidbdepends which describes an non-
**	existent dependent object.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iidbdepends tuple, unless it is
**	in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0322.)
*/
S_DU16C1_NONEXISTENT_INDEP_OBJ
"The %0c described in iidbdepends as\n
inid1 = %1d, inid2 = %2d, itype = %3d, i_qid = %4d\n
does not exist."
/*%
** Parameters:
**	%0c - object type
**	%1d - independent object id (from iidbdepends.inid1)
**	%2d - independent object index id (from iidbdpends.inid2)
**	%3d - independent object type (from iidbdpends.itype)
**      %4d - independent object secondary id (from iidbdpends.i_qid)
**
** Explanation:
**	verifydb test #55 found a tuple in iidbdepends which describes an non-
**	existent independent object.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iidbdepends tuple, unless it is
**	in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0322.)
*/
S_DU16C2_UNEXPECTED_DEP_OBJ_TYPE
"IIDBDEPENDS tuple contained unexpected dtype (%0d).
\ndeid1 = %1d, deid2 = %2d, qid = %3d."
/*%
** Parameters:
**      %0d - unexpected dependent object type (from iidbdpends.dtype)
**      %1d - dependent object id (from iidbdepends.deid1)
**      %2d - dependent object index id (from iidbdpends.deid2)
**      %3d - dependent object secondary id (from iidbdpends.qid)
**
** Explanation:
**      Verifydb test #54 found a tuple in iidbdepends which contains a 
**	description of dependent object of unexpected type.  
**
** System Status:
**      The verifydb process continues.
**
** Recommendation:
**      verifydb will automatically delete the iidbdepends tuple, unless it is
**      in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**      permission. 
*/
S_DU16C3_UNEXPECTED_IND_OBJ_TYPE
"IIDBDEPENDS tuple contained unexpected itype (%0d).
\ninid1 = %1d, inid2 = %2d, i_qid = %3d."
/*%
** Parameters:
**      %0d - unexpected independent object type (from iidbdpends.itype)
**      %1d - independent object id (from iidbdepends.inid1)
**      %2d - independent object index id (from iidbdpends.inid2)
**      %3d - independent object secondary id (from iidbdpends.i_qid)
**
** Explanation:
**      Verifydb test #55 found a tuple in iidbdepends which contains a 
**	description of independent object of unexpected type.  
**
** System Status:
**      The verifydb process continues.
**
** Recommendation:
**      verifydb will automatically delete the iidbdepends tuple, unless it is
**      in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**      permission.  Dependent object described by this tuple will be marked
**	for destruction
*/
S_DU16C4_MISSING_INDEP_OBJ
"Object on which %0c '%1c'.'%2c'
\ndepends does not exist."
/*%
** Parameters:
**	%0c - object type
**	%1c - schema to which the object belongs
**	%2c - name of the object
**
** Explanation:
**	verifydb IIDBDEPENDS test #2 discovered a tuple describing dependence of
**	an object on another object which either does not exist or the type of 
**	independent object is not one of those expected by verifydb.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the object to be 
**	destroyed.  In general, it is a good idea to let destruction proceed 
**	since without complete dependency information the DBMS is unable to 
**	ensure that the database contains no abandoned objects.
*/
S_DU16C5_MISSING_PERM_INDEP_OBJ
"Object on which permit %0d 
\non view '%1c'.'%2c' depends does not exist."
/*%
** Parameters:
**	%0c - permit number
**	%1c - schema to which the view belongs
**	%2c - name of the view
**
** Explanation:
**	verifydb IIDBDEPENDS test #2 discovered a tuple describing dependence of
**	a permit on a view on privileges described by a privilege descriptor 
**	which does not exist or on object of type which is not expected by 
**	verifydb.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the object to be 
**	destroyed.  In general, it is a good idea to let destruction proceed 
**	since without complete dependency information the DBMS is unable to 
**	ensure that the database contains no abandoned objects.  With permits,
**	one may also consider issuing DROP PERMIT or REVOKE.
*/
S_DU16C6_MISSING_CONS_INDEP_OBJ
"Object on which constraint %0c 
\non table '%1c'.'%2c' depends does not exist."
/*%
** Parameters:
**	%0c - name of a constraint
**	%1c - schema to which the table belongs
**	%2c - name of the table
**
** Explanation:
**	verifydb IIDBDEPENDS test #2 discovered a tuple describing dependence of
**	a constraint on an object which does not exist or on object of type 
**	which is not expected by verifydb.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the object to be 
**	destroyed.  In general, it is a good idea to let destruction proceed 
**	since without complete dependency information the DBMS is unable to 
**	ensure that the database contains no abandoned objects. 
*/
S_DU16C7_EXTRA_IIXPRIV
"iixpriv contains a tuple with
\ni_obj_id = %0d, i_obj_idx = %1d, i_priv = %2d, 
\ni_priv_grantee = %3c, and tidp = %4d,
\nbut a corresponding tuple is missing from iipriv."
/*%
** Parameters:
**	%0d -- id of an object (from iixpriv.i_obj_id)
**	%1d -- index id of an object (from iixpriv.i_obj_idx)
**	%2d -- privilege type (from iixpriv.i_priv)
**	%3c -- grantee of a privilege (from iixpriv.i_priv_grantee)
**	%4d -- TID pointer of a tuple (from iixpriv.tidp)
**
** Explanation:
**	verifydb IIXPRIV test #1 found an entry in IIXPRIV for a nonexistent 
**	IIPRIV tuple
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to delete the offending tuple
*/
S_DU16C8_MISSING_IIXPRIV
"iipriv contains a tuple with
\ni_obj_id = %0d, i_obj_idx = %1d, i_priv = %2d, 
\ni_priv_grantee = %3c, and tid = %4d,
\nbut a corresponding tuple is missing from iixpriv."
/*%
** Parameters:
**	%0d -- id of an object (from iipriv.i_obj_id)
**	%1d -- index id of an object (from iipriv.i_obj_idx)
**	%2d -- privilege type (from iipriv.i_priv)
**	%3c -- grantee of a privilege (from iipriv.i_priv_grantee)
**	%4d -- TID of a tuple (from iipriv.tid)
**
** Explanation:
**	verifydb IIXPRIV test #2 found an entry in IIPRIV for which there is
**	no corresponding IIXPRIV tuple
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to insert a missing tuple into
**	IIXPRIV
*/
S_DU16C9_EXTRA_IIXEVENT
"iixevent contains a tuple with
\nevent_idbase = %0d, event_idx = %1d, and tidp = %2d,
\nbut a corresponding tuple is missing from iievent."
/*%
** Parameters:
**	%0d -- id of a dbevent (from iixevent.event_idbase)
**	%1d -- index id of a dbevent (from iixevent.event_idx)
**	%2d -- TID pointer of a tuple (from iixevent.tidp)
**
** Explanation:
**	verifydb IIXEVENT test #1 found an entry in IIXEVENT for a nonexistent 
**	IIEVENT tuple
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to delete the offending tuple from
**	IIXEVENT
*/
S_DU16CA_MISSING_IIXEVENT
"iievent contains a tuple with
\nevent_idbase = %0d, event_idx = %1d, and tid = %2d,
\nbut a corresponding tuple is missing from iixevent."
/*%
** Parameters:
**	%0d -- id of a dbevent (from iievent.event_idbase)
**	%1d -- index id of a dbevent (from iievent.event_idx)
**	%2d -- TID of a tuple (from iievent.tid)
**
** Explanation:
**	verifydb IIXEVENT test #2 found an entry in IIEVENT for which there is
**	no corresponding IIXEVENT tuple
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to insert a missing tuple into
**	IIXEVENT
*/
S_DU16CB_EXTRA_IIXPROCEDURE
"iixprocedure contains a tuple with
\ndbp_id = %0d, dbp_idx = %1d, and tidp = %2d,
\nbut a corresponding tuple is missing from iiprocedure."
/*%
** Parameters:
**	%0d -- id of a database procedure (from iixprocedure.dbp_id)
**	%1d -- index id of a database procedure (from iixprocedure.dbp_idx)
**	%2d -- TID pointer of a tuple (from iixprocedure.tidp)
**
** Explanation:
**	verifydb IIXPROCEDURE test #1 found an entry in IIXPROCEDURE for a 
**	nonexistent IIPROCEDURE tuple
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to delete the offending tuple from
**	IIXPROCEDURE
*/
S_DU16CC_MISSING_IIXPROCEDURE
"iiprocedure contains a tuple with
\ndbp_id = %0d, dbp_idx = %1d, and tid = %2d,
\nbut a corresponding tuple is missing from iixprocedure."
/*%
** Parameters:
**	%0d -- id of a database procedure (from iiprocedure.dbp_id)
**	%1d -- index id of a database procedure (from iiprocedure.dbp_idx)
**	%2d -- TID of a tuple (from iiprocedure.tid)
**
** Explanation:
**	verifydb IIXPROCEDURE test #2 found an entry in IIPROCEDURE for which 
**	there is no corresponding IIXPROCEDURE tuple
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None is required, unless verifydb is in RUNINTERACTIVE mode -- then
**	the user must give yes/no permission to insert a missing tuple into
**	IIXPROCEDURE
*/
S_DU16CF_BAD_OBJ_TYPE_IN_IIPRIV
"IIPRIV tuple contained unexpected d_obj_type (%0d).
\nd_obj_id = %1d, d_obj_idx = %2d, d_priv_number = %3d."
/*%
** Parameters:
**      %0d - unexpected object type (from iipriv.d_obj_type)
**      %1d - object id (from iipriv.d_obj_id)
**      %2d - object index id (from iipriv.d_obj_idx)
**      %3d - privilege descriptor number (from iipriv.d_priv_number)
**
** Explanation:
**      Verifydb IIPRIV test #1 found a tuple in iipriv which contains a 
**	description of privilege descriptor created for an object of unexpected
**	type.  
**
** System Status:
**      The verifydb process continues.
**
** Recommendation:
**      verifydb will automatically delete the iipriv tuple, unless it is
**      in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**      permission. 
*/
S_DU16D0_QRYTEXT_ALARM_MISSING
"iiqrytext tuples with txtid1 = %0d, txtid2 = %1d\n
define a security alarm.  However, this security alarm is missing from 
the database."
/*%
    ** Parameters:
    **	%0d - Low portion of query id (from iiqrytext.txtid1)
    **	%1d - High portion of query id (from iiqrytext.txtid2)
    ** Explanation:
    **  VERIFYDB was unable to find the security alarm definition in 
    **  iisecalarm that corresponds to this alarm querytext. This is 
    **	informational only, as the DBMS Server does not act on iiqrytext.  
    **	However, the user may wish to redefine the alarm to the database.  
    ** System Status:
    **  VERIFYDB keeps running.
    ** User Action:
    **	none required.
*/
S_DU16D1_NO_BASE_FOR_ALARM
"Base object id %0d, %1d for security alarm %2c is not found."
/*%
    ** Parameters:
    **  %0d - object id1 (from iisecalarm.obj_id1)
    **  %1d - object id2 (from iisecalarm.obj_id2)
    **  %2c - alarm name
    ** Explanation:
    **  VERIFYDB found that the table that is supposed to receive
    **	this alarm doesn't exist (at least it's not listed in iirelation.)
    ** System Status:
    **  VERIFYDB keeps running.
    ** User Action:
    **  VERIFYDB will automatically drop all tuples with this base,index
    **	id from iisecalarm unless in RUNINTERACTIVE mode.  Then user must
    **	enter YES/NO permission.
    **  Of course, no correction is made if VERIFYDB is in REPORT mode.
*/
S_DU16D2_DROP_FROM_IISECALARM
"The recommended action is to drop security alarm tuple for '%0c' 
from iisecalarm"
/*%
    ** Parameters:
    **  %0c - alarm name
    ** Explanation:
    **  VERIFYDB found that the table that is supposed to receive
    **	this alarm doesn't exist (at least it's not listed in iirelation.)
    **  The user will be prompted to give yes/no permission.  If yes is given,
    **	VERIFYDB will take the recommended action.  If no is given, VERIFYDB
    **	will proceed to the next test.
*/
S_DU16D3_IISECALARM_DROP
"Tuple for alarm %0c dropped from iisecalarm"
/*%
    ** Parameters:
    **  %0c - alarm name
    ** Explanation:
    **  VERIFYDB found that the table/database that is supposed to receive
    **	this alarm doesn't exist (at least it's not listed in iirelation.)
    ** System Status:
    **  This is only an informative message, so processing continues.
    ** User Action:
    **	none.
*/
S_DU16D4_NO_ALARM_RELSTAT
"iirelation does not indicate that security alarms are defined for table\n
%0c (owner %1c)."
/*%
    ** Parameters:
    **  %0c -- name of table (from iirelation.relid)
    **  %1c -- name of tables owner (from iirelation.relowner)
    ** Explanation:
    **  VERIFYDB found the iirelation tuple for an alarm did not indicate that
    **	this table has rules defined for it.
    ** System Status:
    **  VERIFYDB keeps running.
    ** User Action:
    **  Unless VERIFYDB mode = RUNINTERACTIVE, correction of setting ALARM bit
    **  in iirelation.relstat is automatically performed.  Else, user must
    **  give yes/no permission to make this correction when prompt is given.
    **  Of course, no correction is made if VERIFYDB is in REPORT mode.
*/
S_DU16D5_SET_ALMTUPS
"The recommended action is to indicate that an alarm is defined for this table."
/*%
    ** Parameters:
    **	None.
    ** Explanation:
    **	VERIFYDB found that the table does not indicate that there is an alarm
    **	defined for this table/view.  The recommended solution is to change
    **	iirelation.relstat (set ALARM) for the table.
    **
    **  The user will be prompted to give yes/no permission.  If yes is given,
    **	VERIFYDB will take the recommended action.  If no is given, VERIFYDB
    **	will proceed to the next test.
    **   NOTE:  this message will also be displayed in REPORT mode, but the
    **	       user will not be given the opportunity to input yes/no permission.
    ** 
    **  This is only an informative message, so processing continues.  However,
    **  this type of message requires input from the user.  The S_DU0300_PROMPT
    **  message will be displayed following this message, and processing will
    **  simply wait until some input is given.  If the input is not a valid
    **  response, the message will be repeated until a valid response is given.
    ** SYSTEM STATUS:
    **  This is an informative message.  Processing continues.
    ** User Action:
    **  Enter Y(ES) or N(O).
*/
S_DU16D6_ALMTUPS_SET
"Verifydb indicated that a security alarm is defined for this table."
/*%
    ** Parameters:
    **	None.
    ** Explanation:
    **	VERIFYDB found that the table does not indicate that there is an alarm
    **	defined for this table/view.  The recommended solution is to change
    **	iirelation.relstat (set ALARM) for the table.
    ** SYSTEM STATUS:
    **  This is an informative message.  Processing continues.
    ** User Action:
    **  None.
*/
S_DU16D7_NO_ALARM_QRYTEXT
"The query text %0d %1d\n
for security alarm %2c does not exist."
/*%
    ** Parameters:
    **  %0d - high portion of qryid
    **  %1d - low portion of qryid
    **	%2c - alarm name
    ** Explanation:
    **  VERIFYDB found that the query text used to define this alarm does not
    **	exist.
    ** System Status:
    **	VERIFYDB continues processing.
*/
S_DU16D8_NO_ALARM_EVENT
"The dbevent %0d,%1d was not found for security alarm %2c."
/*%
    ** Parameters:
    ** 	%0d,%1d - Dbevent id
    **  %02c    - Alarm name
    **
    ** Explanation:
    **  VERIFYDB found that the dbevent for the security alarm does not
    **	exist.
    ** System Status:
    **	VERIFYDB continues processing.
*/
S_DU16E0_NONEXISTENT_OBJ
"%0c with id (%1d, %2d) dependent on privilege descriptor %3d
\ndoes not exist."
/*%
** Parameters:
**	%0c - object type (from iipriv.d_obj_type): one of VIEW, PROCEDURE
**	%1d - object id (from iipriv.d_obj_id)
**	%2d - object index id (from iipriv.d_obj_idx)
**	%3d - privilege descriptor number (from iipriv.d_priv_number)
**
** Explanation:
**	verifydb IIPRIV test #1 found a tuple in iipriv which represents a
**	privilege descriptor created for an object which no longer exists.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iipriv tuple, unless it is
**	in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU16E1_NO_OBJ_DEPENDS_ON_DESCR
"There exists no %0c created on table/view with id (%1d, %2d) 
\ndependent on privilege descriptor %3d."
/*%
** Parameters:
**	%0c - object type (from iipriv.d_obj_type): one of PERMIT, CONSTRAINT
**	%1d - id of an object on which the permit/constraint was created
**	      (from iipriv.d_obj_id)
**	%2d - index id of an object on which the permit/constraint was created
**	      (from iipriv.d_obj_idx)
**	%3d - privilege descriptor number (from iipriv.d_priv_number)
**
** Explanation:
**	verifydb IIPRIV test #1 found a tuple in iipriv which represents a
**	privilege descriptor created for a permit or constraint which 
**	apparently no longer exists.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically delete the iipriv tuple, unless it is
**	in RUNINTERACTIVE mode.  Then it will prompt the user for yes/no
**	permission.
**
**	No correction can be made if verifydb is in REPORT mode.
*/
S_DU16E2_LACKING_INDEP_PRIV
"'%0c' no longer possesses privileges described in
\nprivilege descriptor %1d on which %2c '%3c'.'%4c' depends
\nor, if there are permits defined on this %2c,
\n'%0c' no longer possesses privileges described in
\nprivilege descriptor %1d on which such permits depend."
/*%
** Parameters:
**	%0c - auth id who must possess privileges described in the privilege 
**	      descriptor (from iipriv.i_priv_grantee)
**	%1d - privilege descriptor number (from iipriv.d_priv_number)
**	%2c - object type (from iipriv.d_obj_type); 
**	      one of VIEW, PROCEDURE, CONSTRAINT
**	%3c - schema to which the object belongs
**	%4c - name of the object
**
** Explanation:
**	verifydb IIPRIV test #2 discovered that a user named in a tuple 
**	describing privilege on which an object depends does not possess the 
**	privilege described in the tuple or, if there are permits defined on 
**	the object, we may have discovered that the user does not possess 
**	GRANT OPTION FOR privilege on which such permit would depend.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the object to be 
**	destroyed or, in the case of database procedures, marked dormant.
*/
S_DU16E3_LACKING_PERM_INDEP_PRIV
"'%0c' no longer possesses GRANT OPTION FOR privileges described in
\nprivilege descriptor %1d on which permit(s) 
\non view '%2c'.'%3c' depend."
/*%
** Parameters:
**	%0c - auth id who must possess GRANT OPTION FOR privileges described 
**	      in the privilege descriptor (from iipriv.i_priv_grantee)
**	%1d - privilege descriptor number (from iipriv.d_priv_number)
**	%2c - schema to which the view belongs
**	%3c - name of the view
**
** Explanation:
**      verifydb IIPRIV test #2 discovered that a user named in a tuple 
**	describing privilege on GRANT OPTION FOR which permit(s) on a view 
**	depend does not possess GRANT OPTION FOR privilege described in the 
**	tuple.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the view on which 
**	the permit was defined to be destroyed. 
*/
S_DU16E4_LACKING_CONS_INDEP_PRIV
"'%0c' no longer possesses privileges described in
\nprivilege descriptor %1d on which constraint(s) 
\non table '%2c'.'%3c' depend."
/*%
** Parameters:
**	%0c - auth id who must possess privileges described 
**	      in the privilege descriptor (from iipriv.i_priv_grantee)
**	%1d - privilege descriptor number (from iipriv.d_priv_number)
**	%2c - schema to which the table belongs
**	%3c - name of the table
**
** Explanation:
**      verifydb IIPRIV test #2 discovered that a user named in a tuple 
**	describing privilege on which constraint(s) on a table 
**	depend does not possess privilege described in the tuple.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the constraint(s) 
**	which depend on privilege represented by this descriptor to be
**      destroyed.
*/
S_DU16E5_MISSING_INDEP_LIST
"IIPROCEDURE tuple describing database procedure
\n'%0c'.'%1c' indicates that it depends on some
\nobjects and/or privileges, but none could be found."
/*%
** Parameters:
**	%0c - name of a schema to which the database procedure belongs
**	%1c - name of a database procedure
**
** Explanation:
**	verifydb IIPROCEDURE test #5 discovered that while the IIPROCEDURE
**	tuple describing a database procedure indicates that it depends on 
**	some objects and/or privileges, no record of such dependencies could 
**	be found by examining IIDBDEPENDS or IIPRIV.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the database 
**	procedure to be marked dormant.  
*/
S_DU16E6_UNEXPECTED_INDEP_LIST
"IIPROCEDURE tuple describing database procedure
\n'%0c'.'%1c' indicates that it does not depend on any
\nobjects and/or privileges, but other DBMS catalogs
\ncontain records of such dependencies."
/*%
** Parameters:
**	%0c - name of a schema to which the database procedure belongs
**	%1c - name of a database procedure
**
** Explanation:
**	verifydb IIPROCEDURE test #6 discovered that while the IIPROCEDURE
**	tuple describing a database procedure indicates that it does not depend
**	on any objects and/or privileges, record(s) describing such dependencies
**	were found by examining IIDBDEPENDS and/or IIPRIV.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the 
**	DB_DBP_INDEP_LIST bit to be set in dbp_mask1 of the record describing 
**	this database procedure.
*/
S_DU16E7_MISSING_UBT
"Underlying object id (%0d, %1d) contained in IIPROCEDURE tuple 
\ndescribing database procedure '%2c'.'%3c' 
\ndoes not refer to any existing table."
/*%
** Parameters:
**	%0d - underlying table base id
**	%1d - underlying table index id
**	%2c - name of a schema to which the database procedure belongs
**	%3c - name of a database procedure
**
** Explanation:
**	verifydb IIPROCEDURE test #7 discovered that underlying table id
**	contained in IIPROCEDURE tuple describing a database procedure does not
**	refer to any existing table.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the database 
**	procedure to be marked dormant.  
*/
S_DU16E8_UBT_IS_A_CATALOG
"Underlying object id (%0d, %1d) contained in IIPROCEDURE tuple 
\ndescribing database procedure '%2c'.'%3c' 
\nis that of a DBMS catalog."
/*%
** Parameters:
**	%0d - underlying table base id
**	%1d - underlying table index id
**	%2c - name of a schema to which the database procedure belongs
**	%3c - name of a database procedure
**
** Explanation:
**	verifydb IIPROCEDURE test #7 discovered that underlying table id
**	contained in IIPROCEDURE tuple describing a database procedure 
**	refer to a DBMS catalog even though we may not alter timestamp of a 
**	DBMS catalog when a permit on a database procedure is revoked or the
**	database procedure itself becomes dormant or is destroyed, 
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants to have the
**	underlying table id reset to zeroes.
*/
S_DU16E9_UNEXPECTED_DBP_PERMS
"Database procedure '%0c'.'%1c' has permits
\ndefined on it even though it is marked non-grantable."
/*%
** Parameters:
**	%0c - name of a schema to which the database procedure belongs
**	%1c - name of a database procedure
**
** Explanation:
**	verifydb IIPROCEDURE test #8 discovered even though the database 
**	procedure is marked non-grantable, there are some permits defined on it.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the database 
**	procedure to be marked dormant.  
*/
S_DU16EA_UNEXPECTED_PERMOBJ_TYPE
"IIPROTECT contained unexpected probjtype (%0c).
\nprotabbase = %1d, protabidx = %2d, propermid = %3d."
/*%
** Parameters:
**	%0c - type of object on which the permit was defined
**	%1d - base id of an object on which the permit was defined
**	%2d - index id of an object on which the permit was defined
**	%3d - permit number
**
** Explanation:
**	verifydb IIPROTECT test #1 discovered a tuple with unexpected object 
**	type (expected object types are 
**	 - 'T' (table) 		    - ASCII 84, 
**	 - 'V' (view)  		    - ASCII 86,
**	 - 'P' (database procedure) - ASCII 80,
**       - 'E' (dbevent) 	    - ASCII 69
**
** System Status:
**      The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the permit tuple 
**	deleted
*/
S_DU16EB_NO_OBJECT_FOR_PERMIT
"%0c with id (%1d, %2d) on which permit %3d is defined
\ndoes not exist."
/*%
** Parameters:
**	%0c - object type (one of TABLE, VIEW, PROCEDURE, DBEVENT)
**	%1d - base object ID (from iiprotect.protabbase)
**	%2d - index object ID (from iiprotect.protabidx)
**	%3d - permit number (from iiprotect.propermid)
**
** Explanation:
**	verifydb test #44 found that the object that is supposed to receive
**	this permit doesn't exist (at least it's not listed in the appropriate 
**	catalog.)
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop all tuples with this base,index
**	ID from iiprotect unless in RUNINTERACTIVE mode -- then the user must
**	enter YES/NO permission.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU0321.)
*/
S_DU16EC_NAME_MISMATCH
"Name of %0c ('%1c'.'%2c')
\nfound in an IIPROTECT tuple does not match 
\nthe name ('%3c'.'%4c')
\nfound in the catalog describing the object itself."
/*%
** Parameters:
**	%0c - object type (one of TABLE, VIEW, PROCEDURE, DBEVENT)
**	%1c - schema name found in IIPROTECT tuple
**	%2c - object name found in IIPROTECT tuple
**	%3c - schema name found in the catalog describing the object itself
**	%4c - object name found in the catalog describing the object itself
**
** Explanation:
**	verifydb IIPROTECT test #5 found that the name of the schema and/or the
**	object stored in IIPROTECT tuple do not match those in the catalog 
**	describing the object itself (IIRELATION for tables and view, 
**	IIPROCEDURE for database procedures, or IIEVENT for dbevents.)
**
** System Status:
**      The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically update schema and object name in IIPROTECT
**	to match those in the catalog describing the object itself unless 
**	running in RUNINTERACTIVE mode -- then the user must enter YES/NO 
**	permission.
**
**      No correction can be made if verifydb is in REPORT mode.
*/
S_DU16ED_INVALID_OPSET_FOR_OBJ
"IIPROTECT.proopset (%0x) specifies privileges which are
\nillegal for %1c '%2c'.'%3c'."
/*%
** Parameters:
**	%0x - value of IIPROTECT.proopset
**	%1c - object type
**	%2c - schema to which the object belongs
**	%3c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #6 found that proopset attribute of the current
**	tuple represents privileges which are illegal for an object of the 
**	type to which this permit or security_alarm applies.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop this tuple unless it is running
**	in RUNINTERACTIVE mode -- then the user must enter YES/NO permission.
*/
S_DU16EE_INVALID_OPCTL_FOR_OBJ
"IIPROTECT.proopctl (%0x) specifies privileges which are
\nillegal for %1c '%2c'.'%3c'."
/*%
** Parameters:
**	%0x - value of IIPROTECT.proopctl
**	%1c - object type
**	%2c - schema to which the object belongs
**	%3c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #6 found that proopctl attribute of the current
**	tuple represents privileges which are illegal for an object of the 
**	type to which this permit or security_alarm applies.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically drop this tuple unless it is running
**	in RUNINTERACTIVE mode -- then the user must enter YES/NO permission.
*/
S_DU16EF_QUEL_PERM_MARKED_AS_SQL
"IIPROTECT tuple indicates that permit %0d 
\non %1c '%2c'.'%3c' was created using SQL
\nwhereas in fact it was created using QUEL."
/*%
** Parameters:
**	%0d - permit number (from iiprotect.propermid)
**	%1c - object type
**	%2c - schema to which the object belongs
**	%3c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #8 found that proflags indicates that the 
**	permit was created using SQL, but IIQRYTEXT indicates otherwise.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically update IIPROTECT tuple to indicate that the
**	permit was created using QUEL unless running in RUNINTERACTIVE mode -- 
**	then the user must enter YES/NO permission.
**
**      No correction can be made if verifydb is in REPORT mode.
*/
S_DU16F0_SQL_PERM_MARKED_AS_QUEL
"IIPROTECT tuple indicates that permit %0d 
\non %1c '%2c'.'%3c' was created using QUEL
\nwhereas in fact it was created using SQL."
/*%
** Parameters:
**	%0d - permit number (from iiprotect.propermid)
**	%1c - object type
**	%2c - schema to which the object belongs
**	%3c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #9 found that proflags indicates that the 
**	permit was created using QUEL, but IIQRYTEXT indicates otherwise.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically update IIPROTECT tuple to indicate that the
**	permit was created using SQL unless running in RUNINTERACTIVE mode -- 
**	then the user must enter YES/NO permission.
**
**      No correction can be made if verifydb is in REPORT mode.
*/
S_DU16F1_MULT_PRIVS
"IIPROTECT tuple indicates that permit %0d 
\non %1c '%2c'.'%3c' was created after 6.4
\nbut it conveys multiple privileges which suggests
\nthat it was created in 6.4 or before."
/*%
** Parameters:
**	%0d - permit number (from iiprotect.propermid)
**	%1c - object type
**	%2c - schema to which the object belongs
**	%3c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #10 found that proflags indicates that the 
**	permit was created after 6.4, but the permit conveys multiple privileges
**	which suggests that it was created in 6.4 or before
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will automatically update IIPROTECT tuple to indicate that the
**	permit was created prior to 1.0 unless running in RUNINTERACTIVE mode --
**	then the user must enter YES/NO permission.
**
**      No correction can be made if verifydb is in REPORT mode.
*/
S_DU16F2_PRE_10_PERMIT
"IIPROTECT tuple indicates that permit %0d 
\non %1c '%2c'.'%3c' was created before OpenIngres 1.0.
\nYou need to upgrade this database by running UPGRADEDB."
/*%
** Parameters:
**	%0d - permit number (from iiprotect.propermid)
**	%1c - object type
**	%2c - schema to which the object belongs
**	%3c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #11 found that proflags indicates that the 
**	permit was created before release 1.0 which suggests that the 
**	database has not been upgraded using UPGRADEDB.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	Upgrade the database using UPGRADEDB.
*/
S_DU16F3_LACKING_GRANT_OPTION
"User %0c who created permit %1d on
\n%2c '%3c'.'%4c' lacks GRANT OPTION FOR
\nprivileges specified in this permit."
/*%
** Parameters:
**	%0c - grantor of a permit (from iiprotect.prograntor)
**	%1d - permit number (from iiprotect.propermid)
**	%2c - object type
**	%3c - schema to which the object belongs
**	%4c - name of the object
**
** Explanation:
**	verifydb IIPROTECT test #12 found that grantor of a permit on an object
**	owned by some other user lacks GRANT OPTION FOR privileges specified
**	in the permit.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the object to be 
**	destroyed or, in the case of database procedures, marked dormant.
**
**      No correction can be made if verifydb is in REPORT mode.
*/
S_DU16F4_VIEW_NOT_ALWAYS_GRNTBL
"IIRELATION tuple describing view '%0c'.'%1c' 
\nindicates that it does not depend on any privileges, but IIPRIV
\ncontains tuple(s) describing privileges on which this view depends."
/*%
** Parameters:
**	%0c - name of a schema to which the view belongs
**	%1c - name of a view
**
** Explanation:
**	verifydb IIRELATION test #30 discovered that while the IIRELATION
**	tuple describing a view indicates that it does not depend
**	on any privileges, record(s) describing such dependencies
**	were found by examining IIPRIV.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the 
**	VGRANT_OK bit reset.
*/
S_DU16F5_MISSING_VIEW_INDEP_PRVS
"IIRELATION tuple describing view '%0c'.'%1c' 
\nindicates that it depends on some privilege(s), but IIPRIV
\ncontains no record of such dependency."
/*%
** Parameters:
**	%0c - name of a schema to which the view belongs
**	%1c - name of a view
**
** Explanation:
**	verifydb IIRELATION test #31 discovered that while the IIRELATION
**	tuple describing a view indicates that it depends on some privileges, 
**	record(s) describing such dependencies could not be found by examining 
**	IIPRIV.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants the 
**	view destroyed.
*/
S_DU16F6_INVALID_IIPROTECT
"Possible invalid permission found at\n
iiprotect tuple where protabbase = %0d and protabidx = %1d and\n
prouser = '%2c' and proopset = %3d from iiprotect\n
(referring to table %4c)."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb IIPROTECT test #12 found a permit on a table issued by
**	a user where no GRANT OPTION is now present.  Direct user to this
**	iiprotect tuple.
*/
S_DU1700_NO_PURGE_AUTHORITY
"WARNING:\n\n
  You ('%0c') do not have authority to purge\n
  database '%1c' owned by '%2c'."
/*%
** Parameters:
**	%0c - user name
**	%1c - database name
**	%2c - database owner
**
** Explanation:
**	The user does not have authority to purge this database; only the
**	DBA may do so. However, any valid superuser may use the "-u"
**	flag to become the DBA.
**
** System Status:
**	verifydb will not perform any purge operation on this database, but
**	it will continue on to attempt to process other databases in the scope
**	list (if any).
**
** Recommendation:
**	The user should run the verifydb purge operation using the "-u" flag
**	to specify the appropriate DBA name.
*/ 
S_DU1701_TEMP_FILE_FOUND
"Temporary file %0c\n
found in location %1c.\n"
/*%
** Parameters:
**	%0c - file name
**	%1c - file location
**
** Explanation:
**	verifydb found a temporary file while purging a database.
**
** System Status:
**	verifydb will automatically purge this file in PURGE or TEMP_PURGE
**	operation unless the mode restricts this.  The REPORT mode does not
**	permit deletion of the file; RUNINTERACTIVE mode requires user
**	permission to drop the file.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must enter YES/NO permission.
**	Otherwise, no action is required.
*/ 
S_DU1702_EXPIRED_TABLE_FOUND
"Table %0c\n
(owned by %1c) has expired."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	verifydb found an expired relation while purging a database.
**
** System Status:
**	verifydb will automatically purge this table in PURGE or EXPIRED_PURGE
**	operation unless the mode restricts this.  The REPORT mode does not
**	permit destruction of this table; RUNINTERACTIVE mode requires user
**	permission to drop this expired table.
**
** Recommendation:
**	If in RUNINTERACTIVE mode, the user must enter YES/NO permission.
**	Otherwise, no action is required.
*/
S_DU1703_DELETE_ERR
"Warning: verifydb was unable to drop this file.\n
     Please deal with this file manually, then restart verifydb.\n
     (This file is probably a directory or has protections on it)."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb could not delete this file during purge operation.
**
** System Status:
**	The server has already closed the thread, so verifydb has no option
**	but to discontinue execution.
**
** Recommendation:
**	None.
*/
S_DU1705_AFLAG_WARNING
"WARNING:  Verifydb -au flag is only used with ACCESSCHECK operation.\n
This flag is being ignored."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A flag was specified that is meaningless in the OPERATION context;
**	it will be ignored.
**
** System Status:
**	verifydb will ignore this invalid parameter and continue execution.
**
** Recommendation:
**	None.
*/
S_DU1706_NOT_STARDB
"WARNING:  Database %0c is not a Star database and will be skipped."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	The verifydb REFRESH_LDBS operation is only to be performed on
**	distributed databases.  This operation has been specified on a non-
**	Star database, and that database will be skipped.
**
**	If the user has specified the INSTALLATION or DBA scopes, they should
**	expect to see this warning, indicating that non-Star databases have been
**	skipped for an operation that works only on Star (or distributed)
**	databases.  If the DBNAME scope is specified and the user gets this
**	warning, it indicates a user error where they specified the name of
**	a database that they believed to be a Star (distributed) database, but
**	it was not.
**
** System Status:
**	verifydb will continue searching the scope list for other databases
**	to operate on.  If this was the only database in the scope list, then
**	verifydb will terminate.
**
** Recommendation:
**	If the user has specified the DBNAME scope, then the user must re-run
**	verifydb with the correct Star database name(s).
*/
S_DU1707_LCL_CAP_UNKNOWN
"Capability %0c is not defined in Local Database %1c (node %2c).\n
This capability will NOT be updated in Star catalog iiddb_ldb_dbcaps."
/*%
** Parameters:
**	%0c - capability name
**	%1c - database name
**	%2c - node name
**
** Explanation:
**	The verifydb REFRESH_LDBS operation could not determine the value of
**	the specified capability, so it could not update iiddb_ldb_dbcaps.
**
** System Status:
**	verifydb will skip this capability but will continue on to the next
**	database.
**
** Recommendation:
**	None.
*/
S_DU1708_LCL_CAP_MISSING
"Capability %0c has a value of %1c in Local Database %2c (node %3c).\n
This capability was not defined in star catalog iiddb_ldb_dbcaps."
/*%
** Parameters:
**	%0c - capability name
**	%1c - value of capability from local database
**	%2c - database name
**	%3c - node name
**
** Explanation:
**	The verifydb REFRESH_LDBS operation found a capability that is defined
**	in the local database but was not in iiddb_ldb_dbcaps.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1709_LCL_CAP_DIFFERENT
"Capability %0c has a value of %1c in Local Database %2c (node %3c),\n
but Star catalog iiddb_ldb_dbcaps has a value of %4c."
/*%
** Parameters:
**	%0c - capability name
**	%1c - value of capability (from local database)
**	%2c - database name
**	%3c - node name
**	%4c - value of capability (from iiddb_ldb_dbcaps)
**
** Explanation:
**	The verifydb REFRESH_LDBS operation found a capability that is defined
**	at one value in the local database but has a different value in
**	iiddb_ldb_dbcaps.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1710_2NDARY_INDEX
"Secondary index %0c (owned by %1c)\n
exists on the target table."
/*%
** Parameters:
**	%0c - secondary index name
**	%1c - secondary index owner
**
** Explanation:
**	verifydb's patch operation (XTABLE OR TABLE) will patch the disk file
**	associated with a table.  That patch operation may result in some/all
**	tuples being dropped from the table.  The patch operation operates only
**	on the base table.  If it changes the base table, then the strict
**	correlation between the secondary index and base table is lost.
**	Therefore, verifydb must drop all secondary indexes before operating
**	on the base table.
**
** System Status:
**	This is an informational message; processing continues.
**
** Recommendation:
**	None.
*/
S_DU1711_UNIQUE_CONSTRAINT
"Unique constraint %0c \n
exists on the target table."
S_DU1712_CHECK_CONSTRAINT
"Check constraint %0c \n
exists on the target table."
S_DU1713_REFERENCES_CONSTRAINT
"Referential constraint %0c \n
exists on the target table."
S_DU1714_PRIMARY_CONSTRAINT
"Primary constraint %0c \n
exists on the target table."
S_DU1750_LABEL_PURGE
"Starting check for unused security label ids in database %0c."
/*%
** Parameters:
**	%0 - database name
**
** Explanation:
**	This is an advisory message, indicating the start of the label
**	purge operation.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1751_LABEL_SCAN_TBL_COL
"Scanning %0c.%1c."
/*%
** Parameters:
**	%0 - table name
**	%1 - column name
**
**
** Explanation:
**	The is an advisory message, indicating verifydb is processing a
**	table/column.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1752_LABEL_ALL_USED
"All known short security labels in database %0c are in use."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	This is an advisory message, indicates all short security labels
**	are in use so no purge will be performed.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1753_LABEL_PURGING
"Removing unused short security labels from database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	This is an advisory message, indicates unused short security labels
**	are being deleted from the database.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1754_LABEL_UNREF
"%0d short security label(s) in database %1c are currently unused and
are available for purging."
/*%
** Parameters:
**	%0 - number of labels
**	%1 - database name
**
** Explanation:
**	This is an advisory message, indicates that unreferenced short security
**	labels were found in the indicated database. In RUNINTERACTIVE mode,
**	this may be followed by a prompt requesting whether to purge or not.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1755_LABEL_ROW_CNT
"%0d rows were deleted, old total so far is %1d"
/*%
** Parameters:
**	%0 - number of deleted labels
**	%1 - prior number deleted
**
** Explanation:
**	This is a debugging message indicating the number of rows deleted
**	while processing the current table/column.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	None.
*/
S_DU1760_MAND_FLAGS_INCORRECT
"Mandatory flags for catalog %0c are incorrect.\n
 relstat is %1d and should be %2d, relstat2 is %3d and should be %4d"
/*%
** Parameters:
**	%0 - catalog table name
**	%1 - expected value of relstat
**	%2 - actual value of relstat
**	%3 - expected value of relstat2
**	%4 - actual value of relstat2
**
** Explanation:
**	Verifydb found that the relstat and/or relstat2 columns in iirelation
**	were missing some mandatory flags. Certain system catalogs have to 
**      have certain flags set in order to function correctly.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants to set the
**      mandatory flags for relstat and relstat2 
**
**	No correction can be made if verifydb is in REPORT mode. (See DU045A.)
*/
S_DU1761_PHYSL_TBL_NOT_HASH
"Catalog %0c has TCB2_PHYSLOCK_CONCUR set which means its structure must be\n
 HASH but it is not."
/*%
** Parameters:
**	%0 - catalog table name
**
** Explanation:
**	Verifydb found that the TCB2_PHYSLOCK_CONCUR bit was set in relstat2
**	and that relspec indicated a structure other than hash. 
**      TCB2_PHYSLOCK_CONCUR allows transient physical locking on the catalog
**      but consistency can only be guaranteed if the structue is hash.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	verifydb will proceed to ask whether the user wants to modify the
**      catalog to its default storage structure.
**
**	No correction can be made if verifydb is in REPORT mode. (See DU045B.)
*/
S_DU17FC_TABLE_PATCH_WARNING
"WARNING:\n\n
  VERIFYDB operation TABLE (or XTABLE) can result in loss of user data when\n
  run in any mode except REPORT.  It will also modify the table to a heap\n
  and remove all secondary indices on the table.  This program is designed as\n
  a last-ditch attempt to recover a user table.  It is intended only for use\n
  by Ingres Corporation technical support representatives.\n
  It is NOT supported.  You assume all risk if you use the program.\n\n
  If you do use the TABLE or XTABLE operation, it is recommended that you\n
  backup the table's disk file before continuing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is a legal disclaimer that should be printed every time
**	the TABLE or XTABLE operation is specified.  If the user has selected
**	the RUNINTERACTIVE mode, this will be followed by a prompt asking the
**	user if they want to continue or abort.
**
** System Status:
**	This is a warning message, not an error.  Unless the user specifies
**	that they want to stop the process, verifydb continues processing.
**
** Recommendation:
**	No user action is possible unless the RUNINTERACTIVE mode has been
**	specified.  If that mode is specified, the user must respond to the
**	"Do you want to continue?" prompt with either "Y" or "N".
*/
S_DU17FD_CATALOG_PATCH_WARNING
"WARNING:\n\n
  VERIFY operation DBMS_CATALOGS can result in loss of user tables when\n
  used in the RUNINTERACTIVE mode.  This program is designed as a last-ditch\n
  attempt at recovery of an inconsistent database and is only for use by\n
  Ingres Corporation technical support representatives.\n
  It is NOT supported.  You assume all risk if you use this program.\n\n
  If you do use the DBMS_CATALOGS operation of VERIFYDB in the RUNINTERACTIVE\n
  mode, it is recommended that you back up your database before proceeding.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is a legal disclaimer that should be printed every time
**	the  DBMS_CATALOGS operation is specified.  If the user has selected
**	the RUNINTERACTIVE mode, this will be followed by a prompt asking
**	the user if they want to continue or abort.
**
** System Status:
**	This is a warning message, not an error.  Unless the user specifies
**	that they want to stop the process, verifydb continues processing.
**
** Recommendation:
**	If the user has not selected RUNINTERACTIVE mode, no damage will be
**	done to the database, and no interaction is required.  If the user
**	has specified the RUNINTERACTIVE mode, then the user must answer a
**	prompt asking whether or not they really want to run verifydb.
**	If the user has not already backed up their database, they should
**	probably answer "NO" to the prompt, then back up the database.  They
**	can then run verifydb.
**
**	If the user was running with checkpoints and journaling, it may be
**	better to roll back to the last checkpoint and recover their database
**	via journaling.  The verifydb patch option should not be used if the
**	user can recover via journaling.
*/
S_DU17FE_FORCE_CONSISTENT_WARN
"WARNING:\n\n
  VERIFYDB operation FORCE_CONSISTENT makes the database appear consistent to\n
  the DBMS but does not repair whatever caused the inconsistency to occur.\n
  The ONLY supported way to recover your database is to rollforward from a\n
  valid checkpoint or restore from an OS backup taken when Ingres was down.\n
  FORCE_CONSISTENT will make it possible for you to access the database and\n
  attempt to examine and fix the tables. 
  Check the iircp.log to determine why the database was marked inconsistent.\n
  Loss of data may have occurred if the database was marked inconsistent by\n
  the recovery server because of failure during online/offline recovery.\n
  Use logdump to display log records written since the last consistency point.\n
  Use VERIFYDB operation TABLE to check table consistency.\n
  There is an increased risk to data integrity if you continue using a\n 
  database that has been forced consistent.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The message is a legal disclaimer that the DBMS server cannot ensure
**	the database will stay consistent after it has been marked inconsistent
**	and then forced consistent by verifydb.  If the user continues using
**	this database after forcing it consistent, it is possible that the new
**	work done may be lost at some future date.
**
** System Status:
**	This is a warning message, not an error.  Unless the user specifies
**	that they want to stop the process, verifydb keeps processing.
**
** Recommendation:
**	If the user has not selected RUNINTERACTIVE mode, verifydb will just
**	print the warning message and continue its processing.  If the user
**	has specified the RUNINTERACTIVE mode, then the user must answer a
**	prompt asking whether or not they really want to run verifydb.
**
**	If the user has journaling enabled, they may prefer to roll back to
**	the last checkpoint and then use journaling to recapture the work
**	they have done since that checkpoint.  If they do not have journaling
**	enabled, then they probably do want to force the database consistent.
**
**	However, once the database has been forced consistent, it should not
**	be used as though it had never been marked inconsistent.  The
**	user is expected to use unloaddb to get their data out of this
**	database.  They are expected to destroy this database and recreate it,
**	then run the reload.ing script that unloaddb creates.  If they keep
**	using the database they have forced consistent, they may run into
**	future problems.
*/
S_DU17FF_FIX_INTERACTIVELY
"VERIFYDB cannot perform the recommended action unless\n
the RUNINTERACTIVE mode is selected.\n
Please fix manually or rerun VERIFYDB with -mruninteractive"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The fix for this verifydb test may only be performed in RUNINTERACTIVE
**	mode if the user gives Yes permission when prompted for it.  Since the
**	current mode is not RUNINTERACTIVE, no corrective active may be taken.
**
**	This message is displayed in lieu of a prompt (by duve_talk()) whenever
**	the message type is INTERACTIVE (permit fix only in interactive mode),
**	and the mode is not RUNINTERACTIVE. This message will not be displayed
**	if the mode is REPORT.
**
** System Status:
**	The verifydb process continues.
**
** Recommendation:
**	The user may opt to manually take corrective action or to rerun verifydb
**	in the RUNINTERACTIVE mode.
*/
W_DU1800_STAR_CDB_NOT_FOUND
"  The coordinator database does not exist, destruction of '%0c' continue...\n"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	The coordinator database name in the iistar_cdbs was not found
**	in the iidatabase.  
**
** System Status:
**	The process aborts after removing entries of the distributed
**	database from iistar_cdbs and iidatabase.
**
** Recommendation:
**	None; warning message only.
*/
W_DU1801_SV_NO_DDB_SELECTED
"No DDB has been selected yet, cannot 'Go' without a selected DDB\n
Please Select a DDB."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Select "Go" when no distributed database has being selected.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Select a distributed database, then select the "Go" option.
*/
W_DU1802_SV_BAD_NODENAME
"Node name: '%0c' is specified incorrectly."
/*%
** Parameters:
**	%0c - bad node name
**
** Explanation:
**	Node name did not pass the valid name test.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Specify a node name correctly.
*/
W_DU1803_SV_BAD_DDBNAME
"Distributed Database name: '%0c' is specified incorrectly."
/*%
** Parameters:
**	%0c - invalid distributed database name
**
** Explanation:
**	The distributed database name did not pass the valid name test.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Specify a DDB name correctly.
*/
W_DU1804_SV_CANT_GET_LDBLIST
"Cannot obtain list of Local Databases from Distributed Database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot get a list of local database information.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Check the local database permission or network connection.
*/
W_DU1805_SV_CANT_OPEN_DDB
"Cannot open Distributed Database '%0c', check Lock."
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Cannot open the specified distributed database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Check for distributed database lock, or existence.
*/
W_DU1806_SV_CANT_DIRECT_CONNECT
"Cannot direct connect to '%0c'."
/*%
** Parameters:
**	%0c - local or coordinator database
**
** Explanation:
**	Cannot direct-connect to a local or coordinator database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Check server type.
*/
W_DU1807_SV_CANT_GET_DDB_INFO
"Cannot get information from Distributed Database '%0c'."
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Cannot obtain information from the distributed database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the distributed database.
*/
W_DU1808_SV_CANT_GET_LDB_INFO
"Cannot get information from Local Database '%0c'."
/*%
** Parameters:
**	%0c - local database name
**
** Explanation:
**	Cannot obtain information from the local database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the local database.
*/
W_DU1809_SV_CANT_DIR_DISCONNECT
"Cannot direct disconnect, please check if connected to STAR server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	There is a problem with direct disconnect.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the name server, or communication server.
*/
W_DU180A_SV_CANT_OPEN_IIDBDB
"Cannot connect to 'iidbdb', please check Local and Distributed installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot connect to iidbdb.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the name server, or communication server.
*/
W_DU180B_SV_CANT_DISCONNECT
"Cannot disconnect, please check Local and Distributed server status."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot disconnect.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the name server, or communication server.
*/
W_DU180C_SV_CANT_OPEN_DDB
"Cannot establish connection to Distributed Database"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot open the distributed database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the name server, or communication server.
*/
W_DU180D_SV_CANT_FIND_VNODE
"Cannot find system attribute for II_GCNxx_LCL_VNODE, please check STAR installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot obtain system attribute II_GCNxx_LCL_VNODE.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Contact the system administrator.
*/
W_DU180E_SV_ENTER_BAD_VNODE
"Entered VNODE name is bad, please retry."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Bad VNODE name encounter.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU180F_SV_CANT_DIRCONNECT_LDB
"Cannot direct connect to Local Database '%0c'."
/*%
** Parameters:
**	%0c - local database name
**
** Explanation:
**	There is a problem with direct-connect to the local database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the name server, or communication server.
*/
W_DU1810_SV_CANT_CLOSE_DDB
"Cannot disconnect from Distributed database, session could have been dropped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot disconnect from distributed database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Exit StarView and check the name server, or communication server.
*/
W_DU1811_SV_CURSOR_NOT_IN_TABLE
"Screen cursor is not in table field."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The screen cursor is not in the appropriate table field.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Move cursor to the table field.
*/
W_DU1812_SV_EMPTY_TABLE
"Table is empty."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The table is empty.
**
** System Status:
**	Informational only; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1813_SV_CANT_DROP_REG_TABLE
"Error during drop operation on REGISTERED TABLE '%0c'."
/*%
** Parameters:
**	%0c - object name
**
** Explanation:
**	Cannot drop the user-specified registered object.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1814_SV_CANT_DROP_TABLE
"Error during drop operation on TABLE object '%0c'."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	Cannot drop the table.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1815_SV_CANT_DROP_VIEW
"Error during drop operation on VIEW object '%0c'."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	Cannot drop the view.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1816_SV_CANT_REGISTER_TABLE
"Error during REGISTER of object '%0c'."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	Cannot drop the register table.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1817_SV_ERROR_IN_CRITERIA
"Error detected in the selection criteria, please reenter."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Bad data was encountered in selection criteria.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Re-enter the correct data.
*/
W_DU1818_SV_DB_DOES_NOT_EXIST
"Database does not exist, please try again with a different database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Database does not exist.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1819_SV_RM_NONE_REG_OBJ
"Cannot remove a non-register object."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	Cannot remove the non-register object.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU181A_SV_CANT_REG_FROM_CRIT
"Cannot register table from the given selection criteria, please try again."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot register the table from the given criteria.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Re-enter the correct criteria.
*/
W_DU181B_SV_NONE_FUNCTIONAL
"This part of StarView is not yet functional, but will be available\n
in future releases."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user came upon an option that is not yet functional.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Re-enter the correct criteria.
*/
W_DU181C_SV_CANT_FIND_STRING
"Can not find a match in table"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to remove an index object.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU181D_SV_EMPTY_LDB
"Local database is empty."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The local database does not have any objects to register.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Return to the previous browse popup.
*/
W_DU181E_SV_DUPLICATE_REG
"Duplicate REGISTER name, select a different name."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user tried to register a duplicate object.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Re-enter the correct criteria.
*/
W_DU181F_SV_NO_DDBS_FOUND
"No distributed databases were found."
/*%
** Parameters:
**	None.	
**
** Explanation:
**	No distributed databases have been created at this vnode.	
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Choose another vnode, or exit STARVIEW.
*/
W_DU1830_STAR_DESTRY_CDB
"Destroydb destroying a coordinator database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The distributed database in iidatabase has no matching tuple in
**	iistar_cdbs.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1831_SUPUSER_OBSOLETE
"  The -s (SUPERUSER) option is now obsolete.\n
       The appropriate privileges are now determined automatically.\n
       This program will continue to run but usage of the -s option\n
       should be discontinued."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Previous versions of sysmod and destroydb required a superuser
**	to specify a "-s" option, indicating the user is a superuser.
**	The superuser privilege has been replaced by more selective
**	privileges, which are determined automatically. Because of this
**	the "-s" flag is obsolete.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	The user should discontinue using the "-s" flag.
*/
W_DU1840_DDBTUPS_NOT_FOUND_FI
"Distributed databases in IIDATABASE have no corresponding tuple in IISTAR_CDBS:"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The distributed database in iidatabase has no matching tuple in
**	iistar_cdbs.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1841_CDBTUPS_NOT_FOUND_FI
"Coordinator databases in IIDATABASE have no corresponding tuple in IISTAR_CDBS:"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The coordinator database in iidatabase has no matching tuple in
**	iistar_cdbs.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1842_STARTUPS_NOT_FOUND_FI
"Distributed entries found in IISTAR_CDBS not in the IIDATABASE catalog:"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	iistar_cdbs contains more than one entry for the specified database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1843_DUP_STARTUP_FI
"The IISTAR_CDBS catalog in IIDBDB contains more than one entry for the database '%0c'."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The distributed database in iidatabase has no corresponding coordinator
**	database in iistar_cdbs.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1844_DDBCDB_NOT_FOUND_FI
"Distributed databases in IIDATABASE have no corresponding coordinator database:"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The distributed databases in iidatabase have no corresponding
**	coordinator database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1845_DBOPEN_FAIL_FI
">>Unable to open database '%0c' when testing for distributed database"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Cannot restore distributed database in iidatabase.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1846_DDB_RESTORE_FAIL_FI
">>Unable to restore distributed database '%0c' in IIDATABASE:"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	A duplicate tuple was found in iidatabase when restoring database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	None.
*/
W_DU1847_DDBDUPLICATE_ERROR_FI
">>Duplicate tuple was found in IIDATABASE when restoring distributed database '%0c':"
W_DU1850_IIDBDB_ERR
"\nError while converting the iidbdb database.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb could not successfully upgrade the iidbdb.
**
** System Status:
**	Warning message; earlier messages should indicate what the problem was.
**
** Recommendation:
**	Fix installation problem that prevents upgradedb from completing, then
**	reissue the upgradedb command.
*/
W_DU1851_JOURNAL_WARN
"*** WARNING ***\n
Database '%0c' was being journaled.\n
Journaling is being disabled for this database.\n
Be sure to resume journaling by running\n
\t \"ckpdb +j iidbdb\"\n
after this database is successfully upgraded."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb is turning off journaling.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Follow the advice in given in this warning message.
*/
W_DU1852_CDB_AND_DDB
"WARNING!!!\n
\tDatabase '%0c' is marked as both a CDB and a DDB.\n
\tUPGRADEDB will not upgrade this Database."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb cannot determine if this database is a coordinator
**	database (CDB) or a distributed database (DDB).
**
** System Status:
**	The upgrade of this database is skipped, but upgradedb continues to the
**	next database in the target list (if any).
**
** Recommendation:
**	The user needs to correct the erroneous DDB/CDB. Probably the best
**	way is to unload the distributed database, destroy it, re-create it,
**	and reload it with data -- all using the previous release server. Once
**	that is complete, try upgradedb again.
*/
W_DU1853_CDB_SKIPPED
"Database '%0c' is a coordinator database\n
\tfor Distributed Database '%1c'.\n
UPGRADEDB will NOT upgrade CDBs, but will upgrade the DDB.\n
...SKIPPING UPGRADE OF DATABASE '%2c'..."
/*%
** Parameters:
**	%0c - distributed database name
**	%1c - coordinator database name
**	%2c - target database name
**
** Explanation:
**	upgradedb cannot determine if this database is a coordinator database
**	or a distributed database.
**
** System Status:
**	The upgrade of this database is skipped, but upgradedb continues to the
**	next database in the target list (if any).
**
** Recommendation:
**	The user needs to correct the erroneous DDB/CDB. Probably the best
**	way is to unload the distributed database, destroy it, re-create it,
**	and reload it with data -- all using the previous release server. Once
**	that is complete, try upgradedb again.
*/
W_DU1854_ERROR_SKIP
"Database '%0c' is being skipped due to previous errors...."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb is refusing to operate on this particular database.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	None.
*/
W_DU1855_TRACE_3
"UPGRADEDB trace 3.  Upgrade of FE catalogs is skipped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb was invoked with trace 3 selected, so upgradedb is skipping
**	calls to upgradefe.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	None.
*/
W_DU1856_TRACE_4
"UPGRADEDB trace 4.  Upgrade of BE catalogs is skipped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb was invoked with trace 4 selected, so upgradedb is skipping
**	the upgrade of DBMS catalogs.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	None.
*/
W_DU1857_UNCONVERTED_V5DB
"Database '%0c' is an unconverted release 5 database.\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb is refusing to operate on this particular database, because
**	it is an unconverted release 5 database.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	If the user wants to bring this database to the current release 6, use
**	convto60, not upgradedb.
*/
W_DU1858_INOPERATIVE_DB
"Database '%0c' is currently inoperative.\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb is refusing to operate on this particular database, because
**	it is not an operative database (i.e., it is only partially created or
**	partially destroyed.)
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Fix the problem by destroying the inoperative database. If the user
**	user intends for this database to be used, then call
**	Technical Support instead of destroying it.
*/
W_DU1859_WRONG_LEVEL
"Database '%0c' is not at the proper level to be converted by UPGRADEDB.\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb is refusing to operate on this particular database, because
**	it is not an operative database (i.e., it is only partially created or
**	partially destroyed.)
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Fix the problem by destroying the inoperative database. If the user
**	user intends for this database to be used, then call
**	Technical Support instead of destroying it.
*/
W_DU185A_ALREADY_CURRENT
"Database '%0c'\n
is currently at Release %1c level.\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	This database did not meet any criteria for conversion.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	None.
*/
W_DU185B_BAD_DUMP_INFO
" -- Unable to obtain dump information.\n
Conversion bypassed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb could not find an entry in iilocations for the dmpdev
**	(default dump location) from iidatabase.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Use accessdb to correctly define the dump location.
*/
W_DU185C_BAD_CONFIG_READ
" -- Unable to read the configuration file.\n
Conversion bypassed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb could not read status information from the config file.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Verify that the executable is correctly installed and that the
**	default data directory has the correct permissions.  Also verify
**	that the entry in iidatabase and iilocations correctly describes
**	the databases default data location and correctly reflects the
**	level (ie, "6.2", "6.3", "6.34", etc.) that the database is at.
*/
W_DU185D_IS_JOURNALED
"*** WARNING ***\n
Database '%0c' is being journaled.\n
You cannot upgrade a journaled database!\n
Turn off journaling using the command:
\t ckpdb -j <dbname>\n
After upgrading resume journaling with:\n
\t ckpdb +j <dbname>\n\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	The database is journaled and it is at least release 6.3, so upgradedb
**	cannot turn off journaling and the user must do so before the database
**	can be upgraded.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Follow the advice provided in this message.
*/
W_DU185E_BAD_CONFIG_UPDATE
" -- Unable to update the configuration file.\n
Conversion bypassed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb could not update status information from the config file.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Verify that the executable is correctly installed and that the
**	default data directory has the correct permissions.  Also verify
**	that the entry in iidatabase and iilocations correctly describes
**	the databases default data location and correctly reflects the
**	level (ie, "6.2", "6.3", "6.34", etc) that the database is at.
*/
W_DU185F_BAD_DELETE
"Unable to delete the dmp configuration file for database '%0c'\n
Unable to complete conversion for this database."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not clean up outdated journal/recovery information.
**
** System Status:
**	Processing continues, but this database is skipped.
**
** Recommendation:
**	Verify that the executable is correctly installed and that the
**	default data directory has the correct permissions.  Also verify
**	that the entry in iidatabase and iilocations correctly describes
**	the databases default data location and correctly reflects the
**	level (ie, "6.2", "6.3", "6.34", etc) that the database is at.
*/
W_DU1860_SKIP_DB
"Database '%0c' will be skipped."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb encountered an error connecting to this database.  This
**	usually occurs if someone else is in the database (because upgradedb
**	attempts to take an exclusive lock on the database).
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Re-issue the command when the database is available.
*/
W_DU1861_JNL_WARNING
"*** WARNING ***\n
Database '%0c' was being journaled.\n
Journaling is being disabled for this database.\n
Be sure to resume journaling by running\n
\t ckpdb +j %0c\n
after this database is successfully upgraded.\n"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb will be disabling journaling on this database.  It is a
**	side-effect of connecting with the "-A6" flag.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Follow the advice provided in this  message to turn journaling back
**	on after the upgrade completes.
*/
W_DU1862_CNF_UPDATE_FAIL
"Unable to update configuration file compatibility level for database '%0c'\n
Unable to complete conversion for this database."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not update the config file for this database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Verify that the executable is correctly installed and that the
**	default data directory has the correct permissions.  Also verify
**	that the entry in iidatabase and iilocations correctly describes
**	the databases default data location and correctly reflects the
**	level (ie, "6.2", "6.3", "6.34", etc) that the database is at.
*/
W_DU1863_CNF_UPDATE_FAIL
"Unable to update configuration file access for database '%0c'\n
Unable to complete conversion for this database."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not update the config file for this database.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Verify that the executable is correctly installed and that the
**	default data directory has the correct permissions.  Also verify
**	that the entry in iidatabase and iilocations correctly describes
**	the databases default data location and correctly reflects the
**	level (ie, "6.2", "6.3", "6.34", etc) that the database is at.
*/
W_DU1864_FE_CAT_FAIL
"Front End Catalog conversion for '%0c' will be skipped.\n
You may manually run UPGRADEFE on this database at a later date."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not connect to a Star server.  This is usually
**	caused when upgradedb is invoked on a Star database when there
**	is no Star server running.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Bring up a Star server, then run upgradefe as follows:
**	"upgradefe <dbname>/star -f <client list>".
*/
W_DU1865_MISSING_UTEXE_DEF
"Cannot find 'upgradefe' in utexe.def.\n
Please check your installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	File ii_config:utexe.def does not have an entry for upgradefe;
**	therefore, the upgradefe spawn failed.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Add an entry, then run upgradefe or re-run upgradedb.
*/
W_DU1866_BAD_UTEXE_DEF
"Utexe.def entry is apparently incorrect.\n
Please check your installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	File ii_config:utexe.def has an incorrect entry for upgradefe;
**	therefore, the upgradefe spawn failed.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Correct the entry, then run upgradefe or re-run upgradedb.
*/
W_DU1867_UGRADEFE_FAIL
"Error calling 'upgradefe'\n
\tfailure status: %0d\n
\t(UPGRADEFE upgrades Front End catalogs)\n
\tYou may run upgradefe directly on the database\n
\tor you may correct the problem and re-run upgradedb."
/*%
** Parameters:
**	%0d - status of upgradefe spawn
**
** Explanation:
**	The upgradefe spawn failed with the specified status.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	Try running upgradefe directly.
*/
W_DU1868_DESTROYED_DB
"Database '%0c' was destroyed and will not be upgraded."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb cached information about databases in the installation.
**	After it cached this information, this database was destroyed,
**	so upgradedb cannot operate on it.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	The user should determine if the database should have been destroyed or
**	updated.  If it should not have been destroyed, then restore the
**	database from a backup tape, run finddbs, then upgrade the database.
*/
W_DU1869_NOMODIFY
"  Unable to modify system catalog '%0c'.\n
   Please verify that you have privileges required,\n
   and that the II_DATABASE location has adequate free disk space.\n
   When you have determined and addressed the problem,\n
   run sysmod on database '%1c'."
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
**	upgradedb tried to modify a system catalog back to its storage
**	structure, probably after repopulating it.  The server returned
**	an SQL error on the modify.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	The user should determine why the modify failed.  When the cause has
**	been found and fixed, run sysmod on the database.
*/
W_DU186A_MISSING_SYSCAT
" System catalog '%0c' not found in database '%1c'.\n"
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
**	upgradedb couldn't find a system catalog that it wanted to upgrade
**	to 6.5 status.  So, this catalog will be empty in 6.5, because it
**	wasn't there in 6.4 to copy from.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	The user should determine what happened to the missing system catalog.
**	The user may have to add the missing tuples to the 6.5 catalog.
*/
W_DU186B_REGISTER_FAILED
"  Register failed.  Please verify that there is sufficient disk space"
W_DU186C_UPGRADEDB_UNLOADDB
"Warning: UPGRADEDB error upgrading trees, UNLOADDB failed"
W_DU186D_UPGRADEDB_SQL
"Warning: UPGRADEDB error upgrading trees, SQL failed:\n %0c \n"
W_DU186E_VIEW_UPGRADE
"Warning: UPGRADEDB %0d Views, %1d recreated."
W_DU186F_INGEG_UPGRADE
"Warning: UPGRADEDB %0d Integrities, %1d recreated."
W_DU1870_PROC_UPGRADE
"Warning: UPGRADEDB %0d Procedures, %1d recreated."
W_DU1871_CONS_UPGRADE
"Warning: UPGRADEDB %0d Constraints, %1d recreated."
W_DU1872_UPGRADEDB_TREE_DB
"Warning: UPGRADEDB Errors occurred upgrading trees for database %0c."
W_DU1873_UPGRADEDB_TREE
"Warning: UPGRADEDB had errors upgrading trees."
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb couldn't register an IMA table.
**
** System Status:
**	Warning message; processing continues.
**
** Recommendation:
**	The user should determine why the registration failed (e.g., out of
**	disk space).  The user may have to add the missing registration by hand.
*/
W_DU1874_UPGR_NOT_STAR_OWNER
"Database '%0c' is a star database.\n
It is recommended to upgrade a star database as the database owner\n
to ensure access to vnodes of distributed databases.\n
Please re-run upgradedb '%0c' while logged in as user '%1c'."
/*%
** Parameters:
**	%0c - distributed database name
**	%1c - database owner
**
** Explanation:
**	upgradedb cannot upgrade query trees for views registered as links to local
**	databases on private vnodes unless run as the database or vnode owner respectively.
**
** System Status:
**	The upgrade of this database is skipped, but upgradedb continues to the
**	next database in the target list (if any).
**
** Recommendation:
**	The user should re-run upgradedb for the star database as the actual
**	owner of the database.
*/
E_DU2000_BAD_ERLOOKUP
"  Unknown database utility error number '%0x'."
E_DU2001_BAD_ERROR_PARAMS
"  Bad error parameters passed to routine, ds_error()."
E_DU2002_BAD_SYS_ERLOOKUP
"  Bad lookup of operating system error."
E_DU2003_BAD_ACT_JNL_DIR
"  Bad build of pathname for active journal directory %0c %1c"
E_DU2004_BAD_CKPT_DIR
"  Bad build of pathname for checkpoint directory %0c %1c"
E_DU2005_BAD_DFLT_DB_DIR
"  Bad build of pathname for default database directory %0c %1c"
E_DU2006_BAD_XPIR_JNL_DIR
"  Bad build of pathname for expired journal directory %0c %1c"
E_DU2007_BAD_EXT_DB_DIR	
"  Bad build of pathname for extended database directory %0c %1c"
E_DU2008_BAD_FULL_JNL_DIR
"  Bad build of pathname for full journal directory %0c %1c"
E_DU2009_BAD_JNL_DIR
"  Bad build of pathname for journal directory %0c %1c"
E_DU2013_GW_STDCATS_INT_ERR
"  Internal error -- bad Gateway argument passed to dui_stdcats()"
E_DU2014_GRANTING_SEL_ON_CATS
"An error was encountered while attempting to GRANT SELECT TO PUBLIC WITH
\nGRANT OPTION on catalogs."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error occurred when trying to GRANT SELECT TO PUBLIC WITH 
**	GRANT OPTION on catalogs that are base tables or SQL views.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	Examine the error log; contact Ingres Corporation Technical Support.
*/
E_DU2100_BAD_RECONNECT
"Unable to connect to STAR server and also unable to reconnect to local server."
/*% E_DU2100_BAD_RECONNECT
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They do some work via the
**	local server, disconnect from it, then attempt to connect to the Star
**	server.  If the connection to the Star server fails, they will attempt
**	to reconnect to the local server.  If that also fails, then there is
**	probably something that has gone wrong in the installation and
**	this message is printed.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that local and Star servers are up and operative
**	in their installation.  The user should verify that there are no other
**	installation problems that can make it impossible to connect to these
**	servers.  If the user is doing a createdb, they must use destroydb to
**	destroy what they attempted to create, then re-issue the createdb
**	command.  If the user is attempting an upgradedb, they should re-issue
**	the command.
*/
E_DU2101_BAD_DISCONNECT
"Fatal error.  Unable to close the connection to the STAR server and re-open\n
a connection to the local server."
/*% E_DU2101_BAD_DISCONNECT
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They do some work via the
**	local server, disconnect from it, then connect to the Star server and
**	spawn upgradefe.  After upgradefe completes, the utility attempts to
**	disconnect from the Star server and reconnect to the local server.  In
**	this instance, the utility received an error while attempting to
**	disconnect from the Star server or reconnect to the local server.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that local and Star servers are up and operative
**	in their installation.  The user should verify that there are no other
**	installation problems that can make it impossible to connect to these
**	servers.  If the user is doing a createdb, they must use destroydb to
**	destroy what they attempted to create, then re-issue the createdb
**	command.  If the user is attempting an upgradedb, they should re-issue
**	the command.
*/
E_DU2102_BAD_STARDISCONNECT
"Unable to close session to the STAR server."
/*% E_DU2102_BAD_STARDISCONNECT
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They connect to a Star
**	server to do so, and when the upgradefe has completed, the utility
**	disconnects from the Star server and reconnects to the local server.
**	However, the utility received an error when attempting to disconnect
**	from the Star server.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that local and Star servers are up and operative
**	in their installation.  The user should verify that there are no other
**	installation problems that can make it impossible to connect to these
**	servers.  If the user is doing a createdb, they must use destroydb to
**	destroy what they attempted to create, then re-issue the createdb
**	command.  If the user is attempting an upgradedb, they should re-issue
**	the command.
*/
E_DU2103_BAD_COMMIT
"Unable to commit a transaction."
/*% E_DU2103_BAD_COMMIT
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They first attempt to
**	close the transaction they are in, then close the local database
**	association and attempt to connect to a Star server.  However, in
**	this case, the utility received an error while attempting to commit
**	the transaction.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that local and Star servers are up and operative
**	in their installation.  The user should verify that there are no other
**	installation problems that can make it impossible to connect to these
**	servers.  If the user is doing a createdb, they must use destroydb to
**	destroy what they attempted to create, then re-issue the createdb
**	command.  If the user is attempting an upgradedb, they should re-issue
**	the command.
*/
E_DU2104_BAD_LOCALCONNECT
"Error reconnecting to the local server."
/*% E_DU2104_BAD_LOCALCONNECT
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They do some work via the
**	local server, disconnect from it, then connect to the Star server and
**	spawn upgradefe.  After upgradefe completes, the utility attempts to
**	disconnect from the Star server and reconnect to the local server.  In
**	this instance, the utility received an error while attempting to
**	reconnect to the local server.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that local and Star servers are up and operative
**	in their installation.  The user should verify that there are no other
**	installation problems that can make it impossible to connect to these
**	servers.  If the user is doing a createdb, they must use destroydb to
**	destroy what they attempted to create, then re-issue the createdb
**	command.  If the user is attempting an upgradedb, they should re-issue
**	the command.
*/
E_DU2105_BAD_LOCALDISCONNECT
"Fatal error.  Unable to close the connection to the local server."
/*% E_DU2101_BAD_DISCONNECT
** Parameters:
**	None.
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They do some work via the
**	local server, disconnect from it, then connect to the Star server and
**	spawn upgradefe.  After upgradefe completes, the utility attempts to
**	disconnect from the Star server and reconnect to the local server.  In
**	this instance, the utility received an error while attempting to
**	disconnect from the local server.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that local and Star servers are up and operative
**	in their installation.  The user should verify that there are no other
**	installation problems that can make it impossible to connect to these
**	servers.  If the user is doing a createdb, they must use destroydb to
**	destroy what they attempted to create, then re-issue the createdb
**	command.  If the user is attempting an upgradedb, they should re-issue
**	the command.
*/
E_DU2106_UNOPENED_DB
"***********************************************************************\n
ERROR!  Database '%0c' should be open but is not.\n
***********************************************************************"
/*% E_DU2106_UNOPENED_DB
** Parameters:
**	%0c - coordinator database name
**
** Explanation:
**	Some utilities (createdb, upgradedb) need to spawn upgradefe while
**	connected to a Star server if they are attempting to operate (i.e.,
**	create or upgrade) a distributed database.  They do some work via the
**	local server, disconnect from it, then connect to the Star server and
**	spawn upgradefe.  After upgradefe completes, the utility attempts to
**	disconnect from the Star server and reconnect to the local server.  In
**	this instance, the upgradedb utility should have been connected to the
**	coordinator database via a local server and was not.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user should verify that the local and Star servers are up and
**	operative in their installation.  The user should verify that there
**	are no other installation problems that can make it impossible to
**	connect and do commits via these servers, then re-issue the upgradedb
**	command.
*/
E_DU2410_GEN_CL_FAIL
"  An unexpected error was returned from the CL routine '%0c'\n
when called from the routine '%1c'."
E_DU2411_BAD_LOINGPATH
" An error was encountered when trying to build an INGRES path\n
to '%0c %1c'."
E_DU2412_CANT_TRANSLATE_LOC
" An error was encountered when trying to translate location\n
'%0c' into a pathname."
E_DU2414_BAD_CASE_LABEL
" An unknown case label '%0d' was passed into the routine '%1c'."
E_DU2415_BAD_DMP_DIR
" Bad dump directory."
E_DU2416_NOCOPY_SYSCAT
" Unable to copy the system catalog '%0c'\n
  to a temporary table while upgrading database '%1c'.\n  
  Please verify that the II_DATABASE location has sufficient free disk space"
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
** 	6.5 upgradedb copies system catalogs to temporary tables
**	to implement alter table.  If this fails, there's no point in
**	trying to continue.  Probably the disk lacks sufficient
**	free space.
**
** System Status:
**	Execution stops; the database is not upgraded.
**
** Recommendation:
**	Verify that the II_DATABASE location has sufficient free disk space.
*/
E_DU2417_NODROP_SYSCAT
" Unable to drop the system catalog '%0c'\n
  while upgrading database '%1c'.\n
  Please verify that you have sufficient privilege to run upgradedb."
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
** 	After 6.5 upgradedb copies system catalogs to temporary tables,
**	it drops the original table to implement alter table.  If this
**	fails, there's no point in trying to continue.
**
** System Status:
**	Execution stops; the database is not upgraded.
**
** Recommendation:
**	Verify that upgradedb is run with required privileges.
*/
E_DU2418_NOCREATE_SYSCAT
" Unable to create the system catalog '%0c'.\n
  while upgrading database '%1c'.\n
  Please verify that you have sufficient privilege to run upgradedb,\n
  and that the II_DATABASE location has sufficient free disk space."
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
** 	After 6.5 upgradedb copies system catalogs to temporary tables
**	and drops the original table, it creates the original table
**	with the new 6.5 definition.  If this fails, incorrect privileges
**	are the most likely cause.
**
** System Status:
**	Execution stops; the database is not upgraded.
**
** Recommendation:
**	Verify that upgradedb is run with the required privileges.
*/
E_DU2419_NOREPOP_SYSCAT
" Unable to repopulate the system catalog '%0c'\n
  while upgrading database '%1c'.\n
  Please verify that the II_DATABASE location has sufficient free disk space."
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
** 	After 6.5 upgradedb copies system catalogs to temporary tables,
**	drops the original table, and creates the original table
**	with the new 6.5 definition, it copies the data back from
**	the temporary table.  If this fails, free disk space is
**	the most likely cause.
**
** System Status:
**	Execution stops; the database is not upgraded.
**
** Recommendation:
**	Verify that the II_DATABASE location has sufficient free disk space.
*/
E_DU241A_SYSCAT_UPGRADE_FAILED
" Error upgrading a system catalog"
E_DU241B_NODROP_TEMP
" Error dropping a temporary table while upgrading system catalog\n
  '%0c' in database '%1c'.\n
  Please verify that you have sufficient privilege to run upgradedb.\n"
/*%
** Parameters:
**	%0c - system catalog name
**	%1c - database name
**
** Explanation:
** 	When 6.5 upgradedb copies system catalogs to temporary tables,
**	it first checks to see if the temporary table is already
**	there.  If so, it drops it.  If this fails, something is
**	very wrong, and upgradedb cannot proceed.
**
** System Status:
**	Execution stops; the database is not upgraded.
**
** Recommendation:
**	Verify that the user has sufficient privileges.
*/
E_DU241C_SELECT_ERR_UPGR_IIDEFAULT
" SQL error %0x selecting key values from the iidefault catalog."
E_DU241D_INSERT_ERR_UPGR_IIDEFAULT
" SQL error %0x inserting a row into the iidefault catalog."
E_DU2F00_TOO_MANY_ARGS
"  There were too many arguments, %0d, passed to the 6.0 phase of CONVTO60."
E_DU3000_BAD_USRNAME
"  The user name, %0c, is syntactically incorrect for the INGRES DBMS."
E_DU3001_USER_NOT_KNOWN
"  The user, %0c, is not known to this installation of the INGRES\n
DBMS."
E_DU3002_ALIAS_NOT_KNOWN
"  The user alias, %0c, is not known to this installation of INGRES."
E_DU3003_NOT_SUPER_USER
"  You are not authorized to use the super-user flag, '-s'."
E_DU3004_NO_ALIAS_FLAG
"  You are not authorized to use the alias flag, '-u'."
E_DU3005_NOT_AUTHORIZED
"  You are not authorized to run this utility. \n
   Only users with appropriate privilege may do so."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SECURITY and/or OPERATOR privilege is required to run this
**	utility. The user running it does not have sufficient privileges.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	Have the system administrator (or other user with sufficient 
**	privileges) to either: a) use accessdb to give this user
**	more privileges; or b) run this utility for them.
*/
E_DU3006_CANT_OPEN_IIDBDB
"FINDDBS cannot open iidbdb.\n
   This may be caused by an installation definition error, or\n
   may be caused if another fast_commit server is currently using the iidbdb."
/*%
** Parameters:
**	None.
**
** Explanation:
**	finddbs failed while attempting to connect to the iidbdb.  This can
**	be indicative of a serious installation problem, but that is not
**	necessarily the case.  If someone is running a fast-commit server that
**	is accessing the iidbdb and you are using anther server, you can
**	get locked out.  Also, if someone is accessing the iidbdb with the
**	"+U" flag, you will be locked out.
**
** System Status:
**	Execution stops.
**
** Recommendation:
**	The user must determine if the installation is unhealthy or if the
**	iidbdb is locked or simply in use by a different fast-commit server.
**	The user must fix the installation problems, if any exist.  Then the
**	user should re-issue the finddbs command.
*/
E_DU3010_BAD_DBNAME
"  The database name, %0c, is syntactically incorrect for the\n
INGRES DBMS."
E_DU3011_NOT_DBA_DS
"  User %1c cannot destroy a database whose DBA is %0c."
E_DU3012_NODS_DBDB_DS
"  The IIDBDB cannot be destroyed."
E_DU3014_NO_DBNAME
"  No database name given."
E_DU3015_CANT_DESTROY_50DB
"  DESTROYDB ERROR:  You have requested 6.x DESTROYDB to destroy 5.0\n
database '%0c', owned by '%1c'.\n
Please use 5.0 DESTROYDB to destroy 5.0 dbs\n
or use -p option."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	If the 5.0 dbdb is converted to iidbdb or if "finddbs -r" is run
**	in an installation that contains both 5.0 and 6.x databases, there
**	will be entries for 5.0 databases in the iidatabase catalog.  These
**	entries will have the DU_CONVERTING bit set in iidatabase.access.
**
**	destroydb will destroy databases if it can find entries for them in
**	iidatabase and iiextend.  It removes any entries that exist from
**	the iidbdb, then destroys the directories associated with the database.
**	This means that it would be possible to destroy 5.0 databases from
**	release 6.0.  That would leave the 5.0 catalogs indicating the
**	database exists, but the directories would be missing.  Therefore,
**	destroydb refuses to destroy databases that are marked as DU_CONVERTING,
**	unless the "-p" option is selected and the user gives permission.
**
** System Status:
**	destroydb prints a warning and refuses to destroy the database. This
**	message is presented when the "-p" flag is not specified.
**
** Recommendation:
**	If this is a healthy 5.0 database, the user has two choices:
**
**	  1.  use 5.0 destroydb
**	  2.  do not destroy the database
**
**	If this is the obscure case of a conversion that failed and the
**	database is no longer accessible by 5.x, the user must use the "-p"
**	(prompt) option.
*/
E_DU3016_50_DB_FOUND
"WARNING:  Database '%0c' (owner '%1c') is a 5.0 db."
/*%
** Parameters:
**	%0c - database name
**	%1c - database owner
**
** Explanation:
**	If the 5.0 dbdb is converted to iidbdb or if "finddbs -r" is run
**	in an installation that contains both 5.0 and 6.x databases, there
**	will be entries for 5.0 databases in the iidatabase catalog.  These
**	entries will have the DU_CONVERTING bit set in iidatabase.access.
**
**	destroydb will destroy databases if it can find entries for them in
**	iidatabase and iiextend.  It removes any entries that exist from
**	the iidbdb, then destroys the directories associated with the database.
**	This means that it would be possible to destroy 5.0 databases from
**	release 6.0.  That would leave the 5.0 catalogs indicating the
**	database exists, but the directories would be missing.  Therefore,
**	destroydb refuses to destroy databases that are marked as DU_CONVERTING,
**	unless the "-p" option is selected and the user gives permission.
**
** System Status:
**	destroydb prints a warning.  It will prompt the user for permission
**	to destroy this database, and will do so if permission is given.  This
**	could have undesirable effects on the 5.0 installation if the user
**	did not really mean to destroy the database.
**
** Recommendation:
**	The user must determine whether or not this database is known to the 5.0
**	installation. If this database is known to the 5.0 installation, then
**	they should use 5.0 destroydb first, then reissue the 6.x destroydb
**	command to clean up the 6.0 catalogs.
*/
E_DU3020_DB_NOT_FOUND
"  The database, %0c, does not exist in this INGRES installation."
E_DU3021_DB_EXISTS_CR
"  The database, %0c, already exists."
E_DU3022_DB_UNIQUE_CR
"  The database name not unique in first 24 characters, %0c ."
/*%
** Parameters:
**	%0c - non-unique characters
**
** Explanation:
**	Current limitations require all database names be unique in
**	first 24 characters.
**
**
** System Status:
**	Database creation terminates.
**
** Recommendation:
**	The user must choose a name which is unique in first 24 characters.
*/
E_DU3030_NO_LOGNAME
"  There was no translation for the environment variable, %0c."
E_DU3040_NO_DEF_DBLOC
"  The default data location, %0c, for this database could\n
not be found in the IIlocations catalog of the IIDBDB."
E_DU3041_NO_JNLLOC
"  The journaling location, %0c, for this database could\n
not be found in the IIlocations catalog of the IIDBDB."
E_DU3042_NO_CKPLOC
"  The checkpointing location, %0c, for this database could\n
not be found in the IIlocations catalog of the IIDBDB."
E_DU3043_NO_SORTLOC
"  The work location, %0c, for this database could\n
not be found in the IIlocations catalog of the IIDBDB."
E_DU3044_NO_DMPLOC
"  The online dump location, %0c, for this database could\n
not be found in the IIlocations catalog of the IIDBDB."
/*%
** Parameters:
**	%0c - dump location name
**
** Explanation:
**	The online dump location did not exist in the iilocations
**	catalog.  This message is issued by convto60.
**
** System Status:
**	convto60 abnormally terminates.
**
** Recommendation:
**	Call Ingres Corporation Technical Support.
*/
E_DU3045_BAD_LOCNAME
"  Bad location name syntax, %0c."
E_DU3046_BAD_DBLOC
"  The status for location, %0c, indicates it is not configured\n
for database files."
E_DU3047_BAD_CKPLOC
"  The status for location, %0c, indicates it is not configured\n
for checkpoint files."
E_DU3048_BAD_JNLLOC
"  The status for location, %0c, indicates it is not configured\n
for journal files."
E_DU3049_BAD_SORTLOC
"  The status for location, %0c, indicates it is not configured\n
for work files."
E_DU3050_BAD_DMPLOC
"  Invalid dump location."
E_DU3051_NO_DMPLOC
"  No dump location."
E_DU3052_BAD_RDONLYLOC
"  The readonly location, %0c, does not contain an Ingres database."
E_DU3060_NO_IIEXT_ENTRIES_DS
"  There were no entries in the IIextend system catalog of the IIDBDB\n
for this database."
E_DU3064_NO_DBCAT_ENTRY_DS
"  There was no entry in the database system catalog of the IIDBDB for\n
this database."
E_DU3070_BAD_COLLATION
"  The collation %0c is invalid"
/*%
** Parameters:
**	%0c - invalid collation name
**
** Explanation:
**	The specified collation name was bad or did not exist.
**
** System Status:
**	createdb does not continue.
**
** Recommendation:
**	Check installed collation sequences.
*/
E_DU3080_DELETE_DIR_DS
"  The database directory, %0c,\n
could not be deleted."
E_DU3081_NO_ABFLOG_DS
"  There is no environment variable, ING_ABFDIR, for this installation.\n
ABF applications for this database cannot be destroyed."
E_DU3100_UCODE_OLDUSAGE
"  Old usage, '-u:', user codes are no longer recognized by the INGRES DBMS."
E_DU3101_UNKNOWN_FLAG_DS
"  The command line flag, %0c, is not known by the database utility,\n
DESTROYDB."
E_DU3102_NO_CONFIRM_DS
"  Destruction of the database, %0c, has been aborted."
E_DU3121_UNKNOWN_FLAG_CR
"  The flag '%0c' is unknown to the database utility, CREATEDB."
E_DU3122_CONFLICTING_FLAGS
"  The flag '%0c' cannot be used with the flag '%1c'."
E_DU3129_DBDB_NODMPLOC_CR
"  No dump location created."
E_DU3130_NO_CREATEDB_CR
"  You do not have 'CREATEDB' privilege."
E_DU3131_MISSIING_DBDB
"  You must create the database database (iidbdb) before you may\n
   create any user databases.  Type:\n
            createdb -S iidbdb"
E_DU3132_BAD_DBDBNAME_CR
"  The IIDBDB flag, '-S', cannot be used with  database name, %0c."
E_DU3133_NO_DBDB_FLAG_CR
"  The IIDBDB flag, '-S', must be used when creating the IIDBDB."
E_DU3134_DBDB_NODBLOC_CR
"  You cannot specify a default data location for the IIDBDB."
E_DU3135_DBDB_NOJNLLOC_CR
"  You cannot specify a journaling location for the IIDBDB."
E_DU3136_DBDB_NOCKPLOC_CR
"  You cannot specify a checkpoint location for the IIDBDB."
E_DU3137_DBDB_PRIVATE_CR
"  The IIDBDB cannot be made as a private database."
E_DU3138_DBDB_NOSORTLOC_CR
"  You cannot specify a default work location for the IIDBDB."
E_DU3139_DBDB_NORDONLYLOC_CR
"  You cannot specify a readonly database location for the IIDBDB."
E_DU3140_DBDB_NOT_SUPERUSER_CR
"  Only a superuser can create the IIDBDB."
E_DU314A_CONFIG_TOO_BIG_CR
"  The configuration file was too large to be built."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A database utility, such as createdb or convto60, could not create
**	the specified file.  If failure is due to a lack of disk space or
**	permissions, a CL error message should be in the error log.  Another
**	possibility would be if there are extraneous files or multiple versions
**	of valid files in the database template directory.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Consult your system administrator to ensure that the 
**	installation is set up correctly.
**	    
*/
E_DU3150_BAD_FILE_CREATE
"  The file, %0c, couldn't be created.\n
   If running CREATEDB, ensure that no extraneous files exist in\n
      ii_system!ingres!dbtmplt."
E_DU3151_BAD_FILE_OPEN
"  The file, %0c, couldn't be opened."
E_DU3152_BAD_SENSE
"  The header information for the file, %0c,\n
couldn't be read."
E_DU3153_BAD_READ
"  Bad read of the file, %0c."
E_DU3154_BAD_ALLOC
"  Couldn't allocate a page for the file,\n
%0c."
E_DU3155_BAD_WRITE
"  Bad write to the file, %0c."
E_DU3156_BAD_FLUSH
"  Couldn't flush the header for the file,\n
%0c."
E_DU3157_BAD_CLOSE
"  Couldn't close the file, %0c."
E_DU3158_BAD_PAGE_NUM
"  Unexpected page number (%0d)."
E_DU3170_BAD_USRF_OPEN
"  CL error, %0x, opening the 'users' file."
E_DU3171_USRF_NOT_FOUND
"  User, %0c, was not found in the 'users' file.\n
The account you are currently logged into is not a known Ingres\n
account and you cannot create the iidbdb from here. You must\n
create the iidbdb from the same account where you installed Ingres\n
(account where you ran iibuild). Before running CREATEDB again from\n
the correct account, you must first remove the partially created\n
iidbdb directory and then restart your server. See your Site\n
Technical Contact for assistance."
E_DU3172_BAD_USRF_RECORD
"  Bad users record found in 'users' file."
E_DU3180_BAD_TEMPLATE_COPY
"  A CL error, %0x, occurred in copying one of the template files."
E_DU3190_BAD_LKINIT
"  Couldn't initialize the locking system for this process.\n
\tStatus returned was, %0x.\n
\tPlease insure that the process is properly installed and that\n
\t\tyou have sufficient privilege and system resources to invoke it."
/*%
** Parameters:
**	%0x - value returned by call to LKinitialize()
**
** Explanation:
**	destroydb could not connect to the locking system. Connection to 
**	the locking system may be rejected if destroydb is not installed 
**	(or run) with the proper privilege, or the lack of a critical 
**	system resource prevents destroydb from successful operation.  
**	For example, the former condition will occur on VMS if destroydb 
**	isn't installed with CMKRNL and on Unix if destroydb isn't setuid 
**	to the installation owner.  The latter condition can occur on Unix 
**	if the swap space available is insufficient to allow destroydb 
**	to map the shared memory segment used for locking.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Consult your system administrator to ensure the installation
**	and system are configured correctly.
**	    
*/
E_DU3191_BAD_LK_REQUEST
"  Bad lock request."
E_DU3192_BAD_LKLIST_RELEASE
"  Couldn't release active lock list."
E_DU3193_BAD_LKLIST_CREATE
"  Couldn't create the lock list."
E_DU3195_DB_LKBUSY
"  The database, %0c, is currently unavailable."
E_DU31A0_BAD_DIRFILE_DELETE
"  A CL error, %0x, occurred while deleting files of the directory,\n
%2c, in the path, %1c."
E_DU31A1_BAD_DIR_DELETE
"  The directory, %1c, in the path, %0c,\n
couldn't be deleted."
E_DU31A2_BAD_FILE_DELETE
"  The file, %1c, in the directory, %0c,\n
couldn't be deleted."
E_DU31B0_BAD_DIR_CREATE
"  The directory, %1c, in the path, %0c,\n
couldn't be created."
E_DU31C0_BAD_LGOPEN
"  This utility could not open the log file."
E_DU31C1_BAD_LGALTER
"  This utility could not alter information in the log file."
E_DU31C2_BAD_LGADD
"  This database could not be added to the log file."
E_DU31C3_BAD_LGBEGIN
"  This utility could not start a transaction in the logging system."
E_DU31C4_BAD_LGEVENT
"  This utility was unable to wait for a logging event to occur."
E_DU31C5_BAD_LGCLOSE
"  This utility could not close the log file."
E_DU31C6_BAD_LGREMOVE
"  This utility was unable to remove the database from the log file."
E_DU31C7_BAD_LGEND
"  This utility was unable to complete the transaction it had started."
E_DU31C8_NO_ARCHIVER
"  For a journaled database, the archiver has not finished\n 
journaling the data from the log to the journals.  Check to make sure archiver\n
is still running."
E_DU31D0_DBID_MAXTRYS
"  A unique database id could not be determined."
E_DU3200_BAD_APPEND
"  Bad append to table, %0c,\n
in database, %1c."
E_DU3201_BAD_REPLACE
"  Bad replace to table, %0c,\n
in database, %1c."
E_DU3202_BAD_DELETE
"  Bad delete from table, %0c,\n
in database, %1c."
E_DU3203_BAD_REGISTER
"  Error occurred while registering table, %0c."
E_DU3204_BAD_CREATE
"  Error occurred while creating table, %0c."
E_DU3205_BAD_VIEW
"  Error occurred creating view, %0c."
E_DU3300_UNKNOWN_FLAG_SY
"  The flag '%0c' is unknown to the database utility, SYSMOD."
E_DU3303_BAD_PAGESIZE
"  The page size '%0d' is invalid or the dbms is not configured for this page size."
E_DU3304_BAD_SYSTEM_CATALOG
"  There is no back-end or standard catalog named '%0c'."
E_DU3310_NOT_DBA_SY
"  You, %1c, cannot SYSMOD a database whose DBA is, %0c."
E_DU3312_DBDB_ONLY_CAT_SY
"  The system catalog '%0c' can be sysmod only with the '%1c' database."
E_DU3314_NOSUCH_CAT_SY
"  The system catalog '%0c' was listed as a parameter on the SYSMOD\n
command line.  However, it doesn't exist in the database '%1c'."
E_DU3400_BAD_MEM_ALLOC
"  Unable to allocate dynamic memory."
E_DU3500_TOO_MANY_ARGS_FI
"  Too many parameters were given on the command line for FINDDBS."
E_DU3501_BAD_FLAG_FI
"  The flag '%0c' is unknown to the database utility, FINDDBS."
E_DU3510_SAME_DBNAME_FI
"  If both db's are intended for this installation, one of them must\n
be renamed."
E_DU3511_NOCR_FE_CATS
"  CREATEDB could not create and modify the front-end catalogs. An error\n
occurred when attempting to call the utility program '%0c' to create\n
the front-end system catalogs."
/*%
**
** Explanation:
**	    CREATEDB just tried to call "upgradefe" to create the front-end system
**	    catalogs. Upgradefe was not able to successfully create the
**	    front-end catalogs.
**
**
** System Status:
**	    The new database will not be usable.
**
**
** Recommendation:
**	    Check for problems in your environment and DBMS installation
**	    (disk space, subprocess and other quotas, is the installation
**	    complete with executables present in the "bin" subdirectory, etc.).
**	    VMS users can run the "checkenv" utility.
*/
E_DU3512_BAD_CLIENT_NM
"  Conflicting dictionary client names.\n
The list of dictionary client names on the CREATEDB command line\n
contains conflicts. The client name '%0c' appears with\n
another dictionary client name(s);\n
'%0c' cannot be mixed with other dictionary client names,\n
because it tells CREATEDB to not create any front-end catalogs in the\n
new database. Correct the command line and rerun CREATEDB."
/*%
**
** Explanation:
**	The special client name 'nofeclients' appeared on the
**	command line along with some other client name(s). When
**	'nofeclients' is specified, no other dictionary client
**	names can appear. The presence of the dictionary client
**	name 'nofeclients' tells CREATEDB to not create
**	any front-end catalogs in the new database.
**
**
** System Status:
**	    CREATEDB exits and the database is not created.
**
**
** Recommendation:
**	Check the dictionary client names you specified on the CREATEDB
**	command line. Dictionary client names should follow the database name
**	at the end of the command line.
*/
E_DU3513_NO_WRITE_ACCESS
"  Cannot create temporary files.\n
%0c cannot create files in either your current working directory, or\n
(optionally) in the directory pointed to by the environment variable\n
(logical name on VMS) II_TEMPORARY.  %0c needs to create temporary\n
files and cannot run until you have write access to either your\n
working directory or to II_TEMPORARY. Check your access rights and\n
available disk space for these directories. Contact your Site Technical\n
Contact for assistance."
/*%
**
** Explanation:
**	Createdb or Sysmod just checked for write access for creating
**	temporary files, and found it cannot create files. It exited
**	after performing no work.
**
**
** System Status:
**	Createdb or Sysmod will exit. The database is not affected
**	(it is as if Createdb or Sysmod were never run).
**
**
** Recommendation:
**	Check why files cannot be created in your current working directory
**	or (optionally) in II_TEMPORARY. When you can create files, run
**	Createdb or Sysmod again.
*/
E_DU3514_EMPTY_CLIENT_LIST
"  Bad command line flag: empty -f client list.\n
The %0c command contains a -f flag, but there are no front-end\n
dictionary client names present after the -f (for example  -f INGRES).\n
This is an error on the %0c command line. Please rerun this\n
command and either follow the -f flag with a client name(s),\n
or do not specify the -f flag."
/*%
**
** Explanation:
**	Createdb or Sysmod just found a -f flag on the command line
**	with no client name following the -f.
**
**
** System Status:
**	Createdb or Sysmod will exit. The database is not affected
**	(it is as if Createdb or Sysmod were never run).
**
**
** Recommendation:
**	Specify a front-end dictionary client name(s) following the -f flag, 
**	or else do not specify the -f flag.
*/
E_DU3515_CLIENT_NM_CONFLICT
"The list of dictionary client names on the command line contains conflicts.\n
The client name 'NOFECLIENTS' appears with another dictionary client name(s);\n
'NOFECLIENTS' cannot be mixed with other dictionary client names."
/*%
** Parameters:
**	None.
**
** Explanation:
**	"nofeclients" is a special client name indicating that all FE catalog
**	operations are to be bypassed.  It is illegal to use this in conjunction
**	with other client names.
**
** System Status:
**	Processing is aborted.
**
** Recommendation:
**	Check the dictionary client names you specified on the command line.
**	Dictionary client names should follow the database name at the end of
**	the command line.
*/
E_DU3516_DBDB_NOSECURITY
"  SECURITY privilege is required to create the database database (iidbdb)."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Creating the database database (iidbdb) is a privileged operation.
**	Only users with SECURITY privilege can do this.
**
** System Status:
**	Processing is aborted.
**
** Recommendation:
**	Make sure the user trying to create the iidbdb has SECURITY
**	privilege, then try again.
*/
E_DU3519_LEGAL_REG_DELIM_ID
"  Legal combinations are:\n
\n
        reg_id_case   delim_id_case\n
        -----------   -------------\n
        lower         lower            INGRES defaults\n
        lower         mixed\n
        upper         upper\n
        upper         mixed            FIPS\n
"
/*%
** Parameters:
**	None.
**
** Explanation:
**	If conflicting values were picked up from the command line
**	and/or installation defaults, it's helpful to explain which
**	value came from where.  Since the user can see the command
**	line, don't bother explaining that.  Instead, createdb displays
**	this message to point out values that were picked up from the
**	installation defaults.
**
** System Status:
**	createdb aborts; the database is not created.
**
** Recommendation:
**	Check the createdb command-line switches and/or configuration settings.
*/
E_DU351A_MULTI_DB_DESTRY
"You cannot specify more than one database to destroy at one time."
/*%
** Parameters:
**      none
**
** Explanation:
**      Destroydb can only destroy one database at a time.
**
** System Status:
**      destroydb  exits and no databases are destroyed.
**
** Recommendation:
**      Specify only one database on the command line.
*/
E_DU351B_INCOMPATIBLE_VERSION
"Createdb is running in an installation with version %0c,\n
you have connected to a DBMS running in an installation with version\n
%1c, these versions are incompatible for createdb.\n
Please log onto the remote machine to run createdb or use an\n
installation that is compatible."
/*%
** Parameters:
**	Frontend (createdb) Ingres version
**	DBMS Ingres version
**
** Explanation:
**	For architectural reasons createdb cannot run between incompatible
**	versions of ingres, for most purposes the major release ID must be
**	the same.
**
** System Status:
**	Createdb exits, no database is created
**
** Recommendation:
**	Run createdb on the remote node, or on a compatible version of Ingres.
*/
E_DU351C_INCOMPATIBLE_VERSION
"%0c is running in an installation with version %1c,\n
you have connected to a DBMS running in an installation with version\n
%2c, these versions are incompatible.\n
Please log onto the remote machine to run this utility or use an\n
installation that is compatible."
E_DU3600_INTERRUPT
"  Interrupt  --  aborting."
E_DU3601_ACCESS_VIOLATION
"  Access Violation --  aborting."
E_DU4000_BAD_STATF_READ
"  The contents of the CONVTO60 status file could not be read."
E_DU4010_RENAME_FILE_EXISTS
"  The source file, %0c, in the path,\n
%1c, could not be renamed to the target file,\n
%2c.  The target file already exists."
E_DU4012_MAXAREA
"  The area, '%0c'\n
has a length greater than is allowed in 6.0."
E_DU4020_DATABASE_OPEN
"  The database (%1c) was open when it should not have been.  This error
occurred in procedure (%0c) while attempting to reload QRYMOD definitions
into the 6.0 database (%1c)."
E_DU4021_CANT_OPEN_QRYMOD_FILE
"  CONVTO60 could not open the file (%2) which should contain all the 
permit, view, and integrity information to be reloaded into the database.
This error occurred in procedure (%0c) called by (%1c) while attempting 
to reload QRYMOD definitions into the 6.0 database (%3c)."
E_DU4022_BAD_QRYMOD_FILE_READ
"  CONVTO60 could not read some portion of the file (%2) which should 
contain all the permit, view, and integrity information to be reloaded 
into the database.
This error occurred in procedure (%0c) called by (%1c) while attempting 
to reload QRYMOD definitions into the 6.0 database (%3c)."
E_DU4023_WRONG_DATABASE
"  CONVTO60 encountered the wrong database name (%2c) in the QRYMOD
reload file.  The correct database name should have been (%1c).
This error occurred in procedure (%0c) while attempting 
to reload QRYMOD definitions into the 6.0 database (%1c)."
E_DU4024_BAD_CHANGE_USER_FLAG
"  CONVTO60 encountered a badly formatted INGRES command in the 
QRYMOD reload file.  The scanned '-u' argument is (%1c) but should have
been (-uusername).
This error occurred in procedure (%0c) while attempting 
to reload QRYMOD definitions into the 6.0 database (%2c)."
E_DU5000_INVALID_INPUT_NAME
"INVALID NAME IN CMD INPUT LINE: %0c"
/*%
** Parameters:
**	%0c - input parameter (null-terminated character string)
**
** Explanation:
**	The database name or table name specified had a syntax error.
**	The correct syntax for database names is:
**	    "name1 name2 ... name10" -- where 1 to 10 names may be specified
**	The correct syntax for specifying a table name is "tablename"
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU5001_INVALID_MODE_FLAG
"INVALID MODE FLAG: %0c"
/*%
** Parameters:
**	%0c - input parameter (null-terminated character string)
**
** Explanation:
**	An invalid value was specified with the "-m" flag to set the
**	mode for verifydb.  Valid values are: "-mrun", "-mrunsilent",
**	"-mruninteractive", "-mreport".
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU5002_INVALID_SCOPE_FLAG
"INVALID SCOPE FLAG: %0c"
/*%
** Parameters:
**	%0c - input parameter (null-terminated character string)
**
** Explanation:
**	An invalid value was specified with the "-s" flag to set the
**	scope for verifydb. Valid values are: "-sdbname 'name [name]'",
**	"-sdba" or "-sinstallation".
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU5003_INVALID_OPERATION_FLAG
"INVALID OPERATION FLAG: %0c"
/*%
** Parameters:
**	%0c - input parameter (null-terminated character string)
**
** Explanation:
**	An invalid value was specified with the "-o" flag to set the
**	operation for verifydb.  Valid values are: "-odbms_catalogs,"
**	"-oforce_consistent", "-opurge", "-otemp_purge", "-oexpired_purge",
**	"-otable name".
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU5004_TOO_MANY_NAMES_IN_LIST
"Too many names in list.  Max is 10."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A maximum of 10 names may be specified with the "-sdbname" flag.
**	The correct syntax is "name1 [name2 name3 ... name10]". The
**	list starts and ends with a quote.  There is exactly one space
**	separating each name in the list.  Null lists are not valid.
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU5005_SPECIFY_ONLY_1_NAME
"Only 1 table name may be specified with -otable."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A maximum of 1 name may be specified with the "-otable" flag.
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU5006_UNKNOWN_PARAMETER
"invalid parameter is VERIFYDB input line"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A flag that verifydb does not recognize was input. The only valid
**	input flags are:
**	    -m or -mode
**	    -s or -scope 
**	    -o or -operation
**	    -u or -user (optional)
**
** System Status:
**	The invalid flag is ignored, and this warning message is printed.
**
** Recommendation:
**	None.
*/
E_DU5007_SPECIFY_ALL_FLAGS
"VERIFYDB must be evoked with -m, -s and -o flags:\n\n
    VERIFYDB -ModeXXX -ScopeYYY -OperationZZZ <-uUSERNAME> <-nolog>\n\n
where:\n\n
XXX = REPORT, RUN, RUNSILENT, RUNINTERACTIVE\n
YYY = DBNAME (followed by list of up to 10 names in double quotes),\n
      DBA, INSTALLATION\n
ZZZ = DBMS_CATALOGS, FORCE_CONSISTENT, DROP_TABLE, PURGE, TEMP_PURGE,\n
      EXPIRED_PURGE, TABLE, XTABLE, ACCESSCHECK\n
 <>   denotes optional flag."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb must be evoked with the "-m", "-s", and "-o" flags.
**	Defaults are not permitted.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	Re-enter the verifydb command with the correct parameters.
*/
E_DU5008_SPECIFY_TABLE_NAME
"You must specify a table along with the -otable flag:\n
The format is -otable 'table_name'"
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the table operation is selected, verifydb must be informed of
**	the name of the table to operate on.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	Re-enter the verifydb command with the correct parameters.
*/
E_DU5009_SPECIFY_DB_NAME
"You must specify at least one database along with the -sdbname flag:\n
The format is -sdbname database_name 'table1 [table2 ... table10]'"
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the dbname scope is selected, verifydb must be informed of
**	the name(s) of the database(s) to operate on.  The user may specify
**	from 1 to MAX_NAMES (in duvdb.h) databases.  If more are specified,
**	all database names in the list that exceed the limit are ignored.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	Re-enter the verifydb command with the correct parameters.
*/
E_DU5010_NO_SYSCAT_FILE
"Fatal error.  The disk file for system catalog %0c\n
is not found.\n
VERIFYDB will proceed as far as it can before crashing."
/*%
** Parameters:
**	%0c - system catalog name (from iirelation.relid)
**
** Explanation:
**	A disk file for one of the system catalogs cannot be found.  This
**	means that verifydb will not be able to perform all of its checks
**	on system catalogs.
**
** System Status:
**	verifydb will keep running as long as it can -- the length of time
**	depends on which system catalog is missing.  When it cannot continue
**	any longer, it will take an exception exit.
**
** Recommendation:
**	Try to get a backup of the missing system catalog file.
*/
E_DU501A_CANT_CONNECT
"Unable to connect with database %0c."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	Unable to connect to the specified database (specified on the command
**	line with the "-t" flag).
**
** System Status:
**	verifydb could not connect to the database specified by the user.
**
** Recommendation:
**	Re-run the verifydb utility, but specify a valid database name.
*/
E_DU501B_CANT_CONNECT_AS_USER
"Unable to connect with database %0c\n
as user %1c."
/*%
** Parameters:
**	%0c - database name
**	%1c - user name
**
** Explanation:
**	verifydb is unable to connect to the specified database (specified
**	on the command line with the "-t" flag) as the specified user
**	(specified on the command line with the "-u" flag).
**
** System Status:
**	verifydb could not connect to the database specified by the user.
**
** Recommendation:
**	Re-run the verifydb utility, but specify a valid database name and/or
**	a valid user.
*/
E_DU501C_EMPTY_IIRELATON
"FATAL ERROR:  the iirelation system catalog is empty.\n
If database is checkpointed, use ROLLFORWARDDB backup to recover."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Table iirelation is a core DBMS catalog. Without it, it is not possible
**	to access the data in the database.  This is a very severe error.
**
** System Status:
**	verifydb will stop processing, as it is not possible to act on this
**	database.
**
** Recommendation:
**	The user should attempt to recover the contents of the iirelation file
**	using the rollforwarddb utility (or the contents of a backup tape for
**	file aaaaaaab.t01 for location ii_database, if the database was not 
**	checkpointed).
*/
E_DU501D_SCOPE_NOT_IMPLEMENTED
"Sorry, this scope function is not implemented yet.\n
The only scope feature implemented is:\n
         -sdbname 'table_name' "
/*%
** Parameters:
**	None.
**
** Explanation:
**	Not all of the verifydb features are implemented yet.  The only
**	scope feature implemented at this time is dbname.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should reissue the verifydb command using only -sdbname scope.
*/
E_DU501E_OP_NOT_IMPLEMENTED
"Sorry, this operation is not implemented yet."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Not all of the verifydb features are implemented yet.  The operation
**	the user selected is not yet implemented.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should reissue the verifydb command using an implemented 
**	operation.
*/
E_DU501F_CATALOG_CHECK_ERR
"VERIFYDB:  Fatal error encountered while attempting\n
to check/patch DBMS catalogs for database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	verifydb was not able to check/patch the system catalogs completely.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should attempt to use rollforwarddb from the checkpoint to
**	recover the database.
*/
E_DU5020_DB_PATCH_ERR
"VERIFYDB:  fatal error encountered while attempting to patch\n
database %0c consistent.\n
VERIFYDB failed, and database is still inconsistent."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	verifydb was not able to patch the database consistent.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should attempt to use rollforwarddb from the checkpoint to
**	recover the database.
*/
E_DU5021_EMPTY_IITREE
"FATAL ERROR:  the iitree system catalog is empty.\n
If database is checkpointed, use ROLLFORWARDDB backup to recover."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Table iitree is a core DBMS catalog that describes view definitions for
**	the standard catalogs.  If it is empty, then the standard catalogs
**	are corrupted.  This is a very severe error.
**
** System Status:
**	verifydb will stop processing, as it is not reasonable to act on this
**	database.
**
** Recommendation:
**	The user should attempt to recover the contents of the iitree file
**	using the rollforwarddb utility.
*/
E_DU5022_EMPTY_IIDBDEPENDS
"FATAL ERROR:  the iidbdepends system catalog is empty.\n
If database is checkpointed, use ROLLFORWARDDB backup to recover."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Table iidbdepends is a QP DBMS catalog that describes view definitions
**	for the standard catalogs.  If it is empty, then the standard catalogs
**	are corrupted.  This is a very severe error.
**
** System Status:
**	verifydb will stop processing, as it is not reasonable to act on this
**	database.
**
** Recommendation:
**	The user should attempt to recover the contents of the iidbdepends file
**	using the rollforwarddb utility.
*/
E_DU5023_CANT_OPEN_LOG_FILE
"Unable to open VERIFYDB log file:\n 
%0c\n
Check file permissions, or use the -n flag to run without the log file.\n"
/*%
** Parameters:
**	%0c - the full path to the log file.
**
** Explanation:
**	verifydb was not able to open the verifydb log file in the II_CONFIG 
**	directory. This was either because verifydb detected that the path to 
**	the log file does not exist, or the user does not have write permssions 
**	to the log file. 
**
** System Status:
**	verifydb considers it a warning if it is unable to open the log
**	file in the II_CONFIG directory.  In such a case, verifydb will print 
**	this warning message and continue logging to the screen.
**	verifydb will not print this error if the "-n" flag is used.
**
** Recommendation:
**	Check the file path is correct and that you have write permissions to
**	the directory and or the file. 
**
**	You can bypass opening the log file in II_CONFIG by using the "-n" flag. 
**	Depending on the operating system, you could then redirect the screen
**	output to a file of your choice.
*/
E_DU5024_TBL_DROP_ERR
"Unable to destroy table\n
%0c from database %1c."
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	verifydb was not able to delete the table from the system catalogs.  
**
** System Status:
**	The verifydb process terminates.
**
** Recommendation:
**	The best thing to do is to verify that the table really exists,
**	because nonexistent tables are usually why verifydb can't drop the
**	table.
*/
E_DU5025_NO_SUCH_TABLE
"Table %0c (owner %1c)\n
does not exist."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	The table does not exist in the specified database.
**
** System Status:
**	The verifydb process terminates.
**
** Recommendation:
**	Perhaps the table is owned by another user.  Determine who the
**	owner is and run verifydb again, this time using the "-u" flag.
*/
E_DU5026_EMPTY_IIUSER
"FATAL ERROR:  the iiuser database database catalog is empty.\n
If database is checkpointed, use ROLLFORWARDDB backup to recover.\n
Or, use accessdb to define users for this installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Table iiuser defines the users for an installation.  If the table is
**	empty, then no one is authorized to run on this installation.
**	This is a severe error.
**
**	The iiuser table should be recovered from backup, if possible.
**
** System Status:
**	verifydb will stop processing, as it is not reasonable to act on this
**	database.
**
** Recommendation:
**	The user should attempt to recover the contents of the iiuser file.
*/
E_DU5027_CMPTLVL_MISMATCH
"Unable to run VERIFYDB on database %0c\n
due to Major Compatibility Level mismatch.\n
Verifydb is at level=%1d, and the database is at level=%2d"
/*%
** Parameters:
**	%0c - database name
**	%1d - major compatibility level of verifydb
**	%2d - major compatibility level of database
**
** Explanation:
**	You are trying to run the wrong version of verifydb for this database.
**	verifydb's minor and major compatibility levels must be greater than or
**	equal to the database's level.
**
** System Status:
**	verifydb will stop processing for this database, as it is not
**	reasonable to act on this database.
**
** Recommendation:
**	The user should get the correct version of verifydb.  If that is
**	not available, call Ingres Corporation Technical Support.
**
**	NOTE TO TECH SUPPORT: Do not manually change levels in iidatabase
**	catalog -- if you run an old version of verifydb and the catalogs have
**	changed since that version, you will probably destroy the database
**	beyond recovery.  Get the right version -- there is no workaround.
*/
E_DU5028_CMPTMINOR_MISMATCH
"Unable to run VERIFYDB on database %0c\n
due to Minor Compatibility Level mismatch.\n
Verifydb is at level=%1d, and the database is at level=%2d"
/*%
** Parameters:
**	%0c - database name
**	%1d - minor compatibility level of verifydb
**	%2d - minor compatibility level of database
**
** Explanation:
**	You are trying to run the wrong version of verifydb for this database.
**	verifydb's minor and major compatibility levels must be greater than or
**	equal to the database's level.
**
** System Status:
**	verifydb will stop processing for this database, as it is not
**	reasonable to act on this database.
**
** Recommendation:
**	The user should get the correct version of verifydb.  If that is
**	not available, call Ingres Corporation Technical Support.
**
**	NOTE TO TECH SUPPORT: Do not manually change levels in iidatabase
**	catalog -- if you run an old version of verifydb and the catalogs have
**	changed since that version, you will probably destroy the database
**	beyond recovery.  Get the right version -- there is no workaround.
*/
E_DU5029_EMPTY_IIDATABASE
"FATAL ERROR:  the iidatabase catalog is empty.\n
If database is checkpointed, use ROLLFORWARDDB backup to recover.\n
Or, use accessdb to define users for this installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Table iidatabase defines the databases for an installation.  If the
**	table is empty, there are no databases in the installation, including
**	the one we are currently examining. This is a severe error.
**
** System Status:
**	verifydb will stop processing, as it is not reasonable to act on this
**	database.
**
** Recommendation:
**	The user should attempt to recover the contents of the iidatabase file.
*/
E_DU502A_PURGE_ERR
"VERIFYDB:  Fatal error encountered while attempting\n
to purge database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	verifydb was not able to purge temporary or expired relations in
**	this database.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should attempt to rerun verifydb.
*/
E_DU502B_CANT_MAKE_PURGELIST
"VERIFYDB:  Fatal error encountered while attempting\n
to build internal purgelist cache."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb was not able to purge temporary or expired relations in
**	the database.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	This is probably caused by lack of dynamic memory.  The user should
**	increase dynamic memory for this process, then attempt to re-run.
*/
E_DU502C_UNABLE_TO_DELETE_FILE
"VERIFYDB:  FATAL ERROR.\n
Unable to delete temporary file\n
'%0c' from directory '%1c'\n
(database %2c,\n
ingres location '%3c')"
/*%
** Parameters:
**	%0c - filename
**	%1c - pathname to directory containing file
**	%2c - database name
**	%3c - location name
**
** Explanation:
**	verifydb was attempting to purge a temporary file.  DIdelete() failed
**	to delete this file.
**
** System Status:
**	This is a fatal error -- verifydb processing stops.
**
** Recommendation:
**	The user should retry verifydb.  If it still fails, the user should
**	start looking at why -- is this a read only directory, is verifydb
**	installed, does it have the correct privileges, etc.
*/
E_DU502D_NOFREE_PURGETABLE
"VERIFYDB:  FATAL ERROR.\n
Unable to create temp table in name-range of\n
'%0c to '%1c'"
/*%
** Parameters:
**	%0c - lowest temp table name
**	%1c - max temp table name
**
** Explanation:
**	verifydb must create a temporary table (owned by $ingres).  The name
**	of that table must fall between the lower and upper table names --
**	verifydb uses a lookup table of possible names.  In this case, the
**	database already had tables owned by $ingres with all of the names that
**	verifydb has in its lookup table.
**
** System Status:
**	This is a fatal error -- verifydb processing stops.
**
** Recommendation:
**	The user must free up one of these table names.  If these are temp
**	tables, then delete them.  If not, copy one table to something else,
**	drop that table, run verifydb, recreate the table, and copy the data
**	back.
*/
E_DU502E_TABLE_OP_ERROR
"ERROR encountered while attempting TABLE or XTABLE operation on table\n
%0c in database %1c"
/*%
** Parameters:
**	%0c - table name
**	%1c - database name
**
** Explanation:
**	An error was encountered while attempting the table or xtable
**	operation.  That error is reported in the verifydb log.
**
** System Status:
**	The verifydb process stops; the transaction is aborted. But any
**	changes to the disk file may or may not be flushed to disk, depending
**	on DMF's state when the error was encountered.
**
** Recommendation:
**	The user should refer to the verifydb log file to see what went wrong.
**	The user may be required to take some corrective action, based on the
**	nature of the error.  Certain errors are forever fatal -- such as
**	DMF not being able to open the file.  Other errors may be transient,
**	and repeating the verifydb table operation command will often
**	succeed.
*/
E_DU502F_CANT_PATCH_SCONDUR
"It is illegal to patch a DBMS Core Catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	S_CONCUR (or DBMS core catalogs) must be a hash.  DMF assumes the
**	format of the file.  It is illegal to modify the file to any other
**	storage format.  Since patch always modifies a table to heap, it
**	is not reasonable to patch a core catalog.
**
** System Status:
**	The verifydb process aborts its transaction and terminates.
**
** Recommendation:
**	The user should use normal methods to get healthy core catalogs; 
**	i.e., unloaddb and reload or possibly recover from a checkpoint.
*/
E_DU5030_CANT_PATCH_INDEX
"It is illegal to patch a secondary index."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The patch operation may cause individual tuples or groups of tuples
**	to be dropped.  It is not reasonable to drop some tuples from a
**	secondary index, because corresponding tuples will not be dropped
**	from the base table.  Then we will hit a condition where the base
**	table and secondary index are out of sync with each other.
**
** System Status:
**	The verifydb process aborts its transaction and terminates.
**
** Recommendation:
**	The user should drop the secondary index and recreate it.
*/
E_DU5031_TABLE_DOESNT_EXIST
"Table %0c (owner %1c) does not exist.\n
It is not possible to check or patch a nonexistent table."
/*%
** Parameters:
**	%0c - table name
**	%1c - table owner
**
** Explanation:
**	The target for check table or patch table does not exist.
**
** System Status:
**	The verifydb process aborts its transaction and terminates.
**
** Recommendation:
**	The user should determine the table name and user that owns that
**	table.  If the owner is not the user running verifydb, use the "-u"
**	flag to specify the table's owner.
*/
E_DU5032_NO_AUTHORITY
"You, %0c, do not have authority to operate on database %1c (dba %2c)"
/*%
** Parameters:
**	%0c - user name
**	%1c - database name
**	%2c - DBA
**
** Explanation:
**	The user running verifydb does not have superuser privilege
**	and is not the DBA of the target database. Therefore, access to this
**	database is denied.
**
** System Status:
**	verifydb does not grant this user access to this database. If there
**	are other databases in the scope, verifydb will go on to attempt to
**	process the next database.
**
** Recommendation:
**	The user should either get the DBA or a superuser to run the
**	command for them, or they should get the system administrator to make
**	them a superuser.
*/
E_DU5033_REPORTMODE_INVALID
"The selected operation may not be used in conjunction with the REPORT mode.\n
Please try again with one of the run modes: RUN, RUNINTERACTIVE, RUNSILENT"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Certain verifydb operations do not have a REPORT mode.
**
** System Status:
**	The verifydb process reports this error and terminates.
**
** Recommendation:
**	The user should reissue the command using one of the RUN modes.
*/
E_DU5034_EXPIRED_SYNTAX_ERROR
"VERIFYDB received a server syntax error when attempting to purge this table.\n
This is probably caused because the table was created using QUEL and has a name
\nthat is a reserved word in QUEL.  Please use QUEL TM to DESTROY this table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This syntax error occurred during PURGE_EXPIRED processing.  It is
**	probably caused because verifydb encountered an expired table that
**	was created in QUEL.  That table name is legal in QUEL, but happens
**	to be a reserved word in ESQL.  So the DBMS server gave verifydb an
**	error when verifydb attempted to drop this table.
**
** System Status:
**	verifydb reports this error and continues the purge operation as though
**	no error had occurred.  However, the table that this error occurred
**	on is not purged.
**
** Recommendation:
**	The user should manually destroy this expired table from the database
**	using QUEL TM.
*/
E_DU5035_REPORTMODE_ONLY
"VERIFYDB operation ACCESSCHECK may only be used in the REPORT mode."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified one of the RUN modes in conjunction with the
**	ACCESSCHECK operation.
**
** System Status:
**	The verifydb process reports this error and aborts.
**
** Recommendation:
**	The user should reissue the command using REPORT mode.
*/
E_DU5036_NODBS_IN_SCOPE
"FATAL ERROR:  There are no Databases in VERIFYDB scope.\n
This is probably caused by an invalid user name with -U parameter.\n
It may also be cause by specifying a valid user who does not own any DBs."
/*%
** Parameters:
**	None.
**
** Explanation:
**	verifydb tried to translate the scope to a list of databases. It could
**	not find any databases in that scope list.
**
** System Status:
**	The verifydb process reports this error and terminates.
**
** Recommendation:
**	The user should reissue the command using the correct scope and user
**	specifications.
*/
E_DU5037_INVALID_ACCESS_USR
"FATAL ERROR:  The specified access user (%0c)\n
for the ACCESSCHECK operation is NOT a valid INGRES user."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	verifydb tried to translate the scope to a list of databases.  It could
**	not find any databases in that scope list.
**
** System Status:
**	The verifydb process reports this error and terminates.
**
** Recommendation:
**	The user should reissue the command using the correct scope and user
**	specifications.
*/
E_DU5038_CANT_PATCH_VIEW
"It is illegal to patch a view."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This operation is intended for tables, not views.
**
** System Status:
**	The verifydb process aborts its transaction and terminates.
**
** Recommendation:
**	The user should drop the secondary index and recreate it.
*/
E_DU5039_REFRESH_ERR
"VERIFYDB:  Fatal error encountered while attempting\n
to refresh LDB info for distributed database %0c"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	verifydb was not able to perform the REFRESH_LDBS operation.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should attempt to rerun verifydb.
*/
E_DU503A_ILLEGAL_FLAG_COMBO
"VERIFYDB:  Illegal flag combination on verifydb command line.\n
You may not use %0c and %1c on the same command line.\n
These flags are mutually exclusive."
/*%
** Parameters:
**	%0c - first flag
**	%1c - second flag
**
** Explanation:
**	An illegal combination of arguments to was specified to verifydb.
**
** System Status:
**	The verifydb process stops.
**
** Recommendation:
**	The user should re-run verifydb with valid arguments.
*/
E_DU503B_EMPTY_IIPROCEDURE
"FATAL ERROR:  the iiprocedure system catalog is empty.\n
If database is checkpointed, use ROLLFORWARDDB backup to recover."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Table iiprocedure is a query processing DBMS catalog that contains 
**	definitions of stored procedures.
**	If it is empty, then the procedures needed for constraints are missing. 
**	This is a very severe error.
**
** System Status:
**	verifydb will stop processing, as it is not reasonable to act on this
**	database.
**
** Recommendation:
**	The user should attempt to recover the contents of the iiprocedure file
**	using the rollforwarddb utility.
*/
E_DU503C_TOO_MANY_VNODES   
"All dbnames must be in the same vnode."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A maximum of 10 names may be specified with the "-sdbname" flag.
**	The correct syntax is "name1 [name2 name3 ... name10]" or 
**      vnode::name1 [vnode::name2 vnode::name3 ... vnode::name10]". The
**	list starts and ends with a quote.  There is exactly one space
**	separating each name in the list.  Null lists are not valid.
**      All dbnames must be in the same vnode.
**
** System Status:
**	Processing of verifydb stops.
**
** Recommendation:
**	Reissue the verifydb command with the correct syntax.
*/
E_DU503D_RUNMODE_INVALID
"The selected operation may not be used in conjunction with the RUN or \n
RUNSILENT mode. Please try again with RUNINTERACTIVE run mode."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Certain verifydb operations do not have a RUN or RUNSILENT mode.
**
** System Status:
**	The verifydb process reports this error and terminates.
**
** Recommendation:
**	The user should reissue the command using the RUNINTERACTIVE mode.
*/
E_DU503E_INVALID_LOG_FILE
"Existing file (%0c) \n
is not a valid file for VERIFYDB to use.\n
Try removing this file, or run without the log file (use the -n flag on command line)."
/*%
** Parameters:
**	%0c - the full path to the invalid log file.
**
** Explanation:
**	verifydb was not able to open an existing verifydb log file. This was
**	because verifydb detected that the path to the existing log file 
**	did not reference a valid file, but some other operating system object
**	such as a directory or a link. 
**
** System Status:
**	verifydb considers it an error if it is unable to open its log
**	file.  In such a case, verifydb will print this error message and
**	terminate processing.  verifydb will never print this error if the
**	"-n" flag is used.
**
** Recommendation:
**	The best thing to do is to remove the object referred to in the 
**	error message and run verifydb again. 
**
**	You can bypass logging by using the "-n" flag; however, since verifydb
**	outputs lots of messages, use of this flag should be considered
**	carefully.
*/
/*%
E_DU5100_STAR_CDB_DEST_ONLY
"Cannot destroy coordinator database '%0c'.  \n
Destroy the distributed database '%1c' instead.\n"
/*%
** Parameters:
**	%0c - coordinator database name
**	%1c - distributed database name
**
** Explanation:
**	The user cannot destroy a coordinator database; the only way to
**	destroy a coordinator database is through destroying its distributed
**	database.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user should consider destroying the distributed database if
**	they really want to destroy the coordinator database.
**
**	NOTE:  A destroydb of a regular 6.0 or backend will destroy 
**	the coordinator database without knowing.
*/
E_DU5101_STAR_CANT_DEST_CDB
"Can not destroy database '%0c', which is a Coordinator database."
/*%
** Parameters:
**	%0c - coordinator database name
**
** Explanation:
**	The user is attempting to destroy a database that is a coordinator
**	database.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user cannot destroy a coordinator database without destroying a
**	distributed database.
*/
E_DU5102_STAR_DDB_DONT_EXIST
"Distributed entry of '%0c' does not exist in iidatabase."
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Cannot destroy distributed database, because there is no distributed
**	database entry in iidatabase.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	    None.
*/
E_DU5107_PRIVATE_STAR
"Distributed database %0c may NOT be a private database.\n
Please reissue the createdb command without the -p flag."
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	The coordinator database created on behalf of the distributed
**	database must not be a private database.  This is not allowed,
**	because the privileged association will come in as $ingres, and
**	$ingres will not have access to the private database.  Control
**	access to the coordinator database via GCN (iinetu) rather than
**	by trying to create a private distributed database.  The two
**	concepts (private and distributed) are mutually exclusive.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Re-issue the command correctly.  The user my create a private database
**	or the user may create a distributed database.  The user may not use
**	the "-p" flag and /D or /STAR on the same command line.
*/
E_DU5108_STAR_DDB_NAME_TOO_LONG
"Distributed database name is longer then %0d characters, cannot construct\n
coordinator database name because the result will be too long.\n
Please specify the coordinator database name by:\n
	CREATEDB [-S] [-uname] [-p] [-ddb_locationname]\n
		[-cckp_locationname] [-jjnl_locationname] ddbname/d [cdbname]"
/*%
** Parameters:
**	%0d - maximum length of distributed database name
**
** Explanation:
**	If the user does not supply a coordinator database name, distributed
**	createdb will attempt to construct one from the distributed database
**	name by prepending "ii" to it.  This message means that the resulting 
**	coordinator database name will be too long for a database name.
**
** System Status:
**Processing aborts.
**
** Recommendation:
**	The user can re-enter the distributed database name or specify a
**	coordinator database name.
*/
E_DU5109_STAR_INVALID_NAME
"%0c database name '%1c' is an invalid database name.\n"
/*%
** Parameters:
**	%0c - text ("Distributed" or "Coordinator")
**	%1c - distributed or coordinator database name
**
** Explanation:
**	An invalid database name for either a distributed or coordinator
**	database was encountered.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user can re-enter a valid distributed database name and/or
**	specify a valid coordinator database name.
*/
E_DU510A_STAR_CDB_ALREADY_EXIST
"Database name '%0c' already exists, cannot use as coordinator database.\n"
/*%
** Parameters:
**	%0c - coordinator database name
**
** Explanation:
**	The name specified as the coordinator database name already exists as
**	an entry in the iidatabase system catalog.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user can re-enter a valid distributed database name or
**	specify a valid coordinator database name.
*/
E_DU510B_STAR_DDB_ALREADY_EXIST
"Database name '%0c' already exists, cannot use as distributed database.\n"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	The name specified as the distributed database name already exists as
**	an entry in the iidatabase system catalog.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user can re-enter a valid distributed database name or
**	specify a valid coordinator database name.
*/
E_DU510C_STAR_DBDB_NOT_LEGAL
"Illegal distributed database name '%0c'.\n"
/*%
** Parameters:
**	%0c - distributed database name
**
** Explanation:
**	Cannot use "DBDB" or "IIDBDB" as a coordinator or distributed
**	database name.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user can re-enter a valid distributed database name or
**	specify a valid coordinator database name.
*/
E_DU510D_STAR_DDB_EQS_CDB
"Distributed database name is equal to the coordinator database name.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified a distributed database name that is the same as
**	the coordinator database name.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user can re-enter a valid distributed database name or
**	specify a valid coordinator database name.
*/
E_DU510E_STAR_TOO_MANY_ARGS
"Too many database names specified.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Too many coordinator or distributed database names were specified
**	on the command line.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	The user can re-enter a valid distributed database name or
**	specify a valid coordinator database name.
*/
E_DU510F_STAR_CANT_FIND_VNODE
"Cannot obtain value for II_GCNxx_LCL_VNODE, (xx is installation code).\n
Please define or check with DBA."
/*%
** Parameters:
**	None.
**
** Explanation:
**	NMgtAt() on II_GCNxx_LCL_VNODE returned an illegal value.  This should
**	be set to the node name.
**
**	Examples:
**	  On VMS: node name is rabbit and ii_installation = zz, use:
**	            define/group ii_gcnzz_lcl_vnode "rabbit"
**	  On Unix: node name is bunny and II_INSTALLATION = zz, use:
**	            ingsetenv II_GCNZZ_LCL_VNODE "bunny"
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Check with the DBA or define II_GCNxx_LCL_VNODE.
*/
E_DU5110_SV_NOT_STAR_SERVER
"Current server is not a STAR server.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The current server is a local database (LDB) server.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Check the installation.
*/
E_DU51111_SV_CANT_OPEN_IIDBDB
"Cannot open iidbdb in the current installation, please check permission or locks..\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Cannot open the iidbdb.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Check with the DBA for permissions.
*/
E_DU5112_SV_BAD_START_NAME
"Illegal database name to start Star*View..\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An invalid database name construct was specified for a Star*View
**	parameter.
**
** System Status:
**	Processing aborts.
**
** Recommendation:
**	Check with the DBA for permissions.
*/
E_DU5130_GW_INVALID_NAME
"%0c Gateway database name '%1c' is an invalid database name.\n"
/*%
** Parameters:
**	%0c - gateway (e.g., RMS)
**	%1c - gateway database name
**
** Explanation:
**	This message notifies the user that the gateway database name is
**	invalid.
**
** System Status:
**	Process aborts.
**
** Recommendation:
**	The user can re-enter a valid gateway database name.
*/
E_DU5131_GW_DBDB_NOT_LEGAL
"Illegal gateway database name '%0c'.\n"
/*%
** Parameters:
**	%0c - gateway database name
**
** Explanation:
**	Cannot use "DBDB" or "IIDBDB" as a gateway database name.
**
** System Status:
**	Process aborts.
**
** Recommendation:
**	The user can re-run using a valid gateway database name.
*/
E_DU6000_UPGRADEDB_USAGE
"Usage: UPGRADEDB (-all | dbname) [-f clientlist]\n\n
\t\"dbname\" is the name of a single database to convert;\n
\t\"-all\" will cause all databases in the installation\n
\t\tto be selected for conversion.\n
\t\"clientlist\" is an optional list of Front End clients:\n
\t\tNOFECLIENTS, INGRES, INGRES/DBD, VISION and WINDOWS_4GL\n
\t\t\t(NOFECLIENTS may not be used with other client names)"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The correct command-line parameters were not specified on the upgradedb
**	command line.  Therefore, a syntax message is printed.
**
** System Status:
**	  Processing will halt.
**
** Recommendation:
**	  Re-enter the command line with the correct syntax.
*/
E_DU6001_FAILED_CNF_READ
"Unable to read the configuration file for database 'iidbdb'\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A fatal error was encountered, because upgradedb could not read
**	the iidbdb config file.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Check permissions on database tree and verify that the config
**	file exists (if not, you must destroydb the database and recreate it).
**	Otherwise, you should fix permissions and quota, then re-issue the
**	upgradedb command.
*/
E_DU6002_BAD_IIDBDB_CONNECT
"Cannot connect to database 'iidbdb'\n
Please correct the problem and re-run UPGRADEDB.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A fatal error was encountered, because upgradedb could not connect to
**	the iidbdb.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Determine why upgradedb could not connect to the iidbdb (try using
**	TM to connect, or examine the error log). Correct that problem,
**	then re-issue the upgradedb command.
*/
E_DU6003_UNKNOWN_VERSION
"Unable to determine the server's version.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	dbmsinfo('_version') failed on this database.  This indicates some sort
**	of problem with the installation or server.  There may be an error
**	message in the server's error log indicating what the problem is.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Determine what the installation problem is (check the error log).
**	Correct that problem, then re-issue the upgradedb command.
*/
E_DU6004_WRONG_VERSION
"This UPGRADEDB expects an INGRES %0c server.\n
The server you are using is a version %1c server.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	%0c - required version ID string
**	%1c - server version ID string
**
** Explanation:
**	upgradedb is targeted to bring a database to a particular level (e.g.,
**	"6.4").  If the DBMS server is not at the same level as upgradedb, the
**	upgrade cannot occur.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	You may be using the wrong upgradedb frontend executable, or you may
**	have a server running that is at the wrong level.   In either case,
**	your installation is invalid for upgradedb. You should probably re-do
**	the install process, bring the installation back up, then run upgradedb
**	again.
*/
E_DU6005_NONEXISTENT_IIDBDB
"Database 'iidbdb' does not exist.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb is a frontend.  No frontend may run on an installation that
**	does not have an iidbdb.  Actually, there should not be any databases
**	in an installation that does not have an iidbdb, since createdb is also
**	a frontend and also cannot create any user databases until the iidbdb is
**	first created.  It does not make sense to be using upgradedb if you do
**	not have databases to upgrade, so let us assume that you have an
**	installation problem.  Common things that will make it impossible for
**	the system to find the iidbdb include: 1) the II_DATABASE symbol is
**	not correctly defined (concealed device on VMS); 2) the II_DATABASE
**	symbol does not point to the correct location; 3) someone has
**	removed the config file (aaaaaaaa.cnf) from the iidbdb's default
**	data location, or the disk device that contains the iidbdb is off-line;
**	or, 4) II_DBMS_SERVER is defined to point to a non-existent server.
**
**	Another possibility is that you are a release 5 user that is attempting
**	to upgrade to release 6 and you are inadvertently using the wrong tool.
**	Use convto60 to go from release 5 to release 6; upgradedb is only
**	used to go from one version of release 6 to another.  If you are 
**	converting from release 5, you must convert the dbdb to iidbdb before
**	you do anything else.  The command is:
**
**	  # convto60 -mcontinue -sdbname dbdb
**
** System Status:
**	  The upgradedb process stops.
**
** Recommendation:
**	  You must fix the installation problem (see explanation) and then
**	  verify that you can connect to the iidbdb via TM.  After that is
**	  successful, re-issue the upgradedb command.
*/
E_DU6006_INOPERATIVE_IIDBDB
"Database 'iidbdb' is currently inoperative.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The iidbdb database has been marked inoperative.  This can be caused
**	by many things, but an unsuccessful createdb on iidbdb is most likely.
**	In this case, you must destroy the iidbdb by deleting the files in its
**	default data directory and then bring down the DBMS server (to clear
**	cached info about the iidbdb), restart the server, then create iidbdb,
**	using: "createdb -S iidbdb".
**
**	If you already have an installation with other databases in it, attempt
**	the upgradedb of the iidbdb again.  If that fails check the upgradedb
**	log and the error log.  If neither log gives you a clear picture of the
**	problem, call Ingres Corporation Technical Support.
**
** System Status:
**	  The upgradedb process stops.
**
** Recommendation:
**	  You must determine why the iidbdb is inoperative and correct the
**	  problem.
*/
E_DU6007_NONEXISTENT_USER
"Unable to determine user permissions.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is printed when the upgradedb user is not a valid user
**	in this installation.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Contact your DBA and have them add you as a DBMS user.  If they
**	are not willing to make you a superuser, then you must get a 
**	superuser to run the upgradedb for you.
*/
E_DU6008_IIDBDB_WRONG_LEVEL
"Database 'iidbdb' is not at the proper level to be converted by UPGRADEDB.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The iidbdb is at some unknown level. Since upgradedb cannot determine
**	what level the database is at, it cannot convert it to the proper
**	level.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	The user must get the iidbdb to the proper level.  Perhaps it is
**	appropriate to recover the iidbdb from the last checkpoint and roll-
**	forward.  If that is not possible, then delete the iidbdb default
**	data directory, bring down the DBMS server and restart it (to clear the
**	server cache), then run "createdb -S iidbdb".
*/
E_DU6009_IIDBDB_UPGR_DISALLOW
"Database 'iidbdb' needs to be upgraded.\n
You are not authorized to convert the iidbdb database.\n
Only super-users are allowed to upgrade the iidbdb.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	User databases may not be converted unless the iidbdb is first
**	converted.  If a user database is specified for upgradedb before
**	the iidbdb is upgraded, then upgradedb will automatically upgrade
**	the iidbdb before upgrading the user database.  In this case, the
**	DBA is not a superuser, and only superusers may upgrade the iidbdb.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	The user must get a superuser to upgrade the iidbdb before the user may
**	upgradedb his database.
*/
E_DU600A_IIDBDB_JOURNALED
"*** ERROR ***\n
Database '%0c' is being journaled. You cannot upgrade a journaled database!\n
Turn off journaling using the command\n
\t ckpdb -j <dbname>\n
After upgrading resume journaling with:\n
\t ckpdb +j <dbname>\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb will automatically disable journaling via the "-A6" flag
**	if the database is pre-release 6.3.  In this case, the database is
**	post release 6.3, and journaling must be turned off by the user before
**	upgradedb will operate on the database.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Follow the advice provided in this message.
*/
E_DU600B_ACCESS_UPDATE_ERR
"Unable to update configuration file access field for database '%0c'\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not update the access field in the config file.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	This is probably an installation problem where upgradedb could not
**	locate the access field. This can be due to an improperly defined
**	II_DATABASE or other location symbol, or due to the device containing
**	the database default location going offline.  Ensure that the
**	installation is valid and re-issue the upgradedb command.
*/
E_DU600C_CMPT_UPDATE_ERR
"Unable to update configuration file compatlvl fields for database '%0c'\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not update the compatibility level fields in the config
**	file.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	This is probably an installation problem where upgradedb could not
**	locate the access field. This can be due to an improperly defined
**	II_DATABASE or other location symbol, or due to the device containing
**	the database default location going offline.  Ensure that the
**	installation is valid and re-issue the upgradedb command.
*/
E_DU600D_BAD_IIDBDB_RECONNECT
"Cannot reconnect to the iidbdb.\n
Please correct the problem and re-run UPGRADEDB.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upgradedb had to disconnect from the iidbdb and reconnect to ensure
**	access to some cached information, or to ensure the correct locking.
**	However, the reconnect failed.  This usually indicates that someone
**	else has taken a lock on the iidbdb or is in the iidbdb with a fast-
**	commit server that is not the same fast-commit server that upgradedb
**	is attached to, or there is some sort of installation problem.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Determine the installation problem (log full, symbols redefined,
**	concurrent fast-commit servers operating on the iidbdb, etc.) and
**	correct the problem.  Then re-issue the upgradedb command.
*/
E_DU600E_NO_DUMP_INFO
"Unable to obtain dump information for database 'iidbdb'.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb could not determine the iidump location for this database.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Ensure that your dump location symbol, II_DUMP, is properly defined,
**	then re-run upgradedb.
*/
E_DU600F_BAD_DUMP_CLEANUP
"Unable to delete the dump configuration file for database '%0c'.\n
UPGRADEDB is terminating.\n"
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb could not delete the config file from the dump location.
**	file.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Ensure that the dump location has the appropriate permissions and that
**	upgradedb is correctly installed and has the right permissions.  If
**	this is all correct, then someone is probably using the dump file for
**	an online backup or a recovery, and you will need to wait until the
**	operation on the database is complete.
*/
E_DU6010_NOT_SUPERUSER
"You must be an INGRES Super User to specify '-all'\n
If you are a DBA, you may retry specifying the name of a database you own."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Superuser status is required to use the "-all" flag.  The current
**	user is not a superuser.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	The user may specify individual databases for which they are the DBA.
**	Or, the user may get a superuser to do the "upgradedb -all" command
**	for them.  Alternatively, the user may get the system administrator
**	to make them a superuser.
*/
E_DU6011_NOT_DBA
"Database '%0c' is owned by %1c\n
You are not authorized to convert this database.\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**	%1c - database name
**
** Explanation:
**	You are only allowed to upgradedb a database if you are a 
**	superuser or the DBA of the database.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	The user must get an authorized person to run upgradedb for them or they
**	may get the system administrator to make them a superuser.
*/
E_DU6012_NONEXISTENT_TARGET
"Database '%0c' does not exist.\n
Please verify you have spelled db name correctly.\n
Conversion bypassed."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	upgradedb queried iidatabase for information on a user-specified
**	database name, but there was no tuple in iidatabase for that database.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	The user may want to re-enter the upgradedb command with the correct
**	specification of the database name.
*/
E_DU6013_NO_MORE_MEMORY
"Out of Memory.  Unable to proceed...\n"
/*%
** Parameters:
**	None.
**
** Explanation:
**	upgradedb ran out of memory while trying to build a list of target
**	databases.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	The user may want to tune the operating-system setup, or the user may
**	re-enter the upgradedb command for each database individually rather
**	than specifying the "-all" flag.
*/
E_DU6014_DBNAME_TWICE
"SYNTAX ERROR:  target database name is specified more than once."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the command line incorrectly.
**
** System Status:
**	The upgradedb process stops.
**
** Recommendation:
**	Re-issue the command using correct syntax.
*/
E_DU6015_BAD_CLIENT_NM
"  Conflicting dictionary client names.\n
The list of dictionary client names on the UPGRADEDB command line\n
contains conflicts. The client name '%0c' appears with\n
another dictionary client name(s);\n
'%0c' cannot be mixed with other dictionary client names,\n
because it tells UPGRADEDB to not upgrade any front-end catalogs in the\n
new database. Correct the command line and rerun UPGRADEDB."
/*%
** Parameters:
**	%0c - parameter name
**
** Explanation:
**	The special client name "nofeclients" appeared on the command line
**	along with some other client name(s). When "nofeclients" is specified,
**	no other dictionary client names can appear. The presence of the
**	dictionary client name "nofeclients" tells createdb to not create
**	any front-end catalogs in the new database.
**
** System Status:
**	The upgradedb process exits, and the database is not upgraded.
**
** Recommendation:
**	Check the dictionary client names you specified on the upgradedb
**	command line. Dictionary client names should follow the database name
**	at the end of the command line.
*/
E_DU6016_CANT_CRE_DIR
"Error creating the directory for the log file!\n
Unable to create subdirectory '%0c' in II_FILES:upgrade"
/*%
** Parameters:
**	%0c - subdirectory name
**
** Explanation:
**	upgradedb could not build a path to the II_CONFIG:[username]
**	directory, or it built the path but was unable to create the
**	directory.  This can occur if the user's login is not a legal
**	user name, if the II_SYSTEM location is not defined, if there 
**	is a shortage of disk space on the disk containing II_CONFIG, or 
**	if the directory tree permissions are incorrect.
**
** System Status:
**	upgradedb exits, and the database is not upgraded.
**
** Recommendation:
**	Check the items mentioned in the explanation and correct as appropriate.
**	Or, if the user prefers, re-direct standard output and then re-run
**	upgradedb using the "-n" flag, which will suppress output to the
**	upgradedb log file.
*/
E_DU6017_CANT_OPEN_LOG
"Error opening (or creating) log file '%0c'\n
This can occur if the same user is attempting to run two concurrent upgradedbs.\n
If a single login desires to run concurrent upgradedbs, then redirect output\n
to a file and use the -n flag to suppress upgradedb logging"
/*%
** Parameters:
**	%0c - log file path
**
** Explanation:
**	upgradedb could not open its log file: II_CONFIG:[username]upgrade.log
**	This can occur if there is a shortage of disk space on the disk
**	containing II_CONFIG, or if the directory tree permissions are
**	incorrect.  (If II_CONFIG is not defined, the system will default to
**	II_SYSTEM/ingres/files.)
**
** System Status:
**	upgradedb exits, and the database is not upgraded.
**
** Recommendation:
**	Check the items mentioned in the explanation and correct as appropriate.
**	Or, if the user prefers, re-direct standard output and then re-run
**	upgradedb using the "-n" flag, which will suppress output to the
**	upgradedb log file.
*/
E_DU6018_CANT_CSINITIATE
"Could not initiate \n
Failed to initiate access to shared memory (status was %0d).\n
This can occur because INGRES is down.  This can also occur if\n
upgradedb isn't setuid ingres (UNIX) or installed with enhanced\n
privileges (VMS).\n"
/*%
** Parameters:
**	%0d - status returned by CSinitiate()
**
** Explanation:
**	CSinitiate() must be called before any LG or LK routines are
**	called so that access to shared memory is coordinated.
**
** System Status:
**	upgradedb exits, and the database is not upgraded.
**
** Recommendation:
**	Check the items mentioned in the explanation and correct as appropriate.
*/
E_DU6019_CANT_UPGRADESQL92
"Could not upgrade from the non-sql92 installation to \n
the sql92 installation.\n"
/*%
** Parameters:
**	none
**
** Explanation:
**      Upgradedb doesn't allow to upgrade from non-sql92 to the
**      sql92 installation.
**
** System Status:
**	upgradedb exits, and the database is not upgraded.
**
** Recommendation:
**      Use cbf and select the Preferences menu item and then set
**      "ISO_ENTRY_SQL92" parameter to "OFF".
*/
E_DU601A_NOUPDATE_SYSCAT
"An error occurred updating data in a system catalog (%0c).\n
The error code was %1d.  This may have been caused by a lack of disk space,\n
or an undersized transaction log file."
/*%
** Parameters:
**	%0c - The catalog being updated
**	%1d - The Ingres error code returned to upgradedb
**
** Explanation:
**	The upgrade process attempted to update a system catalog, but the
**	update failed.  This may be caused by a lack of disk space in the
**	database's root data area;  it may also be caused by a lack of
**	transaction log space.
**
** System Status:
**	The upgrade is aborted.  The database will be unusable until the
**	problem is corrected, and upgradedb is re-run against the database.
**
** Recommendation:
**	Free up disk space, increase the transaction log size, or resolve
**	such other problem as may be indicated by the error code and
**	error log.  Then, re-run upgradedb for this database.
*/
E_DU601B_CANT_SET_RELFLAGS
"Could not set mandatory relstat, relstat2 flags for system catalogs"
/*%
** Parameters:
**	none
**
** Explanation:
**      Upgradedb failed to set the mandatory iirelation flags for 
**      certain system catalogs that require them.
**
** System Status:
**	upgradedb exits, and the database is not upgraded.
**
** Recommendation:
**      Check any SQL error messages and act accordingly
*/
E_DU601C_CANT_MAKE_CATS_HASH
"Could not modify some or all TCB2_PHYSLOCK_CONCUR tables back to HASH"
/*%
** Parameters:
**	none
**
** Explanation:
**      Upgradedb failed to modify some or all of the tables defined as
**      TCB2_PHYSLOCK_CONCUR back to their intended HASH table structure.
**
** System Status:
**	upgradedb exits, and the database is not upgraded.
**
** Recommendation:
**      Check any SQL error messages and act accordingly. A sysmod may
**      also help.
*/

E_DU6500_FAIL_UCTR_COMASST_LOAD
"Failed loading tables for Unicenter Common Asset products. \n"
/*%
** Parameters:
**      none
**
** Explanation:
**	For DU6500-DU651B: Createmdb cannot load tables 
**	for the particular category. 
**
** System Status:
**      Createmdb exits, and the database objects for mdb are not created.
**
** Recommendation:
**      recreate the mdb database and rerun createmdb to load the tables.
*/

E_DU6501_FAIL_ICAN_SVCMGNT_LOAD
"Failed loading tables for iCan Service Management Suite.\n"
E_DU6502_FAIL_AUTOSYS_JVM_LOAD
"Failed loading tables for Autosys, JV, and JM.\n"
E_DU6503_FAIL_UCTR_SVCPLUS_LOAD
"Failed loading tables for Unicenter ServicePlus Predictive Tool.\n"
E_DU6504_FAIL_DATA_TRSFRM_LOAD
"Failed loading tables for Advantage Data Transformer.\n"
E_DU6505_FAIL_ETRST_DIRECT_LOAD
"Failed loading tables for eTrust Directory.\n"
E_DU6506_FAIL_AFSN_WBENCH_LOAD
"Failed loading tables for Allfusion Change Manager Enterprise Workbench .\n"
E_DU6507_FAIL_ETRST_2020_LOAD
"Failed loading tables for eTrust 20/20 and eTrust Audit.\n"
E_DU6508_FAIL_CLVP_PORTAL_LOAD
"Failed loading tables for CleverPath Portal.\n"
E_DU6509_FAIL_UCTR_MGMTPTL_LOAD
"Failed loading tables for Unicenter Management Portal.\n"
E_DU650A_FAIL_CLVP_CNTMGMT_LOAD
"Failed loading tables for CleverPath Portal Content Management.\n"
E_DU650B_FAIL_HVST_ALL_LOAD
"Failed loading tables for Harvest.\n"
E_DU650C_FAIL_ETRST_PKI_LOAD
"Failed loading tables for eTrust PKI.\n"
E_DU650D_FAIL_ARCSEREV_LOAD
"Failed loading tables for ArcServe.\n"
E_DU650E_FAIL_UCTR_LOTUS_LOAD
"Failed loading tables for Unicenter Management 
for Lotus Notes/Domino and MS Exchange.\n"
E_DU650F_FAIL_MODEL_MGR_LOAD
"Failed loading tables for Model Manager.\n"
E_DU6510_FAIL_UCTR_WEBSPHR_LOAD
"Failed loading tables for Unicenter Management for Websphere MQ.\n"
E_DU6511_FAIL_ETRST_VULMGR_LOAD
"Failed loading tables for eTrust Vulnerability Manager.\n"
E_DU6512_FAIL_ALFSN_PRCMGR_LOAD
"Failed loading tables for AllFusion Process Management Suite.\n"
E_DU6513_FAIL_CLVP_WKFL_LOAD
"Failed loading tables for CleverPath Workflow.\n"
E_DU6514_FAIL_UCTR_TSREORG_LOAD
"Failed loading tables for Unicenter TSreorg for Oracle.\n"
E_DU6515_FAIL_UCTR_ENTDBA_LOAD
"Failed loading tables for Unicenter Enterprise DBA.\n"
E_DU6516_FAIL_UCTR_DBANLZR_LOAD
"Failed loading tables for Unicenter Database Analyzer for Oracle.\n"
E_DU6517_FAIL_UCTR_DBM_LOAD
"Failed loading tables for Unicenter Database Management.\n"
E_DU6518_FAIL_UCTR_DBPERFM_LOAD
"Failed loading tables for Unicenter DB Performance Management.\n"
E_DU6519_FAIL_UCTR_WSDM_LOAD
"Failed loading tables for Unicenter WSDM.\n"
E_DU651A_FAIL_ETRST_EIAM_LOAD
"Failed loading tables for eTrust eIAM.\n"
E_DU651B_FAIL_MDB_CAT_LOAD
"Failed loading tables for MDB catalog.\n" 
E_DU6520_LOADMDB_FAILED
"Execution of LOADMDB failed. Please correct the problem, and\n
rerun loadmdb after destroying and recreating the database.\n"
/*%
** Parameters:
**
** Explanation:
**	createdb was specified with both -i and -n options. Only 
**	one unicode normalization form is allowed for a database. 
**
** System Status:
**	The createdb process exits, and the database is not created.
**
** Recommendation:
**	Please specify only one unicode normalization form. Valid options
**	are -n (Normal form D) or -i (Normal form C).
*/
E_DU6521_UNORM_NFC_WITH_NFD
"Createdb failed to create database, unable to determine unicode\n 
normalization form to use as both -n and -i options are specified\n
Please supply either -n for Normal form D, or -i for Normal form C.\n"
