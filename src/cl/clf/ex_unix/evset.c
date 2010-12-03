#include <compat.h>
#include <ex.h>
#include <st.h>
#include <si.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <evset.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <tr.h>
#include <cv.h>


#define EVSET_MAX_ID 999
#define NEXT_ID(id) (id==EVSET_MAX_ID?0:id+1)

/*
**Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
**
**  Name: evset - Evidence set handling routines
**
**  Description:
**     This module supports the handling of evidence sets all manipulation of
**     evidence sets and their contents should be done through these interfaces
**
**     The following interfaces are supported:-
**
**     EVSetCreate         - Create a new evidence set
**     EVSetDelete         - Delete an evidence set
**     EVSetExport         - Export the contents of an evidence set to a file
**     EVSetImport         - Import an evidence set 
**     EVSetList           - List details of existing evidence sets
**     EVSetCreateFile     - Create a new file in an evidence set
**     EVSetDeleteFile     - Delete a file from an evidence set
**     EVSetFileList       - List files in an evidence set
**     EVSetLookupVar      - Lookup variables in CONTENTS file
**
**  History:
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Dec-2003 (horda03) Bug 111860/INGSRV2722
**          Enable Evidence Sets on all Unix platforms. Need to replace
**          STcmp with STbcompare, as only wish to compare the first 5
**          characters (not the whole string).
**	15-Jun-2004 (schka24)
**	    Safe env variable handling.
**	23-Sep-2005 (hanje04)
**	    Fix prototype of handle to match function.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for CVal definition.
**	17-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
*/

/*[
**  Name: CONTENTS file format
**
**  Description:
**      In each evidence set subdirectory is a file called CONTENTS which
**      is used to hold all administration information about the evidence set
**
**      This has the following format:-
**
**            EVSET_DETAILS structure
**                This is at the start of the file and contains basic details
**                about the evidence set. This is written by the EVSetCreate
**                routine when the evidence set is created and can be accessed
**                via the EVSetList interface
**            EVSET_ENTRY structures
**                There is one of these for each file or variable defined in
**                the evidence set. They follow directly after the EVSET_DETAILS
**                structure.
**
**                The type of each entry is defined by the type field:-
**
**                EVSET_TYPE_VAR     This is a variable. The description
**                                   contains the name the value its value.
**
**                                   Currently only one of these is created and
**                                   that is used to store the version number
**                                   passed to EvSetCreate.
**
**                EVSET_TYPE_FILE    This is a file entry. The description
**                                   field contains the user visible name of the
**                                   file, the value field contains the
**                                   filename it is stored under in the evidence
**                                   set.
**
**                                   These entries are created by the
**                                   EVSetCreateFile interface and can be
**                                   accessed via the EVSetListFile interface.
*/

static STATUS handler(EX_ARGS *);

/*{
**  Name: EVSetCreate - Create a new evidence set
**
**  Description:
**     This routine creates a new evidence set returning a unique identifier
**     for use on subsequent calls. The description passed is stored away and
**     can be retrieved via future calls of EVSetList
**
**     The failure of this routine will result in there being no where to 
**     place evidence at the time of a failure - for this reason external
**     calls are kept to a minimum and exception handlers used to localise
**     fails.
**
**     The evidence set identifier returned is the next free number in the
**     range 0 - 999. If the routine cannot create a new evidence set then an 
**     error is returned (in this case the caller will have to log as much
**     information as possible via other means).
**
**     The code simply scans the except directory to find the highest 
**     currently in use it then attempts to create the next one. Creating a
**     directory is an atomic action so if two processes try it at once one
**     should fail.
**
**     If it fails it tries the next one until the number wraps back to the
**     start again in which case it fails.
**
**     The CONTENTS file is then created and the evidence set id returned.
**
**  Inputs:
**     char *description       - One line description of the evidence set 
**                               (actually a one description of the problem
**                               the evidence set relates to)
**     char *version           - Version of the product (stored as a VERSION
**                               variable in the CONTENTS file)
**
**  Outputs:
**     int *id                 - An evidence set identifier for use on further
**                               calls
**
**  Returns:
**     0                       Evidence set sucessfully created
**     <>0                     An error occurred
**
**  Exceptions:
**     Exceptions are internally trapped and attempts at recovery made. If this
**     is not possible an appropriate status is returned
**
**  History:
**	05-May-2005 (wanfr01)
**	   Bug 114457, INGSRV3291
**	   Added additional messages so it is easier to see why II_EXCEPTION
**	   is failing.
*/

