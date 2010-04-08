/*
** Copyright (c) 2004 Ingres Corporation
**
*/

FUNC_EXTERN DB_STATUS
psl_yalloc(
	PTR		stream,
	SIZE_TYPE	*memleft,
	PTR    		*yacc_cb,
	DB_ERROR        *err_blk);
