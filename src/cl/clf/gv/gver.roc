# include	<compat.h>
# include	<gl.h>
# include	<gv.h>
 
/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
*/
/*
** gver.roc -- INGRES Global Constants Definitions (mostly copyright notices)
**
**	Generic banner formats:
**
**		(Intro)	Ingres <O/S> Version <... (..)> login <time>
**			Copyright (c) 2004 Ingres Corporation
**
**		(Exit)	Ingres Version <... (..)> logout <time>
**
LIBRARY = IMPCOMPATLIBDATA
**
**    History :
**      18-apr-1991 (fpang)
**          Changed Star_Version from GV_VER to GV_STAR_VER.
**      03-Jan-96 (fanra01)
**          Added GLOBALDEFs for DLL import variable name decoration.
**	18-oct-96 (mcgem01)
**	    Text of copyright message changed to reference Computer
**	    Associates.   Release date for 2.0 estimated as Dec 31st '96.
**	15-jun-97 (mcgem01)
**	    Update copyright notice to reflect the correct year.
**	01-jun-1998 (hanch04)
**	    Update copyright notice to reflect the correct year.
**      11-may-99 (hanch04)
**          Update the copyright notice.
**      20-jan-2000 (hanch04)
**          Update the copyright notice.
**      30-jan-2002 (hanch04)
**          Update the copyright notice.
**	21-Jun-2002 (hanje04)
**	    Update the copyright notice.
**	08-may-2003 (abbjo03)
**	    Remove unused Star version variables.
**  	26-Jan-2004 (fanra01)
**	    SIR 111718
**	    Add definition and initialization of ii_ver structure.
**  	10-Feb-2004 (fanra01)
**	    SIR 111718
**	    Renamed II_VERSION type to ING_VERSION
**	13-Feb-2004 (sheco02)
**	    Update the copyright notice and line up the previous comments.
**	05-Jan-2005 (hanje04)
**	    Update copyright and Reldate notices
**	08-Aug-2005 (drivi01)
**	    Updated reldate notice.
**	20-Dec-2006 (drivi01)
**	    Updated reldate notice.
**	08-Jan-2007 (drivi01)
**	    Update reldate notice and copyright.
**	03-Jan-2008 (drivi01)
**	    Update reldate notice and copyright.
**	16-Jun-2008 (drivi01)
**	    Update approximate release date and name of the release.
**	12-Feb-2009 (bonro01)
**	    Update reldate notice and copyright.
**	12-Feb-2010 (frima01) SIR 123269
**	    Update reldate notice and copyright to 2010.
*/
 
/*
**	The wording of the following copyright message is critical.
**	Please don't play around with it without getting advice from
**	the legal department.
**
**	The string may be too long to compile on most systems.  I made
**	it into one string instead of several, because otherwise the
**	linker could spread it out all over the executable image.  We want
**	the notice to be readable from a dump of the executable.  I'm not
**	sure how to deal with this for compilers that don't allow long
**	strings.
**
**	Also, some compilers may not allow backslash continuations of
**	quoted strings.  Again, I did it this way so it could all be in
**	on string.
**
**	The copyright notice variable is defined as a pointer instead of
**	an array.  This forces the actual notices to become string constants.
**	VMS links these constants at the beginning of the executable file.
**	The closer the notice is to the beginning of the file, the better.
**	On UNIX this doesn't matter, but I left it the same anyway to avoid
**	confusion.
**
**	The latter date in the copyright message should be the date of
**	last major revision.
*/

GLOBALDEF char    *II_copyright = "Copyright (C) 1991, 2010 Ingres Corporation\
 All Rights Reserved.";

GLOBALDEF char	Reldate[] = "30-September-2010";/* Target release for Ingres 10 GA */
 
/*
 * GV_* values are set in CLHDR!gv.h
 */
 
GLOBALDEF char	Version[] = GV_VER;		/* e.g. 5.0/01 (apo.u42/01) */
GLOBALDEF char	Env_ID[] = GV_ENV;		/* e.g. UNIX System V */

GLOBALDEF ING_VERSION ii_ver = {
    GV_MAJOR,
    GV_MINOR,
    GV_GENLEVEL,
    GV_BLDLEVEL,
    GV_BYTE_TYPE,
    GV_HW,
    GV_OS,
    GV_BLDINC
};
