/*
** Copyright (c) 2004 Ingres Corporation
*/





/**
** Name: aitparse.h - aitparse.c shared variables and functions header file.
**
** Description:
**	AITparse declarations.
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Added tester control block.  Removed item local to aitparse.c
**	    or exported by other source files.
**	11-Sep-98 (rajus01)
**	    Added descriptorCount definition. Changed descritporList definition
**	    to be an array of pointers to struct _descriptorList. This
**	    was needed since IIapi_formatData() provides two descriptors
**	    and Dscrpt_Assgn_Parse() core dumped while parsing the 2nd
**	    descriptor.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





# ifndef __AITPARSE_H__
# define __AITPARSE_H__





/*
** Global variables.
*/
GLOBALREF CALLTRACKLIST			*callTrackList; 
GLOBALREF CALLTRACKLIST			*curCallPtr;
GLOBALREF i4      		  	cur_line_num;
GLOBALREF struct _varRec          	curVarAsgn[MAX_FIELD_NUM];
GLOBALREF struct  _dataValList    	*dataValList;
GLOBALREF struct  _descriptorList 	*descriptorList[MAX_DESCR_LIST];
GLOBALREF i4				descriptorCount;





/*
** External Functions
*/

FUNC_EXTERN VOID	AITparse( AITCB *aitcb );





# endif			/* __AITPARSE_H__ */
