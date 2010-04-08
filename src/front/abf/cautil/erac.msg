/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erac.msg	- Message for for CAUTIL directory.
**
** History:
**	11-aug-1987 (Joe)
**		Created.
**/

# define	_AC_CLASS	183
/*
**	cvtappl.c
*/
/*
**	cvtform.c
*/
F_AC0001_FORM		"FORM: "
F_AC0002_JOINDEF	", JOINDEF: "
F_AC0003_TABLE		", TABLE: "
/*
** The above 3 strings are used to produce a default short remark for
** a QBFNAME.  In English, the string produced is FORM: formname
** and then depending on the type of QBFNAME either , JOINDEF: or , TABLE:
**
** e.g.
**	FORM: employeeform, JOINDEF: employeejdef
*/
/*
**	cvtgbf.c
*/
/*
**	cvtgbf.c
*/
/*
**	cvtrpt.c
*/
/*
**	rtof.c
*/

/*
** Errors
*/
E_AC0001_INTFILE "Error in intermediate file.\n
During a conversion from a pre 6.0 copyapp file to a 6.0 copyapp file
an error was found in the pre 6.0 file.  Copyapp was expecting to
find a line beginning with `%0c'."
/*%
** Explanation:
**	Copyapp was converting a pre 6.0 copyapp file to a 6.0 copyapp
**	file.  It found a line with an error in it.
** System Status:
**	Copyapp will terminate with a fatal status.
*/


E_AC0002_CVTINFILE	"Could not open file `%0c' for conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not open the pre 6.0 file `%0c' for reading.  Make sure
the file exists, and that you have permission to read it."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not open the file.
** System Status:
**	Copyapp will terminate.
** User Action:
**	Make sure the file exists and that you have permissions to
**	read the file.
*/

E_AC0003_CVTOUTFILE	"Could not open file `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not create the temporary file `%0c' for writing.  Make sure
you have permissions to create files in the current directory."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not create a temporary file to put the 6.0 format
**	into.
** System Status:
**	Copyapp will terminate.
** User Action:
**	Make sure you have permissions to create files in the current
**	directory.
*/

E_AC0004_CREATETEMP	"Could not create the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not create the temporary table `%0c' needed to convert joindefs.
Make sure you have permissions to create tables in the database, and
that there is sufficient space to create a table.  An earlier INGRES
error should have been issued explaining why the table could not be
created."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not create a temporary table needed to convert joindefs.
** System Status:
**	Copyapp will terminate.
** User Action:
**	Make sure you have permissions to create tables in the current
**	database, and that there is enough space to create a table.
**	An earlier INGRES error should explain why the table could not 
**	be created.
*/

E_AC0005_APPENDTEMP	"Could not append to the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not append to the temporary table `%0c' needed to convert joindefs.
An earlier INGRES error should have been issued explaining why the append
could not be done to the table."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not append to a temporary table needed to convert joindefs.
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the table could not
**	be created.
*/

E_AC0007_RETTEMP	"Could not retrieve from the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not retrieve from the temporary table `%0c' needed to convert joindefs.
This table was successfully created and data was appended to it.
An earlier INGRES error should have been issued explaining why the table
could not be retrieved from."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not create a temporary table needed to convert joindefs.
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the table could not
**	be retrieved from.  Please report this error to your INGRES technical
**	representative.
*/


E_AC0008_DESTROYTEMP	"Could not destroy the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not destroy the temporary table `%0c' needed to convert joindefs.
An earlier INGRES error should have been issued explaining why the
table could not be created.  Copyapp will continue to run since this
error does not affect the conversion.  You may wish to destroy the table
after copyapp is finished."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not destroy a temporary table needed to convert joindefs.
** System Status:
**	Copyapp will continue running.
** User Action:
**	An earlier INGRES error should explain why the table could not
**	be created.  The table can be destroyed after copyapp is finished.
*/

