/*
** Copyright (c) 2004 Ingres Corporation
*/





/*
** Name: aitdef.h - APItester common header file
**
** Description:
**	This file contains definitions shared by most APItester modules.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	06-Mar-95 (feige)
**	    Correct typo in funTBL.
**	27-Mar-95 (gordy)
**	    Moved API function information to aitfunc.h
**	11-Sep-98 (rajus01)
**	    Added MAX_DESCR_LIST definition. IIapi_formatData uses
**	    2 descriptors (src, dst).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





# ifndef __AITDEF_H__
# define __AITDEF_H__





/*
** the maximum bytes read in per line
*/
# define	FILE_REC_LEN			132

/*
** the maximum length for variable name
*/
# define	MAX_VAR_LEN			33

/*
** maximum string length
*/
# define	MAX_STR_LEN			32000

/*
** define default output file as standard output
*/
# define	default_output			"stdout"

/*
** parser mode
*/

# define Var_Decl_Mode                   1
# define Var_Assgn_Mode                  2
# define Var_Reuse_Mode                  3
# define Fun_Call_Mode                   4






/*
** Name: CALLTRACKLIST - API function call information
**
** Description:
**	This data structure declares the function number which is index into
**	funTBL, the variable name of the parameter, the pointer pointing to
**	the parameter block buffer, and the pointer pointing to the next
**	node on the callTrackList.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Typedef'd this structure.  Added funInfo as quicker
**	    access than going through funNum.
*/

typedef struct _callTrackList
{	

    struct _callTrackList *next;
    i4			  funNum;	  /* the index to funTBL */
    API_FUNC_INFO	  *funInfo;	  /* entry in funTBL */
    char		  *parmBlockName; /* parmBlock variable name */ 
    II_PTR		  parmBlockPtr;   /* parmBlock buffer */
    bool		  repeated;	  /* repeated execution? */
    IIAPI_DESCRIPTOR	  *descriptor;	  /* IIapi_getColumns descr */

} CALLTRACKLIST;





/*
** Name: struct _varRec	- variable buffer data structure 
**
** Description:
**	This data structure declares the structure of any pair of variable name
**	at the left_hand of assignment and the value at the right_hand of
**	the assignment.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

struct  _varRec
{       char    *left_side;
        char    *right_side;
};





/*
** Name: struct _dataValList - datavalue buffer 
**
** Description:
**	This data structure defines a linked list used as the buffer
**	for an array of IIAPI_DATAVALUE values.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

struct _dataValList
{
        struct  _varRec		valBuf;
        struct  _dataValList    *next;
};





/*
** Name: struct _descriptorList - descriptor buffer 
**
** Description:
**	This data structure defines a linked list used as the buffer
**	for an array of IIAPI_DESCRIPTOR values.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

struct  _descriptorList
{
        struct _varRec		descrptRec[8];
        struct _descriptorList *next;
};

#define	MAX_DESCR_LIST		2





# endif				/* __AITDEF_H__ */
