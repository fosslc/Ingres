/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<er.h>
# include	<fe.h>
# include	<ui.h>
# include	<ug.h>
# include	<adf.h>
# include	<afe.h>
# include	<uigdata.h>
EXEC SQL INCLUDE <ui.sh>;
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"
# include	<cv.h>

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfckcap.sc - Check the dbcapabilities.
**
** Description:
**	This file defines:
**
**	xfcapset	Find and cache all capability information.
**
** History:
**	21-apr-89 (marian) written.
**	26-apr-89 (marian)
**		Check the INGRES/QUEL_LEVEL instead of INGRES.
**	09-apr-90 (marian)
**		Integrate porting change (piccolo # 130926).
**		Fixed programming error - 'with_rules' was being set to value
**		returned by IIUIdcl_logkeys() and 'with_log_key' was being set
**		to value returned by IIUIdcu_rules().
**	14-may-91 (billc)
**		Fix 37530, 37531, 37532 - STAR stuff.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**	17-nov-93 (robf)
**              Add With_hidden_columns for secure,
**	        With_security_alarm
**	17-dec-93 (robf)
**              Rework With_hidden_columns to With_physical_columns
**	        for secure
**	03-feb-95 (wonst02) Bug #63766
**		Look for both $ingres and $INGRES, for fips
**	 8-dec-1995 (nick)
**		Convert catalog to uppercase in a FIPS installation. #73053
**	06-mar-96 (harpa06)
**		Changed UI_LEVEL_66 to UI_LEVEL_66 for VPS project.
**	29-jan-1997 (hanch04)
**		Changed UI_LEVEL_66 to UI_LEVEL_65.
**	27-July-1998 Alan.J.Thackray (thaal01)
**		Bug 87810.Unloaddb does not unload procedures when
**		certain collation sequences are used. Events, and
**		comments are also not unloaded. Convert '$ingres' to 
**		uppercase for FIPS, lower case for traditional Ingres.
**		Remove the OR statement which will result in incorrect
**		optimization with certain collation sequences are used.
**		The optimizer does not seem to know that the system tables
**		do not inherit the custom collation sequence. This is 
**		a cross integration of 433817 from oping12.
**	7-Jun-2004 (schka24)
**	    Check for release 3.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**	26-Aug-2009 (kschendel) 121804
**	    Need ui.h to satisfy gcc 4.3.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**	21-Apr-2010 (toumi01) SIR 122403
**	    Add With_r1000_catalogs for encryption support.
**/


/* # define's */
/* GLOBALREF's */
GLOBALREF bool With_distrib;
GLOBALREF bool With_log_key;
GLOBALREF bool With_multi_locations;
GLOBALREF bool With_comments ;
GLOBALREF bool With_UDT ;
GLOBALREF bool With_registrations ;
GLOBALREF bool With_sequences;
GLOBALREF bool With_dbevents ;
GLOBALREF bool With_procedures ;
GLOBALREF bool With_rules ;
GLOBALREF bool With_synonyms ;
GLOBALREF bool With_defaults ;
GLOBALREF bool With_sec_audit;
GLOBALREF bool With_physical_columns;
GLOBALREF bool With_security_alarms ;

GLOBALREF bool With_64_catalogs ;
GLOBALREF bool With_65_catalogs ;
GLOBALREF bool With_20_catalogs ;
GLOBALREF bool With_r3_catalogs ;
GLOBALREF bool With_r302_catalogs ;
GLOBALREF bool With_r930_catalogs ;


/* extern's */
/* These should be declared in ui.h! */
FUNC_EXTERN char	*IIUIscl_StdCatLevel(); 

/* static's */
static bool cat_exist();

/**
** Name:	xfcapset - Load info from the dbcapabilities.
**
** Description:
**	Get information about the characteristics of this database.
**	Yes, the IIUI stuff caches, making this unneccessary.  But it
**	is much more readable this way.  (Could also have used #defines.)
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		none
**
** History:
**	27-may-89 (marian)
**		Written.
**	06-mar-96 (harpa06)
**		Changed UI_LEVEL_65 to UI_LEVEL_66.
**	29-jan-1997 (hanch04)
**		Changed UI_LEVEL_66 to UI_LEVEL_65.
**	22-sep-1997 (nanpr01)
**		bug 85565 : unloaddb of 1.2 database over the net fails.
**	29-jan-03 (inkdo01)
**	    Add support for iisequences.
**	13-oct-05 (inkdo01)
**	    Add With_r302_catalogs.
**	17-nov-2008 (dougi)
**	    Add With_r930_catalogs.
**       6-Jan-2009 (hanal04) Bug 121468
**          ERx("string") will create a read only constant. In an SQL-92
**          installation we will try to convert lower case "string" to
**          upper case "STRING" and SEGV because the memory is read only.
**          Use locally defined variables as they will exist in writable
**          memory.
**/

void
xfcapset()
{
    static char    iiml[] = "iimulti_locations";
    static char    iidc[] = "iidb_comments";
    static char    iireg[] = "iiregistrations";
    static char    iiseq[] = "iisequences";
    static char    iiev[] = "iievents";
    static char    iiproc[] = "iiprocedures";
    static char    iipc[] = "iiphysical_columns";
    static char    iisa[] = "iisecurity_alarms";

    With_distrib = IIUIdcd_dist();
    With_log_key = IIUIdcl_logkeys();
    With_UDT = IIUIdcy_udt();
    With_rules = IIUIdcu_rules();

    With_multi_locations = cat_exist(iiml);
    With_comments = cat_exist(iidc);
    With_registrations = cat_exist(iireg);
    With_sequences = cat_exist(iiseq);
    With_dbevents = cat_exist(iiev);
    With_procedures = cat_exist(iiproc);
    With_physical_columns = cat_exist(iipc);
    With_security_alarms = cat_exist(iisa);

    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_64) >= 0)
    {
	With_64_catalogs = TRUE;
    }
    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_65) >= 0)
    {
	With_defaults = TRUE;
	With_synonyms = TRUE;
	With_65_catalogs = TRUE;
	With_sec_audit=TRUE;
    }
    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_800) >= 0)
    {
	With_20_catalogs = TRUE;
    }
    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_900) >= 0)
    {
	With_r3_catalogs = TRUE;
    }
    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_902) >= 0)
    {
	With_r302_catalogs = TRUE;
    }
    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_930) >= 0)
    {
	With_r930_catalogs = TRUE;
    }
    if (STcompare(IIUIscl_StdCatLevel(), UI_LEVEL_1000) >= 0)
    {
	With_r1000_catalogs = TRUE;
    }
}

