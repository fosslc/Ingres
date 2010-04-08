/*
**    Copyright (c) 1993, 2000 Ingres Corporation
*/
#include <compat.h>
#include <ex.h>
#include <st.h>
#include <si.h>
#include <lo.h>
#include <pc.h>
#include <me.h>
#include <cv.h>
#include <tm.h>
#include <nm.h>
#include <rmsdef.h>
#include <descrip.h>
#include <ssdef.h>
#include <time.h>
#include <evset.h>
#include <lib$routines.h>


#define EVSET_MAX_ID 999
#define NEXT_ID(id) (id==EVSET_MAX_ID?0:id+1)

/**
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
**     EVSetImport         - Import an evidence set (for ICL support use only)
**     EVSetList           - List details of existing evidence sets
**     EVSetCreateFile     - Create a new file in an evidence set
**     EVsetDeleteFile     - Delete a file from an evidence set
**     EVSetFileList       - List files in an evidence set
**     EVSetLookupVar      - Lookup variables in CONTENTS file
**
**  History:
**     25-Aug-1993 (paulb ICL)
**         Created.
**     15-Oct-1993 (paulb ICL)
**         Change evidence set location to $II_EVIDENCE/ingres/evidence
**     16-Nov-1993 (paulb ICL)
**         Change evidence set allocation scheme to allocate with increasing
**         numbers. Add EVSET_SYSTEM flag to differentiate system and user
**         created evidence files
**     06-Dec-1993 (paulb ICL)
**         Add EVSetDeleteFile support
**     11-May-1994 (paulb ICL)
**         Add version number support
**         - change to support multiple entry types rather than just filenames
**         - Add version parameter to EVSetCreate
**         - Add EVSetLookupVar routine
**	14-Jun-1995 (prida01 CA)
**	   - change location to $II_EXCEPTION/ingres/except
** VMS History
**	22-Aug-1995 (mckba01 VMS)
**	   - Initial VMS port.
**	13-Oct-1995 (mckba01)
**	   - Changed condition handler names to avoid link warnings.
**	20-Oct-1995 (mckba01)
**	   - Replace usage of delete and mkdir with LO function calls.
**      18-May-1998 (horda03)
**         Ported to OI 1.2/01.
**      24-Nov-1998 (horda03)
**         Bug 94281 - Remove any Event Handlers defined within a function
**         before returning from the function.
**      19-may-99 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/*
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

static STATUS ev_handler();
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
**     range 0 - 9999. If the routine cannot create a new evidence set then an 
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
**     i4 *id                 - An evidence set identifier for use on further
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
**     24-Aug-1993 (paulb ICL)
**        Created.
**     11-May-1994 (paulb ICL)
**        Add version parameter
**
**  VMS History:
**	22-Aug-1995 (mckba01 CA)
**	  - Initial VMS port.
**      24-Nov-1998 (horda03)
**         Bug 94281 - Remove any Event Handlers defined within a function
**         before returning from the function.
*/

