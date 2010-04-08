/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cm.h>
# include	<lo.h>
# include	<nm.h>
# include	<st.h>
# include	<er.h>
# include	<si.h>
# include	<fe.h>
# include	<ui.h>
# include	<ug.h>
# include       <adf.h>
# include       <afe.h>
# include	<xf.h>
# include	"erxf.h"

/**
** Name:	xfwrscrp.qsc - write DDL/DML scripts to copy database objects.
**
** Description:
**	This file contains the procedures to write QUEL/SQL scripts to
**	transfer the definition of database objects to intermediate files
**	and back to a database from those files.  
**	All of the logical and physical properties of those objects are 
**	transferred along with them, i.e. expiration date, location, 
**	modification of storage structure, indexes, modification of indexes, 
**	permits and integrities.
**
**	This file defines:
**
**	xfcrscript	create scripts to store and load db objects.
**	xf_found_msg	print message on how many objects were found.
**	xf_is_cat	is given object a catalog?
**	xf_is_fecat	is given object a frontend catalog?
**      xfescape_quotes escape quotes embedded in a string.
**
** History:
**	13-jul-87 (rdesmond) written.
**	16-aug-88 (marian)
**		Added support for stored procedures.
**	12-sep-1988 (marian)	bug 3393
**		Do a 'set autocommit on' at the start of the "copy in" script(s)
**		for 'sql' so the prompt when exiting the terminal when it is
**		not set will not be generated and the data is commited.  
**		Without this, nothing will be copied in.
**	20-sep-1988 (marian)
**		Took out #ifdef HACKFOR50 since it is no longer needed.
**	27-sep-1988 (marian)	bug 2192
**		If an inlist exists, than copydb is running with specific tables
**		specified.  Copydb should only unload those tables specified 
**		and should not unload the views, procedures, permits and 
**		integrities.
**	28-sep-1988 (marian)
**		Move bug fix 3393 to udmain.qsc and cdmain.c so the include
**		script 'iiud.scr' can come after the set autocommit statement.
**	28-sep-1988 (marian)	bug 2804
**		Give an error message when the number of tables specified
**		on the copydb commandline is not the same as that unloaded.
**		and before VIEWS.
**	11-oct-1988 (marian)
**		Changed the handling of procedures so they are handled the
**		same way as views, except it will only do one retrieve on
**		iiprocedures instead of multiple retrieves to unload the
**		procedures.
**	13-oct-1988 (marian)
**		Add support for REGISTRATIONS.  Registrations will be unloaded 
**		after TABLES.  Registered procedures will be unloaded with
**		regular procedures since there is currently no way to determine
**		which type of procedure it is.
**	14-nov-88 (marian)	bug 3898
**		Unload permits and integrities for tables specified on the
**		commandline for copydb.
**	21-apr-89 (marian)
**		Modified code to handle the STAR case.  If is_distrib is TRUE,
**		only unload registered objects and views.
**	03-may-89 (marian)
**		Changed the return status from FErelexists to check for OK.
**	16-may-89 (marian)
**		Add support for RULES.
**	27-may-89 (marian)
**		Change is_distrib to a global with_distrib, and add with_rules.
**	20-jul-89 (marian)	bug 6687
**		If inlist != NULL and objects listed are not all tables,
**		look for views with the names specified.  Don't return
**		after looking at just tables if not all objects were
**		found, check views, then return.
**	20-jul-89 (marian)	bug 6715
**		Pass "V" instead of "T" to xfpermit for views.  This will
**		allow permits to be created correctly for views and tables.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	29-mar-1990 (marian)	bug 20967
**		Added a portable parameter to the call to xfnumrel() and 
**		xffillrel() to determine if "ii_encoded_forms" should be
**		unloaded.
**	04-may-90 (billc)
**		major rewrite to support conversion to SQL, upgrade to 6.3 BE,
**		optimizations, etc.
**	22-jul-91 (billc)
**		do events BEFORE procedures, since procedures many reference 
**		events.
**	03-feb-92 (billc) 	bug 42166
**		Wasn't including .scr macro scripts in COPYDB-generated TM
**		script for $ingres.  Move code here, since shared by UD/CD.
**		(Also, put UD/CD version in a comment at head of TM script.)
**	30-feb-92 (billc) 	
**		major rewrite for FIPS in 6.5.
**	9-nov-1993 (jpk)
**		add "set nojournaling" after "set autocommit on" to copy.in
**		as per rogerk.
**	03-jan-1995 (wolf) 
**		Change GLOBALDEF of With_security_alarms to GLOBALREF;
**		it's already defined in chkcap.sc
**	03-feb-1995 (wonst02)	#63766
**		Change STequal to STalfeq when comparing with "$ingres"
**		and allow II as well as ii for catalogs
**	20-dec-1996 (chech02)
**		Changed GLOBALDEF to GLOBALREF for Owner and Portable.
**      29-apr-1999 (hanch04)
**	    replace STalfeq with STbcompare
**      26-Apr-1999 (carsu07)
**         Copydb/Unloaddb generates a copy.in script which lists the
**         permission statements before th registered tables.  The copy.in
**         script will fail if any permissions have been set on
**         registered tables because it will try to grant the permission
**         before registering the table. (Bug 94843)
**  24-nov-1999 (ashco01)
**		Removed call to xfcomments() as this is now called from xftables()
**      on a table by table basis.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-may-2001 (somsa01)
**	    Corrected cross-integration for bug 101172.
**	02-nov-2001 (somsa01)
**	    Changed S_XF0009_comment_header to S_XF0021_comment_header.
**      04-jan-2002 (stial01)
**          Implement -add_drop for -with_views, -with_proc, -with_rules
**      20-jun-2002 (stial01)
**          Unloaddb changes for 6.4 databases (b108090)
**      29-Apr-2003 (hweho01)
**          Cross-integrated change 455470 by huazh01:
**          27-Aug-2001 (huazh01)
**          Modify fucntion xfcrscript. If it is a STAR database,
**          then do not unload the table.
**          This fixes bug 105610.
**      14-May-2003 (hweho01)
**          Check the output_flags before calling xfpermits().
**          Added output_flags checking for unloading the registrations.   
**	29-Apr-2004 (gupsh01)
**	    Added support for convtouni flag.
**	31-03-2005 (raodi01) bug 114150
**	    When a table is passed to copydb along with database name
**	    if sequences are present in the database they are also being
**	    copied. 
**	03-May-2005 (komve01)
**	    Added no_seq flag for copydb.
**	04-May-2005 (komve01)
**	    Check for XF_NO_SEQUENCES flag had to be changed for unloaddb to
**      properly identify the flag.
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**       8-Jun-2009 (maspa05) trac 386, SIR 122202
**          Added -nologging option to add "set nologging" to copy.in
**          in the case of unloaddb "set session authorization <dbaname>" is
**          also added
**          
*/