STATUS
EVSetCreate(i4 *id,PTR description,PTR version)
{
    EX_CONTEXT ex;
    char evdir[EVSET_MAX_PATH];
    PTR pointer;
    PTR ii_exception;
    DIR *my_dir;
    struct dirent *dirent;
    i4  last_id= -1;
    i4  myid;

    /* Define an exception handler to catch all problems */

    if(EXdeclare(handler,&ex)!=OK)
    {
       /* We come here on a problem */
       EXdelete();
       return(E_EXCEPTION);
    }

    /* Poke the id variable while protected by the exception handler */

    *id = -1;

    /* Unfortunately we need the location of the except directory which */
    /* is only available from the  II_EXCEPTION - so we have to call     */
    /* NMgtAt                                                             */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        /* No except directory defined - not a lot we can do */

        EXdelete();
	TRdisplay ("II_EXCEPTION is not defined.\n");
        return(E_NO_II_EXCEPTION);
    }

    /* Now construct the path to the evidence set root directory */

    STlcopy(ii_exception,evdir,sizeof(evdir)-14-1);
    STcat(evdir,"/ingres/except");

    /* Create except directory if it doesn't exist already */
    {
    	LOCATION loc_tmp;
	if (LOfroms(PATH,evdir,&loc_tmp) == OK)
	{
		LOcreate(&loc_tmp);
	}

    }
    /* Locate the last used evidence set */

    if((my_dir=opendir(evdir))==NULL)
    {
        EXdelete();
	TRdisplay ("%s could not be accessed\n",evdir);
        return(E_NO_II_EXCEPTION);
    }

    while((dirent=readdir(my_dir))!=NULL)
    {
        if(!STbcompare(dirent->d_name,5,"EVSET",5, TRUE))
        {
            int temp;

            CVal(dirent->d_name+5,&temp);
            if(temp > last_id)
                last_id=temp;
        }
    }

    closedir(my_dir);

    /* Create the basic skeleton for an evidence set */

    STcat(evdir,"/EVSET000");
    pointer=evdir+STlength(evdir);

    /* Now we have the basic path name of the except directory - try and */
    /* find a free entry                                                   */

    myid=NEXT_ID(last_id);

    do
    {
        EXdelete();

        /* Any exceptions in here cause the next entry to be tried */

        if(EXdeclare(handler,&ex)==OK)
        {
            pointer[-3] = '0' + (myid/100) % 10;
            pointer[-2] = '0' + (myid/10) %10;
            pointer[-1] = '0' + (myid % 10);

            /* Try and create a directory by this name               */
            /* - mkdir is atomic so only one process doing it at any */
            /*   one time should suceed                              */

            if(mkdir(evdir,0766)==0)
            {
                int contents;

                /* We created a directory - create a CONTENTS file in it */
                /* - but first define another handler so we can tidy up  */
                /*   if we get a problem                                 */

                EXdelete();

                if(EXdeclare(handler,&ex)==OK)
                {
                    STcat(evdir,"/CONTENTS");

                    if((contents=creat(evdir,0666))!= -1)
                    {
                        /* Doing well - finally write away the evidence set */
                        /* details structure and version number and we have */
                        /* finished !!                                      */

                        EVSET_DETAILS details;
 
                        details.id = myid;
                        time(&details.created);
                    
                        /* The description could be corrupt - take steps */
                        /* just in case                                  */

                        EXdelete();
                        if(EXdeclare(handler,&ex)==OK)
                        {
                            STncpy( details.description, description,
                               sizeof(details.description) - 1 );
			    details.description[ sizeof(details.description)
				- 1 ] = EOS;
                        }
                        else
                        {
                            STcopy("????????",details.description);
                        }

                        if(write(contents,(char *)&details,sizeof(details))==
                           sizeof(details))
                        {
                            /* Write away the version number - protect us */
                            /* from a corrupt version                     */

                            EVSET_ENTRY entry;

                            EXdelete();

                            entry.type=EVSET_TYPE_VAR;
                            entry.flags=EVSET_SYSTEM;
                            STcopy("VERSION",entry.description);

                            if(EXdeclare(handler,&ex)==OK)
                            {
                                STcopy(version,entry.value);
                            }
                            else
                            {
                                STcopy("???",entry.value);
                            }

                            if(write(contents,(char *)&entry, sizeof(entry))
                               ==sizeof(entry))
                            {
                                close(contents);
                                *id = myid;
                                EXdelete();
                                return(OK);
                            }

                        }

                    }
                }

                /* If we got here - something nasty happened to our new */
                /* evidence set - delete it and try the next one        */

                EXdelete();
                if(EXdeclare(handler,&ex)==OK)
                {
                    unlink(evdir);
                    *pointer = EOS;
                    rmdir(evdir);
                }
                
            }
        }

        myid=NEXT_ID(last_id);
    }
    while(myid!=NEXT_ID(last_id));

    /* Failed to find a free except directory - return an error */

    TRdisplay ("II_EXCEPTION dump directory is full.  Exiting dump.\n");

    EXdelete();

    return(E_EVSET_FULL);
}

