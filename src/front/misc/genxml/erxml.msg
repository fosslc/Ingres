/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erxml.msg -	Message file for class  XM 
**
** Description:
**	This file defines the messages for class XM.
**
** History:
**	07-june-20001 (gupsh01) 
**		Created.
**	12-jun-2001 (somsa01)
**	    Changed XM_CLASS define to match that of erfac.h .
**      05-sep-2001 (gupsh01)
**	    defined F_XM0006_IMPXML
**	09-oct-2003 (schka70/abbjo03)
**	    Add XM0007 and XM0008 for the VMS implementation of xmlimport.
**	04-Jun-2005 (thaju02)
**	    Add XM0009 and XM000A.
**/

# define	_XM_CLASS	238
/*
**	xmlmain.c
*/
F_XM0001_GENXML
"Genxml"

E_XM0002_Incorrect_language_sp
"Incorrect language specification, using default"

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

E_XM0003_Not_Ingres
"This DBMS does not have the capability to run
genxml.  You must be running against an INGRES dbms to use these products."

E_XM0004_Too_many_objects
"Too many objects specified, will only copy the first %0d"

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

E_XM0005_Connect_Failed
"Failed to connect to the INGRES/Star Coordinator Database."

/*%
** Explanation:
**    Genxml is being run on a distributed database.  To do this,
**    genxml must connect to the coordinator database.  For some
**    reason, this connection failed.
**
** System Status:
**    Program terminates.
**
** User Action:
**    Check for proper configuration of INGRES/Star and INGRES/Net.
*/

/* 
**	xmlimport.c
*/

F_XM0006_IMPXML
"Impxml"

E_XM0007_Locname_Failed
"Could not make a temporary directory name from %0c and %1c"

/*%
** Explanation:
**    xmlimport uses a temporary subdirectory for the generated
**    import SQL.  The generation of this subdirectory name failed.
**
** System Status:
**    Program terminates.
**
** User Action:
**    Make sure that II_TEMPORARY names a valid and writeable
**    temporary directory.
*/

E_XM0008_Create_Temp_Dir
"Could not create the temporary directory %0c"

/*%
** Explanation:
**    xmlimport uses a temporary subdirectory for the generated
**    import SQL.  The creation of this subdirectory failed.
**
** System Status:
**    Program terminates.
**
** User Action:
**    Make sure that II_TEMPORARY names a valid and writeable
**    temporary directory.
*/

E_XM0009_Cannot_Open_File
"Could not open xml file %0c"

E_XM000A_Cannot_Read_File
"Could not read xml file %0c"
