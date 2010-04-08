/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	scu.h - client interface to scu functions.
**
** Description:
**	This file defines prototypes for clients to use for SCU 
**	functions.
**
** History:
**	01-Apr-1998 (jenjo02)
**	    SCD_MCB renamed to SCS_MCB.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parm to scu_information(),
**	    scu_idefine(), scu_malloc(), scu_mfree(),
**	    scu_xencode() prototypes.
**/

FUNC_EXTERN DB_STATUS scu_information(SCF_CB *cb,
				      SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scu_idefine(SCF_CB *cb,
				  SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scu_malloc(SCF_CB *scf_cb,
				 SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scu_mfree(SCF_CB *scf_cb,
				SCD_SCB *scb );

FUNC_EXTERN DB_STATUS scu_mfind_mcb(SCF_CB *scf_cb,
				    SCS_MCB **mcb,
				    i4  *deleted );

FUNC_EXTERN i4 scu_mcount(SCD_SCB *scb, i4  *count );

FUNC_EXTERN DB_STATUS scu_mdump(SCS_MCB *head, i4  by_session );

FUNC_EXTERN DB_STATUS scu_xencode(SCF_CB *scf_cb,
				  SCD_SCB *scb );