/*{
**  Name: EVSetDelete - Delete an evidence set
**
**  Description:
**     This routine deletes an evidence set previously created by the 
**     EVSetCreate interface. All files in the evidence set are deleted and 
**     the subdirectory deleted.
**
**     This routine makes use of the dirent routines to find all files in the
**     evidence set.
**
**  Inputs:
**     int id        Evidence set identifier
**
**  Returns:
**     0                 	Success
**     E_UNKNOWN_EVSET   Unknown evidence set
**     <>0               	Other errors
**
**  History:
*/

STATUS
EVSetDelete(i4 id)
{
    char evdir[EVSET_MAX_PATH];
    PTR ii_exception;
    DIR *my_dir;
    struct dirent *dirent;

    /* Locate the evidence set directory */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
       return(E_NO_II_EXCEPTION);
    }
    STlpolycat(2, sizeof(evdir)-3-1, ii_exception, "/ingres/except/EVSET",
		&evdir[0]);
    ii_exception = &evdir[STlength(evdir)];	/* Append %03d */

    STprintf(ii_exception,"%03d",id);

    /* Open it and delete all files in it */

    if((my_dir=opendir(evdir))==NULL)
    {
        return(E_UNKNOWN_EVSET);
    }

    while((dirent=readdir(my_dir))!=NULL)
    {
        STprintf(evdir,"%s/ingres/except/EVSET%03d/%s",ii_exception,
           id,dirent->d_name);
        unlink(evdir);
        /* We can ignore the error - if this fails the rmdir will fail */
    }

    closedir(my_dir);

    /* Finally delete the directory */

    STprintf(evdir,"%s/ingres/except/EVSET%03d",ii_exception,id);

    if(rmdir(evdir)!=0)
       return(E_DELETE_FAIL);

    return(OK);
}

/*{
**  Name: EVSetExport - Export an evidence set to a named file
**
**  Description:
**      This routine exports the contents of the named evidence set into
**      a CPIO file with the name passed
**
**      This is simply done by constructing a cpio command and executing
**      it via popen
**
**      Using a pipe in this way allows cpio to run in the except directory
**      and therefore be able to archive just the evidence files while still
**      allowing us to write to the export file whose name is relative to the
**      current directory
**
**  Inputs:
**     int id         - Evidence set identifier
**     char *filename - Name of file to export to
**
**  Returns:
**     0                     		Export file sucessfully written
**     E_UNKNOWN_EVSET   	Unknown evidence set
**     E_CREATE_EVSET_FAIL	Unable to create export file
**     <>0                   		Other failure
**
**  History:
*/

STATUS
EVSetExport(i4 id,PTR filename)
{
    FILE 	*pipe;
    FILE 	*export;
    char 	buffer[EVSET_MAX_PATH+30];
    PTR 	ii_exception;
    i4  	count;
    LOCATION	loc;
    i4		flagword;
    i4		no_bytes;
    LOINFORMATION loinfo;

    /* Find except directory */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }

    /* Check evidence set exists */

    STlpolycat(2, EVSET_MAX_PATH-20, ii_exception, "/ingres/except/EVSET",
		&buffer[0]);
    ii_exception = &buffer[STlength(buffer)];	/* Append %03d/CONTENTS */

    STprintf(ii_exception,"%03d/CONTENTS",id);

    /* Stat file */
    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_CREATE_EVSET_FAIL);
    }
    flagword = LO_I_TYPE;
    if (LOinfo(&loc,&flagword,&loinfo) != OK)
    {
	return(E_UNKNOWN_EVSET);
    }
    /* Open the file to write to */
    if (LOfroms(PATH&FILENAME,filename,&loc) != OK)
    {
	if (LOfroms(FILENAME,filename,&loc) != OK)
	{
		return(E_CREATE_EVSET_FAIL);
	}
    }

    /* Open/create export file */
    if (SIfopen(&loc,"w",SI_RACC,sizeof(buffer),&export) != OK)
    {
	return(E_CREATE_EVSET_FAIL);
    }

    /* Build up cpio command */
    STprintf(buffer,"cd %s/ingres/except/EVSET%03d;ls|cpio -oc 2>/dev/null",
       ii_exception,id);

    if((pipe=popen(buffer,"r"))==NULL)
    {
        return(E_PIPE_FAILED);
    }

    while(SIread(pipe,sizeof(buffer),&count,buffer) == OK)
    {
	if(SIwrite(count,buffer,&no_bytes,export) != OK)
        {
            pclose(pipe);
	    SIclose(export);
            return(E_WRITE_FAILED);
        }
    }

    SIclose(export);

    if(pclose(pipe))
        return(E_WRITE_FAILED);

    return(OK);
}

