
/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name: st.h - Compatibility Library String Module Interface Definition.
**
** Description:
**	Contains declarations and definitions that define the interface for
**	the ST module of the CL.
**
** History:
**	3-10-83   - Written (jen)
**	19-jun-87 (bab)
**		Changed type of STcopy to be VOID to match code and
**		CL specification.
**	87/05  wong
**		Added 'STequal()' definition.
**	87/12/08  wong
**		Updated declarations for 6.0.
**	24-jan-89 (mmm)
**		Changed STtrmwhite() to return "nat" to match latest CL spec.
**	11-may-89 (daveb)
**		Straighten out history.
**		Settle types of string routines:  They are nats, not u_i4s.
**		  (Will need to change code to match too.)
**		Add missing STskipblank.
**		NOTE: STpad isn't in the spec.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add STLENGTH_MACRO, which tests the first slot of a string 
**		before calling STlength.
**	07-mar-91 (rudyw)
**	    Eliminate tab characters in comment string causing compiler problem
**      28-nov-92 (terjeb)
**          Redfine STlength, STcompare, STcopy and STscanf for better
**          performance and make the HP compiler in-line when possible.
**          Also include <memory.h> to obtain prototype definitions.
**	30-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    declarations. Delete declarations of STscanf; it's already
**	    generally defined as sscanf.
**	16-feb-93 (pearl)
**	    Some function parameters were defined as "register"; add this
**	    to their prototyped declarations.  Add declaration of STmove().
**	31-mar-93 (swm)
**	    Altered STmove function declaration to correspond to
**	    the CL specification by making the dlen parameter a
**	    i4  rather than an i2. This fix allows compilation
**	    in back/opc to run without errors on Alpha AXP OSF.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      15-jun-93 (terjeb)
**          Indicate by additional defines that we have redefined some of
**          the ST function to use library functions instead.
**	16-sep-93 (vijay)
**	    Use library functions and xlc inline functions on AIX. Include
**	    string.h and systypes.h.
**	28-feb-94 (ajc)
**	    Added hp8_bls port specific changes based on hp8*
**      10-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**	03-apr-95 (canor01)
**	    Use library functions for su4_us5
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	17-oct-95 (emmag)
**	    Add int_wnt & axp_wnt to the list of operating systems using strcmp.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	15-may-1999 (popri01)
**	    Use library functions for Unixware (usl_us5).
**	24-may-1999 (somsa01)
**	    On NT, there is no 'strings.h'. Also, on NT, it's stricmp and
**	    strnicmp.
**	26-apr-99 (schte01)
**	    Add axp_osf to the list of operating systems using native 
**       string functions.
**	18-jun-99 (toumi01)
**	    Use library functions for lnx_us5.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	19-jan-2001 (somsa01)
**	    Added ibm_lnx defines for S/390 Linux.
**      02-jul-2001 (loera01)
**          Added definition of STstr for the ODBC.
**	23-apr-2003 (penga03)
**	    Added STnmbcpy.
**	28-Feb-2005 (hanje04)
**	   Cannot use strncpy() for STnmbcpy with DOUBLEBYTE builds on 
**	   unix as the length used is bytes not characters. Use new CMccopy()
**	   which does what we need it to.
**  08-Jul-2005 (fanra01)
**      Compiler redefinition warnings.  Adjust the conditional for STnmbcpy.
**	11-Feb-2009 (drivi01)
**	   Redefine strtok as STtok.
*/

#include <systypes.h>
#include <string.h>
#ifndef NT_GENERIC
#include <strings.h>
#endif
#include <memory.h>

# define	STcompare	strcmp
# define	STcmp		strcmp
# define	STncmp		strncmp

#ifdef NT_GENERIC
# define	STcasecmp	stricmp
# define	STncasecmp	strnicmp
#else
# define	STcasecmp	strcasecmp
# define	STncasecmp	strncasecmp
#endif

# ifdef NT_GENERIC
# ifdef DOUBLEBYTE
# define	STnmbcpy	_mbsncpy
# else
# define	STnmbcpy	strncpy
# endif /* DOUBLEBYTE */
# else  /* NT_GENERIC */
# ifdef DOUBLEBYTE
/*
** There is no multi-byte sting copy on UNIX so we have to use our own
*/
# define	STnmbcpy(dest, src, len)	CMccopy(src, len, dest)
# else
# define	STnmbcpy	strncpy
# endif /* DOUBLEBYTE */
# endif /* NT_GENERIC */

# define	STcopy(a,b)	strcpy(b,a)
# define	STcpy		strcpy
# define	STncpy		strncpy
# define	STlength	strlen
# define	STlen		strlen
# define	STchr		strchr
# define	STrchr		strrchr
# define	STscanf		sscanf
# define	STstr		strstr
# define	STrstr		strrstr
# define	STcat		strcat
# define	STncat		strncat
# define	STtok		strtok

/*
** Name:	STequal() -	Fast Equality String Comparison Macro.
**
** Description:
**	This macro does a fast exact match between its two string arguments
**	returning TRUE for a match.
**
** Input:
**	s1	{char *}  The first string.
**	s2	{char *}  The second string.
**
** Returns:
**	{bool}	TRUE on match.
**
** History:
**	10/86 (jhw) -- Written.
*/

#define STequal(s1, s2) (*(s1) == *(s2) && STcompare((s1), (s2)) == 0)

/*
** 'STscanf()' --> 'sscanf()' on UNIX
*/

# define STscanf sscanf

/*
** STlength macro
*/
#define STLENGTH_MACRO(str) \
        (((str)[0]==EOS)?0:(((str)[1]==EOS)?1:STlength((str))))

/*
**  STstr macro
*/
#define STstr strstr
