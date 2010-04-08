/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ermt.msg -	Message file for class MT
**
** Description:
**	This file defines the messages for class MT.
**
** History:
**	13-nov-1987 (dave)
**		Created.
**	11/21/87 (dkh) - Changed to use official class number, 185.
**/

# define	_MT_CLASS	185

/*
**	mtview.c
*/
S_MT0001_A_FRSKEY_WAS_PRESSED	"A FRSKEY WAS PRESSED WITH VALUE %d\n"
/*
**	mtdraw.c
*/
F_MT0001_End_of_Output	"End of Output"
F_MT0002_Start_of_Output	"Start of Output"
F_MT0003_Scroll_down_for_more	"Scroll down for more"
F_MT0004_Start_of_in_memory_ou	"Start of in-memory output; use 'Top'"
F_MT0005_Column_header_info	"Column %d/%d  "
F_MT0006_Line_header_info	"Line %d/%d"
/*
**	The previous entry has the extra %d/%d to indicate that we're at
**	line N of M. This next entry doesn't know yet how many lines there
**	are, so only print Line N.
**	.
*/
F_MT0007_Line_info_inc	"Line %d"