/* # define's */
/* GLOBALDEF's */

/* extern's */
GLOBALREF bool Portable;
GLOBALREF char *Owner;
GLOBALREF bool With_distrib;	/* determine if star */
GLOBALREF bool With_dbevents;	/* determine if dbevents supported */
GLOBALREF bool With_sequences;	/* determine if sequences supported */
GLOBALREF bool With_procedures;	/* determine if procedures supported */
GLOBALREF bool With_registrations;	/* determine if registrations */
GLOBALREF bool With_comments;	/* determine if dbms comments supported. */
GLOBALREF bool With_rules;	/* determine if dbms rules supported. */
GLOBALREF bool With_synonyms;	/* determine if synonyms supported. */
GLOBALREF bool With_security_alarms; 
GLOBALREF bool Unload_byuser;
GLOBALREF bool identity_columns; /* determine if there are identity sequences */
GLOBALREF char  Version[];	/* Ingres version, change this when unbundle. */

GLOBALREF i4	Objcount;	/* count of objects requested in COPYDB */
GLOBALREF bool  db_replicated;

FUNC_EXTERN char	*IIUGdmlStr();

/* static's */

/*{
** Name:	xfcrscript - write QUEL/SQL statements into named file to 
**				transfer database objects
**
** Description:
**	This is the driver to write the entire contents of two DDL/DML
**	scripts for one database.  One script copies the objects from
**	the database to intermediate files, the other from intermediate files
**	to a database.  Front End system catalogs may also be copied.  
**
**	A list of tables and views
**	may be specified, in which case only those objects will be copied.  If 
**	an object list is NOT specified and an owner IS specified then all 
**	of the objects owned by the user will be copied.  
**	If owner is NOT specified, we copy everything we can find.
**
**	The DDL/DML statements may be written in either SQL or QUEL and may 
**	also be written in portable format, allowing the transfer of 
**	databases across machines implementing different representations of 
**	binary data in files.
**	
**	Prior to calling this procedure the database must already have been 
**	opened (with the user being either the dba or the one whose script 
**	to create).  The script files must also have been opened prior to
**	calling this procedure, and should be closed after return.
**
** Inputs:
**	owner		owner of objects for whom to create scripts.
**			NULL means the whole database.
**	progname	name of program that calls us, for messages.
**	portable	TRUE - statements are to be written in portable format.
**			FALSE - statements are to be written in binary format.
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**	13-jul-1987 (rdesmond)
**		written.
**	10-nov-1987 (rdesmond)
**		added 'language', 'alltoall' and 'rettoall' parms to calls to
**		xfpermit().
**	16-aug-88 (marian)
**		Added support for stored procedures.
**	12-sep-1988 (marian)	bug 3393
**		Do a 'set autocommit on' at the start of the "copy in" script(s)
**		for 'sql' so the prompt when exiting the terminal when it is
**		not set will not be generated and the data is commited.  Without
**		this, nothing will be copied in.
**	20-sep-1988 (marian)
**		Took out #ifdef HACKFOR50 since it is no longer needed.
**	27-sep-1988 (marian)	bug 2192
**		If an inlist exists, than copydb is running with specific tables
**		specified.  Copydb should only unload those tables specified 
**		and should not unload the views, procedures, permits and 
**		integrities.
**	28-sep-1988 (marian)
**		Move bug fix 3393 to udmain.qsc and cdmain.c so the include
**		script 'iiud.scr' can come after the set autocommit statement.
**	28-sep-1988 (marian)	bug 2804
**		Give an error message when the number of tables specified
**		on the copydb commandline is not the same as that unloaded.
**		and before VIEWS..
**	13-oct-1988 (marian)
**		Add support for REGISTRATIONS.  Registrations will be unloaded 
**		after TABLES.
**	9-nov-1993 (jpk)
**		add "set nojournaling" after "set autocommit on" as per rogerk.
**	18-nov-93 (robf)
**              Add SECURITY_ALARMS after rules, since alarms could call
**		dbevents which have to be created first
**	22-nov-1993 (jpk)
**		fix bug: include scripts left tm in QUEL.
**	29-apr-94 (robf)
**              Turn on privileges before processing data, and
**	        make sure default row visibility is respected.
**  11-May-2000 (fanch01)
**     Bug 101172: PERMISSIONS and CONSTRAINTS were reversed in copy.in
**       generated by unloaddb and copydb.  Moved code from xfcrscript()
**       to xftables() in xfcopy.c to correct ordering.
**	16-nov-2000 (gupsh01)
**		modified the input parameter list to include the 
**		parameter output_flags. This will facilitate the 
**		user requested output flags eg createonly, modifyonly etc.
**	06-june-2000 (gupsh01)
**		Removed the xfpermits() call to xftables. A previous change to
**	        move xfpermits did not remove xfpermits() call from this file.
**      27-Aug-2001 (huazh01)
**              Modify fucntion xfcrscript. If it is a STAR database,
**              then do not unload the table.
**              This fixes bug 105610.
**	23-Oct-2002 (gupsh01)
**		A previous cross integration brought back the xfpermits call
**		in this file causing xfpermits to be executed twice.
**		remove call to xfpermits from this routine.
**	29-jan-03 (inkdo01)
**	    Add support for sequences.
**      31-Jul-2003 (hanal04) SIR 110651 INGDBA 236
**          Added new -journal option to allow fully journalled copy.in.
**	05-Feb-2004 (gupsh01)
**	    Refix bug #110082. Registrations are not being printed
**	    in copydb output.
**      13-July-2004 (zhahu02)
**          Updted for writing permits on sequences (INGSRV2893/b112604). 
**	9-Sep-2004 (schka24)
**	    Need to redo dbevent permits if we're being run by upgradedb
**	    to redo permits.  (dbevent permits existed in 6.4, need reissued)
**	13-Sep-2004 (schka24)
**	    Somehow, constraints got ahead of permits again.  That won't
**	    work for cross user ref constraints.  Fix: always do constraints
**	    last.
**	30-Jan-2006 (gupsh01)
**	    Write sequence definitions (before tables, permits and view
**	    definitions, since tables and  views may reference sequences).
**      11-Apr-2008 (hanal04) SIR 120242
**          Add date format, decimal, money format and money precision
**          SET statements to copy.in and copy.out scripts if we are doing
**          an ascii copy.
**	16-nov-2008 (dougi)
**	    Added call to xfidcolumns() to support identity columns.
**       8-Jun-2009 (maspa05) trac 386, SIR 122202
**          added dbaname parameter so we can do "set session authorization
**          <dbaname>" for -nologging option for unloaddb
**      27-Jul-2009 (hanal04) Bug 122327
**          Star DBs do not have iirelation present. Do not call
**          xf_dbreplicated() if we are a Star DB, instead set db_replicated
**          to FALSE.
*/
void
xfcrscript(char *owner, char *progname, char *dbaname, bool portable, 
	    i4 output_flags, i4 output_flags2)
{
    XF_TABINFO	*tlist_ptr;
    auto i4	foundcount = 0;
    auto STATUS	stat;
    char	tbuf[256];
    ADF_CB *ladfcb;
    char   *cp;

    ladfcb = FEadfcb();

    /* We assume that the owner name has already been normalized. */
    Owner = (owner == NULL ? ERx("") : owner);
    Portable = portable;

    /* Establish whether we are replicated, set FALSE for STAR DBs */
    if(With_distrib)
        db_replicated = FALSE;
    else
        db_replicated = xf_dbreplicated();

    /*
    ** Write informational comment header.
    */
    (void) IIUGfmt(tbuf, sizeof(tbuf) - 1, ERget(S_XF0021_comment_header), 3,
	    IIUGdmlStr(DB_SQL), progname, Version
	);
    if (!(output_flags2 & XF_CONVTOUNI))
    {
      xfwrite(Xf_both, tbuf);

      /* Fix bug 39324: make sure we start in the right language. */
      xfsetlang(Xf_both, DB_SQL);

      /*
      ** bug 3393 and bug 4743
      **      Set autocommit on to improve performance.
      */
      xfwrite(Xf_both, ERx("set autocommit on"));
      xfwrite(Xf_both, GO_STMT);
    }
    else 
    {
      xfwrite(Xf_in, tbuf);

      /* Fix bug 39324: make sure we start in the right language. */
      xfsetlang(Xf_in, DB_SQL);

      /*
      ** bug 3393 and bug 4743
      **      Set autocommit on to improve performance.
      */
      xfwrite(Xf_in, ERx("set autocommit on"));
      xfwrite(Xf_in, GO_STMT);
    }

    /*
    ** set nojournaling when copying in.
    */
    if(SetJournaling == TRUE)
    {
	xfwrite(Xf_in, ERx("set journaling"));
    }
    else
    {
        xfwrite(Xf_in, ERx("set nojournaling"));
    }
    xfwrite(Xf_in, GO_STMT);

    if (!With_distrib)
    {
        if (!(output_flags2 & XF_CONVTOUNI))
        {
	  /* This improves performance. */
	  xfwrite(Xf_out, ERx("set lockmode session where readlock=nolock"));
	  xfwrite(Xf_out, GO_STMT);

	  /* Make sure all appropriate privileges are set, SQL only */
	  xfwrite(Xf_both, ERx("\\sql\n"));
	  /* If Unload_byuser, Don't issue set session with privileges=all */
	  if (!Unload_byuser)
	  {
	    xfwrite(Xf_both, ERx("set session with privileges=all"));
	    xfwrite(Xf_both, GO_STMT);
	  }
	}
	else 
	{
	  /* Make sure all appropriate privileges are set, SQL only */
	  xfwrite(Xf_in, ERx("\\sql\n"));
	  /* If Unload_byuser, Don't issue set session with privileges=all */
	  if (!Unload_byuser)
	  {
	    xfwrite(Xf_in, ERx("set session with privileges=all"));
	    xfwrite(Xf_in, GO_STMT);
	  }
	}
    }


    if(Portable)
    {
        /* set decimal II_DECIMAL */
        STprintf(tbuf, ERx("set decimal '%c'"), ladfcb->adf_decimal.db_decimal);
        xfwrite(Xf_both, tbuf);
        xfwrite(Xf_both, GO_STMT);
    
        /* set date_format II_DATE_FORMAT */
        STprintf(tbuf, ERx("set date_format '%s'"), 
                       adu_date_string(ladfcb->adf_dfmt));
        xfwrite(Xf_both, tbuf);
        xfwrite(Xf_both, GO_STMT);
    
        /* set money_format II_MONEY_FORMAT */
        NMgtAt(ERx("II_MONEY_FORMAT"), &cp);
        if ( cp != NULL && *cp != EOS )
        {
            STprintf(tbuf, ERx("set money_format '%s'"), cp);
        }
        else
        {
            /* II_MONEY_FORMAT not set, default to 'L:$' */
            STprintf(tbuf, ERx("set money_format 'L:$'"));
        }
        xfwrite(Xf_both, tbuf);
        xfwrite(Xf_both, GO_STMT);
    
        /* set money_prec II_MONEY_PREC */
        STprintf(tbuf, ERx("set money_prec '%d'"), ladfcb->adf_mfmt.db_mny_prec);
        xfwrite(Xf_both, tbuf);
        xfwrite(Xf_both, GO_STMT);
    }

    /*
    ** bug 42166: move this stuff from [uc]dmain.qsc since we always want to
    **	    include this stuff for user $ingres.
    **
    ** Only need to include iiud.scr if this is not STAR
    **
    ** bug us1020: Now that the DBA script runs FIRST, the check in
    **      iiud.scr would fail in all other scripts.  So put it in $ingres
    **      script ONLY.
    */
    if (!With_distrib && (Owner[0] == EOS ||
	(STbcompare(Owner, 0,  ERx("$ingres"), 0, TRUE) == 0 )))
    {
	auto LOCATION    inc_loc;
	auto char        *inc_str;

        /*
        ** get the location of the files directory to find
        ** the full path of the iiud.scr file so it can be included.
        */
	NMloc(FILES, FILENAME, ERget(S_XF0090_include_file), &inc_loc);
	LOtos(&inc_loc, &inc_str);
	STprintf(tbuf, ERx("\\include %s\n"), inc_str);
	xfwrite(Xf_in, tbuf);

	/*
	** Fix bug 38881.  Older scripts could include newer macro scripts,
	** so any new funtionality has to go into new files.
	*/
	NMloc(FILES, FILENAME, ERget(S_XF0092_65_include_file), &inc_loc);
	LOtos(&inc_loc, &inc_str);
	STprintf(tbuf, ERx("\\include %s\n"), inc_str);
	xfwrite(Xf_in, tbuf);

	/* Fix bug 56902, 54327: scripts leave tm in QUEL */
	xfwrite(Xf_in, ERx("\\sql\n"));
    }

    /*
    ** -nologging option
    */
    if (NoLogging == TRUE)
    {	
        if (Owner && Owner[0] == EOS)
        {
	/*
	** if no Owner is specified then we're called from unloaddb so
        ** need to add set session authorization
        */
	       STprintf(tbuf, ERx("set session authorization \"%s\""),dbaname);
	       xfwrite(Xf_in, tbuf);
      	       xfwrite(Xf_in, GO_STMT);
        }


    	/* add set nologging wrapped in \nocontinue, \continue */

    	xfwrite(Xf_in, ERx("\\nocontinue\n"));
    	xfwrite(Xf_in, ERx("set nologging"));
    	xfwrite(Xf_in, GO_STMT);
    	xfwrite(Xf_in, ERx("\\continue\n"));

    }
    if ((output_flags & XF_SEQUENCE_ONLY) && (output_flags & XF_DROP_INCLUDE))
	xfdrop_sequences();

    if ((output_flags & XF_RULES_ONLY) && (output_flags & XF_DROP_INCLUDE))
	xfdrop_rules(output_flags);

    if ((output_flags & XF_PROCEDURE_ONLY) && (output_flags & XF_DROP_INCLUDE))
	xfdrop_procs();

    if ((output_flags & XF_VIEW_ONLY) && (output_flags & XF_DROP_INCLUDE))
	xfdrop_views();

    /*
    ** Write sequence definitions (before tables, permits and view definitions, 
    ** since tables and  views may reference sequences).
    */
    if ((!output_flags || 
		(((output_flags & XF_SEQUENCE_ONLY ) || 
				(output_flags & XF_PRINT_TOTAL)))) &&
		!(output_flags2 & XF_NO_SEQUENCES))
        xfsequences();

    /* If this is a STAR database then do not unload tables, only unload
    ** iiregistrations, iiviews, iiprocedures, iipermits and iiintegrities.
    ** This fixes bug 105610
    */
    if ( ! With_distrib )
    {
      /* 
      ** Generate CREATE TABLE, COPY, permission, SAVE UNTIL(!), 
      ** CREATE INDEX, COMMENT ON and all other statements related to tables.
      ** Return table-list pointer for use with constraints, later.
      */
      xftables(&foundcount, output_flags, output_flags2, &tlist_ptr);
    }

    if ( identity_columns )
    {
	/*
	** Generate statements to save "next_value" for identity sequences
	** at time of copy.out to be restored during copy.in. */
	xfidcolumns(tlist_ptr);
    }

    /* Do REGISTRATIONS exist? */
    if (With_registrations && 
	  (!(output_flags) || 
	   (output_flags & (XF_REGISTRATION_ONLY | XF_PRINT_TOTAL))))
    {
        auto i4        regcount = 0;

        /* Unload the registrations */
        xfregistrations(ERx("T"), &regcount);
        foundcount += regcount;

        /* unload registered indexes, if this isn't STAR. */
        if (!With_distrib && regcount > 0)
            xfregistrations(ERx("I"), (i4 *) NULL);
    }

    /* Ok to grant permissions on base tables now. */
    if (!output_flags ||
        (((output_flags & XF_PERMITS_ONLY ) ||
                (output_flags & XF_PRINT_TOTAL)) &&
        !(output_flags & XF_NO_PERMITS)))
    xfpermits();


    /* If user specified an object list, and we've done them all,
    ** skip on down to constraints.  Don't do views, etc.
    */
    if (Objcount == 0 || foundcount < Objcount)
    {
	/*
	** Write user view definitions.  
	*/
	if (!output_flags || 
		(output_flags & XF_VIEW_ONLY ) || 
		(output_flags & XF_PRINT_TOTAL))
	    xfviews(&foundcount);

	if (Objcount != 0)
	{
	    /*
	    ** Produce an error message stating that not all 'objects' were
	    ** found if there were more objects listed on the
	    ** commandline than were found in the db.
	    */
	    if ((Objcount - foundcount) == 1)
		IIUGerr(E_XF0019_Cannot_find_table, UG_ERR_ERROR, 0);
	    else if ((Objcount - foundcount) != 0)
		IIUGerr(E_XF0020_Cannot_find_tables, UG_ERR_ERROR, 0);
	}
	else
	{
	    /*
	    ** When unloading entire (UNLOADDB) databases,
	    ** determine whether certain features exist and, if they do,
	    ** unload the appropriate objects.
	    */

	    if (With_synonyms &&
		(!output_flags || (output_flags & XF_SYNONYM_ONLY ) || 
		 (output_flags & XF_PRINT_TOTAL)))
		xfsynonyms();

	    /*
	    ** Bug 38759: handle events BEFORE procedures, since procedures
	    ** can reference events.
	    ** Also do events if just redoing permits for things.
	    */
	    if (With_dbevents &&
	      (!output_flags || (output_flags & XF_EVENT_ONLY ) || 
			(output_flags & XF_PERMITS_ONLY) ||
			(output_flags & XF_PRINT_TOTAL)))
		xfevents(output_flags, output_flags2);

	    if (With_procedures &&
		(!output_flags || (output_flags & XF_PROCEDURE_ONLY ) || 
			(output_flags & XF_PRINT_TOTAL)))
	    {
		xfprocedures(output_flags2);

		/* Yes, STAR might have registered procedures in 6.5. */
		if (With_registrations &&
		    (!output_flags || (output_flags & XF_REGISTRATION_ONLY ) ||
			(output_flags & XF_PRINT_TOTAL)))
		    xfregistrations(ERx("P"), (i4 *) NULL);
	    }

	    if (With_rules &&
		(!output_flags || (output_flags & XF_RULES_ONLY ) || 
			(output_flags & XF_PRINT_TOTAL)))
		xfrules();

	    if (With_security_alarms &&
		(!output_flags || (output_flags & XF_SECALARM_ONLY ) || 
			(output_flags & XF_PRINT_TOTAL)))
		xfsecalarms();
	}
    }

    /* Generate ALTER TABLE statements for constraints. */
    if (!output_flags || ((output_flags & XF_CONSTRAINTS_ONLY )
	    || (output_flags & XF_PRINT_TOTAL )
	    || (output_flags2 & XF_CONVTOUNI )) )
	xfconstraints(output_flags, tlist_ptr);

}

