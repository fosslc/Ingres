/*
**    Copyright (c) 1995, 2000 Ingres Corporation
*/
#include <compat.h>
#include <st.h>
#include <ex.h>
#include <pc.h>
#include <lo.h>
#include <nm.h>
#include <er.h>
#include <evset.h>
#include <si.h>
#include <id.h>
#include <tr.h>
#include <rms.h>
#include <lib$routines.h>
#include <me.h>
#include <starlet.h>

/**
**  Name: excore - Problem management server
**
**  Description:
**      This module provides the INGRES problem management server.
**	This module contains functions responsible for collecting the
**      appropriate evidence and reporting the problem to the operator.
**
**      This is run (with blocked asts) to avoid any conditions in the
**      server preventing us collecting the required evidence. The calling
**      process is assumed to be the server in error.
**
**      Failures from EXEC problem database instructions will be logged to the
**      evidence file created for their output.
**
** History:
**	21-AUG-1995 (mckba01)
**		Initial VMS Port. Created from original iigcore.c (UNIX CL)
**	13-Oct-1995 (mckba01)
**		Chnaged exception handler name to avoid linker warnings.
**	20-Oct-1995 (mckba01)
**		Initialise RMS blocks to Zeros when allocating.
**      07-Jan-1997 (horda03)
**              Corrected spelling mistakes and add CR to TRdisplay.
**      18-May-1998 (horda03)
**         Ported to OI 1.2/01.
**      24-Nov-1998 (horda03)
**         Bug 94281 - Remove any Event Handlers defined within a function
**         before returning from the function.
** 	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	   Corrent number of arguments to erlookup
**	   Add external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**      18-Feb-2009 (horda03)
**          Make the contents of the Evidence Set generated on VMS conform
**          to the details gathered on Unix. Wrote a "copy" command to
**          get the last few lines of a file using VMS function calls - this
**          can't be done using the SI facility, as text files like the ERRLOG
**          cannot be used as Random Access files.
*/


static	i4	to_hex();
static	i4	SymFileOpen();
static	i4	SymFileRead();
static	i4	SymFileWrite();
static	VOID	SymFileClose();
static	VOID	SymFileCopy();
static 	STATUS ex_handler();
static 	void alarm_handler();
static 	void do_dump();
static 	void do_exec();
static 	void do_stat();
static 	char log_message[ER_MAX_LEN];
static STATUS DIAGhandler();
VOID DIAGoutput();
VOID DIAGtr_output();
VOID DIAGerror();
static FILE *summary_file=NULL;

GLOBALREF  Version[];
GLOBALREF VOID                (*Ex_diag_link)();

typedef struct
{
	struct FAB       *SYMfab;
	struct RAB       *SYMrab;
	struct NAM       *SYMnam;
} FileCB;

FUNC_EXTERN VOID CS_DIAG_dump_stack();
FUNC_EXTERN VOID CS_DIAG_dump_query();


/*{
**  Name: execute - Execute a command and put its output in evidence set
**
**  Description:
**     This routine executes the given command and puts its output into a new
**     evidence set file with the given title.
**
**     If an evidence set file with the given title already exists then it is
**     displayed.
**
**  Inputs:
**     i4  id              Evidence set id
**     PTR command       Comamnd to execute
**     PTR title         Title of evidence set file
**
**  History:
*/

static
execute(id,command,title)
i4  id;
PTR command;
PTR title;
{
   char buffer[OUTPUT_BUFFER];
   EVSET_ENTRY fdetails;
   CL_ERR_DESC err_code;
   LOCATION my_loc;
   STATUS status;
   FILE *fdes;
   i4  file=0;

      if(status = EVSetCreateFile(id,EVSET_SYSTEM|EVSET_TEXT,title,
                fdetails.value,sizeof(fdetails.value))!=OK)
      {
         DIAGoutput("Failure 0x%x",status);
         DIAGoutput("Fail to create evidence set file \"%s\"", title);
         return;
      }

      if(LOfroms(PATH&FILENAME,fdetails.value,&my_loc)==OK &&
         SIfopen(&my_loc,"w",SI_TXT,2048,&fdes)==OK)
      {
          /* THe above SIFopen() succeeded, so the file was created.
          ** Close the stream and delete the file, as PCcmdline() will
          ** generate a new file for us.
          */
          SIclose(fdes);

          LOdelete(&my_loc);

          PCcmdline(NULL,command,PC_WAIT,&my_loc,&err_code);
      }



}
/*{
**  Name: copy - Copy a file (or end of a file) to an evidence set
**
**  Description:
**      This routine copies the contents of a file or the last part of a file
**      into a named evidence set file and echos the data to the screen.
**
**      If the lines parameter is supplied (ie not -1) then this routine scans
**      backwards counting newlines until it reaches the start of the file or
**      the required number of lines. The copy is then done from this point.
**
**      If an evidence set file already exists with the given name then this is
**      instead displayed to the screen.
**
**  Inputs:
**      i4      id              Evidence set id
**      LOCATION *location      Location of file to copy
**      PTR     title           Title for evidence set file
**      i4      lines           Lines to copy (from end of file) or -1 for all
**
**  History:
**      08-May-12 (wanfr01)
**          Bug 120370
**          Remove unneeded set to zero which can cause a memory overwrite.
*/

