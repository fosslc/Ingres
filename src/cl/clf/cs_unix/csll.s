/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**	csll.s -- CS assembler code 
**
**	Since all source in this file is (mutually) exclusively
**	machine independent, this file is broken into machine specific
**	subfiles.
**
**	History:
**		09-may-89 (russ)
**			odt_us5 now uses csll.sqs.asm, except with no
**			underscores on global symbols.
**		16-apr-80 (kimman)
**			Added ds3_ulx specific code to call csll.ds3.asm.
**		5-june-90 (blaise)
**			Added a bunch of machine specific code.
**		4-sep-90 (jkb)
**			Add sqs_ptx to list of defines
**		01-oct-90 (chsieh)
**			Added bu2_us5 and bu3_us5 specific code to include    
**			csll.dpx.asm.
**		02-jul-92 (swm)
**			Added dra_us5 to list of machines using csll.sqs.asm.
**			Added dr6_uv1 to use csll.ucorn.asm.
**		01-dec-92 (mikem)
**			su4_us5 6.5 port.  The solaris 2.1 port uses 
**			csll.sol.asm assembly file.
**		08-jan-93 (sweeney)
**			Yet another intel x86 port.
**		24-mar-93 (swm)
**			Alpha AXP OSF port: axp_osf uses csll.axp.asm.
**		11-oct-93 (robf)
**          		Add su4_cmw
**		10-feb-1994 (ajc)
**			Added hp8_bls in line with hp8*
**		13-mar-1995 (smiba01)
**			Added dr6_ues in line with dr6*
**		15-jun-95 (popri01)
**			Yet another intel x86 port: nc4_us5
**		25-jun-95 (allmi01)
**			added support for dgi_us5 (DG-UX on Intel based platforms) which
**			is yet another intel x86 port.
**		14-jul-1995 (morayf)
**			Added sos_us5 in line with odt_us5
**              17-jul-1995 (pursch)
**                      Add changes made by gmcquary.
**                      Add pym_us5 to use csll.pyrvr4.asm.
**      	10-nov-1995 (murf)
**                 Added sui_us5 to all areas specifically defined with usl_us5.
**                 Reason, to port Solaris for Intel.
**		07-dec-1995 (morayf)
**			Added rmx_us5 for SNI port. This is a Pyramid clone
**			with Sys V OS => we need the pyrvr4.asm (pym_us5) file.
**			Also added identical pym_us5 entry which was somewhat
**			mysteriously missing.
**		29-jul-1997 (walro03)
**			Added pyrvr4.asm for Tandem NonStop (ts2_us5).  As noted
**			in 6.4/05/03, this is becoming the de-facto standard
**			for MIPS machines.
**		07-Nov-1997 (allmi01)
**			Added Silicom Graphic to list of port which use pyrvr4.asm
**			as it is a MIPS port also (sgi_us5).
**		19-feb-1998 (toumi01)
**			Added Linux (lnx_us5) support using csll.lnx.asm.
**		27-Mar-1998 (muhpa01)
**			Added hpb_us5 to list of HP platforms
**		10-may-1999 (walro03)
**			Remove obsolete version string aco_u43, arx_us5,
**			bu2_us5, bu3_us5, bul_us5, cvx_u42, dpx_us5, dr3_us5,
**			dr4_us5, dr5_us5, dr6_ues, dr6_uv1, dra_us5, ix3_us5,
**			odt_us5, ps2_us5, pyr_u42, rtp_us5, sqs_u42, sqs_us5,
**			vax_ulx, x3bx_us5.
**              03-jul-99 (podni01)
**                      Added support for ReliantUNIX V 5.44 - rux_us5 (same as
**                      rmx_us5)
**		06-oct-1999 (toumi01)
**			Change Linux config string from lnx_us5 to int_lnx.
**		20-apr-1999 (hanch04)
**		    Added su9_us5 for 64 bit Solaris 7
**		15-aug-2000 (somsa01)
**			Added support for ibm_lnx.
**		21-Sept-2000 (hanje04)
**			Added support for axp_lnx. Will use axp_osf 
**			Assembler code.
**		19-jul-2001 (toumi01)
**			add support for i64_aix
**		03-Dec-2001 (hanje04)
**		        Added support for IA64 Linux.
**		05-aug-2002 (somsa01)
**			Added support for hp2_us5.
**		31-Oct-2002 (hanje04)
**			Added support for AMD x86-64 Linux a64_lnx
**		21-May-2003 (bonro01)
**			Added support for HP Itanium (i64_hpu)
**		26-Nov-2002 (bonro01)
**			Moved SGI code to csll.sgi.asm
**			Added support for 64-bit SGI
**		10-Dec-2004 (bonro01)
**			Added support for AMD x86-64 Solaris a64_sol
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	    No internal threads.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**      23-Mar-2007
**          SIR 117985
**          Add support for 64bit PowerPC Linux (ppc_lnx)
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Add support for Intel OSX
**	26-Jun-2009 (kschendel) SIR 122138
**	    Include rs4/r64 aix here, jamrules now cpp's before m4.
**	14-July-2009 (toumi01)
**	    Le quatorze juillet change to x86 Linux threading to end the
**	    historical use of csll.lnx.asm and use minimal low level code
**	    added to csnormal.h to handle atomic functions. This avoids
**	    RELTEXT issues that are problematic for SELinux.
*/ 