STATUS
EVSetCreate(id,description,version)
i4 *id;
PTR description;
PTR version;
{
    	STATUS 		status	=	OK;
    	EX_CONTEXT 	ex;
	char		EVsetName[EVSET_MAX_PATH];
       	PTR		TempPath = 	"II_EXCEPTION:[INGRES.EXCEPT]";
	i4		NextId	= 	-1;
	u_i4	Context	= 	0;
	u_i2		TrimLen	= 	8;
	char		TrimName[10];	
	i4		temp;
	char		NextNumStr[10];
	PTR		ii_exception;        
	FILE		*contents;
	LOCATION	loc;
        i4		count;          
	EVSET_DETAILS 	details;
                                           
	
	/* descriptors for LIB$FIND_FILE */

	$DESCRIPTOR		(Resultant,EVsetName);
	$DESCRIPTOR		(Trimmed,TrimName);
	static	$DESCRIPTOR	(Filespec,"II_EXCEPTION:[INGRES.EXCEPT]EVSET*.DIR");
	static	$DESCRIPTOR	(DirSpec,"II_EXCEPTION:[INGRES]EXCEPT.DIR");

    	/* Define an exception handler to catch all problems */

    	if(EXdeclare(ev_handler,&ex)!=OK)
    	{
       		/* We come here on a problem */
       		EXdelete();
       		return(E_EXCEPTION);
    	}

    	/* Poke the id variable while protected by the exception handler */

    	*id = -1;

	/*	
		Check Existence of Evidence set directory	
		if its not present, then create it.
	*/


    	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
        	EXdelete();
        	return(E_NO_II_EXCEPTION);
    	}




	if(lib$find_file( &DirSpec, &Resultant, &Context) != RMS$_NORMAL)
	{
		/*
			Didn't find the directory file so create
			Directory.
		*/
		if(LOfroms(PATH, TempPath, &loc) != OK)
		{
			EXdelete();
			return(E_CREATE_FAIL);
		}
		
		if(LOcreate(&loc) != OK)
		{
			EXdelete();
			return(E_CREATE_FAIL);
		}
		/*
			If its a new directory then
			we know the evidence set number
			will be 0.
		*/
	
		NextId = 0;	
	}

	if(NextId == -1)
	{
		/*	
			Get Next Evidence set number
		*/

		lib$find_file_end(&Context);
		Context = 0;

		while(lib$find_file( &Filespec, &Resultant, &Context) == RMS$_NORMAL)
		{
			lib$trim_filespec( &Resultant, &Trimmed, &TrimLen);
			TrimName[8] = '\0';
            		CVal(&(TrimName[5]),&temp);
            		if(temp > NextId)
                		NextId=temp;
		}
		lib$find_file_end(&Context);

		/*	Generate next id number	*/
     	
		NextId = NEXT_ID(NextId);
	}    
        else
		NextId = 0;

    	/* Create the basic skeleton for an evidence set */

        EXdelete();

        if((EXdeclare(ev_handler,&ex)==OK) & (NextId < 1000))
        {
		STcopy("II_EXCEPTION:[INGRES.EXCEPT.EVSET000]",EVsetName);	
		CVna(NextId,NextNumStr);		

		if(NextId > 99)
		{
			EVsetName[33] = NextNumStr[0];
			EVsetName[34] = NextNumStr[1];
			EVsetName[35] = NextNumStr[2];
		}
		else if(NextId > 9)
		{
			EVsetName[34] = NextNumStr[0];
			EVsetName[35] = NextNumStr[1];
		}
		else
		{
			EVsetName[35] = NextNumStr[0];
		}
            	/* 
			Try and create a directory by this name      
		*/

		if(LOfroms(PATH,EVsetName,&loc) != OK)
		{
			EXdelete();
			return(E_CREATE_FAIL);
		}

		if(LOcreate(&loc) == OK)
            	{

                	/* 
				We created a directory - create a CONTENTS file in it
                		but first define another handler so we can tidy up  
                		if we get a problem
			*/
                	EXdelete();

                	if(EXdeclare(ev_handler,&ex)==OK)
                	{
                    		STcat(EVsetName,"CONTENTS.");


			    	/* Open the CONTENTS file */

    				if (LOfroms(PATH&FILENAME,EVsetName,&loc) != OK)
    				{
					EXdelete();
					return(E_CREATE_EVSET_FAIL);
    				}

    				if (SIfopen(&loc,"w",SI_RACC,sizeof(details),&contents) != OK)
    				{
        				EXdelete();
        				return(E_CREATE_EVSET_FAIL);
    				}
				else
                    		{
                        		/* 
						Doing well - finally write away the evidence set
                        			details structure and version number and we have 
                        			finished
					*/
                                        
                        		details.id = NextId;
                        		details.created = (u_i4) TMsecs();
                    
                        		/* 
						The description could be corrupt - take steps
                        			just in case                                  
					*/

                        		EXdelete();
                        		if(EXdeclare(ev_handler,&ex)==OK)
                        		{
                            			STlcopy(description,details.description,
                               			sizeof(details.description));
                        		}
                        		else
                        		{		
                            			STcopy("????????",details.description);
                        		}

                        		if(SIwrite(sizeof(details),
					     (char *)&details,&count,
					     contents)== OK)
                        		{
                            			/* 
							Write away the version number - protect us
                            			       	from a corrupt version
                     				*/

                            			EVSET_ENTRY entry;

                            			EXdelete();

                            			entry.type=EVSET_TYPE_VAR;
                            			entry.flags=EVSET_SYSTEM;
                            			STcopy("VERSION",entry.description);

                            			if(EXdeclare(ev_handler,&ex)==OK)
                            			{
                                			STcopy(version,entry.value);
                            			}
                           			else
                            			{
                                			STcopy("????",entry.value);
                            			}

                            			if(SIwrite(sizeof(entry),
						  (char *)&entry, &count, 
						  contents)==OK)
                            			{
                                			SIclose(contents);
                                			*id = NextId;
                                			EXdelete();
                                			return(OK);
                            			}
                        		}
                    		}
                	}

                	/* 
				If we got here - something nasty happened to our new
                		evidence set - delete it and try the next one        
			*/

                	EXdelete();                
            	}
   	}

    	/* Failed to find a free except directory - return an error */

    	EXdelete();

    	return(E_EVSET_FULL);
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
**     On VAX machines, the file is not actually created, the name
**     of the file is returned to the caller.  It is then the responsibility
**     of the caller to create the file, and check its existence.
**
**  Inputs:
**     int id            Identifier of evidence set
**     int flags         Flags for this file
**                       EVSET_TEXT    Text file
**                       EVSET_BINARY  Binary file
**                       EVSET_SYSTEM  System created file
**
**     char *description One line description of files contents
**     char *filename    Buffer to return filename to
**     int len           Length of above
**
**  Outputs:
**     char *filename    Filled in with full pathname of created file
**
**  Returns:
**     0                 File created successfully
**     E_UNKNOWN_EVSET   Unknown evidence set
**     E_PARAM_ERROR     Filename buffer too short
**     <>0               Other error
**
**  Exceptions:
**     This routine is called while evidence is being collected, any fail in
**     here will result in there being no file to store evidence in. For this
**     reason external calls are kept to a minimum and exception handlers used 
**     to localise the effect of crashes.
**
**  History:
**     26-Aug-1993 (paulb ICL)
**        Created.
**
**  VMS History:
**	31-Aug-1995 (mckba01 CA)
**		Initial VMS port. 
**      24-Nov-1998 (horda03)
**         Bug 94281 - Remove any Event Handlers defined within a function
**         before returning from the function.
*/