#define MAX_RECORD_LENGTH 32768

typedef struct
{
   i2 rfa [3];
}
COPY_REC_ST;

char copy_buffer[MAX_RECORD_LENGTH];
static
copy(id,location,title,lines)
i4  id;
LOCATION *location;
PTR title;
i4  lines;
{
   LOCATION my_loc;
   EVSET_ENTRY fdetails;
   FILE *source=NULL;
   FILE *dest=NULL;
   i4  len;
   char *filename;
   struct FAB    fab = cc$rms_fab;
   struct RAB    rab = cc$rms_rab;
   struct XABFHC xab = cc$rms_xabfhc;
   COPY_REC_ST *copy_rec = NULL;
   STATUS      status;

   /* If we didn't find it or couldn't open it create a new one */
   /* and use the caller specified file as the source           */

      if(EVSetCreateFile(id,EVSET_SYSTEM|EVSET_TEXT,title,fdetails.value,
         sizeof(fdetails.value))!=OK)
      {
         DIAGoutput("Failed");
         return;
      }

      LOtos( location, &filename);
      fab.fab$l_fna = filename;
      fab.fab$b_fns = STlength( filename );
      fab.fab$b_rfm = FAB$C_VFC;
      fab.fab$b_shr = FAB$M_SHRPUT | FAB$M_SHRGET;
      fab.fab$b_fac = FAB$M_GET;
      fab.fab$l_xab = &xab;

      rab.rab$l_fab = &fab;
      rab.rab$l_rop = RAB$V_NLK;
      rab.rab$l_ubf = copy_buffer;
      rab.rab$w_usz = MAX_RECORD_LENGTH;

      if( LOfroms(PATH&FILENAME,fdetails.value,&my_loc)!=OK ||
          SIfopen(&my_loc,"w",SI_TXT,OUTPUT_BUFFER,&dest)!=OK ||
          (sys$open( &fab ) & 1) == 0 ||
          (sys$connect( &rab ) & 1) == 0)
      {
         DIAGoutput("Fail to open files");

         if ( dest )
         {
            SIclose( dest );
            sys$close( &fab );
         }

         return;
      }

      if(lines!= -1)
      {
         /* Position to required start position */

         i8  offset;
         i4  pos;
         i8  idx;

         /* Find the last LINES lines of the file */

         copy_rec = (COPY_REC_ST *) MEreqmem( 0, sizeof(COPY_REC_ST) * lines, FALSE, &status );

         if (!copy_rec)
         {
            SIclose( dest );
            sys$close( &fab );

            DIAGoutput("Failed allocating COPY buffer");

            return;
         }

         for(offset = 0; sys$find( &rab ) & 1; offset++)
         {
            for(pos = 0; pos < 3; pos++)
            {
               copy_rec [offset % lines].rfa [pos] = rab.rab$w_rfa [pos];
            }
         }

         idx = offset - lines;

         if (idx < 0) idx = 0;

         idx %= lines;

         rab.rab$b_rac = RAB$C_RFA;

         for( pos = 0; pos < 3; pos++)
         {
            rab.rab$w_rfa [pos] = copy_rec [idx].rfa [pos];
         }

         if (copy_rec) MEfree(copy_rec);

         status = sys$find( &rab );
      }

   rab.rab$b_rac = RAB$C_SEQ;

   /* Copy the data over */

   while(sys$get( &rab ) & 1)
   {
      if (rab.rab$w_rsz < MAX_RECORD_LENGTH-1)
      {
         copy_buffer [rab.rab$w_rsz] = '\n';
         copy_buffer [rab.rab$w_rsz+1] = '\0';
         rab.rab$w_rsz++;
      }

      len = rab.rab$w_rsz;

      SIwrite(len,copy_buffer,&len,dest);

      if (lines > 0)
      {
         if (--lines == 0) break;
      }
   }

   sys$close(&fab);
   SIclose(dest);
}