/*{
**  Name: EVSetImport - Import an evidence set from a file
**
**  Description:
**
**      A new evidence set is created, the export file read into it
**      (overwriting the CONTENTS file) and the CONTENTS file fixed up to
**      refer to its new evidence set identifier and directory name
**
**      The cpio option -u is used to make the original CONTENTS file overwrite
**      the local temporary one
**
**      All other details of the evidence set (including its time of creation
**      remain as per its initial creation)
**
**  Inputs:
**      char *filename         - Name of export file
**
**  Outputs:
**      int *id                - New evidence set identifier
**
**  Returns:
**      OK                      Evidence set sucessfully imported
**      E_OPEN_FAIL     	Fail to open or read exportfile
**      <>0                    	Other error
**
**  History:
*/

STATUS
EVSetImport(PTR filename,i4 *id)
{
    STATUS 	status;
    PTR  	ii_exception;
    char	evdir[EVSET_MAX_PATH+1];
    char 	buffer[EVSET_MAX_PATH+80];
    FILE 	*export;
    FILE 	*cpio;
    LOCATION 	loc;
    i4		no_bytes;

    /* Find except directory */
    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }
    /* Copy to prevent overruns */
    STlcopy(ii_exception, &evdir[0], sizeof(evdir)-1);

    /* Create a new evidence set */

    status=EVSetCreate(id,"IMPORT","???");

    if(status==OK)
    {
        /* Open the export file */
	if (LOfroms(PATH&FILENAME,filename,&loc) != OK)
	{
	    if (LOfroms(FILENAME,filename,&loc) != OK)
	    {
		    return(E_OPEN_FAIL);
	    }
	}

	if (SIfopen(&loc,"r",SI_RACC,sizeof(buffer),&export) != OK)
        {
            EVSetDelete(*id);
            status=E_OPEN_FAIL;
        }
        else
        {
            /* Construct and execute the cpio command. This is done via */
            /* a pipe so we can keep the export file name relative to   */
            /* the current directory - the -u option on cpio allows the */
            /* CONTENTS file to be overwritten                          */


            STprintf(buffer,
		"cd %s/ingres/except/EVSET%03d;cpio -iuc 2>/dev/null" ,
		evdir, *id);

            if((cpio=popen(buffer,"w"))==NULL)
            {
                EVSetDelete(*id);
                status=E_PIPE_FAILED;
            }
            else
            {
                int count;

		while(SIread(export,sizeof(buffer),&count,buffer) == OK)
                {
		    if (SIwrite(count,buffer,&no_bytes,cpio) != OK)
                    {
                        EVSetDelete(*id);
                        status=E_WRITE_FAILED;
                    }
                }
		/* On solaris just hangs here sometimes cpio problem */
		/* This will be close on exit of idbg */
                /*pclose(cpio);*/
            }

	    SIclose(export);
        }
    }

    /* If all is OK upto here - fixup the CONTENTS file to refer to its */
    /* new evidence set identifier and location in filestore            */
    if(status==OK)
    {
        EVSET_DETAILS details;
	i4 count;

        STprintf(buffer,"%s/ingres/except/EVSET%03d/CONTENTS",
            evdir,*id);

	if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    	{
	    return(E_CREATE_EVSET_FAIL);
	}

	if (SIfopen(&loc,"rw",SI_RACC,sizeof(details),&export) != OK)
        {
            EVSetDelete(*id);
            status=E_EVSET_CORRUPT;
        }
	else if(SIread(export,sizeof(details),&count,(char *)&details) != OK)
        {
            EVSetDelete(*id);
            SIclose(export);
            status=E_EVSET_CORRUPT;
        }
        else
        {
            details.id=*id;

            /* Write back modified EVSET_DETAILS structure */

	    if (SIfseek(export,0,SI_P_START) != OK)
            {
                EVSetDelete(*id);
                SIclose(export);
                status=E_WRITE_FAILED;
            }
	    else if (SIwrite(sizeof(details),(char *)&details,&no_bytes,
			export) != OK)
            {
                EVSetDelete(*id);
                SIclose(export);
                status=E_WRITE_FAILED;
            }
            else
            {
                /* Modify path name in all EVSET_ENTRY file entries  */

                int file;
                EVSET_ENTRY fdetails;

                for(file=0;;file++)
                {
		    if (SIfseek(export,
			sizeof(EVSET_DETAILS)+(file*sizeof(EVSET_ENTRY)),
			SI_P_START) != OK)
                    {
                        EVSetDelete(*id);
                        status=E_WRITE_FAILED;
                        break;
                    }
		    else 
		    if(SIread(export,sizeof(fdetails),&no_bytes,
			(char *)&fdetails)!=OK)
                    {
                        /* No more file entries */
                        break;
                    }
                    else if(fdetails.type==EVSET_TYPE_FILE)
                    {
                        STprintf(fdetails.value,
                            "%s/ingres/except/EVSET%03d/FILE%d",
                            evdir,*id,file);
			if (SIfseek(export,
			    sizeof(EVSET_DETAILS)+(file*sizeof(EVSET_ENTRY)),
			    SI_P_START) != OK)
                        {
                            EVSetDelete(*id);
                            status=E_WRITE_FAILED;
                        }
			else 
			if(SIwrite(sizeof(fdetails),(char *)&fdetails,
				&no_bytes,export) !=OK)
                        {
                            EVSetDelete(*id);
                            status=E_WRITE_FAILED;
                        }
                    }
                }

		SIclose(export);
            }
        }

    }

    return(status);
}

