
# include 	<compat.h>

/*
** Copyright (c) 1985 Ingres Corporation
**
** EXi.h -- EX Internal Definitions File.
**
**	Internal typedefs and constants for EX.
**
**	History:
**	
 * Revision 1.2  90/05/16  20:10:23  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:14:51  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:40:58  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:48:59  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:09:29  source
 * sc
 * 
 * Revision 1.2  89/03/07  15:27:12  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:03  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:36:21  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.1  87/11/09  12:59:16  mikem
**      Initial revision
**      
 * Revision 1.2  87/09/02  19:10:49  source
 * sc
 * 
 * Revision 1.1  87/07/30  18:04:11  roger
 * This is the new stuff.  We go dancing in.
 * 
**		Revision 1.3  86/04/25  16:54:43  daveb
**		The old NO_EX is now universal.  Remove all
**		unneeded machine dependant stuff.  Add 
**		extern declarations for saved signal handlers
**		added for 3.0 on System V.  Leave room for 
**		similar 4.2 implementation.
**		
 * Revision 1.2  86/04/24  11:59:47  daveb
 * EX is no longer done in assembler on UNIX.
 * Remove obsolete machine specific definitions.
 *
**		Revision 3.0  85/05/13  15:23:11  wong
**		Integrated definitions for variant systems (from 2.0).
**
**	16-may-90 (blaise0
**	    Integrated changes from 61 and ug:
**		The PS2 has sigvec as an int function, not an int* function;
**		Sequent's rename sigvec is now hidden behind sigvec_func,
**		defined in clsigs.h;
**		Added reference to SIGACTION (POSIX equivalent of sigvec).
**	02-jul-91 (johnr)
**		Excluded sigvec_func for hp3_us5.
**      16-nov-92 (terjeb)
**              The HP9000 S800 has sigvec as an int function. 
**	20-jan-92 (smc)
**		Excluded sigvec_func for axp_osf.
**	10-sep-93 (peterk)
**		exclude sigvec_func for hp8_us5; was getting "redefined" error
**	10-feb-94 (ajc)
**		hp8_bls added based on hp8_us5
**	04-mar-94 (ajc)
**		Fixed typo.
**      13-oct-94 (canor01)
**              use prototypes from system headers for sigvec_func for rs4_us5
*/

EX_CONTEXT		*i_EXtop();
EX_CONTEXT		*i_EXnext();

# define	EXMAXARG	100

void	i_EXpop();
void	i_EXpush();

# if defined(xCL_011_USE_SIGVEC) || defined(xCL_068_USE_SIGACTION)

# if  !defined(rs4_us5)
# if defined(ps2_us5)
extern TYPESIG sigvec_func();
# else
# if !defined(hp3_us5) && !defined(axp_osf) && !defined(hp8_us5) && \
     !defined(hp8_bls)
extern TYPESIG (*sigvec_func())();
# endif /* hp3_us5 & axp_osf & hp8_us5 & hp8_bls */
# endif
# endif /* rs4_us5*/

# endif
