/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/
/*
** Ming hints
NEEDLIBS = COMPAT
*/


#include <compat.h>
#include <systypes.h>
#include <sys/acct.h>
#include <sys/param.h>
#include <fcntl.h>
#include <me.h>
#include <lo.h>
#include <nm.h>
#include <st.h>
#include <pc.h>
#include <si.h>


/* 
  Machine level HZ is 50. defined in sys/machdep/param.h 
  User level is 100 in sys/param.h.  50 is the value
  to use.
*/

/**
** Name: acctconv.c - utility to convert iiacct to native format
**
** Description:
**	This program reads the standard format INGRES session accounting
**	file (iiacct) and creates an equivalent file in the native acct
**	format of the current platform, performing any and all appropriate
**	conversions as necessary.  Usage is as follows:
**
**	    convacct [source_file] dest_file
**
**	where source_file is the iiacct file to be converted and dest_file
**	is the resulting converted copy of the iiacct file.  If source_file
**	is not given, the utility assumes that the iiacct input file is
**	$II_CONFIG/iiacct, as that is the only allowable working location.
**
**	Any and all errors encountered are fatal.  If an error does occur,
**	the cause of the error must be corrected and the program rerun.
**	The possible errors are:
**
**	    1. Incorrect usage (arguments)
**	    2. Unable to locate the iiacct file
**	    3. Unable to open one of the files
**	    4. I/O errors
**
**	The routines defined in this file are:
**
**	comp_t_uncompress() - converts a comp_t to a time_t, the inverse of
**	    CS_compress().
**	format_acct() - converts a record from the iiacct format to the
**	    native format, the heart of the utility.
**	error_exit() - prints an error message and exits with the OS exit
**	    status set to failure.
**	main() - the main routine.
**
**	Please review the notes in the format_acct() routine for more
**	information about porting related issues.
**
** History:
**	21-oct-91 (kirke)
**	    Created.
**      24-oct-91 (hhsiung)
**          Add for bu2/bu3.
**	 5-feb-92 (sweeney)
**	    Add dr6_us5 and dra_us5.
**	27-apr-92 (deastman)
**	    Add pym_us5 support.
**      16-apr-92 (johnr)
**          Piggybacked ds3_ulx for ds3_osf
**	29-apr-92 (purusho)
**	    Added support for amd_us5
**      01-apr-92 (pearl)
**          Add port specific changes for nc5_us5.  Set ac_flag to ' ', since
**          acctcom expects this as a delimiter at the beginning of each
**          record.
**	05-jun-92 (kevinm)
**	    Add dg8_us5 support.
**	22-jul-92 (sweeney)
**	    apl_us5 is a 60 tick machine.
**	17-aug-92 (peterk)
**	    Added defines for su4_us5 based on bonobo's su4_sol.
**      12-oct-92 (rkumar)
**          Added sqs_ptx in the format_acct routine to the list 
**          hp3_us5, hp8_us5 etc.,
**	27-jan-93 (pghosh)
**	    Modified nc5_us5 to nc4_us5. Till date nc5_us5 & nc4_us5 were
**	    being used for the same port. This is to clean up the mess so
**	    that later on no confusion is created.
**	07-may-93 (shelby)
**	    Added define for t15_us5.
**	23-jun-93 (pauland)
**	    Added usl_us5 as per dra_us5.
**	10-sep-93 (kchin)
**	    Added entries for axp_osf.
**      20-dec-93 (jowu)
**          Added support for amd_u54
**	11-Jan-93 (bconnell)
**	    Add port specific changes for m88_us5
**	11-jan-94(rcs)
**	    Added dol_us5 as per dg8_us5
**	05-Dec-1994 (kch)
**		Documented convex change: added cvx_u42. 
**         	Previous change uncommented. 
**	02-Feb-1995 (ramra01)
**	   The struct acctp is not defined. It is acct, test were done to 
**	   using the HP box to check the utility
**	07-Apr-1995 (abowler)
**	   Bug 60613 - pulled fix from 6.4 (prida01) to add ac_flag = ' '
**	   to seperate records for dr6_us5, su4_us5, usl_us5.
**	05-May-1995 (smiba01)
**	   Added support for dr6_ues (same as dr6_us5).
**	22-jun-95 (allmi01)
**	   Added support for dgi_us5 (DG-UX on Intel platforms) following dg8_us5.
**	25-jul-95 (morayf)
**	   Added support for sos_us5 (like odt_us5).
**	21-jul-95 (allst01)
**	   Added su4_cmw as for su4_u42
**	10-nov-1995 (murf)
**		Added release identifier sui_us5 to areas specific to su4_us5
**		in order to build all areas for Solaris on Intel (execpt CS).
**	24/jan-96 (prida01)
**		Sun Solaris is now using convacct. We need to convert to the 
**		correct HZ whatever they are. Because is information the back end
**		passes out assumes 60 ticks per second. Add su4_us5 define.
**	6-feb-95 (stephenb)
**	    Fix smiba01's integration for allst01, he forgot to add
**	    continuation characher in defined list when resolving
**	    cross-integration conflicts, so module would not build.
**	14-dec-95 (morayf)
**	   Added support for SNI RMx00 - rmx_us5 (like pym_us5).
**	28-jul-1997 (walro03)
**	    Added Tandem NonStop (ts2_us5), like pym_us5.
**      28-Oct-96 (mosjo01 & linke01)
**         Added rs4_us5 for define GOT_FORMAT
**	18-feb-98 (toumi01)
**	    Added Linux (lnx_us5).  It doesn't have a typedef for comp_t
**	    in sys/acct.h, nor does the accounting structure contain the
**	    fields ac_io and ac_rw;
**	22-Mar-1998 (kosma01)
**	   Added SGI IRIX, IRIX64 (sgi_us5), like ts2_us5.
**	02-jul-99 (toumi01)
**	    For Linux glibc 2.1 skip define for comp_t, which is now properly
**	    defined in the system headers.  Setting of _GNU_SOURCE is unique
**	    to the glibc 2.1 build, so key off of that.
**	10-may-1999 (walro03)
**	    Remove obsolete version string amd_us5, apl_us5, bu2_us5, bu3_us5,
**	    cvx_u42, dr6_ues, dra_us5, ds3_osf, odt_us5, vax_ulx.
**      03-jul-1999 (podni01)
**         Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5
**	06-oct-99 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      28-Oct-99 (hweho01)
**          defined GOT_FORMAT for ris_u64 platform.
**	16-aug-2000 (somsa01)
**	    Added support for ibm_lnx.
**	24-Aug-2000 (hanje04)
**	    Added settings for Alpha Linux (axp_lnx) port buy matching Intel
**	    Linux entries (int_lnx), except for typedef of comp_t. This is not
**	    defined for Alpha Linux so we use _GNU_SOURCE linux defn.
**	29-jul-2001 (toumi01)
**	    For i64_aix piggyback the settings for the other AIX platforms.
**	08-May-2002 (hanje04)
**	    Add Itanium Linux to linux defs.
**	07-Oct-2002 (hanje04)
**	    For AMD x86-64 Linux port, tidy up linux build and replace
**	    various linux defines with generic LNX
**	23-Mar-2005 (bonro01)
**	    Added support for Solaris AMD64 a64_sol
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	30-Jun-2009 (kschendel) SIR 122138
**	    Use sparc_sol, any_hpux, any_aix symbols as needed.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	   Also remove local declaration of errno, shouldn't be there.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
**/