/*{
**  Name: EVSetList - List details of evidence sets
**
**  Description:
**     This routine is used to get details of evidence sets. The first call
**     is passed the evidence set identifier to start from, subsequent calls
**     passing the evidence set identifier returned from the previous call
**     will return details of the next evidence set in order of evidence set
**     identifier.
**
**     An evidence set id of -1 can be used to locate the first evidence set
**
**     The identifiers returned to allow scans are -ve to allow them to be 
**     distinguished from normal ids for which details are required. The use
**     of -1 for find first complicates this a bit as follows:-
**
**     -2   Scan from evidence set 1
**     -1   Scan from first evidence set (ie from 0)
**      0   Get details for evidence set 0
**      1   Get details for evidence set 1
**
**     The details returned are read from the CONTENTS file of the appropriate
**     evidence set - if the CONTENTS file does not exist then an error is
**     returned.
**
**  Inputs:
**     int *id                 - Evidence set identifier
**                               (or value returned from previous call)
**
**  Outputs:
**     EVSET_DETAILS *details  - Details of the evidence set
**
**  Returns:
**     0                       	Details sucessfully obtained
**     E_UNKNOWN_EVSET  	Evidence set does not exist
**     E_EVSET_NOMORE   	No more evidence sets can be found
**     <>0                     	Other error
**
**  History:
*/

STATUS
EVSetList(i4 *id,EVSET_DETAILS *details)
{
    i4   	my_id=EVSET_MAX_ID+1;
    PTR  	ii_exception;
    char	evdir[EVSET_MAX_PATH-50];
    char 	buffer[EVSET_MAX_PATH];
    FILE   	*contents;
    LOCATION	loc;
    i4		count;


    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }
    STlcopy(ii_exception, &evdir[0], sizeof(evdir)-1);

    /* If the evidence set is positive then it is a real one and should exist */

    if(*id >= 0)
    {
        my_id=*id;
    }
    else
    {
        /* We have to scan for the next id */

        DIR 	*my_dir;
        struct 	dirent *dirent;
        i4  	lowid=-((*id)+1);

        STprintf(buffer,"%s/ingres/except",evdir);

        if((my_dir=opendir(buffer))==NULL)
        {
            return(E_NO_II_EXCEPTION);
        }

        while((dirent=readdir(my_dir))!=NULL)
        {
            if(!STbcompare(dirent->d_name,5,"EVSET",5, TRUE))
            {
                i4  currid;

                CVal(dirent->d_name+5,&currid);

                if(currid >= lowid && currid < my_id)
                    my_id=currid;
            }
        }

        closedir(my_dir);

        /* Did the scan find one ? */
        if(my_id==EVSET_MAX_ID+1)
            return(E_EVSET_NOMORE);
    }

    /* Now we have an id - setup the next one */

    *id=-(my_id+2);

    /* Open up and read the CONTENTS file */

    STprintf(buffer,"%s/ingres/except/EVSET%03d/CONTENTS",evdir,my_id);
    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_UNKNOWN_EVSET);
    }
    if (SIfopen(&loc,"r",SI_RACC,sizeof(EVSET_DETAILS),&contents) != OK)
    {
        return(E_UNKNOWN_EVSET);
    }

    if (SIread(contents,sizeof(EVSET_DETAILS),&count,(PTR)details)!=OK)
    {
        SIclose(contents);
        return(E_EVSET_CORRUPT);
    }

    SIclose(contents);
    return(OK);
}

