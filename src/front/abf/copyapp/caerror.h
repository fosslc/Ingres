/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**	copyappl.h
**
**	
**	Defines
**		Header file for copyappl utility
**
**	Called by:
**		All routines in copyappl utility
**
**	History:
**		Written, 11/15/83 (agh)
**		Made into separate file, 11/12/85 (agh)
**
*/

# include	"erca.h"

/*
** Errors
*/
# define	DUPOBJNAME	E_CA0007_DUPOBJNAME
# define	REPLACE		E_CA0008_REPLACE
# define	NOCHANGE	E_CA0009_NOCHANGE
# define	DIRECTION	E_CA000A_DIRECTION
# define	FLAGARG		E_CA000B_FLAGARG
# define	BADFLAG		E_CA000C_BADFLAG
# define	FILEOPEN	E_CA000D_FILEOPEN
# define	DIREXIST	E_CA000E_DIREXIST
# define	EXITMSG		E_CA000F_EXITMSG
# define	TOOLONGNAME	E_CA0010_TOOLONGNAME
# define	CANOSRCDIR	E_CA0011_CANOSRCDIR
# define	FLAGCNFLT	E_CA0012_FLAGCNFLT
