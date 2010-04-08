/*
** Copyright (C) 1987, 2004 Ingres Corporation
*/

/**
** Name:	csmgmt.h	- shared between CSMO files and clients.
**
** Description:
**	Information shared between CSMO related files, like
**	attach/detach function externs and class def array locations.
**
** History:
**	13-Nov-1998 (wonst02)
**	    Add CS_samp_classes, CS_samp_threads.
**	23-jan-2004 (somsa01)
**	    Added CS_num_active(), CS_hwm_active().
**/

/* data */

GLOBALREF MO_CLASS_DEF CS_scb_classes[];
GLOBALREF MO_CLASS_DEF CS_sem_classes[];
GLOBALREF MO_CLASS_DEF CS_cnd_classes[];
GLOBALREF MO_CLASS_DEF CS_mon_classes[];
GLOBALREF MO_CLASS_DEF CS_int_classes[];
GLOBALREF MO_CLASS_DEF CS_samp_classes[];
GLOBALREF MO_CLASS_DEF CS_samp_threads[];

/* utilities */

FUNC_EXTERN VOID CS_mo_init(void);

FUNC_EXTERN STATUS CS_sem_attach( CS_SEMAPHORE *sem );
FUNC_EXTERN STATUS CS_detach_sem( CS_SEMAPHORE *sem );

FUNC_EXTERN void CS_scb_attach( CS_SCB *sem );
FUNC_EXTERN void CS_detach_scb( CS_SCB *sem );

FUNC_EXTERN STATUS CS_get_block( char *instance, SPTREE *tree, PTR *block );
FUNC_EXTERN STATUS CS_nxt_block( char *instance, SPTREE *tree, PTR *block );

FUNC_EXTERN i4 CS_num_active();
FUNC_EXTERN i4 CS_hwm_active();