/*{
** Name:	xf_found_msg - display number of objects found for a given
**			object type.
**
** Description:
** 	This is a generic "n objects for user u" or "n objects in the 
**	database" message generator.  It is table-driven, using arrays of
**	the MSG_TMP struct.
** 	MSG_TMP (for message-template) has an entry for a single item and
** 	for zero or multiple items, so we can print "one zorf found for ..."
** 	or "42 zorfs found for...", as appropriate.
**
** Inputs:
**	otype	a character code indicating the object type.
**	count	the number of objects found.
**
** Outputs:
**	none.
**
**	Returns:
**		None.
**
** History:
**	6-jul-1992 (billc)
**		written.
*/

typedef struct m_tmps
{
	char		*m_type;	/* string identifying the obj. type */
	ER_MSGID	m_onefound;	/* msg for 'one zorf found for ...' */
	ER_MSGID	m_manyfound;	/* msg for 'n zorfs found for ...' */
} MSG_TMP;

/*
** We have two tables, one for UNLOADDB messages and one for COPYDB.
** COPYDB unloads objects owned by the user, so we mention the owner/user
** name.  UNLOADDB unloads everything, so we just give the object count.
**
** The 'type' (m_type) keys the lookup.  Note the sneaky trick: we use
** strings instead of chars so we can handle registered views, say, with
** the key "V*".
*/

