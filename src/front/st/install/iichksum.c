/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <si.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <gc.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <te.h>
# include <iplist.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <sys/stat.h>
# include <ip.h>
# include <ipcl.h>
# include <pc.h>

/*
**  Name: iichksum -- Build tool to regenerate the CHECKSUM and SIZE entries
**                    in the release.dat
**                    (Unix version!)
**
**  Entry points:
**	This file contains no externally visible functions.
**
**  Command-line options:
**
**      None.
**    
**  History:
**	16-July-2001 (hanal04) Bug 105159 INGINS 109
**		Created as a workaround to bug 105159 which does not occur
**              under Ingres 2.5 mark 2500 but requires the later ingbuild
**              executable to be added to the original distribution file with
**              the appropriate changes made to the release.dat
**	21-Jun-2001 (hanje04)
**	     Added GLOBALDEFs for expresponse, respfilename and mkresponse to 
**	     unresolved GLOBALREF error from ipfile.o.
**	24-Nov-2009 (frima01) Bug 122490
**	     Added include of pc.h to eliminate gcc 4.3 warnings.
*/

/*
PROGRAM =	iichksum
**
NEEDLIBS =	INSTALLLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB SQLCALIB GCFLIB ADFLIB CUFLIB \
		SQLCALIB COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

GLOBALDEF char releaseManifest[ 32 ];
GLOBALDEF char installDir[ MAX_LOC + 1 ];
GLOBALDEF bool mkresponse = FALSE;   /* TRUE if in 'make response file' mode */
GLOBALDEF bool exresponse = FALSE;   /* TRUE if in 'execute response file' mode
*/
GLOBALDEF char *respfilename = NULL; /* user supplied response file name */

/* 
** dummy fields.  Some of the library functions we link in will refer
** to these fields.  They can be defined as dummies as long as we never
** actually call the functions.
*/

GLOBALDEF char distribDev[ 1 ];		/*** dummy ***/
GLOBALDEF char distribHost[ 1 ];	/*** dummy ***/
GLOBALDEF i4  ver_err_cnt;		/*** dummy ***/
GLOBALDEF bool batchMode;		/*** dummy ***/
GLOBALDEF bool ignoreChecksum=FALSE;	/*** dummy ***/
GLOBALDEF bool eXpress;			/*** dummy ***/
GLOBALDEF bool pmode;			/*** dummy ***/

/* global lists... */

LIST distPkgList;
LIST instPkgList;

static bool silent = FALSE;

# define COND( x ) { if( !silent ) {x;} }

/*
** usage() - display usage message and exit.
*/

void
usage()
{
	SIfprintf( stderr,
		ERx( "\n Usage: iichksum filename\n" ) );
        PCexit( FAIL );
}

/*
** buildrel's main()
*/

main(i4 argc, char *argv[])
{
	STATUS rtn;
	char *cp;
	i4 argvx;
	char cmd[ MAX_LOC * 2 ];
	char userName[ 64 ];
	LOCATION loc;
	char lobuf[ MAX_LOC + 1 ];
        FILBLK fil;

	MEadvise( ME_INGRES_ALLOC );

        if (argc != 2)
	    usage();

        STcopy( argv[1], lobuf);
	LOfroms( FILENAME, lobuf, &loc );
	if( OK != ip_file_info_comp( &loc, &fil.size,
	    &fil.checksum, FALSE ) )
        {

	    SIfprintf( stderr, ERx( "FILE\t%s\t\t** NOT FOUND **\n" ), lobuf);
        }
	else
	{
	    SIfprintf(stdout, ERx("FILE\t%s\n\tCHECKSUM\t%d\n\tSIZE\t\t%d\n"),
		lobuf, fil.checksum, fil.size);

	}
	PCexit( OK );
}

/*
** message display functions; called from various library routines.
*/

void ip_display_transient_msg( char *msg )
{
	COND( SIprintf( ERx( "%s\n" ), msg ) );
}

void ip_display_msg( char *msg )
{
	COND( SIprintf( ERx( "%s\n" ), msg ) );
}

/*
** ip_forms() - dummy stub
*/

void
ip_forms(bool yesno)
{
	return;
}

/*
** ip_dep_popup() - dummy stub.
*/

bool
ip_dep_popup(PKGBLK *pkg1, PKGBLK *pkg2, char *oldvers)
{
	return( TRUE );
}