/*{
**  Name: EVSetCreateFile - Create a file in an evidence set
**
**  Description:
**     This routine creates a new file in an evidence set and returns the
**     name of the created file so that any required information can be
**     written to it. The supplied file flags and description are recorded in
**     the CONTENTS file for later retrieval via the EVSetFileList interface
**
**     Files are created in the appropriate evidence set directory with names
**     of the form FILEn where n is a decimal number increasing from 0
**
**     Any slots left by files deleted with EVSetDeleteFile are reused if
**     available.
**
**  Inputs:
**     i4  id            Identifier of evidence set
**     i4  flags         Flags for this file
**                       EVSET_TEXT    Text file
**                       EVSET_BINARY  Binary file
**                       EVSET_SYSTEM  System created file
**
**     PTR description   One line description of files contents
**     PTR filename    	 Buffer to return filename to
**     i4  len           Length of above
**
**  Outputs:
**     PTR filename      Filled in with full pathname of created file
**
**  Returns:
**     OK                 	File created successfully
**     E_UNKNOWN_EVSET   	Unknown evidence set
**     E_PARAM_ERROR    	Filename buffer too short
**     <>0               	Other error
**
**  Exceptions:
**     This routine is called while evidence is being collected, any fail in
**     here will result in there being no file to store evidence in. For this
**     reason external calls are kept to a minimum and exception handlers used 
**     to localise the effect of crashes.
**
**  History:
*/

STATUS
EVSetCreateFile(i4 id,i4 flags,PTR description,PTR filename,i4 len)
{
    EX_CONTEXT ex;
    PTR ii_exception;
    char evdir[EVSET_MAX_PATH-50];
    char buffer[EVSET_MAX_PATH];
    EVSET_ENTRY details;
    i4  new;
    FILE *contents;
    i4  eof;
    i4  fileid;
    i4  count;
    LOCATION loc;
    LOINFORMATION loinfo;

    /* Protect ourselves with an exception handler */
    if(EXdeclare(handler,&ex)!=OK)
    {
        EXdelete();
        return(E_EXCEPTION);
    }

    /* Find the except directory */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        EXdelete();
        return(E_NO_II_EXCEPTION);
    }
    STlcopy(ii_exception, &evdir[0], sizeof(evdir)-1);

    /* Open the CONTENTS file */

    STprintf(buffer,"%s/ingres/except/EVSET%03d/CONTENTS",evdir,id);

    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_CREATE_EVSET_FAIL);
    }

    if (SIfopen(&loc,"rw",SI_RACC,sizeof(details),&contents) != OK)
    {
        EXdelete();
        return(E_UNKNOWN_EVSET);
    }

    /* Scan for any deleted entries */

    details.flags=EVSET_DELETED+1; /* ie not EVSET_DELETED */

    for(fileid=0;
       (details.type!=EVSET_TYPE_FILE || details.flags!=EVSET_DELETED);++fileid)
    {
	i4 count;
	if (SIfseek(contents,sizeof(EVSET_DETAILS)+(fileid*sizeof(EVSET_ENTRY)),
	SI_P_START) != OK)
            break;
	if (SIread(contents,sizeof(details),&count,(char *)&details) != OK)
            break;
    }

    /* If we found one reuse it otherwise allocate a new one */

    if(details.type==EVSET_TYPE_FILE && details.flags==EVSET_DELETED)
    {
        fileid--;

	if (SIfseek(contents,-sizeof(EVSET_ENTRY),SI_P_CURRENT)!=OK)

        {
	    SIclose(contents);
            EXdelete();
            return(E_EVSET_CORRUPT);
        }
    }
    else
    {
        /* Locate the end and from this work out the next free file number */

	i4 flagword = LO_I_SIZE;
	if (LOinfo(&loc,&flagword,&loinfo) != OK)
	{
		return(E_EVSET_CORRUPT);
	}
	eof=loinfo.li_size;

        if(eof== -1 || eof < sizeof(EVSET_DETAILS))
        {
	    SIclose(contents);
            EXdelete();
            return(E_EVSET_CORRUPT);
        }

        /* The entry structures are after the EVSET_DETAILS structure work */
        /* out how many we have                                            */

        fileid=(eof - sizeof(EVSET_DETAILS))/sizeof(EVSET_ENTRY);

    }

    /* Copy filename to users buffer - if it fits */

    STprintf(buffer,"%s/ingres/except/EVSET%03d/FILE%d",
       evdir,id,fileid);

    if((i4)STlength(buffer) >= len)
    {
        EXdelete();
        return(E_PARAM_ERROR);
    }

    STcopy(buffer,filename);

    /* Build a file description structure - any failures here result in */
    /* a default structure being built                                  */

    EXdelete();

    if(EXdeclare(handler,&ex)==OK)
    {
        MEfill(sizeof(details),0,(char *)&details);
        details.type=EVSET_TYPE_FILE;
        details.flags=flags;
        STncpy(details.description, description, sizeof(details.description)-1);
	details.description[ sizeof(details.description) - 1 ] = EOS;
        STcopy(buffer,details.value);
    }
    else
    {
        MEfill(sizeof(details),0,(char *)&details);
        details.type=EVSET_TYPE_FILE;
        details.flags=EVSET_SYSTEM | EVSET_BINARY;
        STcopy("** Unknown",details.description);
        STcopy(buffer,details.value);
    }

    /* Create a file with this id */


    if((new=creat(buffer,0766))== -1)
    {
        SIclose(contents);
        EXdelete();
        return(E_CREATE_EVSET_FAIL);
    }

    /* We now have a file - any failures from this point should delete it */

    EXdelete();

    if(EXdeclare(handler,&ex)!=OK)
    {
        unlink(buffer);
	SIclose(contents);
        return(E_EXCEPTION);
    }
    else
    {
        close(new);

        /* Write away details of the new file to the CONTENTS file */

	if (SIwrite(sizeof(details),(char *)&details,&count,contents)!=OK)
        {
            unlink(buffer);
	    SIclose(contents);
            return(E_WRITE_FAILED);
        }

    }

    /* All OK */

    EXdelete();
    SIclose(contents);
    return(OK);
}

