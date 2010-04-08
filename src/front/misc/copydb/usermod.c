/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: usermod - utility to modify user defined tables.
**
**  Usage: 
**    	usermod database [-uusername][{table1 table2 table3 ...}] [-noint]
**
**  Description:
**	copydb and generating the modify scripts. 
**	These modify scripts will then be executed and the user tables 
**	Will be modified and the indexes are recreated.
**	-noint uninterrupted modify : Runs the modify even when there are
**	errors.	
**
**  Exit Status:
**	OK	copydb succeeded.
**	FAIL	copydb failed.	
**
**  History:
**	21-jul-2000 (gupsh01)
**		Created.  
**	17-Aug-2000 (gupsh01)
**		Added a flag for checking if the user provided a
**		continue flag. 
** 	15-Feb-2001 (gupsh01)
**		Modified the usage message to show -noint option
**		bug 104002.
**	14-Jun-2001 (gupsh01)
**		Added flags -no_persist to copydb for not 
**		writing create index statement for persistent indexes.
**		Bug 104883.
**	27-Aug-2001 (hanje04)
**	    Initialize err_code to NULL to prevent SEGV in PCdocmdline on 
**	    Linux.
**	05-Sep-2001 (gupsh01)
**	    Fixed the -uusername flag. The username is now passed to the sql
**	    command. Bug 105689. 
**	17-oct-2001 (gupsh01)
**	    Fixed direct reference to the field fname in LOCATION structure.
**	    VMS does not have fname field. Using LOdetail instead. 
**	16-Nov-2001 (gupsh01)
**	    Added check for case when LOdetail does not return a value for
**	    suffix (eg on Unix platform).
**	25-Jun-2002 (guphs01)
**	    Added -no_repmod option in order to allow for the replicator 
**	    catalogs not be modified when running usermod. These catalogs 
**	    should be modified by repmod utility.
**	28-Jun-2002 (gupsh01)
**	    Added new option -repmod which will run the utility repmod
**	    in order to modify the replicator catalogs as well for the 
**	    user. if the database is not replicated then the an error
**	    will be returned to the user.
**	23-jul-2002 (somsa01)
**	    When creating the "copydb" command to execute, surround the
**	    temporary directory name and file name with quotes, as they can
**	    have embedded spaces within them.
**	23-Oct-2002 (gupsh01)
**	    Usermod gives out incorrect usage message when the input 
**	    parameter is just the '-' character.
**	15-nov-2002 (abbjo03)
**	    Change err_code to a structure not a pointer. Also, on VMS the
**	    filename has to be right after the redirection operator.
**	30-jan-2003 (devjo01)
**	    Add '-parallel' to command line passed to copydb, so that non-
**	    persistent secondary indices are recreated in parallel.
**	20-mar-2004 (raodi01) INGCBT#2684, bug 111685
**	   A unique copy.out file is created similar to the unique copy.in and 
**	   both copy.in and copy.out are cleaned up as the control leaves 
**	   usermod
**	19-Jan-2004 (drivi01) 
**	   usermod failed to create two different temp files for copy.in and 
**	   copy.out b/c tloc and loc_out were pointing to the same address.
**	16-feb-2005 (thaju02)
**	   Added -online param (concurrent_updates) for usermod.
**      20-Apr-2006 (hanal04) Bug 116006
**         Allow first non -/+ argument to be used as the DB name instead of
**         assuming the first argument will be the DB name. Strictly speaking
**         the old behaviour is the same as the defined usage, but Linux is
**         allowing "usermod -uUserName DB" to work as expected.
**      31-Oct-2006 (kiria01) b116944
**         Call copydb with -nodependency_check to allow modify to ignore
**         constraint checks.
**      17-Mar-2008 (hanal04) SIR 101117
**         Added -group_tab_idx to call to copydb. Reduces window in which
**         indexes are lost when an error occurs afer a modify but before
**	   the tables indexes have been recreated.
**         Also added -with_comments to prevent all COMMENTs being lost
**         when usermod is run.
**       2-Oct-2009 (hanal04) Bug b122647
**         Add -no_warn flag to call to copydb. usermod does not want the
**         user to be told to run the copy.out
*/

