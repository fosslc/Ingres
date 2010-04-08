/*
 * accompat.c
 *
 * History:
 *      8-7-93 (JEFFR)
 *
 *   Made changes for Achilles Version 2 - ACfork() rewritten 
 *   to use lib$spawn - ACcopier is no longer needed
 *
 * 
 *      10-21-93 (JEFFR)
 *   Changed  int active_children to GLOBALDEF active_children
 *
 *	06-22-95 (duursma)
 *   Replaced use of perror() after call to lib$spawn with SIprintf() call
 *   that outputs the return code from lib$spawn.  The perror() routine
 *   is not intended to work in situations like this.
**
**      25-Mar-98 (kinte01)
**              Rename optarg to ach_optarg as symbol is already in use
**              by the DEC C shared library.
**
**      25-Mar-98 (kinte01)
**              Rename optind to ach_optind as symbol is already in use
**              by the DEC C shared library.
**      09-Jun-98 (kinte01)
**              Back out previous change as optarg is a predefined symbol on
**              Unix and I inadvertantly broke the achilles build on Unix
**              platforms. Instead replace the globaldef optarg in
**              achilles_vms!accompat.c with an include of unistd.h which
**              will also correct the problem
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	    Include VMS headers for global symbols.  Replace homegrown item
**	    list by ILE3.
*/

#include <achilles.h>	/* includes compat.h and CL headers */
#include <starlet.h>
#include <descrip.h>
#include <efndef.h>
#include <iledef.h>
#include <iodef.h>
#include <iosbdef.h>
#include <lnmdef.h>
#include <ssdef.h>
#include <sjcdef.h>
#include <jbcmsgdef.h>
#include <quidef.h>
#include <rmsdef>
#include <stat.h>
#include <message.h>
#include <unistd.h>
#include <file.h>


static PID npid;
static i4 start=1;
static i4 nchannel=0;
static i4 status;
char *tfile="ing_tst:[output.jeffr]test.log";
FILE *fpt;
globalref FILE *fptr;

char outfbuf[MAX_LOC];

 struct dsc$descriptor_s outfile = {
                0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0
        };

char mboxbuf[128];
        struct dsc$descriptor_s ncsp_mbx_name = {
                0, DSC$K_DTYPE_T, DSC$K_CLASS_S, mboxbuf
        };

static  void        read_complete();
static  void        nread_complete();

$DESCRIPTOR(input_desc,"NLA0");

VOID ACerrMsg( msg, rval )
char	    *msg;
unsigned long
	    rval;				    /* Error code */
{
	struct					    /* Putmsg messge vector */
	{
	    short   agrcnt,
		    msgopt;
	    unsigned long
		    rval;
        } msgvec = { 1, 15, rval };

	printf( "%s\n", msg );
        sys$putmsg( &msgvec, 0, 0, 0 );
	printf( "\n" );
} /* ACerrMsg */




GLOBALDEF active_children = 0;
GLOBALDEF unsigned short mboxchan;