/*}
** Name: iiacct - the standard INGRES session accounting file format,
**	lifted directly from cshl.c.
*/

# if ((defined(int_lnx) || defined(int_rpl) || defined(ibm_lnx)) && \
      !defined(_GNU_SOURCE)) || defined(axp_lnx)
/* Linux doesn't have comp_t in sys/acct.h */
typedef time_t comp_t;
# endif /* Linux */

struct iiacct
{
    char    ac_flag;	    /* Unused */
    char    ac_stat;	    /* Unused */
    unsigned short ac_uid;  /* Session's OS real uid */
    unsigned short ac_gid;  /* Unused */
    short   ac_tty;         /* Unused */
    long    ac_btime;       /* Session start time */
    comp_t  ac_utime;       /* Session CPU time (secs*60) */
    comp_t  ac_stime;       /* Unused */
    comp_t  ac_etime;       /* Session elapsed time (secs*60) */
    comp_t  ac_mem;         /* average memory usage (unused) */
    comp_t  ac_io;          /* I/O between FE and BE */
    comp_t  ac_rw;          /* I/O to disk by BE */
    char    ac_comm[8];     /* "IIDBMS" */
};


/* defines for the various errors */
#define NO_ERROR	0
#define USAGE_ERROR	1
#define LOCATE_ERROR	2
#define OPEN_ERROR	3
#define READ_ERROR	4
#define WRITE_ERROR	5


char *
error_text[] =
{
    "No Errors",
    "Usage: convacct [source_file] dest_file",
    "Unable to locate iiacct file",
    "Unable to open file %s",
    "Error reading from file %s",
    "Error writing to file %s"
};