/*{
**  Name: EXcore - Main interface to problem management server
**
**  Description:
**      This is the main interface to the problem management system
**
**  Inputs:
**      int ErrNum         Error number in decimal
**
**  Side effects:
**
**  History:
**	21-8-1995 (mckba01 CA)
**		Initial VMS port.
**      24-Nov-1998 (horda03)
**         Bug 94281 - Remove any Event Handlers defined within a function
**         before returning from the function.
*/
VOID
EXcore(ErrNum)
i4  ErrNum;
{
    	EX_CONTEXT ex;
    	CL_ERR_DESC err_desc;
    	char name[32];
    	PTR  name_ptr=name;
   	char my_description[80];
        char        *summary_name;
    	i4 evset;
        i4 len;
        i4  status;
    EVSET_DETAILS details;
    EVSET_ENTRY fdetails;
    	LOCATION my_loc;
    	FILE *iiexcept;
    	char buffer[200];
        bool        do_dmf_stats = FALSE;
        bool        do_psf_stats = FALSE;
        bool        do_qsf_stats = FALSE;
        i4   id;

    	/*
		If ingres isn't running this then bomb out or we get
		permissions errors on the evidence sub-directories
		VMS ingres accounts can have installation
		appened on name, they usually start with ingres
	*/

    	IDname(&name_ptr);


    	/*
		At this stage any problems will cause a syslog message to be
    		logged, there is not a lot more we can do
   	*/

    	if(EXdeclare(ex_handler,&ex)!=OK)
    	{
        	EXdelete();
       	 	return;  /* FAILURE  */
    	}


    	/*
		Now try and build a description for the evidence set we are
	*/

	STcopy("Unknown error",my_description);

        /* Ask ER for the name of the error */

        if(ERlookup(ErrNum,NULL,ER_NAMEONLY,NULL,my_description,
           sizeof(my_description),-1,&len,&err_desc,0,NULL)!=OK)
        {
            STprintf(my_description,"E_%x",ErrNum);
        }

    	if(EVSetCreate(&evset,my_description,(PTR)Version))
    	{
        	/* Cannot create an evidence set */
                TRdisplay("Cannot create EVset\n");
		EXdelete();
        	return;	/*	FAILURE	*/
    	}

    	/*
		From here on we have an evidence set - the default action
		is now to take a dump to it and log a syslog message
    	*/

    status=EVSetCreateFile(evset,EVSET_TEXT|EVSET_SYSTEM,"Evidence summary",
          fdetails.value,sizeof(fdetails.value));
    if(status!=OK)
    {
        TRdisplay("Can create evidence summary\n");
    }
    else
    {
        summary_name = STalloc(fdetails.value);
    }
    if (summary_name != NULL)
    {
        if(LOfroms(PATH&FILENAME,summary_name,&my_loc) == OK)
             SIopen(&my_loc,"w",&summary_file);
    }
    else
    {
        TRdisplay("DIAG: Failed to create summary file\n");
        return;
    }

    id = evset;
    if ((status=EVSetList(&id,&details)))
    {
        TRdisplay("EVSetList Error\n");
    }

    DIAGoutput("INGRES EVIDENCE SUMMARY");
    DIAGoutput("=======================");
    DIAGoutput("Description:   %s",my_description);
    DIAGoutput("Version:       %s",Version);
    TMstr(&details.created,buffer);
    DIAGoutput("Creation date: %s\n",buffer);

	/* Open problem database file (if it exists) */

    	if(NMloc(FILES,FILENAME,"iiexcept.opt",&my_loc)==OK &&
       	SIfopen(&my_loc,"r",SI_TXT,sizeof(buffer),&iiexcept)==OK)
    	{
        	while(SIgetrec(buffer,sizeof(buffer),iiexcept)==OK)
        	{
            		u_i4 	from=0;
            		u_i4 	to=0;
            		i4	c;

            		/* Remove the newline and decode */

            		buffer[STlength(buffer)-1]=EOS;
            		for(c=0;c<8;++c)
                		from=(from*16)+	to_hex(&buffer[c]);
            		for(c=8;c<16;++c)
                		to=(to*16)+to_hex(&buffer[c]);

            if (EV_DMF_DIAGNOSTICS >=from && EV_DMF_DIAGNOSTICS <= to && buffer[16] == 'D')
                do_dmf_stats = TRUE;
            if (EV_QSF_DIAGNOSTICS >=from && EV_QSF_DIAGNOSTICS <= to && buffer[16] == 'D')
                do_qsf_stats = TRUE;
            if (EV_PSF_DIAGNOSTICS >=from && EV_PSF_DIAGNOSTICS <= to && buffer[16] == 'D')
                do_psf_stats = TRUE;

            		if(ErrNum >= from && ErrNum <= to)
            		{
                		switch(buffer[16])
                		{
                    		case 'D':
                        		do_dump(evset);
                        		break;
                    		case 'E':
                        		do_exec(evset,&buffer[17]);
                        		break;
                		}
            		}
        	}
       	 	SIclose(iiexcept);
    	}

	EXdelete();

    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        DIAGoutput
        ("----------------------- ERRLOG.LOG ------------------------\n");

        if(NMloc(FILES,FILENAME,"errlog.log",&my_loc)==OK)
            copy(evset,&my_loc,"Errlog.log file",100);
        DIAGoutput
        ("CREATED OK\n");
    }
    EXdelete();

   if(EXdeclare(DIAGhandler,&ex)==OK)
   {
        DIAGoutput
        ("----------------- ENVIRONMENT VARIABLES -------------------\n");
        execute(evset,EVSET_SHOW_ENV_CMD,"Environment variables");
        DIAGoutput
        ("CREATED OK\n");
   }
   EXdelete();


    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        DIAGoutput
        ("----------------------- CONFIG.DAT ------------------------\n");

        if(NMloc(FILES,FILENAME,"config.dat",&my_loc)==OK)
            copy(evset,&my_loc,"Config.dat file",-1);
        DIAGoutput
        ("CREATED OK\n");
    }
    EXdelete();


    if (do_dmf_stats == TRUE) do_stats(evset,"DMF STATS",DMF_SELF_DIAGNOSTICS);
    if (do_qsf_stats == TRUE) do_stats(evset,"QSF STATS",QSF_SELF_DIAGNOSTICS);
    if (do_psf_stats == TRUE) do_stats(evset,"PSF STATS",PSF_SELF_DIAGNOSTICS);

    DIAGoutput(NULL);
    SIclose(summary_file);
	return; /* SUCCESS */
}


