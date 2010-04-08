#ifndef lint
static char rcsid[]="$Header: /cmlib1/ingres63p.lib/unix/tools/port/eval/machine.c,v 1.2 90/07/20 13:48:38 source Exp $";
#endif

/*
** History:
**	20-jul-1990 (chsieh)
**		add # include <sys/types.h> for system V. 
**		Otherwise, fcntl.h may complain.
**	09-feb-1993 (smc)
**	        Added axp_osf.
**	28-jul-1997 (walro03)
**		Added Nandem NonStopUX (ts2_us5).
**	21-May-2003 (bonro01)
**		Add support for HP Itanium (i64_hpu)
**      22-dec-2008 (stegr01)
**              Add support of VMS Itanium (i64_vms)
*/

# include	<stdio.h>
# include	<stdlib.h>
# include	<sys/types.h>
# include	<fcntl.h>

/*
**	machine.c - Copyright (c) 2004 Ingres Corporation 
**
**	What machine are we on?
**
**	Usage:	machine [-v][name]
**
**	If name is selected, prints either `yes' or `no',
**	otherwise prints one of:
**
**		uts		Amdahl Mainframe
**		u3b		ATT 3b/20
**		u3b2		3b/2
**		u3b5		3b/5
**		mc68k		some 68000's
**		mc68000		other 68000's
**		m68k		still other 68000's
**		m68000		and still other 68000's
**		vax		DEC VAX
**		pdp11		pdp 11
**		pyr		Pyramid 90x
**		tahoe		CCI Power 6
**		sel		Gould/SEL
**		elxsi		Elxsi 6400
**		tower		NCR Tower
**		ns32000		ns32000 Balance 8000
**
**	If 'unknown' is printed, we don't know what we're on.
*/

struct {
	char	*name;
	char	*vname;
} table[] = {
#ifdef __alpha
	{ "alpha", "DEC Alpha AXP"},
#endif
#ifdef uts
	{ "uts", "Amdahl Mainframe"},
#endif
#ifdef sun
	{ "sun", "sun microsystems" },
#endif
#ifdef aiws
	{ "aiws", "IBM RT/PC or AIX?" },
#endif
#ifdef u3b
	{ "u3b", "ATTIS 3b20" },
#endif
#ifdef u3b2
	{ "u3b2", "ATTIS 3b2" },
#endif
#ifdef u3b5
	{ "u3b5", "ATTIS 3b5" },
#endif
#ifdef u3b15
	{ "u3b15", "ATTIS 3b15" },
#endif
#ifdef mc68k
	{ "mc68k", "Some (System V?) 68000" },
#endif
#ifdef mc68000
	{ "mc68000", "Some (BSD?) 68000" },
#endif
#ifdef m68k
	{ "m68k", "Some (System V?) 68000" },
#endif
#ifdef m68000
	{ "m68000", "Some (System V?) 68000" },
#endif
#ifdef vax
	{ "vax", "DEC VAX" },
#endif
#ifdef pdp11
	{ "pdp11", "DEC PDP-11" },
#endif
#ifdef pyr
	{ "pyr", "Pyramid 90x" },
#endif
#ifdef tahoe
	{ "tahoe", "CCI Power 6" },
#endif
#ifdef sel
	{ "sel", "Gould/SEL" },
#endif
#ifdef elxsi
	{ "elxsi", "Elxsi 6400" },
#endif
#ifdef ns32000
	{ "ns32000", "Sequent Balance 8000" },
#endif
#ifdef tower
	{ "tower", "NCR tower" },
#endif
#ifdef apollo
	{ "apollo", "Apollo Domain Computer" },
#endif
#ifdef hp9000s300
	{ "hp9000s300", "HP 9000 Series 300" },
#endif
#ifdef hp9000s500
	{ "hp9000s500", "HP 9000 Series 500" },
#endif
#ifdef hp9000s800
	{ "hp9000s800", "HP 9000 Series 800" },
#endif
#ifdef i386
	{ "i386", "Intel 80386" },
#endif
#ifdef __nonstopux
	{ "NonStop", "Tandem NonStop UX" },
#endif
#if defined(__ia64) && defined(__hpux)
	{ "hp_ia64", "HP on Itanium" },
#endif
#if defined(__ia64) && defined(__vms)
        { "hp_ia64", "HP on Itanium"},
#endif
	{ "UNKNOWN", "Unknown box - fix machine.c" }
} ;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	strcmp();
	int	vflag = 0;
	char	*mach = NULL;

	argv++;
	argc--;
	while(argc--)
	{
		if(!strcmp(*argv, "-v"))
			vflag = 1;
		else 
			mach = *argv;
	}

	if( mach )
		printf( strcmp( mach, table[0].name ) ? "no\n" : "yes\n" );
	else if ( vflag )
		printf( "\t%s\n\t%s\n", table[0].name, table[0].vname );
	else
		printf( "%s\n", table[0].name );
	exit(0);
}