VOID ACchildHandler (pid)
i4 *pid;
{
	register i4    i,
		       j,
		       found_it;
	unsigned long  rval;
	char	      *fname,
		       fbuf[MBOX_BUFSIZE],
		       mboxbuf[MBOX_BUFSIZE];
	struct stat    st;
	HI_RES_TIME    now;
	ACTIVETEST    *curtest;
       ACblock(ALARM|ABORT|STATRPT);

	found_it = 0;


	/* Find which child process terminated. */
	for (i = 0 ; i < numtests ; i++)
	{
		for (j = 0 ; j < tests[i]->t_nkids ; j++)
			if (tests[i]->t_children[j].a_pid == *pid)
			{
				found_it++;
				break;
			}
		if (found_it)
			break;
	}
	ACgetTime(&now, (LO_RES_TIME *) NULL);

	/* There's no reason why we should ever get info on a process
	   that's not stored in the data structures, but in case...
	*/
	if (i == numtests)
	{
		log_entry(now, -1, -1, -1, *pid, C_NOT_FOUND, 0, 0);

		ACunblock();
		return;
	}
	curtest = &(tests[i]->t_children[j]);
        /** keep running total of child times for each instance **/
      tests[i]->t_ctime+= (float)ACcompareTime(&now,&curtest->a_start)/3600;

	/** verify the state first to see if it didnt die normally **/
	if (curtest->a_status[0] % 2 == 0) tests[i]->t_cbad++   ;

	/* Log the dead process. */
	log_entry(&now, i, j, curtest->a_iters, *pid, C_EXIT,
	  curtest->a_status[0], &curtest->a_start);


	LOtos(&curtest->a_outfloc, &fname);
	/*STprintf(mboxbuf, "%s.TEMP;%d %s.LOG", fname, curtest->a_iters,
	  fname);
	if ( (rval = sys$qio(0, mboxchan, IO$_WRITEVBLK, 0, 0, 0, mboxbuf,
	  sizeof(mboxbuf), 0, 0, 0, 0) ) != SS$_NORMAL)
		lib$signal(rval);
         */
	curtest->a_iters++;
	/* If more iteration if this instance should be run, start a
	   new one.
	*/
	if ( (curtest->a_iters <= tests[i]->t_niters)
	  || (tests[i]->t_niters <= 0) ) 
	    {
		start_child(i, j, curtest->a_iters);
		/** add to total children run for this test - incl** /  
	        tests[i]->t_ctotal++;
	        active_children--;
           }
	else
	{
		/* If we don't need to start any more tests, clean up
		   the output file: If no output file was specified in
		   the config file, and the "temporary" output file
		   remained empty (ie no unexpected error messages),
		   remove it. If it shouldn't be removed, make it owned
		   by the user who's running this program. (It's owned
		   by root for now.)
		*/
		if (!*tests[i]->t_outfile)
		{
			for (j = 0 ; j < tests[i]->t_nkids ; j++)
			{
				LOtos(&tests[i]->t_children[j].a_outfloc,
				  &fname);
				STprintf(fbuf, "%s.LOG", fname);
				stat(fbuf, &st);
				if (st.st_size == 0)
					delete(fbuf);
                         
			}
		}
		/* Record the end time of the last process in a_start,
		   for use by print_status.
		*/
		curtest->a_start = now.time;
		active_children--;
	}
	/* If there are no more outstanding child processes, we're done */
	if (active_children == 0)
	{
		/** generate report **/
                ACreport();
		puts("No more active children - exiting.");
		if (logfile !=stdout)
			SIclose(logfile);

		PCexit(OK);
	}
	ACunblock();
	return;
} /* ACchildHandler */

VOID ACchown (loc, uid)
LOCATION *loc;
UID uid;
{
	char *fname;

	LOtos(loc, &fname);
	chown(fname, uid, getgid() );
} /* ACchown */


/************************** readmbox2() ****************************/

STATUS
readmbox2()

{

MESSAGE     message,*msg = &message ;
i4  count=0;
i4  i;
i4  mode;
i4  astlm;
i4  biolm;

msg->msg.data[0]='\0';

status = sys$qio(0, nchannel, IO$_READVBLK ,msg->msg_iosb, nread_complete, 
msg, &msg->msg.data, 80, 0, 0, 0, 0);

if (status != SS$_NORMAL) {
           SIfprintf(logfile,"sys$qio failed %d",nchannel);
	    lib$signal(status);
	}
return status;

}

/************************** nread_complete() *******************************/

static VOID
nread_complete(message)
MESSAGE         *message;

{
char newbuf[4096];


/*** terminate buffer message ***/
if ( (message->msg_iosb[0] >> 16) > 1) {


message->msg.data[message->msg_iosb[0] >> 16 ] = '\0';

/** put it in the logfile with new-line added **/

SIfprintf(logfile,STprintf(newbuf,"%s\n",message->msg.data));
SIflush(logfile);

}


/** post Asynch read again **/
readmbox2();

}


STATUS ACfindExec (tempdata, gooddata, data_end, argv, cur_argv)
char  *tempdata,
     **gooddata,
     **data_end,
      *argv[];
i4    *cur_argv;
{
	*gooddata = tempdata;
	return(OK);
} /* ACFindExec */