/*{
**  Name: do_dump - Take a dump of a process to an evidence set
**
**  Description:
**      This routine takes a core dump of a specified process to a specified
**      evidence set.
**
**      This is done by creating a new file in the evidences set and then
**      executing the gcore(1) command with the filename returned.
**      This results in a file with the name <filename>.<pid> which is renamed
**      back to the required <filename>.
**
**  Inputs:
**      int evset       Evidence set to dump to
**
**  Exceptions:
**
**  History:
**	21-AUG-1995 (mckba01)
**      	Initial VMS port.
*/

static VOID
do_dump(evset)
i4 evset;
{
	EX_CONTEXT	ex;
	char 		buffer[(EVSET_MAX_PATH*3)];
	char 		*iisystem;
    	char 		filename[EVSET_MAX_PATH];
	CL_ERR_DESC 	err_code;

	/*
		Get DBMS symbol table as part of the evidence set.
		This allows us to build a symbolic stack trace
		for each thread.
	*/
	TRdisplay("dumping file\n");
	if(EXdeclare(ex_handler,&ex) == OK)
	{
    		STprintf(buffer,"SYMBOL table file");

    		if(EVSetCreateFile(evset,EVSET_BINARY|EVSET_SYSTEM,buffer,filename,
        		sizeof(filename)) == OK)
    		{
    			/*
				Copy Symbol table to correct location and name
			*/
			NMgtAt("II_SYSTEM",&iisystem);

			if((iisystem != NULL) && (*iisystem != EOS))
			{
				/*
					Do copy
				*/
                                TRdisplay("Copying symbol table \n");
				SymFileCopy("II_SYSTEM:[INGRES.DEBUG]IIDBMS.STB",filename);

				/*
					Ignore return value, well have a normal
					thread stack dump regardless
				*/
			}
		}
	}
	EXdelete();
	/*
		Now get the stack dump of each thread.
	*/
	if(EXdeclare(ex_handler,&ex) == OK)
	{
           Ex_diag_link(DIAG_DUMP_STACKS,DIAGtr_output,DIAGoutput,(PTR)NULL);
        }
        else
        {
           DIAGoutput("Error occurred while dumping stacks Ex_diag_link 0x%x",Ex_diag_link);
           DIAGoutput("To get more information set dump on fatal");
	}
	EXdelete();
	/*
		Get the current query
	*/
	if(EXdeclare(ex_handler,&ex) == OK)
	{
           DIAGoutput
              ("----------------------- OPEN TABLES -----------------------\n");
           Ex_diag_link(DMF_OPEN_TABLES,DIAGoutput,DIAGerror,NULL);
           DIAGoutput
              ("---------------------- CURRENT QUERY ----------------------\n");
           Ex_diag_link(SCF_CURR_QUERY,DIAGoutput,DIAGerror,NULL);
	}
	EXdelete();
}


