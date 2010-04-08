/*
**  Copyright (c) 2004 Ingres Corporation
**
**  Name: xmlimport - utility to import the xml data file into ingres.
**
**  Usage: 
**      xmlimport database [-uusername] [-debug] xmlfile 
**
**  Description:
**	This utility will import any xml data file containing ingres 
**	table data and conforming to ingres dtd into ingres database.
**	The user may have several table definitions and index definitions
**      in the xml file and all these tables and indexes will be created in
**      the provided data base and the data will be uploaded. 
** 	Internally this will run impxml executable, which using the Xerces
**	SAX parser, parses the xml file to generate an sql script from the 
**	metadata information in the xml file and respective data files 
**	for each table data. 
**	xmlimport will then run the sql script to create tables and 
**	upload the data from the data files.
**	Files are created in temp directory and deleted when the 
**	script has been run, except when the -debug flag is provided.
**
**  Exit Status:
**	OK	xmlimport succeeded.
**	FAIL	xmlimport failed.	
**
**  History:
**	01-jun-2001 (gupsh01)
**		Created.  
**	27-Aug-2001 (hanje04)
**	    Initialize err_code to null to stop SEGV in PCdocmdline on Linux
**	05-sep-2001 (gupsh01)
**	    Added code to handle input parameters using utexe.def.
**	29-Oct-2001 (hanch04)
**	    Pass the username , password and groupid to the sql command
**	30-May-2002 (gupsh01)
**	    Prompt the user for an xml filename if not provided on the command 
**	    line. 
**	10-oct-2003 (schka70/abbjo03)
**	    Fix temp directory handling so that it works on VMS; err_code has
**	    to point to something on VMS; straighten out garbled location
**	    string usage.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries.
**	27-May-2005 (thaju02)
**	    Pass encoding as a param to impxml command. (B114632)
**      18-Feb-2009 (huazh01)
**          we now use a random number to generate the temp directory name. 
**          On Linux, PCpid() is defined as getpgrp(), meaning "pid" 
**          returned by PCunique() may not be unique, causing duplicated temp 
**          directory name gets generated and leads to E_XM0008 error. 
**          (b121678)
*/

# include <compat.h>
# include <gl.h>
# include <mh.h>
# include <iicommon.h>
# include <er.h>
# include <ex.h>
# include <fe.h>
# include <uigdata.h>
# include <ug.h>
# include <ui.h>
# include <pc.h>
# include <st.h>
# include <lo.h>
# include <nm.h>
# include <cm.h>
# include <ug.h>
# include "erxml.h"

/*
**      MKMFIN Hints
**
PROGRAM =       xmlimport 

NEEDLIBS =      XFERDBLIB \
                GNLIB SHFRAMELIB SHQLIB SHCOMPATLIB SHEMBEDLIB

UNDEFS =        II_copyright
*/

void usage();

