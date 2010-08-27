SQLSTATE_MAPPING_REQUIRED

/*
**    Message file for Facility code: _US
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	11-may-93 (anitap)
**	    Added support for syntax errors for "SET UPDATE_ROWCOUNT TO CHANGED/
**	    QUALIFIED."
**	06-jul-93 (anitap)
**	    Removed above change.
**	16-aug-93 (nancy)
**	    Upgraded message US0008 to reflect new conversion steps.
**	3-sep-93 (stephenb)
**	    Updated message E_US09CC to include USER as a valid option on DROP.
**	10-sep-93 (robf)
**	   Add E_US4122/4123/4124 for CREATE/ALTER/DROP PROFILE syntax.
**	15-sep-93 (robf)
**	   Updated text for E_US080/0081 so be appropriate for both
**	   CREATEDB and ACCESSDB - could be issued on failed EXTEND operation
**	   when referring to CREATEDB was inappropriate.
**	11-oct-93 (rblumer)
**	   Added message for syntax error on single-quoted ids (E_US0A9B_2715);
**	   added specific messages for different REF constraint violations
**	   (E_US1906 thru E_US1909); changed sqlstates for NOT NULL/mandatory
**	   errors from SS23000_CONS_VIOLATION to SS23500_CHECK_CONS_VIOLATION.
**	   changed sqlstates for UNIQUE violation errors from
**	   SS50001_INVALID_DUP_ROW to SS23501_UNIQUE_CONS_VIOLATION.
**      13-oct-1993 (rmuth)
**	   Alter the text for E_US14E6.
**	18-oct-93 (robf)
**         Corrected E_US0087, this is really E_US0087_EXTEND_ALREADY, not
**	   SPARE.
**	28-oct-93 (robf)
**         Add E_US101D_4125 for new session SET MAX limits.
**	28-oct-93 (stephenb)
**	   Update E_US18F6 to reflect the fact that the user is actually create
**	   despite the error.
**	17-nov-93 (robf)
** 	   Add E_US101E_4126 for SET ROWLABEL statement, add
**	   WITH [NO]DESCRIPTION clause to SET SESSION statement
**	26-nov-93 (robf)
**	    Add E_US2473_9331_ALARM_ABSENT, E_US2472_9330_ALARM_EXISTS
**	    Add E_US2473_9331_ALARM_ABSENT, E_US2472_9330_ALARM_EXISTS,
**		E_US2473_9331_ALARM_ABSENT, E_US2474_9332_ALARM_NO_DB
**		E_US2475_9333_IISECALARM_OPEN, E_US2476_9334_IISECALARM_ERROR,
**		E_US2477_9335_ALARM_EVENT_ERR, E_US2478_9336_BAD_ALARM_GRANTEE,
**	03-dec-93 (dianeh)
**	    Refine text destined for messages.txt file.
**	21-dec-93 (robf)
**          Added E_US247A_9338_MISSING_ALM_EVENT
**	22-feb-94 (robf)
**          Add E_US1021_4129. 
**	        E_US247B_9339_IIROLEGRANT_OPEN
**	        E_US247C_9340_IIROLEGRANT_ERROR
**	10-mar-94 (andre)
**	    during SQLSTATE project, a number of messages were mapped to 
**	    incorrect SQLSTATEs - now we will correct it
**	22-mar-94 (robf)
**          Add E_US1022_4130, E_US1023_4131, 
**	        E_US2480_9344_REVOKE_ROLE_TYPE
**	18-apr-1994 (pearl)
**	    Add E_US1764_5988.
**      02-Oct-1994 (Bin Li)
**          Modify the error message for E_US0010, so that the database name
**          can be passed as an argument.   
**	15-jul-94 (robf)
**          Add E_US1A06_6662
**	06-jan-95 (forky01)
**	    Add E_US2481_9345_MODIFY_TO_NO_WITH.
**	09-jan-95 (nanpr01)
**	    Added E_US14E9_5353_LOG_INCONSIST_TBL_ERR
**	    Added E_US14EA_5354_PHYS_INCONSIST_TBL_ERR
**	03-feb-95 (forky01)
**	    Added E_US0988_2440_SEL_CNTA_FROM
**      06_Feb-95 (liibi01)
**          Modify the help message for the grammar of 'ALTER USER' statement.
**      01-Mar-95 (liibi01)
**          Bug 67100, fix typo for message E_US14EA.
**      22-Mar-95 (liibi01)
**          Modify the message E_US0EF2_3826:SS42000_SYN_OR_ACCESS_ERR.
**      23-Mar-95 (liibi01)
**          Bug 67665, modify the error message E_US0AC7.
**      07-apr-1995 (kch)
**          Correction to mapping of error E_US1208_4616, "Duplicate
**          records were found" - now maps to Error
**          SS23501_UNIQUE_CONS_VIOLATION, giving correct GE Code 30210.
**          This change fixes bugs 51669 and 56940.
**      14-apr-1995 (dilma04)
**          Modify the error messages E_US172C_5932 and E_US1725_9925
**          according to CURSOR STABILITY/REPEATABLE READ project.
**	02-may-1995 (harpa06)
**	    Correct the syntax for CREATE USER and ALTER USER error messages.
**	    This fixes bug #68423.
**      17-may-1995 (dilma04)
**          Add the message E_US14EB_5355_DROP_READONLY_TABLE_INDEX.
**          Alter the text for E_US14E6_5350_READONLY_TBL_ERR.
**	30-may-1995 (harpa06)
**	    Corrected the syntax for DELETE in QUEL. This fixes bug #68849.
**	06-jun-1995 (dilma04)
**	    Added the error message E_US0AA7_2727. This fixes bug #69039.
**      07-mar-96 (dilma04)
**          Back out changes related to CURSOR STABILITY/REPEATABLE READ
**          project which are not ducumented in OpenINGRES 1.2/00.
**      19-apr-1996 (stial01)
**          Added E_US14EC_5356 for invalid page_size message
**	19-jun-1996 (sweeney)
**	    Fix typos/anglicisms.
**	23-jul-1996 (ramra01)
**	    Modify some of the CREATE TABLE messages to 
**	    CREATE or ALTER TABLE.
**	    Added message E_US0F11_3857.
**	23-aug-96 (nick)
**	    Added E_US14ED_5357_INCONSISTENT_INDEX_ERR.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**	16-Oct-96 (ramra01)
**	    Added msg E_US0F12_3858 for Alter Table drop column
**	06-nov-1996 (canor01)
**	    Add page_size to quel create table message E_US07FF.
**	08-nov-1996 (nanpr01)
**	    Add page_size to the list for message E_US14DE.
**	14-nov-1996 (nanpr01)
**	    Add message E_US0801_2049 for disallowing modify on temp table.
**	18-nov-1996 (hanch04)
**	    Added MODIFY for description of E_US13F9_5113
**      22-nov-1996 (dilma04)
**          Row-Level Locking Project:
**            - Re-added changes related to Isolation Levels which had to be
**              backed out because of the integration mistake;                  
**            - Added ROW parameter to E_US1724;
**            - Added message E_US0F1C_3868.
**	10-dec-1996 (nanpr01)
**	    bug 51716 : Comment syntax still shows obsolete extensions.
**	10-jan-97 (inkdo01)
**	    slight alteration to text of US0F11.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_US138F_5007.
**	    Added <access mode> syntax to US0F1C.
**	14-Aug-1997 (shero03)
**	    B84393: give a better msg for range used in create table.
**      14-oct-1997 (stial01)
**          Added E_US1723_5923, changed text of E_US1725_5925.
**      12-nov-1997 (stial01)
**          Corrected syntax of SET SESSION in E_US1019
**	18-nov-1997 (nanpr01)
**	    Added E_US2485_9348 for not allowing create integrity on
**	    secondary index.
**	18-may-1998 (nanpr01)
**	    Added message for parallel Index.
**	24-aug-98 (inkdo01)
**	    Added messages for FOR loops.
**	27-aug-98 (stephenb)
**	    E_US0A27_2599 should be E_US0A26_2598 (coding slip that's been
**	    there for ever).
**	05-Nov-98 (carsu07) 
**	    Removed the reference to CONVTO60 in the error message E_US0014.
**	10-nov-98 (sarjo01)
**	    Added E_US2485_9349, Insufficient db priv to issue TIMEOUTABORT.
**      28-Jan-1999 (hanal04)
**          Added E_US13FF_5119 to distinguish between BAD LOCATION
**          (E_US13F9_5113) and BAD OLDLOCATION errors. b92641.
**	25-nov-1998 (nanpr01)
**	    Added E_US10DE_4318 fro interval day overflow message.
**	09-Jun-1999 (shero03)
**	    Added E_US113B_4411 - BAD_HEX_CHAR.
**	6-aug-99 (inkdo01)
**	    One more slight alteration to text of US0F11.
**	3-sep-99 (inkdo01)
**	    Added E_US0B75_2933 for case function.
**	1-oct-99 (inkdo01)
**	    Added E_US0B76_2934 - binary aggs not yet supported.
**	17-dec-1999 (gupsh01)
**	    Modified text for error message E_US138F_5007, in order to include 
**	    error reporting for the readonly database. 
**      04-feb-2000 (gupsh01)
**	    Added new error text for illegal IP address type entered.(bug 100265)
**	07-mar-2000 (gupsh01)
**	    Added new error E_US0090_CREATEDB_RONLY_NOTEXIST for the readonly   
**	    databases, for cases where the user provides the createdb command for 
**	    readonly databse and the database directory does not exist at  
**	    specified data location. 
**      09-feb-2000 (gupsh01)
**          Modified the error text for E_US09CE to include the syntax for
**	    parallel index creation.
**	6-apr-00 (inkdo01)
**	    Added US0F09 for syntax errors in constraint clauses.
**	14-mar-2000 (gupsh01)
**	    Added error checking for invalid parameters passed to SET
**	    random_seed statement. 
**	6-jul-00 (inkdo01)
**	    Changed text of errors US0885/6 to include bad datatype for GROUP BY,
**	    as well as ORDER BY.
**      6-feb-2001 (gupsh01) 
**          Added new error message E_US14EE for invalid 'with' clause supplied 
**          for the copy statement. 
**	23-mar-2001 (toumi01)
**	    Added message E_US0F80 for QUEL SET TRANSACTION support.
**	27-mar-01 (inkdo01)
**	    Added E_US098D to indicate "return row" in a proc with no "result
**	    row" declaration.
**	02-Apr-2001 (jenjo02)
**	    Added E_US0088, E_US0089, E_US18DD, E_US18DE for raw locations.
**	10-apr-01 (inkdo01)
**	    Fix text in some row producing procedure errors.
**	18-apr-01 (inkdo01)
**	    Added US0800 to flag nchar's with no corresponding "createdb -n"
**	27-apr-01 (inkdo01)
**	    Added US098F for row procs called from dyn SQL or another proc.
**	30-apr-01 (inkdo01)
**	    Added US086C to flag "first n" clauses in bad places.
**	11-May-2001 (jenjo02)
**	    Modified text of US18DD from rawblocks to raw percent.
**	19-may-2001 (somsa01)
**	    Modified intgroduction of E_US2486_9350_MODIFY_TO_TABLE_OPTION
**	    to E_US248A_9354_MODIFY_TO_TABLE_OPTION.
**	22-may-01 (inkdo01)
**	    Added US0F24 to explain RETURN ROW syntax error.
**	15-Oct-2001 (jenjo02)
**	    Changed E_US18DE_INVALID_USAGE to INVALID_RAW_USAGE,
**	    Added E_US18DF, E_US18E0, E_US18E1, E_US18D7.
**	    Added RAWPCT syntax to E_US0EF4.
**	30-oct-01 (inkdo01)
**	    Added E_US1421 to flag Quel range statement with table qualifier
**	    other than "session".
**	6-nov-01 (inkdo01)
**	    Reused E_US0B76 to identify functions that use special position/
**	    trim syntax.
**	7-nov-01 (inkdo01)
**	    Added E_US0B77/8 for various "trim" function syntax errors.
**	17-dec-01 (abbjo03)
**	    Added E_US248B/C/D for ALTER TABLE WITH [NO]REPLICATION.
**	04-jan-2002 (abbjo03)
**	    Add E_US248E for ALTER TABLE WITH REPLICATION.
**	7-mar-02 (inkdo01)
**	    Added E_US0F27, E_US0F28, E_US0F29, E_US18AC, E_US18AD, E_US18AE
**	    E_US18AF, E_US18B0, E_US1912, E_US1913, E_US1914, 
**	    E_US1915 for sequence definitions.
**	19-mar-02 (toumi01)
**	    Changed messages for DB_MAX_COLS from 300 to 1024.
**	08-Aug-02 (bonro01)
**	    Bug 108471. Modified US0F1C and US0F80 for SET TRANSACTION syntax.
**	13-Aug-2002 (jenjo02)
**	    Added E_US1722_5922 for SET LOCKMODE TIMEOUT = NOWAIT,
**	    modified US1726_5932 to exclude LOCKMODE reference.
**	28-feb-03 (inkdo01)
**	    Edit text for US0F28 message and add US18B3 message - for sequences.
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          Added E_US2490_9360 for DDL operations which have been blocked
**          because they would cause a constraint to be abandoned.
**	18-Jul-2003 (gupsh01)
**	    Added error E_US1968_6504_UNOPRIV_WDEFAULT for Create user 
**	    statements which have -noprivileges and with profile options, but
**	    do not have -nodefault_privileges option, as user may inherit 
**	    default privileges from the profile. 
**	27-Jan-2004 (schka24)
**	    Added partition= syntax messages.
**	23-Feb-2004 (schka24)
**	    More partition errmsgs, for modify.
**	15-Apr-2004 (gupsh01)
**	    Added alter table alter column erro erro.
**	18-May-2004 (jenjo02)
**	    Added ON_LOGFULL stuff to E_US1019.
**	18-Jun-2004 (gupsh01)
**	    Added error message E_US0F2E_3886, for reporting syntax errors on
**	    Alter table ...Alter column statements.
**	27-july-04 (inkdo01)
**	    Added E_US1400_5120 to show "natural join" syntax not yet supported.
**	5-Aug-2004 (schka24)
**	    Add message for modify uniqueness vs partitioning.
**	11-Nov-2004 (gupsh01)
**	    Fixed E_US0F11_3857 error, which is not applicable to alter table 
**	    alter column.
**	13-dec-04 (inkdo01)
**	    Added E_US1918_6424 for collation support.
**	28-Dec-2004 (jenjo02)
**	    Added E_US008A, E_US008B.
**      02-mar-2005 (stial01)
**          Added E_US191C_6428_NODUPLICATES (b113993)
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	10-may-2005 (raodi01) bug110709 INGSRV2470
**        Added E_US0806_2054 to handle modifying temporary tables with 
**        key compression
**	19-Jul-2005 (toumi01)
**	    Added E_US18E6
**	25-Jul-2005 (mutma03)
**	    Added '-i' option to help message  "national character data types 
**          require '-n' option with CREATEDB".  
**	    Added E_US18E6.
**	29-Jul-2005 (thaju02)
**	    Added E_US008C, E_US008D.
**	24-Aug-2005 (thaju02)
**	    Modified E_US1019.
**	13-Sep-2005 (thaju02) B115209
**	    Added E_US15B7.
**	29-nov-05 (inkdo01)
**	    Added E_US0AA8 for invalid authID qualifier in grant/revoke.
**	7-dec-05 (inkdo01)
**	    Added E_US0AA9 for bad Unicode literal syntax.
**	30-jan-06 (dougi)
**	    Added E_US086D for derived tables with inconsistent column
**	    override list.
**	22-mar-06 (toumi01)
**	    Add E_US2491 akin to E_US2490 to supply advice re
**	    NODEPENDENCY_CHECK for the MODIFY command.
**	07-May-2006 (jenjo02)
**	    Add "CLUSTERED" to US1586, US15A4
**	2-Jun-2006 (kschendel)
**	    Delete a few unused parameter-marker error messages.
**	    (US0903 thru US0905, ie 2307 thru 2309).
**	15-june-06 (dougi)
**	    Altered text for US18A7 and added US18B4 & US18B5 for "before" 
**	    triggers.
**	22-jun-06 (toumi01)
**	    Allow session temporary tables to be referenced without the
**	    "session." in DML. This eliminates the need for this Ingres-
**	    specific extension, which is a hinderance to app portability.
**	    Add E_US0851 to forbid a (dangerous) ambiguous reference
**	    should a temp table and a permanent table have the same name.
**	23-Jun-2006 (kschendel)
**	    Add too-many-partitions message.
**	26-Jun-2006 (jonj)
**	    Changed text of US1262, as things other than logfull can
**	    trigger this response.
**	16-Aug-2006 (toumi01)
**	    For messages E_US2490 and E_US2491 fix spell ("contraint" should
**	    be "constraint").
**	29-Aug-2006 (gupsh01)
**	    Added syntax errors for ANSI datetime literals.
**	05-Sep-2006 (gupsh01)
**	    Added error E_US10DF for ANSI datetime string coercion.
**	06-Sep-2006 (gupsh01)
**	    Correct numbers of messages 2730 and 2731.
**	18-Sep-2006 (gupsh01)
**	    Added error message E_US10E0_4320 for Date alias not set.
**	22-sep-06 (toumi01)
**	    Repurpose E_US0851 to forbid mix of old/new GTT syntax.
**	15-nov-2006 (dougi)
**	    Added US0853 when attempt is made to alter/drop partitioning col.
**	16-Nov-2006 (gupsh01)
**	    Added error message E_US10E1_4321:SS50000_MISC_ERRORS.
**	22-Nov-2006 (gupsh01)
**	    Added error message E_US10E2_4322
**	10-Dec-2006 (gupsh01)
**	    Added error message E_US10E3_4323.
**	11-dec-2006 (dougi)
**	    Change various messages to say "date/time" instead of "date".
**	11-dec-2006 (gupsh01)
**	    Reworded Error message E_US10E2_4322.
**	12-dec-2006 (gupsh01)
**	    Added error messages E_US10E_4325 to E_US10EB_4331 for incorrect
**	    formats for ANSI date/time types.
**      27-dec-06 (kibro01)
**          Change incorrect '%s' to '%<n>c' in error messages
**	8-jan-2007 (gupsh01)
**	    Added error messages E_US10ED_4333. 
**	1-feb-2007 (dougi)
**	    Added message E_US10F0_4336 for retrievals of unsupported types.
**      27-feb-07 (kibro01) b117546
**          Add error for structure not sortkeyed in IMA tables
**	19-mar-07 (kibro01) b117391
**	10-apr-2007 (dougi)
**	    Add US08B5 (2229) for non-updateable KEYSET cursors.
**	24-may-2007 (toumi01)
**	    Add US0903 (2307) for KEYSET error on missing unique key/ix.
**	1-june-2007 (dougi)
**	    Add US08B7 (2231) for scrollables with LOB result column.
**	19-june-2007 (dougi)
**	    Add US1401 to prohibit scalar subqueries in some contexts and US08B8
**	    to prevent scrollable updateable cursors on a Star db.
**	26-sep-07 (kibro01) b119081
**	    Fix error message for MODIFY
**	27-sep-2007 (kibro01) b119191
**	    Since US0B61 can mean two things now (and they cannot be 
**	    distinguished from where they arise) ensure the error message
**	    reflects the two possibilities.
**      19-Dec-2007 (kibro01) b119624
**          Add US15B8_5560 error for unknown column in alter table statement.
**	19-dec-2007 (dougi)
**	    Add US086E/86F for first/offset "n" improvements.
**	29-jan-2008 (dougi)
**	    Add US0874/5 for first/offset "n" value checking.
**	10-Feb-2008 (kiria01) b120043
**	    Added US10F1/2/3 for date operators
**	5-Mar-2008 (kibro01) b119726
**	    Add US0892 for AFTER restriction on LOBs
**	7-Mar-2008 (kibro01) b119726
**	    Amend US0892 for all large objects
**      12-Mar-2008  (coomi01) b119995
**          Add US10F4 for ansi trim spec error.
**	18-march-2008 (dougi)
**	    Added US0990 for positional parms and US0876 for table procs.
**	28-Apr-2008 (thaju02) (B120283)
**	    Added E_US1950_6468_NO_PARTRULE_SPEC.
**	22-May-2008 (toumi01) b120418
**	    Correct "dependant" (noun) to "dependent" (adj.) in 2 messages.
**	06-Oct-2008 (gupsh01)
**	    Added error message E_US082C_2092, for create table statement
**	    where the length value is specified out of the valid range
**	    for the column datatype.
**	10-Oct-2008 (gupsh01)
**	    Fixed the typo in the error message E_US082C_2092.
**	03-Nov-2008 (gupsh01)
**	    Add the missing line number in E_US0AA9_2729 error message
**	21-nov-2008 (dougi) Bug 121260
**	    Remove parameter for error 2448 (US0990).
**	3-dec-2008 (dougi) BUG 121324
**	    Add error 2167 to flag attempts at recursion in RPPs/table procs.
**	9-jan-2009 (dougi) BUG 121486
**	    Add error 2168 to flag LOB retrieval from tprocs (not supported).
**	16-Jan-2008 (kiria01) b121515
**	    Add error 2183 for union of typeless NULLs
**      21-Jan-2009 (horda03) Bug 121519
**          Add E_USE_US082D_2093 for CREATE TABLE failure when all table ids used.
**	10-Feb-2009 (kiria01) SIR 120804
**	    Added missing options to help for 'random' sequence support
**      30-Mar-2009 (gefei01) bug 121870
**          Added E_US0991_2449_TPROC_DML_ERR to disallow DML
**          in table procedure.
**	15-Apr-2009 (kibro01) b121897
**	    Add E_US2493_9363 for no subselects allowed in select list error.
**	15-Apr-2009 (gupsh01)
**	    Added E_US0AAC_2732 for reporting failure to convert between unicode
**	    and nonunicode encoding due to missing convertor file. (bug 121953)
**	7-May-2009 (kibro01) b121738
**	    Added E_US08B8_2232 for an ORDER BY element which only appears
**	    on one side of a UNION query.
**	28-May-2009 (kschendel) b122118
**	    Spiff up 5540 message as part of code polishing in psf.
**	20-Aug-2009 (kschendel) b122510
**	    New message 2512 for bad type in CAST expression.
**      28-sep-2009 (joea)
**          Add E_US0ADE_2782.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Update US0008 just a tiny bit.
**	25-nov-2009 (stephenb)
**		Alter the text for E_US1265_4709 to more accurately describe the
**	    problem and offer advice on how to fix it.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add E_US125B Unable to serialize row access,
**	    US120C invalid lock level for table
**	18-Mar-2010 (kiria01) b123438
**	    Added user error E_AD1028_NOT_ZEROONE_ROWS for SINGLETON
**	    aggregate for scalar sub-query support and updated the UPDATE
**	    table syntax.
**      23-Mar-2010 (hanal04) Bug 122436
**          Added E_US1072.
**	02-Apr-2010 (jonj)
**	    SIR 121619 MVCC: add E_US125A_4698 Insufficient undo information
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	12-Apr-2010 (gupsh01) SIR 123444.
**	    Added E_US2494_9364, E_US2495_9365 for rename table/columns.
**	21-apr-2010 (toumi01) SIR 122403
**	    Added messages for encryption.
**	20-May-2010 (kiria01) b123773
**	    Added E_US0A00_2560 and corrected column alias syntax in error text.
**	09-Jun-2010 (gupsh01) b123841.
**	    Added error E_US24C4_9412.
**      23-Jun-2010 (coomi01) b123763
**	    Add error E_US18E8_6376 for attempts to alter iidbdb locations.
**      01-Jul-2010 (horda03) B123234
**          Altered the E_US10D9 message as date_trunc only works on absolute dates,
**          the date_part function may/may not work on an Interval, it depends on
**          the 'unit' being requested (for example 'week' will not work on an
**          interval. For this latter case, added new error E_US10F5.
**	29-Jul-2010 (miket) SIR 122403
**	    Clarify language for msg E_US24BF.
**	30-Jul-2010 (miket) SIR 122403
**	    Improve content/format for E_US0F0F, and add encrypt clause.
*/

#define _US_CLASS   0
E_US0001:SS50002_LIMIT_EXCEEDED
"Number of users has exceeded limit."
E_US0002:SS42503_INSUF_PRIV
"User does not have permission to use
one of the flags given."
E_US0003:SS42503_INSUF_PRIV
"User does not have permission to access this database."
E_US0004:SS50000_MISC_ERRORS
"Could not access the database.
Make sure images are installed correctly."
E_US0005:SS50000_MISC_ERRORS
"Could not open the iirelation table."
E_US0006:SS50000_MISC_ERRORS
"Could not open the iiattribute table."
E_US0007:SS50000_MISC_ERRORS
"Could not read the config file."
E_US0008:SS50000_MISC_ERRORS
"The requested database is not compatible with this server.
\nUpgrade it with the UPGRADEDB utility.
\nIf a Release 5 database, convert it to Release 6.4 with the CONVTO60 utility,
\nthen upgrade it with the UPGRADEDB utility."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The specified database was not compatible with the server.
**
** System Status:
**	The session is terminated.
**
** Recommendation:
**	Convert the specified database, using the appropriate conversion
**	utility.  To determine what utility to use:
**
**	    $ SQL iidbdb
**	    > select compat_level from iidatabase_info
**	    > where database_name = 'databasename'
**	    > \g\q
**
**	If compat_level is "5.0", run convto60; else run upgradedb.
**
*/
E_US0009:SS50000_MISC_ERRORS
"Bad parameter given."
E_US000A:SS50000_MISC_ERRORS
"iiuser table does not exist."
E_US000B:SS50000_MISC_ERRORS
"Cannot open iiuser table."
E_US000C:SS50000_MISC_ERRORS
"You are not a valid user."
E_US000D:SS50000_MISC_ERRORS
"The -u flag specified a nonexistent user."
E_US000E:SS50000_MISC_ERRORS
"iidatabase table does not exist."
E_US000F:SS50000_MISC_ERRORS
"Could not open or read the iidatabase table."
E_US0010:SS08004_CONNECTION_REJECTED
"Database does not exist: '%0c'."
E_US0011:SS50000_MISC_ERRORS
"Table iidbaccess does not exist."
E_US0012:SS50000_MISC_ERRORS
"Cannot open or read iidbaccess table."
E_US0013:SS50000_MISC_ERRORS
"Request for a lock failed."
E_US0014:SS5000H_UNAVAILABLE_RESOURCE
"Database not available at this time.\n
    o The database may be marked inoperable.  This can occur if\n
      CREATEDB failed.\n
    o An exclusive database lock may be held by another session.\n
    o The database may be open by an exclusive (/SOLE) DBMS server."
E_US0015:SS50004_SYS_CONFIG_ERROR
"Error initializing transaction system."
E_US0016:SS50000_MISC_ERRORS
"Error writing to CONFIG file."
E_US0017:SS50000_MISC_ERRORS
"Couldn't find an CONFIG file."
E_US0018:SS50000_MISC_ERRORS
"Couldn't find CONFIG file."
E_US0019:SS5000J_INCONSISTENT_DBMS_CAT
"The database may be inconsistent; please contact the\n
        system manager"
E_US001A:SS50000_MISC_ERRORS
"Could not find database default AREA in IILOCATIONS."
E_US001B:SS50000_MISC_ERRORS
"Could not find database checkpoint AREA in IILOCATIONS."
E_US001C:SS50000_MISC_ERRORS
"Could not find database journal AREA in IILOCATIONS."
E_US001D:SS50000_MISC_ERRORS
"Fatal in looking up an AREA in IILOCATIONS."
E_US001E:SS50000_MISC_ERRORS
"Could not find an AREA for LOCATIONNAMES in IIEXTEND catalog."
E_US001F:SS50000_MISC_ERRORS
"Could not open or read the IIEXTEND catalog."
E_US0020:SS50000_MISC_ERRORS
"Could not open or read the IILOCATIONS catalog."
E_US0021:SS5000B_INTERNAL_ERROR
"Fatal error in forming list of available LOCATIONNAMES."
E_US0022:SS50000_MISC_ERRORS
"Either the flag format or one of the flags is incorrect,\n
or the parameters are not in proper order."
E_US0023:SS42503_INSUF_PRIV
"You are not authorized to use one of the specified flags."
E_US0024:SS50000_MISC_ERRORS
"Couldn't access DB directory."
E_US0025:SS50000_MISC_ERRORS
"Couldn't access JNL directory."
E_US0026:SS5000J_INCONSISTENT_DBMS_CAT
"Database is inconsistent. please contact the\n
        system manager"
E_US0027:SS50000_MISC_ERRORS
"The are no more journal logs available. Please try later."
E_US0028:SS50000_MISC_ERRORS
"Couldn't open or write to the journal file."
E_US0029:SS50000_MISC_ERRORS
"The global section containing the page cache is incompatible\n
with this version of the software."
E_US002A:SS50006_FATAL_ERROR
"Couldn't acquire cache lock, either privilege or quota problem."
E_US002B:SS50000_MISC_ERRORS
"Could not open the iidbdb database."
E_US002C:SS5000J_INCONSISTENT_DBMS_CAT
"The iidbdb database is inconsistent; please contact the\n
        system manager"
E_US002D:SS5000B_INTERNAL_ERROR
"An error occurred while closing a database in preparation\n
for opening another one."
E_US002E:SS5000B_INTERNAL_ERROR
"An error occurred in trying to get a lock\n
        while opening a database."
E_US002F:SS5000B_INTERNAL_ERROR
"Could not map global section containing the global cache."
E_US0030:SS50000_MISC_ERRORS
"An error occurred while initializing DMF."
E_US0031:SS5000B_INTERNAL_ERROR
"An error occurred while cleaning up from an error\n
        while opening a database."
E_US0032:SS50000_MISC_ERRORS
"An error occurred while finding the name of the
database directory."
E_US0033:SS5000B_INTERNAL_ERROR
"An error occurred while initializing the database directory."
E_US0034:SS50000_MISC_ERRORS
"An error occurred while resetting the database directory."
E_US0035:SS50000_MISC_ERRORS
"An error occurred while creating the DMF buffer."
E_US0036:SS5000B_INTERNAL_ERROR
"An error occurred while reading and processing the config file\n
        during database initialization."
E_US0037:SS5000B_INTERNAL_ERROR
"An error occurred while initializing the descriptor cache."
E_US0038:SS5000B_INTERNAL_ERROR
"An error occurred while setting a lock on the config file."
E_US0039:SS5000B_INTERNAL_ERROR
"An error occurred while opening the config file."
E_US003A:SS50000_MISC_ERRORS
"An error occurred while initializing the transaction system.\n
         Please contact the system manager"
E_US003B:SS5000B_INTERNAL_ERROR
"An error occurred while re-writing the CONFIG file."
E_US003C:SS5000B_INTERNAL_ERROR
"An error occurred while closing the CONFIG file."
E_US003D:SS5000B_INTERNAL_ERROR
"An error occurred while unlocking the CONFIG file."
E_US003E:SS50000_MISC_ERRORS
"An error occurred while opening the iirelation table.\n
        Please contact the system manager"
E_US003F:SS0A000_FEATUR_NOT_SUPPORTED
"An error occurred while opening the CONFIG table."
E_US0040:SS0A000_FEATUR_NOT_SUPPORTED
"%0c can only be invoked from Application Processor 00."
E_US0041:SS5000J_INCONSISTENT_DBMS_CAT
"The user code of dba in CONFIG header does not match that in iidbdb."
E_US0042_COMPAT_LEVEL:SS50006_FATAL_ERROR
"The major database compatibility level does not match that of the server.
\nBe sure the server is compatible with the requested database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The accessed database's compatibility level does not match the
**	server's major compatibility level.
**
** System Status:
**	The session is terminated.
**
** Recommendation:
**	Convert the database using the appropriate conversion utility.
**
*/
E_US0043_COMPAT_MINOR:SS50006_FATAL_ERROR
"The minor database compatibility level supersedes that of the server.
\nBe sure the server is compatible with the requested database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The accessed database's dbcmptminor is greater than the server's
**	minor compatibility level.
**
** System Status:
**	The session is terminated.
**
** Recommendation:
**	Convert the database using the appropriate conversion utility.
**
*/
E_US0044_CONVERT_DBDB:SS5000J_INCONSISTENT_DBMS_CAT
"The iidbdb database is not compatible with this server.
\nYou must convert the iidbdb database with the UPGRADEDB utility
\nbefore attempting to access any other database in this installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The iidbdb database's config file was not compatible with
**	the server, and the session was attempting to connect to
**	a non-iidbdb database.
**
** System Status:
**	The session is terminated.
**
** Recommendation:
**	Convert the iidbdb database, using the upgradedb utility.
**
*/
E_US0046:SS50004_SYS_CONFIG_ERROR
"The environment variable, %0c_TIMEZONE, is undefined."
E_US0047:SS42506_INVALID_IDENTIFIER
"Database name invalid."
E_US0048:SS50002_LIMIT_EXCEEDED
"Database quota for this server has been exceeded."
E_US0049:SS50000_MISC_ERRORS
"Invalid internal data prevents database access."
E_US0050:SS50000_MISC_ERRORS
"%0c servers may not access %1c databases."
E_US0051:SS5000J_INCONSISTENT_DBMS_CAT
"Database type and server type are incompatible."
E_US0060_CREATEDB_NO_PRIV:SS42000_SYN_OR_ACCESS_ERR
"You do not have enough privilege to create new databases."
E_US0061_USER_EXPIRED:SS42000_SYN_OR_ACCESS_ERR
"Your account has expired. Please contact your system\n
administrator to have your account restored."
E_US0067:SS50004_SYS_CONFIG_ERROR
"Remote node not configured to run %0c/Net."
E_US0068:SS50004_SYS_CONFIG_ERROR
"Local node not configured to run %0c/Net."
E_US006D:SS50004_SYS_CONFIG_ERROR
"Local node not licensed to run %0c/Net."
E_US006E:SS50004_SYS_CONFIG_ERROR
"Remote node not licensed to run %0c/Net."
E_US0070:SS50004_SYS_CONFIG_ERROR
"Local node not licensed to run %0c."
E_US0071:SS50004_SYS_CONFIG_ERROR
"Remote node not licensed to run %0c."
E_US0072:SS5000J_INCONSISTENT_DBMS_CAT
"The IIEXTEND entry for the database is missing.\n
        This can be caused by one of two events.\n
        1) The IIDBDB is corrupted. Run FINDDBS.\n
        2) The database was not build properly.\n
           Destroy it and then recreate it."
E_US0073:SS50000_MISC_ERRORS
"The IIDBDB is unavailable at this time,\n
        please try again later when it has been unlocked.\n
Could not open the iidbdb database."
E_US0080_LOCATION_NOT_FOUND:SS5000H_UNAVAILABLE_RESOURCE
"One of the locations named for the database was not found\n
        or is not available for use."
E_US0081_LOCATION_NOT_ALLOWED:SS50000_MISC_ERRORS
"One of the locations named for the database exists but\n
        the type data to be allocated doesn't match the type of\n
        data allowed in the location.  Use CATALOGDB to check\n
        location names and types."
E_US0082_DATABASE_UPDATE:SS50000_MISC_ERRORS
"Error occurred updating the IIDATABASE table in the IIDBDB database.\n
    Check error log (errlog.log) for more information."
E_US0083_EXTEND_UPDATE:SS50000_MISC_ERRORS
"Error occurred updating the IIEXTEND table in the IIDBDB database.\n
    Check error log (errlog.log) for more information."
E_US0084_DBACCESS_UPDATE:SS50000_MISC_ERRORS
"Error occurred updating the IIDBACCESS table in the IIDBDB database.\n
    Check error log (errlog.log) for more information."
E_US0085_DATABASE_EXIST:SS50000_MISC_ERRORS
"Attempted to create a database that already exists."
E_US0086_DBPRIV_UPDATE:SS5000J_INCONSISTENT_DBMS_CAT
"Error occurred updating the IIDBPRIV table in the IIDBDB database.\n
    Check error log (errlog.log) for more information."
E_US0087_EXTEND_ALREADY:SS50000_MISC_ERRORS
"The database has already been extended to the indicated location."
E_US0088_RAW_AREA_IN_USE:SS50000_MISC_ERRORS
"The location's area has already been extended to another database."
E_US0089_RAW_LOC_IN_USE:SS50000_MISC_ERRORS
"Another database has already been extended to the indicated location."
E_US008A_NOT_EXTENDED:SS50000_MISC_ERRORS
"The database has not been extended to this location."
E_US008B_LOCATION_NOT_EMPTY:SS50000_MISC_ERRORS
"The location cannot be unextended because it contains one or more files."
E_US008C_LOC_WORK_AND_AWORK:SS50000_MISC_ERRORS
"The location cannot be specified for both work and auxiliary work."
E_US008D_LOC_AREA_WORK_AND_AWORK:SS50000_MISC_ERRORS
"The location's area cannot be for both work and auxiliary work."
E_US0090_CREATEDB_RONLY_NOTEXIST:SS50000_MISC_ERRORS
"The CREATEDB statement failed for readonly database as the relevant files
or directories for the readonly database do not exist at location specified.
Make sure that the database directory exists and data files are present at
the data location for the readonly database."
E_US00C8_200:SS42000_SYN_OR_ACCESS_ERR
"Bad parameter format '%0c' in repeat query."
E_US07D1_2001:SS5000A_QUERY_ERROR
"CREATE or ALTER TABLE: number of column names and number of target list 
elements in the subselect do not match."
E_US07D2_2002:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: Bad storage structure '%0c'.\n
The valid storage structure names are [c]btree, [c]isam, [c]hash, [c]heap."
E_US07D3_2003:SS42000_SYN_OR_ACCESS_ERR
"CREATE or ALTER TABLE: invalid option '%0c'. Try NOT NULL WITH DEFAULT."
E_US07D4_2004:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: either invalid attribute qualifier '%0c', wrong order, 
or wrong combination of qualifiers.  Try
'[[not null [with | not default]] | with null] [not | with system_maintained]'."
/*%
** Parameters:
**	%0c - attribute qualifier which caused the error
**
** Explanation:
**	NOTE: errors 2003 and 2004 show syntax for 6.4 and before.
**	      See new syntax errors 4112 and 4113 for post-6.4 versions.
**	
**	The accepted syntax diagram for the column qualifier clause is:
**
**	'[[not null [with | not default]] | with null]
**		[not | with system_maintained]'
**
**	with one exception. It is not allowed to specify either "not default"
**	or "with null" in conjunction with the "with system_maintained" 
**	qualifier.
**
**	The order of these qualifiers is significant, and must appear as
**	described.  The following is a complete list of accepted attribute
**	qualifiers (a few options, although syntactically correct per the 
**	above diagrams, are disallowed due to the "with system_maintained"
**	restrictions):
**
**		with null
**		not null with default
**		not null not default
**		not null with default with system_maintained
**		with null not system_maintained
**		not null with default not system_maintained
**		not null not default not system_maintained
**		with system_maintained
**		not system_maintained
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "CREATE" statement after correcting the problem with
**	the attribute qualifier clause.
*/
E_US07D5_2005:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: '%0c' is either an invalid option or the syntax of\n
        the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_US07D6_2006:SS5000A_QUERY_ERROR
"CREATE TABLE: Location specified twice: '%0c' conflicts with '%1c'."
E_US07D7_2007:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n[NO]DUPLICATES, [NO]JOURNALING, [NO]COMPRESSION."
E_US07D8_2008:SS50002_LIMIT_EXCEEDED
"CREATE or ALTER TABLE: excessive table width on column '%0c'."
E_US07D9_2009:SS5000A_QUERY_ERROR
"'%0c' is a system table."
E_US07DA_2010:SS42502_DUPL_OBJECT
"Duplicate object name '%0c'."
E_US07DB_2011:SS50002_LIMIT_EXCEEDED
"CREATE or ALTER TABLE: the number of columns must not exceed 1024."
E_US07DC_2012:SS42506_INVALID_IDENTIFIER
"CREATE or ALTER TABLE: invalid column name '%0c'."
E_US07DD_2013:SS5000A_QUERY_ERROR
"CREATE TABLE/VIEW: duplicate column name '%0c'."
E_US07DE_2014:SS42000_SYN_OR_ACCESS_ERR
"CREATE or ALTER TABLE: invalid column format '%0c' on column '%1c'."
E_US07DF_2015:SS5000A_QUERY_ERROR
"CREATE TABLE: Too many options in the WITH clause."
E_US07E0_2016:SS5000A_QUERY_ERROR
"CREATE TABLE: Improper use of NULL in the target list;\n
        use conversion function to specify data type of the null value."
E_US07E1_2017:SS5000A_QUERY_ERROR
"CREATE TABLE: Invalid FILLFACTOR of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US07E2_2018:SS5000A_QUERY_ERROR
"CREATE TABLE: Invalid MINPAGES of '%0d' specified. Must be greater than 0."
E_US07E3_2019:SS5000A_QUERY_ERROR
"CREATE TABLE: Invalid MAXPAGES of '%0d' specified. Must be greater than 0."
E_US07E4_2020:SS5000A_QUERY_ERROR
"CREATE TABLE: Invalid NONLEAFFILL of '%0d' specified.
\nMust be in 1 - 100 range (inclusive)."
E_US07E5_2021:SS5000A_QUERY_ERROR
"CREATE TABLE: Invalid LEAFFILL/INDEXFILL of '%0d' specified.
\nMust be in 1 - 100 range (inclusive)."
E_US07E6_2022:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nValid keywords for this type of subclause are: KEY, LOCATION, COMPRESSION."
E_US07E7_2023:SS5000A_QUERY_ERROR
"CREATE TABLE: Number of key columns must not be greater than
the number of columns in the table."
E_US07E8_2024:SS5000A_QUERY_ERROR
"CREATE TABLE: Key column '%0c' is undefined."
E_US07E9_2025:SS5000A_QUERY_ERROR
"CREATE TABLE: Duplicate key column '%0c'."
E_US07EA_2026:SS5000A_QUERY_ERROR
"CREATE TABLE: WITH option '%0c' allowed only in the version of the
\n      CREATE TABLE statement using a subselect."
E_US07EB_2027:SS5000A_QUERY_ERROR
"CREATE TABLE: Illegal MINPAGES value '%0d' in a CREATE TABLE command.
\n      The value must be less than or equal to the MAXPAGES value."
E_US07EC_2028:SS5000A_QUERY_ERROR
"CREATE TABLE: The '%0c' entry in the WITH clause
is incompatible with the storage structure."
E_US07ED_2029:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: '%0c' is either an invalid option or the syntax of the WITH
subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US07EE_2030:SS42000_SYN_OR_ACCESS_ERR
"RETRIEVE/CREATE TABLE: Storage structure type may not be quoted."
E_US07EF_2031:SS5000A_QUERY_ERROR
"%0c: In the with clause, NODE and DATABASE clauses are
\n      optional, but only as a unit."
E_US07F0_2032:SS5000A_QUERY_ERROR
"%0c: duplicate column name '%1c'."
E_US07F1_2033:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: Syntax error on '%0c'.  The correct syntax is:
\n    REGISTER INDEX indexname ON tablename AS LINK [FROM source]"
E_US07F2_2034:SS42000_SYN_OR_ACCESS_ERR
"CREATE: '%0c = %1c' is not allowed by the syntax of the WITH clause."
E_US07F3_2035:SS42000_SYN_OR_ACCESS_ERR
"CREATE: '%0c' is either an invalid option or the syntax of\n
        the WITH subclause is invalid. \n
Valid keywords for this type of subclause are TABLE, NODE, DBMS, and DATABASE."
E_US07F4_2036:SS42000_SYN_OR_ACCESS_ERR
"REGISTER: Syntax error on '%0c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n      AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH"
E_US07F5_2037:SS5000A_QUERY_ERROR
"REGISTER/CREATE LINK: '%0c' is an index.
\n Indices may not be registered in this release."
E_US07F6_2038:SS5000A_QUERY_ERROR
"REGISTER %0c: '%1c' is of type '%2c'.\n
You must use 'REGISTER [%2c]'"
E_US07F7_2039:SS5000A_QUERY_ERROR
"REGISTER/CREATE LINK: You have to specify column names.\n
 Converting to lower case produced duplicate column name '%0c'."
E_US07F8_2040:SS50008_UNSUPPORTED_STMT
"'REGISTER INDEX' in not supported in this release."
E_US07F9_2041:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c' in the KEY option of the WITH clause.
\n Correct syntax is:
\n      WITH ... KEY = (key_name {, key_name})"
E_US07FA_2042:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c' in the LOCATION option of the WITH clause.
\n Correct syntax is:
\n      WITH ... LOCATION = (loc_name {, loc_name})"
E_US07FB_2043:SS5000A_QUERY_ERROR
"REGISTER ... WITH REFRESH: %0c is a view."
E_US07FC_2044:SS5000I_UNEXP_LDB_SCHEMA_CHNG
"REGISTER ... WITH REFRESH: Schema of the LDB object has changed."
E_US07FD_2045:SS50002_LIMIT_EXCEEDED
"An error occurred when attempting to create or alter a table. The specified 
row size exceeded the maximum allowable row width."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to create a table where the width of the row
**	exceeds the maximum allowable row width. The request may be a user
**	request to create a user table or an index, or it may be an internal
**	request to create a temporary table for use in query execution
**	(e.g., temporary tables may be created when evaluating some aggregate
**	queries).
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	If the error occurs creating a user table/index, the user should
**	specify a smaller row width before attempting to create the
**	table/index again. If the error is generated as a result of an
**	internal request to create a temporary table, the user should report
**	the problem to Ingres Corporation Technical Support.
*/
E_US07FE_2046:SS42000_SYN_OR_ACCESS_ERR
"CREATE: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n[NO]DUPLICATES, [NO]JOURNALING, [NO]LOGGING."
E_US07FF_2047:SS42000_SYN_OR_ACCESS_ERR
"CREATE: '%0c' is either an invalid option or the syntax of the
\nWITH subclause is invalid.
\nThe only valid keywords for this type of subclause are LOCATION
\nand PAGE_SIZE."
E_US0800_2048:SS42000_SYN_OR_ACCESS_ERR
"CREATE or ALTER TABLE: national character data types require 
\n'-n' or '-i' option with CREATEDB."
E_US0801_2049:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: you may not modify page_size for a temporary table."
E_US0802_2050:SS42000_SYN_OR_ACCESS_ERR
"%0c: you may only use '%1c' with CREATE INDEX and STRUCTURE=RTREE"
E_US0803_2051:SS5000A_QUERY_ERROR
"CREATE VIEW: number of column names and number of target list elements
\n      in the subselect do not match."
E_US0804_2052:SS42507_RESERVED_IDENTIFIER
"CREATE/DEFINE VIEW: invalid column name '%0c'."
E_US0805_2053_SEQDEF_TYPE:SS42000_SYN_OR_ACCESS_ERR
"Column %0c is not numeric and therefore may not have a sequence value default."
E_US0806_2054:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: you may not modify temporary table with key compression"
/*%
** Parameters:
**	%0c - the column being defined (or altered)
**
** Explanation:
**	A non-numeric column may not use a numeric value as its default.
**	Sequence values are always numeric and so cannot be used as
**	a default for the column in question.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Use a numeric column type.
*/
E_US081B_2075:SS42000_SYN_OR_ACCESS_ERR
"DIRECT DISCONNECT: no parameters may be specified"
E_US081C_2076:SS42000_SYN_OR_ACCESS_ERR
"Syntax error.  Last symbol read was: '%0c'."
E_US081D_2077:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c', 'CONNECT' was expected.\n
The correct syntax is:\n
        DIRECT CONNECT [with_clause]"
E_US081E_2078:SS5000A_QUERY_ERROR
"'DIRECT DISCONNECT' may only be issued if connected to a LDBMS via\n
'DIRECT CONNECT'"
E_US081F_2079:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
        [NODE = [\']node_name[\'],\n
         DATABASE = [\']ldb_name[\']]\n
        [,DBMS = [\']dbms_name[\']]"
E_US0820_2080:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:
\n  WITH
\n      [NODE = [\']node_name[\'], DATABASE = [\']ldb_name[\']]
\n      [,DBMS = [\']dbms_name[\']]
\n
\n  WITH REFRESH"
E_US0823_2083:SS42000_SYN_OR_ACCESS_ERR
"A with_clause is not allowed with DIRECT DISCONNECT command"
E_US0826_2086:SS50008_UNSUPPORTED_STMT
"%0c command is not allowed in non-distributed INGRES."
E_US0827_2087:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
        [NODE = [\']node_name[\'],\n
         DATABASE = [\']ldb_name[\']]\n
        [,DBMS = [\']dbms_name[\']]\n
        [,TABLE =[\']table_name[\']]"
E_US0829_2089:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c', 'CONNECT' was expected.\n
The correct syntax is:\n
 DIRECT CONNECT\n
     [WITH\n
        [NODE = [\"]node_name[\"],\n
         DATABASE = [\"]database_name[\"]]\n
        [,DBMS = [\"]dbms_name[\"]]]"
E_US082A_2090:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
        [NODE = [\"]node_name[\"],\n
         DATABASE = [\"]ldb_name[\"]]\n
        [,DBMS = [\"]dbms_name[\"]]"
E_US082B_2091:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
        [NODE = [\"]node_name[\"],\n
         DATABASE = [\"]ldb_name[\"]]\n
        [,DBMS = [\"]dbms_name[\"]\n
        [,TABLE = [\"]table_name[\"]]"
E_US082C_2092:SS42000_SYN_OR_ACCESS_ERR
"CREATE or ALTER TABLE: Invalid Length '%0d', has been specified for column '%1c' of type '%2c'. Please provide a length value that is within allowable limits for '%3c' datatype."
E_US082D_2093:SS50000_MISC_ERRORS
"The Table/Index could not be created. The maximum Table ID has 
been assigned for this Database."
E_US0834_2100:SS42501_COL_NOT_FOUND
"line %0d, Table '%1c' owned by '%2c' does not contain column '%3c'."
E_US0835_2101:SS5000A_QUERY_ERROR
"line %0d, Column '%1c' found in more than one FROM list table."
E_US0836_2102:SS42501_COL_NOT_FOUND
"line %0d, Column '%1c' not found in any specified table."
E_US0837_2103:SS5000A_QUERY_ERROR
"line %0d, FROM clause is mandatory when a column ('%1c') has been specified
in the target list."
E_US0838_2104:SS5000A_QUERY_ERROR
"line %0d, 'select *' type of subselect requires a FROM clause."
E_US0839_2105:SS5000A_QUERY_ERROR
"line %0d, Column '%1c' must not be specified in this context."
E_US083A_2106:SS42503_INSUF_PRIV
"'%0c' is a secondary index.
\nSecondary indexes cannot be updated directly."
/*%
** Parameters:
**	%0c - index name
**
** Explanation:
**	The user attempted to delete/insert/update a secondary index, but
**	direct update of secondary indexes is disallowed.
**
** System Status:
**	An error is returned for the statement; the transaction is not aborted.
**
** Recommendation:
**	Do not attempt to delete/insert/update a secondary index.
*/
E_US083B_2107:SS42503_INSUF_PRIV
"line %0d, You are not allowed to update '%1c' table."
/*%
** Parameters:
**	%0d - line number
**	%1c - table name
**
** Explanation:
**	The user tried to update a table, but they do not have update
**	permission on this table.
**
** System Status:
**	An error is returned for the statement; the transaction is not aborted.
**
** Recommendation:
**	Only update tables you have update permission on.
*/
E_US083C_2108:SS42500_TBL_NOT_FOUND
"line %0d, Result table '%1c' for APPEND does not
exist or cannot be accessed by you."
E_US083D_2109:SS5000A_QUERY_ERROR
"line %0d, The range variable '%1c' has not been defined."
E_US083E_2110:SS42000_SYN_OR_ACCESS_ERR
"line %0d, The construct '$number = name' may not be used except
\nwhen defining a repeat query.  This error is usually caused by
\nan internal error in the pre-processor.  Please report this error
\nto your vendor if you have not explicitly used this construct yourself."
E_US083F_2111:SS50002_LIMIT_EXCEEDED
"line %0d, Too many columns in key of INDEX."
E_US0840_2112:SS42507_RESERVED_IDENTIFIER
"line %0d, Invalid column name 'tid'."
E_US0841_2113:SS5000A_QUERY_ERROR
"line %0d, Too many entries in argument list to process in this statement."
E_US0842_2114:SS5000A_QUERY_ERROR
"line %0d, Attempt to specify location more than once."
E_US0843_2115:SS5000A_QUERY_ERROR
"line %0d, Too many locations specified."
E_US0844_2116:SS5000A_QUERY_ERROR
"line %0d, Location '%1c' has already been specified."
E_US0845_2117:SS42500_TBL_NOT_FOUND
"Table '%0c' does not exist or is not owned by you."
E_US0846_2118:SS5000A_QUERY_ERROR
"Invalid qualifier '%0c'. '%0c' must be declared in the FROM clause or
equivalent. If used in a target list, '%0c' must be declared at the current
scope."
E_US0847_2119:SS42500_TBL_NOT_FOUND
"User '%0c' does not own table '%1c'."
E_US0848_2120:SS42500_TBL_NOT_FOUND
"The table(s) that the view is dependent upon does not exist."
E_US0849_2121:SS5000A_QUERY_ERROR
"line %0d, Column '%1c' was declared 'for update', but is not
updatable because it is based on a constant or a calculation."
E_US084B_2123:SS5000A_QUERY_ERROR
"line %0d, '.ALL' is not allowed in the target list of a REPLACE command."
E_US084C_2124:SS5000A_QUERY_ERROR
"line %0d, Update of BTREE index '%1c' not allowed."
E_US084D_2125:SS5000A_QUERY_ERROR
"SAVE: You do not own table '%0c'."
E_US084E_2126:SS5000A_QUERY_ERROR
"SAVE: '%0c' is a system table."
E_US084F_2127:SS5000A_QUERY_ERROR
"SAVE: '%0c' is not a base table."
E_US0850_2128:SS42500_TBL_NOT_FOUND
"Table '%1c' owned by '%0c' does not exist
\nor cannot be accessed by you."
E_US0851_2129:SS5000A_QUERY_ERROR
"Invalid mix of syntax modes declaring session table '%0c'.
Please specify all session tables in the same way: either with or
without the 'SESSION.' schema qualification."
/*%
** Parameters:
**	%0c - owner name
**	%1c - table name
**
** Explanation:
**	The user has explicitly referenced a table that is not accessible to
**	them, because it is owned by a user who is none of: the current user,
**	the DBA, or the system catalog owner. This message will not appear
**	after release 6.4.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	Reformulate the query so that it does not explicitly reference tables
**	inaccessible to this user.
*/
E_US0852_2130:SS50002_LIMIT_EXCEEDED
"line %0d, Maximum number of columns exceeded for query."
E_US0853_2131:SS5000A_QUERY_ERROR
"line %0d, Table '%1c' owned by '%2c' is partitioned on column '%3c'. Column\
may not be altered or dropped."
E_US0857_2135:SS42502_DUPL_OBJECT
"line %0d, You cannot duplicate the name of an existing relation '%1c'."
E_US0858_2136:SS5000A_QUERY_ERROR
"line %0d, There is no such hour as '%1d', use a 24 hour clock system."
E_US0859_2137:SS5000A_QUERY_ERROR
"line %0d, There is no such minute as '%1d', use a 24 hour clock system."
E_US085A_2138:SS5000A_QUERY_ERROR
"line %0d, There is no such time as 24:%1d, use a 24 hour clock system."
E_US085B_2139:SS42000_SYN_OR_ACCESS_ERR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_US085C_2140:SS42000_SYN_OR_ACCESS_ERR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US085D_2141:SS5000A_QUERY_ERROR
"You must not specify a WITH clause on a RETRIEVE command
unless it is a RETRIEVE INTO."
E_US085E_2142:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Improper use of NULL in the target list; use conversion
\nfunction to specify data type of the null value."
E_US085F_2143:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Illegal MINPAGES value '%0d' in a RETRIEVE command.
\nThe value must be less or equal to the MAXPAGES value."
E_US0860_2144:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Bad storage structure '%0c'.
\nThe valid storage structure names are [c]btree, [c]isam, [c]hash, [c]heap."
E_US0861_2145:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Invalid FILLFACTOR of '%0d' specified.
\nMust be in 1 - 100 range (inclusive)."
E_US0862_2146:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Invalid MINPAGES of '%0d' specified. Must be greater than 0."
E_US0863_2147:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Invalid MAXPAGES of '%0d' specified. Must be greater than 0."
E_US0864_2148:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Invalid NONLEAFFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US0865_2149:SS5000A_QUERY_ERROR
"RETRIEVE INTO: Invalid LEAFFILL/INDEXFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US0866_2150:SS42000_SYN_OR_ACCESS_ERR
"RETRIEVE INTO: Unrecognized option '%0c' in the WITH clause."
E_US0867_2151:SS5000A_QUERY_ERROR
"RETRIEVE INTO: duplicate key '%0c'."
E_US0868_2152:SS5000A_QUERY_ERROR
"RETRIEVE INTO: The '%0c' entry in the WITH clause
is incompatible with the storage structure."
E_US0869_2153:SS5000H_UNAVAILABLE_RESOURCE
"Repeat query is unknown to server.  Please send the definition"
E_US086A_2154:SS42000_SYN_OR_ACCESS_ERR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n[NO]DUPLICATES, [NO]JOURNALING."
E_US086B_2155:SS42000_SYN_OR_ACCESS_ERR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
\nof the WITH subclause is invalid.
\nValid keywords for this type of subclause are STRUCTURE, NODE, DATABASE,
\n DBMS, and TABLE."
E_US086C_2156:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, 'first n' clause is only permitted on first 'select'
\nof a query, not in subselects, view definitions or union'ed selects."
/*%
** Parameters:
**	none
**
** Explanation:
**	The "first n" clause of the select statement can only be used on the
**	first select of a query. It cannot be used on a subselect (in a 
**	where clause), a union'ed select or the select of a "create view"
**	statement.
**
** System Status:
**	An error is returned for the query.  Any in-progress transactions are
**	not aborted.
**
** Recommendation:
**	Use the "first n" clause according to the defined syntax.
*/
E_US086D_2157:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, number of entries in column list for derived table '%1c'
does not match number of entries in select list."
E_US086E_2158:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, fetch first n rows/offset n clause must specify integer constant,
\ninteger host variable or integer procedure parameter/local variable only."
E_US086F_2159:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, 'select first n' and 'fetch first n rows only' 
\ncoded in same query, only one may be used in a single query."
E_US0870_2160:SS42000_SYN_OR_ACCESS_ERR
"CREATE/DECLARE ... AS SELECT: line %0d, may only have 'order by'
\nin the presence of 'offset n' and/or 'fetch first n' clauses."
E_US0871_2161:SS42501_COL_NOT_FOUND
"Sort column '%0c' not found in target list."
E_US0872_2162:SS42501_COL_NOT_FOUND
"Sort column syntax error in sort expression."
/*%
** Parameters:
**	none
**
** Explanation:
**	Order by clause elements must be either integer constants identifying
**	an entry in the select list of the query, or an expression involving
**	at least one column contained in the tables of the from clause. The
**	permissible expressions may, of course, include a single column
**	reference.
**
** System Status:
**	An error is returned for the query.  Any in-progress transactions are
**	not aborted.
**
** Recommendation:
**	Code the order by list according to the syntax rules.
*/
E_US0873_2163:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Unrecognized sort order '%1c' in a RETRIEVE command.
Please use one of the following:
\n
\n          ascending, a, descending, d"
E_US0874_2164:SS42000_SYN_OR_ACCESS_ERR
"Invalid fetch first n rows value. It must be a positive integer."
E_US0875_2165:SS42000_SYN_OR_ACCESS_ERR
"Invalid offset n value. It must be a non-negative integer."
E_US0876_2166:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, procedure '%1c' in FROM clause must be row producing."
E_US0877_2167:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, recursive call to procedure '%1c' is not permitted."
E_US0878_2168:SS42000_SYN_OR_ACCESS_ERR
"SELECT: line %0d, column '%1c' - retrieval of LOB columns 
\nfrom table procedures is not supported."
E_US0879_2169:SS5000A_QUERY_ERROR
"Invalid name specified for a savepoint."
E_US087A_2170:SS5000A_QUERY_ERROR
"Attempt to abort to a non-existent savepoint."
E_US087B_2171:SS25000_INV_XACT_STATE
"You cannot abort when you are not in a multi-query transaction."
E_US087C_2172:SS25000_INV_XACT_STATE
"You are already in a transaction, try a savepoint."
E_US087D_2173:SS2D000_INV_XACT_TERMINATION
"There is no multi-query transaction to end."
E_US087E_2174:SS25000_INV_XACT_STATE
"You cannot declare a savepoint if you are not in a
multi-query transaction."
E_US0880_2176:SS25000_INV_XACT_STATE
"You cannot modify autocommit during a transaction."
E_US0881_2177:SS25000_INV_XACT_STATE
"No transaction is currently in progress."
E_US0882_2178:SS25000_INV_XACT_STATE
"You cannot declare a savepoint when autocommit is on."
E_US0883_2179:SS5000A_QUERY_ERROR
"Line %0d. CREATE/MODIFY:  Column '%1c' cannot be used as a key field.
The datatype of the field has been restricted from use as a key."
/*%
** Parameters:
**	%0d - line number
**	%1c - column name
**
** Explanation:
**	The column involved has a datatype which is designated as non-keyable.
**
** System Status:
**	An error is returned for the query.  Any in-progress transactions are
**	not aborted.
**
** Recommendation:
**	Either use a keyable datatype for the column, or pick a different key.
*/
E_US0884_2180:SS5000A_QUERY_ERROR
"Line %0d. INDEX: Column '%1c' cannot be used in a secondary index.
The datatype of the column has been restricted from use in indices."
/*%
** Parameters:
**	%0d - line number
**	%1c - column name
**
** Explanation:
**	The column involved has a datatype which is designated as non-keyable,
**	which implicitly disqualifies the column for use in a secondary index.
**
** System Status:
**	An error is returned for the query.  Any in-progress transactions are
**	not aborted.
**
** Recommendation:
**	Either pick a keyable datatype for the column (in the base table),
**	or do not include this column in the secondary index.
*/
E_US0885_2181:SS5000A_QUERY_ERROR
"Column '%0c' cannot be sorted.  The datatype of the column is not sortable
or groupable."
/*%
** Parameters:
**	%0c - column name
**
** Explanation:
**	The column involved has been specified in a "SORT", "ORDER BY"
**	or "GROUP BY" clause, but the datatype for the column is not sortable.
**
** System Status:
**	An error is returned for the query.  No in-progress transactions are
**	aborted.
**
** Recommendation:
**	Do not use the "SORT", "ORDER BY" or "GROUP BY" clause in conjunction 
**	with this column.
*/
E_US0886_2182:SS5000A_QUERY_ERROR
"Column '%0d' cannot be sorted.  The datatype of the column is not sortable
or groupable."
E_US0887_2183:SS5000A_QUERY_ERROR
"UNION: Improper use of NULL in the target list; use conversion
\nfunction to specify data type of the null value."
/*%
** Parameters:
**	%0d - column position number
**
** Explanation:
**	The column involved has been specified in a "SORT", "ORDER BY"
**	or "GROUP BY" clause, but the datatype for the column is not sortable.
**
** System Status:
**	An error is returned for the query.  No in-progress transactions are
**	aborted.
**
** Recommendation:
**	Do not use the "SORT", "ORDER BY" or "GROUP BY" clause in conjunction 
**	with this column.
*/
E_US088E_2190_INACCESSIBLE_TBL:SS42000_SYN_OR_ACCESS_ERR
"%0c: Schema '%1c' does not contain 
\ntable '%2c' or you lack privileges on that table 
\nwhich are required in order to execute this statement."
/*%
** Parameters:
**	%0c - query the user attempted to execute
**	%1c - schema name
**	%2c - table name
**
** Explanation:
**	The table specified in the query does not exist or the user lacks some 
**	required privileges on it.  "Required privileges" vary with the type of 
**	the query. For example: for GRANT/REVOKE, grantor (or PUBLIC) must
**	possess some privilege on the table; for SELECT, user (or PUBLIC) must 
**	possess SELECT privilege on the table, etc.
**
**	SQL92 tries to ensure that a user lacking privileges required to execute
**	a statement should not be able to deduce any information about the 
**	table from the error messages issued by the DBMS, which explains the
**	vagueness of this message.
**
** System Status:
**	If processing GRANT or REVOKE, the DBMS will attempt to process the
**	next object, if any, named in the statement (this is our extension,
**	as is the ability to name multiple objects in one statement).  For
**	other statements, the statement will be rolled back.
**
** Recommendation:
**	Verify that the table or view you specify in the statement exists and 
**	that you possess all privileges required to execute the statement.
*/
E_US088F_2191_INACCESSIBLE_EVENT:SS42000_SYN_OR_ACCESS_ERR
"%0c: Schema '%1c' does not contain 
\ndbevent '%2c' or you lack privileges on that dbevent
\nwhich are required in order to execute this statement."
/*%
** Parameters:
**	%0c - query the user attempted to execute
**	%1c - schema name
**	%2c - dbevent name
**
** Explanation:
**	The dbevent specified in the query does not exist or the user lacks
**	some required privileges on it.  "Required privileges" vary with the
**	type of query. For example: for GRANT/REVOKE, grantor (or PUBLIC) must 
**	possess some privilege on the dbevent; for RAISE, user (or PUBLIC) must 
**	possess RAISE privilege on the dbevent, etc.
**
**	SQL92 tries to ensure that a user lacking privileges required to execute
**	a statement should not be able to deduce any information about the 
**	object from the error messages issued by the DBMS, which explains the
**	vagueness of this message.
**
** System Status:
**	If processing GRANT or REVOKE, the DBMS will attempt to process the
**	next object, if any, named in the statement (this is our extension,
**	as is the ability to name multiple objects in one statement).  For
**	other statements, the statement will be rolled back.
**
** Recommendation:
**	Verify that the dbevent you specify in the statement exists and 
**	that you possess all privileges required to execute the statement.
*/
E_US0890_2192_INACCESSIBLE_DBP:SS42000_SYN_OR_ACCESS_ERR
"%0c: Schema '%1c' does not contain 
\ndatabase procedure '%2c' or you lack privileges 
\non that database procedure which are required in order to 
\nexecute this statement."
/*%
** Parameters:
**	%0c - query the user attempted to execute
**	%1c - schema name
**	%2c - database procedure name
**
** Explanation:
**	The database procedure specified in the query does not exist or the
**	user lacks some required privileges on it.  "Required privileges" vary
**	with the type of query. For example: for GRANT/REVOKE, grantor (or
**	PUBLIC) must possess some privilege on the database procedure; for
**	EXECUTE, user (or PUBLIC) must possess EXECUTE privilege on the
**	database procedure, etc.
**
**	SQL92 tries to ensure that a user lacking privileges required to execute
**	a statement should not be able to deduce any information about the 
**	object from the error messages issued by the DBMS, which explains the
**	vagueness of this message.
**
** System Status:
**	If processing GRANT or REVOKE, the DBMS will attempt to process the
**	next object, if any, named in the statement (this is our extension,
**	as is the ability to name multiple objects in one statement). For
**	other statements, the statement will be rolled back.
**
** Recommendation:
**	Verify that the database procedure you specify in the statement exists 
**	and that you possess all privileges required to execute the statement.
*/
E_US0891_2193_INACCESSIBLE_SEQ:SS42000_SYN_OR_ACCESS_ERR
"%0c: Schema '%1c' does not contain 
\nsequence '%2c' or you lack privileges on that sequence
\nwhich are required in order to execute this statement."
/*%
* Parameters:
**	%0c - query the user attempted to execute
**	%1c - schema name
**	%2c - sequence name
**
** Explanation:
**	The sequence specified in the query does not exist or the user lacks
**	some required privileges on it.  "Required privileges" vary with the
**	type of query. For example: for GRANT/REVOKE, grantor (or PUBLIC) must 
**	possess some privilege on the sequence; for RAISE, user (or PUBLIC) must 
**	possess RAISE privilege on the sequence, etc.
**
**	SQL92 tries to ensure that a user lacking privileges required to execute
**	a statement should not be able to deduce any information about the 
**	object from the error messages issued by the DBMS, which explains the
**	vagueness of this message.
**
** System Status:
**	If processing GRANT or REVOKE, the DBMS will attempt to process the
**	next object, if any, named in the statement (this is our extension,
**	as is the ability to name multiple objects in one statement).  For
**	other statements, the statement will be rolled back.
**
** Recommendation:
**	Verify that the sequence you specify in the statement exists and 
**	that you possess all privileges required to execute the statement.
*/
E_US0892_2194:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Cannot use OLD version of large object (%1c.%2c.%3c) in AFTER rule."
E_US0898_2200:SS5000A_QUERY_ERROR
"line %0d.  If an update column list in cursor '%1c'
\nis not specified, the FOR UPDATE clause must not contain
\nDIRECT or DEFERRED keywords."
E_US0899_2201:SS24000_INV_CURS_STATE
"line %0d, Cursor '%1c' already open."
E_US089A_2202:SS5000A_QUERY_ERROR
"line %0d, Column '%2c' in cursor '%1c' is not updatable
\n      because it is based on a constant or a calculation."
E_US089B_2203:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the cursor
\n      definition contains none or more than one range variable."
E_US089C_2204:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the cursor
\n      definition contains a 'sort by' or 'unique' clause."
E_US089D_2205:SS24000_INV_CURS_STATE
"line %0d, Cursor '%1c' is not currently open."
E_US089E_2206:SS5000A_QUERY_ERROR
"line %0d, Column '%2c' does not exist in cursor '%1c'."
E_US089F_2207:SS5000A_QUERY_ERROR
"line %0d, Column '%2c' in cursor '%1c' is not updatable."
E_US08A0_2208:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' was declared 'for update', but uniqueness was
specified in the retrieve statement."
E_US08A1_2209:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' was declared with a 'retrieve into' statement.
No result table is allowed with cursors."
E_US08A2_2210:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is already defined."
E_US08A3_2211:SS42503_INSUF_PRIV
"line %0d, Cursor '%1c' does not have delete permission."
E_US08A4_2212:SS5000A_QUERY_ERROR
"line %0d, Range variables are not allowed in REPLACE CURSOR statements."
E_US08A5_2213:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' was declared 'for update', but it is
not updatable because the cursor definition contains an
aggregate with a BY list."
E_US08A6_2214:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' was declared 'for update', but it is
not updatable because the cursor definition contains an
aggregate function with a GROUP BY clause."
E_US08A7_2215:SS5000A_QUERY_ERROR
"Cursor has been declared for table '%0c' not '%1c'."
E_US08A8_2216:SS5000A_QUERY_ERROR
"Repeat queries cannot use 'where current of' clause."
E_US08A9_2217:SS24000_INV_CURS_STATE
"Cursor '%0c' has not been opened for update."
E_US08AA_2218:SS5000A_QUERY_ERROR
"The parameter in the target list of the repeat query is not allowed
in this context."
E_US08AB_2219:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because of implied request
to remove duplicates."
E_US08AC_2220:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because of implied UNION."
E_US08AD_2221:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the specified variable
is a view, which contains either DISTINCT or UNION in its definition."
E_US08AE_2222:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the specified variable
is an index."
E_US08AF_2223:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because resulting query has
an aggregate in its target list."
E_US08B0_2224:SS5000A_QUERY_ERROR
"line %0d, 'for update' clause in cursor '%1c' contains nonupdatable
column '%2c'."
E_US08B1_2225:SS5000A_QUERY_ERROR
"Updatable cursor '%0c' defined on an nonupdatable view '%1c'."
E_US08B2_2226:SS5000A_QUERY_ERROR
"Ambiguous dynamically specified cursor name '%0c' in a DELETE or UPDATE statement."
/*%
** Parameters:
**	%0c - cursor name
**
** Explanation:
**	The dynamically specified cursor name is not unique among currently
**	open cursors.  It may be unique within the original source module,
**	but it must also be unique within the current transaction.
**
** System Status:
**	The "UPDATE/DELETE" statement is ignored; the cursor is left open.
**
** Recommendation:
**	Modify the original "DECLARE CURSOR" statement to specify a unique
**	cursor name if a dynamic "DELETE/UPDATE" statement will be issued.
**	This restriction does not apply to static cursor "DELETE/UPDATE"
**	statements.
*/
E_US08B3_2227:SS5000A_QUERY_ERROR
"Cursor has been declared for table '%0c' owned by '%1c'
\nnot for '%2c' owned by '%3c'."
/*%
** Parameters:
**	%0c - table name as it appeared in DECLARE CURSOR
**	%1c - table owner as determined in DECLARE CURSOR
**	%2c - table name as specified in DELETE/UPDATE ... WHERE CURRENT OF...
**	%3c - table owner as determined in DELETE/UPDATE ... WHERE CURRENT OF...
**
** Explanation:
**	The name of the table and/or table owner as they were determined
**	(note that owner name is optional) in "UPDATE/DELETE ... WHERE CURRENT
**	OF ..." are different from those determined in "DECLARE CURSOR".
**
** System Status:
**	The "UPDATE/DELETE" statement is ignored; the cursor is left open.
**
** Recommendation:
**	Modify the "UPDATE/DELETE" statement to make sure that the names of
**	the table and table owner are the same as they were determined in
**	"DECLARE CURSOR".
**
**	It is possible that the table which existed when "DECLARE CURSOR" was
**	executed has since been dropped, in which case the user may consider
**	some corrective action (e.g., redeclare the cursor to use an existing
**	table).
*/
E_US08B4_2228:SS25000_INV_XACT_STATE
"No MST is currently in progress, cannot declare another cursor."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Another cursor cannot be opened when no MST is in progress. The
**	SQL user may have autocommit ON, which does not allow nested cursors.
**
** System Status:
**	An error is returned for the query.
**
** Recommendation:
**	For SQL, turn autocommit OFF;  QUEL users should put nested cursors
**	inside an MST.
*/
E_US08B5_2229:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' cannot be KEYSET because it is non-updateable."
/*%
** Parameters:
**	%0c - statement name
**	%1c - cursor name
**
** Explanation:
**	A non-updateable cursor was declared SCROLL KEYSET and this is not 
**	permitted. The accompanying message describes why the cursor is
**	considered non-updateable.
**
** System Status:
**	An error is returned for the query.
**
** Recommendation:
**	Modify the cursor to be updateable, or remove the KEYSET option.
*/
E_US08B6_2230:SS5000A_QUERY_ERROR
"Line %0d, Scrollable cursor cannot be declared FOR DEFERRED UPDATE."
/*%
** Parameters:
**	%0c - statement number
**
** Explanation:
**	A SCROLL cursor was declared FOR DEFERRED UPDATE. Updateable 
**	scrollable cursors cannot use DEFERRED update.
**
** System Status:
**	An error is returned for the query.
**
** Recommendation:
**	Modify the cursor to be FOR DIRECT UPDATE or just remove the 
**	DEFERRED option.
*/
E_US08B7_2231:SS5000A_QUERY_ERROR
"Line %0d, Select list of scrollable cursor '%1c' cannot contain LOB result column/expression."
/*%
** Parameters:
**	%0c - statement number
**	%1c - cursor name
**
** Explanation:
**	A SCROLL cursor contained a result column/expression of LOB type
**	(CLOB, BLOB or NCLOB). This is not permitted, though LOB locators
**	are acceptible.
**
** System Status:
**	An error is returned for the query.
**
** Recommendation:
**	Remove the LOB column/expression from the select list, or change
**	it to use a LOB locator.
*/
E_US08B8_2232:SS5000A_QUERY_ERROR
"Line %0d, Scrollable updateable cursor '%1c' is not supported in Star database."
/*%
** Parameters:
**	%0c - statement number
**	%1c - cursor name
**
** Explanation:
**	A SCROLL cursor with update/keyset options cannot be coded on a 
**	Star database.
**
** System Status:
**	An error is returned for the query.
**
** Recommendation:
**	Remove update/keyset option or change to non-scrollable cursor.
*/
E_US08B9_2233:SS5000A_QUERY_ERROR
"line %0d, Column '%1c' not found in FROM list tables on both sides of UNION."
E_US08FC_2300:SS5000A_QUERY_ERROR
"line %0d, Argument(s) missing."
E_US08FD_2301:SS5000A_QUERY_ERROR
"line %0d, Expecting more arguments."
E_US08FE_2302:SS5000A_QUERY_ERROR
"line %0d, The dynamically defined statement '%1c' is either a SELECT statement
that cannot be EXECUTED, or a non-SELECT statement that cannot be OPENED."
E_US08FF_2303:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Invalid keyword '%1c'. Only NAMES option allowed."
E_US0900_2304:SS26000_INV_SQL_STMT_NAME
"line %0d, The dynamically defined statement '%1c' not found.\n
Perhaps a PREPARE or DESCRIBE wasn't successful."
E_US0901_2305:SS5000A_QUERY_ERROR
"line %0d, The dynamically defined statement '%1c' is not a SELECT statement."
E_US0902_2306:SS5000A_QUERY_ERROR
"line %0d, Parameter markers not allowed in this statement."
E_US0903_2307:SS5000A_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the table being selected does
not have a unique primary key or index that is included in the result set."
E_US0906_2310:SS24000_INV_CURS_STATE
"Invalid attempt to execute Dynamic SQL statement '%0c'.\n
The statement refers to cursor '%1c' that is already closed."
/*%
** Parameters:
**	%0c - statement name
**	%1c - cursor name
**
** Explanation:
**	The statement was prepared from a cursor statement while the specified
**	cursor was open.  By the time the statement was executed, the cursor
**	was already closed.
**
** System Status:
**	The "EXECUTE" statement is ignored.
**
** Recommendation:
**	Only issue this statement while the cursor is open.  This statement
**	may not apply to the same named cursor that is opened twice.  You
**	must reprepare the statement to associate it with the correct cursor.
*/
E_US0915_2325:SS50008_UNSUPPORTED_STMT
"The GRANT statement is not allowed in a distributed INGRES."
E_US0916_2326:SS42503_INSUF_PRIV
"GRANT/REVOKE: only the DBA can use the GRANT and REVOKE statements."
E_US0917_2327:SS42503_INSUF_PRIV
"GRANT: Table '%0c' is not owned by you."
E_US0918_2328:SS5000A_QUERY_ERROR
"GRANT: Object ('%0c') must not be an index."
E_US0919_2329:SS5000A_QUERY_ERROR
"GRANT: Only EXECUTE privilege may be granted on a DB procedure."
E_US091A_2330:SS5000A_QUERY_ERROR
"GRANT: EXECUTE privilege may not be granted on a table."
E_US091B_2331:SS01000_WARNING
"line %0d, Warning: creating non GRANT compatible permit on table '%1c'."
E_US091C_2332:SS42503_INSUF_PRIV
"CREATE SECURITY_ALARM: Only the DBA can use the CREATE SECURITY_ALARM
statement"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "CREATE SECURITY_ALARM" statement is restricted in B1 environments
**	to DBAs only, until name-space issues are resolved in front-ends.
**
** System Status:
**	The statement is rejected.
**
** Recommendation:
**	Contact the DBA to assume ownership of the table.
*/
E_US092E_2350:SS42000_SYN_OR_ACCESS_ERR
"EXECUTE IMMEDIATE: a SELECT statement cannot be a target of this command."
E_US0960_2400:SS42502_DUPL_OBJECT
"Procedure '%0c' already exists."
E_US0961_2401:SS5000A_QUERY_ERROR
"Parameter/local variable '%0c' already exists."
E_US0962_2402:SS42000_SYN_OR_ACCESS_ERR
"Invalid column format '%0c'."
E_US0963_2403:SS5000A_QUERY_ERROR
"Parameter/local variable '%0c' has not been declared."
E_US0964_2404:SS42503_INSUF_PRIV
"You are not the owner of '%0c' procedure."
E_US0965_2405:SS42505_OBJ_NOT_FOUND
"Procedure '%0c' not found."
E_US0966_2406:SS5000A_QUERY_ERROR
"Label '%0c' already declared."
E_US0967_2407:SS5000A_QUERY_ERROR
"Label '%0c' not declared or out of scope."
E_US0968_2408:SS5000A_QUERY_ERROR
"MESSAGE statement requires first parameter to be of INTEGER type."
E_US0969_2409:SS5000A_QUERY_ERROR
"MESSAGE statement requires second parameter to be of character type."
E_US096A_2410:SS5000A_QUERY_ERROR
"MESSAGE statement can only be followed by a parameter of integer
or character type."
E_US096B_2411:SS50008_UNSUPPORTED_STMT
"This statement has not been implemented yet."
E_US096C_2412:SS5000A_QUERY_ERROR
"line %0d, The outermost target list of the first subselect in a SELECT
statement in a database procedure must retrieve data into local variables/parameters.
Target list element '%1c' violates this rule."
E_US096D_2413:SS5000A_QUERY_ERROR
"Local variable/parameter '%0c' has not been declared or is out of scope."
E_US096E_2414:SS5000A_QUERY_ERROR
"Local variable/parameter ':%0c' can only be used within a database procedure."
E_US096F_2415:SS5000A_QUERY_ERROR
"Line %0d: This type of predicate is not allowed in a condition of a database
procedure control statement."
E_US0970_2416:SS50008_UNSUPPORTED_STMT
"The assignment statement is not supported yet. Use SELECT statement
instead."
E_US0971_2417:SS5000A_QUERY_ERROR
"Target list expression of type '%0c' cannot be coerced into local
variable/parameter of type '%1c'."
E_US0972_2418:SS5000A_QUERY_ERROR
"A target list with the INTO clause may be used only in the outermost
level of a SELECT statement."
E_US0973_2419:SS5000A_QUERY_ERROR
"A SELECT statement with the INTO clause in the target list must not be
used outside of a database procedure context."
E_US0974_2420:SS50008_UNSUPPORTED_STMT
"Nesting of BEGIN END blocks is not supported yet."
E_US0975_2421:SS5000A_QUERY_ERROR
"Return value must be of non-nullable data type."
E_US0976_2422:SS5000A_QUERY_ERROR
"Message statement parameters must be of non-nullable data type."
E_US0977_2423:SS50002_LIMIT_EXCEEDED
"Number of arguments passed to a procedure or columns defined 
in a result row must not exceed %0d."
E_US0978_2424:SS5000A_QUERY_ERROR
"Return value must be of INTEGER type."
E_US0979_2425:SS5000A_QUERY_ERROR
"Empty statement blocks are not allowed."
E_US097A_2426:SS5000A_QUERY_ERROR
"'ENDLOOP [label]' statement is only allowed in a WHILE."
E_US097B_2427:SS5000A_QUERY_ERROR
"INTO clause: number of elements in the parameter/local variable list
doesn't match the number of elements in the target list."
E_US097C_2428:SS5000A_QUERY_ERROR
"INTO clause must appear only once in the SELECT statement in
the outermost target list of the first subselect."
E_US097D_2429:SS5000A_QUERY_ERROR
"line %0d, INTO clause must not be mixed with '[:]loc_var_name = expression'
way of assigning values to local variables."
E_US097E_2430:SS5000A_QUERY_ERROR
"line %0d, The outermost target lists of all subselects other than the first one in a SELECT
with UNION statement in a database procedure must not retrieve data into local variables/parameters.
Target list element '%1c' violates this rule."
E_US097F_2431:SS5000A_QUERY_ERROR
"line %0d, Local variables/parameters can appear only in the SELECT statement
in the outermost target list of the first subselect."
E_US0980_2432:SS42507_RESERVED_IDENTIFIER
"`%0c' is not a valid database procedure name.
\n Names starting with '%0c' are reserved for system use."
E_US0981_2433:SS42507_RESERVED_IDENTIFIER
"`%0c' is not a valid parameter/local variable name. Name must not start with `%1c'."
E_US0982_2434:SS5000A_QUERY_ERROR
"line %0d, IIROWCOUNT and IIERRORNUMBER built-in variables are read-only."
E_US0983_2435:SS50008_UNSUPPORTED_STMT
"SELECT..UNION..SELECT is not supported in this release of STAR."
E_US0984_2436_RSER_TYPE:SS5000A_QUERY_ERROR
"RAISE ERROR parameters must be of type integer and character respectively."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The parameters to the "RAISE ERROR" procedure statement must be a
**	non-nullable integer error number, optionally followed by a
**	non-nullable error message string.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Redeclare your "RAISE ERROR" parameters as the correct datatypes and
**	with the NOT NULL attribute.
*/
E_US0985_2437_RSER_NUL:SS5000A_QUERY_ERROR
"RAISE ERROR statement parameters must be of a non-nullable data type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The parameters to the "RAISE ERROR" procedure statement must be a
**	non-nullable integer error number, optionally followed by a
**	non-nullable error message string.
**
** System Status:
**	Program continues and current statement is aborted.
**
** Recommendation:
**	Redeclare your "RAISE ERROR" parameters with the NOT NULL attribute.
*/
E_US0986_2438_EXEC_COL:SS42000_SYN_OR_ACCESS_ERR
"Invalid correlation or column name '%0c' in EXECUTE PROCEDURE statement.
Column expressions of the form [table.]column are not allowed in the
target list."
/*%
** Parameters:
**	%0c - column name or correlation name
**
** Explanation:
**	Values in the "EXECUTE PROCEDURE" target list may be literal values,
**	some symbolic constants (such as USER and NULL) and, when the
**	statement is nested within a database procedure, a local variable
**	or parameter.  A value such as:
**
**	        EXECUTE PROCEDURE p (a = table.col)
**
**	is invalid as the meaning is ambiguous (is this a full column
**	aggregate or the column value of "some current row".
**
** System Status:
**	The current statement is aborted.  If the statement is nested within
**	a database procedure, the procedure creation is aborted.
**
** Recommendation:
**	Correct the value usage.
*/
E_US0987_2439_EXEC_TYPE:SS5000A_QUERY_ERROR
"The data type of the EXECUTE PROCEDURE result variable '%0c' must be INTEGER."
/*%
** Parameters:
**	%0c - result variable name
**
** Explanation:
**	The result datatype of any procedure is INTEGER.  All variables
**	and parameters that are to retrieve the result of a procedure (via
**	the "result_variable =" clause) must be declared as INTEGER.
**
** System Status:
**	The current statement is aborted.  If the statement is nested within
**	a database procedure, the procedure creation is aborted.
**
** Recommendation:
**	Correct the datatype of the specified variable or parameter.
*/
E_US0988_2440_SEL_CNTA_FROM:SS5000A_QUERY_ERROR
"line %0d, SELECT COUNT(*) specified without required FROM clause."
E_US0989_2441_TTAB_PARM:SS5000A_QUERY_ERROR
"A temporary table dbprocedure parameter must be the ONLY parameter in
an EXECUTE PROCEDURE statement."
E_US098A_2442_RULE_PROC:SS50000_MISC_ERRORS
"Row level rule is calling SET OF procedure '%0c'. Must be statement level rule."
/*%
** Parameters:
**      %0c - procedure name.
**
** Explanation:
**      A row level rule triggers the invocation of a procedure with a
**      SET OF parameter. Only statement level rules may invoke SET OF
**      procedures.
**
** System Status:
**      PSF will reject the query.
**
** Recommendation:
**      Either the rule must be changed to be statement level, or the
**      procedure must be recoded to use a simple (non-SET OF) parameter
**      list.
*/
E_US098B_2443_FOR_DML:SS5000A_QUERY_ERROR
"A FOR statement must begin with a SELECT statement."
E_US098C_2444_FOR_NONEST:SS5000A_QUERY_ERROR
"FOR statements may NOT be nested."
E_US098D_2445_RETROW_NO_RESROW:SS5000A_QUERY_ERROR
"A RETURN ROW statement can only be issued from a database procedure with
a corresponding RESULT ROW declaration."
E_US098E_2446_RET_RESROW_MATCH:SS5000A_QUERY_ERROR
"The number of entries in a RETURN ROW list must match the number of 
entries in the corresponding RESULT ROW declaration."
E_US098F_2447_ROWPROC_NODYN:SS5000A_QUERY_ERROR
"Row producing procedure '%0c' cannot be called using dynamic SQL 
or from within another procedure."
/*%
** Parameters:
**      %0c - procedure name.
**
** Explanation:
**      A row producing procedure was executed using dynamic SQL or by
**	a nested call from within another procedure. Row producing
**	procedures may only be called from a ESQL program using a 
**	static EXECUTE PROCEDURE statement.
**
** System Status:
**      QEF will reject the query.
**
*/
E_US0990_2448_POSPARM_ERR:SS42000_SYN_OR_ACCESS_ERR
"EXECUTE PROCEDURE: positional parameters must not follow named parameters."
/*%
** Parameters:
**      None
**
** Explanation:
**      A procedure invocation had a parameter list in which nameless
**	positional parameters followed named parameters. All nameless
**	parameters must precede any named parameter.
**
** System Status:
**      QEF will reject the query.
**
*/
E_US0991_2449_TPROC_DML_ERR:SS5000A_QUERY_ERROR
"Table procedure '%0c' cannot contain DML operation."
/*%
** Parameters:
**      %0c - procedure name.
**
** Explanation:
**      A table procedure is not allowed to contain DML
**      statements (INSERT/DELETE/UPDATE).
**
** System Status:
**      PSF will reject the query.
**
*/
E_US09C4_2500:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on line %0d.  Last symbol read was: '%1c'."
E_US09C5_2501:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SELECT [ALL|DISTINCT] target_list
\n  FROM table(s)
\n  [WHERE search_cond]
\n  [GROUP BY col(s)]
\n  [HAVING search_cond]
\n [UNION subselect]
\n [ORDER BY col(s)]"
E_US09C6_2502:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE TABLE tablename
\n   [(columnname {, ... })]
\n   AS subselect
\n   [with_clause]"
E_US09C7_2503:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n INSERT INTO tablename [(column {, ... })]
\n   [OVERRIDING {SYSTEM | USER} VALUE] VALUES (expression {, ... })
\n
\n INSERT INTO tablename [(column {, ... })] 
\n   [OVERRIDING {SYSTEM | USER} VALUE] subselect"
E_US09C8_2504:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n UPDATE table [alias]
\n   [FROM table {, ... }]
\n   SET [alias.]column = expression|NULL {, ... }
\n   [WHERE search_condition]
\n or:
\n UPDATE table [alias]
\n   [FROM table {, ... }]
\n   SET ([alias.]column,[alias.]column,...) = (SELECT expr,expr,... FROM ... )
\n   [WHERE search_condition]"
E_US09C9_2505:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DELETE FROM tablename [WHERE search_condition]"
E_US09CA_2506:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n COPY TABLE tablename (columnname = format [null_clause] {, ... })
\n   INTO|FROM 'filename'
\n   [with_clause]"
E_US09CB_2507:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE TABLE tablename
\n   (columnname format [null_default_clause] {, ... })
\n   [with_clause]
\n
\n CREATE TABLE tablename [(colname_list)]
\n   AS subselect
\n   [with_clause]"
/*%
** Parameters:
**	%0d - line number of error
**	%1c - token causing error
**
** Explanation:
**	A syntax error occurred while parsing column qualifications
**	in a "CREATE TABLE" statement (SQL).
**
**	For post-6.4 local SQL error, see error 3855 (0x0F0F).
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax and retry.
*/
E_US09CC_2508:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP [TABLE|VIEW|INDEX|LINK|USER] object_name {, object_name}"
E_US09CE_2510:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE [UNIQUE] INDEX (indexname 
\n   ON table (column [ASC|DESC] {, ... })
\n   [with_clause]) {, ...} or
\n CREATE INDEX (indexname on table (column [ASC|DESC] {, ... }) [UNIQUE]
\n	[with_clause]) {, ...}"
E_US09CF_2511:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n MODIFY table TO storage-structure [UNIQUE]
\n   [ON column [ASC|DESC] {, ... }]
\n   [with_clause]"
E_US09D0_2512_INVALID_CAST_TYPE:SS42000_SYN_OR_ACCESS_ERR
"Syntax error in CAST: '%0c' is not a valid type name"
/*%
** Parameters:
**	%0c - type name
**
** Explanation:
**	An invalid type name was given in a CAST(expr AS type) expression.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax and retry.
*/
E_US09D2_2514:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SAVE tablename UNTIL month day year"
E_US09D3_2515:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n OPEN CURSORID CURSOR FOR fullselect
\n   [FOR [DIRECT | DEFERRED] UPDATE OF column {, ... }]
\n   [FOR READONLY]"
E_US09D5_2517:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE VIEW viewname [ (column {, ... }) ]
\n   AS subselect [WITH CHECK OPTION]"
E_US09D6_2518:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ABORT [TO savepoint_name]"
E_US09D7_2519:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE PERMIT SELECT|UPDATE|DELETE|INSERT
\n   ON|OF|TO table [ (column(s)) ]
\n   TO user_name [AT term] [FROM time TO time]
\n   [ON day TO day] [WHERE search_cond]
\n
\n CREATE PERMIT ALL ..."
E_US09D8_2520:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE INTEGRITY ON tablename IS search_condition"
E_US09D9_2521:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CLOSE cursor_name"
E_US09DA_2522:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n RELOCATE tablename TO locationname"
E_US09DB_2523:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DEFINE QUERY queryname IS delete|select|update
\n
\n DEFINE QUERY open_cursor"
E_US09DC_2524:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n EXECUTE stmnt_name USING const {, ... }"
E_US09DE_2526:SS42000_SYN_OR_ACCESS_ERR
"line %0d Syntax error on '%1c'.  The correct syntax is:
\n BEGIN TRANSACTION"
E_US09DF_2527:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n END TRANSACTION"
E_US09E0_2528:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SAVEPOINT savepoint_name"
E_US09E1_2529:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ABORT [TO savepoint_name]"
E_US09E2_2530:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP INTEGRITY ON table integer {, ... }
\n
\n DROP INTEGRITY ON table ALL"
E_US09E3_2531:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP PERMIT ON table integer {, ... }
\n
\n DROP PERMIT ON table ALL"
E_US09E5_2533:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    CREATE DBEVENT dbevent_name"
E_US09E6_2534:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n EXECUTE stmnt_name USING const {, ... }"
E_US09E7_2535:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n [result_variable =] EXECUTE PROCEDURE | CALLPROC proc_name
\n        [(parameter = value, ...)]"
E_US09E8_2536:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    DROP DBEVENT dbevent_name"
E_US09E9_2537:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    REGISTER DBEVENT [dbevent_owner.]dbevent_name"
E_US09EA_2538:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    REMOVE DBEVENT [dbevent_owner.]dbevent_name"
E_US09EB_2539:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
  RAISE DBEVENT [dbevent_owner.]dbevent_name  [dbevent_text] [WITH [NO]SHARE]"
E_US09EC_2540:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n UPDATE table [alias]
\n   SET column = expression|NULL {, ... }
\n   WHERE CURRENT OF cursor_name"
E_US09ED_2541:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n FETCH CURSOR cursor_name"
E_US09F3_2547:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET CPUFACTOR integer"
E_US09F4_2548:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET DATE_FORMAT \"us\" | \"multinational\"
\n   | \"iso\" | \"finland\" | \"sweden\" | \"german\""
E_US09F6_2550:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET DECIMAL \".\" | \",\""
E_US09F7_2551:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET WORK LOCATIONS
\n    ADD | DROP | USE
\n        ( work_location {, work_location} )"
E_US09F8_2552:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET JOINOP [NO]TIMEOUT [milliseconds]"
E_US09F9_2553:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]JOURNALING [ON tablename]"
E_US09FA_2554:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET MONEY_FORMAT \"T|L:string\""
E_US09FB_2555:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET MONEY_PREC integer"
E_US09FE_2558:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET RESULT_STRUCTURE
\n   \"heap | cheap | heapsort | cheapsort | hash
\n   | chash | isam | cisam | btree | cbtree\""
E_US0A00_2560:SS5000A_QUERY_ERROR
"line %0d, If a table reference is used to qualify a SET column,
it must refer to the update table. This SET clause uses '%1c' which is
a reference to a table in the FROM clause."
E_US0A01_2561:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]STATISTICS"
E_US0A02_2562:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\nSET LOCKMODE SESSION|ON table
\n WHERE [LEVEL=level_option]
\n [,READLOCK=readlock_option]
\n [,MAXLOCKS=maxlock_option]
\n [,TIMEOUT=timeout_option]"
E_US0A03_2563:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT DMnnnn [integer {, integer }]"
E_US0A04_2564:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT PSnnnn [integer {, integer }]"
E_US0A05_2565:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT SCnnnn [integer {, integer }]"
E_US0A06_2566:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT RDnnnn [integer {, integer }]"
E_US0A07_2567:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT QEnnnn [integer {, integer }]"
E_US0A08_2568:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT OPnnnn [integer {, integer }]"
E_US0A09_2569:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT QSnnnn [integer {, integer }]"
E_US0A0A_2570:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT ADnnnn [integer {, integer }]"
E_US0A0B_2571:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE ..."
E_US0A0D_2573:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]QEP"
E_US0A0E_2574:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n COMMIT [WORK]"
E_US0A0F_2575:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DESCRIBE stmnt_name [USING NAMES]"
E_US0A10_2576:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n PREPARE stmnt_name [INTO name] FROM statement_text"
E_US0A11_2577:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n PREPARE stmnt_name [INTO name] FROM statement_text"
E_US0A12_2578:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ROLLBACK [WORK] [TO savepoint]"
E_US0A13_2579:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET AUTOCOMMIT ON|OFF"
E_US0A14_2580:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ROLLBACK [WORK] [TO savepoint]"
E_US0A15_2581:SS50008_UNSUPPORTED_STMT
"line %0d, Syntax error on '%1c'.
\n This SET statement is not supported."
E_US0A16_2582:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET JOINOP [NO]TIMEOUT [milliseconds]"
E_US0A18_2584:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n GRANT ALL|privilege(s) ON [obj_type] object(s)
\n TO {PUBLIC| [grantee_type] grantee(s)}
\n privilege: SELECT|INSERT|DELETE|UPDATE[(col(s))]|EXECUTE."
E_US0A19_2585:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]JOURNALING"
E_US0A1B_2587:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 [CREATE] PROCEDURE procedure[(parms)]\n
    AS | =\n
    [DECLARE local-variables]\n
    BEGIN\n
     DML/control statements\n
    END"
E_US0A1C_2588:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DROP PROCEDURE procedure."
E_US0A1D_2589:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 EXECUTE PROCEDURE procedure."
E_US0A1E_2590:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 IF [(] condition [)] THEN statement-list\n
 {ELSEIF [(] condition [)] THEN statement-list}\n
 [ELSE statement-list]\n
 ENDIF"
E_US0A1F_2591:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 RETURN [return-status]."
E_US0A20_2592:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 MESSAGE message-number\n
       | message-text\n
       | message-number message-text
\n   [WITH DESTINATION = (<dest> {,<dest>})];
\n
\n where <dest> is SESSION, ERROR_LOG or AUDIT_LOG"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was detected while parsing the "MESSAGE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax and retry.
*/
E_US0A21_2593:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DECLARE var_name {, var_name} [=] var_type;\n
        {var_name {, var_name} [=] var_type;}"
E_US0A22_2594:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 [label:] WHILE [(] condition [)] DO\n
                statement-list\n
          ENDWHILE\n
Use of ENDLOOP [label] is allowed."
E_US0A23_2595:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 [:] parm/local var [:] = expression"
E_US0A24_2596:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT CONNECT\n
     [WITH\n
        [NODE = [\']node_name[\'],\n
         DATABASE = [\']database_name[\']]\n
        [,DBMS = [\']ingres | db2 | ds | ims | dg ...[\']]]"
E_US0A25_2597:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT DISCONNECT"
E_US0A26_2598:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT EXECUTE IMMEDIATE \'query_text\'\n
     [WITH\n
        [NODE = [\']node_name[\'],\n
         DATABASE = [\']ldb_name[\']]\n
        [,DBMS = [\']dbms_name[\']]]"
E_US0A28_2600:SS42000_SYN_OR_ACCESS_ERR
"Syntax error on line %0d.  Last symbol read was: %1c."
E_US0A29_2601:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE [[INTO] tablename] (target_list)
\n                [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]
\n        RETRIEVE UNIQUE (target_list) [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]"
E_US0A2A_2602:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE [[INTO] tablename] (target_list)
\n                [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]
\n        RETRIEVE UNIQUE (target_list) [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]"
E_US0A2B_2603:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        [repeat] APPEND [to] tablename 
\n                 ( column = expression {, column = expression} )
\n                 [WHERE qualification]" 
E_US0A2C_2604:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        REPLACE row_variable (target_list) [WHERE qual]"
E_US0A2D_2605:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n	  [repeat] DELETE  range_var | tablename [where qual]"
E_US0A2E_2606:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        COPY tablename (columnname = format [null_clause] {, ... })
\n            INTO|FROM \"filename\"
\n            [with_clause]"
E_US0A2F_2607:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        CREATE tablename
\n            (columnname1 = format{, columnname2 = format})
\n            [with clause]"
E_US0A30_2608:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DESTROY tablename {, tablename}
\n        DESTROY [PERMIT | INTEGRITY] tablename integer {, integer} | ALL"
E_US0A31_2609:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP
\n        HELP tablename {,tablename}
\n        HELP ALL"
E_US0A32_2610:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        INDEX [unique] ON tablename IS indexname (column1{, column2})
\n        [with_clause]"
E_US0A33_2611:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n MODIFY [tablename | indexname] TO storage-structure
\n  [ON key1 [: sortorder] [{, key2 [:sortorder]}]]
\n  [with_clause]"
E_US0A34_2612:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        PRINT tablename{, tablename}"
E_US0A35_2613:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RANGE OF variable IS tablename"
E_US0A36_2614:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SAVE tablename UNTIL month day year"
E_US0A37_2615:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        OPEN CURSOR CURSORID <retrieve statement> [FOR [DIRECT | DEFERRED]
\n                UPDATE [OF (<column list>)]] [FOR READONLY]"
E_US0A38_2616:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE [[INTO] tablename] (target_list)
\n                [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]
\n        RETRIEVE UNIQUE (target_list) [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]"
E_US0A39_2617:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DEFINE VIEW name (target list) [WHERE qual]"
E_US0A3A_2618:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        ABORT TO savepoint_name"
E_US0A3B_2619:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DEFINE PERMIT oplist ON|OF|TO var [(columnlist)]
\n                TO name [AT term] [FROM time TO time]
\n                [ON day TO day] [WHERE qual]"
E_US0A3C_2620:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DEFINE INTEGRITY ON var IS qual"
E_US0A3D_2621:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A3E_2622:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RELOCATE tablename TO locationname"
E_US0A3F_2623:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A40_2624:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        EXECUTE query_name [( parameter, ... )]"
E_US0A41_2625:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A42_2626:SS42000_SYN_OR_ACCESS_ERR
"line %0d Syntax error on '%1c'.  The correct syntax is:
\n        BEGIN TRANSACTION"
E_US0A43_2627:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        END TRANSACTION"
E_US0A44_2628:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SAVEPOINT savepoint_name"
E_US0A45_2629:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        ABORT"
E_US0A46_2630:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DESTROY INTEGRITY tablename integer{, integer}
\n        DESTROY INTEGRITY tablename ALL"
E_US0A47_2631:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DESTROY PERMIT tablename integer{, integer}
\n        DESTROY PERMIT tablename ALL"
E_US0A48_2632:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A49_2633:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A4A_2634:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        EXECUTE CURSOR CURSORID [( parameter, ...)]"
E_US0A4B_2635:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP INTEGRITY tablename[, tablename]"
E_US0A4C_2636:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP PERMIT tablename[, tablename]"
E_US0A4D_2637:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP VIEW tablename[, tablename]"
E_US0A4E_2638:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A4F_2639:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A50_2640:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        REPLACE CURSOR CURSORID (target list)"
E_US0A51_2641:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE CURSOR CURSORID"
E_US0A52_2642:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ1 integer"
E_US0A53_2643:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ2 integer"
E_US0A54_2644:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ3 integer"
E_US0A55_2645:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ4 integer"
E_US0A56_2646:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET AGGREGATE [NO]PROJECT"
E_US0A57_2647:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET CPUFACTOR integer"
E_US0A58_2648:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET DATE_FORMAT \"us\" | \"multinational\"
\n                       | \"iso\" | \"finland\" | \"sweden\" | \"german\""
E_US0A59_2649:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]DEADLOCK"
E_US0A5A_2650:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET DECIMAL \".\" | \",\""
E_US0A5B_2651:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]EMBEDDED
\n        SET [NO]EQUEL"
E_US0A5C_2652:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]JOINOP"
E_US0A5D_2653:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]JOURNALING [ON tablename]"
E_US0A5E_2654:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET MONEY_FORMAT \"T|L:string\""
E_US0A5F_2655:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET MONEY_PREC integer"
E_US0A60_2656:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET QBUFSIZE integer"
E_US0A61_2657:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET QUERY_SIZE integer"
E_US0A62_2658:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET RET_INTO
\n             \"heap | cheap | heapsort | cheapsort | hash
\n              | chash | isam | cisam | btree | cbtree\""
E_US0A63_2659:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET SORTBUFSIZE integer"
E_US0A64_2660:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET SQL"
E_US0A65_2661:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET STATISTICS \"tablename\""
E_US0A66_2662:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\nSET LOCKMODE SESSION|ON table
\n WHERE [LEVEL=PAGE|TABLE|SESSION|SYSTEM]
\n [,READLOCK=NOLOCK|SHARED|EXCLUSIVE|SESSION|SYSTEM]
\n [,MAXLOCKS|TIMEOUT=int|SESSION|SYSTEM]"
E_US0A67_2663:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT DMnnnn [integer [, integer]]"
E_US0A68_2664:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT PSnnnn [integer [, integer]]"
E_US0A69_2665:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT SCnnnn [integer [, integer]]"
E_US0A6A_2666:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT RDnnnn [integer [, integer]]"
E_US0A6B_2667:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT QEnnnn [integer [, integer]]"
E_US0A6C_2668:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT OPnnnn [integer [, integer]]"
E_US0A6D_2669:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT QSnnnn [integer [, integer]]"
E_US0A6E_2670:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT ADnnnn [integer [, integer]]"
E_US0A6F_2671:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE ..."
E_US0A79_2681:SS50008_UNSUPPORTED_STMT
"line %0d, Syntax error on '%1c'.
\n This SET statement is not supported."
E_US0A7A_2682:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n        SET JOINOP [NO]TIMEOUT [milliseconds]"
E_US0A7B_2683:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n        DEFINE LOCATION location_name WITH TYPE= DATA,
\n             AREA = \"area_name\""
E_US0A7D_2685:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]JOURNALING"
E_US0A88_2696:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT CONNECT\n
     [WITH\n
        [NODE = [\"]node_name[\"],\n
         DATABASE = [\"]database_name[\"]]\n
        [,DBMS = [\"]ingres | db2 | ds | ims | dg ...[\"]]]"
E_US0A89_2697:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT DISCONNECT"
E_US0A8B_2699:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT EXECUTE IMMEDIATE \"query_text\"\n
     [WITH\n
        [NODE = [\"]node_name[\"],\n
         DATABASE = [\"]database_name[\"]]\n
        [,DBMS = [\"]ingres | db2 | ds | ims | dg ...[\"]]]"
E_US0A8C_2700:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Non-terminated string."
E_US0A8D_2701:SS50002_LIMIT_EXCEEDED
"line %0d, String too long."
E_US0A8E_2702:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Invalid operator '%1c'."
E_US0A8F_2703:SS42506_INVALID_IDENTIFIER
"line %0d, Name too long: '%1c'."
E_US0A90_2704:SS50003_EXHAUSTED_RESOURCE
"Attempt to allocate symbol table space failed."
E_US0A91_2705:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Non-terminated comment."
E_US0A92_2706:SS42000_SYN_OR_ACCESS_ERR
"Missing closing delimiter in the delimited identifier."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The scanner encountered the end of the query buffer while trying to
**	read a delimited identifier.  This error can be caused by a missing
**	closing double-quote.
**
** System Status:
**	The query is rejected.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0A93_2707:SS5000A_QUERY_ERROR
"line %0d, Bad floating constant: '%1c'."
E_US0A94_2708:SS5000A_QUERY_ERROR
"line %0d, Unexpected character '%1c'."
E_US0A95_2709:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Bad value in hex constant: '%1c'."
E_US0A96_2710:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Illegal character in string constant."
E_US0A97_2711:SS50002_LIMIT_EXCEEDED
"line %0d, Symbol stack overflow. Reduce target list complexity."
E_US0A98_2712:SS22025_INV_ESCAPE_SEQ
"line %0d, Illegal pattern matching notation."
E_US0A99_2713:SS22019_INV_ESCAPE_CHAR
"line %0d, String constant '%1c' specified for the ESCAPE clause
\nmust be a single character."
E_US0A9A_2714:SS42506_INVALID_IDENTIFIER
"Delimited identifier \"%0c\" is too long."
/*%
** Parameters:
**	%0c - delimited identifier name
**
** Explanation:
**	The delimited identifier being read exceeds the maximum length
**	defined for an identifier.
**
** System Status:
**	The query is rejected.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0A9B_2715:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on <%1c>.\n
Single-quoted identifiers are not supported in this clause or statement.\n
Use delimited identifiers instead (i.e. use double-quote delimiters)."
/*%
** Parameters:
**	%0d - line number of query
**	%1c - token causing error (in this case, a single-quoted identifier)
**
** Explanation:
**	This error is returned when trace point PS251 is turned on,
**	in order to help DBMS tools and users remove single-quoted
**	identifiers from their applications.
**	
**	Support for single-quoted identifiers is being deprecated for most
**	statements, including:
**		o CREATE/ALTER/DROP GROUP '<group>'
**		o CREATE GROUP ... WITH USERS = ('<user>')
**		o ALTER GROUP ... ADD USERS ('<user>')
**		o ALTER GROUP ... DROP USERS ('<user>')
**		o CREATE/ALTER/DROP ROLE '<role>'
**		o GRANT ... TO [USER|GROUP|ROLE] '<authid>'
**	
**	Single-quoted identifiers were never documented for these
**	statements, and users can now use delimited identifiers instead.
**
**	NOTE: Single-quoted identifiers are still allowed in the following
**	statements/clauses: 
**	     REGISTER .. AS LINK
**	        [FROM [[']local_owner_name['].][']local_table_name[']]
**	     CREATE TABLE ..  
**	        [WITH TABLE = [[']local_owner_name['].][']local_table_name[']]
**	     4GL CONNECT statements
**	
** System Status:
**	The query is rejected.
**
** Recommendation:
**	Replace single-quote delimiters with double-quote delimiters, and retry.
*/
E_US0A9C_2716:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Illegal binary value in query text."
E_US0A9D_2717:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Illegal character in query."
E_US0A9E_2718:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Illegal location type '%1c'. Use DATA."
E_US0A9F_2719:SS5000A_QUERY_ERROR
"line %0d, Illegal area name '%1c'."
E_US0AA0_2720:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. Make sure that the specified column name
is not a keyword and is a valid identifier."
E_US0AA1_2721:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. Make sure that the specified table location
or table name is not a keyword and is a valid identifier."
E_US0AA2_2722:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. An integer constant expected."
E_US0AA3_2723:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. A string constant expected."
E_US0AA4_2724:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. Make sure that the specified table name is
not a keyword and is a valid identifier."
E_US0AA5_2725:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. Make sure that the specified index location
or index name is not a keyword and is a valid identifier."
E_US0AA6_2726:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Bad value in bit constant: '%1c'."
E_US0AA7_2727:SS42000_SYN_OR_ACCESS_ERR
"QUEL does not support delimited identifiers 
\n for user name in DEFINE PERMIT statement."
E_US0AA8_2728:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. Invalid authorization ID qualifier."
E_US0AA9_2729:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Invalid syntax for Unicode literal."
/*%
** Parameters:
**
** Explanation:
**	The Unicode literal did not conform to the proper syntax. 
**	Unicode literals are of the form U&' literal data ', where 
**	literal data can contain either standard ASCII characters or
**	escape sequences in the form \hhhh, where hhhh is a 4 digit 
**	hexadecimal value.
**
** System Status:
**	The query is rejected.
**
** Recommendation:
**	Correct the format of the literal and rerun.
*/
E_US0AAA_2730:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Bad value in date/time literal: '%1c'."
E_US0AAB_2731:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Incorrect syntax for interval literal."
E_US0AAC_2732:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Specified unicode converter mapping table is not found in this installation. Unicode conversion is not possible."
E_US0AAD_2733:SS42000_SYN_OR_ACCESS_ERR
"%0c name %1c is too long. The maximum size allowed is %2d."
E_US0ABE_2750:SS5000A_QUERY_ERROR
"DROP TABLE: object '%0c' is not a base table."
E_US0ABF_2751:SS5000A_QUERY_ERROR
"DROP VIEW: object '%0c' is not a view."
E_US0AC0_2752:SS5000A_QUERY_ERROR
"DROP INDEX: object '%0c' is not an index."
E_US0AC1_2753:SS42500_TBL_NOT_FOUND
"%0c: '%1c' does not exist or is not owned by you."
E_US0AC2_2754:SS5000A_QUERY_ERROR
"DROP: Too many objects specified."
E_US0AC3_2755:SS5000A_QUERY_ERROR
"DROP/DESTROY %0c: STAR object '%1c' was
\n created via CREATE %2c.  Use DROP [%2c] or DESTROY."
E_US0AC4_2756:SS5000A_QUERY_ERROR
"DROP/DESTROY %0c:  STAR object '%1c' was used
\n to REGISTER an LDB object of type '%2c'.
\n Use DROP [%2c] or DESTROY."
E_US0AC5_2757:SS5000A_QUERY_ERROR
"REMOVE %0c:  STAR object '%1c' corresponds to an LDB\n
object of type '%2c'.  Use REMOVE [%2c]."
E_US0AC6_2758:SS5000A_QUERY_ERROR
"DROP LINK/REMOVE: STAR object '%0c' was created using
\n CREATE VIEW.  Use DROP [VIEW]."
E_US0AC7_2759:SS5000A_QUERY_ERROR
"%0c: You may not DROP object '%1c' because it was not created through 
STAR.\nUse the SQL REMOVE statement to drop its registration from STAR."
E_US0AC8_2760:SS50008_UNSUPPORTED_STMT
"SET RESULT_STRUCTURE is not allowed in distributed INGRES."
E_US0AC9_2761:SS42000_SYN_OR_ACCESS_ERR
"SET: invalid storage structure '%0c'."
E_US0ACA_2762:SS5000A_QUERY_ERROR
"Use CKPDB +j <dbname> to turn on journaling for catalogs.\n
`%0c' is a catalog."
E_US0ACB_2763:SS42000_SYN_OR_ACCESS_ERR
"REMOVE: syntax error on '%0c'.  The correct syntax is:\n
REMOVE [TABLE|VIEW] object_name {, object_name}"
E_US0ACC_2764:SS42503_INSUF_PRIV
"%0c: '%1c' is not owned by %2c."
E_US0ACD_2765:SS42503_INSUF_PRIV
"You may not %0c '%1c' owned by %2c."
E_US0ACE_2766:SS5000A_QUERY_ERROR
"SET [NO]JOURNALING is not allowed on indices.\n
`%0c' is an index."
E_US0ACF_2767:SS50008_UNSUPPORTED_STMT
"The SET WORK LOCATIONS statement is not allowed in distributed INGRES."
E_US0AD6_2774:SS5000A_QUERY_ERROR
"INSERT: Duplicate column '%0c' specified."
E_US0AD7_2775:SS42000_SYN_OR_ACCESS_ERR
"INSERT: number of target columns must equal the number of specified values."
E_US0AD8_2776:SS5000A_QUERY_ERROR
"INSERT: number of target columns must equal the number of target list
elements in the specified subselect."
E_US0AD9_2777:SS23500_CHECK_CONS_VIOLATION
"INSERT: Must not insert NULL value into a nonnullable column '%0c'."
E_US0ADA_2778:SS5000A_QUERY_ERROR
"INSERT: Only constant expressions allowed in the value list."
E_US0ADB_2779:SS23500_CHECK_CONS_VIOLATION
"No value specified for mandatory column '%0c'."
E_US0ADC_2780:SS23500_CHECK_CONS_VIOLATION
"UPDATE: Column '%0c' is nonnullable and cannot accept a NULL value."
E_US0ADD_2781:SS23500_CHECK_CONS_VIOLATION
"INSERT: No value specified for mandatory column '%0c'
\nof base table '%1c' on which view '%2c' is based."
E_US0ADE_2782:SS22500_INVALID_DATA_TYPE
"The UNKNOWN keyword can only be used when applied to a BOOLEAN type."
E_US0AF0_2800:SS50002_LIMIT_EXCEEDED
"line %0d, Stack overflow while parsing query.
Please simplify the query."
E_US0AF2_2802:SS50008_UNSUPPORTED_STMT
"line %0d, The RETRIEVE INTO statement is not allowed in OEM servers."
E_US0B57_2903:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, Function '%1c' specified with incorrect number of parameters."
/*%
** Parameters:
**	%0d  - line number of script the error occurred in
**	%1c  - function name
**
** Explanation:
**	A function was coded in a SQL statement with an incorrect number 
**	of parameters. 
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Correct the function specification.
*/
E_US0B58_2904:SS42000_SYN_OR_ACCESS_ERR
"Value of type <%0c> was supplied where
\nonly a character string is appropriate."
/*%
** Parameters:
**	%0c - datatype of supplied value
**
** Explanation:
**	A language variable contained a value of a type other than string type
**	(i.e., C, CHAR, TEXT, VARCHAR, LONGTEXT), which is the only type
**	appropriate for the current context.
**
**	This message will be returned, for instance, if a user were to supply an
**	integer variable in place of <escape character> of a <like predicate>.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_US0B59_2905:SS42000_SYN_OR_ACCESS_ERR
"Unary PLUS or MINUS operators may be attached only to numeric literals
in this context.  Expressions, symbolic constants or column values may not
be preceded by a unary operator."
/*%
** Parameters:
**	None.
**
** Explanation:
**	In this statement, an expression in the target list must be a numeric
**	literal in order for it to be preceded by a unary plus or minus.  This
**	is likely to occur in a statement that does not allow expressions in
**	its target lists.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	If this is a syntax error, correct the error.  If your program
**	requires the unary operator on a dynamic value, then either calculate
**	the negative value prior to the use of the value, or consider delaying
**	the assumption that the value is negative until later in the program.
*/
E_US0B5A_2906:SS42000_SYN_OR_ACCESS_ERR
"line %0d, There is no such function as '%1c'."
E_US0B5B_2907:SS5000A_QUERY_ERROR
"line %0d, Function '%1c' does not take an argument of type '%2c'.
Explicitly convert the argument to the desired type."
/*%
** Parameters:
**	%0d  - line number of script the error occurred in
**	%1c  - function name
**	%2c  - first argument of function
**
** Explanation:
**	The function specified takes an argument of a specified type.  See the
**	appropriate command language reference manual section on functions.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Convert the argument to the required type.
*/
E_US0B5C_2908:SS5000A_QUERY_ERROR
"line %0d, Function '%1c' is not defined for arguments of type '%2c' and '%3c'.
Explicitly convert its arguments to the desired type(s)."
/*%
** Parameters:
**	%0d  - line number
**	%1c  - function name
**	%2c  - first argument type
**	%3c  - second argument type
**
** Explanation:
**	The function specified takes arguments of a specified type.  See the
**	appropriate command language reference manual section on functions.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Convert the arguments to the required type. For example, change
**	    retrieve (x=concat(str_field, int_field))
**	to
**	    retrieve (x=concat(str_field, c(int_field))
*/
E_US0B5D_2909:SS5000A_QUERY_ERROR
"line %0d, Function '%1c' with argument type '%2c' is ambiguous.
Explicitly convert its argument to the desired type."
E_US0B5E_2910:SS5000A_QUERY_ERROR
"line %0d, Operator '%1c' with operand(s) '%2c' and '%3c' is ambiguous."
E_US0B5F_2911:SS5000A_QUERY_ERROR
"line %0d, Datatype '%1c' can't be converted to '%2c'."
E_US0B60_2912:SS5000A_QUERY_ERROR
"line %0d, Aggregate function '%1c' with result type '%2c' is ambiguous.
Explicitly convert the function or its arguments to the desired
type(s)."
E_US0B61_2913:SS42000_SYN_OR_ACCESS_ERR
"line %0d, You cannot assign this '%1c' value to a column of type
'%2c'. Explicitly convert the value to the required type or check it
is in the valid range for the target datatype."
E_US0B62_2914:SS42000_SYN_OR_ACCESS_ERR
"line %0d, You cannot have an aggregate function in a WHERE clause."
E_US0B63_2915:SS42000_SYN_OR_ACCESS_ERR
"line %0d, The columns in the SELECT clause must be contained in
the GROUP BY clause."
E_US0B64_2916:SS5000A_QUERY_ERROR
"line %0d, Too many select items in SUBSELECT."
E_US0B65_2917:SS5000A_QUERY_ERROR
"line %0d, Number of target list elements in all UNIONed subselects
must be the same."
E_US0B66_2918:SS5000A_QUERY_ERROR
"line %0d, SELECT item types '%1c' and '%2c' are not coercible
to the same type."
E_US0B67_2919:SS5000A_QUERY_ERROR
"line %0d, Conversion of types '%1c' and '%2c' to the same type is ambiguous."
E_US0B68_2920:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Illegal expression in the HAVING clause."
E_US0B69_2921:SS5000A_QUERY_ERROR
"line %0d, Table reference '%1c' is ambiguous."
E_US0B6A_2922:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Queries without from lists may not contain HAVING,
WHERE, or GROUP BY clauses."
E_US0B6B_2923:SS5000A_QUERY_ERROR
"line %0d, Aggregates cannot contain correlated variables."
E_US0B6C_2924:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error.
There are too many entries in the WITH or WHERE clause."
E_US0B6D_2925:SS5000A_QUERY_ERROR
"SELECT: FOR UPDATE clause found in non-updatable SELECT statement.
\n      May not use set function, GROUP, HAVING, DISTINCT, UNION, ORDER or more
\n      than one variable in the FROM clause, at the outermost query level."
E_US0B6E_2926:SS42000_SYN_OR_ACCESS_ERR
"DESCRIBE: invalid USING '%0c' clause. May specify NAMES only."
E_US0B6F_2927:SS5000A_QUERY_ERROR
"UNION and UNION ALL set operators must not be mixed."
E_US0B70_2928:SS5000A_QUERY_ERROR
"line %0d, aggregate function '%1c' not allowed in this context."
E_US0B71_2929:SS5000A_QUERY_ERROR
"Value of host language variable of type '%0c' must not be negative."
E_US0B72_2930:SS5000A_QUERY_ERROR
"line %0d.  Illegal use of aggregate function in a WHERE clause."
/*%
** Parameters:
**	%0d  - line number
**
** Explanation:
**	Given
**	    [SELECT ...] SELECT ... FROM from_list WHERE where_clause ...,
**	use of aggregate(x.col_name) where x is a part of the from_list will be
**	prohibited.  This constitutes a relaxation of the restriction as it
**	existed in 6.3, where use of correlated aggregates was prohibited; use
**	of aggregates in WHERE clause outside of SUBSELECT was also prohibited.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0B73_2931:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Function '%1c' is undefined, or is not a valid predicate function."
/*%
** Parameters:
**	%0d  - line number
**
** Explanation:
**	A WHERE clause contains a predicate in the form:
**	    expression1 operator_name expression2
**	This is only valid if "operator_name" is a user-defined function
**	with the "binary predicate" attribute. See the "Object
**	Management Extension User's Guide" for more information.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0B74_2932:SS42000_SYN_OR_ACCESS_ERR
"line %0d, The GROUP BY clause must contain simple column references,
a number referring to an entry in the select list or an expression 
which doesn't contain a set function."
E_US0B75_2933:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error. Case function %1c expressions must all
be of comparable/coercible types."
/*%
** Parameters:
**	%0d  - line number
**	%1c  - "result" or "source/comparand" literals
**
** Explanation:
**	All result expressions in a case function must be of comparable/
**	coercible type. Likewise, the source and comparand expressions
**	in a case statement of the form "CASE value WHEN ..." must also
**	be of comparable type. 
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0B76_2934:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error. Invalid syntax for %1c function."
/*%
** Parameters:
**	%0d  - line number
**	%1c  - function name
**
** Explanation:
**	The supplied function name was used with special syntax reserved
**	for the ANSI SQL position and trim functions.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0B77_2935:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error. Trim function with FROM must also include
at least one of trim specification or trim character."
E_US0B78_2936:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error. Invalid trim specification value %1c."
E_US0B79_2937:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error. Invalid date/time specification value %1c."
/*%
** Parameters:
**	%0d  - line number
**	%1c  - trim specifier
**
** Explanation:
**	The trim function must have a trim specification of leading,
**	trailing or both. This function invocation specified an invalid 
**	value.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0C0B_3083:SS5000A_QUERY_ERROR
"line %0d. REGISTER %1c is only allowed in a distributed INGRES."
/*%
** Parameters:
**	%0d - line number
**	%1c - register name
**
** Explanation:
**	The "REGISTER [VIEW]" statement is only allowed in a Distributed
**	DBMS.  If you were trying to register a gateway table as
**	"IMPORT", please make sure you have specified the "TABLE" keyword.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Correct the error and reenter.
*/
E_US0C1C_3100:SS50002_LIMIT_EXCEEDED
"Query requires too many range variables."
E_US0CE4_3300:SS5000A_QUERY_ERROR
"Cannot update a view."
E_US0CEE_3310:SS5000A_QUERY_ERROR
"%0c on view %1c: can only update simple columns. No constants or
expressions."
E_US0CF8_3320:SS5000A_QUERY_ERROR
"%0c on view %1c: column occurs in qualification of view."
E_US0CF9_3321:SS5000A_QUERY_ERROR
"%0c on view '%1c' may not be performed because definition
\nof '%1c' involved WITH CHECK OPTION and a qualification."
E_US0D02_3330:SS5000A_QUERY_ERROR
"%0c on view %1c: update of a multi-variable view not allowed."
E_US0D0C_3340:SS5000A_QUERY_ERROR
"%0c on view %1c: views do not have TID's."
E_US0D16_3350:SS5000A_QUERY_ERROR
"%0c on view %1c: cannot update views containing an aggregate."
E_US0D20_3360:SS5000A_QUERY_ERROR
"%0c on view %1c: that update might be non-functional."
E_US0D21_3361:SS5000A_QUERY_ERROR
"%0c on view %1c: cannot update this view."
E_US0D2A_3370:SS42500_TBL_NOT_FOUND
"%0c on view %1c: base table %2c does not exist."
E_US0D34_3380:SS42503_INSUF_PRIV
"No permission to update base table '%0c'."
E_US0D35_3381:SS42500_TBL_NOT_FOUND
"Permission on the subject table has qualification that
references nonexistent table."
E_US0D36_3382:SS42503_INSUF_PRIV
"User has no permission to execute database procedure '%0c'."
E_US0DA2_3490:SS5000A_QUERY_ERROR
"INTEGRITY on %0c: cannot handle aggregates."
E_US0DA3_3491:SS5000A_QUERY_ERROR
"INTEGRITY on %0c: cannot handle multivariable constraints."
E_US0DA4_3492:SS5000A_QUERY_ERROR
"INTEGRITY on %0c: constraint does not initially hold."
E_US0DA5_3493:SS42000_SYN_OR_ACCESS_ERR
"INTEGRITY on %0c: cannot define integrity on a view."
E_US0DA6_3494:SS42503_INSUF_PRIV
"INTEGRITY on %0c: you must own %0c."
E_US0DA7_3495:SS50008_UNSUPPORTED_STMT
"The CREATE/DEFINE INTEGRITY command is not allowed in distributed INGRES."
E_US0DA8_3496:SS5000A_QUERY_ERROR
"You cannot define an integrity constraint on a TID."
E_US0DA9_3497:SS5000A_QUERY_ERROR
"INTEGRITY on %0c: cannot have subselects in constraint."
E_US0DAA_3498:SS42503_INSUF_PRIV
"line %0d, You are not allowed to create integrity on '%1c' table."
E_US0DAC_3500:SS42503_INSUF_PRIV
"%2c on table %0c: protection violation."
E_US0DAD_3501:SS50002_LIMIT_EXCEEDED
"Number of protection qualifications for %0c in this query
exceeds maximum (%1d)."
E_US0DAE_3502:SS42503_INSUF_PRIV
"%2c on table %0c: no GRANT or GRANT compatible permit exists."
E_US0E06_3590:SS5000A_QUERY_ERROR
"PERMIT: bad terminal identifier '%0c'."
E_US0E07_3591:SS28000_INV_AUTH_SPEC
"PERMIT: bad user name '%0c'."
E_US0E08_3592:SS42503_INSUF_PRIV
"PERMIT: Table '%0c' not owned by you."
E_US0E09_3593:SS5000A_QUERY_ERROR
"PERMIT: Table '%0c' must be a real table (not a view)."
E_US0E0A_3594:SS5000A_QUERY_ERROR
"PERMIT on %0c: bad day-of-week '%1c'."
E_US0E0B_3595:SS42503_INSUF_PRIV
"PERMIT on %0c: only the DBA can use the PERMIT statement."
E_US0E0C_3596:SS50008_UNSUPPORTED_STMT
"The CREATE/DEFINE PERMIT statement is not allowed in distributed INGRES."
E_US0E0D_3597:SS5000A_QUERY_ERROR
"PERMIT: Error on '%0c'. Views are not allowed in the qualification."
E_US0E0E_3598:SS5000A_QUERY_ERROR
"GRANT: You cannot grant permissions on views created in QUEL."
E_US0E0F_3599:SS50008_UNSUPPORTED_STMT
"CREATE PERMIT: This release of STAR only supports creation of
\n      ALL TO ALL and SELECT TO ALL permits."
E_US0E74_3700:SS5000B_INTERNAL_ERROR
"Tree buffer overflow in query modification."
E_US0E75_3701:SS50002_LIMIT_EXCEEDED
"Tree build stack overflow in query modification."
/*
** NOTE: range 3800-3999 is RESERVED for syntax error messages.
** PLEASE do not use it for any other purposes
*/
E_US0ED8_3800:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 CREATE LINK link_name [(col_list)]\n
     [WITH\n
        [NODE = [\']node_name[\'],\n
         DATABASE = [\']ldb_name[\']]\n
        [,DBMS = [\']dbms_name[\']\n
        [,TABLE = [\']table_name[\']]]"
E_US0ED9_3801:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n      AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH"
E_US0EDA_3802:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REMOVE [TABLE|VIEW] object_name {, object_name}"
/*%
** schang (05/15/92) : gateway syntax now uses
**	PSQ_REG_IMPORT  for REGISTER TABLE (103)
**	PSQ_REG_INDEX   for REGISTER INDEX (104)
**	PSQ_REG_REMOVE  for REMOVE [TABLE|INDEX] (105)
**
**	those messages commented out below are ones not defined
**	in psfparse.h
**
** US0EDC_3804:E_GE7918_SYNTAX_ERROR
** "Line %0d, syntax error on '%1c'.  The correct syntax is:\n
** REGISTER objname AS LINK WITH REFRESH"
** US0EDD_3805:E_GE7918_SYNTAX_ERROR
** "Line %0d, syntax error on '%1c'.  The correct syntax is:\n
** SET DDL_CONCURRENCY ON | OFF"
**
*/
E_US0EDB_3803:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER TABLE tablename (col_name col_format {, col_name col_format})
\n      AS IMPORT FROM 'source' with_clause."
E_US0EDC_3804:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER [UNIQUE] INDEX indexname
\n      ON tablename (col_name {, col_name})
\n      AS IMPORT FROM 'source' with_clause."
E_US0EDD_3805:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REMOVE [TABLE|INDEX] objname {,objname}"
E_US0EE2_3810:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
 CREATE RULE|TRIGGER [schema.]rule_name\n
        BEFORE|AFTER DELETE|INSERT|UPDATE[(col {, col})]\n
	    {, DELETE|INSERT|UPDATE[(col {, col})]}\n
	ON|OF|FROM|INTO [schema.]table_name\n
        [REFERENCING [OLD AS old_corr_name]|[NEW AS new_corr_name]]\n
	[WHERE qual]\n
	[FOR EACH {ROW|STATEMENT}]\n
    EXECUTE PROCEDURE proc_name [(plist)]"
E_US0EE3_3811:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 DROP RULE rule_name"
E_US0EE4_3812:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n RAISE ERROR errornumber [errortext]
\n   [WITH DESTINATION = (<dest> {,<dest>})];
\n
\n where <dest> is SESSION, ERROR_LOG or AUDIT_LOG"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "RAISE ERROR" statement
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EE5_3813:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE GROUP groupname {,groupname}
\n   [WITH USERS = (user {,user})]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE GROUP" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EE6_3814:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER GROUP groupname {,groupname}
\n     ADD  USERS (user {,user})
\n   | DROP USERS (user {,user})"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER GROUP" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EE7_3815:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER GROUP groupname {,groupname}
\n     ADD  USERS (user {,user})
\n   | DROP USERS (user {,user})"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER GROUP...DROP"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EE8_3816:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP GROUP groupname {,groupname}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP GROUP" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EE9_3817:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE ROLE roleid {,roleid}
\n   WITH
\n          PASSWORD = 'password'
\n        | NOPASSWORD"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE ROLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EEA_3818:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER ROLE roleid {,roleid}
\n   WITH
\n          PASSWORD = 'password'
\n        | NOPASSWORD"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER ROLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EEB_3819:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP ROLE roleid {,roleid}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP ROLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EEC_3820:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n REVOKE dbpriv {,dbpriv}
\n  ON CURRENT INSTALLATION | DATABASE dbname {,dbname}
\n  FROM [authtype] authid {,authid}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "REVOKE" statement
**	for database or installation privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EED_3821:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n GRANT dbpriv {,dbpriv}
\n  ON CURRENT INSTALLATION | DATABASE dbname {,dbname}
\n  TO [authtype] authid {,authid}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "GRANT" statement
**	for database or installation privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EEE_3822:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n REVOKE dbpriv {,dbpriv}
\n  ON CURRENT INSTALLATION | DATABASE dbname {,dbname}
\n  FROM [authtype] authid {,authid}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "REVOKE" statement
**	for database or installation privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF0_3824:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]MAXxxxx [nnnn]
\n  where xxxx is IO, QUERY, or ROW;
\n  nnnn is a positive integer less than or equal to
\n  the value of QUERY_xxxx_LIMIT for your session.
\n  A value must not be specified with [NO]MAXxxxx."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A syntax error was found while parsing the "SET MAX ..." statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF1_3825:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE USER <user_name>
\n  [WITH
\n      [  PRIVILEGES = (<priv> {,<priv>})  |  NOPRIVILEGES]
\n      [, GROUP = <group>  | NOGROUP]
\n      [, DEFAULT_PRIVILEGES = (<priv> {,<priv>})| ALL |  NODEFAULT_PRIVILEGES]
\n      [, EXPIRE_DATE = 'expire_date' | NOEXPIRE_DATE] |
\n      [, LIMITING_SECURITY_LABEL = 'security_label' | 
\n         NOLIMITING_SECURITY_LABEL] |
\n      [, PROFILE=,profile_name> | NOPROFILE] |
\n      [, SECURITY_AUDIT = ( <opt> {,<opt> ) | NOSECURITY_AUDIT]
\n      [, PASSWORD = 'password'  | NOPASSWORD]
\n  ]
\n
\n NOTE: Not all privileges or user attributes may be available in your
\n       installation."
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE USER" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF2_3826:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER USER <user_name>
\n  [ADD|DROP PRIVILEGES ( priv {,priv})]
\n  [WITH
\n    [  PRIVILEGES = (<priv> {,<priv>}) | NOPRIVILEGES]
\n    [, GROUP = <group>  | NOGROUP]
\n    [, DEFAULT_PRIVILEGES = (<priv> {,<priv>}) | ALL | NODEFAULT_PRIVILEGES]
\n    [, EXPIRE_DATE = 'expire_date' | NOEXPIRE_DATE]
\n    [, LIMITING_SECURITY_LABEL = 'security_label'
\n       NOLIMITING_SECURITY_LABEL]
\n    [, PROFILE=profile_name> | NOPROFILE]
\n    [, SECURITY_AUDIT = ( <opt> {,<opt> ) | NOSECURITY_AUDIT]
\n    [, PASSWORD = 'password'  | NOPASSWORD]
\n    [, OLDPASSWORD = 'password' ]
\n  ]
\n
\n NOTE: Not all privileges or user attributes may be available in your
\n       installation."
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER USER" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
**
*/
E_US0EF3_3827:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP USER <user_name>"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP USER" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF4_3828:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE LOCATION <location_name>
\n  WITH
\n         AREA  = '<area_name>'
\n      [, USAGE = (<usage> {,<usage>}) | NOUSAGE]
\n      [, RAWPCT = <rawpct> ]
\n
\n  where <usage> is DATABASE, WORK, JOURNAL, CHECKPOINT, DUMP or
\n                   ALL
\n    and <rawpct> is a positive integer from 0 through 100."
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE LOCATION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF5_3829:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER LOCATION <location_name>
\n  WITH USAGE = (<usage> {,<usage>}) | NOUSAGE
\n
\n  where <usage> is DATABASE, WORK, JOURNAL, CHECKPOINT, DUMP or
\n                   ALL."
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER LOCATION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF6_3830:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP LOCATION <location_name>"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP LOCATION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF7_3831:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE SECURITY_ALARM
\n  ON [TABLE] <table_name> {,<table_name>}
\n  [IF <condition> {,<condition>}]
\n  [WHEN <privilege> {,<privilege>}]
\n  [BY [USER] <user_name> {,<user_name>} | PUBLIC]
\n
\n  where
\n      <condition> is SUCCESS or FAILURE
\n      <privilege> is SELECT, UPDATE, INSERT or DELETE"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE SECURITY_ALARM"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF8_3832:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP SECURITY_ALARM on <table_name> <number> {,<number>}
\n
\n  where <number> is the security alarm number as returned
\n                  by 'HELP SECURITY_ALARM <table_name>'"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP SECURITY_ALARM"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EF9_3833:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ENABLE SECURITY_AUDIT <audit_type>
\n
\n  where <audit_type> is one of:
\n          ALL
\n          ROW
\n          TABLE
\n          VIEW
\n          DATABASE
\n          PROCEDURE
\n          LOCATION
\n          USER
\n          SECURITY
\n          ALARM
\n          ROLE
\n          RULE
\n          DBEVENT
\n          LEVEL('<security_label>')
\n          RESOURCE
\n          QUERY_TEXT"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ENABLE SECURITY_AUDIT"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EFA_3834:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DISABLE SECURITY_AUDIT <audit_type>
\n
\n  where <audit_type> is one of:
\n          ALL
\n          ROW
\n          TABLE
\n          VIEW
\n          DATABASE
\n          PROCEDURE
\n          LOCATION
\n          USER
\n          SECURITY
\n          ALARM
\n          ROLE
\n          RULE
\n          DBEVENT
\n          LEVEL('<security_label>')
\n          RESOURCE
\n          QUERY_TEXT"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DISABLE SECURITY_AUDIT"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EFB_3835:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  COMMENT ON TABLE tbl_name [IS long_remark]
\n
\n              or
\n
\n  COMMENT ON COLUMN tbl_name.column_name [IS long_remark]."
/*
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "COMMENT ON TABLE/COLUMN"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EFC_3836:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n GRANT dbpriv {,dbpriv}
\n  ON CURRENT INSTALLATION | DATABASE dbname {,dbname}
\n  TO [authtype] authid {,authid}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "GRANT" statement
**	for database or installation privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EFD_3837:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n REVOKE dbpriv {,dbpriv}
\n  ON CURRENT INSTALLATION | DATABASE dbname {,dbname}
\n  FROM [authtype] authid {,authid}"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "REVOKE" statement
**	for database or installation privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EFE_3838:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  CREATE SYNONYM synonym_name FOR [owner.]tbl_name."
/*
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE SYNONYM" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0EFF_3839:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  DROP SYNONYM synonym_name {, synonym_name}."
/*
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP SYNONYM" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F00_3840:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n      SET SESSION AUTHORIZATION user_name | USER | CURRENT_USER |
				  SYSTEM_USER | SESSION_USER | INITIAL_USER"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET SESSION AUTHORIZATION"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F01_3841:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  ENDLOOP [<label>]."
/*
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ENDLOOP" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F02_3842:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]LOGGING"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET [NO]LOGGING" statement
**	to enable/disable logging.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F03_3843:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET SESSION
\n     WITH ON_ERROR = ROLLBACK STATEMENT |
\n                     ROLLBACK TRANSACTION"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET SESSION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F04_3844:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n      SET ROLE role_name | SESSION ROLE
\n or
\n      SET NOROLE"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET ROLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F05_3845:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n      SET GROUP group_name | SESSION GROUP
\n or
\n      SET NOGROUP"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET GROUP" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F06_3846:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DECLARE GLOBAL TEMPORARY TABLE tablename
\n   (columnname format [null_default_clause] {, ... })
\n   ON COMMIT PRESERVE ROWS
\n   WITH NORECOVERY [,with_clause]
\n
\n DECLARE GLOBAL TEMPORARY TABLE tablename
\n   [(columnname {, ... })]
\n   AS subselect
\n   ON COMMIT PRESERVE ROWS
\n   WITH NORECOVERY [,with_clause]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DECLARE GLOBAL TEMPORARY
**	TABLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F07_3847:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DECLARE GLOBAL TEMPORARY TABLE tablename
\n   [(columnname {, ... })]
\n   AS subselect
\n   ON COMMIT PRESERVE ROWS
\n   WITH NORECOVERY [,with_clause]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DECLARE GLOBAL TEMPORARY
**	TABLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F09_3849:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n [CONSTRAINT constraint_name] constraint_clause
\n     [constraint_with_clause],
\n     where constraint_with_clause is:
\n   WITH NO INDEX 
\n   | WITH BASE TABLE STRUCTURE
\n   | WITH (with_keyword = with_option {,with_keyword = with_option})"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing a "constraint_clause" or a
**	"constraint_with_clause" in a CREATE or ALTER TABLE statement.
**	
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F0A_3850:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER TABLE tablename
\n     ADD [CONSTRAINT constraint_name] constraint_clause
\n   | DROP CONSTRAINT constraint_name RESTRICT | CASCADE
\n   | ADD [COLUMN] columnname format [default_clause] [null_clause] 
[column_constraint]
\n   | DROP [COLUMN] column_name RESTRICT | CASCADE
\n   | ALTER [COLUMN] columnname format [default_clause] [null_clause]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER TABLE" statement.
**	
**	You cannot use both the "ADD" clause and the "DROP" clause in a single
**	statement.  Only one constraint or column can be added or dropped in a
**	single statement. 
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F0B_3851:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE SCHEMA AUTHORIZATION authid
\n   [ CREATE TABLE ... | CREATE VIEW ... | GRANT ... { ... } ] ;
\n
\n where:
\n
\n      o authid must be the current effective user identifier
\n      o Subsequent DDL substatements are not separated with semicolons
\n      o CREATE TABLE cannot contain any AS SELECT clauses
\n      o GRANT can be issued on tables or views only"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE SCHEMA" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F0D_3853:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n EXECUTE PROCEDURE | CALLPROC proc_name
\n        [(parameter = value, ...)]"
/*%
** Parameters:
**	%0d - line number
**	%0c - token name
**
** Explanation:
**	A syntax error was found while parsing "EXECUTE PROCEDURE" statement
**	in Star for a registered procedure.  Taken from E_US09E7_2535.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
/** Note: error number 3854 is not used, as the CRESETDBP query mode is
*** mapped to the CREDBP syntax error (#2587)
**/
E_US0F0F_3855:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE TABLE tablename
\n   ( columnname format [default_clause] [null_clause]
\n     [column_constraint] [collate_clause] [encrypt_clause]
\n     | table_constraint
\n     {, ... }
\n   ) [with_clause]
\n
\n CREATE TABLE tablename [(colname_list)]
\n   AS subselect
\n      [with_clause]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing a post-6.4 "CREATE TABLE"
**	statement (SQL).
**
**	NOTE: The 6.4 syntax message (#2507) is still used for 
**	      Star "CREATE TABLE" statement syntax errors.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F10_3856:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP SCHEMA schema_name CASCADE|RESTRICT"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP SCHEMA" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F11_3857:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: Invalid combination of attribute qualifiers specified. 
\n ADD COLUMN does not support 'with system_maintained', or 'not null' 
\n without an accompanying 'with default' (with no explicit default 
\n value)."
/*%
** Explanation:
**	A syntax error was found while parsing the
**	"ALTER TABLE tbl ADD COLUMN" statement
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F12_3858:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: the number of columns cannot be less than 1 "
/*%
** Explanation:
**      A syntax error was found while parsing the
**      "ALTER TABLE tbl DROP COLUMN" statement
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and reenter.
*/
E_US0F13_3859:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: invalid change of attributes on an ALTER COLUMN"
/*%
** Explanation:
**	A invalid combination of attributes/changes in the
**	"ALTER TABLE tbl ADD COLUMN" statement
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and reenter.
*/
E_US0F1C_3868:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is: 
\n
\nSET TRANSACTION [READ ONLY|WRITE]
\n                [,ISOLATION LEVEL  READ UNCOMMITTED
\n                                 | READ COMMITTED
\n                                 | REPEATABLE READ
\n                                 | SERIALIZABLE ]"
/*%
** Explanation:
**      A syntax error was found while parsing the
**      "SET TRANSACTION" statement.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and reenter.
*/
E_US0F23_3875:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n FOR subselect DO
\n   other procedure statements
\n ENDFOR;"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the database procedure
**	FOR statement.
**
** System Status:
**	The procedure creation is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F24_3876:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n RETURN ROW ( return-list );"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the database procedure
**	RETURN ROW statement.
**
** System Status:
**	The procedure creation is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F25_3877:SS42000_SYN_OR_ACCESS_ERR
"Invalid parameters for SET random_seed "
/*%
** Explanation:
**      Illegal parameters were detected while parsing 
**      "SET random_seed" statement.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and renter.
*/
E_US0F26_3878:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER DATABASE <database_name>
\n     WITH REPLICATION | NOREPLICATION"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER DATABASE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F27_3879:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE SEQUENCE <sequence name>
\n	[AS <datatype>]
\n	[START WITH <integer>]
\n	[INCREMENT BY <integer>]
\n	[MAXVALUE <integer> | NOMAXVALUE]
\n	[MINVALUE <integer> | NOMINVALUE]
\n	[UNORDERED | SEQUENTIAL]
\n	[CACHE <integer> | NOCACHE]
\n	[CYCLE | NOCYCLE]
\n	[ORDER | NOORDER]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "CREATE SEQUENCE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F28_3880:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER SEQUENCE <sequence name>
\n	[RESTART WITH <integer>]
\n	[INCREMENT BY <integer>]
\n	[MAXVALUE <integer> | NOMAXVALUE]
\n	[MINVALUE <integer> | NOMINVALUE]
\n	[UNORDERED | SEQUENTIAL]
\n	[CACHE <integer> | NOCACHE]
\n	[CYCLE | NOCYCLE]
\n	[ORDER | NOORDER]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "ALTER SEQUENCE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F29_3881:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP SEQUENCE <sequence name>"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "DROP SEQUENCE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F2E_3886:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER TABLE tablename
\n     ADD [CONSTRAINT constraint_name] constraint_clause
\n   | DROP CONSTRAINT constraint_name RESTRICT | CASCADE
\n   | ADD [COLUMN] columnname format [default_clause] [null_clause]
[column_constraint]
\n   | DROP [COLUMN] column_name RESTRICT | CASCADE
\n   | ALTER [COLUMN] columnname format [default_clause] [null_clause]"
/*%
** Parameters:
**      %0d - line number
**      %1c - token name
**
** Explanation:
**      A syntax error was found while parsing the "ALTER TABLE" statement.
**
**      If you have specified alter table alter column statement you cannot
**      specify the constraint specification.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and reenter.
*/
E_US0F3C_3900:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 DEFINE LINK link_name [(col_list)]\n
     [WITH\n
        [NODE = [\"]node_name[\"],\n
         DATABASE = [\"]ldb_name[\"]]\n
        [,DBMS = [\"]dbms_name[\"]\n
        [,TABLE = [\"]table_name[\"]]]"
E_US0F3D_3901:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n      AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH"
E_US0F3E_3902:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REMOVE [TABLE|VIEW] object_name {, object_name}"
E_US0F40_3904:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REGISTER objname AS LINK WITH REFRESH"
E_US0F41_3905:SS42000_SYN_OR_ACCESS_ERR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
SET DDL_CONCURRENCY ON | OFF"
E_US0F54_3924:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]MAXxxxx [nnnn]
\n  where xxxx is IO, QUERY, or ROW;
\n  nnnn is a positive integer less than or equal to
\n  the value of QUERY_xxxx_LIMIT for your session.
\n  A value must not be specified with [NO]MAXxxxx."
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET MAX ..." statement.
**	This is the QUEL version of E_US0FE0_3824.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F64_3940:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n      SET SESSION AUTHORIZATION user_name | USER | CURRENT_USER |
				  SYSTEM_USER | SESSION_USER | INITIAL_USER"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET SESSION AUTHORIZATION"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F66_3942:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]LOGGING"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET [NO]LOGGING" statement
**	to enable/disable logging.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F67_3943:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET SESSION
\n     WITH ON_ERROR = ROLLBACK STATEMENT |
\n                     ROLLBACK TRANSACTION"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET SESSION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F68_3944:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n      SET ROLE role_name | SESSION ROLE
\n or
\n      SET NOROLE"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET ROLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F69_3945:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n      SET GROUP group_name | SESSION GROUP
\n or
\n      SET NOGROUP"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error was found while parsing the "SET GROUP" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F76_3958:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n     SET SESSION 
\n	   [WITH ON_ERROR = ROLLBACK TRANSACTION|STATEMENT]
\n       [READ ONLY | READ WRITE]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "SET SESSION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US0F80_3968:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is: 
\n
\nSET TRANSACTION [READ ONLY|WRITE]
\n                [,ISOLATION LEVEL  READ UNCOMMITTED
\n                                 | READ COMMITTED
\n                                 | REPEATABLE READ
\n                                 | SERIALIZABLE ]"
/*%
** Explanation:
**      A syntax error was found while parsing the
**      "SET TRANSACTION" statement.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Correct the syntax error and reenter.
*/
E_US100D_4109:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n\tGRANT ALL [PRIVILEGES] | <tbl_priv>(s) ON [TABLE] <obj_spec>(s)
\n\t\tTO [<grantee type>] <grantee>(s) [WITH GRANT OPTION] or
\n\tGRANT ALL [PRIVILEGES] | EXECUTE ON PROCEDURE <obj_spec>(s)
\n\t\tTO [<grantee type>] <grantee>(s) [WITH GRANT OPTION] or
\n\tGRANT ALL [PRIVILEGES] | <ev_priv>(s) ON DBEVENT <obj_spec>(s)
\n\t\tTO [<grantee type>] <grantee>(s) [WITH GRANT OPTION] or
\n\tGRANT ALL [PRIVILEGES] | NEXT ON SEQUENCE <obj_spec>(s)
\n\t\tTO [<grantee type>] <grantee>(s) [WITH GRANT OPTION] or
\n\tGRANT ALL [PRIVILEGES] | <db_priv>(s)
\n\t\tON CURRENT INSTALLATION | DATABASE <database name>(s)
\n\t\tTO [<grantee type>] <grantee>(s)
\n where
\n\t <tbl_priv> ::= SELECT | INSERT | DELETE | UPDATE[<column list>] |
\n\t\t\tREFERENCES[<column list>]
\n\t <ev_priv>  ::= REGISTER | RAISE
\n\t <grantee type> ::= [USER] | GROUP | ROLE
\n\t <grantee> ::= <auth_id> | PUBLIC"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A post-6.4 syntax error was found while parsing the "GRANT" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US100E_4110:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n\tREVOKE [GRANT OPTION FOR] ALL [PRIVILEGES] | <tbl_priv>(s)
\n\t\tON [TABLE] <obj_spec>(s) FROM [<grantee type>] <grantee>(s)
\n\t\tRESTRICT | CASCADE or
\n\tREVOKE [GRANT OPTION FOR] ALL [PRIVILEGES] | EXECUTE
\n\t\tON PROCEDURE <obj_spec>(s) FROM [<grantee type>] <grantee>(s)
\n\t\tRESTRICT | CASCADE or
\n\tREVOKE [GRANT OPTION FOR] ALL [PRIVILEGES] | <ev_priv>(s)
\n\t\tON DBEVENT <obj_spec>(s) FROM [<grantee type>] <grantee>(s)
\n\t\tRESTRICT | CASCADE or
\n\tREVOKE ALL [PRIVILEGES] | <db_priv>(s)
\n\t\tON CURRENT INSTALLATION | DATABASE <database name>(s)
\n\t\tFROM [<grantee type>] <grantee>(s)
\n where
\n\t <tbl_priv> ::= SELECT | INSERT | DELETE | UPDATE[<column list>] |
\n\t\t\tREFERENCES[<column list>]
\n\t <ev_priv>  ::= REGISTER | RAISE
\n\t <grantee type> ::= [USER] | GROUP | ROLE
\n\t <grantee> ::= <auth_id> | PUBLIC"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A post-6.4 syntax error was found while parsing the "REVOKE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US100F_4111:SS42000_SYN_OR_ACCESS_ERR
"String cannot be converted to numeric due to incorrect syntax."
E_US1010_4112:SS42000_SYN_OR_ACCESS_ERR
"CREATE or ALTER TABLE: invalid option '%0c'. 
\nTry [WITH | NOT NULL] [WITH | NOT DEFAULT]."
/*%
** Parameters:
**	%0c - option name
**
** Explanation:
**	A post-6.4 syntax error was found while parsing column qualifications
**	in a "CREATE TABLE" statement. This error message is used instead of
**	error #2003, because we don't want 6.5 syntax printing out for 6.4
**	servers.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US1011_4113:SS42000_SYN_OR_ACCESS_ERR
"CREATE or ALTER TABLE: either invalid attribute qualifier '%0c'
or wrong combination of qualifiers.  Try one or more of
\n    [with | not null]
\n    [with default | not default | default <value>]
\n    [with | not system_maintained]
\n    [UNIQUE | PRIMARY KEY]
\n    [REFERENCES <table>(<col_list>)]
\n    [CHECK (<boolean_expr>)]."
/*%
** Parameters:
**	%0c - attribute qualifier
**
** Explanation:
**	A post-6.4 syntax error was found while parsing column qualifications
**	in a "CREATE TABLE" statement.  This error message is used instead of
**	error #2004.
**	
**	The accepted syntax diagram for the column qualifier clause is:
**
**	    '[with | not null] [not | with system_maintained]
**	      [with default | not default | [with] default <value>]
**	      [UNIQUE | PRIMARY KEY] [REFERENCES <table>(<col_list>)]
**	      [CHECK (<boolean_expr>)]'
**
**	with two exceptions. It is not allowed to specify "not default",
**	"with null", or "default <value>" in conjunction with the
**	"with system_maintained" qualifier.
**
**	The order of these qualifiers is no longer significant (as of 6.5).
**	The following is a complete list of accepted attribute qualifiers
**	(a few options, although syntactically correct per the above syntax
**	diagram, are disallowed due to the "with system_maintained"
**	restrictions): 
**	
**	        with null
**	        not  null
**	        with null with default
**	        with null [with] default <value>
**	        with null not default
**	        not  null with default
**	        not  null [with] default <value>
**	        not  null not default
**	        not  null with default with system_maintained
**	        with null not system_maintained
**	        not  null with default not system_maintained
**	        not  null not default not system_maintained
**	        with system_maintained
**	        not  system_maintained
**	        not  null UNIQUE
**	        not  null PRIMARY KEY
**
**	"REFERENCES" and "CHECK" clauses can be used with any of these.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the "CREATE" statement after correcting the problem with
**	the attribute qualifier clause.
*/
E_US1018_4120:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n     ALTER SECURITY_AUDIT [RESUME|SUSPEND|STOP|RESTART]
\n     [WITH AUDIT_LOG='file_name']"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing an "ALTER SECURITY_AUDIT" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US1019_4121:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n     SET SESSION 
\n     [ADD|DROP PRIVILEGES ( priv {,priv} )]
\n     [,READ ONLY|WRITE]
\n     [,ISOLATION LEVEL isolation_level]
\n     [WITH [SECURITY_LABEL = 'label' | INITIAL_LABEL]
\n	     [,ON_ERROR = ROLLBACK TRANSACTION|STATEMENT]
\n	     [,ON_USER_ERROR = NOROLLBACK | ROLLBACK TRANSACTION]
\n	     [,ON_LOGFULL = ABORT|COMMIT|NOTIFY]
\n	     [,[NO]PRIVILEGES [= ( priv {,priv} )| ALL | DEFAULT]]
\n           [,[NO]PRIORITY [= nnnn | INITIAL | MAXIMUM | MINIMUM]]
\n           [,[NO]DESCRIPTION [= description]]
\n     ]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "SET SESSION" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US101A_4122:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n   CREATE PROFILE profile_name
\n   [WITH
\n   [  PRIVILEGES = (<priv> {,<priv>})  |  NOPRIVILEGES]
\n   [, GROUP = '<group>'  | NOGROUP]
\n   [, DEFAULT_PRIVILEGES = (<priv> {,<priv>}) | ALL | NODEFAULT_PRIVILEGES]
\n   [, EXPIRE_DATE = 'expire_date' | NOEXPIRE_DATE] |
\n   [, LIMITING_SECURITY_LABEL = 'security_label' | 
\n      NOLIMITING_SECURITY_LABEL] |
\n   [, SECURITY_AUDIT = ( <opt> {,<opt> ) | NOSECURITY_AUDIT]
\n   ]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "CREATE PROFILE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US101B_4123:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n   ALTER [DEFAULT] PROFILE [profile_name]
\n    [ADD PRIVILEGES ( priv {,priv} )|
\n    DROP PRIVILEGES ( priv {,priv} )]
\n   [WITH
\n   [  PRIVILEGES = (<priv> {,<priv>})  |  NOPRIVILEGES]
\n   [, GROUP = '<group>'  | NOGROUP]
\n   [, DEFAULT_PRIVILEGES = (<priv> {,<priv>})| ALL | NODEFAULT_PRIVILEGES]
\n   [, EXPIRE_DATE = 'expire_date' | NOEXPIRE_DATE] |
\n     [, LIMITING_SECURITY_LABEL = 'security_label' | 
\n      NOLIMITING_SECURITY_LABEL] |
\n     [, SECURITY_AUDIT = ( <opt> {,<opt> ) | NOSECURITY_AUDIT]
\n   ]"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "ALTER PROFILE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US101C_4124:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      DROP PROFILE profile_name [CASCADE|RESTRICT]
"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "DROP PROFILE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US101D_4125:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      SET [NO]MAXxxxx nnnn
\n
\n  where xxxx is IDLE or CONNECT;
\n  nnnn is a positive integer less than or equal to
\n  the value of the session limit for your session.
\n  A value must not be specified with [NO]MAXxxxx.
"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "SET [NO]MAX ..." statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US101E_4126:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      SET [NO]ROWLABEL_VISIBLE
\n
"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "SET [NO]ROWLABEL_VISIBLE"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US101F_4127:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      CREATE SECURITY_ALARM [alarmname] ON alarmobj
\n		[IF alarmcond {,alarmcond}{] [WHEN alarmact {,alarmact}] 
\n		[BY [GROUP|ROLE|USER] alarmsubject | PUBLIC]
\n		[RAISE DBEVENT [dbeventowner.]dbeventname ['dbeventtext']
\n where:
\n	alarmobj::= [TABLE] tablespec | DATABASE dbname | CURRENT INSTALLATION
\n
\n      alarmcond::= SUCCESS | FAILURE 
\n
\n      alarmact::= SELECT | INSERT | UPDATE | DELETE (for TABLE alarms)
\n		    CONNECT | DISCONNECT	      (for DATABASE alarms)
\n
"
/*%
** Parameters:
**      0 - line number
**      1 - token
**
** Explanation:
**      A syntax error occurred processing a CREATE SECURITY_ALARM statement
**
** System Status:
**      The statement is aborted.
**
** User Action:
**      Correct the syntax error.
**
*/
E_US1020_4128:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n     DROP SECURITY_ALARM ON alarmobj alarmspec {,alarmspec} | ALL
\n
\n where:
\n	alarmobj::= [TABLE] tablespec | DATABASE dbname | CURRENT INSTALLATION
\n
\n      alarmspec::= alarmname | alarmnum
"
/*%
** Parameters:
**      0 - line number
**      1 - token
**
** Explanation:
**      A syntax error occurred processing a DROP SECURITY_ALARM statement
**
** System Status:
**      The statement is aborted.
**
** User Action:
**      Correct the syntax error.
**
*/
E_US1021_4129:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      SET ROLE role_name | NONE
\n	[WITH PASSWORD = 'role_password']
\n
"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "SET ROLE"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US1022_4130:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      GRANT rolename {,rolename} TO [USER] username {,username} | PUBLIC
\n
"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "GRANT role"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US1023_4131:SS42000_SYN_OR_ACCESS_ERR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n
\n      REVOKE rolename {,rolename} FROM [USER] username {,username} | PUBLIC
\n
"
/*%
** Parameters:
**	%0d - line number
**	%1c - token name
**
** Explanation:
**	A syntax error occurred processing a "REVOKE role"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the syntax error and reenter.
*/
E_US1036_4150:SS5000B_INTERNAL_ERROR
"Illegal filename for a table was specified: '%0c'"
E_US1068_4200:SS22003_NUM_VAL_OUT_OF_RANGE
"Integer overflow detected in query."
E_US1069_4201:SS22012_DIVISION_BY_ZERO
"Integer divide by zero detected in query."
E_US106A_4202:SS22003_NUM_VAL_OUT_OF_RANGE
"Floating overflow detected in query."
E_US106B_4203:SS22012_DIVISION_BY_ZERO
"Floating divide by zero detected in query."
E_US106C_4204:SS22003_NUM_VAL_OUT_OF_RANGE
"Floating underflow detected in query."
E_US1071_4209:SS50000_MISC_ERRORS
"Invalid argument in call to math function."
E_US1072_4210:SS50000_MISC_ERRORS
"The locate() function does not accept non-string parameters.  Explicitly convert parameters with non-string types to parameters with string types."
E_US1077_4215:SS01000_WARNING
"Warning: %0d Integer overflows detected."
E_US1078_4216:SS01000_WARNING
"Warning: %0d Integer divide by zero operations detected."
E_US1079_4217:SS01000_WARNING
"Warning: %0d Floating overflows detected."
E_US107A_4218:SS01000_WARNING
"Warning: %0d Floating divide by zero operations detected."
E_US107B_4219:SS01000_WARNING
"Warning: %0d Floating underflows detected."
E_US1080_4224:SS01000_WARNING
"Warning: Non-printable character(s) converted to blank(s)."
E_US1081_4225:SS42000_SYN_OR_ACCESS_ERR
"Error: An illegal IP address is supplied."
E_US10CC_4300:SS5000N_OPERAND_TYPE_MISMATCH
"Addition of two absolute dates is undefined.  Addition on dates must
involve an absolute date and an interval."
E_US10CD_4301:SS5000N_OPERAND_TYPE_MISMATCH
"Subtracting an absolute date from an interval is not supported."
E_US10CE_4302:SS22008_DATETIME_FLD_OVFLOW
"'%0c' is not a valid date/time value."
E_US10CF_4303:SS22008_DATETIME_FLD_OVFLOW
"%0d is not a valid year for a date/time column."
E_US10D0_4304:SS22008_DATETIME_FLD_OVFLOW
"%0d is not a valid month for a date/time column."
E_US10D1_4305:SS22008_DATETIME_FLD_OVFLOW
"%0d is not a valid day for a date/time column."
E_US10D2_4306:SS22008_DATETIME_FLD_OVFLOW
"%0d:%1d:%2d is not a valid time for a date/time column."
E_US10D3_4307:SS22008_DATETIME_FLD_OVFLOW
"%0c is not a valid time for use with the '%1c' qualifier in a date/time column."
E_US10D4_4308:SS22008_DATETIME_FLD_OVFLOW
"bad character found in date/time string beginning with '%0c'."
E_US10D5_4309:SS22008_DATETIME_FLD_OVFLOW
"cannot use am/pm specifications in a time interval."
E_US10D6_4310:SS22008_DATETIME_FLD_OVFLOW
"%0d years is out of valid range for a date interval."
E_US10D7_4311:SS22007_INV_DATETIME_FMT
"date is out of range of valid years."
E_US10D8_4312:SS5000O_INVALID_FUNC_ARG_TYPE
"function dow (day-of-week) only valid on absolute dates."
E_US10D9_4313:SS5000O_INVALID_FUNC_ARG_TYPE
"the date_trunc function will work on absolute dates only."
E_US10DA_4314:SS22008_DATETIME_FLD_OVFLOW
"weeks are not implemented in dates."
E_US10DB_4315:SS5000O_INVALID_FUNC_ARG_TYPE
"the interval function recognizes no such unit as %0c."
E_US10DC_4316:SS5000O_INVALID_FUNC_ARG_TYPE
"the interval function does not work on absolute dates."
E_US10DD_4317:SS01000_WARNING
"Warning: %0d absolute dates ignored in sum() or avg() aggregate."
E_US10DE_4318:SS22008_DATETIME_FLD_OVFLOW
"%0d %1c is out of valid range for a date/time interval."
E_US10DF_4319:SS22008_DATETIME_FLD_OVFLOW
"'%0c' is not a valid syntax for ANSI date/time value. Use correct syntax or cast this string using CAST function or explicit ANSI date/time coercion functions."
E_US10E0_4320:SS50000_MISC_ERRORS
"Configuration parameter DATE_ALIAS is set to NONE. Date cannot be used as a valid data type name in this installation. Set the parameter in the config file to ingresdate or ansidate if this is not desired behaviour."
/*%
** Parameters:
**      None.
**
** Explanation:
**	The column data type date has been provided but the "DATE_ALIAS"
**	configuration parameter is set to "NONE", disallowing the word "date".
**	To change the behaviour, change ii.<hostname>.config.date_alias to
**	ingresdate or ansidate.  ansidate is an ANSI compliant date type. 
**
** System Status:
**      The query fails with error.
**
** Recommendation:
**      Convert the iidbdb database, using the upgradedb utility.
**
*/
E_US10E1_4321:SS50000_MISC_ERRORS
"Cannot convert %0c type to %1c data type."
E_US10E2_4322:SS50000_MISC_ERRORS
"Incorrect format for year field. ANSI date literals require a four digit year value. Please provide four digits for the year, using leading zeroes if necessary."
E_US10E3_4323:SS5000N_OPERAND_TYPE_MISMATCH
"Subtracting unlike date/time types is not supported. Only like date/time types or interval types can be subtracted from date/time types."
E_US10E4_4324:SS5000N_OPERAND_TYPE_MISMATCH
"Adding date/time types is not supported. Only date/time types and interval types, or like interval types can be added."
E_US10E5_4325:SS22007_INV_DATETIME_FMT
"'%0c' is an invalid format or value for ANSI date. Please enter a valid date in yyyy-mm-dd format."
E_US10E6_4326:SS22007_INV_DATETIME_FMT
"'%0c' is an invalid format or value for time without time zone or time with local time zone. Please enter a valid time value in hh:mm:ss[.fff...] format."
E_US10E7_4327:SS22007_INV_DATETIME_FMT
"'%0c' is invalid format or value for time with time zone. Please enter a valid time value in hh:mm:ss[.fff...]+|-hh:mm format."
E_US10E8_4328:SS22007_INV_DATETIME_FMT
"'%0c' is invalid format or value for timestamp without time zone or timestamp with local time zone. Please enter a valid timestamp value in yyyy-mm-dd hh:mm:ss[.fff...] format."
E_US10E9_4329:SS22007_INV_DATETIME_FMT
"'%0c' is invalid format or value for timestamp with time zone. Please enter a valid timestamp value in yyyy-mm-dd hh:mm:ss[.fff...]+|-hh:mm format."
E_US10EA_4330:SS22007_INV_DATETIME_FMT
"'%0c' is invalid format or value for interval year to month. Please enter a valid interval value in [+/-]years-months format."
E_US10EB_4331:SS22007_INV_DATETIME_FMT
"'%0c' is invalid format or value for interval day to second. Please enter a valid interval value in [+/-]days hh:mm:ss[fff...] format."
E_US10EC_4332:SS22007_INV_DATETIME_FMT
"Invalid values specified for time zone. Time zone must be between +14:00 to -12:59"
E_US10ED_4333:SS5000N_OPERAND_TYPE_MISMATCH
"Cannot add a year to month or day interval to a time type."
E_US10F0_4336:SS50000_MISC_ERRORS
"Client at level %0d is attempting to retrieve unsupported data type '%1c'"
E_US10F1_DATE_DIV_BY_ZERO:SS22012_DIVISION_BY_ZERO
"Date interval divide by 0 detected."
E_US10F2_DATE_DIV_ANSI_INTV:SS42000_SYN_OR_ACCESS_ERR
"Cannot mix ANSI intervals in division."
E_US10F3_DATE_ARITH_NOABS:SS42000_SYN_OR_ACCESS_ERR
"The operators unary-'+', unary-'-', '*' and '/' cannot be used with absolute dates."
E_US10F4_ANSISPEC:SS42000_SYN_OR_ACCESS_ERR
"Incorrect ANSI trim specification. Only one character allowed"
E_US10F5_4341:SS5000O_INVALID_FUNC_ARG_TYPE
"The date function/date_part('unit') used is for absolute dates only."
E_US1130_4400:SS22000_DATA_EXCEPTION
"illegal character to integer conversion or non-numeric character
in money field."
E_US1131_4401:SS22000_DATA_EXCEPTION
"exceeded the maximum money value allowed."
E_US1132_4402:SS22000_DATA_EXCEPTION
"value specified is less than the minimal legal value."
E_US1133_4403:SS22000_DATA_EXCEPTION
"illegal blanks detected in money field."
E_US1134_4404:SS22000_DATA_EXCEPTION
"multiple dollar signs detected in money field."
E_US1135_4405:SS22000_DATA_EXCEPTION
"multiple positive and/or negative signs detected in money field."
E_US1136_4406:SS22000_DATA_EXCEPTION
"multiple decimal points detected in money field."
E_US1137_4407:SS22000_DATA_EXCEPTION
"comma detected in money field - enter money values without commas."
E_US1138_4408:SS22000_DATA_EXCEPTION
"positive or negative sign detected in numeric part of money field."
E_US1139_4409:SS22000_DATA_EXCEPTION
"dollar sign detected in numeric part of money field."
E_US113A_4410:SS22000_DATA_EXCEPTION
"money divide by zero detected."
E_US113B_4411:SS22000_DATA_EXCEPTION
"Invalid hexadecimal character.  Hexadecimal characters must be 0..9, A..F."
E_US1194_4500:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate key on INSERT detected."
E_US1195_4501:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate key on UPDATE detected."
E_US1196_NOT_ZEROONE_ROWS:SS21000_CARD_VIOLATION
"SELECT returned multiple values."
E_US1197_4503:SS5000A_QUERY_ERROR
"line %0d, %0c is a non-nullable datatype."
E_US1198_4504:SS23501_UNIQUE_CONS_VIOLATION
"This update duplicates a row in the table."
E_US1199_4505:SS23000_CONSTR_VIOLATION
"Update failed because an integrity constraint was violated."
E_US11F7_4599:SS50000_MISC_ERRORS
"Unexpected end of table during cursor update.  Transaction is aborted."
E_US11F8_4600:SS24000_INV_CURS_STATE
"Single table cursor is not open."
E_US11F9_4601:SS24000_INV_CURS_STATE
"The cursor specified has not been opened."
E_US11FA_4602:SS24000_INV_CURS_STATE
"The specified cursor does not support the requested action."
E_US11FB_4603:SS24000_INV_CURS_STATE
"The cursor was opened read-only."
E_US11FC_4604:SS24000_INV_CURS_STATE
"Either this cursor is already open or an attempt was\n
made to declare a savepoint when a cursor was open."
E_US11FD_4605:SS24000_INV_CURS_STATE
"The cursor is not currently positioned on a row."
E_US11FF_4607:SS24000_INV_CURS_STATE
"Only one deferred cursor can be opened at a time."
E_US1200_4608:SS50000_MISC_ERRORS
"Table name is not valid."
E_US1201_4609:SS42506_INVALID_IDENTIFIER
"Owner name is not valid."
E_US1202_4610:SS50000_MISC_ERRORS
"Invalid attribute type specified."
E_US1203_4611:SS50000_MISC_ERRORS
"A bad attribute name was specified."
E_US1204_4612:SS50000_MISC_ERRORS
"Invalid size for specified attribute type."
E_US1205_4613:SS50000_MISC_ERRORS
"Invalid precision for specified attribute type."
E_US1206_4614:SS50000_MISC_ERRORS
"The specified location name is invalid."
E_US1207_4615:SS5000B_INTERNAL_ERROR
"An error occurred while deleting a record."
E_US1208_4616:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate records were found."
E_US1209_4617:SS50000_MISC_ERRORS
"Database access conflict."
E_US120A_4618:SS5000B_INTERNAL_ERROR
"Database is already open."
E_US120B_4619:SS5000H_UNAVAILABLE_RESOURCE
"Nonexistent database."
E_US120C_4620:SS50000_MISC_ERRORS
"Invalid lock level for table or database."
E_US125A_4698:SS40001_SERIALIZATION_FAIL
"Insufficient undo information to make read consistent page."
E_US125B_4699:SS40001_SERIALIZATION_FAIL
"Unable to serialize access to row."
E_US125C_4700:SS40001_SERIALIZATION_FAIL
"Deadlock detected, your single or multi-query transaction has been aborted."
E_US125D_4701:SS5000A_QUERY_ERROR
"Ambiguous replace: you have specified several replacements for the same row."
E_US125E_4702:SS5000P_TIMEOUT_ON_LOCK_REQUEST
"Timeout occurred on lock request (see SET LOCKMODE command)."
E_US125F_4703:SS50002_LIMIT_EXCEEDED
"Out of locks.  Reduce number of tables referenced inside of
multi-query transaction or number of page locks allowed on a table
(see SET LOCKMODE command)."
E_US1260_4704:SS50002_LIMIT_EXCEEDED
"The IIDBDB is unavailable at this time because the DBMS is out of locks.\n
        Please reduce the number of locks being held or enlarge the\n
        lock table."
E_US1261_4705:SS50003_EXHAUSTED_RESOURCE
"Out of locks.  Reduce number of tables referenced inside of
multi-query transaction or number of page locks allowed on a table
(see SET LOCKMODE command)."
E_US1262_4706:SS40001_SERIALIZATION_FAIL
"Your transaction has been externally aborted.
See errlog.log for more information."
E_US1263_4707:SS50002_LIMIT_EXCEEDED
"Error allocating file system resource or bad file specification given.
Check disk space, disk quota, open file quota and the physical location."
E_US1264_4708:SS40001_SERIALIZATION_FAIL
"The query has been aborted."
E_US1265_4709:SS5000Q_DB_REORG_INVALIDATED_QP
"Database reorganization activity or excessive thrashing in QSF has resulted in
the inability to find a valid query plan after several successive attempts.
Increasing the size of QSF cache using CBF/VCBF may resolve this problem.
If database reorganization is in progress, try again after the db activity is 
complete; in case of dynamic SQL, the statement must be prepared again. "
E_US1266_4710_TRAN_ID_NOTUNIQUE:SS50000_MISC_ERRORS
"The distributed transaction ID is not unique in the local system."
E_US1267_4711_DIS_TRAN_UNKNOWN:SS50000_MISC_ERRORS
"The distributed transaction ID is unknown to the local system."
E_US1268_4712_DIS_TRAN_OWNER:SS50000_MISC_ERRORS
"The distributed transaction is owned by another server in the local system.\n
Disconnect the previous association and try to re-connect later."
E_US1269_4713_NOSECUREINCLUSTER:SS0A000_FEATUR_NOT_SUPPORTED
"Can't support two-phase commit protocol in a cluster installation."
E_US126A_4714_ILLEGAL_STMT:SS25000_INV_XACT_STATE
"Illegal statement specified. Only ROLLBACK or COMMIT statement is allowed \n
immediately after the RE-CONNECT or PREPARE TO COMMIT statement."
E_US126B_4715_DIS_DB_UNKNOWN:SS50000_MISC_ERRORS
"The database connected by the distributed transaction is unknown to the local system."
E_US126C_4716_LOCK_RESOURCE_BUSY:SS5000H_UNAVAILABLE_RESOURCE
"A lock resource needed by the query is busy."
E_US1271_4721_IIDBDB_TRAN_LIMIT:SS50003_EXHAUSTED_RESOURCE
"The IIDBDB is unavailable at this time because a Logging System quota
has been exceeded.  Either reduce the number of active sessions
or alter the logging system parameters."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The logging system is configured with quotas on the number
**	of open databases and the number of active transactions in the
**	installation.  One of these limits has been reached.  The exact
**	quota that has been exceeded can be determined from the error log,
**	which will indicate either an error opening/adding a database (database
**	quota) or beginning a transaction (transaction quota).
**
** System Status:
**	The application or front-end program will not start up.
**
** Recommendation:
**	The user must either wait for active sessions to complete so that the
**	system falls below its defined quotas, or the logging system must
**	be reconfigured with greater limits.
*/
E_US1272_4722_DB_TRAN_LIMIT:SS50003_EXHAUSTED_RESOURCE
"The database cannot be opened at this time because a Logging System quota
has been exceeded.  Either reduce the number of active sessions
or alter the logging system parameters."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The logging system is configured with quotas on the number
**	of open databases and the number of active transactions in the
**	installation.  One of these limits has been reached.  The exact
**	quota which has been exceeded can be determined from the error log,
**	which will indicate either an error opening/adding a database (database
**	quota) or beginning a transaction (transaction quota).
**
** System Status:
**	The application or front-end program will not start up.
**
** Recommendation:
**	The user must either wait for active sessions to complete so that the
**	system falls below its defined quotas, or the logging system must
**	be reconfigured with greater limits.
*/
E_US1273_4723:SS25000_INV_XACT_STATE
"COPY WITH RECOVERY=FALSE: This statement qualifier is not allowed in a multi-query transaction."
/*%
** Parameters"
**	None.
**
** Explanation:
**	The "COPY" statement included the 'with recovery=false' qualifier,
**	which is illegal inside a multi-statement transaction.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	Reissue the "COPY" statement, either leaving out the
**	'with recovery=false' qualifier or making sure this is the
**	only statement inside the transaction.
*/
E_US1389_5001:SS42506_INVALID_IDENTIFIER
"PRINT: bad table name %0c."
E_US138A_5002:SS5000A_QUERY_ERROR
"PRINT: %0c is a view and can't be printed."
E_US138B_5003:SS42503_INSUF_PRIV
"PRINT: Table %0c is protected."
E_US138C_5004:SS50008_UNSUPPORTED_STMT
"The PRINT command is not allowed in distributed INGRES."
E_US138D_5005:SS50000_MISC_ERRORS
"Table '%0c' is already in use. Close that table and try again."
E_US138E_5006:SS50000_MISC_ERRORS
"Table is in use. Close that table and try again."
E_US138F_5007:SS25000_INV_XACT_STATE
"Updating operations are disallowed in a READ ONLY transaction or on a READ ONLY Database."
E_US13EE_5102:SS42502_DUPL_OBJECT
"%0c: duplicate table name '%1c'."
E_US13EF_5103:SS42503_INSUF_PRIV
"%0c: '%1c' is a system table."
E_US13F0_5104:SS42506_INVALID_IDENTIFIER
"CREATE %0c: invalid column name '%1c'."
E_US13F1_5105:SS42502_DUPL_OBJECT
"CREATE %0c: duplicate column name '%1c'."
E_US13F2_5106:SS42000_SYN_OR_ACCESS_ERR
"CREATE %0c: invalid column format '%1c' on column '%2c'."
E_US13F3_5107:SS50002_LIMIT_EXCEEDED
"CREATE: It is not allowed to create a table with more than 1024 columns."
E_US13F4_5108:SS50002_LIMIT_EXCEEDED
"CREATE %0c: excessive table width on column '%1c'."
E_US13F5_5109:SS42506_INVALID_IDENTIFIER
"CREATE %0c: underscore not allowed in table names."
E_US13F6_5110:SS50000_MISC_ERRORS
"CREATE %0c: journaling not allowed on this database."
E_US13F7_5111:SS42502_DUPL_OBJECT
"CREATE %0c: You cannot create this table because the dba's table of
the same name has already been referenced by this query or
multi-query transaction."
E_US13F8_5112:SS50003_EXHAUSTED_RESOURCE
"CREATE %0c: Disk error trying to create table.  You are probably out
of disk space or disk quota."
E_US13F9_5113:SS50000_MISC_ERRORS
"CREATE/MODIFY: '%0c' is an invalid locationname for your database.  It is
either not defined or your database can't use it.  Define it through ACCESSDB."
E_US13FA_5114:SS50008_UNSUPPORTED_STMT
"The CREATE command is not allowed in distributed INGRES."
E_US13FB_5115:SS42000_SYN_OR_ACCESS_ERR
"The CREATE command does not recognize the parameter '%1c'."
E_US13FC_5116:SS42507_RESERVED_IDENTIFIER
"%0c: Illegal tablename '%1c'. '%2c' is reserved for system catalogs."
E_US13FD_5117:SS00000_SUCCESS
"CREATE: %0c: Journaling will be enabled at the next checkpoint."
E_US13FE_5118:SS50008_UNSUPPORTED_STMT
"%0c is not allowed in distributed INGRES."
/* b92641 */
E_US13FF_5119:SS50000_MISC_ERRORS
"CREATE/MODIFY: '%0c' is not a valid OLDLOCATION for the table %1c."
E_US1400_5120:SS42000_SYN_OR_ACCESS_ERR
"Natural join syntax is not yet supported."
E_US1401_5121:SS42000_SYN_OR_ACCESS_ERR
"Scalar subqueries not yet supported in this context."
/*%
** Parameters"
**	None.
**
** Explanation:
**	Scalar subqueries are supported in the WHERE clause, but not yet in 
**	the SELECT-list, UPDATE/SET clause and other such locations.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	Fix the syntax to drop the unsupported subselect.
*/
E_US141E_5150:SS50008_UNSUPPORTED_STMT
"The EXCLUDE NODE command is not allowed in non-distributed INGRES."
E_US141F_5151:SS50008_UNSUPPORTED_STMT
"The INCLUDE NODE command is not allowed in non-distributed INGRES."
E_US1420_5152:SS42000_SYN_OR_ACCESS_ERR
"The DEFINE REFERENCE command doesn't recognize the parameter '%1c'."
E_US1421_5153:SS42000_SYN_OR_ACCESS_ERR
"Invalid table qualifier '%1c'. Only SESSION is allowed."
E_US1451_5201:SS42503_INSUF_PRIV
"DESTROY: %0c is a system table."
E_US1452_5202:SS42500_TBL_NOT_FOUND
"DESTROY: %0c does not exist or is not owned by you."
E_US1453_5203:SS42505_OBJ_NOT_FOUND
"DROP/DESTROY: %0d does not identify an existing integrity constraint."
E_US1454_5204:SS42505_OBJ_NOT_FOUND
"DROP/DESTROY: %0d does not identify an existing protection constraint."
E_US1455_5205:SS50008_UNSUPPORTED_STMT
"The DEFINE REFERENCE command is not allowed in non-distributed INGRES."
E_US1456_5206:SS50008_UNSUPPORTED_STMT
"The DROP/DESTROY INTEGRITY command is not allowed in distributed INGRES."
E_US1457_5207:SS50008_UNSUPPORTED_STMT
"The DROP/DESTROY PERMIT command is not allowed in distributed INGRES."
E_US1458_5208:SS50008_UNSUPPORTED_STMT
"DROP/REMOVE INDEX is not allowed in distributed INGRES."
E_US1459_5209:SS50002_LIMIT_EXCEEDED
"The maximum number of views in a HELP VIEW command is %0d."
E_US145A_5210:SS42503_INSUF_PRIV
"DROP/DESTROY PERMIT: only the DBA can use the DROP/DESTROY PERMIT statement."
E_US145B_5211:SS42000_SYN_OR_ACCESS_ERR
"Can not %1c '%0c'.
\n One may not %1c a DDB object whose underlying LDB object is an index."
E_US145C_5212:SS50008_UNSUPPORTED_STMT
"QUEL queries are not supported in this release of distributed INGRES."
E_US145D_5213:SS42505_OBJ_NOT_FOUND
"DROP SECURITY_ALARM: %0d does not identify an existing security alarm."
/*%
** Parameters:
**	%0d - specified security alarm number
**
** Explanation:
**	The security alarm number specified in the "DROP SECURITY_ALARM"
**	statement does not exist on the specified table.
**
** System Status:
**	This error message is issued, and the remaining security alarms
**	(if any) are processed.
**
** Recommendation:
**	Verify that the specified number identifies a valid security alarm.
**	If not, respecify the command with a valid security alarm number.
*/
E_US14B4_5300:SS42500_TBL_NOT_FOUND
"CREATE INDEX: cannot find primary table '%0c'."
E_US14B5_5301:SS50002_LIMIT_EXCEEDED
"CREATE INDEX: A secondary index may be created on at most 6 columns."
E_US14B6_5302:SS42501_COL_NOT_FOUND
"CREATE INDEX: Invalid column '%0c'."
E_US14B7_5303:SS42500_TBL_NOT_FOUND
"CREATE INDEX: Table '%0c' not owned by you."
E_US14B8_5304:SS5000A_QUERY_ERROR
"CREATE INDEX: Table '%0c' is already an index."
E_US14B9_5305:SS5000A_QUERY_ERROR
"CREATE INDEX: Table '%0c' is a system table."
E_US14BA_5306:SS5000A_QUERY_ERROR
"CREATE INDEX: '%0c' is a view and an index can't be built on it."
E_US14BB_5307:SS42502_DUPL_OBJECT
"CREATE INDEX: duplicate column '%0c'."
E_US14BE_5310:SS50000_MISC_ERRORS
"CREATE INDEX: index %0c has been corrupted.  It contains a pointer (tid:%1d),
which points to a non-existent row in base table %2c. The error status was %3d."
E_US14BF_5311:SS50008_UNSUPPORTED_STMT
"The INDEX statement is not allowed in distributed INGRES."
E_US14C0_5312:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Location specified twice: '%0c' conflicts with '%1c'."
E_US14C1_5313:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Bad storage structure '%0c'.
The valid storage structure names are [c]btree, [c]isam, [c]hash."
E_US14C2_5314:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_US14C3_5315:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Structure must be specified before '%0c'."
E_US14C4_5316:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Invalid FILLFACTOR of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US14C5_5317:SS42000_SYN_OR_ACCESS_ERR
"INDEX/CREATE INDEX: FILLFACTOR can be specified only for
\nbtree, [c]isam, [c]hash storage structure."
E_US14C6_5318:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Invalid MINPAGES of '%0d' specified. Must be greater than 0."
E_US14C7_5319:SS42000_SYN_OR_ACCESS_ERR
"INDEX/CREATE INDEX: MINPAGES can be specified only for
\n[c]hash storage structure."
E_US14C8_5320:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Invalid MAXPAGES of '%0d' specified. Must be greater than 0."
E_US14C9_5321:SS42000_SYN_OR_ACCESS_ERR
"INDEX/CREATE INDEX: MAXPAGES can be specified only for
\n[c]hash storage structure."
E_US14CA_5322:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Invalid NONLEAFFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US14CB_5323:SS42000_SYN_OR_ACCESS_ERR
"INDEX/CREATE INDEX: NONLEAFFILL can be specified only for
\nbtree storage structure."
E_US14CC_5324:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Invalid LEAFFILL/INDEXFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US14CD_5325:SS42000_SYN_OR_ACCESS_ERR
"INDEX/CREATE INDEX: LEAFFILL/INDEXFILL can be specified only for
\nbtree storage structure."
E_US14CE_5326:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nValid options for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US14CF_5327:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Too many options in the WITH clause."
E_US14D0_5328:SS50002_LIMIT_EXCEEDED
"CREATE INDEX: Too many columns in key of index."
E_US14D1_5329:SS42501_COL_NOT_FOUND
"CREATE INDEX: %0c: Key '%1c' is out of sequence or is not in the column
list.  The key list must be a prefix of the column list."
E_US14D3_5331:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: The number of elements in the KEY clause must not be greater
than the number of indexed columns."
E_US14D4_5332:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX: Illegal MINPAGES value '%0d' in a CREATE INDEX command.
The value must be less or equal to the MAXPAGES value."
E_US14D5_5333:SS42000_SYN_OR_ACCESS_ERR
"%0c: MINPAGES can be specified only for
\n[c]hash storage structure."
E_US14D6_5334:SS42000_SYN_OR_ACCESS_ERR
"%0c: MAXPAGES can be specified only for
\n[c]hash storage structure."
E_US14D7_5335:SS42000_SYN_OR_ACCESS_ERR
"%0c: NONLEAFFILL can be specified only for
\n[c]btree storage structure."
E_US14D8_5336:SS42000_SYN_OR_ACCESS_ERR
"%0c: LEAFFILL/INDEXFILL can be specified only for
\n[c]btree storage structure."
E_US14D9_5337:SS42000_SYN_OR_ACCESS_ERR
"%0c: FILLFACTOR can be specified only for
\n[c]btree, [c]isam, [c]hash storage structure."
E_US14DA_5338:SS42000_SYN_OR_ACCESS_ERR
"%0c: Illegal MINPAGES value '%1d' in a %0c command.
The value must be less or equal to the MAXPAGES value."
E_US14DB_5339:SS42000_SYN_OR_ACCESS_ERR
"%0c: KEY can be specified only for
\n[c]btree, [c]isam, [c]hash storage structure."
E_US14DC_5340:SS42000_SYN_OR_ACCESS_ERR
"%0c: '%1c' is either an invalid option or the syntax of\n
        the WITH subclause is invalid."
E_US14DD_5341:SS42000_SYN_OR_ACCESS_ERR
"%0c: Invalid %1c of '%2d' specified.
\nMust be between %3d and %4d inclusively."
E_US14DE_5342:SS42000_SYN_OR_ACCESS_ERR
"%0c: '%1c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nValid options for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL,
\n   ALLOCATION, EXTEND, PAGE_SIZE."
E_US14DF_5343:SS42000_SYN_OR_ACCESS_ERR
"%0c: '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nValid keywords for this type of subclause are: KEY, LOCATION, COMPRESSION."
E_US14E0_5344:SS42000_SYN_OR_ACCESS_ERR
"%0c: Too many options in the WITH clause."
E_US14E1_5345:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nValid keywords for this type of subclause are:
\nLOCATION, OLDLOCATION, NEWLOCATION, COMPRESSION."
E_US14E2_5346:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe only valid keyword for this type of subclause is EXTEND."
E_US14E3_5347:SS42000_SYN_OR_ACCESS_ERR
"%0c clause is an invalid option for %1c into %2c table."
E_US14E4_5348_COPY_NON_FAST:SS42000_SYN_OR_ACCESS_ERR
"Invalid options have been specified for a non fast-load COPY.
\nInvalid options are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL,
\n   ALLOCATION, EXTEND."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has specified options which are only valid for the
**	fast-load version of "COPY", but fast-load is not possible. 
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the statement with valid options.
*/
E_US14E5_5349_MODIFY_NO_WITH:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: the WITH subclause is invalid for 
\n MODIFY to [NO]READONLY"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has specified options which are only valid for the
**	fast-load version of "COPY", but fast-load is not possible. 
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the statement with valid options.
*/
E_US14E6_5350_READONLY_TBL_ERR:SS42000_SYN_OR_ACCESS_ERR
"READONLY:  The user has issued a statement which attempted
\nto write to a READONLY table"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has issued a statement which attempted to write to a 
**	READONLY table. Such statements are "UPDATE", "DELETE", "INSERT",
**	"COPY", "DROP", "MODIFY" (to anything other than [NO]READONLY),
**	"CREATE", "ALTER" and "SET".
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Do not attempt to write to a READONLY table.
*/
E_US14E7_5351_CONCUR_IDX_ERR:SS5000B_INTERNAL_ERROR
"A concurrent index can only be built on a READONLY table"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has attempted to build a concurrent index on a table
**	that is not designated READONLY.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
** 	Either create the index non-concurrent, or modify the table to
**	READONLY, create the concurrent index, then modify the table 
**	back to NOREADONLY.
*/
E_US14E8_5352_DROP_READONLY:SS42000_SYN_OR_ACCESS_ERR
"%0c: Cannot drop '%1c' because it is a READONLY table."
/*%
** Parameters:
**	%0c - statement name
**	%1c - table name
**
** Explanation:
**	READONLY tables cannot be dropped.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To drop the table, modify it to NOREADONLY, then reissue the
**	statement.
*/
E_US14E9_5353_LOG_INCONSIST_TBL_ERR:SS42000_SYN_OR_ACCESS_ERR
"Table and index are logically inconsistent. No operations can be performed 
 \nwith index until it is made consistent."
/*%
** Parameters:
**	None.
**
** Explanation:
**	User has issued a dml statement which cannot be performed on
**	an inconsistent index.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Restore the table from checkpoint or in worst case use modify table
**	to log_consistent.
*/
E_US14EA_5354_PHYS_INCONSIST_TBL_ERR:SS42000_SYN_OR_ACCESS_ERR
"Table and index are physically inconsistent. No operations can be performed 
 \nwith index until it is made consistent."
/*%
** Parameters:
**	none.
**
** Explanation:
**	User has issued a dml statement which cannot be performed on
**	an inconsistent index.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Restore the table from checkpoint or in worst case use modify table
**	to phys_consistent.
*/
E_US14EB_5355_DROP_READONLY_TABLE_INDEX:SS42000_SYN_OR_ACCESS_ERR
"DROP: Cannot drop an index on READONLY table."
/*%
** Parameters:
**      none.
**
** Explanation:
**      Index cannot be dropped on READONLY tables.
**
** System Status:
**      The current statement is aborted; the program continues.
**
** Recommendation:
**      To drop the index, modify the table to NOREADONLY, then reissue the
**      statement.
*/
E_US14EC_5356:SS42000_SYN_OR_ACCESS_ERR
"%0c: Invalid %1c of '%2d' specified."
E_US14ED_5357_INCONSISTENT_INDEX_ERR:SS42000_SYN_OR_ACCESS_ERR
"Table has one or more inconsistent indicies.  Drop or recreate them."
E_US14EE_5358:SS42000_SYN_OR_ACCESS_ERR
"%0c: '%1c' is an invalid option for with clause of copy statement. 
Valid options for this subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL,
\n   ALLOCATION, EXTEND, ROW_ESTIMATE"
/*%
** Parameters:
**      none.
**
** Explanation:
**      User has supplied an invalid option for the with clause of copy statement,
** eg page_size.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
*/
E_US1518_5400:SS5000B_INTERNAL_ERROR
"HELP: Internal errors preclude further operation.  Please report
problem to product vendor."
E_US1519_5401:SS42500_TBL_NOT_FOUND
"HELP: table %0c does not exist."
E_US151A_5402:SS5000H_UNAVAILABLE_RESOURCE
"HELP: cannot find manual section '%0c'."
E_US151B_5403:SS5000A_QUERY_ERROR
"HELP: table %0c is not a view."
E_US151C_5404:SS42503_INSUF_PRIV
"HELP: table %0c has no permissions granted on it."
E_US151D_5405:SS50000_MISC_ERRORS
"HELP: table %0c has no integrity constraints on it."
E_US1522_5410:SS50000_MISC_ERRORS
"HELP: tree buffer overflowed."
E_US1523_5411:SS50002_LIMIT_EXCEEDED
"HELP: tree stack overflowed."
E_US1524_5412:SS50008_UNSUPPORTED_STMT
"The HELP INTEGRITY command is not allowed in distributed INGRES."
E_US1525_5413:SS50008_UNSUPPORTED_STMT
"The HELP PERMIT command is not allowed in distributed INGRES."
E_US1526_5414:SS50008_UNSUPPORTED_STMT
"The HELP VIEW command is not allowed in distributed INGRES."
E_US157C_5500:SS42500_TBL_NOT_FOUND
"MODIFY: table %0c does not exist."
E_US157D_5501:SS42500_TBL_NOT_FOUND
"MODIFY: you do not own table %0c."
E_US157E_5502:SS42000_SYN_OR_ACCESS_ERR
"MODIFY %0c: you may not provide key columns %1c."
E_US157F_5503:SS50002_LIMIT_EXCEEDED
"MODIFY: It is not allowed to list more than 1024 columns in a MODIFY command."
E_US1580_5504:SS42503_INSUF_PRIV
"MODIFY %0c: cannot modify system table."
E_US1581_5505:SS42000_SYN_OR_ACCESS_ERR
"MODIFY %0c: CHECKLINK and PATCHLINK are no longer part of the MODIFY
command. Check the reference manual for details."
E_US1583_5507:SS42502_DUPL_OBJECT
"MODIFY %0c: duplicate key '%1c'."
E_US1584_5508:SS50002_LIMIT_EXCEEDED
"MODIFY %0c: key width (%1d) too large for ISAM; maximum size is %2d."
E_US1586_5510:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Invalid option '%0c'.
\n Try one of the following:
\n   [C]ISAM, [C]HASH, [C]HEAP, [C]HEAPSORT, [C]BTREE, CLUSTERED, MERGE,
\n   TRUNCATED, REORGANIZE, REORGANISE, RELOCATE, [NO]PERSISTENCE"
E_US1587_5511:SS42501_COL_NOT_FOUND
"MODIFY %0c: bad column name '%1c'."
E_US1588_5512:SS42000_SYN_OR_ACCESS_ERR
"MODIFY %0c: '%1c' not allowed or specified more than once."
E_US1589_5513:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal FILLFACTOR '%0d' in a MODIFY command.  The value
must be between 1 and 100, inclusive."
E_US158A_5514:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal MINPAGES value '%0d' in a MODIFY command.
The value must be greater than zero."
E_US158B_5515:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: column keys must not be specified unless storage
structure is present."
E_US158C_5516:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal MAXPAGES value '%0d' in a MODIFY command.
The value must be greater than zero."
E_US158D_5517:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal MINPAGES value '%0d' in a MODIFY command.
The value must be less or equal to the MAXPAGES value."
E_US158E_5518:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: invalid sequence specifier '%0c' for column '%1c'."
E_US158F_5519:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: %0c is a view and can't be modified."
E_US1590_5520:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: %0c: sequence specifier '%1c' on column %2c is not allowed
with the specified storage structure."
E_US1591_5521:SS5000R_RUN_TIME_LOGICAL_ERROR
"MODIFY: table could not be modified because rows contain duplicate keys."
E_US1592_5522:SS5000R_RUN_TIME_LOGICAL_ERROR
"INDEX: table could not be indexed because rows contain duplicate keys."
E_US1593_5523:SS50003_EXHAUSTED_RESOURCE
"MODIFY: %0c: ran out of disk space while performing modify."
E_US1594_5524:SS50002_LIMIT_EXCEEDED
"MODIFY: %0c: key is %1d bytes; maximum size key for btrees is %2d bytes."
E_US1595_5525:SS5000A_QUERY_ERROR
"MODIFY: %0c: table is not a btree; only a btree table can be modified to
merge."
E_US1596_5526:SS5000A_QUERY_ERROR
"MODIFY: %0c: all columns except the 'tidp' column should be part of the
key in a btree secondary index."
E_US1597_5527:SS5000A_QUERY_ERROR
"MODIFY: %0c: compressed row size can exceed maximum row width."
E_US1598_5528:SS5000A_QUERY_ERROR
"MODIFY: May not alter key sequence of secondary index table."
E_US1599_5529:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL,
\n   ALLOCATION, EXTEND, TABLE_OPTION."
E_US159A_5530:SS50008_UNSUPPORTED_STMT
"The MODIFY command is not allowed in distributed INGRES."
E_US159B_5531:SS5000A_QUERY_ERROR
"MODIFY: %0c: The '%1c' entry in the WHERE or WITH clause
is incompatible with the storage structure."
E_US159C_5532:SS5000A_QUERY_ERROR
"INDEX: %0c: The '%1c' entry in the WITH clause
is incompatible with the storage structure."
E_US159D_5533:SS5000B_INTERNAL_ERROR
"MODIFY: %0c: Can't change the storage structure of this catalog."
E_US159E_5534:SS42000_SYN_OR_ACCESS_ERR
"INDEX: Unrecognized parameter '%0c' in the WITH clause.
 Use one of the following.
\n              STRUCTURE, KEY, FILLFACTOR, MINPAGES, MAXPAGES,
\n              LEAFFILL, INDEXFILL, NONLEAFFILL, MAXINDEXFILL"
E_US159F_5535:SS42000_SYN_OR_ACCESS_ERR
"INDEX: Unrecognized storage structure '%0c' in an INDEX command.
 Please use one of the following:
\n          [C]ISAM, [C]HASH, [C]BTREE"
E_US15A0_5536:SS5000A_QUERY_ERROR
"MODIFY: %0c: maximum size of a compressed row could exceed maximum
row width. Remember that the compressed C type uses an extra byte
and the compressed CHAR type uses two extra."
E_US15A1_5537:SS50002_LIMIT_EXCEEDED
"INDEX: %0c: key width (%1d) too large for ISAM; maximum size is %2d."
E_US15A2_5538:SS50002_LIMIT_EXCEEDED
"INDEX: %0c: index entry is %1d bytes; maximum size index entry for btrees
is %2d bytes."
E_US15A3_5539:SS50002_LIMIT_EXCEEDED
"INDEX: %0c: maximum size of a compressed row could exceed maximum
row width. Remember that the compressed C type uses an extra byte."
E_US15A4_5540:SS5000A_QUERY_ERROR
"MODIFY: %0c: A secondary index may not be modified to %1c."
E_US15A5_5541:SS5000A_QUERY_ERROR
"MODIFY: %0c: It is not allowed to modify a secondary index with
\nnon-unique keys to one with unique keys.  You can destroy and
\nre-create the index with unique keys."
E_US15A6_5542:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: The RELOCATE variant requires new and old location lists,
they must be of equal length."
E_US15A7_5543:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: The LOCATION with-option may only be used with MODIFY TO
storage-structure, RECONSTRUCT, or REORGANIZE variants."
E_US15A8_5544:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: line %0d, The WITH clause of the RELOCATE variant must have
only NEWLOCATION and OLDLOCATION subclauses."
E_US15A9_5545:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: line %0d, LOCATION keyword not allowed in the RELOCATE variant.
Use NEWLOCATION or OLDLOCATION instead."
E_US15AA_5546:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: line %0d, '%1c' can be only used with RELOCATE variant."
E_US15AB_5547:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: '%0c' option has already been specified."
E_US15AC_5548:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: REORGANIZE variant requires location(s) to be specified."
E_US15AD_5549:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: line %0d, '%1c' option cannot be used with REORGANIZE variant."
E_US15AE_5550:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal ALLOCATION value '%0d' in a MODIFY command."
E_US15AF_5551:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal EXTEND value '%0d' in a MODIFY command."
E_US15B0_5552:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: Illegal TABLE_OPTION value '%0d' in a MODIFY command."
E_US15B1_5553:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX : Illegal ALLOCATION value '%0d' in a INDEX command."
E_US15B2_5554:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX : Illegal EXTEND value '%0d' in a INDEX command."
E_US15B3_5555:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: Illegal ALLOCATION value '%0d' in a CREATE TABLE command."
E_US15B4_5556:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: Illegal EXTEND value '%0d' in a CREATE TABLE command."
E_US15B5_5557:SS42000_SYN_OR_ACCESS_ERR
"MODIFY or ALTER TABLE: %0c is a gateway table and cannot be modified."
/*%
** Parameters:
**	%0c - gateway table name
**
** Explanation:
**	The user entered a "MODIFY" or "ALTER TABLE tbl ADD | DROP COLUMN"
**	statement on a gateway table.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	To change the structure of a gateway table, change the structure
**	in the underlying gateway/DBMS, then reissue the "REGISTER...IMPORT"
**	command, specifying the appropriate "STRUCTURE" clause.
*/
E_US15B6_5558:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: '%0c' is either an invalid option or the syntax of\n
        the WITH subclause is invalid.
\nUNIQUE_SCOPE is the only valid keyword for this type of subclause."
E_US15B7_5559:SS42000_SYN_OR_ACCESS_ERR
"CREATE TABLE: column '%0c' can not be defined as a non-nullable\n
	datatype with a default value of null."
E_US15B8_5560:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: attempt to alter unknown column '%0c'."
E_US15E0_5600:SS5000A_QUERY_ERROR
"Save: Can't save system table '%0c'."
E_US15E1_5601:SS5000A_QUERY_ERROR
"Save: bad month '%0c'."
E_US15E2_5602:SS5000A_QUERY_ERROR
"Save: bad day '%0d'."
E_US15E3_5603:SS5000A_QUERY_ERROR
"Save: bad year '%0d'."
E_US15E4_5604:SS42500_TBL_NOT_FOUND
"Save: table '%0c' does not exist or is not owned by you."
E_US15E5_5605:SS50008_UNSUPPORTED_STMT
"The SAVE command is not allowed in distributed INGRES."
E_US16A8_5800:SS42500_TBL_NOT_FOUND
"COPY: table '%0c' doesn't exist or is not owned by you."
E_US16A9_5801:SS42501_COL_NOT_FOUND
"COPY: column '%0c' in table '%1c' doesn't exist or it has been listed twice."
E_US16AB_5803:SS50002_LIMIT_EXCEEDED
"COPY: too many columns."
E_US16AC_5804:SS42000_SYN_OR_ACCESS_ERR
"COPY: bad length for column '%0c'.  Length = '%1c'."
E_US16AD_5805:SS50000_MISC_ERRORS
"COPY: can't open file '%0c'."
E_US16AE_5806:SS50000_MISC_ERRORS
"COPY: can't create file '%0c'."
E_US16AF_5807:SS42000_SYN_OR_ACCESS_ERR
"COPY: unrecognizable dummy domain '%0c'."
E_US16B0_5808:SS5000A_QUERY_ERROR
"COPY: domain '%0c', size too small for conversion.
There were %2c rows successfully copied from '%3c' into '%4c'."
E_US16B1_5809:SS22000_DATA_EXCEPTION
"COPY: bad input string for domain '%0c'. Input was '%1c'.
There were %2c rows successfully copied from '%3c' into '%4c'."
E_US16B2_5810:SS50000_MISC_ERRORS
"COPY: unexpected end of file while filling domain '%0c'."
E_US16B3_5811:SS42000_SYN_OR_ACCESS_ERR
"COPY: bad type for column '%0c'. Type = '%1c'."
E_US16B4_5812:SS5000A_QUERY_ERROR
"COPY: The table '%0c' has a secondary index.
The index(es) must be destroyed before doing a 'copy from'."
E_US16B5_5813:SS42503_INSUF_PRIV
"COPY: You are not allowed to update the table '%0c'."
E_US16B6_5814:SS42503_INSUF_PRIV
"COPY: You do not own the table '%0c'."
E_US16B7_5815:SS50000_MISC_ERRORS
"COPY: An unterminated 'c0' or 'text(0)' field occurred while filling domain
'%0c'."
E_US16B8_5816:SS42000_SYN_OR_ACCESS_ERR
"COPY: The full pathname must be specified for the file '%0c'."
E_US16B9_5817:SS50002_LIMIT_EXCEEDED
"COPY: The maximum width of the output file cannot exceed 2008 bytes per row."
E_US16BA_5818:SS5000A_QUERY_ERROR
"COPY: '%0c' is a view and can't be copied."
E_US16BB_5819:SS01000_WARNING
"COPY: Warning: %0c duplicate rows were ignored."
E_US16BC_5820:SS01000_WARNING
"COPY: Warning: %0c domains had control characters which were converted
to blanks."
E_US16BD_5821:SS01000_WARNING
"COPY: Warning: %0c c0 or text(0) character domains were truncated."
E_US16BE_5822:SS23000_CONSTR_VIOLATION
"COPY: Table '%0c' is protected."
E_US16BF_5823:SS42000_SYN_OR_ACCESS_ERR
"COPY: Copy Into a text file must have 'd0nl' delimiter as the last domain."
E_US16C0_5824:SS42000_SYN_OR_ACCESS_ERR
"COPY: '%0c' is an invalid filetype.
\nFiletype must be 'text', 'binary', or 'variable'."
E_US16C1_5825:SS5000A_QUERY_ERROR
"COPY: The filetype must not be 'binary' when any of the columns are 'text(0)'."
E_US16C2_5826:SS22000_DATA_EXCEPTION
"COPY: Error encountered while processing row %0c."
E_US16C3_5827:SS22021_CHAR_NOT_IN_RPRTR
"COPY: Invalid character value (numeric value %0d) found while
processing row %1c."
E_US16C4_5828:SS50008_UNSUPPORTED_STMT
"The COPY command is not allowed in distributed INGRES."
E_US16C5_5829:SS50002_LIMIT_EXCEEDED
"COPY: Computed row length exceeds allowable row length."
E_US16C6_5830:SS5000A_QUERY_ERROR
"COPY: Column '%0c', cannot convert type '%1c' to type '%2c'."
E_US16C7_5831:SS42000_SYN_OR_ACCESS_ERR
"COPY: Column '%0c', bad delimiter '%1c'."
E_US16C8_5832:SS42000_SYN_OR_ACCESS_ERR
"COPY: Column '%0c', delimiter not allowed.  Use type 'd0' to specify
delimiter."
E_US16C9_5833:SS42000_SYN_OR_ACCESS_ERR
"COPY: Column '%0c', illegal column type"
E_US16CA_5834:SS5000B_INTERNAL_ERROR
"COPY: Internal error initializing COPY."
E_US16CB_5835:SS5000B_INTERNAL_ERROR
"COPY: internal error processing row %0c."
E_US16CC_5836:SS5000B_INTERNAL_ERROR
"COPY: Unexpected END OF FILE while processing row %0c."
E_US16CD_5837:SS50003_EXHAUSTED_RESOURCE
"COPY: Error allocating memory while processing row %0c."
E_US16CE_5838:SS5000A_QUERY_ERROR
"COPY: underlying LDB object for '%0c' is a view.\n
You may not use COPY on views."
E_US16D0_5840:SS01000_WARNING
"COPY: Warning: Copy completed with %0c warnings.  %1c rows successfully
copied."
E_US16D1_5841:SS5000B_INTERNAL_ERROR
"COPY: Copy terminated abnormally.  %0c rows successfully copied."
E_US16D2_5842:SS5000B_INTERNAL_ERROR
"COPY: Copy has been aborted."
E_US16D3_5843:SS5000B_INTERNAL_ERROR
"COPY: %0c rows could not be processed."
E_US16D4_5844:SS00000_SUCCESS
"COPY: %0c rows were written to the Copy Log File '%1c'."
E_US16D5_5845:SS42500_TBL_NOT_FOUND
"Table specified no longer exists."
E_US16D6_5846:SS42502_DUPL_OBJECT
"Table specified already exists."
E_US16D7_5847:SS50000_MISC_ERRORS
"COPY: Error writing to Copy Log File '%0c' while processing row %1c."
E_US16D8_5848:SS50000_MISC_ERRORS
"COPY: Error closing Copy Log File '%0c'."
E_US16D9_5849:SS50000_MISC_ERRORS
"COPY: Error closing Copy File '%0c'."
E_US16DA_5850:SS22000_DATA_EXCEPTION
"COPY: Bad data value for column '%0c' encountered while processing row %1c."
E_US16DB_5851:SS42000_SYN_OR_ACCESS_ERR
"COPY: Invalid Copy option '%0c'."
E_US16DC_5852:SS42000_SYN_OR_ACCESS_ERR
"COPY: Invalid Copy option value '%0c'."
E_US16DD_5853:SS42000_SYN_OR_ACCESS_ERR
"COPY: Ambiguous Copy options."
E_US16DE_5854:SS50000_MISC_ERRORS
"COPY: Error opening Copy Log File '%0c'."
E_US16DF_5855:SS50003_EXHAUSTED_RESOURCE
"COPY: Error allocating memory while initializing Copy."
E_US16E0_5856:SS50003_EXHAUSTED_RESOURCE
"COPY: Error allocating memory while processing row %0c."
E_US16E1_5857:SS22000_DATA_EXCEPTION
"COPY: Error processing row %0c.  Cannot convert column '%1c' to tuple format."
E_US16E2_5858:SS22000_DATA_EXCEPTION
"COPY: Error processing row %0c.  Cannot convert column '%1c' to row format."
E_US16E3_5859:SS50000_MISC_ERRORS
"COPY: Error writing to Copy File while processing row %0c."
E_US16E4_5860:SS50000_MISC_ERRORS
"COPY: Error reading row from Application while processing row %0c."
E_US16E5_5861:SS50000_MISC_ERRORS
"COPY: Error writing row to Application while processing row %0c."
E_US16E6_5862:SS5000A_QUERY_ERROR
"COPY: NULL specification not allowed for the dummy column '%0c'."
E_US16E7_5863:SS5000A_QUERY_ERROR
"COPY: Column '%0c', NULL without null value may not be used with variable
length format."
E_US16E8_5864:SS5000A_QUERY_ERROR
"COPY: Column '%0c', NULL specification missing.  Cannot copy rows
containing nulls."
E_US16E9_5865:SS5000A_QUERY_ERROR
"COPY: Column '%0c', NULL value not compatible with format specification."
E_US16EA_5866:SS50000_MISC_ERRORS
"A copy command is currently in progress."
E_US16EB_5867:SS50000_MISC_ERRORS
"An error in the copy command occurred."
E_US170C_5900:SS42000_SYN_OR_ACCESS_ERR
"SET: %0c is not a valid set function."
E_US170D_5901:SS42000_SYN_OR_ACCESS_ERR
"SET JOURNALING: Set journaling does not have any parameters."
E_US170E_5902:SS50000_MISC_ERRORS
"SET JOURNALING: Tables created during session will be logged."
E_US170F_5903:SS50000_MISC_ERRORS
"SET JOURNALING: Tables create during session will NOT be logged."
E_US1710_5904:SS42000_SYN_OR_ACCESS_ERR
"SET: Parameter must be constant or string name."
E_US1712_5906:SS42503_INSUF_PRIV
"SET TRACE: You do not have permission to set trace flags."
E_US1713_5907:SS42000_SYN_OR_ACCESS_ERR
"SET TRACE: Trace flag specification too long."
E_US1714_5908:SS42000_SYN_OR_ACCESS_ERR
"SET JOURNAL_SIZE: This function no longer available."
E_US1715_5909:SS50000_MISC_ERRORS
"SET JOURNALING: Journaling not enabled on this database."
E_US1716_5910:SS42000_SYN_OR_ACCESS_ERR
"SET: This set command takes only a single argument (in quotes)."
E_US1717_5911:SS42000_SYN_OR_ACCESS_ERR
"SET: An integer argument is required (in quotes)."
E_US1718_5912:SS42000_SYN_OR_ACCESS_ERR
"SET: The merge factor must be between 3 and 15 inclusive."
E_US1719_5913:SS42000_SYN_OR_ACCESS_ERR
"SET: The sort buffer size must be between 1000 and 1000000 inclusive."
E_US171A_5914:SS42000_SYN_OR_ACCESS_ERR
"SET: The freespace buffer size must be between 1000 and 1000000 inclusive."
E_US171B_5915:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized storage structure in a SET RET_INTO command.
Please use one of the following:
\n
\n          HEAP, CHEAP, HEAPSORT, CHEAPSORT, HASH, CHASH, ISAM, CISAM,
\n          BTREE, CBTREE"
E_US171C_5916:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized storage structure in a SET RESULT_STRUCTURE command.
Please use one of the following:
\n
\n          HEAP, CHEAP, HEAPSORT, CHEAPSORT, HASH, CHASH, ISAM, CISAM,
\n          BTREE, CBTREE"
E_US1722_5922:SS25000_INV_XACT_STATE
"While in a multi-query transaction, the only WHERE clause parameter
of SET LOCKMODE that can be specified is TIMEOUT."
E_US1723_5923:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized value for the ISOLATION LEVEL parameter in a SET SESSION
command.  Please use one of the following:
\n
\n          SERIALIZABLE, REPEATABLE READ, READ COMMITTED,
\n          READ UNCOMMITTED, SYSTEM"
E_US1724_5924:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized value for the LEVEL parameter in a SET LOCKMODE
command.  Please use one of the following:
\n
\n          ROW, PAGE, TABLE, SESSION, SYSTEM"
E_US1725_5925:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized value for the READLOCK parameter of a
SET LOCKMODE command.  Please use one of the following:
\n
\nNOLOCK, SHARED, EXCLUSIVE, SESSION, SYSTEM"
E_US1726_5926:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized value for the MAXLOCKS parameter of a
SET LOCKMODE command.  Please use one of the following:
\n
\n          a number, SESSION, SYSTEM"
E_US1727_5927:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized value for the TIMEOUT parameter of a SET LOCKMODE
command.  Please use one of the following:
\n
\n          a number, SESSION, SYSTEM, NOWAIT"
E_US1728_5928:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized parameter '%0c' in a SET LOCKMODE command.
Please use one of the following:
\n
\n          LEVEL, READLOCK, MAXLOCKS, TIMEOUT"
E_US1729_5929:SS42506_INVALID_IDENTIFIER
"SET LOCKMODE: Invalid table name in a set command."
E_US172A_5930:SS42000_SYN_OR_ACCESS_ERR
"SET LOCKMODE: Cannot set session lock default to 'session'."
E_US172B_5931:SS42000_SYN_OR_ACCESS_ERR
"SET LOCKMODE: Too many lockmode parameters."
E_US172C_5932:SS25000_INV_XACT_STATE
"SET TRANSACTION: This statement is not allowed 
\nin a multi-query transaction."
E_US172D_5933:SS50008_UNSUPPORTED_STMT
"The SET [NO]JOINOP command is no longer supported.
All queries now use JOINOP."
E_US172E_5934:SS42000_SYN_OR_ACCESS_ERR
"Bad date format '%0c' in a SET DATE_FORMAT command.
Please choose from one of the following:
\n
\n  \"US\", \"MULTINATIONAL\", \"ISO\", \"FINLAND\", \"SWEDEN\" | \"GERMAN\""
E_US172F_5935:SS42000_SYN_OR_ACCESS_ERR
"Bad money format '%0c' in a SET MONEY_FORMAT command.
Please use the following form:
\n
\n          \"L:xxxx\" or \"T:xxxx\"
\n
where xxxx is a zero to four character currency symbol.
L indicates a leading currency symbol, and T a trailing one.
\n
Example: \"T:DM\" would result in the value 10.85 displayed as '10.85 DM'"
E_US1730_5936:SS42000_SYN_OR_ACCESS_ERR
"Bad decimal indicator '%0c' in a SET DECIMAL command.
Please use either a comma (\",\") or a period (\".\")."
E_US1731_5937:SS42000_SYN_OR_ACCESS_ERR
"Bad precision '%0d' in a SET MONEY_PREC command.  Money precision
determines the number of digits displayed after the decimal indicator
in a money value.  Allowed precision values are '0', '1', or '2'
\n
\n  '0' indicates no digits after the decimal indicator (e.g., 10)
\n  '1' indicates one digit (e.g. 10.8)
\n  '2' indicates two digits (e.g. 10.85)"
E_US1732_5938:SS42000_SYN_OR_ACCESS_ERR
"SET CACHE: Invalid cache size.  Legal range is 0 - 500.  Setting the
size of the cache to a larger size than 500 will result in a cache size of 500."
E_US1733_5939:SS25000_INV_XACT_STATE
"SET CACHE: You may not issue a SET CACHE command in a multi-query transaction."
E_US1734_5940:SS50008_UNSUPPORTED_STMT
"The SET J_FREESZ1 command is no longer supported.
This function is now done automatically by the system."
E_US1735_5941:SS50008_UNSUPPORTED_STMT
"The SET J_FREESZ2 command is no longer supported.
This function is now down automatically by the system."
E_US1736_5942:SS50008_UNSUPPORTED_STMT
"The SET J_FREESZ3 command is no longer supported.
This function is now done automatically by the system."
E_US1737_5943:SS50008_UNSUPPORTED_STMT
"The SET J_FREESZ4 command is no longer supported.
This function is now done automatically by the system."
E_US1738_5944:SS50008_UNSUPPORTED_STMT
"The SET AGGREGATE command is not allowed in distributed INGRES."
E_US1739_5945:SS50008_UNSUPPORTED_STMT
"The SET CPUFACTOR command is not allowed in distributed INGRES."
E_US173A_5946:SS50008_UNSUPPORTED_STMT
"The SET DATE_FORMAT command is not allowed in distributed INGRES."
E_US173B_5947:SS50008_UNSUPPORTED_STMT
"The SET [NO]DEADLOCK command is no longer supported."
E_US173C_5948:SS50008_UNSUPPORTED_STMT
"The SET DECIMAL command is not allowed in distributed INGRES."
E_US173D_5949:SS50008_UNSUPPORTED_STMT
"The SET EMBEDDED and SET EQUEL commands are not allowed in distributed INGRES."
E_US173E_5950:SS50008_UNSUPPORTED_STMT
"The SET [NO]JOINOP command is not allowed in distributed INGRES."
E_US173F_5951:SS50008_UNSUPPORTED_STMT
"The SET [NO]JOURNALING command is not allowed in distributed INGRES."
E_US1740_5952:SS50008_UNSUPPORTED_STMT
"The SET LOCKMODE command is not allowed in distributed INGRES."
E_US1741_5953:SS50008_UNSUPPORTED_STMT
"The SET MONEY_FORMAT command is not allowed in distributed INGRES."
E_US1742_5954:SS50008_UNSUPPORTED_STMT
"The SET MONEY_PREC command is not allowed in distributed INGRES."
E_US1743_5955:SS50008_UNSUPPORTED_STMT
"The SET QBUFSIZE command is not longer supported.
This function is now done automatically by the system."
E_US1744_5956:SS50008_UNSUPPORTED_STMT
"The SET RET_INTO command in not allowed in distributed INGRES."
E_US1745_5957:SS50008_UNSUPPORTED_STMT
"The SET SORTBUFSIZE command is not allowed in distributed INGRES."
E_US1746_5958:SS50008_UNSUPPORTED_STMT
"The SET SQL command is not allowed in distributed INGRES."
E_US1747_5959:SS50008_UNSUPPORTED_STMT
"The SET [NO]STATISTICS command is not allowed in distributed INGRES."
E_US1748_5960:SS50008_UNSUPPORTED_STMT
"The SET [NO]TRACE command is not allowed in distributed INGRES."
E_US1749_5961:SS50008_UNSUPPORTED_STMT
"The CREATE/DEFINE VIEW command is not allowed in distributed INGRES."
E_US174A_5962:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized parameter '%0c' given in a SET [NO]TRACE command.
Please use one of the following:
\n
\n          TERMINAL, INPUT, OUTPUT, POINT"
E_US174B_5963:SS42000_SYN_OR_ACCESS_ERR
"It is not allowed to give a file name with the SET NOTRACE TERMINAL command."
E_US174C_5964:SS42000_SYN_OR_ACCESS_ERR
"It is not allowed to give a file name with the SET NOTRACE INPUT
 and SET NOTRACE OUTPUT commands."
E_US174D_5965:SS42000_SYN_OR_ACCESS_ERR
"The SET [NO]TRACE command must have a facility code and flag number."
E_US174E_5966:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized facility code in SET [NO]TRACE %0c.
Please use one of the following:
\n
\n          DM - Data Manipulation Facility
\n          PS - Parser Facility
\n          SC - System Control Facility
\n          RD - Relation Description Facility
\n          QE - Query Execution Facility
\n          OP - Optimizer Facility
\n          QS - Query Storage Facility
\n          AD - Abstract Datatype Facility
\n          GW - Gateway Facility
\n          SX - Security Extensions Facility"
E_US174F_5967:SS42000_SYN_OR_ACCESS_ERR
"Bad flag number in SET [NO]TRACE %0c.  It must be
between 0 and 9999, inclusive."
E_US1750_5968:SS42000_SYN_OR_ACCESS_ERR
"Integer parameters are not allowed in the
SET [NO]TRACE TERMINAL, SET [NO]TRACE OUTPUT,
and SET [NO]TRACE INPUT commands."
E_US1751_5969:SS42000_SYN_OR_ACCESS_ERR
"In a SET command, the non-integer '%0c' was found
where an integer was expected."
E_US1752_5970:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized scope '%0c' in a SET LOCKMODE command.
Please use one of the following:
\n
\n          SESSION, ON tablename"
E_US1753_5971:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized parameter in a SET AGGREGATE command.
Please use one of the following:
\n
\n          PROJECT, NOPROJECT"
E_US1754_5972:SS50008_UNSUPPORTED_STMT
"The SET QUERY_SIZE command is no longer supported.
This function is now done automatically by the system."
E_US1755_5973:SS42000_SYN_OR_ACCESS_ERR
"No file name was given for a 'SET TRACE INPUT' or 'SET TRACE OUTPUT' command."
E_US1756_5974:SS50008_UNSUPPORTED_STMT
"The SET AUTOCOMMIT command is not supported in QUEL."
E_US1757_5975:SS50008_UNSUPPORTED_STMT
"The SET CACHE command is no longer supported."
E_US1758_5976:SS50008_UNSUPPORTED_STMT
"The SET J_SORBUFSZ command is no longer supported."
E_US1759_5977:SS50008_UNSUPPORTED_STMT
"The SET RESULT_STRUCTURE command is no longer supported."
E_US175A_5978:SS50008_UNSUPPORTED_STMT
"The SET RET_INTO command is no longer supported."
E_US175B_5979:SS50008_UNSUPPORTED_STMT
"The SET AGGREGATE command is no longer supported."
E_US175C_5980:SS50008_UNSUPPORTED_STMT
"The SET JOINOP [NO]TIMEOUT command is not allowed in distributed INGRES."
E_US175D_5981:SS42000_SYN_OR_ACCESS_ERR
"SET JOINOP: invalid option '%0c'. Must be [NO]TIMEOUT."
E_US175E_5982:SS50008_UNSUPPORTED_STMT
"The SET J_SORTBUFSZ command is no longer supported."
E_US175F_5983:SS42000_SYN_OR_ACCESS_ERR
"SET AUTOCOMMIT: invalid option '%0c'. Must be ON or OFF."
E_US1760_5984:SS50000_MISC_ERRORS
"A compressed table is not accessible due to a change in the\n
tuple compression routines. MODIFYing the table to a compressed\n
structure under this version makes it accessible once again."
E_US1761_5985:SS42000_SYN_OR_ACCESS_ERR
"SET DDL_CONCURRENCY: invalid option '%0c'.
\nMust be ON or OFF."
E_US1762_5986:SS50008_UNSUPPORTED_STMT
"'SET LOCKMODE' on views is not supported in distributed INGRES"
E_US1763_5987:SS50008_UNSUPPORTED_STMT
"SET LOCKMODE SESSION: parameter '%0c' is not supported
\n    in distributed INGRES"
E_US1764_5988:SS50000_MISC_ERRORS
"SET TRACE : Can't open file or terminal '%0c' for set trace command."
E_US1765_5989:SS42000_SYN_OR_ACCESS_ERR
"Unsupported option in the WITH clause."
E_US1766_5990:SS42000_SYN_OR_ACCESS_ERR
"Contradictory options in the WITH clause."
E_US1839_6201:SS42500_TBL_NOT_FOUND
"RELOCATE: Table '%0c' doesn't exist."
E_US183A_6202:SS50000_MISC_ERRORS
"RELOCATE: Table '%0c' can not be relocated."
E_US183B_6203:SS5000A_QUERY_ERROR
"RELOCATE: Locationname '%0c' is not valid for this data base."
E_US183C_6204:SS42500_TBL_NOT_FOUND
"RELOCATE: Table '%0c' does not belong to you."
E_US183D_6205:SS5000A_QUERY_ERROR
"RELOCATE: Table '%0c' is a view."
E_US183E_6206:SS50008_UNSUPPORTED_STMT
"The RELOCATE command is not allowed in distributed INGRES."
E_US183F_6207:SS5000A_QUERY_ERROR
"RELOCATE: To relocate table '%0c', which resides in multiple locations,
use RELOCATE variant of the MODIFY command.
\nRELOCATE command can only be used on a table residing in a single location."
E_US185B_6235:SS50008_UNSUPPORTED_STMT
"The SET [NO]MAX... statements are not allowed in distributed INGRES.\n
Enter this command only under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered a "SET [NO]MAX..." statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Issue the command only under a local DBMS.
*/
E_US185C_6236:SS42000_SYN_OR_ACCESS_ERR
"You cannot specify a value on SET NOMAX... statements.\n
Reissue the statement with no value specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified a value on a "SET NOMAX..." statement.
**	This is a syntax error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the statement with no value specified.
*/
E_US185D_6237:SS42000_SYN_OR_ACCESS_ERR
"You must specify a non-negative integral value on SET MAX... statements.\n
Reissue the statement with a non-negative integral value specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user omitted a value on a "SET MAX..." statement, or an
**	invalid value was specified.  This is a syntax error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the statement with a valid value specified.
*/
E_US1860_EXCESSIVE_MAXCOST:SS42503_INSUF_PRIV
"The value specified on the SET MAXCOST statement (%0d)\n
exceeds your session's QUERY_COST_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXCOST to reset MAXCOST to your limit."
/*%
** Parameters:
**	%0d - value specified on the "SET MAXCOST" statement
**	%1d - session's value for QUERY_COST_LIMIT
**
** Explanation:
**	The user specified a value on the "SET MAXCOST" statement that was
**	greater than the session's QUERY_COST_LIMIT. This is not allowed.
**
** System Status:
**	The session's value for MAXCOST is not adjusted.
**
** Recommendation:
**	Reissue the statement with a value less than or equal to the
**	session's QUERY_COST_LIMIT.
*/
E_US1861_EXCESSIVE_MAXCPU:SS42503_INSUF_PRIV
"The value specified on the SET MAXCPU statement (%0d)\n
exceeds your session's QUERY_CPU_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXCPU to reset MAXCPU to your limit."
/*%
** Parameters:
**	%0d - value specified on the "SET MAXCPU" statement
**	%1d - session's value for QUERY_CPU_LIMIT
**
** Explanation:
**	The user specified a value on the "SET MAXCPU" statement that was
**	greater than the session's QUERY_CPU_LIMIT. This is not allowed.
**
** System Status:
**	The session's value for MAXCPU is not adjusted.
**
** Recommendation:
**	Reissue the statement with a value less than or
**	equal to the session's QUERY_CPU_LIMIT.
*/
E_US1862_EXCESSIVE_MAXIO:SS42503_INSUF_PRIV
"The value specified on the SET MAXIO/MAXQUERY statement (%0d)\n
exceeds your session's QUERY_IO_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXIO to reset MAXIO/MAXQUERY to your limit."
/*%
** Parameters:
**	%0d - value specified on the "SET MAXIO/MAXQUERY" statement
**	%1d - session's value for QUERY_IO_LIMIT
**
** Explanation:
**	The user specified a value on the "SET MAXIO/MAXQUERY" statement
**	that was greater than the session's QUERY_IO_LIMIT. This is not
**	allowed.
**
** System Status:
**	The session's value for MAXIO is not adjusted.
**
** Recommendation:
**	Reissue the statement with a value less than or equal to the
**	session's QUERY_IO_LIMIT.
**
*/
E_US1863_EXCESSIVE_MAXPAGE:SS42503_INSUF_PRIV
"The value specified on the SET MAXPAGE statement (%0d)\n
exceeds your session's QUERY_PAGE_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXPAGE to reset MAXPAGE to your limit."
/*%
** Parameters:
**	%0d - value specified on the "SET MAXPAGE" statement
**	%1d - session's value for QUERY_PAGE_LIMIT
**
** Explanation:
**	The user specified a value on the "SET MAXPAGE" statement that was
**	greater than the session's QUERY_PAGE_LIMIT.  This is not allowed.
**
** System Status:
**	The session's value for MAXPAGE is not adjusted.
**
** Recommendation:
**	Reissue the statement with a value less than or equal to the
**	session's QUERY_PAGE_LIMIT.
*/
E_US1864_EXCESSIVE_MAXROW:SS42503_INSUF_PRIV
"The value specified on the SET MAXROW statement (%0d)\n
exceeds your session's QUERY_ROW_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXROW to reset MAXROW to your limit."
/*%
** Parameters:
**	%0d - value specified on the "SET MAXROW" statement
**	%1d - session's value for QUERY_ROW_LIMIT
**
** Explanation:
**	The user specified a value on the "SET MAXROW" statement that was
**	greater than the session's QUERY_ROW_LIMIT. This is not allowed.
**
** System Status:
**	The session's value for MAXROW is not adjusted.
**
** Recommendation:
**	Reissue the statement with a value less than or equal to the
**	session's QUERY_ROW_LIMIT.
*/
E_US1865_6245:SS42503_INSUF_PRIV
"You are not allowed to create tables in this database.\n
Contact your Database Administrator for further information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to create a permanent table, but the session
**	does not have CREATE_TABLE privileges on the active database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact the database administrator to get CREATE_TABLE privileges.
**	NOTE: This privilege may be granted to one of several authorization
**	identifiers that apply to a session, including the ROLE, GROUP, and
**	USER identifiers, as well as PUBLIC.
*/
E_US1866_6246:SS42503_INSUF_PRIV
"You are not allowed to create database procedures in this database.\n
Contact your Database Administrator for further information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the [CREATE] PROCEDURE command, but the
**	session does not have CREATE_PROCEDURE privileges on the active
**	database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact the database administrator to get CREATE_PROCEDURE privileges.
**	NOTE: This privilege may be granted to one of several authorization
**	identifiers that apply to a session, including the ROLE, GROUP, and
**	USER identifiers, as well as PUBLIC.
*/
E_US1867_6247:SS42503_INSUF_PRIV
"You are not allowed to specify the SET LOCKMODE statement\n
while connected to this database.\n
Contact your Database Administrator for further information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the SET LOCKMODE command, but the session
**	does not have LOCKMODE privileges on the active database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact the database administrator to get LOCKMODE privileges.
**	NOTE: This privilege may be granted to one of several authorization
**	identifiers that apply to a session, including the ROLE, GROUP, and
**	USER identifiers, as well as PUBLIC.
*/
E_US1868_6248:SS5000A_QUERY_ERROR
"One or more users have group '%0c' as a default group.\n
You cannot drop a group (or all users from a group)\n
if the group is the default for any user.\n
Use ACCESSDB to remove this group as default and try again."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to drop a group, or all members from a group,
**	but one or more users have the group as a default group.
**
** System Status:
**	The statement is not aborted; subsequent groups are processed.
**
** Recommendation:
**	Make sure the group was properly entered. If so, use accessdb to
**	change the default group for all users who have this group
**	as their default.
*/
E_US1869_6249:SS5000A_QUERY_ERROR
"Group '%0c' is the default group for user '%1c'.\n
You cannot drop a user from a group\n
if the group is the user's default group.\n
Use ACCESSDB to change the user's default group and try again."
/*%
** Parameters:
**	%0c - specified group identifier
**	%1c - specified member identifier
**
** Explanation:
**	The user attempted to drop a member from a group with the "ALTER GROUP"
**	statement, but the group is the specified user's default group.
**
** System Status:
**	The statement is not aborted; subsequent members are processed.
**
** Recommendation:
**	Make sure the member was properly entered.  If so, use accessdb to
**	change the user's default group before dropping the user from the group.
*/
E_US186A_6250:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iiusergroup catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing the user group catalog.  It will appear if that catalog
**	has been corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs during session initialization, the session
**	is disconnected.  NOTE: Group identifiers are not verified for
**	system administrators.  If it occurs during "CREATE/ALTER/DROP"
**	processing, the statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US186B_6251:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iirole catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing the system catalog while looking up information on a role.
**	It will appear if that catalog has been corrupted, or if it doesn't
**	exist.
**
** System Status:
**	If this error occurs during session initialization, the session is
**	disconnected.  NOTE: Role identifiers are not verified for 
**	system administrators.  If it occurs during "CREATE/ALTER/DROP"
**	processing, the statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US186C_6252:SS5000H_UNAVAILABLE_RESOURCE
"Group authorization check failed.\n
Either the specified (or default) group does not exist,\n
or you are not a member of the specified (or default) group.\n
Contact your system manager for further assistance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued by SCF during session initialization
**	if the session's group identifier is not defined, or if
**	the user is not a member of the session's group.
**
**	NOTE: If no group was specified on the connect statement,
**	the default group (if any) is used.
**
** System Status:
**	For non-system administrators, the session is disconnected.
**	For system administrators, this message is written to the
**	error log only, and the session is allowed to continue with the
**	specified group identifier in effect.
**
** Recommendation:
**	Ensure the proper group identifier was specified.  Contact the 
**	system administrator, if no error in the identifier is found.
*/
E_US186D_6253:SS5000H_UNAVAILABLE_RESOURCE
"Role authorization check failed.\n
Either the specified role does not exist,\n
or the specified password is invalid.\n
Contact your system manager for further assistance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued by SCF during session initialization if the
**	session's role identifier is not defined, or if the specified password
**	is not valid.
**
**	NOTE: The user may not be aware that a role identifier was specified
**	on the connect statement by the invoked application.
**
** System Status:
**	For non-system administrators, the session is disconnected.
**	For system administrators, this message is written to the
**	error log only, and the session is allowed to continue with the
**	specified role identifier in effect.
**
** Recommendation:
**	Ensure the proper role identifier was specified.  Contact the 
**	system administrator, if no error in the identifier is found.
*/
E_US186E_6254:SS42503_INSUF_PRIV
"You are not authorized to issue\n
CREATE/ALTER/DROP GROUP statements.\n
Contact your system manager for further assistance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "CREATE GROUP", "ALTER GROUP", or
**	"DROP GROUP" statement, but the user was not a system
**	administrator, or did not have UPDATE SYSTEM CATALOG privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US186F_6255:SS42503_INSUF_PRIV
"You are not authorized to issue\n
CREATE/ALTER/DROP ROLE statements.\n
Contact your system manager for further assistance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "CREATE ROLE", "ALTER ROLE", or
**	"DROP ROLE" statement, but the user was not a user with SECURITY
**	privilege, or did not have UPDATE SYSTEM CATALOG privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US1870_6256:SS42503_INSUF_PRIV
"You cannot CREATE/ALTER/DROP GROUP\n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "CREATE GROUP", "ALTER GROUP",
**	or "DROP GROUP" statement, but the user was not in session
**	with the database database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Issue these commands only when connected to the database database.
*/
E_US1871_6257:SS42503_INSUF_PRIV
"You cannot CREATE/ALTER/DROP ROLE\n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "CREATE ROLE", "ALTER ROLE", or
**	"DROP ROLE" statement, but the user was not in session with the
**	database database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Issue these commands only when connected to the database database.
*/
E_US1872_6258:SS42502_DUPL_OBJECT
"Group '%0c' already exists.\n
You cannot create a group that already exists.\n
Ensure you entered the correct group identifier."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to create a group that already exists.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure the group name was properly entered.
*/
E_US1873_6259:SS42502_DUPL_OBJECT
"Role '%0c' already exists.\n
You cannot create a role that already exists.\n
Ensure you entered the correct role identifier."
/*%
** Parameters:
**	%0c - specified role identifier
**
** Explanation:
**	The user attempted to create a role that already exists.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure the role was properly entered.
*/
E_US1874_6260:SS5000H_UNAVAILABLE_RESOURCE
"Group '%0c' does not exist.\n
You cannot alter or drop a non-existing group.\n
Ensure you entered the correct group identifier."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to alter or drop a group identifier that does
**	not exist.
**
** System Status:
**	For "ALTER GROUP", the statement is aborted.  For "DROP GROUP", the
**	statement is not aborted; subsequent specified groups are processed.
**
** Recommendation:
**	Make sure the group identifier was properly entered.
*/
E_US1875_6261:SS5000H_UNAVAILABLE_RESOURCE
"Role '%0c' does not exist.\n
You cannot alter or drop a non-existing role.\n
Ensure you entered the correct role identifier."
/*%
** Parameters:
**	%0c - specified role identifier
**
** Explanation:
**	The user attempted to alter or drop a role identifier that does
**	not exist.
**
** System Status:
**	For "ALTER ROLE", the statement is aborted.  For "DROP ROLE", the
**	statement is not aborted; subsequent specified roles are processed.
**
** Recommendation:
**	Make sure the role was properly entered.
*/
E_US1876_6262:SS42502_DUPL_OBJECT
"Group '%0c' already contains user '%1c'.\n
You cannot add a member to a group\n
if the member already exists in the group.\n
Ensure you entered the correct group and user identifiers."
/*%
** Parameters:
**	%0c - specified group identifier
**	%1c - specified member identifier
**
** Explanation:
**	The user attempted to add a member to a group with "ALTER GROUP",
**	but the specified member already exists in the group.
**
** System Status:
**	The statement is not aborted; subsequent members are processed.
**
** Recommendation:
**	Make sure the member was properly specified.
*/
E_US1877_6263:SS5000H_UNAVAILABLE_RESOURCE
"Group '%0c' does not contain user '%1c'.\n
You cannot drop a member from a group\n
if the member does not exist in the group.\n
Ensure you entered the correct group and user identifiers."
/*%
** Parameters:
**	%0c - specified group identifier
**	%1c - specified member identifier
**
** Explanation:
**	The user attempted to drop a member from a group with "ALTER GROUP",
**	but the specified member does not exist in the group.
**
** System Status:
**	The statement is not aborted; subsequent members are processed.
**
** Recommendation:
**	Make sure the member was properly specified.
*/
E_US1878_6264:SS5000A_QUERY_ERROR
"Group '%0c' is not empty.\n
You cannot drop a group when members exist in the group.\n
Use 'ALTER GROUP %0c DROP ALL'\n
to drop all members of the group before you drop the group."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to drop a group identifier that still has members.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the group identifier was properly specified.  If so, drop all
**	members from the group before attempting to drop the group. This can be
**	done with the "ALTER GROUP ... DROP ALL" statement.
*/
E_US1879_6265:SS42502_DUPL_OBJECT
"Unable to create group '%0c'.\n
A user with this name already exists in this installation.\n
Group names must be unique within groups, roles, and users.\n
Please specify a different group name."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to create a group that has the same name as an
**	existing user.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the group identifier was properly specified.  If so, select
**	a different group identifier.
*/
E_US187A_6266:SS42502_DUPL_OBJECT
"Unable to create group '%0c'.\n
A role with this name already exists in this installation.\n
Group names must be unique within groups, roles, and users.\n
Please specify a different group name."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to create a group that has the same name as an
**	existing role.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the group identifier was properly specified.  If so, select
**	a different group identifier.
*/
E_US187B_6267:SS42502_DUPL_OBJECT
"Unable to create role '%0c'.\n
A user with this name already exists in this installation.\n
Role names must be unique within groups, roles, and users.\n
Please specify a different role name."
/*%
** Parameters:
**	%0c - specified role identifier
**
** Explanation:
**	The user attempted to create a role that has the same name as an
**	existing user.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the role identifier was properly specified.  If so, select
**	a different role identifier.
**
*/
E_US187C_6268:SS42502_DUPL_OBJECT
"Unable to create role '%0c'.\n
A group with this name already exists in this installation.\n
Role names must be unique within groups, roles, and users.\n
Please specify a different role name."
/*%
** Parameters:
**	%0c - specified role identifier
**
** Explanation:
**	The user attempted to create a role that has the same name as an
**	existing group.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the role identifier was properly specified.  If so, select
**	a different role identifier.
*/
E_US187D_6269:SS50000_MISC_ERRORS
"Unable to add '%1c' to group '%0c'.\n
You cannot add a member to a group\n
if the member does not exist as a user in the installation.\n
Ensure you entered the correct user identifier."
/*%
** Parameters:
**	%0c - specified group identifier
**	%1c - specified member identifier
**
** Explanation:
**	The user attempted to add a member to a group with "CREATE/ALTER GROUP",
**	but the specified member does not exist as a user in the system catalog.
**
** System Status:
**	The statement is not aborted; subsequent members are processed.
**
** Recommendation:
**	Make sure the member was properly specified.
*/
E_US187E_6270:SS42000_SYN_OR_ACCESS_ERR
"CREATE GROUP: '%0c' is either an invalid option or the syntax
of the WITH clause is invalid.
\nThe syntax of the WITH clause for this statement is:
\n      WITH USERS = (user {,user})"
/*%
** Parameters:
**	%0c - specified keyword
**
** Explanation:
**	The user entered an invalid "WITH" clause on the "CREATE GROUP"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the command, using the correct syntax.
*/
E_US187F_6271:SS42000_SYN_OR_ACCESS_ERR
"ALTER GROUP: '%0c' is either an invalid option or the syntax
of the ADD/DROP clause is invalid.
\nSpecify one of:
\n      ADD  USERS (user {,user})
\n      DROP USERS (user {,user})
\n      DROP ALL"
/*%
** Parameters:
**	%0c - specified keyword
**
** Explanation:
**	The user entered an invalid "ADD" or "DROP" clause on the "ALTER GROUP"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the command, using the correct syntax.
*/
E_US1880_6272:SS42000_SYN_OR_ACCESS_ERR
"CREATE ROLE: '%0c' is either an invalid option or the syntax
of the WITH clause is invalid.
\nSpecify one of:
\n      WITH PASSWORD = 'password'
\n      WITH NOPASSWORD"
/*%
** Parameters:
**	%0c - specified keyword
**
** Explanation:
**	The user entered an invalid "WITH" clause on the "CREATE ROLE"
**	statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the command, using the correct syntax.
*/
E_US1881_6273:SS42000_SYN_OR_ACCESS_ERR
"ALTER ROLE: '%0c' is either an invalid option or the syntax
of the WITH clause is invalid.
\nSpecify one of:
\n      WITH PASSWORD = 'password'
\n      WITH NOPASSWORD"
/*%
** Parameters:
**	%0c - specified keyword
**
** Explanation:
**	The user entered an invalid "WITH" clause on the "ALTER ROLE" statement.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the command, using the correct syntax.
*/
E_US1882_6274:SS50008_UNSUPPORTED_STMT
"The CREATE GROUP statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "CREATE GROUP" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under a local DBMS.
*/
E_US1883_6275:SS50008_UNSUPPORTED_STMT
"The ALTER GROUP statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "ALTER GROUP" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US1884_6276:SS50008_UNSUPPORTED_STMT
"The DROP GROUP statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "DROP GROUP" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US1885_6277:SS50008_UNSUPPORTED_STMT
"The CREATE ROLE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "CREATE ROLE" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US1886_6278:SS50008_UNSUPPORTED_STMT
"The ALTER ROLE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "ALTER ROLE" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US1887_6279:SS50008_UNSUPPORTED_STMT
"The DROP ROLE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "DROP ROLE" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US1888_6280:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iidbpriv catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing the system catalog.  It will appear if that catalog has
**	been corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs during session initialization, the session is
**	disconnected; superusers are allowed to continue. If it occurs while
**	processing "GRANT/REVOKE" statements for database privileges, the
**	statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US1889_6281:SS42503_INSUF_PRIV
"You are not authorized to GRANT or REVOKE privileges\n
on database '%0c'.\n
Contact your system manager for further assistance."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	The user attempted to issue the "GRANT/REVOKE" statement for database
**	privileges, but the user was not a superuser, did not have UPDATE
**	SYSTEM CATALOG privileges, or was not the DBA for the specified
**	database.
**
** System Status:
**	The statement continues with other databases specified, if any.
**
** Recommendation:
**	Be sure the database name was properly specified.  Contact the 
**	system administrator, if no error in the name is found.
*/
E_US188A_6282:SS5000R_RUN_TIME_LOGICAL_ERROR
"You cannot GRANT or REVOKE database or installation privileges\n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement
**	for database or installation privileges, but the user was not
**	in session with the database database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Issue these commands only when connected to the database database.
*/
E_US188B_6283:SS5000A_QUERY_ERROR
"You cannot GRANT or REVOKE database or installation privileges\n
on non-database, non-installation objects.\n
Ensure the privileges that you are granting or revoking\n
are consistent with the objects that you specify."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement
**	for database or installation privileges, but the specified objects
**	are not databases.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Check to ensure "ON DATABASE" or "ON CURRENT INSTALLATION" is specified
**	when granting database or installation privileges.
*/
E_US188C_6284:SS5000A_QUERY_ERROR
"You cannot GRANT or REVOKE non-database or non-installation privileges\n
on database or installation objects.\n
Ensure the privileges that you are granting or revoking\n
are consistent with the objects that you specify."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement
**	for non-database or non-installation privileges, but specified
**	objects that are databases.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure "ON DATABASE" or "ON CURRENT INSTALLATION" is not specified
**	when granting non-database privileges.
*/
E_US188D_6285:SS42000_SYN_OR_ACCESS_ERR
"You must specify a non-negative integral value for '%0c'\n
when granting this database or installation privilege.\n
Please reenter this statement with a non-negative integral value specified."
/*%
** Parameters:
**	%0c - privilege name
**
** Explanation:
**	The user attempted to "GRANT" a database or installation privilege
**	that requires a value, but no value was specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with a value specified
**	for the database privilege.
*/
E_US188E_6286:SS42000_SYN_OR_ACCESS_ERR
"You must not specify a value for '%0c'\n
when revoking this privilege.\n
Please reenter this statement without a value specified."
/*%
** Parameters:
**	%0c - privilege name
**
** Explanation:
**	The user attempted to "REVOKE" a database or installation privilege,
**	but a value was specified for that privilege; this is not consistent
**	with the "REVOKE" statement syntax.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command without a value specified
**	for the database or installation privilege.
*/
E_US188F_6287:SS42000_SYN_OR_ACCESS_ERR
"You must never specify a value for '%0c'\n
when granting or revoking this privilege.\n
Please reenter this statement without a value specified."
/*%
** Parameters:
**	%0c - privilege name
**
** Explanation:
**	The user attempted to "GRANT" or "REVOKE" a privilege that does not
**	require a value, but a value was specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command without a value specified
**	for the database or installation privilege.
*/
E_US1890_6288:SS42000_SYN_OR_ACCESS_ERR
"Unrecognized privilege specified: '%0c'\n
Please check the spelling of this privilege and reenter the command."
/*%
** Parameters:
**	%0c - specified privilege
**
** Explanation:
**	The user attempted to "GRANT" or "REVOKE" an unknown privilege.
**	This error is also issued for "CREATE/ALTER USER".
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the correct privilege.
*/
E_US1891_6289:SS5000A_QUERY_ERROR
"Conflicting values specified for '%0c'\n
Please reenter the command with one value specified for this privilege."
/*%
** Parameters:
**	%0c - privilege name
**
** Explanation:
**	The user attempted to "GRANT" a database or installation privilege
**	that requires a value, but the privilege was specified with
**	multiple values.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the privilege specified once.
*/
E_US1892_6290:SS50008_UNSUPPORTED_STMT
"The REVOKE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user entered the "REVOKE" statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US1893_6291:SS5000H_UNAVAILABLE_RESOURCE
"Database '%0c' does not exist.\n
You cannot GRANT or REVOKE privileges on a database that doesn't exist.\n
Be sure you specified the correct database name."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	The user specified a non-existing database on the "GRANT/REVOKE"
**	statement.  NOTE: Superusers and users with UPDATE SYSTEM CATALOG
**	privileges are allowed to revoke privileges on non-existing databases.
**
** System Status:
**	The statement continues with other databases specified, if any.
**
** Recommendation:
**	Be sure the database name was properly specified. Contact the 
**	system administrator, if no error was found.
*/
E_US1894_6292:SS5000A_QUERY_ERROR
"REVOKE error:\n
One or more of the specified privileges were never granted\n
on database '%0c' to '%1c'.\n
You cannot revoke privileges that were never granted.\n
Be sure you specified the correct privileges."
/*%
** Parameters:
**	%0c - database name
**	%1c - grantee name
**
** Explanation:
**	The user attempted to "REVOKE" privileges that were never granted.
**
** System Status:
**	The statement continues, but the noted database/grantee is ignored.
**	All other database/grantees are processed.
**
** Recommendation:
**	Be sure the privilege and grantee were properly specified.
*/
E_US1895_6293:SS42505_OBJ_NOT_FOUND
"User '%1c' does not exist.\n
You cannot GRANT database or installation privileges to a user\n
that doesn't exist.\n
Be sure you specified the correct grantee type and name."
/*%
** Parameters:
**	%0c - database name
**	%1c - grantee name
**
** Explanation:
**	The user attempted to "GRANT" database or installation privileges to
**	a user that does not exist.
**
** System Status:
**	The statement continues, but the noted database/grantee is ignored.
**	All other database/grantees are processed.
**
** Recommendation:
**	Be sure the grantee was properly specified.
*/
E_US1896_6294:SS42505_OBJ_NOT_FOUND
"Group '%1c' does not exist.\n
You cannot GRANT database or installation privileges to a group\n
that doesn't exist.\n
Be sure you specified the correct grantee type and name."
/*%
** Parameters:
**	%0c - database name
**	%1c - grantee name
**
** Explanation:
**	The user attempted to "GRANT" database or installation privileges to
**	a group that does not exist.
**
** System Status:
**	The statement continues, but the noted database/grantee is ignored.
**	All other database/grantees are processed.
**
** Recommendation:
**	Be sure the grantee was properly specified.
*/
E_US1897_6295:SS42505_OBJ_NOT_FOUND
"Role '%1c' does not exist.\n
You cannot GRANT database or installation privileges to a role\n
that doesn't exist.\n
Be sure you specified the correct grantee type and name."
/*%
** Parameters:
**	%0c - database name
**	%1c - grantee name
**
** Explanation:
**	The user attempted to "GRANT" database or installation privileges to
**	a role that does not exist.
**
** System Status:
**	The statement continues, but the noted database/grantee is ignored.
**	All other database/grantees are processed.
**
** Recommendation:
**	Be sure the grantee was properly specified.
*/
E_US1898_6296:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iidatabase catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF has trouble opening, reading, or closing
**	the catalog.  It will appear if that catalog has been
**	corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs while processing "GRANT/REVOKE" statements for
**	database privileges, the statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US1899_6297:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iiuser catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF has trouble opening, reading, or closing
**	the catalog.  It will appear if that catalog has been corrupted,
**	or if it doesn't exist.
**
** System Status:
**	If this error occurs while processing "GRANT/REVOKE" statements for
**	database privileges, the statement is aborted.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US189A_6298:SS01000_WARNING
"All database and installation privileges have been removed for '%0c'\n
Database and installation privileges granted to a user, group, or role \n
are removed when that user, group, or role is dropped."
/*%
** Parameters:
**	%0c - specified user, group, or role identifier
**
** Explanation:
**	When dropping a user, group, or role, all database and installation
**	privileges granted to that grantee are implicitly revoked.  This is
**	an informational message only.
**
** System Status:
**	The statement continues.
**
** Recommendation:
**	None.
*/
E_US189B_6299:SS5000A_QUERY_ERROR
"REVOKE error:\n
One or more of the specified privileges were never granted\n
on the installation to '%1c'.\n
You cannot revoke privileges that were never granted.\n
Be sure you specified the correct privileges."
/*%
** Parameters:
**	%1c - grantee name
**
** Explanation:
**	The user attempted to "REVOKE" privileges that were never granted on
**	the installation.
**
** System Status:
**	The statement continues, but the noted grantee is ignored. All other
**	grantees are processed.
**
** Recommendation:
**	Be sure the privilege and grantee were properly specified.
*/
E_US189C_6300_RULE_EXISTS:SS42502_DUPL_OBJECT
"CREATE RULE: Rule '%0c' already exists."
/*%
** Parameters:
**	%0c - rule name
**
** Explanation:
**	The named rule already exists for the current user.  Duplicate rule
**	names, even if applied to different tables, are not allowed.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Choose a unique name for the new rule or delete the existing named
**	rule, then create the new one.
*/
E_US189D_6301_RULE_ABSENT:SS42505_OBJ_NOT_FOUND
"DROP RULE: Rule '%0c' does not exist."
/*%
** Parameters:
**	%0c - rule name
**
** Explanation:
**	The named rule does not exist for the current user.  Rules owned
**	by other users cannot be dropped.
**
** System Status:
**	The program continues, but the current "DROP" statement is aborted.
**
** Recommendation:
**	If the rule name was improperly entered, retype the rule name.
*/
E_US189E_6302_RULE_QUAL:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Invalid predicate in WHERE clause."
/*%
** Parameters:
**	None.
**
** Explanation:
**	"WHERE" clauses associated with the "CREATE RULE" statement may not
**	reference sub-SELECT queries or set functions (such as SUM or COUNT).
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Restructure your "WHERE" clause (or remove it) so that it is accepted
**	as a rule qualification.  You may also move your "WHERE" clause into
**	a procedure being fired by the rule and use it as part of a qualifying
**	"SELECT" statement to filter out the row.
*/
E_US189F_6303_RULE_COL:SS5000A_QUERY_ERROR
"CREATE RULE: Unqualified reference to column '%0c' - use OLD or NEW."
/*%
** Parameters:
**	%0c - column name
**
** Explanation:
**	Since OLD and NEW (and the table name) are implicit correlation
**	names for column values before and after updating a row in the
**	referenced table, you must qualify the column.  Unqualified columns
**	may yield ambiguous results.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Qualify all column names referenced in the rule.
*/
E_US18A0_6304_RULE_STAR:SS50008_UNSUPPORTED_STMT
"The CREATE and DROP RULE statements are not allowed in distributed INGRES."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Star does not yet support rule creation.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Remove the statement or send it through a "DIRECT CONNECT" session.
*/
E_US18A1_6305_RULE_II:SS42507_RESERVED_IDENTIFIER
"CREATE RULE: Invalid rule name '%0c' starts with reserved prefix '%1c'."
/*%
** Parameters:
**	%0c - rule name
**	%1c - system prefix
**
** Explanation:
**	All objects, including rule names, that begin with the system prefix
**	are reserved.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Specify a name that doesn't begin with the reserved prefix.
*/
E_US18A2_6306_RULE_COLTAB:SS42501_COL_NOT_FOUND
"CREATE RULE: Column '%0c' is not in table '%1c'."
/*%
** Parameters:
**	%0c - column name
**	%1c - table name
**
** Explanation:
**	The column specified in the rule was not found in the table for
**	which the rule is being created.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Retype the column name (or table name).
*/
E_US18A3_6307_RULE_STMT:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Duplicate statement types listed in AFTER clause."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "AFTER" clause of a "CREATE RULE" statement may not specify a
**	statement type twice.  This could lead to ambiguous rule application,
**	specifically for the UPDATE operation.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Specify each statement type at most once.
*/
E_US18A4_6308_RULE_VIEW:SS5000A_QUERY_ERROR
"CREATE RULE: Rules cannot be applied to a view '%0c'."
/*%
** Parameters:
**	%0c - view name
**
** Explanation:
**	Rules may only be applied to base tables.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Apply the rule to the base table for the view.
*/
E_US18A5_6309_RULE_OWN:SS42503_INSUF_PRIV
"CREATE RULE: You must be the owner of '%0c' in order to apply rules."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	You can only apply a rule to a table if you own the table.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Apply the rule on to tables you own.
*/
E_US18A6_6310_RULE_OLDNEW:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Invalid word '%0c' in REFERENCING clause - use OLD or NEW."
/*%
** Parameters:
**	%0c - error token
**
** Explanation:
**	The syntax of a rule "REFERENCING" clause is:
**
**	   REFERENCING ref_clause [ref_clause]
**
**	where ref_clause is:
**
**	   OLD | NEW  AS correlation_name
**
**	This clause is available in order to apply rules to tables actually
**	called "NEW" or "OLD" with the intent of using the old and/or new
**	values of a column.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Use "OLD" and/or "NEW".
*/
E_US18A7_6311_RULE_WHEN:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: A rule can only be fired BEFORE or AFTER an SQL statement.
Check spelling and position of the words BEFORE or AFTER."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Action time for defined rule is incorrect (must be BEFORE or AFTER).
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Check the spelling of action time and correct the statement.
*/
E_US18A8_6312_RULE_DRPTAB:SS5000J_INCONSISTENT_DBMS_CAT
"DROP RULE: Rule '%0c' dropped but table referenced by rule no longer exists."
/*%
** Parameters:
**	%0c - rule name
**
** Explanation:
**	The specified rule was removed from the system catalogs, but the table
**	referenced by the rule (in the original "CREATE RULE" statement) no
**	longer exists.  This could occur due to direct removal of the table
**	from the system catalogs without using the "DROP TABLE" statement or
**	issuing the "DROP TABLE" statement through an older version of the
**	software.
**
** System Status:
**	The rule is dropped, and the program continues.
**
** Recommendation:
**	Verify that the rule no longer exists, and that the table referenced by
**	the rule did not exist.  If the table still exists under the original
**	owner, contact the system administrator.
*/
E_US18A9_6313_RULE_CAT:SS42503_INSUF_PRIV
"CREATE RULE: You do not have permission to apply a rule to table '%0c'."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The specified table is an protected table and rules cannot
**	be applied to it without explicit UPDATE CATALOG permission.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Only the DBA or system administrator may apply rules to
**	database tables.
*/
E_US18AA_6314_SET_NORULES:SS42503_INSUF_PRIV
"SET [NO]RULES:  You do not have permission to turn rules on or off.
Only the DBA may issue this statement or equivalent SET TRACE statement."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "SET [NO]RULES" statement may only be issued by the DBA.  This
**	statement is provided for rule development purposes and DBA tools.
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Consult your DBA for permission to turn rules on or off.
**	Alternatives:
**	  1. "DROP" rules and recreate them later.
**	  2. Insert a check into your procedures (invoked by a rule) to
**	     "SELECT" some global debug parameter from a global table.  If this
**	     is set to say, "rules_off", then immediately return from the
**	     procedure.
*/
E_US18AB_6315_NOT_DROPPABLE:SS42000_SYN_OR_ACCESS_ERR
"%0c: Cannot drop '%1c' because it is a system-generated object."
/*%
** Parameters:
**	%0c - statement name
**	%1c - object name
**
** Explanation:
**	Certain new features are implemented using internal rules,
**	procedures, and/or indexes.  These "system-generated objects"
**	cannot be dropped by the user; otherwise, the tables on which they
**	are defined could become inconsistent.  This includes SQL92 Constraints
**	(internal rules and procedures for CHECK and REFERENTIAL constraints,
**	internal indexes for UNIQUE constraints) and Views WITH CHECK OPTION
**	(internal rule and internal procedure).
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Don't attempt to drop this object. If this object really must be
**	dropped, drop the constraint or view for which it was created.  This
**	object will then be dropped automatically.
*/
E_US18AC_6316_SEQ_INT:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER SEQUENCE: An integer value must be specified for the '%0c' option."
/*%
** Parameters:
**	%0c - sequence definition option keyword
**
** Explanation:
**	The [re]start, increment, maxvalue, minvalue and cache options of sequence
**	definitions MUST specify integer values.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER" statement is aborted.
**
** Recommendation:
**	Correct the indicated syntax.
*/
E_US18AD_6317_SEQ_INCONS:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER SEQUENCE: The '%0c' options are inconsistent and cannot be used together."
/*%
** Parameters:
**	%0c - invalid option pair
**
** Explanation:
**	A mutually exclusive pair of sequence options was specified (e.g. CYCLE and 
**	NOCYCLE). Only one of the indicated options may be used.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER" statement is aborted.
**
** Recommendation:
**	Correct the indicated syntax.
*/
E_US18AE_6318_SEQ_DTYPE:SS42000_SYN_OR_ACCESS_ERR
"CREATE SEQUENCE: Invalid data type for sequence definition."
/*%
** Parameters:
**	none
**
** Explanation:
**	Sequence data types must be exact numeric with scale 0. That is, they must
**	be either integer, or decimal with scale 0.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Correct the indicated data type.
*/
E_US18AF_6319_BAD_NEXTVAL:SS42000_SYN_OR_ACCESS_ERR
"Next value/current value sequence operators used in incorrect context."
/*%
** Parameters:
**	none
**
** Explanation:
**	The sequence operators cannot be used in where clause, union queries, 
**	subselects, aggregate operations and several other contexts. Consult the
**	documentation to verify the correct use of "next/current value".
**
** System Status:
**	The program continues, but the current statement is aborted.
**
** Recommendation:
**	Correct the indicated syntax.
*/
E_US18B0_6320_SEQ_BADVAL:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER SEQUENCE: The magnitude of the '%0c' option \n
is too large for the declared sequence type."
/*%
** Parameters:
**	%0c - invalid sequence option
**
** Explanation:
**	A value was specified for a sequence option that was too large for the 
**	declared type. For example, a MAXVALUE in excess of 2**31-1 was specified
**	for an INTEGER sequence. Or a value was specified for a decimnal sequence
**	that contained more digits than the declared decimal precision.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER" statement is aborted.
**
** Recommendation:
**	Correct the indicated option or change the sequence data type.
*/
E_US18B1_6321:SS42503_INSUF_PRIV
"You are not allowed to create/alter sequences in this database.\n
Contact your Database Administrator for further information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the CREATE/ALTER SEQUENCE command, but the
**	session does not have CREATE_SEQUENCE privileges on the active
**	database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Contact the database administrator to get CREATE_SEQUENCE privileges.
**	NOTE: This privilege may be granted to one of several authorization
**	identifiers that apply to a session, including the ROLE, GROUP, and
**	USER identifiers, as well as PUBLIC.
*/
E_US18B2_6322_BADINCR:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER SEQUENCE: The specified value of the INCREMENT option\n
must be non-zero."
/*%
** Parameters:
**	none
**
** Explanation:
**	0 was specified as the increment value for a sequence. Sequence
**	increments may be negative or positive, but cannot be 0.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER" statement is aborted.
**
** Recommendation:
**	Correct the INCREMENT option value.
*/
E_US18B3_6323_BADTYPE:SS42000_SYN_OR_ACCESS_ERR
"CREATE SEQUENCE: '%0c' is invalid type for sequence definition."
/*%
** Parameters:
**	%0c - type name
**
** Explanation:
**	The specified type is invalid for sequence definitions. Sequences
**	must be of type "integer" or "decimal(n)" (where 1 <= n <= 31).
**
** System Status:
**	The program continues, but the current "CREATE SEQUENCE" statement is
**	aborted.
**
** Recommendation:
**	Correct the data type of the defined sequence.
*/
E_US18B4_6324_NOSTMTBEFORE:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: FOR EACH STATEMENT is not supported for BEFORE rules."
/*%
** Parameters:
**
** Explanation:
**	The database procedure identified for execution by a BEFORE rule
**	contains at least one data change statement (INSERT, UPDATE, DELETE). 
**	This is not permitted.
**
** System Status:
**	The program continues, but the current "CREATE RULE" statement is
**	aborted.
**
** Recommendation:
**	Procedures whose execution is triggered by BEFORE rules may not 
**	contain any data change statements. Modify the procedure accordingly
**	and rerun the rule definition.
*/
E_US18B5_6325_NODCBEFORE:SS42000_SYN_OR_ACCESS_ERR
"CREATE RULE: Data change statements are not permitted in procedures\n
triggered by BEFORE rules."
E_US18B6_6326_USER_EXISTS:SS42502_DUPL_OBJECT
"CREATE USER: The user '%0c' already exists."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	The specified user already exists; duplicate users are not allowed.
**
** System Status:
**	The program continues, but the current "CREATE USER" statement is
**	aborted.
**
** Recommendation:
**	A user can only be created once; verify that the user name was entered
**	correctly.
*/
E_US18B7_6327_USER_ABSENT:SS42505_OBJ_NOT_FOUND
"The user '%0c' was not found."
/*
** Parameters:
**	%0c - user name
**
** Explanation:
**	The administrator attempted to alter/drop an user that did not exist,
**	or to grant/revoke a role access to a user that did not exist.
**
** System Status:
**	The program continues, but the current "ALTER/DROP USER"  or
**	GRANT/REVOKE on role statement is aborted.
**
** Recommendation:
**	Verify that the user name was entered correctly; only existing users
**	may be dropped or altered.
*/
E_US18B8_6328_LOC_EXISTS:SS42502_DUPL_OBJECT
"CREATE LOCATION: The location '%0c' already exists."
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	The name specified in the "CREATE LOCATION" statement is the same
**	as a location that already exists.
**
** System Status:
**	The program continues, but the current "CREATE LOCATION" statement
**	is aborted.
**
** Recommendation:
**	A location can only be created once; verify that the location name
**	was entered correctly.
*/
E_US18B9_6329_LOC_ABSENT:SS42505_OBJ_NOT_FOUND
"ALTER/DROP LOCATION: The location '%0c' not found."
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	The location specified in the "ALTER/DROP LOCATION" statement does
**	not exist.
**
** System Status:
**	The program continues, but the current "ALTER/DROP LOCATION" statement
**	is aborted.
**
** Recommendation:
**	Verify that the location name was entered correctly; only existing
**	locations may be dropped or altered.
*/
E_US18BA_6330_DB_UNKNOWN:SS42505_OBJ_NOT_FOUND
"DBACCESS: The database '%0c' not found."
/*%
** Parameters:
**	%0c - database name
**
** Explanation:
**	The specified table is a dbaccess table.
**
** System Status:
**	The program continues, but the current "DBACCESS" statement is aborted.
**
** Recommendation:
**	Only a user with SECURITY privilege can grant access to a database.
*/
E_US18BB_6331_USR_UNKNOWN:SS42505_OBJ_NOT_FOUND
"DBACCESS: The user '%0c' not found."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	The user specified in the "DBACCESS" statement does not exist.
**
** System Status:
**	The program continues, but the current "DBACCESS" statement is aborted.
**
** Recommendation:
**	Verify that the user name was entered correctly, then reissue the
**	statement.
*/
E_US18BC_6332_DBACC_EXISTS:SS42502_DUPL_OBJECT
"DBACCESS: Database access already granted to user '%0c'."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	The system administrator attempted to grant database access to a
**	user who already has access.
**
** System Status:
**	The program continues, but the current "DBACCESS" statement is aborted.
**
** Recommendation:
**	Verify that the user name was entered correctly; database access to
**	a user can only be granted once.
*/
E_US18BD_6333_DBACC_ABSENT:SS42505_OBJ_NOT_FOUND
"DBACCESS: User'%0c' does not have access to database."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	The system administrator attempted to revoke database access to a
**	user who does not have access.
**
** System Status:
**	The program continues, but the current "DBACCESS" statement is aborted.
**
** Recommendation:
**	Verify that the user name was entered correctly; database access can
**	only be revoked for users who already have access.
*/
E_US18BE_6334:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iiuser catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing the catalog.  It will appear if that catalog has
**	been corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs during session initialization, the session is
**	disconnected; superusers are allowed to continue.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US18BF_6335:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iilocation catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing this catalog.  It will appear if that catalog
**	has been corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs during session initialization, the session is
**	disconnected; superusers are allowed to continue.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US18C0_6336:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iidbaccess catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing this catalog.  It will appear if that catalog
**	has been corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs during session initialization, the session is
**	disconnected; superusers are allowed to continue.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US18C1_6337:SS5000J_INCONSISTENT_DBMS_CAT
"An error occurred while processing the iisecuritystate catalog.\n
Please contact your system manager."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening, reading,
**	or writing the security state catalog.  It will appear if that catalog
**	has been corrupted, or if it doesn't exist.
**
** System Status:
**	If this error occurs during session initialization, the session is
**	disconnected; superusers are allowed to continue.
**
** Recommendation:
**	Contact the system administrator.
*/
E_US18C2_6338:SS50000_MISC_ERRORS
"ORDER BY column list is either illegal or contradictory."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message is issued if QEF cannot sort resulting tuples by the
**	specified "order by" list, because the column is either specified
**	more than once, or has both descending and ascending attributes
**	associated in the ordering.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Please check that the "order by" list specifies each column only once.
*/
E_US18C3_6339:SS5000A_QUERY_ERROR
"DROP USER: User '%0c' is a Database Administrator.\n
You cannot remove a user from an installation if that\n
user owns databases in the installation.\n
Run DESTROYDB to remove all databases owned by '%0c'\n
before attempting to drop that user."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	This message is issued by QEF if "DROP USER" is specified for
**	a user that is the DBA for any database in the installation.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Use destroydb to destroy any databases owned by the user
**	before attempting to drop that user.
*/
E_US18C4_6340:SS42000_SYN_OR_ACCESS_ERR
"GRANT/REVOKE:  You must not specify a list of objects\n
when granting or revoking privileges ON CURRENT INSTALLATION.\n
Reenter this statement without objects specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement
**	"ON CURRENT INSTALLATION", but a list of objects was specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Check to ensure no objects are specified when granting or revoking
**	privileges on installations.
*/
E_US18C5_6341:SS42000_SYN_OR_ACCESS_ERR
"GRANT/REVOKE:  You must specify a list of objects\n
when granting or revoking privileges on specific\n
tables, procedures, databases or dbevents.\n
Reenter this statement with objects specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement, but
**	no object list was specified, and "ON CURRENT INSTALLATION" was not
**	specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Check to ensure objects are specified when granting or revoking
**	privileges, except when "ON CURRENT INSTALLATION" is specified.
*/
E_US18C6_6342:SS42503_INSUF_PRIV
"You are not authorized to GRANT or REVOKE privileges ON CURRENT INSTALLATION.\n
Please contact your System Administrator for further assistance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The non-privileged user attempted to grant privileges "ON CURRENT
**	INSTALLATION"; this requires security privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should contact the system administrator to determine
**	who should handle this request.
*/
E_US18C7_6343:SS42000_SYN_OR_ACCESS_ERR
"GRANT/REVOKE: The `TO <grantee>' clause must be %0c\n
when granting or revoking one or more of the specified privileges.\n
Please ensure that the specified privileges require inclusion or\n
omission of the target grantee clause."
/*%
** Parameters:
**	%0c - either "omitted" or "included"
**
** Explanation:
**	Either the user specified a privilege that requires omission of the
**	target grantee clause (such as GLOBAL_USAGE), but the target grantee
**	clause was included; or, the user specified a privilege that requires
**	inclusion of the target grantee clause (such as QUERY_ROW_LIMIT), but
**	the target grantee clause was omitted.
**
**	The context is determined by the message parameter:
**	        "omitted"  implies that the TO clause should be omitted
**	        "included" implies that the TO clause should be included
**	Note that the privilege list may contain privileges that are
**	inconsistent -- some privileges may require inclusion of the
**	TO clause, while some may require omission of the TO clause.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should examine the specified privileges to ensure
**	they are consistent with the statement syntax.
*/
E_US18CE_6350:SS50008_UNSUPPORTED_STMT
"The %0c statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	%0c - statement name (e.g., "CREATE USER")
**
** Explanation:
**	The user entered the specified statement under Star.
**	This is not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reissue the command under local DBMS.
*/
E_US18CF_6351:SS5000A_QUERY_ERROR
"%0c: '%1c' specified multiple times.\n
Please reenter the statement with one instance of this parameter."
/*%
** Parameters:
**	%0c - statement name (e.g., "CREATE USER")
**	%1c - conflicting parameter (e.g., "GROUP")
**
** Explanation:
**	The user attempted to issue a statement, but a parameter was specified
**	multiple times.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the parameter specified once.
*/
E_US18D1_6353:SS42000_SYN_OR_ACCESS_ERR
"%0c: Syntax error in the 'WITH' clause.\n
Last token read was '%1c'.\n
Please check the syntax of the 'WITH' clause and reenter the statement."
/*%
** Parameters:
**	%0c - statement name (e.g., "ALTER LOCATION")
**	%1c - token name (e.g., "AREA")
**
** Explanation:
**	The user attempted to issue a statement, but the "WITH" clause was
**	not well formed.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should correct the "WITH" clause and reenter the statement.
*/
E_US18D2_6354:SS42503_INSUF_PRIV
"%0c: only the DBA can use the '%0c' statement."
/*%
** Parameters:
**	%0c - statement name (e.g., "CREATE SECURITY_ALARM")
**
** Explanation:
**	A user who is not the DBA attempted to issue a statement that can only
**	be issued by the DBA.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should contact the DBA to perform this operation.
*/
E_US18D3_6355_NOT_AUTH:SS42503_INSUF_PRIV
"You do not have privileges to issue '%0c' statements.\n
Please contact your System Administrator for further assistance."
/*%
** Parameters:
**	%0c - statement name (e.g., "CREATE USER")
**
** Explanation:
**	The user attempted to issue a statement that requires special
**	privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should contact the system administrator to determine
**	who should handle this request.
*/
E_US18D4_6356_NOT_DBDB:SS42000_SYN_OR_ACCESS_ERR
"You must be connected to the 'iidbdb' database\n
to issue '%0c' statements.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters:
**	%0c - statement name (e.g., "CREATE USER")
**
** Explanation:
**	The user attempted to issue a statement requiring connection to the
**	database database while connected a different database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reissue the statement only while connected to the
**	database database.
*/
E_US18D5_6357_FORM_NOT_AUTH:SS42503_INSUF_PRIV
"You do not have privileges to issue this form of '%0c' statements.\n
Please contact your System Administrator for further assistance."
/*%
** Parameters:
**	%0c - statement name (e.g., "ALTER USER")
**
** Explanation:
**	The user attempted to issue a special form of a statement that
**	requires special privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should contact the system administrator to determine
**	who should handle this request.
*/
E_US18D6_6358_ALTER_BAD_PASS:SS50000_MISC_ERRORS
"ALTER USER: Either you specified a bad value for OLDPASSWORD,\n
or the OLDPASSWORD clause was omitted altogether.\n
Please respecify the statement with the correct value for OLDPASSWORD."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to change passwords with the "ALTER USER" statement,
**	but the value specified for OLDPASSWORD did not match the user's old
**	password, or the OLDPASSWORD clause was omitted altogether.
**
**	NOTE: Currently, user passwords are not supported.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the statement with the correct value
**	for OLDPASSWORD specified on the "ALTER USER" statement.
*/
E_US18D7_6359_NO_RAW_SPACE:SS50003_EXHAUSTED_RESOURCE
"All space in this Raw Area has been allocated to other Location(s).\n
    Check error log (errlog.log) for more information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to extend the database to a Raw area
**	but other Locations already extended to the area consume
**	all available space.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ignore the extension, or destroy the database and reconfigure
**	the RAWPCT values of all Locations assigned to the Raw area.
*/
E_US18D8_6360:SS5000A_QUERY_ERROR
"'AREA' is a required parameter on the CREATE LOCATION statement.\n
Please reenter this statement with the 'AREA' parameter specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user omitted the AREA parameter on the "CREATE LOCATION"
**	statement; this is a required parameter.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the statement with the AREA parameter
**	specified.
*/
E_US18D9_6361:SS5000A_QUERY_ERROR
"'AREA' cannot be specified on the ALTER LOCATION statement.\n
Please reenter this statement with the 'AREA' parameter omitted."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified the AREA parameter on the "ALTER LOCATION"
**	statement; this parameter cannot be specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the statement with the AREA parameter omitted.
*/
E_US18DA_6362:SS42000_SYN_OR_ACCESS_ERR
"CREATE/ALTER LOCATION: Unrecognized usage specified: '%0c'\n
Please check the spelling of this usage and reenter the command."
/*%
** Parameters:
**	%0c - specified usage
**
** Explanation:
**	The user attempted to "CREATE" or "ALTER" a location, but one of the
**	specified usages was invalid.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the correct usage specified.
*/
E_US18DB_6363:SS5000A_QUERY_ERROR
"CREATE/ALTER LOCATION: Conflicting usages specified.\n
Usages 'ALL' and 'NONE' cannot be specified with any other usage.\n
Please eliminate conflicting usages and reenter the statement."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to "CREATE" or "ALTER" a location, and specified
**	"ALL" or "NONE" in the usage clause with other usages.  When "ALL" or
**	"NONE" is specified, no other usages may be specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command without conflicting usages
**	specified.
*/
E_US18DC_LOC_IN_USE:SS50000_MISC_ERRORS
"Unable to drop location `%0c'.\n
This location is currently used by one or more databases,\n
as reflected by the iidatabase and/or iiextend catalogs.\n
You must destroy all databases that occupy a location\n
before that location can be dropped."
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	The user attempted to drop a location, but the location is referenced
**	by one or more iidatabase and/or iiextend tuples.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user must destroy all databases that reference the location before
**	it can be dropped.
*/
E_US18DD_6365:SS5000A_QUERY_ERROR
"RAWPCT must be an integer between 0 and 100."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The user attempted to CREATE a location but specified
**	a value for RAWPCT that is not an integer between zero
**	and one hundred. Zero, the default, prevents this location 
**	from being extended to a raw area.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with a correct value for RAWPCT.
*/
E_US18DE_6366_INVALID_RAW_USAGE:SS50000_MISC_ERRORS
"DATABASE is the only valid usage for raw locations."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The user attempted to CREATE or ALTER a raw location but specified
**	a usage other than DATABASE.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the correct usage.
*/
E_US18DF_6367_RUN_MKRAWAREA:SS01000_WARNING
"Warning: You must run MKRAWAREA before this location's area can be used."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The user attempted to CREATE or ALTER a raw location whose Area
**	does not exist and cannot be created by the Server.
**	Raw areas can only be created using MKRAWAREA.
**
** System Status:
**	The location is created, the area is not.
**
** Recommendation:
**	The user must run MKRAWAREA before extending a database
**	to this location.
*/
E_US18E0_6368_AREA_IS_RAW:SS50000_MISC_ERRORS
"RAWPCT must be non-zero when assigning a location to a raw area."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The user attempted to CREATE or ALTER a location whose Area
**	is raw and failed to specify a non-zero value for RAWPCT.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with a value for RAWPCT.
*/
E_US18E1_6369_AREA_NOT_RAW:SS50000_MISC_ERRORS
"RAWPCT must be zero when assigning a location to a non-raw area."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The user attempted to CREATE or ALTER a location whose Area
**	is not raw but specified a non-zero value for RAWPCT.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with a zero value for RAWPCT.
*/
E_US18E2_6370:SS42000_SYN_OR_ACCESS_ERR
"ENABLE/DISABLE AUDIT: Either audit type '%0c' is invalid,\n
or this audit type was specified incorrectly.\n
Please check the specification of this audit type and reenter the command."
/*%
** Parameters:
**	%0c - audit type
**
** Explanation:
**	The user attempted to issue the "ENABLE/DISABLE SECURITY_AUDIT"
**	statement, but the specified audit type was invalid or improperly
**	specified.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the correct audit type
**	specified.
*/
E_US18E3_6371:SS42000_SYN_OR_ACCESS_ERR
"ENABLE/DISABLE AUDIT:  A value must be provided when 'LEVEL' is specified.\n
Please reenter the statement with a value specified for 'LEVEL'."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "ENABLE/DISABLE SECURITY_AUDIT"
**	statement and specified "LEVEL", but omitted the level parameter.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the statement with the level parameter
**	specified.
*/
E_US18E4_6372:SS5000A_QUERY_ERROR
"ENABLE/DISABLE AUDIT:  The specified level parameter,\n
'%0c', is invalid.\n
Please check that the level parameter was properly specified\n
and reenter the statement."
/*%
** Parameters:
**	%0c - specified level parameter
**
** Explanation:
**	The user attempted to issue the "ENABLE/DISABLE SECURITY_AUDIT"
**	statement with "LEVEL" specified, but the specified level parameter
**	was invalid.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the statement with the correct level parameter
**	specified.
*/
E_US18E5_6373_INVALID_ROOT_LOC:SS50000_MISC_ERRORS
"A Raw location cannot be used as the root data location."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The user attempted to create a database with the -d option
**	and the <area_name> specified is a raw location.
**
** System Status:
**	The CREATEDB is aborted.
**
** Recommendation:
**	Use an area that is not raw.
*/
E_US18E6_6374:SS42000_SYN_OR_ACCESS_ERR
"%0c: Invalid encrypted password.\n
The specified hex constant must be the exact and full length hex string\n
returned by the Ingres password encryption routine."
/*%
** Parameters:
**	%0c - statement name
**
** Explanation:
**	Hex constants specified for CREATE/ALTER USER/ROLE must be the
**	exact and full length constant that is returned by the encryption
**	algorithm.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should correct the hex constant and reenter the statement.
*/
E_US18E7_6375:SS5000A_QUERY_ERROR
"%0c: This operation is not allowed."
/*%
** Parameters:
**	none
**
** Explanation:
**	The user attempted to adapt location details for iidbdb.
**	This could give rise chicken/egg problems as dbms comes up. 
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	There should be no reason for the user to attempt this.
*/
E_US18E8_6376_EXT_IIDBDB_LOC:SS50000_MISC_ERRORS
"Locations for IIDBDB may not be adjusted."
/*%
** Parameters: 
**	%0c - specified operation
**
** Explanation:
**	A command was attempted that is not allowed in the current installation.
**
** System Status:
**	The operation is rejected.
**
*/
E_US18EC_6380:SS5000A_QUERY_ERROR
"DROP SECURITY_ALARM:  Security alarms can be dropped only from tables.\n
Please ensure the object from which you are attempting\n
to drop a security alarm is a table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to drop a security alarm from a non-table object.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should ensure all specified objects are tables, and reenter
**	the command.
*/
E_US18ED_6381:SS5000A_QUERY_ERROR
"CREATE SECURITY_ALARM:  Security alarms can be created only on tables.\n
Please ensure the object upon which you are attempting\n
to create a security alarm is a table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to create a security alarm on a non-table object.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should ensure all specified objects are tables, and reenter
**	the command.
*/
E_US18EE_6382:SS42000_SYN_OR_ACCESS_ERR
"CREATE SECURITY_ALARM:  Invalid privileges specified.\n
You may only specify SELECT, UPDATE, INSERT and/or DELETE\n
on the WHEN clause of CREATE SECURITY_ALARM.\n
Please reenter the statement will valid privileges specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to create a security alarm with invalid privileges
**	specified on the "WHEN" clause.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should only specify "SELECT", "UPDATE", "INSERT", and/or
**	"DELETE" on the "WHEN" clause.
*/
E_US18EF_6383:SS42000_SYN_OR_ACCESS_ERR
"CREATE SECURITY_ALARM:  Column list specified.\n
You cannot specify a column list in the WHEN clause\n
of the CREATE SECURITY_ALARM statement.\n
Please reenter the statement, but omit the column list."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to create a security alarm with invalid privileges
**	specified on the "WHEN" clause.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should only specify "SELECT", "UPDATE", "INSERT", and/or
**	"DELETE" on the "WHEN" clause.
*/
E_US18F0_6384:SS42000_SYN_OR_ACCESS_ERR
"CREATE SECURITY_ALARM: Invalid condition specified.\n
'%0c', is not a valid condition.\n
Only SUCCESS and/or FAILURE can be specified on the IF clause.\n
Please reenter the statement with correct conditions specified."
/*%
** Parameters:
**	%0c - specified condition
**
** Explanation:
**	The user attempted to create a security alarm, but the specified
**	condition was invalid.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should reenter the command with the correct condition
**	specified.
**
*/
E_US18F6_6390:SS5000H_UNAVAILABLE_RESOURCE
"CREATE/ALTER USER: Group '%0c' does not exist.\n
You cannot specify a non-existing group as a user's default group.\n
The user has been created without a default group, or the user's\n
default group has not been altered. Ensure you entered the correct\n
group identifier, and reset the default group with ALTER USER, if necesary."
/*%
** Parameters:
**	%0c - specified group identifier
**
** Explanation:
**	The user attempted to create or alter a user, but specified a default
**	group identifier that does not exist.
**
** System Status:
**	For "CREATE USER", the statement is not aborted; the user does not
**	have a default group. For "ALTER USER", the statement is not aborted;
**	the user's default group is not changed.
**
** Recommendation:
**	Make sure the group identifier was properly entered.
*/
E_US18F7_6391:SS01000_WARNING
"CREATE/ALTER USER: User '%1c' added to group '%0c'\n
The user was not a member of the specified group,\n
but was added to the group due to the CREATE/ALTER USER request."
/*%
** Parameters:
**	%0c - specified group identifier
**	%1c - specified member identifier
**
** Explanation:
**	This is a warning message to let the user know that the user was
**	added to the specified group.
**
** System Status:
**	The statement is not aborted; this message is information only.
**
** Recommendation:
**	None.
*/
E_US18F8_6392:SS01000_WARNING
"User '%0c' has been removed from all groups.\n
A user is implicitly removed from all groups for which\n
that user was a member when the user is dropped."
/*%
** Parameters:
**	%0c - specified user identifier
**
** Explanation:
**	When dropping a user, the user is removed from all groups for which
**	that user was a member.  This is an informational message only.
**
** System Status:
**	The statement continues.
**
** Recommendation:
**	None.
*/
E_US18F9_6393:SS42502_DUPL_OBJECT
"Unable to create user '%0c'.\n
A role with this name already exists in this installation.\n
User names must be unique within groups, roles, and users.\n
Please specify a different user name."
/*%
** Parameters:
**	%0c - specified user identifier
**
** Explanation:
**	The user attempted to create a user that has the same name as an
**	existing role.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the user identifier was properly specified.  If so, select
**	a different user identifier.
*/
E_US18FA_6394:SS42502_DUPL_OBJECT
"Unable to create user '%0c'.\n
A group with this name already exists in this installation.\n
User names must be unique within groups, roles, and users.\n
Please specify a different user name."
/*%
** Parameters:
**	%0c - specified user identifier
**
** Explanation:
**	The user attempted to create a user that has the same name as an
**	existing group.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the user identifier was properly specified. If so, select
**	a different user identifier.
*/
E_US18FB_6395:SS42502_DUPL_OBJECT
"Unable to create %0c '%1c'\n
A %2c with this name already exists in this installation.\n
Names must be unique within groups, roles, users and profiles."
/*%
** Parameters:
**	%0c - specified object type
**	%1c - specified object name
**	%2c - extant object type
**
** Explanation:
**	The user attempted to create an object (user, group, role, or profile)
**	that has the same name as an existing user, group, role, or profile.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure the user identifier was properly specified.  If so, select
**	a different user identifier.
*/
E_US18FC_6396:SS42505_OBJ_NOT_FOUND
"%0c '%1c' was not found.\n
The specified object does not exist 
or you do not have privilege to access it."
/*%
** Parameters:
**	%0c - object type
**	%1c - object name
**
** Explanation:
**	An object was specified which could not be found.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Make sure the object is properly specified.
*/


E_US18FF_6399:SS5000H_UNAVAILABLE_RESOURCE
"User authorization check failed.\n
Your user identifier was not known to this installation.\n
Contact your system manager for further assistance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	User authorization identifier check failed due to one of the following:
**	    o The real user ID was not known to the installation
**
** System Status:
**	The session is terminated.
**
** Recommendation:
**	Contact the security officer to determine if the user ID
**	is valid.
**
*/
E_US1900_6400_UPD_LOGKEY:SS5000A_QUERY_ERROR
"Error occurred on line %0d, Table '%1c' owned by '%2c', column '%3c'.
The UPDATE or INSERT statement illegally specified a value for a
system_maintained column."
/*%
** Parameters:
**	%0d - line number
**	%1c - table name
**	%2c - table owner
**	%3c - column name
**
** Explanation:
**	This message is issued by PSF when a system_maintained column
**	is specified in either an "INSERT" or "UPDATE" statement.  The
**	value of a system_maintained column cannot be specified by the
**	user; the value is assigned internally by the DBMS.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "INSERT" or "UPDATE" statement without specifying a value
**	for the system-maintained column.
*/
E_US1901_6401_BAD_SYSMNT:SS5000A_QUERY_ERROR
"CREATE or ALTER TABLE: error on attribute '%1c'.\n The 'with system_maintained'
qualifier is not supported on the datatype specified for this attribute
(%0c)."
/*%
** Parameters:
**	%0c - attribute type
**	%1c - attribute name
**
** Explanation:
**	This message is issued by PSF when a system_maintained column
**	is specified in a "CREATE or ALTER TABLE" statement on a datatype which
**	doesn't support 'with system_maintained'.  Currently, only the
**	'table_key' and 'object_key' datatypes support the
**	'with system_maintained' attribute qualifier.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "CREATE or ALTER TABLE" statement after correcting the 
**	invalid attribute declaration.
*/
E_US1902_6402_BAD_ROWLBLTYPE:SS5000A_QUERY_ERROR
"%0c: The attribute '%1c' has a datatype which may not be used
for a row granularity security label (%2c). The datatype must be a 
non-nullable security label or short security label."
/*%
** Parameters:
**	%0c - specified statement
**	%1c - attribute name
**	%2c - attribute datatype
**
** Explanation:
**	This message is issued by PSF when a table is created with
**	row-granularity security labeling, and an attribute was specified
**	that has an inappropriate datatype to hold a security label.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "CREATE TABLE" statement after correcting the invalid
**	attribute declaration.
*/
E_US1903_6403_BAD_LABEL_GRAN:SS5000A_QUERY_ERROR
"%0c: %1c is not a legal value for the WITH LABEL_GRANULARITY
clause. The allowed values are ROW, TABLE or SYSTEM_DEFAULT."
/*%
** Parameters:
**	%0c - specified statement
**	%1c - specified value
**
** Explanation:
**	This message is issued by PSF when a table is created with
**	row-granularity security labeling, but an invalid granularity
**	was specified.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the statement after correcting the invalid granularity
**	declaration.
*/

E_US1905_INTEGRITY_VIOLATION:SS23500_CHECK_CONS_VIOLATION
"%0c integrity constraint '%1c'\n on table '%2c' was violated."
/*%
** Parameters:
**	%0c - ANSI constraint type ("CHECK", "UNIQUE", "REFERENTIAL")
**	%1c - constraint name
**	%2c - table name
**
** Explanation:
**	This message is emitted at query execution time when an
**	"UPDATE/INSERT/DELETE" action violates an ANSI integrity.
**
**	NOTE: This error will now only be used by CHECK constraints,
**	as the four new error messages below will be used by REF constraints,
**	and UNIQUE constraint errors are the same as normal 6.4 UNIQUE
**	errors. 
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look up the constraint text in the iiconstraints catalog, using
**	the constraint name in the error message.  This is the condition
**	this query violated.
*/
E_US1906_REFING_FK_INS_VIOLATION:SS23502_REF_FK_INS_VIOLATION
"Cannot INSERT into table '%0c' because the values do not match
those in table '%1c' (violation of REFERENTIAL constraint '%2c')."
/*%
** Parameters:
**	%0c - referencing (FOREIGN KEY) table name
**	%1c - referenced (PRIMARY KEY) table name
**	%2c - violated constraint name
**
** Explanation:
**	This message is emitted at query execution time when an
**	"INSERT" action violates an ANSI REFERENTIAL constraint.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look up the constraint text in the iiconstraints catalog, using
**	the constraint name in the error message.  This is the condition
**	this query violated.
**	
**	You must first insert values into the referenced (PRIMARY KEY)
**	table before you can insert those values into the referencing
**	(FOREIGN KEY) table.
*/
E_US1907_REFING_FK_UPD_VIOLATION:SS23503_REF_FK_UPD_VIOLATION
"Cannot UPDATE table '%0c' because the values will not match
those in table '%1c' (violation of REFERENTIAL constraint '%2c')."
/*%
** Parameters:
**	%0c - referencing (FOREIGN KEY) table name
**	%1c - referenced  (PRIMARY KEY) table name
**	%2c - violated constraint name
**
** Explanation:
**	This message is emitted at query execution time when an
**	"UPDATE" action violates an ANSI REFERENTIAL constraint.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look up the constraint text in the iiconstraints catalog, using
**	the constraint name in the error message.  This is the condition
**	this query violated.
**	
**	You must first insert the new values into the referenced (PRIMARY
**	KEY) table before you can update those values in the referencing
**	(FOREIGN KEY) table.
*/
E_US1908_REFED_PK_DEL_VIOLATION:SS23504_REF_PK_DEL_VIOLATION
"Cannot DELETE from table '%0c' because the values are still used
in table '%1c' (violation of REFERENTIAL constraint '%2c')."
/*%
** Parameters:
**	%0c - referenced  (PRIMARY KEY) table name
**	%1c - referencing (FOREIGN KEY) table name
**	%2c - violated constraint name
**
** Explanation:
**	This message is emitted at query execution time when an
**	"DELETE" action violates an ANSI REFERENTIAL constraint.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look up the constraint text in the iiconstraints catalog, using
**	the constraint name in the error message.  This is the condition
**	this query violated.
**	
**	You must first delete values from the referencing (FOREIGN KEY)
**	table before you can insert those values into the referenced
**	(PRIMARY KEY) table.
*/
E_US1909_REFED_PK_UPD_VIOLATION:SS23505_REF_PK_UPD_VIOLATION
"Cannot UPDATE table '%0c' because the values are still used
in table '%1c' (violation of REFERENTIAL constraint '%2c')."
/*%
** Parameters:
**	%0c - referenced  (PRIMARY KEY) table name
**	%1c - referencing (FOREIGN KEY) table name
**	%2c - violated constraint name
**
** Explanation:
**	This message is emitted at query execution time when an
**	"UPDATE" action violates an ANSI REFERENTIAL constraint.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look up the constraint text in the iiconstraints catalog, using
**	the constraint name in the error message.  This is the condition
**	this query violated.
**	
**	You must first delete values in the referencing (FOREIGN KEY)
**	table before you can update those values in the referenced
**	(PRIMARY KEY) table.
*/

E_US1910_6416_CHECK_OPTION_ERR:SS44000_CHECK_OPTION_ERR
"A query will be rolled back because it resulted in a violation
\nof CHECK OPTION defined on view \"%0c\".\"%1c\"."
/*%
** Parameters:
**	%0c - schema name
**	%1c - view name
**
** Explanation:
**	A query ("INSERT" or "UPDATE") against a view which was either itself
**	created "WITH CHECK OPTION" or whose underlying view was created "WITH
**	CHECK OPTION" resulted in creation of rows that are no longer visible
**	through a view created "WITH CHECK OPTION" which constitutes a CHECK
**	OPTION violation.
**
** System Status:
**	The current statement is rolled back.
**
** Recommendation:
**	Correct the query so that it does not result in creation of rows that
**	are no longer visible through a view created "WITH CHECK OPTION", or
**	reformulate it so it no longer involves a view "WITH CHECK OPTION" for
**	which has been violated by the query.
*/
E_US1911_REFINT_RESTR_VIOLATION:SS23001_REF_RESTRICT_VIOLATION
"Cannot DELETE/UPDATE table '%0c' because the values are still used
in table '%1c' (violation of REFERENTIAL constraint '%2c' with
RESTRICT referential action)."
/*%
** Parameters:
**	%0c - referenced  (PRIMARY KEY) table name
**	%1c - referencing (FOREIGN KEY) table name
**	%2c - violated constraint name
**
** Explanation:
**	This message is emitted at query execution time when a
**	"DELETE" or "UPDATE" action on the referenced table 
**	violates an ANSI REFERENTIAL constraint with the RESTRICT 
**	action.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Look up the constraint text in the iiconstraints catalog, using
**	the constraint name in the error message.  This is the condition
**	this query violated.
**	
**	You must first delete or update matching values in the referencing 
**	(FOREIGN KEY) table before you can delete or update those values 
**	in the referenced (PRIMARY KEY) table.
*/
E_US1912_6418_SEQ_EXISTS:SS42502_DUPL_OBJECT
"CREATE SEQUENCE: The sequence '%0c' already exists."
/*%
** Parameters:
**	%0c - sequence name
**
** Explanation:
**	The name specified in the "CREATE SEQUENCE" statement is the same
**	as a sequence that already exists.
**
** System Status:
**	The program continues, but the current "CREATE SEQUENCE" statement
**	is aborted.
**
** Recommendation:
**	A sequence can only be created once; verify that the sequence name
**	was entered correctly.
*/
E_US1913_6419_SEQ_NOT_EXISTS:SS42505_OBJ_NOT_FOUND
"ALTER/DROP SEQUENCE: The sequence '%0c' was not found."
/*%
** Parameters:
**	%0c - sequence name
**
** Explanation:
**	The sequence specified in the "ALTER/DROP SEQUENCE" statement does
**	not exist.
**
** System Status:
**	The program continues, but the current "ALTER/DROP SEQUENCE" statement
**	is aborted.
**
** Recommendation:
**	Verify that the sequence name was entered correctly; only existing
**	sequences may be dropped or altered.
*/
E_US1914_6420_SEQ_NOT_FOUND:SS42000_SYN_OR_ACCESS_ERR
"%0c: Sequence '%1c' referenced in next/current value was not found."
/*%
** Parameters:
**	%0c - query containing error
**	%1c - sequence name
**
** Explanation:
**	The sequence specified in the identified query does not exist.
**
** System Status:
**	The program continues, but the current statement
**	is aborted.
**
** Recommendation:
**	Verify that the sequence name was entered correctly; sequences must
**	be defined before they may be referenced in a query.
*/
E_US1915_6421_SEQ_LIMIT_EXCEEDED:SS2200H_SEQ_EXCEEDED
"The ending value of Sequence '%0c' has been reached."
/*%
** Parameters:
**	%0c - Sequence name
**
** Explanation:
**	The Sequence was created with the "NO CYCLE" option and
**	the next value exceeds the range of the sequence.
**
** System Status
**	The statement is aborted.
**
** Recommendation:
**	Recreate or alter the Sequence to specify "CYCLE", or
**	increase the range of values of the Sequence.
*/
E_US1916_6422_ALTCOL_BAD_TYPE:SS50000_MISC_ERRORS
"ALTER TABLE : Invalid datatype for alter table alter column. \n
Please check the error log (errlog.log) for more details. "
/*%
** Parameters:
**      None.
**
** Explanation:
**      The user attempted to alter the columns of the table
**	however the subject column cannot be modified to the
**	column type provided by the user.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      The user should reenter the statement with the correct type 
**      for paramter specified on the "ALTER TABLE" statement.
*/
E_US1917_6423_ACOL_KEY_NOT_ALLOWED:SS50000_MISC_ERRORS
"ALTER TABLE : Illegal column for alter table alter column. \n
Alter table cannot alter a column which is part of a key. Please check
the error log (errlog.log) for more details. "
/*%
** Parameters:
**      None.
**
** Explanation:
**      The user attempted to alter the columns of the table
**      which is part of a key. Alter table does not support
**      altering columns of a key.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      The table structure should be modified to remove the
**	column as the key. Redine the key once alter table 
**	statement is done and modify of the table is completed.
*/
E_US1918_6424_COLL_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"CREATE/ALTER TABLE: Named collation '%0c' is not defined"
/*%
** Parameters:
**	%0c - collation name
**
** Explanation:
**	The collation specified in a column definition in the
**	 "CREATE/ALTER TABLE" statement does not exist.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER TABLE" statement
**	is aborted.
**
** Recommendation:
**	Verify that the collation name was entered correctly; only existing
**	collations may be referenced in a column definition.
*/
E_US1919_6425_COLL_NOT_SUPPORTED:SS50000_MISC_ERRORS
"CREATE/ALTER TABLE: Column level collation '%0c' is not yet supported"
/*%
** Parameters:
**	%0c - collation name
**
** Explanation:
**	The collation specified in a column definition in the
**	 "CREATE/ALTER TABLE" statement is not yet supported, though 
**	it can be specified at the installation level.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER TABLE" statement
**	is aborted.
**
** Recommendation:
**	Verify that the collation name was entered correctly; only 
**	"sql_character", "unicode" and "unicode_case_insensitive" are
**	supported for column level collations.
*/
E_US191A_6426_COLL_TYPE_ERROR:SS50000_MISC_ERRORS
"CREATE/ALTER TABLE: Named collation '%0c' can only be used with '%1c' types"
/*%
** Parameters:
**	%0c - collation name
**	%1c - data type names
**
** Explanation:
**	The collation specified in a column definition in the
**	"CREATE/ALTER TABLE" statement cannot be used with the specified
**	data types.
**
** System Status:
**	The program continues, but the current "CREATE/ALTER TABLE" statement
**	is aborted.
**
** Recommendation:
**	Verify that the collation name was entered correctly; named collations
**	typically apply to only one set of either Unicode types (NCHAR,
**	NVARCHAR) or non-Unicode types (CHAR, VARCHAR, C, TXT).
*/
E_US191B_6427_COLL_COMP:SS50000_MISC_ERRORS
"Comparison was attempted between columns with incompatible collations"
/*%
** Parameters:
**
** Explanation:
**	A comparison was attempted between 2 columns or expressions with
**	different and incompatible collations. Comparisons may only
**	be performed between columns/expressions with the same collation or
**	collations that are compatible.
**
** System Status:
**	The program continues, but the current query is aborted.
**
** Recommendation:
**	Resolve the collation conflict and try the query again.
*/
E_US191C_6428_NODUPLICATES:SS50000_MISC_ERRORS
"CREATE/MODIFY: Rows that span pages with noduplicates is not supported."
E_US1930_6448_PARTITION_EXPECTS:SS42000_SYN_OR_ACCESS_ERR
"%0c: The PARTITION= option is expecting '%1c', but saw '%2c'."
E_US1931_6449_PARTITION_BADOPT:SS42000_SYN_OR_ACCESS_ERR
"%0c: The WITH option '%1c' is not allowed inside a partitioning definition."
E_US1932_6450_PARTITION_NOTALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: The PARTITION= WITH option is not allowed in combination with %1c."
/* Note: I think there is only too many dimensions, if more toomany's
** turn up, parameterize the "what".
*/
E_US1933_6451_PART_TOOMANY:SS42000_SYN_OR_ACCESS_ERR
"%0c: Too many partitioning dimensions defined, maximum allowed is %1d."
E_US1934_6452_PART_BADCOL:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c %2c was %3c; processing the PARTITION= option for table %4c."
E_US1935_6453_PART_BADCOLTYPE:SS5000A_QUERY_ERROR
"%0c: Column %1c cannot be used as a partitioning column, because its
datatype does not allow it."
/*%
** Parameters:
**	%0c - Query name containing PARTITION definition
**	%1c - Column name
**
** Explanation:
**	Some datatypes, such as LONG VARCHAR, are marked as non-sortable
**	or non-keyable.  Any column used as a partitioning column must be
**	an ordinary sortable datatype.  LIST and RANGE distributions also
**	exclude logical key and BIT datatypes.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Choose a different column for a partitioning column.
*/
E_US1936_6454_PART_NNAMES:SS42000_SYN_OR_ACCESS_ERR
"%0c: A PARTITION clause defined %1d partitions, but %2d partition names
were listed."
/*%
** Parameters:
**	%0c - The query name
**	%1d - The number of partitions to be defined
**	%2d - The number of names listed
**
** Explanation:
**	AUTOMATIC and HASH partitions are defined using a PARTITION
**	clause of the form:  nn PARTITIONS (name, ...)  where either
**	the number of partitions nn, or the name list, or both may
**	be omitted.  The number of partitions defaults to 1.  If
**	the name list is not omitted entirely, a name must be listed
**	for every partition to be defined.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	List the same number of names as the number of partitions nn,
**	or omit the name list entirely and allow names to be generated
**	by the system.
*/
E_US1937_6455_PART_VALTOOLONG:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c partitioning constant '%2c' for column %3c is too long.
\n The maximum allowed constant size is %4d bytes."
/*%
** Parameters:
**	%0c - The query name
**	%1c - The distribution rule for this partitioning dimension
**	%2c - The start of the constant
**	%3c - The partitioning column
**	%4d - The maximum allowed length
**
** Explanation:
**	The partitioning system catalogs have a fixed maximum
**	size for storing LIST or RANGE partitioning constants.
**	A constant larger than that size cannot be used as
**	a partitioning constant, as it cannot be stored.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Use a shorter partitioning constant.  For RANGE, use
**	a leading portion of the string constant rather than the
**	entire string.
*/
E_US1938_6456_PART_VALTYPE:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c partitioning constant does not match datatype of column %2c.
\n %3c."
E_US1939_6457_PART_VALCOUNT:SS42000_SYN_OR_ACCESS_ERR
"%0c: Wrong number of value components in partitioning constant.
\n Expected %1d components, saw %2d; error near '%3c'."
E_US1940_6458_PART_INVALID:SS42000_SYN_OR_ACCESS_ERR
"%0c: Error in partition definition: %1c."
E_US1941_6459_NOT_PART:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c is not a partitioned table."
E_US1942_6460_EXPECTED_BUT:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c was expected, but saw '%2c'."
E_US1943_6461_NOT_SUBPART:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c is not a subpartition of logical partition %2c."
/*%
** Parameters:
**	%0c - The query name
**	%1c - This logical partition name
**	%2c - The preceding logical partition name
**
** Explanation:
**	When specifying a particular logical partition with the
**	PARTITION logpart.logpart... syntax, the logical partition names
**	must occur in order by increasing dimension (that is, from outer
**	dimensions towards inner dimensions).
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	List the logical partition names in the proper order.
*/
E_US1944_6462_STRUCTURE_REQ:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c is only allowed with MODIFY TO storage structure, or
MODIFY TO RECONSTRUCT."
E_US1945_6463_LPART_NOTALLOWED:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c is not allowed when logical partition(s) are listed."
/*%
** Parameters:
**	%0c - The query name
**	%1c - The operation or WITH-option in question
**
** Explanation:
**	The query has specified one or more specific logical partitions,
**	using the table PARTITION partition-name syntax;  but the operation
**	or WITH-option must operate on an entire table, not part of a
**	table.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Either list just the table name, or choose a different operation
**	or WITH-option.
*/
E_US1946_6464_PPART_ONLY:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c is only allowed when one specific partition is named."
E_US1947_6465_X_NOTWITH_Y:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c may not be used with %2c."
/*%
** Parameters:
**	%0c - The query name
**
** Explanation:
**	The query has specified a combination of operations or options
**	that is not allowed.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Choose a different combination of operations.
*/
E_US1948_6466_UNIQUE_PART_INCOMPAT:SS42000_SYN_OR_ACCESS_ERR
"%0c: The storage structure cannot be UNIQUE because of the %1c partitioning scheme."
/*%
** Parameters:
**	%0c - The query name
**
** Explanation:
**	When a UNIQUE storage structure is applied to a partitioned table,
**	Ingres requires that the uniqueness checking be local to each
**	partition.  If rows with a given storage structure key might be
**	located in more than one partition, uniqueness cannot be enforced
**	via the storage structure.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Make the partitioning columns equal to (or a subset of) the
**	desired storage structure key.
**	Or, enforce uniqueness with a global index, or a UNIQUE constraint.
*/
E_US1949_6467_PART_TOOMANYP:SS42000_SYN_OR_ACCESS_ERR
"%0c: Too many physical partitions defined.  This partition definition
requires %1d partitions, but the maximum allowed is %2d."
/*%
** Parameters:
**	%0c - The query name
**	%1d - The number of physical partitions requested
**	%2d - The maximum number allowed
**
** Explanation:
**	The number of physical partitions is the product of the number
**	of partitions in each dimension of your partitioning scheme.
**	There is a maximum number of physical partitions allowed by
**	this version of Ingres, and your partitioning scheme exceeds it.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Revise your partition definition so that fewer physical partitions
**	are required.
*/
E_US1950_6468_NO_PARTRULE_SPEC:SS42000_SYN_OR_ACCESS_ERR
"%0c: The PARTITION= clause is missing a partition rule."
E_US1951_6481_PART_ONEP:SS42000_SYN_OR_ACCESS_ERR
"%0c: Partition dimension %1d is defined with only a single partition."
/*%
** Parameters:
**     %0c - The query name
**     %1d - The dimension (counting from 1)
**
** Explanation:
**     Some partitioning dimension was defined with only a single
**     partition in it.  This defeats the purpose of having a
**     partitioning, and is not allowed.
**
** System Status:
**     The query is aborted.
**
** Recommendation:
**     Revise your partition definition so that each dimension has at
**     least two partitions defined.
*/
E_US1963_6499_X_ONLYWITH_Y:SS42000_SYN_OR_ACCESS_ERR
"%0c: %1c may only be used with %2c."
/*%
** Parameters:
**	%0c - The query name
**
** Explanation:
**	The query has specified a combination of operations or options
**	that is not allowed.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Choose a different combination of operations.
*/
E_US1964_6500:SS42503_INSUF_PRIV
"You cannot perform DDL operations such as \n
DESTROY/INDEX/MODIFY/RELOCATE/CREATE VIEWS \n
DROP VIEWS/GRANT PRIVILEGES/REVOKE PRIVILEGES \n
unless your current security label exactly matches
the security label on the object you are accessing, 
or you have appropriate privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	For a B1 secure installation, any operation which alters the state of 
**	an object will be rejected if the current subject label (user label)
**	does not exactly match the label associated with the object.
**	For example, if you created a table with a security label of
**	"unclassified", you cannot modify the table if you are currently
**	running at top-secret, even though a top-secret user can see
**	unclassified data.  This is part of our B1 security model.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should log in with the correct security label, and re-issue
**	the operation.
*/
E_US1965_6501:SS42503_INSUF_PRIV
"The requested security DML operation is not allowed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	For a B1 secure installation, any operation which alters the state 
**	of a row will be rejected if the user does not have appropriate 
**	privileges or the new label would be invalid for the indicated database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	The user should log in with the correct security label, and re-issue
**	the operation.
*/
E_US1966_6502:SS42503_INSUF_PRIV
"The row security label must dominate the table security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A row security label must dominate the label of the table in
**	which it occurs.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Label the row so that it dominates the table label.
*/
E_US1967_6503_DEF_PRIVS_NSUB:SS5000A_QUERY_ERROR
"The default privileges for a user/profile must be included in the full 
privileges for that user/profile."
/*%
** Parameters:
**	 None.
**
** Explanation:
**	When issing a "CREATE/ALTER USER/PROFILE" statement, the full
**	privileges did not include all the default privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Make sure all default privileges are included in the full privileges.
*/
E_US1968_6504_UNOPRIV_WDEFAULT:SS5000A_QUERY_ERROR
"Create user statement must contain nodefault_privileges, when noprivileges and profile options are provided together. User may inherit default privileges from the profile."
/*%
** Parameters:
**       None.
**
** Explanation:
**	When issing a "CREATE/ALTER USER" statement, noprivileges and 
**	'with profile' been specified but nodefault_privilege has not been
**	included, this is ambiguous, as the profile may contain 
**	default_privileges, which the user will get and we wish user should
**	have no privileges. 
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**	Specify -nodefault_privileges, specification with -noprivileges, for 
**	create user statement, where profile is specified.
*/
E_US1A00_6656:SS5000A_QUERY_ERROR
"%0c: '%1c' statements are not allowed."
/*%
** Parameters:
**	%0c - statement type (e.g., "CREATE SCHEMA")
**	%1c - substatement type (e.g., "CREATE TABLE ... AS SELECT")
**
** Explanation:
**	The substatement type is not allowed within the statement.
**	For example, "CREATE TABLE ... AS SELECT" is not allowed
**	within the "CREATE SCHEMA" wrapper.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_US1A01_6657:SS5000A_QUERY_ERROR
"%0c: An error was detected in the following substatement:
\n'%1c'"
/*%
** Parameters:
**	%0c - statement type (e.g., "CREATE SCHEMA")
**	%1c - substatement text
**
** Explanation:
**	A semantic error was detected in the listed substatement.
**	Subsequent error message(s) detail the nature of the error.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Read the subsequent messages and correct the statement.
*/
E_US1A02_6658:SS5000A_QUERY_ERROR
"%0c: You may not create object '%2c' in foreign schema '%1c'."
/*%
** Parameters:
**	%0c - statement type (e.g., "CREATE SCHEMA")
**	%1c - foreign schema name
**	%2c - object name
**
** Explanation:
**	An attempt was made to create an object in a foreign schema
**	within the "CREATE SCHEMA" wrapper.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_US1A03_6659:SS5000A_QUERY_ERROR
"%0c: Object '%1c' was referenced but not defined."
/*%
** Parameters:
**	%0c - statement type (e.g., "CREATE SCHEMA")
**	%1c - object name
**
** Explanation:
**	The specified object was referenced but not defined.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_US1A04_6660:SS5000A_QUERY_ERROR
"%0c: Object '%1c' was involved in a circular dependency chain."
/*%
** Parameters:
**	%0c - statement type (e.g., "CREATE SCHEMA")
**	%0c - object name
**
** Explanation:
**	The specified object was part of a circular dependency chain.
**	For example, if the following statements were issued:
**	
**		CREATE VIEW v1 AS SELECT * FROM v2
**		CREATE VIEW v2 AS SELECT * FROM v1
**
**	both v1 and v2 are involved in a circular dependency chain.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Correct the statement and reenter.
*/
E_US1A05_6661:SS5000A_QUERY_ERROR
"%0c: You do not have permission perform this operation \n
on a schema owned by '%1c'."
/*%
** Parameters:
**	%0c - statement type (e.g., "CREATE SCHEMA")
**	%1c - schema owner
**
** Explanation:
**	The user attempted to perform an operation, such as "CREATE SCHEMA",
**	on a schema owned by another user. For example, if the following
**	statement is issued:
**	
**	    CREATE SCHEMA AUTHORIZATION foo;
**
**	but the session's effective userid is "bar", the statement would fail.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	To perform this operation, you must connect to the DBMS with an
**	effective user ID the same as the schema owner.
*/
E_US1A06_6662:SS5000A_QUERY_ERROR
"CREATE TABLE: The WITH NOSECURITY_AUDIT_KEY clause may not be 
specified since the audit configuration for this table requires a
security audit key."
/*%
** Parameters:
**	None
**
** Explanation:
**	A  WITH NOSECURITY_AUDIT_KEY clause was specified but the
**	table audit configuration requires a security audit key to be
**      present.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Specify an appropriate combination of WITH clause options or
**      default values when creating the table.
*/
E_US1A11_ID_ANSI_TOO_LONG:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
violates ANSI specifications for length."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified an identifier that violates ANSI SQL92 entry
**	length restriction of 18 characters.  This messages is issued
**	only when ANSI checking is performed by the DBMS, and may be
**	considered a warning by the application.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	Ensure that the identifier specified is valid in your environment.
*/
E_US1A12_ID_ANSI_END:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
violates ANSI specifications for ending characters."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified an identifier that violates ANSI SQL92 entry
**	restrictions on ending characters.  This messages is issued
**	only when ANSI checking is performed by the DBMS, and may be
**	considered a warning by the application.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	Ensure that the identifier specified is valid in your environment.
*/
E_US1A13_ID_ANSI_BODY:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
violates ANSI specifications for valid body characters."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified an identifier that violates ANSI SQL92 entry
**	restrictions on body characters.  This messages is issued
**	only when ANSI checking is performed by the dbms, and may be
**	considered a warning by the application.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	Ensure that the identifier specified is valid in your environment.
*/
E_US1A14_ID_ANSI_START:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
violates ANSI specifications for valid starting characters."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified an identifier that violates ANSI SQL92 entry
**	restrictions on starting characters.  This messages is issued
**	only when ANSI checking is performed by the DBMS, and may be
**	considered a warning by the application.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	Ensure that the identifier specified is valid in your environment.
*/
E_US1A20_ID_START:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
starts with an invalid character."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified an identifier that violates restrictions
**	on starting characters.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the identifier was properly specified.
*/
E_US1A21_ID_DLM_BODY:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
contains invalid characters."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified a delimited identifier that violates 
**	restrictions on characters.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the identifier was properly specified.
*/
E_US1A22_ID_BODY:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
contains invalid characters."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified a regular identifier that violates 
**	restrictions on characters.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the identifier was properly specified.
*/
E_US1A23_ID_DLM_END:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
does not end with a delimiter."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified a delimited identifier that does not end
**	with a delimiter.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the identifier was properly specified.
*/
E_US1A24_ID_TOO_SHORT:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
contains no valid characters."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified a delimited identifier that does not
**	contain any non-white characters.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the identifier was properly specified.
*/
E_US1A25_ID_TOO_LONG:SS5000A_QUERY_ERROR
"%0c starting with '%1c' \n
is too long."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	The user specified an identifier that exceeds identifier
**	length restriction of 32 characters.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the identifier was properly specified.
*/
E_US1A26_CU_ID_PARAM_ERROR:SS5000B_INTERNAL_ERROR
"Internal error:\n
Invalid parameter passed to cui_id function\n
(while processing a '%0c' starting with '%1c')."
/*%
** Parameters:
**	%0c - identifier type
**	%1c - identifier prefix
**
** Explanation:
**	Internal call to cui function passed it an invalid parameter
**	(e.g. invalid length).
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Record the query that caused the problem, and contact your support
**	representative.
*/
E_US1F42_8002:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: Bad gateway storage structure '%0c'.\n
The valid storage structure names are 'keyed', 'fullkey', 'sortkeyed' or 'none'.
Not all structures are valid for all gateways.  Consult the User's Manual for
your gateway for information on which structures are valid and how to use them."
E_US1F43_8003:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: invalid option '%0c'. Try NOT NULL WITH DEFAULT."
E_US1F44_8004:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: either invalid attribute qualifier '%0c', wrong order,
or wrong combination of qualifiers.  Try
'[[not null [with | not default]] | with null]'."
/*%
** Parameters:
**	%0c - attribute qualifier
**
** Explanation:
**	The accepted syntax diagram for the column qualifier clause is:
**
**	    '[[not null [with | not default]] | with null]
**	        [not | with system_maintained]'
**
**	with one exception. It is not allowed to specify either "not default"
**	or "with null" in conjunction with the "with system_maintained"
**	qualifier.
**
**	The order of these qualifiers is significant and must appear as
**	described.  The following is a complete list of accepted attribute
**	qualifiers (a few options, although syntactically correct per the
**	above diagrams described by the syntax diagram, are disallowed due
**	to the "with system_maintained" restrictions):
**
**	    with null
**	    not null with default
**	    not null not default
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement after correcting the problem with
**	the attribute qualifier clause.
*/
E_US1F45_8005:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: Invalid option '%0c' in 'with' clause.  Either the option
itself is invalid, or it has been incorrectly qualified."
/*%
** Parameters:
**	%0c - specified option
**
** Explanation:
**	Either an incorrect option has been specified, or it has been
**	qualified incorrectly.
**
**	Valid options are:
**
**	    dbms = rms          only valid dbms qualifier in 6.3/02 (vax.vms)
**	    rows = nnnn         where nnnn is an integer indicating # of rows
**	    path = 'string'     where string indicates a path value
**	    [no]duplicates      no qualifier
**	    [no]journaling      no qualifier
**	    [no]recovery        no qualifier
**	    [no]update          no qualifier
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement after correcting the problem.
*/
E_US1F47_8007:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: invalid option '%0c'. Legal single-word options for the 'with'
clause are:  [no]duplicates, [no]journaling, [no]update, [no]recovery."
E_US1F48_8008:SS50002_LIMIT_EXCEEDED
"REGISTER TABLE: excessive table width on column '%0c'."
E_US1F49_8009:SS5000A_QUERY_ERROR
"REGISTER TABLE: '%0c' is a system table."
E_US1F4A_8010:SS42502_DUPL_OBJECT
"REGISTER TABLE: Duplicate object name '%0c'."
E_US1F4B_8011:SS50002_LIMIT_EXCEEDED
"REGISTER TABLE: the number of columns must not exceed 1024."
E_US1F4C_8012:SS42507_RESERVED_IDENTIFIER
"REGISTER TABLE: invalid column name '%0c'."
E_US1F4D_8013:SS42502_DUPL_OBJECT
"REGISTER TABLE/INDEX: duplicate column name '%0c'."
E_US1F4E_8014:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: invalid column format '%0c' on column '%1c'."
E_US206E_8302:SS42501_COL_NOT_FOUND
"REGISTER INDEX: Invalid column '%0c'."
E_US206F_8303:SS42503_INSUF_PRIV
"REGISTER INDEX: Table '%0c' not owned by you."
E_US2070_8304:SS5000A_QUERY_ERROR
"REGISTER INDEX: Table '%0c' is already an index."
E_US2072_8306:SS5000A_QUERY_ERROR
"REGISTER INDEX: '%0c' is a view and an index can't be built on it."
E_US2073_8307:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: duplicate column '%0c'."
E_US2077_8311:SS50008_UNSUPPORTED_STMT
"The REGISTER INDEX statement is not allowed in distributed INGRES."
E_US2079_8313:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: Bad storage structure '%0c'."
/*%
** Parameters:
**	%0c - specified structure name
**
** Explanation:
**	An invalid storage structure was specified when "REGISTER"ing a gateway
**	secondary index.  Current valid storage structures for a gateway
**	secondary index are: keyed; fullkey; and sortkeyed.
**
**	Not all of the above storage structures are valid for every gateway.
**	See your gateway's user's guide for a list of valid storage structures.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement after correcting the storage structure.
*/
E_US207A_8314:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: '%0c' is either an invalid option or the syntax of the WITH
subclause is invalid."
E_US2087_8327:SS42000_SYN_OR_ACCESS_ERR
"REGISTER INDEX: Too many options in the WITH clause."
E_US2088_8328:SS50002_LIMIT_EXCEEDED
"REGISTER INDEX: Too many columns in key of index."
E_US213E_8510:SS42000_SYN_OR_ACCESS_ERR
"Column name in list incorrectly modified (word seen is '%0c').  The format of
a column list is '(col_name [ASC | DESC] {,col_name [ASC | DESC]})."
E_US215C_8540:SS5000A_QUERY_ERROR
"REGISTER INDEX: %0c: invalid or no storage structure specified for gateway
secondary index."
/*%
** Parameters:
**	%0c - index name
**
** Explanation:
**	An invalid storage structure, or no storage structure, was specified
**	when "REGISTER"ing a gateway secondary index.  Current valid storage
**	structures for a gateway secondary index are: keyed; fullkey; and
**	sortkeyed.
**
**	Not all of the above storage structures are valid for every gateway.
**	See your gateway's user's guide for a list of valid storage structures.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement after correcting the storage structure.
*/
E_US222E_8750:SS5000A_QUERY_ERROR
"REMOVE TABLE: object '%0c' is not a base table."
E_US2230_8752:SS5000A_QUERY_ERROR
"REMOVE INDEX: object '%0c' is not an index."
E_US2232_8754:SS5000A_QUERY_ERROR
"REMOVE: Too many objects specified."
E_US2233_8755:SS5000A_QUERY_ERROR
"REMOVE [TABLE]: object '%0c' is not a gateway object"
E_US239A_9114:SS50008_UNSUPPORTED_STMT
"The REGISTER command is not allowed in distributed INGRES."
E_US245B_9307:SS5000A_QUERY_ERROR
"REGISTER TABLE/INDEX: duplicate column name '%0c' specified in 'key' clause."
E_US245C_9308:SS50002_LIMIT_EXCEEDED
"REGISTER TABLE: name for gateway object in 'from' clause is too long."
E_US245D_9309:SS5000A_QUERY_ERROR
"REGISTER TABLE: duplicate '%0c' entry supplied in 'with' clause."
E_US245E_9310:SS5000A_QUERY_ERROR
"REGISTER TABLE: invalid dbms type '%0c' supplied in 'with' clause."
/*%
** Parameters:
**	%0c - specified type
**
** Explanation:
**	An invalid DBMS type was specified in the 'with dbms=' clause of the
**	"REGISTER TABLE" statement.  Currently, rms is the only valid DBMS
**	type.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER TABLE" statement after correcting the DBMS type.
*/
E_US245F_9311:SS5000A_QUERY_ERROR
"REGISTER TABLE: invalid option '%0c' specified in 'with' clause."
/*%
** Parameters:
**	%0c - specified option
**
** Explanation:
**	An invalid option was specified in the "WITH" clause.  In this context
**	(NAME = NAME), valid options are:
**
**	    structure = keyed | fullkey | sortkeyed | none
**	    dbms = rms | sxa | ima 
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement with one of the valid options for
**	'structure' or 'dbms'.
*/
E_US2460_9312:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: Bad gateway storage structure '%0c' specified with UNIQUE
qualifier.  Valid storage structures which can be qualified with UNIQUE are:
'keyed', 'fullkey', or 'sortkeyed'.  Not all structures are valid for all
gateways.  Consult the User's Manual for your gateway for information on which
structures are valid and how to use them."
E_US2461_9313:SS42000_SYN_OR_ACCESS_ERR
"REGISTER TABLE: Bad option '%0c' specified with UNIQUE qualifier.  Only the
'structure' option may include the UNIQUE qualifier."
E_US2462_9314:SS5000A_QUERY_ERROR
"REGISTER TABLE: duplicate or conflicting '%0c' entry in 'with' clause."
/*%
** Parameters:
**	%0c - specified option
**
** Explanation:
**	A duplicate or conflicting option was specified in the "WITH" clause.
**	For example, both "RECOVERY" and "NORECOVERY" were specified.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement with the desired option specified only
**	once, and without conflicting options being specified.
*/
E_US2463_9315:SS5000A_QUERY_ERROR
"REGISTER TABLE:  No 'DBMS' clause specified.  'DBMS' is a required parameter
in the 'register table' command."
E_US2464_9316:SS5000A_QUERY_ERROR
"REGISTER TABLE:  A 'key' clause has been specified, but no column names were
provided."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "WITH" clause included a 'key=' specifier, but no columns were
**	listed.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, either leaving out the 'key='
**	specifier or listing one or more columns as key columns.  Note that
**	there may be limitations on how many and/or what types of columns may
**	be in a key for a gateway table.  These limitations may be different
**	from those in a native database.  Consult the user's guide for
**	your gateway to find out whether such limitations apply and if so,
**	what they are.
*/
E_US2465_9317:SS5000A_QUERY_ERROR
"REGISTER TABLE:  One or more key columns has been specified for the table, but
the structure has been specified as 'none' or it has not been specified
(default storage structure is 'none')."
E_US2466_9318:SS5000A_QUERY_ERROR
"REGISTER TABLE:  table structure other than 'none' has been specified, but no
keys have been specified."
E_US2468_9320:SS50002_LIMIT_EXCEEDED
"REGISTER TABLE: path parameter is too long."
E_US2469_9321:SS5000A_QUERY_ERROR
"REGISTER TABLE:  A non-keyed table structure was specified along with either or
both of:  'unique', 'noduplicates' qualifiers.  These qualifiers can only be
specified for keyed tables."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "WITH" clause included incompatible qualifiers; namely, a non-keyed
**	table was qualified with either 'unique' and/or 'noduplicates'.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, either specifying a keyed structure
**	or removing the 'unique' and/or 'noduplicates' qualifiers.
*/
E_US246A_9322:SS5000A_QUERY_ERROR
"REGISTER TABLE:  The 'duplicates' qualifier was specified along with a unique
keyed storage type.  These qualifiers are incompatible."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The "WITH" clause included incompatible qualifiers; namely, a unique
**	keyed structure was specified for the table along with the 'duplicates'
**	qualifier. These qualifiers are incompatible (if keys are unique,
**	duplicate rows are not possible).
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, either changing the structure to be
**	non-unique (or non-keyed) or specify 'noduplicates'; or just remove
**	the 'duplicates' qualifier.
*/
E_US246B_9323:SS5000A_QUERY_ERROR
"REGISTER TABLE:  Incorrect structure specified:  structure type '%0c' is not
supported in the %1c gateway."
/*%
** Parameters:
**	%0c - structure type
**	%1c - gateway name
**
** Explanation:
**	The structure type specified is not supported for this gateway.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, specifying a supported structure type
**	for this gateway.
*/
E_US246C_9324:SS5000A_QUERY_ERROR
"REGISTER TABLE: The '%0c' DBMS type is not available in this server."
/*%
** Parameters:
**	%0c - DBMS type
**
** Explanation:
**	The DBMS type specified is not available in this server.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, specifying a DBMS type which is
**	available for this server, or issue the command against an
**	appropriate server or gateway.
*/
E_US246D_9325:SS5000A_QUERY_ERROR
"Invalid security label.\n
Check the spelling and syntax of the security to label to make sure\n
it is legal for your installation."
E_US246E_9326_NEEDS_B1:SS42000_SYN_OR_ACCESS_ERR
"%0c: This statement requires a B1 secure DBMS."
/*%
** Parameters:
**	%0c - specified statement
**
** Explanation:
**	Certain features require a B1 enabled secure DBMS, typically those
**	related to security label or MLS processing.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Only try to execute this command when connected to a B1 secure DBMS.
*/
E_US246F_9327_SESS_LBL_NO_PRIV:SS42000_SYN_OR_ACCESS_ERR
"%0c: You do not have privilege to set your session security label to the
indicated value."
/*%
** Parameters:
**	%0c - specified statement
**
** Explanation:
**	Setting the session security label is a privileged operation. The
**	user tried to set the session label without having the appropriate
**	privileges to do so.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	Only execute this command when you have the appropriate privileges.
*/
E_US2470_9328_DEF_LBL_ERROR:SS42000_SYN_OR_ACCESS_ERR
"Error determining the default security label for an object."
/*%
** Parameters:
**	None.
**	
** Explanation:
**	An error occurred while attempting to determine the current default
**	security label. The default security label is to label newly created
**	objects, and is determined from the current session security label.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Check the error log for more information as to the cause of
**	this error. If the error persists, contact Ingres Corporation
**	Technical Support.
*/
E_US2471_9329_INVLD_SET_PRIV:SS42000_SYN_OR_ACCESS_ERR
"Invalid privileges requested. You may only use privileges which you possess."
/*%
** Parameters:
**	None.
**
** Explanatation:
**	A session attempted to use a privilege which it did not possess.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Only try to use privileges which you have been granted.
*/
E_US2472_9330_ALARM_EXISTS:SS42502_DUPL_OBJECT
"CREATE SECURITY_ALARM: Alarm '%0c' already exists."
/*%
** Parameters:
**	%0c - alarm name
**
** Explanation:
**	The named alarm already exists in the current scope.  Duplicate alarm
**	names, even if applied to different tables, are not allowed.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Choose a unique name for the new alarm, or delete the existing named
**	alarm.
*/
E_US2473_9331_ALARM_ABSENT:SS42505_OBJ_NOT_FOUND
"DROP SECURITY_ALARM: Alarm '%0c' does not exist."
/*%
** Parameters:
**	%0c - alarm name
**
** Explanation:
**	The named alarm does not exist in the current scope.  
**
** System Status:
**	The program continues, but the current "DROP" statement is aborted.
**
** Recommendation:
**	Reenter the alarm name.
*/
E_US2474_9332_ALARM_NO_DB:SS42505_OBJ_NOT_FOUND
"Database '%0c' does not exist.\n
You cannot CREATE SECURITY_ALARM on a database that doesn't exist.\n
Be sure you specified the correct database name."
/*%
** Parameters:
**      %0c - the database name
**
** Explanation:
**      The user specified a non-existing database on the CREATE SECURITY_ALARM
**      statement.  
**
** System Status:
**      The statement continues with other databases specified, if any.
**
** Recommendation:
**      Be sure the database name was properly specified.
**      Call the system administrator if no trouble found.
**
*/
E_US2475_9333_IISECALARM_OPEN:SS50000_MISC_ERRORS
"Error opening iisecalarm catalog. Please check the error log
(errlog.log) for more information why this occurred"
/*%
** Parameters:
**      None
**
** Explanation:
**      The system was unable to open the security alarm catalog.
**
** System Status:
**      Statement is aborted.
**
** Recommendation:
**      Check the error log for more information.
**      Call the system administrator if no trouble found.
**
*/
E_US2476_9334_IISECALARM_ERROR:SS50000_MISC_ERRORS
"Error processing iisecalarm catalog. Please check the error log
(errlog.log) for more information why this occurred"
/*%
** Parameters:
**      None
**
** Explanation:
**      An error was encountered while processing the security alarm catalog.
**
** System Status:
**      Statement is aborted.
**
** Recommendation:
**      Check the error log for more information.
**      Call the system administrator if no trouble found.
**
*/
E_US2477_9335_ALARM_EVENT_ERR:SS50000_MISC_ERRORS
"An error occurred raising the dbevent associated with the 
firing of a security alarm during alarm processing."
/*%
** Parameters:
**      None
**
** Explanation:
**      While trying to raise the dbevent associated with a security
**	alarm, the operation failed unexpectedly.
**
** System Status:
**      Statement is aborted.
**
** Recommendation:
**      Check the error log for more information.
**      Call the system administrator if no trouble found.
**
*/
E_US2478_9336_BAD_ALARM_GRANTEE:SS5000B_INTERNAL_ERROR
"Invalid grantee type (%0d) encountered in IISECALARM tuple for
\nsecurity alarm '%1c'."
/*%
** Parameters:
**      %0d     - grantee type
**      %1c     - alarm name
**
**
** Explanation:
**      An invalid grantee type was discovered while trying to determine if a
**      security alarm tuple applies to the current session.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Delete the alarm associated with this error.  Valid grantee types
**	are: 0 (user); 1 (group); 2 (role); and 3 (public).
*/
E_US2479_9337_ALARM_USES_EVENT:SS5000B_INTERNAL_ERROR
"Dbevent '%0c.%1c' may not be dropped since it is referred to by a security alarm."
/*%
** Parameters:
**      %0c - Owner of event
**	%1c - Name of event
**
** Explanation:
**      An attempt was made to drop a dbevent which is used by a security
**	alarm. You must drop the security alarm first.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Delete the security alarm first, then try to drop the dbevent
*/
E_US247A_9338_MISSING_ALM_EVENT:SS5000B_INTERNAL_ERROR
"Database event %0d:%1d for security alarm %2c could not be found."
/*%
** Parameters:
**	%0d,%1d - Event identifier
**	%2c	- Alarm name
**
** Explanation:
**      An attempt was made to lookup the dbevent associated with
**	a security alarm but the dbevent could not be found. This
**	may indicate an internal error,  inconsistent DBMS catalogs,
**	or some other error.
**
** System Status:
**      The statement is aborted. 
**
** Recommendation:
**	If the condition persists try dropping, the recreating the
**	security alarm and dbevent, or contact your support representative.
*/
E_US247B_9339_IIROLEGRANT_OPEN:SS50000_MISC_ERRORS
"Error opening iirolegrant catalog. Please check the error log
(errlog.log) for more information why this occurred"
/*%
** Parameters:
**      None
**
** Explanation:
**      DBMS server was unable to open this catalog.
**
** System Status:
**      Statement is aborted.
**
** Recommendation:
**      Check the error log for more information.
**      Call the system administrator if no trouble found.
**
*/
E_US247C_9340_IIROLEGRANT_ERROR:SS50000_MISC_ERRORS
"Error processing iirolegrant catalog. Please check the error log
(errlog.log) for more information why this occurred"
/*%
** Parameters:
**      None
**
** Explanation:
**      An error was encountered while processing this catalog.
**
** System Status:
**      Statement is aborted.
**
** Recommendation:
**      Check the error log for more information.
**      Call the system administrator if no trouble found.
**
*/
E_US247D_9341_GRANTEE_ROLE_TYPE:SS42000_SYN_OR_ACCESS_ERR
"GRANT: Role access may not be granted if the grantee 
type is 'group' or 'role'."
/*
** Parameters: 
**	none
**
** Description:
**	An attempt was made to grant access to a role to an invalid grantee
**	type
**
** System status:
**	Statement is aborted
**
** Recommendation:
**	Specify a valid grantee then try again
*/
E_US247E_9342_ROLE_RG_DBDB:SS42000_SYN_OR_ACCESS_ERR
"You cannot GRANT or REVOKE role access \n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement
**	for role access, but the user was not
**	in session with the database database.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Issue these commands only when connected to the database database.
*/
E_US247F_9343_ROLE_RG_NOT_AUTH:SS42000_SYN_OR_ACCESS_ERR
"You are not authorized to GRANT or REVOKE role access."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to issue the "GRANT" or "REVOKE" statement
**	for role access, but is not authorized to do so.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Issue these commands only when you have appropriate privileges.
*/
E_US2480_9344_REVOKE_ROLE_TYPE:SS42000_SYN_OR_ACCESS_ERR
"REVOKE: Role access may not be revoked if the grantee 
type is 'group' or 'role'."
/*
** Parameters: 
**	none
**
** Description:
**	An attempt was made to revoke access to a role from an invalid grantee
**	type
**
** System status:
**	Statement is aborted
**
** Recommendation:
**	Specify a valid grantee then try again
*/
E_US2481_9345_MODIFY_TO_NO_WITH:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: the WITH subclause is invalid for\n
 MODIFY tablename TO\n
  [ PHYS_[IN]CONSISTENT | LOG_[IN]CONSISTENT | TABLE_RECOVERY_[DIS]ALLOWED ]"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has specified options which alter status of a table or index
**	and cannot be used with other options.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Reenter the statement with valid option.
*/
E_US2482_9346:SS42000_SYN_OR_ACCESS_ERR
"An invalid permit operation has been specified."
/*%
** Parameters:
**      None.
**
** Explanation:
**      The user has specified an invalid QUEL permit operation.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Reenter the statement with valid permit operation.
*/
E_US2483_9347:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: %0c is a system catalog and cannot be modified."
/*%
** Parameters:
**      %0c - system catalog table name
**
** Explanation:
**      The user entered a "ALTER TABLE tbl ADD | DROP COLUMN"
**      statement on a sys catalog table
**
**      These statements only apply to database tables.
**
** System Status:
**      The statement is aborted.
**
** Recommendation:
**      Add /Drop columns are not permitted on catalog tables.
*/
E_US2484_9348:SS42000_SYN_OR_ACCESS_ERR
"INTEGRITY on %0c: cannot define integrity on a secondary index."
E_US2485_9349:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX : You may specify only one base table."
E_US2486_9350:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX : Inconsistent use of unique keyword. You may not use 
UNIQUE twice."
E_US2487_9351:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX : You must use concurrent_access with all indexes or
none."
E_US2488_9352:SS42000_SYN_OR_ACCESS_ERR
"CREATE INDEX : You may not create more than 32 indexes concurrently."
E_US2489_9353:SS42503_INSUF_PRIV
"You are not allowed to issue the SET JOINOP TIMEABORT statement\n
while connected to this database.\n
Contact your Database Administrator for further information."
/*%
** Parameters: 
**	none
**
** Description:
**	An attempt was made to revoke access to a role from an invalid grantee
**	type
**
** System status:
**	Statement is aborted
**
** Recommendation:
**	Specify a valid grantee then try again
*/
E_US2490_9360:SS42000_SYN_OR_ACCESS_ERR
"%0c: Cannot %1c '%2c' because it would cause dependent
constraint(s) to be abandoned."
/*%
** Parameters:
**      %0c - operation
**      %1c - lowercase operation
**	%2c - table name
**
** Explanation:
**	Must not drop tables/indexes which are in use by constraints.
**
** System Status:
**	The current statement is aborted; the program continues.
**      Similar behaviour to an attempt to drop a readonly table.
**
** Recommendation:
**	To drop the table, drop the associated constraints, then reissue 
**	the statement.
*/
E_US2491_9361:SS42000_SYN_OR_ACCESS_ERR
"%0c: Cannot %1c '%2c' because it would cause dependent
constraint(s) to be abandoned. Use WITH NODEPENDENCY_CHECK to force
the operation; the user takes responsibility for preserving or recreating
the table structure necessary to enforce the constraint(s)."
/*%
** Parameters:
**	%0c - operation
**	%1c - lowercase operation
**	%2c - table name
**
** Explanation:
**	Must not modify tables/indexes which are in use by constraints.
**
** System Status:
**	The current statement is aborted; the program continues.
**
** Recommendation:
**	To modify the table, either drop the associated constraints, then
**	reissue the statement, or override using the WITH clause
**	NODEPENDENCY_CHECK.
*/

E_US248A_9354_MODIFY_TO_TABLE_OPTION:SS42000_SYN_OR_ACCESS_ERR
"MODIFY: TABLE_OPTION is an invalid option of the\n
 MODIFY tablename TO command."
/*%
** Parameters:
**	%0c - invalid name in "is" clause
**
** Explanation:
**	The name given is not valid within IMA gateway
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, specifying a valid "is" clause
*/
E_US248B_9355:SS5000A_QUERY_ERROR
"REGISTER TABLE: Invalid item name in \"is\" clause: %0c"
/*%
** Parameters:
**
** Explanation:
**	More than a single instance of SERVER and/or VNODE specified in
**	REGISTER TABLE statement
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, specifying only one of SERVER or
**	VNODE
*/
E_US248C_9356:SS5000A_QUERY_ERROR
"REGISTER TABLE: Only one instance of SERVER and/or VNODE\n
 permitted in IMA gateway table."

/*%
** Parameters:
**
** Explanation:
**	IMA tables must be structure=sortkeyed
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, specifying structure=sortkeyed
**	or structure = unique sortkeyed
*/

E_US248D_9357:SS5000A_QUERY_ERROR
"REGISTER TABLE: Structure must be unique sortkeyed or\n
 sortkeyed in IMA gateway table."

/*%
** Parameters:
**
** Explanation:
**	IMA tables must be structure=sortkeyed
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the "REGISTER" statement, specifying structure=sortkeyed
**	or structure = unique sortkeyed
*/
E_US248E_9358:SS5000A_QUERY_ERROR
"CREATE INDEX: Indices may not be created on IMA gateway tables."

E_US2492_9362:SS42000_SYN_OR_ACCESS_ERR
"Sum and Avg functions are not supported for ANSI date/time types."

E_US2493_9363:SS42000_SYN_OR_ACCESS_ERR
"Subselects are not supported within select target lists."

E_US2494_9364:SS5000B_INTERNAL_ERROR
"line %0d, Internal error occured when parsing an ALTER table '%1c'.'%2c' RENAME statement."
/*%
** Parameters:
**
**      %0c - line number.
**      %1c - owner name 
**      %2c - table name
**
** Explanation:
**	Internal parser error occured when parsing an ALTER TABLE
**	...RENAME TO.. statement. This indicates an internal 
**	coding error.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**      Check the error log for more information.
**      Call the system administrator if no trouble found.
*/

E_US2495_9365:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE: Cannot rename table '%0c' to '%1c' as a table 
with the same name already exists."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - new table name
**
** Explanation:
**      The user entered a "ALTER TABLE tbl RENAME TO newtabname "
**	but the newtabname already exists in the datbase and owned by
**	the same user.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Reissue the ALTER TABLE ..RENAME TO statement with a different
**      new table name.	
*/
E_US2496_9366:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is a system catalog."
/*%
** Parameters:
**
**      %1c - table name
**      %2c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	rename of table or column as the table supplied is a system 
**	catalog.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US2497_9367:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is generated by the system."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	rename of table or column as the table supplied is a system 
**	generated table.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US2498_9368:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is a blob extension table."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	rename of table or column as the table supplied is a blob 
**	extension table.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US2499_9369:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is a Gateway table."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	rename of table or column as the table supplied is a Gateway 
**	table.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US249A_9370:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is a Partition."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	rename of table or column as the table supplied is a Partition 
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US249B_9371:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is inconsistent."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	rename of table or column as the table supplied is inconsistent. 
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US249C_9372:SS42000_SYN_OR_ACCESS_ERR
"ALTER TABLE : Cannot rename table '%0c' owned by '%1c', or its columns as 
it is read only table."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	table or column as the table supplied is a 
**	read only table.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
*/
E_US249D_9373:SS42000_SYN_OR_ACCESS_ERR
"Cannot rename table '%0c' owned by '%1c', or it's columns as one or
more objects eg procedures, views, constraints, rules etc depend
on it. Check the error log (errlog.log) for details."
/*%
** Parameters:
**
**      %0c - table name
**      %1c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	table as one or more objects eg
**	views, procedure, rules, constraints, integrities, 
**	non grant permits etc, dependent on it. See the error 
**	log for detail description of the dependent objects.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Delete the dependent objects first and then rename. You
**	can create the dependent objects after renaming.
*/
E_US249E_9374:SS42000_SYN_OR_ACCESS_ERR
"Cannot rename column '%0c' for table '%1c' owned by '%2c', as one or
more objects eg procedures, views, constraints, rules etc depend
on it. Check the error log (errlog.log) for details."
/*%
** Parameters:
**
**      %0c - column name
**      %1c - table name
**      %2c - owner name 
**
** Explanation:
**	Error occured in executing the rename operation on the 
**	table column as one or more objects eg
**	views, procedure, rules, constraints, integrities, 
**	non grant permits etc, dependent on it. See the error 
**	log for detail description of the dependent objects.
**
** System Status:
**	The current statement is aborted.
**
** Recommendation:
**	Delete the dependent objects first and then rename. You
**	can create the dependent objects after renaming.
*/
E_US24B8_9400:SS42000_SYN_OR_ACCESS_ERR
"Invalid ENCRYPT option '%0c'. Valid options are SALT (the default) and NOSALT."
/*%
** Parameters:
**
**      %0c - erroneous option
**
** Explanation:
**	Valid column-level ENCRYPT options are SALT and NOSALT.
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24B9_9401:SS42000_SYN_OR_ACCESS_ERR
"Invalid ENCRYPTION type '%0c'. Valid types are AES128, AES192 and AES256."
/*%
** Parameters:
**
**      %0c - erroneous type
**
** Explanation:
**	Valid table-level ENCRYPT options are AES128, AES192, AES256.
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24BA_9402:SS42000_SYN_OR_ACCESS_ERR
"The encryption type has not been set. Please provide the WITH option
ENCRYPTION= with type AES128, AES192 or AES256 before supplying the
PASSPHRASE= specification."
/*%
** Parameters:
**
** Explanation:
**	This arises when the PASSPHRASE= has been parsed without the
**	ENCRYPTION= type having first been specified.
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24BB_9403:SS42000_SYN_OR_ACCESS_ERR
"The PASSPHRASE must be a minimum of 8 characters in length."
/*%
** Parameters:
**
** Explanation:
**	The PASSPHRASE= value is too short (must be >= 8).
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24BC_9404:SS42000_SYN_OR_ACCESS_ERR
"The NEW_PASSPHRASE= option is not valid when PASSPHRASE='' (disabling
encryption) has been specified for the MODIFY command."
/*%
** Parameters:
**
** Explanation:
**	To change to a new passphrase you have to provide the old one.
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24BD_9405:SS42000_SYN_OR_ACCESS_ERR
"The NEW_PASSPHRASE= option is only valid for the MODIFY command."
/*%
** Parameters:
**
** Explanation:
**	NEW_PASSPHRASE= is for MODIFY, not CREATE.
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24BE_9406:SS42000_SYN_OR_ACCESS_ERR
"Invalid AESKEY= specification. The length of the hex value provided
(%0d) is incorrect for the encryption type. The correct type/length
options are AES128/16 AES192/24 AES256/32."
/*%
** Parameters:
**
**      %0d - length of hex string that the user provided
**
** Explanation:
**	A key specified via AESKEY= must be the exact size of the
**	encryption key for type specified by the ENCRYPTION= parameter.
**
** System Status:
**	The command is not executed.
**
** Recommendation:
**	Correct the syntax error and retry.
*/
E_US24BF_9407:SS5000A_QUERY_ERROR
"A query has been issued against a table for which encryption has not
been unlocked with the MODIFY statement. This unlock statement must be
issued every time the Ingres installation is restarted. Please contact
your system administrator."
E_US24C0_9408:SS5000A_QUERY_ERROR
"The table is not encrypted."
E_US24C1_9409:SS5000A_QUERY_ERROR
"Incorrect passphrase specified."
E_US24C2_9410:SS5000A_QUERY_ERROR
"The key for a modify or index operation is of a datatype which
is either non-keyable or non-sortable. All key fields must be both
keyable and sortable and (for primary keys) cannot be encrypted. An
encrypted index may contain only a single encrypted column."
E_US24C3_9411:SS5000A_QUERY_ERROR
"Invalid encrypted index. An encrypted index must: (1) contain only one
column, (2) that column must be encrypted with NOSALT, (3) the index
must be of structure HASH."
E_US24C4_9412:SS50000_MISC_ERRORS
"line '%0d', string constant supplied exceeds the maximum limit of '%1d' for
UTF8 installation."

NO_SQLSTATE_MAPPING