STATUS
EVSetCreateFile(id,flags,description,filename,len)
i4 id;
i4 flags;
PTR description;
PTR filename;
i4 len;
{
    	EX_CONTEXT 	ex;
    	PTR 		ii_exception;
    	char 		buffer[EVSET_MAX_PATH];
    	EVSET_ENTRY 	details;
    	i4 		new;
    	FILE 		*contents;
    	i4 		eof;
    	i4 		fileid;
    	i4 		count;
    	LOCATION 	loc;
    	LOINFORMATION 	loinfo;
	i4		LoopStat;

	    /* Protect ourselves with an exception handler */

    	if(EXdeclare(ev_handler,&ex)!=OK)
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

    	/* Open the CONTENTS file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS.",id);
        
    	if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    	{
		EXdelete();
		return(E_CREATE_EVSET_FAIL);
    	}

    	if (SIfopen(&loc,"rw",SI_RACC,sizeof(details),&contents) != OK)
    	{
        	EXdelete();
        	return(E_UNKNOWN_EVSET);
    	}

   	/* Scan for any deleted entries */ 

	fileid = -1;
	LoopStat = 0;

	do
	{
		if(SIfseek(contents,sizeof(EVSET_DETAILS)+((fileid+2)*sizeof(EVSET_ENTRY)), SI_P_START) != OK)
		{
	    		SIclose(contents);
            		EXdelete();
            		return(E_EVSET_CORRUPT);
            	}
		if (SIread(contents,sizeof(details),&count,
		     (char *)&details) != OK)
		{
			LoopStat = 1;
            		break;
		}
		fileid++;
		if((details.type == EVSET_TYPE_FILE) && (details.flags ==EVSET_DELETED))
		{
			LoopStat = 2;
			break;
		}
    	} while((details.type == EVSET_TYPE_FILE ) || (details.type ==	EVSET_TYPE_VAR));
    
                                                                                                

    	/* If we found one reuse it otherwise allocate a new one */
	switch(LoopStat)
	{
   	case 0:
	case 2:	
       		{
			/*	
				Read a record at the end of the file
				or we found a deleted entry
			*/
			if (SIfseek(contents,-sizeof(EVSET_ENTRY),SI_P_CURRENT)!=OK)
        		{
	    	       		SIclose(contents);
            	       		EXdelete();
            			return(E_EVSET_CORRUPT);
        		}
			break;
		}
	case 1:
	    	{
			/*
				End of file, new fileid = last id + 1
			*/
		
			fileid++;

			/*
				Seek to end of the file
			*/

			if (SIfseek( contents, 0, SI_P_END)!=OK)
        		{
	    	       		SIclose(contents);
            	       		EXdelete();
            			return(E_EVSET_CORRUPT);
        		}
			break;
		}
	default:
		{
	    	       	SIclose(contents);
            	       	EXdelete();
            		return(E_EVSET_CORRUPT);
			break;
		}
	}


    	/* Copy filename to users buffer - if it fits */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]FILE%d.",id,fileid);

    	if(STlength(buffer) >= len)
    	{
        	EXdelete();
        	return(E_PARAM_ERROR);
    	}

    	STcopy(buffer,filename);

    	/* 
		Build a file description structure - any failures here result in
    		a default structure being built                                 
	*/

    	EXdelete();

    	if(EXdeclare(ev_handler,&ex)==OK)
    	{
        	MEfill(sizeof(details),0,(char *)&details);
        	details.type=EVSET_TYPE_FILE;
        	details.flags=flags;
        	STlcopy(description,details.description,sizeof(details.description));
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

        /* Write away details of the new file to the CONTENTS file */

	if (SIwrite(sizeof(details),(char *)&details,&count,contents)!=OK)
        {
	    SIclose(contents);
	    EXdelete();
            return(E_WRITE_FAILED);
        }

    	/* All OK */

    	EXdelete();
    	SIclose(contents);
    	return(OK);
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
**     0                       Details sucessfully obtained
**     E_UNKNOWN_EVSET         Evidence set does not exist
**     E_EVSET_NOMORE          No more evidence sets can be found
**     <>0                     Other error
**
**  History:
**     24-Aug-1993 (paulb ICL)
**        Created.
**
**  VMS History:
**	31-Aug-1995 (mckba01 CA)
**		Initial VMS port. 
*/

STATUS
EVSetList(id,details)
i4 *id;
EVSET_DETAILS *details;
{
    	i4  		my_id=EVSET_MAX_ID+1;
    	PTR 	 	ii_exception;
    	char 		buffer[EVSET_MAX_PATH];
	char		TrimName[EVSET_MAX_PATH];
    	FILE   		*contents;
    	LOCATION	loc;
    	i4		count;
	i4		currid;
	i4		TrimLen =	8;
	u_i4	Context	= 	0;

	/* descriptors for LIB$FIND_FILE */

	$DESCRIPTOR		(Resultant,buffer);
	$DESCRIPTOR		(Trimmed,TrimName);
	static	$DESCRIPTOR	(Filespec,"II_EXCEPTION:[INGRES.EXCEPT]EVSET*.DIR");
	static	$DESCRIPTOR	(DirSpec,"II_EXCEPTION:[INGRES]EXCEPT.DIR");



    	/* Find except directory */

    	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
        	return(E_NO_II_EXCEPTION);
    	}

    	/* 
		If the evidence set is positive then it is 
		a real one and should exist
	*/

    	if(*id >= 0)
    	{
        	my_id = (*id);
    	}
    	else
    	{
        	/* 
			We have to scan for the next id
		*/

        	i4 lowid = (-((*id)+1));  /* gives us first number to find */

		/*
			Check exception directory exists
		*/

		if(lib$find_file( &DirSpec, &Resultant, &Context) != RMS$_NORMAL)
		{
            		return(E_NO_II_EXCEPTION);
		}

		/*	
			Get Next Evidence set number
		*/

		lib$find_file_end(&Context);
		Context = 0;

		while(lib$find_file( &Filespec, &Resultant, &Context) == RMS$_NORMAL)
		{
			lib$trim_filespec( &Resultant, &Trimmed, &TrimLen);
			TrimName[8] = '\0';
            		CVal(&(TrimName[5]),&currid);
            		if(currid >= lowid && currid < my_id)
                		my_id = currid;
		}
		lib$find_file_end(&Context);

	        /* Did the scan find one ? */

        	if(my_id==EVSET_MAX_ID+1)
            		return(E_EVSET_NOMORE);
	}    


	/* Now we have an id - setup the next one */

    	*id = (-(my_id+2));

    	/* Open up and read the CONTENTS file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS.",my_id);

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
**      i4 id              Evidence set identifier
**      i4 *file           File identifier (0 for first or value from previous
**                          call for next)
**
**  Outputs:
**      i4 *file           Updated with value suitable for use to get details
**                          of next file 
**      EVSET_ENTRY *details Filled in with details of evidence file
**
**  Returns:
**      0                   File details obtained
**      E_UNKNOWN_EVSET     Unknown evidence set
**      E_EVSET_NOMORE      No (more) files exist
**      <>0                 Other error
**
**  History:
**     26-Aug-1993 (paulb ICL)
**        Created.
*/

STATUS
EVSetFileList(id,file,details)
i4 id;
i4 *file;
EVSET_ENTRY *details;
{
    	PTR  		ii_exception;
    	char 		buffer[EVSET_MAX_PATH];
    	FILE  		*contents;
    	LOCATION 	loc;
    	i4		count;


    	/* Find the except directory */

    	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
        	return(E_NO_II_EXCEPTION);
    	}

    	/* Open the CONTENTS file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS",id);

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
        	if(SIfseek(contents,sizeof(EVSET_DETAILS)+(*file*sizeof(EVSET_ENTRY)),SI_P_START)!=OK)
        	{
            		SIclose(contents);
            		return(E_EVSET_NOMORE);
        	}

		if (SIread(contents,sizeof(EVSET_ENTRY),&count,(PTR)details) !=OK)
        	{
            		SIclose(contents);
            		return(E_EVSET_NOMORE);
		}

		if((details->type != EVSET_TYPE_FILE) && (details->type !=EVSET_TYPE_VAR))
		{
            		SIclose(contents);
            		return(E_EVSET_NOMORE);
		}

        	(*file)++;
    	}

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
**     int id           Evidence set id
**     int file         File id
**
**  Returns:
**     0                File deleted
**     E_UNKNOWN_EVSET  Unknown evidence set or file
**     <>0              Other errors
**
**  History:
**     06-Dec-1993 (paulb ICL)
**        Created.
**
**  VMS History:
**	31-Aug-1995 (mckba01 CA)
**		Initial VMS port.
*/

STATUS
EVSetDeleteFile(id,file)
i4 id;
i4 file;
{
    	PTR  		ii_exception;
    	char 		buffer[EVSET_MAX_PATH];
    	EVSET_ENTRY 	details;
    	FILE 		*contents;
    	LOCATION 	loc;
    	i4		count;

    	/* Find the except directory */

    	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
       	 	return(E_NO_II_EXCEPTION);
    	}

    	/* Open the CONTENTS file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS.",id);

    	if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    	{
		return(E_UNKNOWN_EVSET);
    	}

    	if (SIfopen(&loc,"rw",SI_RACC,sizeof(details),&contents)!=OK)
    	{
        	return(E_UNKNOWN_EVSET);
    	}

    	/* Find the EVSET_ENTRY structure */

        if (SIfseek(contents,sizeof(EVSET_DETAILS)+((file+1)*sizeof(EVSET_ENTRY)),SI_P_START) != OK)
        {
            	SIclose(contents);
            	return(E_UNKNOWN_EVSET);
        }
    
        if(SIread(contents,sizeof(details),&count,(char *)&details) !=OK)
        {
            	SIclose(contents);
            	return(E_UNKNOWN_EVSET);
        }
     
	if(details.flags == EVSET_TYPE_VAR)
	{
		SIclose(contents);
		return(E_UNKNOWN_EVSET);
	}

	if(details.flags == EVSET_DELETED)
	{
		SIclose(contents);
		return(OK);
	}

    	/* Delete the underlying file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]FILE%d.",id,file);

    	if (LOfroms(PATH&FILENAME,buffer,&loc) == OK)
    	{
		if(LOexist(&loc) == OK)
		{
    			if(LOdelete(&loc) != OK)
    			{
        			SIclose(contents);
        			return(E_DELETE_FAIL);
                      	}
		}
    	}

    	/* Mark as deleted */

    	details.flags=EVSET_DELETED;

    	if (SIfseek(contents,-sizeof(details),SI_P_CURRENT) != OK)
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
**  Name: EVSetLookupVar - Lookup the value of a variable
**
**  Description:
**      This routine looks up a named variable in the CONTENTS file of a 
**      specified evidence set and returns its value if present. If the variable
**      is not found the string "unknown" is returned.
**
**  Inputs:
**      i4 id              Evidence set identifier
**      char *name          Name of variable to lookup
**      char *value         Place to return value
**      i4 len             Length of above
**
**  Outputs:
**      int *value          Updated with value of variable (or "unknown" if
**                          not found)
**
**  Returns:
**      0                   File details obtained
**      E_UNKNOWN_EVSET     Unknown evidence set
**      <>0                 Other error
**
**  History:
**      11-May-1994 (paulb ICL)
**          Created.
**      13-Jun-1994 (paulb ICL)
**          Fix problem where structure was passed by value due to missing &
**
**  VMS History:
**	31-Aug-1995 (mckba01 CA)
**		Initial VMS Port.
*/

STATUS
EVSetLookupVar(id,name,value,len)
i4 id;
char *name;
char *value;
i4 len;
{
    	PTR  		ii_exception;
    	char 		buffer[EVSET_MAX_PATH];
    	PTR  		my_value="unknown";
    	EVSET_ENTRY 	details;
    	FILE  		*contents;
    	LOCATION 	loc;
    	i4		count;

    	/* Find the except directory */
    
	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
        	return(E_NO_II_EXCEPTION);
    	}

    	/* Open the CONTENTS file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS.",id);

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

    	if(STlength(my_value) < len)
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
**     0                 Success
**     E_UNKNOWN_EVSET   Unknown evidence set
**     <>0               Other errors
**
**  History:
**     24-Aug-1993 (paulb ICL)
**        Created.
**
**  VMS History:
**	31-Aug-1995 (mckba01 CA)
**		Initial VMS Port.
*/

STATUS
EVSetDelete(id)
i4 id;
{
    	char 		evdir[EVSET_MAX_PATH];
	char		sspec[EVSET_MAX_PATH];
    	PTR 		ii_exception;
	LOCATION	loc;   
	u_i4	Context = 0;

	/* descriptors for LIB$FIND_FILE */

	$DESCRIPTOR		(Resultant,evdir);
	$DESCRIPTOR		(Filespec,sspec);

    	STprintf(sspec,"II_EXCEPTION:[ingres.except.EVSET%03d]*.*;*",id);
	Filespec.dsc$w_length = STlength(sspec);                           



    	/* Locate the evidence set directory */

    	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
       		return(E_NO_II_EXCEPTION);
    	}
                           


    	STprintf(evdir,"II_EXCEPTION:[ingres.except.EVSET%03d]",id);
                           
	/* 
		check directory exists 
	*/

    	if (LOfroms(PATH,evdir,&loc) != OK)
    	{
		return(E_UNKNOWN_EVSET);
    	}

    	if(LOexist(&loc))
    	{
        	return(E_UNKNOWN_EVSET);
    	}

	/*
		delete all files in the
		directory
	*/
  
    	if(LOdelete(&loc)!= OK)
       		return(E_DELETE_FAIL);

    	return(OK);
}