int
main(int argc, char *argv[])
{
#define 	MAXBUF	4095

	char 	buf[ MAXBUF+1 ];
	int	iarg, ibuf, ichr;
	bool	debug = FALSE;
        CL_ERR_DESC     err_code;
        char		*p1 = NULL;
        char		pid[MAXBUF];
        char    	*database = ERx("");
        char    	*user = ERx("");
        char    	*xmlfile = ERx("");
        char    	sql_fname[LO_NM_LEN + 1];
        char    	*work_dir = NULL;
        char    	directory[MAX_LOC + 1]; 
        char    	*tmp_dir = NULL;
        char		tmp_buf[MAX_LOC + 1];
        char		subdir_buf[MAX_LOC + 1];
        char		sql_loc_buf[MAX_LOC + 1];
	char		*progname; 
        LOCATION    tmp_dir_loc;
        LOCATION    tmp_subdir_loc;
        LOCATION    tmp_buff_loc;
        LOCATION    curr_loc;
        LOCATION    sql_file_loc;
	char        *password = ERx("");
	char        *groupid = ERx("");
	ARGRET  rarg;
        i4      pos;
	LOCATION    xmlfile_loc;
	FILE	    *xmlfile_read;
	STATUS	    stat = FAIL;
	char	    dbuf[256];
	char	    encode[32];
        u_i4        tmppid;
        TM_STAMP    tm_stamp;


	/* Tell EX this is an ingres tool. */
   	(void) EXsetclient(EX_INGRES_TOOL);

	/* Call IIUGinit to initialize character set attribute table */
	if ( IIUGinit() != OK)
	    PCexit(FAIL);

	progname = ERget(F_XM0006_IMPXML);
	FEcopyright(progname, ERx("2001")); 

	/*
	** Get arguments from command line
	*/

	/* required parameters */

	if (FEutaopen(argc, argv, ERx("xmlimport")) != OK)
          PCexit(FAIL);

	/* database name is required */
	if (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) != OK)
          PCexit(FAIL);
	database = rarg.dat.name;

        if (FEutaget(ERx("xmlfile"), 0, FARG_PROMPT, &rarg, &pos) != OK)
            PCexit(FAIL);
        xmlfile = rarg.dat.name;

        if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
            user = rarg.dat.name;

        if (FEutaget(ERx("password"), 0, FARG_FAIL, &rarg, &pos) == OK)
        {
            char *IIUIpassword();

            if ((password = IIUIpassword(ERx("-P"))) == NULL)
            {
                FEutaerr(BADARG, 1, ERx(""));
                PCexit(FAIL);
            }
        }

        if (FEutaget(ERx("groupid"), 0, FARG_FAIL, &rarg, &pos) == OK)
          groupid = rarg.dat.name;

        if (FEutaget(ERx("debug"), 0, FARG_FAIL, &rarg, &pos) == OK)
	  debug = TRUE;

        ibuf = STlength(buf); 

        /* b121678: pid is no longer based on process id, but it's
        ** a random number instead.
        */
        PCpid(&tmppid);
        TMget_stamp(&tm_stamp);
        MHsrand2(tmppid * tm_stamp.tms_usec);
        STprintf(pid, "%x", MHrand2());

#ifdef xDEBUG
        SIprintf(" the pid is: %s \n", pid);
#endif

        /* create the sql file */
	/* Avoid a name like "foo.xml.sql" on VMS, use pid.sql instead */
        STcopy(pid, sql_fname); 
        STcat(sql_fname, ".sql");

        /* 
        ** create in the temp location a directory 
        ** with the name pid. set this directory 
        ** as the working directory for impxml
        */

      NMloc (TEMP, PATH, NULL, &tmp_dir_loc);	
      /* make a location for TMP loc */
      /* print location name */
      LOcopy (&tmp_dir_loc, tmp_buf, &tmp_buff_loc);
      LOtos (&tmp_buff_loc, &tmp_dir);

#ifdef xDEBUG
      SIprintf ("temploc: %s \n", tmp_dir);
#endif

      /* make a subdir location with filename, pid */
      STcopy (pid, subdir_buf);				
      /* Initialize result loc so that everyone is happy */
      LOcopy (&tmp_dir_loc, sql_loc_buf, &sql_file_loc); 
      /* Generate location for temp subdirectory */
      if (LOfaddpath (&tmp_dir_loc, subdir_buf, &sql_file_loc) != OK)
      {
	 IIUGerr(E_XM0007_Locname_Failed, UG_ERR_FATAL, 2, tmp_dir, subdir_buf);
	 /* NOTREACHED */
      }

      /* print the location name */
      LOcopy (&sql_file_loc, tmp_buf, &tmp_buff_loc);    
      LOtos (&tmp_buff_loc, &work_dir);

#ifdef xDEBUG
	SIprintf ("work dir loc: %s \n", work_dir);