# ifdef ds3_ulx
# define got_1
# include "csll.ds3.asm"
# endif /* ds3_ulx */


# if defined(sun_u42) || defined(su4_u42) || defined(su4_cmw)
# define got_1
# include "csll.sun.asm"
# endif  /* sun_u42 su4_u42 */

# if defined(su4_us5) && !defined(su9_us5)
# define got_1
# include "csll.sol.asm"
# endif  /* su4_us5 */

# if defined(su9_us5)
# define got_1
# include "csll.su9.asm"
# endif  /* su4_us5 */

# ifdef dr4_us5
# define _CS_tas                CS_tas
# define _CS_getspin            CS_getspin
# define _CS_swuser             CS_swuser
# define _ex_sptr               ex_sptr
# define _Cs_srv_block          Cs_srv_block
# define _Cs_idle_scb           Cs_idle_scb
# define _CS_xchng_thread       CS_xchng_thread
# define got_1
# include "csll.400e.asm"
# endif  /* dr4_us5 */

# if defined(rmx_us5) || defined(rux_us5)
# define got_1
# include "csll.pyrvr4.asm"
# endif /* rmx_us5 */

# if defined(pym_us5) 
# define got_1
# include "csll.pyrvr4.asm"
# endif /* pym_us5 */

# if defined(ts2_us5) 
# define got_1
# include "csll.pyrvr4.asm"
# endif /* ts2_us5 */

# if defined(sgi_us5) 
# define got_1
# include "csll.sgi.asm"
# endif /* sgi_us5 */

# if defined(sco_us5) || defined(sur_u42) || \
     defined(sqs_ptx) || defined(usl_us5) || \
     defined(nc4_us5) || defined(sos_us5) || defined(sui_us5)
     
# define got_1
# define _CS_tas		CS_tas
# define _CS_getspin		CS_getspin
# define _CS_relspin		CS_relspin
# define _CS_swuser		CS_swuser
# define _ex_sptr		ex_sptr
# define _Cs_srv_block		Cs_srv_block
# define _Cs_idle_scb		Cs_idle_scb
# define _CS_xchng_thread	CS_xchng_thread
# include "csll.sqs.asm"
# endif	/* sco_us5 sur_u42 sqs_ptx usl_us5 nc4_us5 sos_us5 sui_us5 */
 
# if defined(dgi_us5)
     
