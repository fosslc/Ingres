/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erud.msg -	Message file for class UD
**
** Description:
**	This file defines the messages for class UD.
**
** History:
**	12-aug-1987 (rdesmond)
**		Created.
**	16-apr-1990 (marian)
**		Integrated changes from ICL.  Added S_UD000F_Unloaddb_include
**		to allow portable retrieval of file name.
**	29-jul-1991 (billc)
**		Fix bug 38881.  Old scripts could include the wrong .scr
**		file, so new functionality must go into new file.
**	07-jul-1995 (harpa06) Bug #69236
**		Modified S_UD000B_Don_t_forget_to_run_u error message.
**	19-jul-1995 (emmag)
**		Unload and reload scripts have a .bat extension on NT.
**/

# define	_UD_CLASS	89
/*
**	../unloaddb/udmain.qc
*/
F_UD0001_UNLOADDB
"UNLOADDB"

E_UD0001_Incorrect_language_sp
"Incorrect language specification."

/*%
** Explanation:
**	The user specified the DDL/DML language incorrectly.
**
** System Status:
**	The default language for the installation is used.
**
** User Action:
**	Check the language specifications and rerun if necessary.
*/

E_UD0002_You_are_not_the_dba	"You (%0c) are not the dba for this database.\n
Only the dba is allowed to run UNLOADDB."

/*%
** Explanation:
**	A user other than the dba attempted to run Unloaddb.
**
** System Status:
**	program terminates.
**
** User Action:
**	Only the dba may run Unloaddb.
*/

E_UD0007_Cannot_start_name_gen	
"Cannot start name generator."

/*%
** Explanation:
**	The file name generator cannot be initialized.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Rerun, then contact Ingres Technical Support.
*/

S_UD0008_Completed_UNLOADDB_an
"Completed UNLOADDB and built execs named UNLOAD and RELOAD."

S_UD0009_Don_t_forget_to_run_U
"Don't forget to run UNLOAD before destroying the database."

S_UD000A_Completed_unload_ing_
"\nUNLOADDB has created the scripts 'unload.ing' and 'reload.ing'."

S_UD000B_Don_t_forget_to_run_u
"\nDon't forget to run unload.ing before destroying the database
\nand to run 'CKPDB +j %0c' to enable journaling of the system catalogs.\n"

S_UD0016_Completed_unload_bat_
"\nUNLOADDB has created the scripts 'unload.bat' and 'reload.bat'."

S_UD0017_Don_t_forget_to_run_b
"\nDon't forget to run unload.bat before destroying the database
\nand to run 'CKPDB +j %0c' to enable journaling of the system catalogs.\n"

/*%
** Explanation:
**	The reload script has been aborted because there are already
**	frontend objects in the database.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Reloading data should only be run on an empty database.
*/

E_UD000C_abort_reload_script
"There are existing frontend objects in this database.  
You cannot use the reload scripts on a database that 
currently contains frontend objects."

E_UD000D_Not_Ingres
"This DBMS does not have the capability to run
unloaddb.  You must be running against an INGRES dbms to use these products."

/*%
** Explanation:
**	The reload scripts cannot reload frontend catalogs unless the user 
**  has permission to delete frontend catalogs.  Usually, only the 
**  user "$ingres" can do this.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Run SQL with the flags +U and -u'$ingres'.  If you do not have 
**  permission to run with these flags, contact the database administrator.
*/

E_UD0010_Wrong_User
"Frontend catalogs are owned by the user $ingres.
You must be the $ingres user in order to reload the catalogs."

/*%
** Explanation:
**	The reload scripts for frontend catalogs must create a temporary
**  file.  This file could not be created.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Ensure that you have correct permissions to create files in the
**  current directory.
*/

E_UD0011_Copy_Failed
"The reload scripts were unable to create the temporary file 
'ii_copy.tmp' in this directory.  Please check that you have the proper 
permissions to write a file in this directory."

S_UD000F_Unloaddb_include
"iiud.scr"

S_UD0012_Unloaddb64_include
"iiud64.scr"

E_UD0015_Flag_Ignored
"The language flag, -l,  is now ignored."

/*%
** Explanation:
**    Copydb no longer uses the -l flag.  This should have no effect on
**    the user.  All database objects may now be recreated regardless
**    of the language used.
**
** System Status:
**    Warning only, the program continues.
**
** User Action:
**    None needed.
*/

