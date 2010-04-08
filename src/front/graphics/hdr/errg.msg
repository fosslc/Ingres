/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	errg.msg -	Message file for class RG
**
** Description:
**	This file defines the messages for class RG.
**
** History:
**	 5-aug-1987 (bobm)
**		Created.
**	04-may-97 (mcgem01)
**		Remove hard-coded references to TERM_INGRES.
**/

# define	_RG_CLASS	75
/*
**	dograf.c
*/
/*
**	Status message displayed during graph retrieval
**		Format string argument is graph name
*/
F_RG0001_Retrieving	"Retrieving graph '%0c' . . .\n"
F_RG0002_Retrieving_data	"Retrieving data . . .\n"
F_RG0003_map_warnings	"Graph map warnings."
F_RG0004_Graph_plotted	"Graph plotted."
/*
**	rgraf.qc
*/
/*
**	fatal error - not supposed to happen.
**	(Only applies to -D option, anyway)
*/
S_RG0001_dump_open	"vig: cannot open dump file, %0c\n"	/* DIAG */
/*
**	fatal error - not supposed to happen
*/
S_RG0002_Excep_set	"rungraf: could not set exception handler" /* DIAG */
/*
**	Bad TERM / TERM_INGRES setting.
*/
E_RG0003_NO_TERM	"Can not determine user's terminal type.\n  Please
define your terminal type via %0c."
