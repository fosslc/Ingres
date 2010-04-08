/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*	static char	Sccsid[] = "@(#)ctrl.h	30.1	12/1/84";	*/

#ifndef EBCDIC
# define	ctrlNUL		00
# define	ctrlA		01
# define	ctrlB		02
# define	ctrlC		03
# define	ctrlD		04
# define	ctrlE		05
# define	ctrlF		06
# define	ctrlG		07
# define	ctrlH		010
# define	ctrlI		011
# define	ctrlJ		012
# define	ctrlK		013
# define	ctrlL		014
# define	ctrlM		015
# define	ctrlN		016
# define	ctrlO		017
# define	ctrlP		020
# define	ctrlQ		021
# define	ctrlR		022
# define	ctrlS		023
# define	ctrlT		024
# define	ctrlU		025
# define	ctrlV		026
# define	ctrlW		027
# define	ctrlX		030
# define	ctrlY		031
# define	ctrlZ		032
# define	ctrlESC		033
# define	ctrlBKSL	034
# define	ctrlLBRK	035
# define	ctrlARROW	036
# define	ctrlDASH	037
# define	ctrlDEL		0177
# define	ctrlUPPER	037
#else
# define	ctrlNUL		0x00
# define	ctrlA		0x01
# define	ctrlB		0x02
# define	ctrlC		0x03
# define	ctrlD		0x37
# define	ctrlE		0x2d
# define	ctrlF		0x2e
# define	ctrlG		0x2f
# define	ctrlH		0x16
# define	ctrlI		0x05
# define	ctrlJ		0x25
# define	ctrlK		0x0b
# define	ctrlL		0x0c
# define	ctrlM		0x0d
# define	ctrlN		0x0e
# define	ctrlO		0x0f
# define	ctrlP		0x10
# define	ctrlQ		0x11
# define	ctrlR		0x12
# define	ctrlS		0x13
# define	ctrlT		0x3c
# define	ctrlU		0x3d
# define	ctrlV		0x32
# define	ctrlW		0x26
# define	ctrlX		0x18
# define	ctrlY		0x19
# define	ctrlZ		0x3f
# define	ctrlESC		0x27
# define	ctrlBKSL	0x1c
# define	ctrlLBRK	0x1d
# define	ctrlARROW	0x1e
# define	ctrlDASH	0x1f
# define	ctrlDEL		0x07
# define	ctrlUPPER	0x1f
#endif
