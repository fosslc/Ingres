/*
**	Copyright (c) 2004 Ingres Corporation
*/
/**
** Name:	g4consts.h 
**
** Description:
**	Constants used by the EXEC 4GL interface
**
** History:
**      10/92 (Mike S) Initial version
**/

/* 
** Object access types 
**
** Negative means that a resulting NULL object is OK. -OA_ARRAY is nonsensical
** and should never occur.
*/
# define G4OA_OBJECT 1		/* Any non-NULL object is OK */
# define G4OA_ARRAY  2		/* object must be array */	
# define G4OA_ROW    3		/* object is row of array */
# define G4OA_INVALID 999	/* An invalid value for access */

/* Caller IDs */
# define G4INTERNAL_ID	0	/* internal call */
# define G4GATTR_ID	1	/* 3GL GET ATTRIBUTE call */
# define G4SATTR_ID	2	/* 3GL SET ATTRIBUTE call */
# define G4CLRARR_ID	3	/* 3GL CLEAR ARRAY call */
# define G4DELROW_ID	4	/* 3GL SETROW DELETED call */
# define G4GETROW_ID	5	/* 3GL GETROW call */
# define G4INSROW_ID	6	/* 3GL INSERTROW call */
# define G4REMROW_ID	7	/* 3GL REMOVEROW call */
# define G4SETROW_ID	8	/* 3GL SET ROW call */
# define G4CALLPROC_ID	9	/* 3GL CALLPROC call */
# define G4CALLFRAME_ID 10	/* 3GL CALLFRAME call */
# define G4GETCONST_ID	11	/* 3GL GET GLOBAL CONSTANT call */
# define G4GETVAR_ID	12	/* 3GL GET GLOBAL VARIABLE call */
# define G4SETVAR_ID	13	/* 3GL SET GLOBAL VARIABLE call */
# define G4DESCRIBE_ID	14	/* 3GL DESCRIBE call */
# define G4INQUIRE_ID	15	/* 3GL INQUIRE_4GL call */
# define G4SET_ID	16	/* 3GL SET_4GL call */
# define G4SEND_UEV_ID	17	/* 3GL SEND USEREVENT call */

/* INQUIRE_4GL requests */
# define G4IQerrtext	0	/* Get error text */
# define G4IQerrno	1	/* Get error number */

# define G4IQallrows	2	/* Get number of rows in array */	
# define G4IQlastrow	3	/* Get highest-numbered in array */	
# define G4IQfirstrow	4	/* Get lowest-numbered in array */	

# define G4IQisarray	5	/* Ask whether object is an array */
# define G4IQclassname	6	/* Get object's class name */

/* SET_4GL requests */
# define G4STmessages	0	/* Set whether to report errors */

/* Mask for 'which' parameter of INSERTROW */
# define G4IR_ROW	0x1	/* Row specified */
# define G4IR_STATE	0x2	/* State specified */

/* Types of call */
# define G4CT_PROC	0	/* CALLPROC */
# define G4CT_FRAME	1	/* CALLFRAME */

/* Types of globals */
# define G4GT_CONST	0	/* Global constant */
# define G4GT_VAR	1	/* Global variable */

/* Directions for using SQLDA */
# define G4SD_SET	0	/* SET ATTRIBUTE ... USING */
# define G4SD_GET	1	/* GET ATTRIBUTE ... USING */
