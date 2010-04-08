/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**	DM2A.H	Definitions for Aggregate Processor
**
** Description:
**	This file contains definitions used by the DM2A.C aggregate
**	processor, for execution of those aggregates that QEF sees
**	fit to pass of to DMF.
**
** History:
**	04-sep-1995 (cohmi01)
**	    Added, for aggregate optimization project.
**	30-jul-96 (nick)
**	    Add function prototypes.
**	11-Feb-1997 (nanpr01)
**	    Bug 80636 : Pass the QEF_ADF_CB for proper initialization.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Apr-2008 (kschendel)
**	    Remove QEF exception handler from call sequence.
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_? functions converted to DB_ERROR *
*/


/* 
** Aggregate Limit Indicators
**
** These bits indicate which of the 'dmf-optimizable' aggregates have
** been specified in this request. There must be code in the ADE_EXCB block
** passed in from QEF that must correspond to these requests, except in
** the case of count. The presence of any of these bits means that NO OTHER
** aggregates (eg SUM) are present in this request, and that DMF is
** therefore able to optimize where possible, to the extent of not even
** reading data, which would otherwise be necessary for processing any of
** the non-'dmf-optimizable' aggregates.
*/
#define DM2A_AGLIM_COUNT	DMR_AGLIM_COUNT /* COUNT aggregate requested*/
#define DM2A_AGLIM_MAX 		DMR_AGLIM_MAX   /* MAX aggregate requested  */
#define DM2A_AGLIM_MIN   	DMR_AGLIM_MIN   /* MIN aggregate requested  */

/* 
** Maximum number of Aggregate Action blocks that could be required to 
** process any arbitrary combination of aggregates.
*/
#define	DM2A_MAXAGGACT		3

/* 
** Aggregate Action block - describes one or more actions that must be
** executed to satisfy the request. Each action involves some type of
** scan of data, with type of interpretation, as indicated in the fields
** for each action.
*/

typedef struct _DM2A_AGGACT	
{
    i4		operation;			/* what type of scan	      */
#define			DM2A_TUPSCAN	1	/* Read each tuple 	      */
#define 		DM2A_PAGESCAN	2	/* just read leaf/data pages  */
    i4		direction;			/* NEXT or PREV  	      */
    i4		break_on;			/* when to break from loop    */
#define			DM2A_BRK_NONEXT 1	/* loop till no more recs     */
#define			DM2A_BRK_FIRST  2	/* Break loop after 1st entry */
#define			DM2A_BRK_NEWMAINPG  3	/* Break when pass this mainpg*/
    bool	call_ade;			/* Need to call ade_execute ? */
    DB_STATUS	(*scanner)();			/* function for DM2A_PAGESCAN */
    DM_PAGENO	orig_mainpg;			/* mainpg of ISAM range	      */
    DMP_RCB	*rcb;				/* rcb for this action        */
} DM2A_AGGACT;


/* Indicators of where final record count should be taken from  */
#define			DM2A_COUNT_REC	1	/* use record count    	      */
#define			DM2A_COUNT_PAG 	2  	/* use page counter's count   */
#define			DM2A_COUNT_NONE 3  	/* no count is available      */

/* functions */

FUNC_EXTERN DB_STATUS dm2a_simpagg(
	DMP_RCB		*rcb,
	i4		agglims,
	ADE_EXCB	*excb,
	ADF_CB		*qef_adf_cb,
	char        	*rec,
	i4		*count,
	DB_ERROR     	*dberr );