/*{
**  Name: EVSetDeleteFile - Delete a file from an evidence set
**
**  Description:
**     This routine deletes a file from an evidence set. The EVSET_ENTRY entry
**     in the contents file is marked as EVSET_DELETED and the underlying data
**     file deleted.
**
**     If the number passed references a deleted slot then it is moved on to
**     point at the next non deleted slot - ie the numbering scheme skips 
**     deleted slots
**
**  Inputs:
**     i4  id           Evidence set id
**     i4  file         File id
**
**  Returns:
**     OK               File deleted
**     E_UNKNOWN_EVSET  Unknown evidence set or file
**     <>0              Other errors
**
**  History:
*/

STATUS
EVSetDeleteFile(i4 id,i4 file)
{
    PTR  ii_exception;
    char evdir[EVSET_MAX_PATH-50];
    char buffer[EVSET_MAX_PATH];
    EVSET_ENTRY details;
    FILE *contents;
    LOCATION loc;
    i4		count;

    /* Find the except directory */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }
    STlcopy(ii_exception, &evdir[0], sizeof(evdir)-1);

    /* Open the CONTENTS file */

    STprintf(buffer,"%s/ingres/except/EVSET%03d/CONTENTS",evdir,id);

    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_UNKNOWN_EVSET);
    }

    if (SIfopen(&loc,"rw",SI_RACC,sizeof(details),&contents)!=OK)
    {
        return(E_UNKNOWN_EVSET);
    }

    /* Find the EVSET_ENTRY structure */

    details.flags=EVSET_DELETED; 
    details.type =EVSET_TYPE_FILE;
    /* Our first file has to start from 1 ensure it does */
    while (details.flags==EVSET_DELETED && details.type==EVSET_TYPE_FILE)
    {
        if (SIfseek(contents,sizeof(EVSET_DETAILS)+(file*sizeof(EVSET_ENTRY)),
	   SI_P_START) != OK)
        {
            SIclose(contents);
            return(E_UNKNOWN_EVSET);
        }
    
        if(SIread(contents,sizeof(details),&count,(char *)&details) !=OK)
        {
            SIclose(contents);
            return(E_UNKNOWN_EVSET);
        }

        file++;
    }

    file--;


    /* Delete the underlying file */

    STprintf(buffer,"%s/ingres/except/EVSET%03d/FILE%d",
        evdir,id,file);
    if(unlink(buffer)== -1)
    {
        SIclose(contents);
        return(E_EVSET_FILE_DEL_FAIL);
    }

    /* Mark as deleted */

    details.flags=EVSET_DELETED;

    if (SIfseek(contents,sizeof(EVSET_DETAILS)+(file*sizeof(EVSET_ENTRY)),
	       SI_P_START) != OK)
    {
       SIclose(contents);
       return(E_WRITE_FAILED);
    }

    if (SIwrite(sizeof(details),(char *)&details,&count,contents) != OK)
    {
       SIclose(contents);
       return(E_WRITE_FAILED);
    }

    SIclose(contents);

    return(OK);
}

/*{
**  Name: EVSetFileList - List details of files in an evidence set
**
**  Description:
**     This routine provides details of files in a particular evidence set.
**     It provides access to the filename of each evidence file along with
**     its type and the description supplied at create time
**
**     The CONTENTS file in the appropriate evidence set is read to obtain
**     the file details. 
**
**     The numbering scheme ignores deleted slots - if you request details on
**     a deleted file you will actually receive details on the next one that
**     is not deleted. 
**
**  Inputs:
**      i4  id              Evidence set identifier
**      i4  *file           File identifier (0 for first or value from previous
**                          call for next)
**
**  Outputs:
**      i4  *file           Updated with value suitable for use to get details
**                          of next file 
**      EVSET_ENTRY *details Filled in with details of evidence file
**
**  Returns:
**      OK                  File details obtained
**      E_UNKNOWN_EVSET     Unknown evidence set
**      E_EVSET_NOMORE      No (more) files exist
**      <>0                 Other error
**
**  History:
*/

