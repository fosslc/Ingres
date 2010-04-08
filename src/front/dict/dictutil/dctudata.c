/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include       <gl.h>    
# include       <sl.h>    
# include       <iicommon.h>
# include       <fe.h>    

/**
** Name:	dctudata.c
**
** Description: Global data for dictuil facility
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
**      07-nov-2001 (stial01)
**          Added IIDDpagesize.
*/

GLOBALDEF	bool	IIDDsilent = FALSE;
GLOBALDEF char  IIDDResolvefilemsg[FE_MAXNAME+1];
GLOBALDEF	i4	IIDDpagesize = 0;
