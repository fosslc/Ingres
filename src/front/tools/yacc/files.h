/*
** Copyright (c) 1990, 2004 Ingres Corporation
**
*/
/*
 * files.h
 *
 * History:
 *	10-apr-1990 (boba)
 *		Make UNIX parser files local.
 *	16-apr-1990 (kimman)
 *		Changing name of 'yaccpar' to 'iyacc.par' because
 *		yaccpar is used by 'iyacc' and there's a special
 *		mkming rule for '.par' files. 
 *	16-apr-1990 (kimman)
 *		Adding an endif as it was taken out by mistake.
**	23-nov-2004 (abbjo03)
**	    Use the same parser on VMS as on UNIX.
 */

	/*
	 * this file has the location of the parser, and the size of the progam
	 * desired. It may also contain definitions to override various
	 * defaults: for example, WORD32 tells yacc that there are at least 32
	 * bits per int. On some systems, notably IBM, the names for the output
	 * files and tempfiles must also be changed.
	 */

#ifndef WORD32
#    define WORD32
#endif


	/* location of the parser text file */
#ifdef vms
# define PARSER		"iyacc.par"
# define LPARSER	"pgparser.parr"
# define NPARSER	"pgparser.parr"
#else
# define PARSER		"iyacc.par"
# define NPARSER	"./yaccparr"
# define LPARSER	"./yaccparr"		/* check here first */
#endif /* vms */

	/* basic size of the Yacc implementation */
#define XHUGE