MSG_TMP CD_msgs[] =
{
    { ERx("T"), S_XF0010_one_table_for_user, S_XF0011_n_tables_for_user },
    { ERx("V"), S_XF0012_one_view_for_user, S_XF0013_n_views_for_user },
    { ERx("P"), S_XF0017_one_proc_for_user, S_XF0018_n_procs_for_user },
    { ERx("I"), S_XF001E_one_index_for_user, S_XF001F_n_indexes_for_user },
    { ERx("R"), S_XF0027_one_rule_for_user, S_XF0028_n_rules_for_user },
    { ERx("C"), S_XF0040_one_comment_for_user, S_XF0041_n_comments_for_user },
    { ERx("E"), S_XF002A_one_event_for_user, S_XF002B_n_events_for_user },
    { ERx("Q"), S_XF002C_one_sequence_for_user, S_XF002D_n_sequences_for_user },
    { ERx("S"), S_XF012C_one_synonym_for_user, S_XF012D_n_synonyms_for_user },
    { ERx("T*"), S_XF001C_one_tbl_for_user, S_XF001D_n_tbls_for_user },
    { ERx("V*"), S_XF0022_one_view_for_user, S_XF0023_n_views_for_user },
    { ERx("I*"), S_XF001E_one_index_for_user, S_XF001F_n_indexes_for_user },
    { NULL, S_XF0010_one_table_for_user, S_XF0011_n_tables_for_user },
};

