/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	csmgmt.h	- shared between CSMO files and clients.
**
** Description:
**	Information shared between CSMO related files, like
**	attach/detach function externs and class def array locations.
**
**/

/* data */

GLOBALREF MO_CLASS_DEF CS_scb_classes[];
GLOBALREF MO_CLASS_DEF CS_sem_classes[];
GLOBALREF MO_CLASS_DEF CS_cnd_classes[];
GLOBALREF MO_CLASS_DEF CS_mon_classes[];
GLOBALREF MO_CLASS_DEF CS_int_classes[];

/* utilities */

FUNC_EXTERN STATUS CS_sem_attach( CS_SEMAPHORE *sem );
FUNC_EXTERN STATUS CS_detach_sem( CS_SEMAPHORE *sem );

FUNC_EXTERN void CS_scb_attach( CS_SCB *sem );
FUNC_EXTERN void CS_detach_scb( CS_SCB *sem );

FUNC_EXTERN STATUS CS_get_block( char *instance, SPTREE *tree, PTR *block );
FUNC_EXTERN STATUS CS_nxt_block( char *instance, SPTREE *tree, PTR *block );