/*{
** Name:	cat_exist - Called to see if a table exists.
**
** Description:
**	This routine looks to see if the given table exists.
**	xferdb uses this to check for the existence of catalogs.
**	This routine used to cache the answers, but no-one does multiple
**	calls anymore.
**
** Inputs:
**	name		name of table.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	exists
**		FALSE	nope
**
** History:
**	3-may-1990 (billc) written.
**
**	 8-dec-1995 (nick)
**	    Convert to uppercase in a FIPS installation. Note that we don't
**	    handle delimited ids or convert to lowercase as this function 
**	    is currently only ever called from xfcapset() above. #73053
**	27-July-1998 (thaal01) Bug 87810
**	    unloaddb does not work with certain collation sequences on 
**	    a non-SQL92 system
*/

static bool
cat_exist(name)
EXEC SQL BEGIN DECLARE SECTION;
    char    *name;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
	char	tname[DB_MAXNAME + 1];
	char	uname[DB_MAXNAME + 1];
    EXEC SQL END DECLARE SECTION;

    tname[0] = EOS;
    STcopy("$ingres", uname );

    if (IIUIdbdata()->name_Case == UI_UPPERCASE)
	{
	CVupper(name);
	CVupper(uname);
	}

    EXEC SQL REPEATED SELECT table_name
	INTO :tname
	FROM iitables
	WHERE table_name = :name
	AND table_owner = :uname ;

    return ((bool) (tname[0] != EOS));
}