STATUS
EVSetFileList(i4 id,i4 *file_next,EVSET_ENTRY *details)
{
    PTR  	ii_exception;
    char 	buffer[EVSET_MAX_PATH];
    FILE  	*contents;
    LOCATION 	loc;
    i4		count;

    /* Find the except directory */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }

    /* Open the CONTENTS file */

    STlpolycat(2, EVSET_MAX_PATH-20, ii_exception, "/ingres/except/EVSET",
		&buffer[0]);
    ii_exception = &buffer[STlength(buffer)];	/* Append %03d/CONTENTS */
    STprintf(ii_exception,"%03d/CONTENTS",id);

    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_UNKNOWN_EVSET);
    }

    if (SIfopen(&loc,"rw",SI_RACC,sizeof(EVSET_ENTRY),&contents)!=OK)
    {
        return(E_UNKNOWN_EVSET);
    }
    
    /* Scan for next non deleted entry */

    details->flags=EVSET_DELETED;

    while(details->flags==EVSET_DELETED || details->type!=EVSET_TYPE_FILE)
    {
        if(SIfseek(contents, sizeof(EVSET_DETAILS)+
			(*file_next*sizeof(EVSET_ENTRY)),SI_P_START)!=OK)
        {
            SIclose(contents);
            return(E_EVSET_NOMORE);
        }

	if (SIread(contents,sizeof(EVSET_ENTRY),&count,(PTR)details) !=OK)
        {
            SIclose(contents);
            return(E_EVSET_NOMORE);
	}
        (*file_next)++;
    }

    SIclose(contents);

    return(OK);
}

/*{
**  Name: EVSetLookupVar - Lookup the value of a variable
**
**  Description:
**      This routine looks up a named variable in the CONTENTS file of a 
**      specified evidence set and returns its value if present. If the variable
**      is not found the string "unknown" is returned.
**
**  Inputs:
**      i4  id              Evidence set identifier
**      PTR name            Name of variable to lookup
**      PTR value           Place to return value
**      i4  len             Length of above
**
**  Outputs:
**      i4  *value          Updated with value of variable (or "unknown" if
**                          not found)
**
**  Returns:
**      OK                  File details obtained
**      E_UNKNOWN_EVSET     Unknown evidence set
**      <>0                 Other error
**
**  History:
*/

STATUS
EVSetLookupVar(i4 id,PTR name,PTR value,i4 len)
{
    PTR  	ii_exception;
    char 	buffer[EVSET_MAX_PATH];
    PTR  	my_value="unknown";
    EVSET_ENTRY details;
    FILE  	*contents;
    LOCATION 	loc;
    i4		count;

    /* Find the except directory */
    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }

    /* Open the CONTENTS file */

    STlpolycat(2, EVSET_MAX_PATH-20, ii_exception, "/ingres/except/EVSET",
		&buffer[0]);
    ii_exception = &buffer[STlength(buffer)];	/* Append %03d/CONTENTS */
    STprintf(ii_exception,"%03d/CONTENTS",id);

    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_UNKNOWN_EVSET);
    }

    if (SIfopen(&loc,"r",SI_RACC,sizeof(details),&contents)!=OK)
    {
        return(E_UNKNOWN_EVSET);
    }
    
    /* Scan for variable of specified name */

    if (SIfseek(contents,sizeof(EVSET_DETAILS),SI_P_START) !=OK)
    {
        SIclose(contents);
        return(E_EVSET_CORRUPT);
    }

    while(SIread(contents,sizeof(EVSET_ENTRY),&count,(char *)&details)==OK)
    {
        if(details.type==EVSET_TYPE_VAR && !STcompare(details.description,name))
        {
            my_value=details.value;
            break;
        }
    }

    SIclose(contents);

    if((i4)STlength(my_value) < len)
    {
        STcopy(my_value,value);
        return(OK);
    }
    else
    {
        return(E_PARAM_ERROR);
    }
}

/*{
**  Name: handler - exception handler
**
**  Description:
**     This routine is used as an exception handler in various routines in 
**     this module. We are not interested ion what the exception is just that
**     it happened - in all cases we return EXDECLARE to get the code to 
**     return back to the EXdeclare call which defined the handler
**
**  Inputs:
**     EX_ARGS *ex_args        Parameters to handler (not used)
**
**  Returns:
**     EXDECLARE
**
**  History:
*/

static STATUS
handler(EX_ARGS *ex_args)
{
    return(EXDECLARE);
}