/*{
** Name: comp_t_uncompress() - convert a comp_t into a time_t, the inverse
**	of CS_compress()
**
** Inputs:
**	value - the comp_t to be converted.  The comp_t type is a 3 bit base
**	    8 exponent, 13 bit fraction "floating point" number.  comp_t
**	    numbers have approximately 3 decimal digits of precision.
**
** Returns:
**	the converted value as a time_t (a long on most systems)
*/

time_t
comp_t_uncompress(value)
register comp_t value;
{
    register time_t result;
    register int exponent;

    exponent = (value & 0x0000E000) >> 13;	/* extract the exponent */
    result = value & 0x1FFF;			/* extract the mantissa */
    result <<= (3 * exponent);

    return(result);

} /* comp_t_uncompress() */


/*{
** Name: format_acct() - convert an iiacct record to the native acct format
**
** Description:
**	This routine performs the actual conversion between the iiacct format
**	and the native acct format.  A case for each platform must be defined.
**
**	These seven fields in the iiacct structure contain useful information:
**
**	unsigned short ac_uid;   Session's OS real uid
**	long    ac_btime;        Session start time (secs since 1/1/70)
**	comp_t  ac_utime;        Session CPU time (secs*60)
**	comp_t  ac_etime;        Session elapsed time (secs*60)
**	comp_t  ac_io;           I/O count between FE and BE
**	comp_t  ac_rw;           I/O count to disk by BE
**	char    ac_comm[8];      command name - always "IIDBMS"
**
**	There are two areas that are of concern to the porting engineer:
**
**	1.  Differences in the iiacct and acct structures.  For example,
**	differences in the type of various structure elements, or different
**	names for equivalent structure elements.
**
**	2.  The units of measurement for the various structure elements.  In
**	particular ac_utime and ac_etime, which are in units of SUN4 clock
**	ticks (secs * 60) in the iiacct structure, may need to be converted
**	to some other units.
**
**	Look at some of the existing cases below if you are unsure about your
**	platform.
**
** Inputs:
**	input_rec - address of an iiacct struct
**	output_rec - address of a native acct struct to be filled in
**
** Outputs:
**	output_rec - address of the filled in acct struct
**
** Returns:
**	Nothing
*/

VOID
format_acct(input_rec, output_rec)
struct iiacct *input_rec;
struct acct *output_rec;
{
    MEfill(sizeof(struct acct), '\0', output_rec);

    /* fill in the applicable fields */

# if defined(sparc_sol) || defined(a64_sol) || defined(LNX) || defined(OSX)
# define GOT_FORMAT
# if defined(sparc_sol) || defined(a64_sol)
        /* sun solaris expects this to delimit records */
        output_rec->ac_flag = ' ';
# endif
    /* All fields match, clock ticks == 60/sec */
    output_rec->ac_uid = input_rec->ac_uid;
    output_rec->ac_btime = input_rec->ac_btime;
    output_rec->ac_utime = input_rec->ac_utime;
    output_rec->ac_etime = input_rec->ac_etime;
# if ! ( defined(LNX) || defined(OSX) )
    output_rec->ac_io = input_rec->ac_io;
    output_rec->ac_rw = input_rec->ac_rw;
# endif /* !Linux */
    STcopy(input_rec->ac_comm, output_rec->ac_comm);
# endif /* sol linux osx */

# if defined(any_hpux) || defined(usl_us5) || defined(sgi_us5)
# define GOT_FORMAT
    /* All fields match, clock ticks != 60/sec */
# if defined(usl_us5)
    /* Set ac_flag to ' ' to delimit records in iiacct file */
    output_rec->ac_flag = ' ';
# endif /* usl_us5 */
    /* It appears this flag is needed for svr4 to delimit record */
    output_rec->ac_uid = input_rec->ac_uid;
    output_rec->ac_btime = input_rec->ac_btime;
    output_rec->ac_utime = CS_compress(
	((comp_t_uncompress(input_rec->ac_utime) * HZ) + 30) / 60);
    output_rec->ac_etime = CS_compress(
	((comp_t_uncompress(input_rec->ac_etime) * HZ) + 30) / 60);
    output_rec->ac_io = input_rec->ac_io;
    output_rec->ac_rw = input_rec->ac_rw;
    STcopy(input_rec->ac_comm, output_rec->ac_comm);
# endif /* hpux usl */

# if defined(any_aix)
# define GOT_FORMAT
    /*
    ** RS6000 uses AHZ for clock ticks.  Although ac_io and ac_rw are
    ** integral values, they are expressed in AHZ units.  ac_etime is
    ** apparently in 100 * AHZ units.
    */
    output_rec->ac_uid = input_rec->ac_uid;
    output_rec->ac_btime = input_rec->ac_btime;
    output_rec->ac_utime = CS_compress(
	((comp_t_uncompress(input_rec->ac_utime) * AHZ) + 30) / 60);
    output_rec->ac_etime = CS_compress(
	((comp_t_uncompress(input_rec->ac_etime) * AHZ * 100) + 30) / 60);
    output_rec->ac_io = CS_compress(comp_t_uncompress(input_rec->ac_io) * AHZ);
    output_rec->ac_rw = CS_compress(comp_t_uncompress(input_rec->ac_rw) * AHZ);
    STcopy(input_rec->ac_comm, output_rec->ac_comm);
# endif /* aix */

# ifdef axp_osf
# define GOT_FORMAT
    output_rec->ac_uid = input_rec->ac_uid;
    output_rec->ac_btime = input_rec->ac_btime;
    output_rec->ac_utime = CS_compress(
        ((comp_t_uncompress(input_rec->ac_utime) * AHZ) + 30) / 60);
    output_rec->ac_etime = CS_compress(
        ((comp_t_uncompress(input_rec->ac_etime) * AHZ) + 30) / 60);
    output_rec->ac_io = CS_compress(comp_t_uncompress(input_rec->ac_io) * AHZ);
    STcopy(input_rec->ac_comm, output_rec->ac_comm);
# endif /* axp_osf */

# ifndef GOT_FORMAT
	# error: You MUST add a format conversion for your box
# endif /* GOT_FORMAT */

} /* format_acct() */


