/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	feconfig.h	-	FE configuration file.
**
** Description:
**	This file contains # define constants that determine
**	the configuration of the FEs.  These should be set
**	based on the OS.
**
**	The constants are grouped according to the subsystem
**	they affect.
*/

/*
** For ABF
*/
#	define	COMPILE_OSL
#	define	IMTDY

# ifdef PCINGRES

# define	NOVIGRAPH
# define	NOGBF
# define	NORBF

# endif /* PCINGRES */
