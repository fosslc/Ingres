/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	grproc.h -	OC_OBJECT procedure extern declarations.
**
** Description:
**	Generated automatically by classout.
**
** History:
**	Revision 6.2  88/12  wong
**	Deleted Class procedures for Graph only objects.
**
**	Revision 4.0  86/01  peterk
**	Initial revision.
**
**	21-Aug-2009 (kschendel) 121804
**	    Kill off bogus definitions of functions that don't exist.
**	    Fix return types in case someone actually uses these.
**/
FUNC_EXTERN OOID	iiooAlloc();
FUNC_EXTERN OOID	iiooClook();
FUNC_EXTERN OOID	iiooCnew();
FUNC_EXTERN OOID	iiooC0new();
FUNC_EXTERN OOID	iiooCDbnew();
FUNC_EXTERN OOID	iiooCperform();
FUNC_EXTERN OOID	iiooCwithName();
FUNC_EXTERN OOID	ENdecode();
FUNC_EXTERN OOID	ENdestroy();
FUNC_EXTERN OOID	ENfetch();
FUNC_EXTERN OOID	ENflush();
FUNC_EXTERN OOID	ENinit();
FUNC_EXTERN OOID	ENinit();
FUNC_EXTERN OOID	ENreadfile();
FUNC_EXTERN i4		ENwritefile();
FUNC_EXTERN int		GRCencode();
FUNC_EXTERN OOID	GRaxinit();
FUNC_EXTERN OOID	GRaxinit();
FUNC_EXTERN OOID	GRbinit();
FUNC_EXTERN OOID	GRbinit();
FUNC_EXTERN OOID	GRcinit();
FUNC_EXTERN OOID	GRcinit();
FUNC_EXTERN OOID	GRdecode();
FUNC_EXTERN OOID	GRdepinit();
FUNC_EXTERN OOID	GRdepinit();
FUNC_EXTERN bool	GRdestroy();
FUNC_EXTERN OOID	GRencode();
FUNC_EXTERN OOID	GRfetch();
FUNC_EXTERN OOID	GRflush();
FUNC_EXTERN OOID	GRinit();
FUNC_EXTERN OOID	GRinitDb();
FUNC_EXTERN OOID	GRleginit();
FUNC_EXTERN OOID	GRleginit();
FUNC_EXTERN OOID	GRlinit();
FUNC_EXTERN OOID	GRlinit();
FUNC_EXTERN bool	GRnameOk();
FUNC_EXTERN OOID	GRoptInit();
FUNC_EXTERN OOID	GRoptInit();
FUNC_EXTERN OOID	GRpinit();
FUNC_EXTERN OOID	GRpinit();
FUNC_EXTERN OOID	GRsinit();
FUNC_EXTERN OOID	GRsinit();
FUNC_EXTERN OOID	GRtinit();
FUNC_EXTERN OOID	GRtinit();
FUNC_EXTERN int		GRvig1Axd();
FUNC_EXTERN int		GRvigAxdat();
FUNC_EXTERN GR_OBJ	*GRvigBar();
FUNC_EXTERN GR_OBJ	*GRvigComp();
FUNC_EXTERN int		GRvigDepdat();
FUNC_EXTERN OOID	GRvigGraph();
FUNC_EXTERN GR_OBJ	*GRvigLegend();
FUNC_EXTERN GR_OBJ	*GRvigLine();
FUNC_EXTERN OOID	GRvigOpt();
FUNC_EXTERN GR_OBJ	*GRvigPie();
FUNC_EXTERN GR_OBJ	*GRvigScat();
FUNC_EXTERN GR_OBJ	*GRvigText();
FUNC_EXTERN OOID	Minit();
FUNC_EXTERN OOID	MinitDb();
FUNC_EXTERN OOID	OBclass();
FUNC_EXTERN OOID	OBcopy();
FUNC_EXTERN OO_OBJECT *	OBcopyId();
FUNC_EXTERN bool	OBdestroy();
FUNC_EXTERN OOID	OBedit();
FUNC_EXTERN OOID	OBencode();
FUNC_EXTERN OOID	OBinit();
FUNC_EXTERN void	OBinsertrow();
FUNC_EXTERN bool	OBisEmpty();
FUNC_EXTERN bool	OBisNil();
FUNC_EXTERN OOID	OBmarkChange();
FUNC_EXTERN char *	OBname();
FUNC_EXTERN OOID	OBnoMethod();
FUNC_EXTERN int		OBprint();
FUNC_EXTERN char *	OBrename();
FUNC_EXTERN OOID	OBsetPermit();
FUNC_EXTERN OOID	OBsetRefNull();
FUNC_EXTERN OOID	OBsubResp();
FUNC_EXTERN OOID	OBtime();
FUNC_EXTERN OO_REFERENCE *REinit();
FUNC_EXTERN OO_REFERENCE *REinitDb();
FUNC_EXTERN OOID	fetch0();
FUNC_EXTERN OOID	fetchAll();
FUNC_EXTERN OOID	fetchAt();
FUNC_EXTERN OOID	fetchDb();
FUNC_EXTERN OOID	flush0();
FUNC_EXTERN OOID	flushAll();
FUNC_EXTERN OOID	flushAt();