# define got_1
# define _CS_tas              CS_tas
# define _CS_getspin          CS_getspin
# define _CS_relspin          CS_relspin
# define _CS_swuser           CS_swuser
# define _ex_sptr             ex_sptr
# define _Cs_srv_block        Cs_srv_block
# define _Cs_idle_scb         Cs_idle_scb
# define _CS_xchng_thread     CS_xchng_thread
# include "csll.dgi.asm"
# endif       /* dgi_us5 */

/*
** Jamdefs and Jamrules are not set up to support the inclusion of bzarch.h
** when the action preprocesses with the C precompiler and then assembles.
** That's where we'd find out if NO_INTERNAL_THREADS were set. For now just
** force the issue because we're not building with internal Ingres threading
** on Linux anyway. If this situation changes then modify the build rules,
** include bzarch.h in this file, and delete the following 3 lines.
*/
# if defined(int_lnx) || defined (int_rpl)
# define NO_INTERNAL_THREADS
# endif

# if (defined(int_lnx) && !defined(NO_INTERNAL_THREADS)) || \
     (defined(int_rpl) && !defined(NO_INTERNAL_THREADS))
     
# define got_1
# define _CS_tas              CS_tas
# define _CS_getspin          CS_getspin
# define _CS_relspin          CS_relspin
# define _CS_swuser           CS_swuser
# define _ex_sptr             ex_sptr
# define _Cs_srv_block        Cs_srv_block
# define _Cs_idle_scb         Cs_idle_scb
# define _CS_xchng_thread     CS_xchng_thread
# include "csll.lnx.asm"
# endif       /* int_lnx || int_rpl && !NO_INTERNAL_THREADS */

# if (defined(hp8_us5) || defined(hp8_bls) || defined(hpb_us5)) && \
     !defined(hp2_us5)
# include "csll.hp8.asm"
# endif

# if defined(hp2_us5)
# include "csll.hp2.asm"
# endif

# if defined(ib1_us5)
# define got_1
# include "csll.pow6.asm"
# endif       /* CCI Power6/32 processors */

# ifdef hp3_us5
# define got_1
# include "csll.hp3.asm"
# endif

# if defined(dr6_us5)
# define _CS_tas                CS_tas
# define _CS_swuser             CS_swuser
# define _ex_sptr               ex_sptr
# define _Cs_srv_block          Cs_srv_block
# define _Cs_idle_scb           Cs_idle_scb
# define _CS_xchng_thread       CS_xchng_thread
# define got_1
# include "csll.ucorn.asm"
# endif /* dr6_us5 */

# ifdef dg8_us5
# define got_1
# include "csll.dg8.asm"
# endif

#if defined(axp_osf) || defined(axp_lnx) 
# define got_1
# include "csll.axp.asm"
# endif

# ifdef ibm_lnx
# define got_1
# include "csll.ibm.asm"
# endif       /* ibm_lnx */

# if defined(i64_lnx)
# define got_1
# include "csll.i64.asm"
# endif /* i64_lnx */

# if defined(a64_lnx)
# define got_1
# include "csll.a64.asm"
# endif /* i64_aix */

# if defined(a64_sol)
# define got_1
# include "csll.a64sol.asm"
# endif /* a64_sol */

# if defined(i64_hpu)
# define got_1
# include "csll.i64hp.asm"
# endif /* i64_hpu */

#if defined(rs4_us5) || defined(r64_us5)
# define got_1
# include "csll.rs4.asm"
#endif

# if defined(mg5_osx) || defined(ppc_lnx) || defined(int_osx) || \
     (defined(int_lnx) && defined(NO_INTERNAL_THREADS)) || \
     (defined(int_rpl) && defined(NO_INTERNAL_THREADS))
# define got_1
/* no internal threads, no asm */
# endif /* mac_osx || pcc_lnx || int_osx || ( int_lnx || int_rpl && NO_INTERNAL_THREADS ) */
# ifndef got_1
Missing_machine_dependent_code
# endif /* got_1 */