/*{
**  Name: EVSetExport - Export an evidence set to a named file
**
**  Description:
**      This routine exports the contents of the named evidence set into
**      a saveset file with the name passed
**
**      This is simply done by constructing a backup command
**
**
**  Inputs:
**     int id         - Evidence set identifier
**     char *filename - Name of file to export to
**
**  Returns:
**     0                     Export file sucessfully written
**     E_UNKNOWN_EVSET       Unknown evidence set
**     E_EXPORT_CREATE       Unable to create export file
**     <>0                   Other failure
**
**  History:
**     24-Aug-1993 (paulb ICL)
**        Created.
**     16-05-95 (prida01)
**		Using SI and LO functions from CL
**  VMS History:
**	08-Sep-1995 (mckba01 CA)
**		Initial VMS Port.
*/

STATUS
EVSetExport(id,filename)
i4 id;
PTR filename;
{
    FILE 		*pipe;
    FILE 		*export;
    char 		buffer[EVSET_MAX_PATH+30];
    PTR 		ii_exception;
    i4 		count;
    LOCATION		loc;
    i4			flagword;
    i4			no_bytes;
    LOINFORMATION 	loinfo;
 	CL_ERR_DESC	err_code;


    /* Find except directory */

    NMgtAt("II_EXCEPTION",&ii_exception);

    if(ii_exception==NULL || *ii_exception==EOS)
    {
        return(E_NO_II_EXCEPTION);
    }

    /* Check evidence set exists */

    STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS.",id);

    /* Stat file */
    if (LOfroms(PATH&FILENAME,buffer,&loc) != OK)
    {
	return(E_EXPORT_CREATE);
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
		return(E_EXPORT_CREATE);
	}
    }

    /* Build up backup command */

    STprintf(buffer,"backup II_EXCEPTION:[ingres.except.EVSET%03d]*.* %s/save_set",id,filename);
             
    /* Execute backup command */
    
    if(PCcmdline(NULL,buffer,PC_WAIT,NULL,&err_code) != OK)
    {
	return(E_EXPORT_CREATE);
    }

    return(OK);
}