MSG_TMP UD_msgs[] =
{
    { ERx("T"), S_XF0110_one_table, S_XF0111_n_tables},
    { ERx("V"), S_XF0112_one_view, S_XF0113_n_views},
    { ERx("P"), S_XF0117_one_proc, S_XF0118_n_procs},
    { ERx("I"), S_XF011E_one_index, S_XF011F_n_indexes},
    { ERx("R"), S_XF0127_one_rule, S_XF0128_n_rules},
    { ERx("C"), S_XF0140_one_comment, S_XF0141_n_comments},
    { ERx("E"), S_XF012A_one_event, S_XF012B_n_events},
    { ERx("Q"), S_XF002E_one_sequence, S_XF002F_n_sequences},
    { ERx("S"), S_XF012E_one_synonym, S_XF012F_n_synonyms},
    { ERx("T*"), S_XF011C_one_tbl, S_XF011D_n_tbls},
    { ERx("V*"), S_XF0122_one_view, S_XF0123_n_views},
    { ERx("I*"), S_XF011E_one_index, S_XF011F_n_indexes},
    { NULL, S_XF0110_one_table, S_XF0111_n_tables},
};

void
xf_found_msg(char *otype, i4  count)
{
    auto i4	lcnt = count;
    register MSG_TMP	*mp;
    i4		argcnt;

    if (Owner && Owner[0] == EOS)
    {
	/*
	** We're called by UNLOADDB, no owner specified, so get the
	** appropriate message table.
	*/
	mp = UD_msgs;
	argcnt = 0;
    }
    else
    {
	/*
	** Called from COPYDB, which specifies an owner, so get the table 
	** of messages that mention the owner.
	*/
	mp = CD_msgs;
	argcnt = 1;
    }

    for ( ; mp->m_type != NULL; mp++)
    {
	if (STequal(mp->m_type, otype))
	    break;
    }

    if (count == 1)
        IIUGmsg(ERget(mp->m_onefound), FALSE, 0 + argcnt, (PTR)Owner);
    else
        IIUGmsg(ERget(mp->m_manyfound), FALSE, 1 + argcnt, &lcnt, (PTR)Owner);
}