/*{
** Name: error_exit() - print an error message and exit.
**
** Description:
**	Print an error message and exit with the OS exit status set to
**	failure.  The error message is output to stderr.  We use perror()
**	to try and give some indication as to the cause of the error if
**	it is system call related.
**
** Inputs:
**	error_number - the error number, an index into the error_text[] array.
**	arg1 - an optional argument for the error_text message.
**
** Returns:
**	Does not return.
*/

VOID
error_exit(error_number, arg1)
int error_number;
char *arg1;
{
    SIfprintf(stderr, "convacct: ");
    SIfprintf(stderr, error_text[error_number], arg1);
    if (errno)
	perror(" ");
    else
	SIfprintf(stderr, "\n");
    PCexit(FAIL);
} /* error_exit() */


/*{
** Name: main() - the main routine
**
** Description:
**	Analyzes the command line arguments, opens the appropriate files,
**	reads the input, converts the input, writes the output, and then
**	closes the files.
**
** Inputs:
**	Command line arguments specifying the source and destination files.
**	The source file is optional while the destination file is mandatory.
**
** Returns:
**	OS exit status of OK on normal exit, exits through error_exit()
**	when error occurs.
*/

int
main(argc, argv)
int argc;
char *argv[];
{
    LOCATION loc;
    char *input_fname;
    char *output_fname;
    int input_fd;
    int output_fd;
    struct iiacct input_rec;
    struct acct output_rec;
    int nbytes;
    int nrecs;

    errno = 0;

    /* get command line arguments */
    if (argc < 2 || argc > 3)
	error_exit(USAGE_ERROR, NULL);

    if (argc == 3)
    {
	input_fname = argv[1];
    }
    else
    {
	if (NMloc(ADMIN, FILENAME, "iiacct", &loc) == OK)
	    LOtos(&loc, &input_fname);
	else
	    error_exit(LOCATE_ERROR, NULL);
    }

    output_fname = argv[argc - 1];

    /* open input file */
    if ((input_fd = open(input_fname, O_RDONLY, 0)) < 0)
	error_exit(OPEN_ERROR, input_fname);

    /* open output file */
    if ((output_fd =
	open(output_fname, O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0)
	error_exit(OPEN_ERROR, output_fname);

    /* convert the file */
    nrecs = 0;
    while ((nbytes = read(input_fd, &input_rec, sizeof(input_rec))) != 0)
    {
	if (nbytes == -1)
	    error_exit(READ_ERROR, input_fname);
	format_acct(&input_rec, &output_rec);
	if ((nbytes = write(output_fd, &output_rec, sizeof(output_rec))) !=
	    sizeof(output_rec))
	{
	    if (nbytes != -1)
		/* partial write - try it again to set errno */
		write(output_fd, &output_rec, sizeof(output_rec));
	    error_exit(WRITE_ERROR, output_fname);
	}
	nrecs++;
    }

    /* close the files */
    close(input_fd);
    close(output_fd);

# ifdef xDEBUG
    SIprintf("sizeof(iiacct) = %d\n", sizeof(struct iiacct));
    SIprintf("sizeof(acct) = %d\n", sizeof(struct acct));
    SIprintf("number of records = %d\n", nrecs);
# endif /* xDEBUG */

    PCexit(OK);

} /* main() */