#endif

      /* create the subdir */
      if (LOcreate (&sql_file_loc) != OK) {
	 IIUGerr(E_XM0008_Create_Temp_Dir, UG_ERR_ERROR, 1, work_dir);
	 PCexit(FAIL);
      }

      STcopy(work_dir, directory);

#ifdef xDEBUG
      SIprintf ("sql file name: %s \n", sql_fname);
      SIprintf ("xml file name: %s \n", xmlfile);
#endif

      /* Execute the command impxml */	
      STprintf (buf, ERx( "impxml -d=\"%s\" -o=\"%s\" " ),  directory, sql_fname);

      /* encoding? */
      if ( (LOfroms(PATH & FILENAME, xmlfile, &xmlfile_loc) != OK)
          ||
	  (SIopen(&xmlfile_loc, "r", &xmlfile_read) != OK)
	  ||
	  (xmlfile_read == NULL)
	 )
      {
	  IIUGerr(E_XM0009_Cannot_Open_File, UG_ERR_ERROR, 1, xmlfile);
          PCexit(FAIL);
      }

      /* scan XML declaration for encoding, if any */
      if (stat = SIgetrec(dbuf, sizeof(dbuf) - 1, xmlfile_read) == OK)
      {
	  char 	*d = dbuf;
	  i4	i = 0;

	  for (d = dbuf; d != (dbuf + sizeof(dbuf)); d++)
	  {
	      if (MEcmp(d, ERx("encoding="), sizeof(ERx("encoding=")) - 1) == 0)
	      {
		  d += sizeof(ERx("encoding="));
		  while (MEcmp (d, "\'", sizeof(char)) && MEcmp(d, "\"", sizeof(char)))
		      MEcopy(d++, sizeof(char), &encode[i++]);
		  encode[i++] = MIN_CHAR;
		  encode[i] = EOS;
		  STcat(buf, ERx("-x="));
		  STcat(buf, encode);
		  break;
	      }
	  }
      }
      else if (stat != ENDFILE)
      {
          /* unable to read file, report error */
	  IIUGerr(E_XM000A_Cannot_Read_File, UG_ERR_ERROR, 1, xmlfile);
	  PCexit(FAIL);
      }

      stat = SIclose(xmlfile_read);

      STcat(buf, xmlfile);

#ifdef xDEBUG
      SIprintf ( " query send: %s \n", buf);
#endif

      /* 	Execute the command.  
      */

      if( PCcmdline((LOCATION *) NULL, buf, PC_WAIT, 
  		(LOCATION *)NULL, &err_code) != OK )
      {
	if (!debug)
	    LOdelete(&sql_file_loc);
	PCexit(FAIL);
      }

      /*
      **	we should run the sql script 
      **	sql dbname < new_filename
      */
      
      /* save the current location */
      LOcopy(&sql_file_loc, tmp_buf, &curr_loc);

      /* make a full location path to the location first */
      LOfroms(FILENAME, sql_fname, &tmp_buff_loc);
      LOstfile(&tmp_buff_loc, &curr_loc);
      LOcopy (&curr_loc, tmp_buf, &tmp_buff_loc);
      LOtos (&tmp_buff_loc, &tmp_dir);

#ifdef xDEBUG
      SIprintf ("sql file is: %s \n", tmp_dir);
#endif

      /* No space between < and input file for VMS */
      STprintf(buf, ERx( "sql -s %s %s %s %s <%s" ),
			database, user, password, groupid, tmp_dir);
#ifdef xDEBUG
      SIprintf (" query send: %s \n", buf);
#endif

      /*
      **	Execute the command.
      */

      if( PCcmdline((LOCATION *) NULL, buf, PC_WAIT, 
              (LOCATION *)NULL, &err_code) != OK )
      {
	if (!debug)
	    LOdelete(&sql_file_loc);
        PCexit(FAIL); 
      }

      /*
      **    Delete the location
      */
      if (!debug)
	LOdelete(&sql_file_loc);

      PCexit(OK);
}