# include <compat.h>
# include <er.h>
# include <pc.h>
# include <st.h>
# include <lo.h>
# include <nm.h>
# include <cm.h>

/*
**   MKMFIN Hints
**
PROGRAM =       usermod

NEEDLIBS =      XFERDBLIB \
                GNLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB \
                COMPATLIB MALLOCLIB

UNDEFS =        II_copyright
*/

void usage();

void 
main(int argc, char *argv[])
{
#define 	MAXBUF	4095
#define 	MAX_NAME  32	

	char 		buf[ MAXBUF+1 ];
	char 		repbuf[ MAXBUF+1 ];
	int		iarg, ibuf, ichr;
	char		*database = NULL;
	char 		username[MAX_NAME] = {""};
	LOCATION	tloc;
	LOCATION        loc_out;
	LOCATION        loc_in;
	char            loc_buf[MAX_LOC + 1];
	char            tmp_buf[MAX_LOC + 1];
	char			otmp_buf[MAX_LOC + 1];
	char		dev[MAX_LOC + 1];
	char		path[MAX_LOC + 1];
	char		fprefix[MAX_LOC + 1];
	char		fsuffix[MAX_LOC + 1];
	char		version[MAX_LOC + 1];
	char            odev[MAX_LOC + 1];
	char            opath[MAX_LOC + 1];
	char            ofprefix[MAX_LOC + 1];
	char            ofsuffix[MAX_LOC + 1];
	char            oversion[MAX_LOC + 1];
	char            *new_filename;
	char		*temploc;
	CL_ERR_DESC 	err_code;
	char 		*p1 = NULL;
	bool		usergiven = FALSE;
	bool		Repmod = FALSE;
	bool		nowait = TRUE;
	char	 	waitflag[3];
	
	
	if (argc < 2) 
	{
		usage();
	    PCexit(FAIL);
	}
	
	/* Get the temporary path location */	
	NMloc(TEMP, PATH, NULL, &tloc);		
	LOcopy(&tloc, tmp_buf, &loc_in);
	LOtos(&tloc, &temploc);
	LOcopy(&tloc, otmp_buf, &loc_out);
	     	
	/* Get the new filename for path location */	
	LOuniq(ERx("usrmod"), ERx("sql"), &loc_in);	
	LOtos(&loc_in, &new_filename);
	
	LOuniq(ERx("usrmod"), ERx("out"), &loc_out);

	/* Get just the filename for copydb code   */	
	LOdetail(&loc_in, dev, path, fprefix, fsuffix, version); 
	if ( *fsuffix !=  '\0' )
	{
	    STcat(fprefix, ".");
	    STcat(fprefix, fsuffix);
	}
       /* Get just the filename for copydb code   */
         LOdetail(&loc_out, odev, opath, ofprefix, ofsuffix,oversion);
		STcat(ofprefix, ".");
		STcat(ofprefix, ofsuffix);
	

	STprintf(buf, 
        ERx("copydb -with_modify -nodependency_check -with_index -with_comments -no_persist -parallel -no_repmod -group_tab_idx -no_warn -d\"%s\" -infile=\"%s\" -outfile=\"%s\""
	), temploc, fprefix,ofprefix);
	ibuf = STlength(buf); 

	for (iarg = 1; (iarg < argc) && (ibuf < MAXBUF); iarg++) 
	{

	    if( STscompare( argv[ iarg ], 1, ERx( "-" ), 1 ) == 0 )
		{
			p1 = argv[ iarg ]; 
			(void) CMnext( p1 );

			if ( (STlength(p1) == 0) || 
			  !(((STscompare( p1, 1, ERx( "u" ), 1 ) == 0 ) ||
			  (STscompare( p1, 5, ERx( "noint" ), 5 ) == 0 ) || 
			  (STscompare( p1, 6, ERx( "repmod" ), 6 ) == 0 ) || 
			  (STscompare( p1, 1, ERx( "w" ), 6 ) == 0 ) ||
			  (STscompare( p1, 6, ERx( "online"), 6 ) == 0 )) &&
			  ( argc > 2 ) ))
			{
				usage(); 
				PCexit(FAIL);
			}
			/*
			** Get the username if the -u flag is passed in
			** with the input
			*/
			
			if (STscompare( p1, 1, ERx( "u" ), 1 ) == 0 )
			{
				STcopy(&argv[iarg][2] , (char *)&username);
				usergiven = TRUE;
			}
			else if (STscompare( p1, 1, ERx( "r" ), 1 ) == 0 )
			{ 	
				Repmod = TRUE;
				continue;
			}
			else if (STscompare( p1, 1, ERx( "w"), 1 ) == 0)	
			{	
				nowait = TRUE;
				continue;
			}	
                }              	
		
        if( STscompare( argv[ iarg ], 1, ERx( "+" ), 1 ) == 0 )
		{
			p1 = argv[ iarg ];
			(void) CMnext( p1 );
			if (STscompare( p1, 1, ERx( "w" ), 1 ) == 0 )
			{
				nowait = FALSE;	
				continue;
			}
			else
			{
				usage();
				PCexit(FAIL); 
			}
		}

        if((database == NULL) &&
           (STscompare( argv[ iarg ], 1, ERx( "+" ), 1 ) != 0) &&
           (STscompare( argv[ iarg ], 1, ERx( "-" ), 1 ) != 0))
        {
            database =  argv[ iarg ];
        }

	buf[ibuf++] = ' ';
		for (ichr = 0; (argv[iarg][ichr] != '\0') && (ibuf < MAXBUF); 
			ichr++, ibuf++)
		{
			buf[ibuf] = argv[iarg][ichr];
		}
		buf[ibuf] = '\0';
	}

	/*
	**	Execute the command.
	*/

	if( PCcmdline((LOCATION *) NULL, buf, PC_WAIT, 
			(LOCATION *) NULL, &err_code) != OK )
	    PCexit(FAIL);

	/*
	**	we should run the sql script 
	**	  sql dbname < new_filename
	**	if -u flag given then run:
	**	  sql -uusername dbname < new_filename	
	*/

	if (usergiven)
	  STprintf(buf, ERx( "sql -u%s -s %s <%s" ),
			username, database, new_filename );
	else
	  STprintf(buf, ERx( "sql -s %s <%s" ),
				database, new_filename );

	/*
	**	Execute the command.
	*/

        if( PCcmdline((LOCATION *) NULL, buf, PC_WAIT, 
                        (LOCATION *) NULL, &err_code) != OK )
        {
            STprintf(buf, ERx(" Warning: Non-persistent objects associated with the base table\n in the failed query may have been lost. Please refer to\n %s\n to determine whether any corrective action is required.\n"), new_filename);
            SIfprintf(stdout, buf);
	    PCexit(FAIL);
        }

	/* Now execute the repmod command if the replicator 
	** modify is also required by the user (-repmod)
	*/
	
	if (Repmod)
	{
		
	  if (nowait)
	    STcopy("-w", waitflag) ;
	  else
	    STcopy("+w", waitflag) ;
		
	  if (usergiven)
	    STprintf(repbuf, ERx( "repmod -u%s %s %s" ), 
		username, database, waitflag );
	  else
	    STprintf(repbuf, ERx( "repmod %s %s" ), 
		database, waitflag );
            	
	
	  if( PCcmdline((LOCATION *) NULL, repbuf, PC_WAIT,
                 (LOCATION *) NULL, &err_code) != OK )
     	    PCexit(FAIL);

	}	
	/*
	**	Delete the location
	*/
	LOdelete(&loc_in); 
	LOdelete(&loc_out);
	PCexit(OK);
}

/*
**  Name: usage : prints out the usage message.
**      21-nov-2000 (gupsh01)
**              Created.
**  	29-May-2002 (xu$we01)
**		Modified usage message by changing "database" to "dbname"
**		to make it consistent with other usage, such as copydb
**		genxml and xmlimport.
*/
void usage()
{
SIfprintf(stdout, ERx(" Error: Incorrect parameters supplied \n"));
SIfprintf(stdout, ERx(" \t Usage: usermod dbname [-uusername][{table1 table2 table3 ...}] [-noint] [-repmod [+w|-w]] [-online] \n"));
}