static VOID
do_stats(evset,name,type)
i4      evset;
char    *name;
i4      type;
{
    i4  status;
    char filename[EVSET_MAX_PATH];
    EX_CONTEXT ex;

    status=EVSetCreateFile(evset,EVSET_TEXT|EVSET_SYSTEM,name,filename,sizeof(filename));
    DIAGoutput
    ("----------------------- %s -----------------------\n",name);

    if (status!=OK)
    {
        TRdisplay("Cannot get stats for %s\n",name);
        return;
    }

    if(EXdeclare(DIAGhandler,&ex)==OK)
    {
        Ex_diag_link(type,DIAGoutput,DIAGerror,filename);
        DIAGoutput("CREATED OK");
    }
    else
        DIAGoutput("Error creating");
    EXdelete();

}

/*{
**  Name: do_exec - Execute an evidence collection script
**
**  Description:
**      This routine executes an evidence collection script putting its stdout
**      into a new file in the evidence set
**
**  Inputs:
**      int evset       Evidence set to put output in
**      char *script    Script to execute
**
**  Exceptions:
**      All failures are contained via an exception handler
**
**  History:
**     14-Sep-1993 (paulb ICL)
**        Created.
*/

static VOID
do_exec(evset,script)
i4 evset;
PTR script;
{
    EX_CONTEXT ex;
    char title[80];
    char filename[EVSET_MAX_PATH];

    /* Scripts are given 5 minutes to execute and then aborted */
/*
    alarm(5*60);
*/
    TRdisplay("Doing exec of file\n");
    if(EXdeclare(ex_handler,&ex)!=OK)
    {
        EXdelete();
    }
    else
    {
        STcopy("EXEC ",title);
        STlcopy(script,&title[5],STlength(script)>74?74:STlength(script));

        if(EVSetCreateFile(evset,EVSET_SYSTEM|EVSET_TEXT,title,filename,
           sizeof(filename))==OK)
        {
            CL_ERR_DESC err_code;
            LOCATION my_loc;

            LOfroms(PATH & FILENAME,filename,&my_loc);
            PCcmdline(NULL,script,PC_WAIT,&my_loc,&err_code);
        }

        EXdelete();
    }
}

/*{
**  Name: ex_handler - exception handler
**
**  Description:
**     This routine is used as an exception handler in various routines in
**     this module. We are not interested in what the exception is just that
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
ex_handler(ex_args)
EX_ARGS *ex_args;
{
    return(EXDECLARE);
}

/*{
**  Name: alarm_handler - Exception handler for alarm signals
**
**  Description:
**      This exception handler is called if our alarm clock expires and simply
**      calls EXsignal to raise an EX exception - this will be picked up by
**      the standard exception mechanism in this module
**
**  Inputs:
**      int dummy
**
**  History:
**     17-Sep-1993 (paulb ICL)
**        Created.
*/

static void
alarm_handler(dummy)
i4 dummy;
{
    EXsignal(EXKILL,0);
}