/*{
**  Name: EVSetImport - Import an evidence set from a file
**
**  Description:
**      This routine is supplied for use on support machines and provides
**      a means of importing an evidence set exported by EVSetExport
**
**      A new evidence set is created, the export file read into it
**      (overwriting the CONTENTS file) and the CONTENTS file fixed up to
**      refer to its new evidence set identifier and directory name
**
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
**      0                      Evidence set sucessfully imported
**      E_OPEN_FAIL            Fail to open or read exportfile
**      <>0                    Other error
**
**  History:
**     27-Aug-1993 (paulb ICL)
**        Created.
**  VMS History
**	
**	08-Sep-1995 (mckba01 CA)
**		Initial VMS port.
*/

STATUS
EVSetImport(filename,id)
PTR filename;
i4 *id;
{
    	STATUS		status;
    	PTR		ii_exception;
    	char 		buffer[EVSET_MAX_PATH+30];
    	FILE 		*export;
    	FILE 		*cpio;
    	LOCATION 	loc;
    	i4		no_bytes;
	CL_ERR_DESC	err_code;   
       	EVSET_DETAILS details;
	i4 count;


    	/* Find except directory */
    	NMgtAt("II_EXCEPTION",&ii_exception);

    	if(ii_exception==NULL || *ii_exception==EOS)
    	{
        	return(E_NO_II_EXCEPTION);
    	}

    	/* Create a new evidence set */

    	if(EVSetCreate(id,"IMPORT","???") != OK)
		return(E_OPEN_FAIL);

    	
    	
        /* check the import file exists */

	if (LOfroms(PATH&FILENAME,filename,&loc) != OK)
	{	
	    	if (LOfroms(FILENAME,filename,&loc) != OK)
	    	{
		    	return(E_OPEN_FAIL);
	    	}
	}
                                            
        /* Construct and execute the backup command. */

    	STprintf(buffer,"backup %s/SAVE_SET/REPLACE II_EXCEPTION:[ingres.except.EVSET%03d]",filename,*id);

        if(PCcmdline(NULL,buffer,PC_WAIT,NULL,&err_code) != OK)
    	{
         	EVSetDelete(*id);
       		return(E_WRITE_FAILED);
      	}
            
    	/* If all is OK upto here - fixup the CONTENTS file to refer to its */
    	/* new evidence set identifier and location in filestore            */
 	
	/* Open the CONTENTS file */

    	STprintf(buffer,"II_EXCEPTION:[ingres.except.EVSET%03d]CONTENTS.",*id);
        
	if (LOfroms(PATH & FILENAME,buffer,&loc) != OK)
	{
		return(E_OPEN_FAIL);
	}                                            
    
	if (SIfopen(&loc,"rw",SI_RACC,sizeof(details),&export) != OK)
        {
            	EVSetDelete(*id);
            	return(E_EVSET_CORRUPT);
        }
	else if(SIread(export,sizeof(details),&count,(char *)&details) != OK)
        {
            	EVSetDelete(*id);
            	SIclose(export);
            	return(E_EVSET_CORRUPT);
        }
        else
        {
            	details.id = (*id);

            	/* Write back modified EVSET_DETAILS structure */

	    	if (SIfseek(export,0,SI_P_START) != OK)
            	{
                	EVSetDelete(*id);
                	SIclose(export);
                	return(E_WRITE_FAILED);
            	}
	    	else if (SIwrite(sizeof(details),(char *)&details,&no_bytes,
			    export) != OK)
            	{
                	EVSetDelete(*id);
                	SIclose(export);
                	return(E_WRITE_FAILED);
            	}
            	else
            	{
                	/* Modify path name in all EVSET_ENTRY file entries  */

                	i4 file;
			i4 file_no;
                	EVSET_ENTRY fdetails;

			file_no = -1;
                	for(file=0;;file++)
                	{
		    		if (SIfseek(export,sizeof(EVSET_DETAILS)+(file*sizeof(EVSET_ENTRY)),SI_P_START) != OK)
                    		{
                        		EVSetDelete(*id);
                        		return(E_WRITE_FAILED);
                    		}
		    		else if(SIread(export,sizeof(fdetails),
				     &no_bytes,(char *)&fdetails) != OK)
                    		{
                       			break;
                    		}
                    		else if(fdetails.type==EVSET_TYPE_FILE)
                    		{
					file_no++;
                        		STprintf(fdetails.value,"II_EXCEPTION:[ingres.except.EVSET%03d]FILE%d.",*id,file_no);
                                    	if(SIfseek(export,sizeof(EVSET_DETAILS)+(file*sizeof(EVSET_ENTRY)),SI_P_START) !=OK)
                       		 	{ 			
                            			EVSetDelete(*id);
						SIclose(export);
                            			return(E_WRITE_FAILED);
                        		}
					else if (SIwrite(sizeof(fdetails),
						   (char *)&fdetails,&no_bytes,
						   export)!=OK)
                        		{
                            			EVSetDelete(*id);
						SIclose(export);
                            			return(E_WRITE_FAILED);
                        		}
                    		}
                	}
			SIclose(export);
            	}
        }
    	return(OK);
}
               


/*{
**  Name: ev_handler - exception handler
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
**     24-Aug-1993 (paulb ICL)
**        Created.
*/

static STATUS
ev_handler(ex_args)
EX_ARGS *ex_args;
{
    return(EXDECLARE);
}