/*{
** Name:	xf_is_cat - is the named table a catalog?
**
** Description:
** 	tells us whether the named table is a catalog.
**	This could just as well have been implemented as a macro, but
**	this isn't a performance bottleneck, so why bother?
**
** Inputs:
**	name	an object name
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE if the object fulfills our rules for catalog naming.
**		FALSE otherwise.
*/

bool
xf_is_cat(char *name)
{
    if (name != NULL && ((*name == 'i' && name[1] == 'i')
		      || (*name == 'I' && name[1] == 'I')))
	return (TRUE);
    return (FALSE);
}

/*{
** Name:	xf_is_fecat - is the named table a frontend catalog?
**
** Description:
** 	tells us whether the named table is a frontend catalog.
**
** Inputs:
**	name	an object name
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE if the object fulfills our rules for frontend catalog 
**			naming.
**		FALSE otherwise.
*/

bool
xf_is_fecat(char *name)
{
    if (name != NULL && ((*name == 'i' && name[1] == 'i' && name[2] == '_')
		      || (*name == 'I' && name[1] == 'I' && name[2] == '_')))
	return (TRUE);
    return (FALSE);
}

/*{
** Name:	xfescape_quotes - escape quotes embedded in string.
**
** Description:
**
** Inputs:
**	str	string to escape
**
** Outputs:
**	buf	buffer to write to.  Better be twice as big as str.
**
**	Returns:
**		none.
*/

void
xfescape_quotes(char *str, char *buf)
{
    for (; *str != EOS; CMnext(str), CMnext(buf))
    {
	if (!CMdbl1st(str) && *str == '\'')
	    *buf++ = '\'';
	CMcpychar(str, buf);
    }
    *buf = EOS;
}