VOID ACforkChild (testnum, curtest, iternum)
i4 	    testnum;
ACTIVETEST *curtest;
i4	    iternum;
{

/*	FILE *fp;
*/	
	FILE *batfile, *inputfile;
	char *fname,  cmd_str[128], *p, *q;
	char batname[50], line[132];
	int i;
	unsigned long rval, spawn_flags = 1, pid;
	char *aptr;
	char *aname="TEMP.TXT";
/*** new mbox stuff ***/
        char cmd_buff[128];      
	char tbuf[12];
	char tmpbuf[128];
	LOCATION loc;

/*** new mbox stuff end **/

	struct dsc$descriptor_s infile = {
		0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0
	};

	struct dsc$descriptor_s *infptr;

	struct dsc$descriptor_s outdesc = {
	       0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0
        };

	struct dsc$descriptor_s *fp;

	struct dsc$descriptor_s cmd_name = {
		0, DSC$K_DTYPE_T, DSC$K_CLASS_S, cmd_str
	};
	$DESCRIPTOR(prompt, "DUMMY->");

	ACblock(ALARM|ABORT|STATRPT);

	if (*tests[testnum]->t_infile)
	{
		infile.dsc$a_pointer = tests[testnum]->t_infile;
		infile.dsc$w_length = strlen(tests[testnum]->t_infile);
		infptr = &infile;
	}
	else

		infptr = NULL;

/*** output-file **/

	LOtos(&curtest->a_outfloc, &outfile.dsc$a_pointer);
	STcopy(outfile.dsc$a_pointer, outfbuf);
	outfile.dsc$a_pointer = outfbuf;
	STprintf(outfile.dsc$a_pointer + STlength(outfile.dsc$a_pointer),
	  ".TEMP;%d", iternum);
	outfile.dsc$w_length = strlen(outfile.dsc$a_pointer);
	
        for (i = 0, p = cmd_str ; curtest->a_argv[i] ; i++)
	{
		for (q = curtest->a_argv[i] ; *q ; q++)
			*p++ = *q;
		*p++ = ' ';
	}
	*--p = '\0';


	cmd_name.dsc$w_length = (unsigned short) (p - cmd_str);

if (start) { /** create mailbox command to for child messages **/
    PCpid(&npid);
    STprintf(mboxbuf,"%d",npid);

	ncsp_mbx_name.dsc$w_length = STlength(mboxbuf);
	status = sys$crembx(0, &nchannel, 128, 1024,
                0, 0, &ncsp_mbx_name);

/*	dup2(outfile.dsc$a_pointer,1);
*/
        if (status != SS$_NORMAL) puts("crembx failed");        
        start=0; /** only create once **/
  };
        
	cmd_name.dsc$w_length = STlength(cmd_str);

	/* If there is an input file, append it */
	if ( infptr )
	{
		inputfile = fopen( infile.dsc$a_pointer, "r" );
		while ( fgets( line, 132, inputfile ) != NULL )
			fputs( line, batfile );
		fclose( inputfile );
	};

if (logfile != stdout) {

 status = lib$spawn (&cmd_name, infptr, &ncsp_mbx_name, &spawn_flags, 0 ,
         &pid,&curtest->a_status,0, ACchildHandler,&curtest->a_pid
          , 0, 0);
}

else {

status = lib$spawn(&cmd_name, infptr, &outfile, &spawn_flags, 0,
	  &pid, &curtest->a_status, 0, ACchildHandler,
	  &curtest->a_pid, &prompt, 0);
}

curtest->a_pid = pid;
if (status != SS$_NORMAL) {
    SIprintf("Status %d returned from LIB$SPAWN\n", status);
}


/*	if (iternum == 1)
		sys$suspnd(&pid, 0);	
*/
	if (curtest->a_iters == 1)
		active_children++;

} /* ACforkChild */

GLOBALDEF int ach_optind = 1;

i4 ACgetopt (argc, argv, valids)
i4 argc;
char **argv, *valids;
{
	char *p, *strchr();
	int i;

	if ( (ach_optind >= argc) || (*argv[ach_optind] != '-') )
		return(-1);
	if ( (p = strchr(valids, argv[ach_optind][1]) ) != NULL)
		if (*(p+1) == ':')
			if (argv[ach_optind][2] == '\0')
				if (++ach_optind < argc)
					optarg = argv[ach_optind];
				else
					return('?');
			else
				optarg = argv[ach_optind] + 2;
	ach_optind++;
	return(p ? *p : '?');
} /* ACgetopt */



STATUS ACgetUser (tempdata, user)
char *tempdata;
UID  *user;
{
	char uname[32];

/*
 * 21-jun-89 (mca) - Since we can't currently handle different users anyway,
 * just quietly return OK and run as the current user.
 */
	return(OK);

	if (STcompare(tempdata, "<USER>") )
	{
		cuserid(uname);
		CVlower(uname);
		CVlower(tempdata);
		if (STcompare(tempdata, uname) )
			return(NOT_IMPLEMENTED);
	}
	return(OK);
} /* ACgetUser */



