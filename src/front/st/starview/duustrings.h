/*
** Copyright (c) 2004 Ingres Corporation
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
**/

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
#define		DUVE_0BANNER	    ":------------ VERIFYDB Sys Catalog Check: "
#define		DUVE_1BANNER	    " --------------:"

/* 
**  verifydb test numbers 
*/
#define		DUVE_1		    "1"
#define		DUVE_2		    "2"
#define		DUVE_3		    "3"
#define		DUVE_4		    "4"
#define		DUVE_5		    "5"
#define		DUVE_6		    "6"
#define		DUVE_7		    "7"
#define		DUVE_8		    "8"
#define		DUVE_9		    "9"
#define		DUVE_10		    "10"
#define		DUVE_11		    "11"
#define		DUVE_12		    "12"
#define		DUVE_13		    "13"
#define		DUVE_14		    "14"
#define		DUVE_15		    "15"
#define		DUVE_16		    "16"
#define		DUVE_17		    "17"
#define		DUVE_18		    "18"
#define		DUVE_19		    "19"
#define		DUVE_20		    "20"
#define		DUVE_21		    "21"
#define		DUVE_22		    "22"
#define		DUVE_23		    "23"
#define		DUVE_24		    "24"
#define		DUVE_25		    "25"
#define		DUVE_26		    "26"
#define		DUVE_27		    "27"
#define		DUVE_28		    "28"
#define		DUVE_29		    "29"
#define		DUVE_30		    "30"
#define		DUVE_31		    "31"
#define		DUVE_32		    "32"
#define		DUVE_33		    "33"
#define		DUVE_34		    "34"
#define		DUVE_35		    "35"
#define		DUVE_36		    "36"
#define		DUVE_37		    "37"
#define		DUVE_38		    "38"
#define		DUVE_39		    "39"
#define		DUVE_40		    "40"
#define		DUVE_41		    "41"
#define		DUVE_42		    "42"
#define		DUVE_43		    "43"
#define		DUVE_44		    "44"
#define		DUVE_45		    "45"
#define		DUVE_47		    "47"
#define		DUVE_48		    "48"
#define		DUVE_49		    "49"
#define		DUVE_50		    "50"
#define		DUVE_50A	    "50a"
#define		DUVE_51		    "51"
#define		DUVE_52		    "52"
#define		DUVE_53		    "53"
#define		DUVE_54		    "54"
#define		DUVE_55		    "55"
#define		DUVE_56		    "56"
#define		DUVE_57		    "57"
#define		DUVE_58		    "58"
#define		DUVE_59		    "59"
#define		DUVE_60		    "60"
#define		DUVE_61		    "61"
#define		DUVE_62		    "62"
#define		DUVE_63		    "63"
#define		DUVE_64		    "64"
#define		DUVE_65		    "65"
#define		DUVE_66		    "66"
#define		DUVE_67		    "67"
#define		DUVE_68		    "68"
#define		DUVE_69		    "69"
#define		DUVE_70A	    "70a"
#define		DUVE_70B	    "70b"
#define		DUVE_71		    "71"
#define		DUVE_72		    "72"
#define		DUVE_73A	    "73a"
#define		DUVE_73B	    "73b"
#define		DUVE_74A	    "74a"
#define		DUVE_74B	    "74b"
#define		DUVE_75		    "75"
#define		DUVE_76		    "76"
#define		DUVE_77		    "77"
#define		DUVE_78		    "78"
#define		DUVE_79		    "79"
#define		DUVE_80		    "80"
#define		DUVE_81		    "81"
#define		DUVE_82		    "82"
#define		DUVE_83		    "83"
#define		DUVE_84		    "84"
#define		DUVE_85		    "85"
#define		DUVE_86		    "86"
#define		DUVE_87		    "87"
#define		DUVE_88		    "88"
#define		DUVE_89		    "89"
#define		DUVE_90		    "90"
#define		DUVE_91		    "91"