/*{
**  Name: to_hex - Convert an character digit to a number 0-15
**
**  Description:
**      This routine is used to convert a character representation of
**      a hex digit into a number in the range 0-15
**
**  Inputs:
**      char *pointer       Pointer to the digit to convert
**
**  Returns:
**      -1       Invalid hex digit
**     0-15      Value of hex digit
**
**  History:
**     08-Sep-1993 (paulb ICL)
**        Created.
*/

static i4
to_hex(pointer)
PTR pointer;
{
    i4 ret_val;

    switch(*pointer)
    {
       case '0':
           ret_val=0;
           break;
       case '1':
           ret_val=1;
           break;
       case '2':
           ret_val=2;
           break;
       case '3':
           ret_val=3;
           break;
       case '4':
           ret_val=4;
           break;
       case '5':
           ret_val=5;
           break;
       case '6':
           ret_val=6;
           break;
       case '7':
           ret_val=7;
           break;
       case '8':
           ret_val=8;
           break;
       case '9':
           ret_val=9;
           break;
       case 'a':
       case 'A':
           ret_val=10;
           break;
       case 'b':
       case 'B':
           ret_val=11;
           break;
       case 'c':
       case 'C':
           ret_val=12;
           break;
       case 'd':
       case 'D':
           ret_val=13;
           break;
       case 'e':
       case 'E':
           ret_val=14;
           break;
       case 'f':
       case 'F':
           ret_val=15;
           break;
       default:
          ret_val= -1;
    }

    return(ret_val);
}


/*{
**  Name: SymFileCopy -  Copy a symbol table file.
**
**  Description:
**	Opens a VMS symbol table file. reads records from
**	it and writes them to a destination file.
**
**  Inputs
**	PTR	InFileName	Filename of source symbol table.
**	PTR	OutFileName	Filename of destination file.
**
**  Side effects:
**
**  History:
**	10-9-1995 (mckba01 CA)
**		Initial VMS version.
**
*/
static
VOID
SymFileCopy(InFileName, OutFileName)
{
	FileCB	InFile;
	FileCB	OutFile;
	char	RecBuf[600];
	i4	Size;

	if(SymFileOpen(InFileName,0,&InFile))
		return;

	if(SymFileOpen(OutFileName,1,&OutFile))
		return;

	while(SymFileRead((PTR) &RecBuf, &Size, &InFile) == 0)
		SymFileWrite((PTR) &RecBuf, Size, &OutFile);

	SymFileClose(InFile);
	SymFileClose(OutFile);
}