VOID ACinitAlarm (waittime)
i4 waittime;
{
	char tstr[16];
	long tval[2], rval;
	VOID alarm_handler();
	$DESCRIPTOR(tdesc, tstr);
	i4 nwaittime=1;

	STprintf(tstr, "0 ::%d", nwaittime);
	tdesc.dsc$w_length = strlen(tstr);
	sys$bintim(&tdesc, tval);
	if ( (rval = sys$setimr(0, tval, alarm_handler, 1, 0) ) != SS$_NORMAL)
		lib$signal(rval);
} /* ACinitAlarm */


VOID ACioInit ()
{
	char	 buf[MAX_LOC], mboxbuf[12];
	unsigned long rval, status, flags = 1;
	PID pid;
	struct dsc$descriptor_s mboxname = {
		0, DSC$K_DTYPE_T, DSC$K_CLASS_S, mboxbuf
	};
	struct dsc$descriptor_s cmdname = {
		0, DSC$K_DTYPE_T, DSC$K_CLASS_S, buf
	};
	LOCATION loc;
	FILE *dummy;

	STcopy("SYS$INPUT:", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	/*SIopen(&loc, "r", &dummy);
	STcopy("SYS$OUTPUT:", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	*/
	SIopen(&loc, "w", &dummy);
	STcopy("SYS$ERROR:", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	SIopen(&loc, "w", &dummy);
} /* ACioInit */


VOID ACioIni ()
{
	char	 buf[MAX_LOC];
	struct dsc$descriptor_s cmdname = {
		0, DSC$K_DTYPE_T, DSC$K_CLASS_S, buf
	};
	LOCATION loc;
	FILE *dummy;

	STcopy("SYS$INPUT:", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	SIopen(&loc, "r", &dummy);
	STcopy("SYS$OUTPUT:", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	SIopen(&loc, "w", &dummy);
	STcopy("SYS$ERROR:", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	SIopen(&loc, "w", &dummy);

	SIflush(stdout);
} /* ACioIni */


VOID ACprintUser (user)
char *user;
{
/*	char uname[32];

	cuserid(uname);
	CVlower(uname);*/
	SIprintf("   Effective user = '%s'\n", user);
} /* ACprintUser */


VOID ACreleaseChildren ()
{
	int i, j;
/*
	for (i = 0 ; i < numtests ; i++)
		for (j = 0 ; j < tests[i]->t_nkids ; j++)
			sys$resume(&tests[i]->t_children[j].a_pid, 0); 
*/
} /* ACreleaseChildren */


VOID ACresetAlarm ()
{
	static long tval[2] = {0, 0};
	static $DESCRIPTOR(tdesc, "0 ::0.25");
	VOID alarm_handler();

	if (!*tval)
		sys$bintim(&tdesc, tval);
	sys$setimr(0, tval, alarm_handler, 1, 0);
} /* ACresetAlarm */

GLOBALDEF char stat_char = 's';
GLOBALDEF char input_char;
unsigned short channel;

VOID ACresetPrint()
{
	VOID print_status();


/*	sys$qio(0, channel, IO$_READVBLK | IO$M_NOECHO, 0, print_status,
	  0, &input_char, 1, 0, 0, 0, 0); */
} /* ACresetPrint */


VOID ACsetServer (server)
char *server;
{
	long itemlist[] = {
		LNM$_STRING << 16,
		server,
		0,
		0
	};
	$DESCRIPTOR(logname, "II_DBMS_SERVER");
	$DESCRIPTOR(tablename, "LNM$JOB");

	itemlist[0] |= strlen(server);
	sys$crelnm(0, &tablename, &logname, 0, itemlist);
} /* ACsetServer */


VOID ACsetTerm ()
{
	$DESCRIPTOR(device, "SYS$INPUT:");

	sys$assign(&device, &channel, 0, 0);
	ACresetPrint();
} /* ACsetTerm */

int abort_handler();

VOID ACsetupHandlers ()
{
	unsigned short channel;
	unsigned long rval;
	IOSB iosb;
	$DESCRIPTOR(device, "SYS$INPUT:");

	sys$assign(&device, &channel, 0, 0);

	/* Ignore any errors as we may be running as a batch job */
	rval = sys$qiow(EFN$C_ENF, channel, IO$_SETMODE|IO$M_CTRLCAST, &iosb,
			0, 0, 0, 0, 0, 0, 0, 0);
	if (rval & 1)
	    rval = iosb.iosb$w_status;
	if (rval != SS$_NORMAL)
	    return;

	rval = sys$qiow(EFN$C_ENF, channel, IO$_SETMODE|IO$M_CTRLCAST, &iosb,
			0, 0, abort_handler, 0, 0, 0, 0, 0);
	if (rval & 1)
	    rval = iosb.iosb$w_status;
	if (rval != SS$_NORMAL)
	    return;

	if (iosb.iosb$w_status != SS$_NORMAL)
	    printf("SYS$QIOW returned %d in iosb.\n", iosb.iosb$w_status);
} /* ACsetupHandlers */


VOID ACsubmit( batname, outfile, rtnpid, rtniosb, astadr, astprm, uname )

char			*batname;	    /* Batch file name */
struct dsc$descriptor_s *outfile;	    /* Output (log) file */
unsigned long		*rtnpid;	    /* Addr to return job's pid */
IOSB			*rtniosb;	    /* Addr for job's compl iosb */
i4			*astprm;	    /* AST parm */
unsigned long		*astadr;	    /* AST to call when job ends */
char			*uname;		    /* User name to login as */

{
static char	
	queue[35];			    /* Processing batch queue */

char	queuefound[35],
	user[13],
	lfile[132],
	tmp_str[132];

unsigned long	
	entrynum,
	entryfound,
	pid,
	searchflag,
	rval;
    IOSB iosb;

static int
	priority = 4,
	zero = 0;

int	i;

/* Descriptor for log file */
struct dsc$descriptor_s 
	logdesc = { sizeof(lfile), DSC$K_DTYPE_T, DSC$K_CLASS_VS, lfile };

/* Item list for the Enter File call to submit the batch job */
ILE3 itm_1[] = { 	
		strlen(batname), SJC$_FILE_SPECIFICATION, batname, 0,
		0, SJC$_LOG_SPECIFICATION, 0, 0,
		strlen(queue), SJC$_QUEUE, queue, 0,
		4, SJC$_PRIORITY, &priority, 0,
	     	12, SJC$_USERNAME, user, 0, 	 /* Temporary Problem */
		sizeof(entrynum), SJC$_ENTRY_NUMBER_OUTPUT, &entrynum, 0,
		0, SJC$_NO_LOG_SPOOL, 0, 0,
		0, SJC$_NO_LOG_DELETE, 0, 0,
		0, SJC$_DELETE_FILE, 0, 0, 
		0, 0 };

/* Item list for the Synchronize job call */
ILE3 itm_2[] = {
		sizeof(queue), SJC$_QUEUE, queue, 0,
		sizeof(entrynum), SJC$_ENTRY_NUMBER, &entrynum, 0,
		0, 0 };

/* Item list for the Display queue call */
ILE3 itm_3[] = {
		sizeof(queue), QUI$_SEARCH_NAME, queue, 0,
		sizeof(searchflag), QUI$_SEARCH_FLAGS, &searchflag, 0,
		31, QUI$_QUEUE_NAME, queuefound, 0,
		0, 0 };

/* Item list for the Display job call */
ILE3 itm_4[] = {
		sizeof(searchflag), QUI$_SEARCH_FLAGS, &searchflag, 0,
		sizeof(entryfound), QUI$_ENTRY_NUMBER, &entryfound, 0,
		sizeof(pid), QUI$_JOB_PID, &pid, 0,
		0, 0 };

/* added by Don Johnson... ( start ) */

unsigned int	trn_len = 0, trn_mask = LNM$M_CASE_BLIND;

ILE3 trnlnm_itm[] = {
		{ sizeof(queue), LNM$_STRING, queue, &trn_len },
		{ 0,	       0,	    0,     0 } };

	$DESCRIPTOR( tlog,"ACHILLES$BATCH" );

	$DESCRIPTOR( ttab,"LNM$PROCESS_TABLE" );
	rval = sys$trnlnm(&trn_mask,&ttab,&tlog,0,&trnlnm_itm );
	if ( rval == SS$_NOLOGNAM )
	{
	    $DESCRIPTOR( ttab,"LNM$JOB" );
	    rval = sys$trnlnm(&trn_mask,&ttab,&tlog,0,&trnlnm_itm );
	    if ( rval == SS$_NOLOGNAM )
	    {
		$DESCRIPTOR( ttab,"LNM$GROUP" );
		rval = sys$trnlnm(&trn_mask,&ttab,&tlog,0,&trnlnm_itm );
		if ( rval == SS$_NOLOGNAM )
		{
		    $DESCRIPTOR( ttab,"LNM$SYSTEM_TABLE" );
		    rval = sys$trnlnm(&trn_mask,&ttab,&tlog,0,&trnlnm_itm );
		    if ( rval == SS$_NOLOGNAM )
		    {
			strcpy( queue,"ACHILLES$BATCH" );
			trn_len = strlen( queue );
			rval = SS$_NORMAL;
		    }
		}
	    }
	}
	if ( rval != SS$_NORMAL )
	{
		ACerrMsg( "ACsubmit sys$trnlnm rval", rval );
		return;
	}
	queue[trn_len] = '\0';
        itm_1[2].ile3$w_length = trn_len;	/*  When itm_1 structure is inited, there's
				    no length yet. */

/* added by Don Johnson... ( end   ) */

	/* The login user name must be 12 characters long, upper case */
	sprintf( user, "%-12s", uname ); 
	for (i=0; i<12; i++)
		user[i] = toupper(user[i]);

	/* Convert the output file to a true path name */
	lib$find_file( outfile, &logdesc, &zero, 0, 0, 0, 0 );

	/* Put the path name into the item list */
	itm_1[1].ile3$w_length = lfile[0];
	itm_1[1].ile3$ps_bufaddr = &lfile[2];

	/* Enter the job in the queue */
	rval = sys$sndjbcw(EFN$C_ENF, SJC$_ENTER_FILE, 0, &itm_1, &iosb, 0, 0);
	if ( rval != SS$_NORMAL )
	{
		ACerrMsg( "ACsubmit sys$sndjbcw rval", rval );
		return;
	}
	if ( iosb.iosb$w_status != JBC$_NORMAL )
	{
		sprintf(  tmp_str
			, "ACsubmit <%s> sys$sndjbcw iosb"
			, itm_1[1].ile3$ps_bufaddr);

		ACerrMsg( tmp_str, iosb.iosb$w_status );
		return;
	}

	/* Dissolve any internal search context blocks */
	sys$getquiw(EFN$C_ENF, QUI$_CANCEL_OPERATION, 0, 0, &iosb, 0, 0);

	/* Establish the queue context. */
	searchflag = QUI$M_SEARCH_WILDCARD | QUI$M_SEARCH_BATCH | 
		     QUI$M_SEARCH_ALL_JOBS;
	do
	{
		rval = sys$getquiw(EFN$C_ENF, QUI$_DISPLAY_QUEUE, 0, &itm_3,
				   &iosb, 0, 0);
		if ( rval != SS$_NORMAL )
		{
			ACerrMsg( "ACsubmit Display_queue", rval );
			return;
		}
		if ( iosb.iosb$w_status != JBC$_NORMAL )
		{
			ACerrMsg( "ACsubmit Display_queue", iosb.iosb$w_status );
			return;
		}
	} while ( strcmp(queue, queuefound) );


	/* Search all the jobs in the queue */
	entryfound = 0;
	do
	{
		rval = sys$getquiw(EFN$C_ENF, QUI$_DISPLAY_JOB, 0, &itm_4,
				   &iosb, 0, 0);
		if ( rval != SS$_NORMAL )
		{
			ACerrMsg( "ACsubmit Display_job", rval );
			return;
		}
		if ( iosb.iosb$w_status != JBC$_NORMAL )
		{
			ACerrMsg( "ACsubmit Display_job", iosb.iosb$w_status );
			return;
		}
	} while( entryfound != entrynum );

	/* Save the PID of the process */
	*rtnpid = pid;

	/* Dissolve any internal search context blocks */
	sys$getquiw(EFN$C_ENF, QUI$_CANCEL_OPERATION, 0, 0, &iosb, 0, 0);

	/* And finally synchronize the Ast with the job termination */
	sys$sndjbc( 0, SJC$_SYNCHRONIZE_JOB, 0, &itm_2, rtniosb, 
	    astadr, astprm );
} /* ACsubmit */

