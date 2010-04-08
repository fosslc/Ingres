/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUUSTRINGS.H -	    String constants used in database utility error
**			    messages.
**
** Description:
**        This file contains string constants that are used in the database
**	utility error messages exclusively.  These have been put in an
**	individual header file to make internationalization easier.  Only
**	the error message text in erduf.txt and these constants will have to
**	be updated to port to a new language.
**
** History: 
**      23-Oct-86 (ericj)
**          Initial creation.
**      19-oct-88 (alexc)
**          Inserted definitions used in TITAN createdb and destroydb.
**		They are: DU_2INITIALIZE, DU_0DISTRIBUTED, DU_1COORDINATOR
**	26-Jun-89 (teg)
**	    Moved alot of inline text to become constants here.
**	25-may-90 (edwin)
**	    Moved in a couple of strings for CREATEDB.
**	12-jun-90 (teresa)
**	    added new verifydb strings.
**	19-nov-90 (teresa)
**	    added DUVE_16A  and DUVE_17A
**	16-jun-93 (anitap0
**	    Added #defines for IISCHEMA, IIKEY and IIDEFAULT.	
**	30-jul-93 (stephenb)
**	    Added strings for iigw06_relation and iigw06_attribute
**	18-sep-93 (teresa)
**	    Added #define for DUVE_IIDBMS_COMMENT
**	20-dec-93 (robf)
**          Added #define for DUVE_IIPROFILE
**	19-jan-94 (andre)
**	    added DUVE_IIXPRIV, DUVE_IIPRIV, DUVE_IIXEVENT, DUVE_IIXPROCEDURE,
**	    and DUVE_IIXPROTECT
**	11-jun-2004 (gupsh01)
**	    Loadmdb changes.
**	30-Dec-2004 (schka24)
**	    Remove iixprotect.
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/* 
**  string used by du_error to warn that ERLOOKUP was unable to print the
**  message or format a message that it could not format the specified error
**  message number.
*/
#define DUU_ERRLOOKUP_FAIL "Warning:  ERLOOKUP failure.  This utility will probably exit silently."

/*
**      String constants that are used exclusively in "createdb" and "destroydb" 
**	for distributed database messages.
*/
#define                 DU_0DISTRIBUTED	"Distributed"
#define			DU_1COORDINATOR "Coordinator"


#define			DU_0INITIALIZING "Initializing"
#define			DU_1INITIALIZATION "Initialization"
#define			DU_2INITIALIZE	"Initialize"

#define			DU_0MODIFYING	"Modifying"
#define			DU_1MODIFICATION "Modification"

/*
**      String constants that are used exclusively in "createdb" messages.
*/
#define                 DU_0CREATING	"Creating"
#define			DU_1CREATION	"Creation"

/*
**      String constants that are used in "destroydb" error messages.
*/
#define                 DU_0DESTROYING  "Destroying"
#define			DU_1DESTRUCTION	"Destruction"

/*
**      String constants that are used in "sysmod" error messages.
*/
#define                 DU_0SYSMOD	"Sysmod"
#define                 DU_1SYSMODING	"Sysmoding"

/*
**      String constants that are used in "loadmdb" error messages.
*/
#define                 DU_1LOADMDB	"Loading"


/*
**      String constants that are used in "finddbs" messages.
*/
#define                 DUF_0FINDDBS	    "Finddbs"
#define			DUF_REPLACE_MODE    "REPLACE"
#define			DUF_ANALYZE_MODE    "ANALYZE"

/*
**      String constants that are used in "StarView" messages.
*/
#define			DUT_0CDB_SHORT	    "CDB"
#define			DUT_1CDB_LONG	    "Coordinator Database"
#define			DUT_2DDB_SHORT	    "DDB"
#define			DUT_3DDB_LONG	    "Distributed Database"
#define			DUT_4LDB_SHORT	    "LDB"
#define			DUT_5LDB_LONG	    "Local Database"
#define			DUT_6IIDBDB	    "iidbdb"

/*
**	Strings used for prompts.
*/
#define		DU_YES		    "yes"
#define		DU_NO		    "no"
#define		DU_Y		    'y'	    /* abbreviation for yes */
#define		DU_N		    'n'	    /* abbreviation for no */
#define		DU_DSTRYMSG	    "Intent is to destroy database, "
#define		DU_ANSWER_PROMPT    " (y/n): "

/*
The following banner constants are used to comprise this banner:

:---------------- VERIFYDB: catalog xxxxxxxxxxxx -- check #NN ----------------:
*/
#define		DUVE_0BANNER	    ":-------------- VERIFYDB: catalog "
#define		DUVE_1BANNER	    " -- check # "
#define		DUVE_2BANNER	    " -------------:"


/* 
**  DBMS Catalog Names (used by verifydb)
*/
#define		DUVE_IIATTRIBUTE	"IIATTRIBUTE"
#define		DUVE_IICDBID_IDX	"IICDBID_IDX"
#define		DUVE_IIDATABASE		"IIDATABASE"
#define		DUVE_IIDBDEPENDS	"IIDBDEPENDS"
#define		DUVE_IIDBID_IDX		"IIDBID_IDX"
#define		DUVE_IIDBMS_COMMENT	"IIDBMS_COMMENT"
#define		DUVE_IIDBPRIV		"IIDBPRIV"
#define		DUVE_IIDEFAULT		"IIDEFAULT"
#define		DUVE_IIDEVICES		"IIDEVICES"
#define		DUVE_IIEVENT		"IIEVENT"
#define		DUVE_IIEXTEND		"IIEXTEND"
#define		DUVE_IIGW06_RELATION	"IIGW06_RELATION"
#define		DUVE_IIGW06_ATTRIBUTE	"IIGW06_ATTRIBUTE"
#define		DUVE_IIINDEX		"IIINDEX"
#define		DUVE_IIINTEGRITY	"IIINTEGRITY"
#define		DUVE_IIHISTOGRAM	"IIHISTOGRAM"
#define		DUVE_IIKEY		"IIKEY"
#define		DUVE_IILOCATIONS	"IILOCATIONS"
#define		DUVE_IIPROCEDURE	"IIPROCEDURE"
#define		DUVE_IIPROFILE		"IIPROFILE"
#define		DUVE_IIPROTECT		"IIPROTECT"
#define		DUVE_IIQRYTEXT		"IIQRYTEXT"
#define		DUVE_IIRELATION		"IIRELATION"
#define		DUVE_IIREL_IDX		"IIREL_IDX"
#define		DUVE_IIRULE		"IIRULE"
#define		DUVE_IISCHEMA		"IISCHEMA"
#define		DUVE_IISECALARM		"IISECALARM"
#define		DUVE_IISTAR_CDBS	"IISTAR_CDBS"
#define		DUVE_IISTATISTICS	"IISTATISTICS"
#define		DUVE_IISYNONYM		"IISYNONYM"
#define		DUVE_IITREE		"IITREE"
#define		DUVE_IIUSER		"IIUSER"
#define		DUVE_IIUSERGROUP	"IIUSERGROUP"
#define		DUVE_IIXDBDEPENDS	"IIXDBDEPENDS"
#define		DUVE_IIXPRIV		"IIXPRIV"
#define		DUVE_IIPRIV		"IIPRIV"
#define		DUVE_IIXEVENT		"IIXEVENT"
#define		DUVE_IIXPROCEDURE	"IIXPROCEDURE"


/* rms gateway strings */
#define			DU_0RMS_GATEWAY	"RMS Gateway"
#define			DU_0RMS		"RMS"
#define			DU_0GMSG	"duc_gmsg_creating"
#define			DU_0GINIT	"duc_ginit_dbname"
#define			DU_0GSET	"duc_gset_service"
#define			DU_0GMAKE	"duc_gmake_gdb"
#define	    DU_0INT_ERR	    "Internal CREATEDB error detected in routine %s.\n"
/*
[@group_of_defined_constants@]...
*/

/*
[@global_variable_references@]
[@type_definitions@]
*/