/*{
**  Name: SymFileOpen -  Open a symbol table file.
**
**  Description:
**	Opens a VMS symbol table file. File can be opened
**	for read or write depending upon the argument
**	ReadWrite.  Symbol table files are RMS
**	variable length records, sequential file.
**
**  Inputs
**	PTR	SymFileName	Buffer holding filename to open
**	i4	ReadWrite 	0 = open for read, 1 = open for write (create)
**	FileCB	*Fdesc		Structure to hold initialised RMS control
**				blocks.
**
**  Returns
**	0	SUCCESS
**	1	FAIL
**
**  Side effects:
**
**  History:
**	10-9-1995 (mckba01 CA)
**		Initial VMS version.
**
*/
static
i4
SymFileOpen(Sym_Tab_Name, ReadWrite, Fdesc)
PTR	Sym_Tab_Name;
i4	ReadWrite;
FileCB	*Fdesc;
{
	i4	RetVal;

	/* Allocate FAB	*/

	Fdesc->SYMfab = (struct FAB *) MEreqmem(0, (u_i4) sizeof (struct FAB), TRUE, NULL);
	Fdesc->SYMnam = (struct NAM *) MEreqmem(0, (u_i4) sizeof (struct NAM), TRUE, NULL);
	Fdesc->SYMrab = (struct RAB *) MEreqmem(0, (u_i4) sizeof (struct RAB), TRUE, NULL);

	/* Initialize FAB, with file name */

	Fdesc->SYMfab->fab$b_bid = FAB$C_BID;
	Fdesc->SYMfab->fab$b_bln = FAB$C_BLN;
	Fdesc->SYMfab->fab$b_fns = STlength(Sym_Tab_Name);
	Fdesc->SYMfab->fab$l_fna = Sym_Tab_Name;

	if(ReadWrite)
	{
		/*	Its a write open	*/

        	Fdesc->SYMfab->fab$b_fac = FAB$M_PUT;
	}
	else
	{
		/*	Allow read access only	*/

		Fdesc->SYMfab->fab$b_fac = FAB$M_GET;
	}

	/*	Initialize FAB record details	*/

	Fdesc->SYMfab->fab$b_org = FAB$C_SEQ;
	Fdesc->SYMfab->fab$b_rfm = FAB$C_VAR;
	Fdesc->SYMfab->fab$b_rat = NULL;
	Fdesc->SYMfab->fab$w_mrs = 512;

	/*	Initialize RAB details	*/

	Fdesc->SYMrab->rab$b_bid = RAB$C_BID;
	Fdesc->SYMrab->rab$b_bln = RAB$C_BLN;
	Fdesc->SYMrab->rab$l_fab = Fdesc->SYMfab;
	Fdesc->SYMrab->rab$b_rac = RAB$C_SEQ;

	Fdesc->SYMnam->nam$b_bid = NAM$C_BID;
	Fdesc->SYMnam->nam$b_bln = NAM$C_BLN;
	Fdesc->SYMnam->nam$l_esa = MEreqmem(0, (u_i4) NAM$C_MAXRSS, TRUE, NULL);
	Fdesc->SYMnam->nam$b_ess = NAM$C_MAXRSS;
	Fdesc->SYMfab->fab$l_nam = Fdesc->SYMnam;


	if(ReadWrite)
		sys$create(Fdesc->SYMfab);
	else
		sys$open(Fdesc->SYMfab);

	if ((Fdesc->SYMfab->fab$l_sts & 1) == 1)
	{
         	/*	Success try to connect	*/

		if((sys$connect(Fdesc->SYMrab) & 1) == 1)
		{
			/*	return success status	*/

			if(!ReadWrite)                           			{
				/* Allocate space for user buffer area	*/

				Fdesc->SYMrab->rab$l_ubf = MEreqmem(0, (u_i4) 512, TRUE, NULL);
		       		Fdesc->SYMrab->rab$w_usz = 512;
			}

			Fdesc->SYMrab->rab$b_rac = RAB$C_SEQ;


			return(0);	/* SUCCESS */
		}
	}

	/*	Its failed if we are here, so clean
		up and return failure			*/

	sys$close(Fdesc->SYMfab);
	MEfree((PTR)Fdesc->SYMfab);
	MEfree((PTR)Fdesc->SYMrab);
	MEfree((PTR)Fdesc->SYMnam);

	return(1);  /* FAIL */
}




/*{
**  Name: SymFileRead -  Reads a record from a file
**
**  Description:
**	Reads a record from the next position
**	of a source Symbol Table File
**
**  Inputs
**	PTR	Rec_Ptr	Pointer to buffer to place record
**	i4	*Size	Number of read.
**	FileCB	Fdesc	Structure holding RMS control blocks
**			for an open file.
**
**  Returns
**	0	SUCCESS
**	1	FAIL
**
**  Side effects:
**
**  History:
**	10-9-1995 (mckba01 CA)
**		Initial VMS version.
**
*/
static
i4
SymFileRead(Rec_Ptr,Size,Fdesc)
PTR	Rec_Ptr;
i4	*Size;
FileCB	*Fdesc;
{
	if(Fdesc->SYMrab == NULL)
		return(1); /*FAIL*/


	if((sys$get(Fdesc->SYMrab) & 1) == 1)
	{

     		*Size = Fdesc->SYMrab->rab$w_rsz;
		MEcopy((PTR) Fdesc->SYMrab->rab$l_rbf,(u_i2) 512, Rec_Ptr);
		return(0);	/* SUCCESS */
	}
	else
	{
		return(1); /* FAIL */
	}
}


/*{
**  Name: SymFileWrite -  Writes record to file
**
**  Description:
**	Writes a record to the next position
**	of a destination Symbol Table File
**
**  Inputs
**	PTR	Rec_Ptr	Pointer to buffer holdin record to write
**	i4	Size	Number of bytes to write.
**	FileCB	Fdesc	Structure holding RMS control blocks
**			for an open file.
**
**  Returns
**	0	SUCCESS
**	1	FAIL
**
**  Side effects:
**
**  History:
**	10-9-1995 (mckba01 CA)
**		Initial VMS version.
**
*/
static
i4
SymFileWrite(Rec_Ptr,Size,Fdesc)
PTR	Rec_Ptr;
i4	Size;
FileCB	*Fdesc;
{
	if(Fdesc->SYMrab == NULL)
		return(1); /*FAIL*/



     	Fdesc->SYMrab->rab$w_rsz = Size;
	Fdesc->SYMrab->rab$l_rbf = Rec_Ptr;



	if((sys$put(Fdesc->SYMrab) & 1) == 1)
	{
		return(0);	/* SUCCESS */
	}
	else
	{
		return(1); /* FAIL */
	}
}



