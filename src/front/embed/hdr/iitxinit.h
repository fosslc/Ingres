/*--------------------------- iitxinit.h -----------------------------------*/
/* Copyright (c) 2004 Ingres Corporation */
/*
** Name: iitxinit.h
**
** Description:
**	Header file for Tuxedo bridge contains entry points for 
**      tuxedo-specific functions
**
** History:
**	10-Nov-93 (larry)
**	    written.
*/

#ifndef IITXINIT_H
#define IITXINIT_H

/*
** LIBQTXXA initialization routines
*/

FUNC_EXTERN int
IItux_shutdown();

FUNC_EXTERN int
IItux_init(char	*server_group);

#endif /* ifndef IITXINIT_H */
/*--------------------------- iitxinit.h -----------------------------------*/