E_AC0009_PrepareCreate	"Could not create the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not PREPARE the SQL CREATE statement for 
the temporary table `%0c' needed to convert joindefs.  An earlier INGRES
error should have been issued explaining why the CREATE statement could
not be prepared."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not PREPARE the CREATE statement for a temporary table
**	needed to convert joindefs.
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the statement could not
**	be prepared.  Please report this error to your INGRES technical
**	representative.
*/

E_AC000A_PrepareDrop	"Table `%0c' will not be destroyed after conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not PREPEARE the SQL DROP statement for
the temporary table `%0c' needed to convert joindefs.
An earlier INGRES error should have been issued explaining why the
table could not be prepared.  Copyapp will continue to run since this
error does not affect the conversion.  You may wish to destroy the table
after copyapp is finished."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not destroy a temporary table needed to convert joindefs.
** System Status:
**	Copyapp will continue running.
** User Action:
**	An earlier INGRES error should explain why the table could not
**	be created.  The table can be destroyed after copyapp is finished.
*/

E_AC000B_PrepareInsert	"Could not append to the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not PREPARE the SQL INSERT statement for 
the temporary table `%0c' needed to convert joindefs.
An earlier INGRES error should have been issued explaining why the
INSERT could not be prepared."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not prepare the SQL INSERT statement required to 
**	perform the INSERT.
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the PREPARE failed.
**	Please report this error to your INGRES technical representative.
*/

E_AC000C_PrepareSelect	"Could not retrieve from the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not PREPARE the SQL SELECT statement for
the temporary table `%0c' needed to convert joindefs.
This table was successfully created and data was appended to it.
An earlier INGRES error should have been issued explaining why the SQL SELECT
statement could not be prepared."
/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not prepare the SELECT statement for the temporary table. 
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the SELECT could not
**	be prepared.  Please report this error to your INGRES technical
**	representative.
*/

E_AC000D_DeclareSelect	"Could not retrieve from the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not successfully perform the SQL DECLARE statement
necessary for retrieval from the temporary
table `%0c' needed to convert joindefs.
This table was successfully created and data was appended to it.
An earlier INGRES error should have been issued explaining why the DECLARE 
statement failed."

/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not successfully execute a DECLARE statement for the SELECT
**	required to read the temporary file.
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the SELECT could not
**	be declared.  Please report this error to your INGRES technical
**	representative.
*/

E_AC000E_OpenSelect	"Could not retrieve from the table `%0c' during conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not successfully perform the SQL OPEN statement
necessary for retrieval from the temporary
table `%0c' needed to convert joindefs.
This table was successfully created and data was appended to it.
An earlier INGRES error should have been issued explaining why the OPEN
statement failed."

/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not open the cursor for the SELECT.
** System Status:
**	Copyapp will terminate.
** User Action:
**	An earlier INGRES error should explain why the cursor could not
**	be opened.  Please report this error to your INGRES technical
**	representative.
*/

E_AC000F_CloseSelect	"Could not close cursor for the table `%0c' after conversion.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
could not successfully close the cursor associated with
the table `%0c' needed to convert joindefs.
This table was successfully created, data was appended to it, and the
data was successfully retrieved.
An earlier INGRES error should have been issued explaining why the cursor
could not be closed."

/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	It could not create a temporary table needed to convert joindefs.
** System Status:
**	Copyapp will continue to run.
** User Action:
**	An earlier INGRES error should explain why the cursor could not
**	be closed.  Please report this error to your INGRES technical
**	representative.
*/

E_AC0010_TemptabExists	"A temporary table named `%0c' already exists.\n
Copyapp was trying to convert a pre 6.0 copyapp file to 6.0.  Copyapp
creates a temporary table used in the conversion of JoinDefs.  A temporary
table named `%0c' already exists.  Copyapp will generate a new table
name to use for its temporary table.  You may wish to destroy any
temporary tables in the database that are no longer required by copyapp."

/*%
** Explanation:
**	Copyapp was trying to convert a pre 6.0 file to 6.0 format.
**	The temporary table that copyapp tries to create already exists.
**	Copyapp will try a different name.
** System Status:
**	Copyapp will continue to run.
** User Action:
**	The user may wish to clean up his/her database.  There may be
**	temporary tables lying around that, for some reason, copyapp
**	did not previously destroy.  If these tables are old copyapp temporary
**	tables, the user should destroy them.
*/