/*{
**  Name: SymFileClose -  Close file
**
**  Description:
**     	Closes a RMS stream associated with a file and
**	Frees any memory used by the RMS control blocks
**
**  Inputs
**	FileCB	Fdesc	Structure holding RMS contro blocks
**			for an open file.
**
**  Side effects:
**
**  History:
**	10-9-1995 (mckba01 CA)
**		Initial VMS version.
**
*/
static
VOID
SymFileClose(Fdesc)
FileCB	Fdesc;
{
	sys$disconnect(Fdesc.SYMfab);
	sys$close(Fdesc.SYMfab);
	MEfree((PTR)Fdesc.SYMfab);
	MEfree((PTR)Fdesc.SYMrab);
	MEfree((PTR)Fdesc.SYMnam);
}

/*{
**  Name: output - Output information to evidence summary
**
**  Description:
**     This routine acts very much like SIprintf buts it outputs the information
**     to a file in the evidence set and to stdout. Information is only output
**     to stdout if less than 128K of data has been output (which is the
**     current telservice limit).
**
**     If the supplied format string is NULL then an end of evidence summary
**     line is output containing the percentage of the evidence summary maximum
**     we have used - to enable tracking effect adding extra evidence to the
**     summary would have.
**
**  Inputs:
**     PTR format                    Printf style format string (NULL for end
**                                   of summary)
**     PTR  a1,a2,a3,a4,a5,a6,a7,a8   Parameter inserts
**
**  Side effects:
**     Internally keeps track of the amount of data output and stops sending
**     data to stdout when more than 128K would result.
**
**  History:
*/

VOID
DIAGoutput(format,a1,a2,a3,a4,a5,a6,a7,a8)
PTR format;
PTR a1;
PTR a2;
PTR a3;
PTR a4;
PTR a5;
PTR a6;
PTR a7;
PTR a8;
{
    char output_buffer[OUTPUT_BUFFER];

    if(format==NULL)
    {
        STprintf(output_buffer,
        "============= END OF EVIDENCE SUMMARY =============\n");
    }
    else
    {
         STprintf(output_buffer,format,a1,a2,a3,a4,a5,a6,a7,a8);
    }

    if (summary_file!=NULL)
    {
        SIfprintf(summary_file,"%s\n",output_buffer);
        SIflush(summary_file);
    }
}

VOID
DIAGtr_output(arg, length, buffer)
i4             arg;
i4             length;
char                *buffer;
{
    i4             count;

    if (summary_file!=NULL)
    {
        SIwrite(length, buffer, &count, summary_file);
        SIwrite(1, "\n", &count, summary_file);
        SIflush(summary_file);
    }
    else
    {
        TRdisplay("DIAG: evset_output: Cannot write to evset file:\n");
        TRdisplay("DIAG: evset_output: evset_file == NULL FILE *.\n");
    }
}

/*{
**  Name: DIAGerror - Error handler
**
**  Description:
**     Any error in any of the analysis routines calls this routine. It is
**     responsible for outputing an error to the evidence set and recovering
**
**     The recovery is actually handled by the driver which declares exception
**     handlers which are signalled by this routine
**
**  Inputs:
**     PTR format                    Printf style format string
**     PTR a1,a2,a3,a4,a5,a6         Parameter inserts
**
**  Returns:
**     Doesn't
**
**  Exceptions:
**     Raises EXKILL signal
**
**  History:
*/

VOID
DIAGerror(format,a1,a2,a3,a4,a5,a6)
PTR format;
PTR a1;
PTR a2;
PTR a3;
PTR a4;
PTR a5;
PTR a6;
{
    DIAGoutput(NULL,format,a1,a2,a3,a4,a5,a6,0,0);
}


static STATUS
DIAGhandler(ex_args)
EX_ARGS *ex_args;
{
    return(EXDECLARE);
}
