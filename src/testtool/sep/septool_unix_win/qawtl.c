/*  qawtl.c
** Write a record to $II_SYSTEM/ingres/files/errlog.log to denote
** a testing event.
** qawtl <YOUR MESSAGE>
**
** Alternatively, if you want your messages written to a file other
** than errlog.log, you can set the OS environment variable QAWTL_LOG
** to an alternate location.  However, insure you specify the full
** absolute path name of your log file.
** setenv QAWTL_LOG <absolute_path_of_my_log_file>
**
** If you want to disable these messages all together:
** setenv QAWTL_LOG /dev/null
**
** The time stamp written to the log is the time qawtl was invoked
** in accordance with the Ingres environment variable II_TIMEZONE_NAME
** 
**  Also if qawtl is going to invoked by a user other than ingres
** (testenv), it must be owned by ingres and installed with the
**  sticky bit set:
**    chmod u+s qawtl
** However, qawtl is built with this protection by default and this
** should not be a problem.
**
**  History:
**     (merja01 & liama01) 21-May-1999
**         Created to emulate the orignal qawtl unix shell script.  This
**         will allow it to run on both UNIX and NT.
**     (merja01) 10-June-1999
**         Correct call to ERlog to resolve bus error on DGI.
**     (xu$we01) 23-Dec-2002
**	   qawtl will abend or core dumped if the text passed exceeds  
**	   the defined message length. Fixed the bug by truncating the 
**	   message if it exceeds the message length limit. Increased
**	   the QA_MAX_MSG from 200 to 280. 
**	   Took off "qa_logfilename = (char *) malloc(QA_MAX_LOG)" because
**	   it is not needed. Also because it caused circulative using of
**	   stack.
**	   Took off '\' in the printf statements to resolve the compilation
**	   warning "unknown escape sequence `\)'".
**	2-Jul-2009 (kschendel)
**	    Fix bus error on sparc64, TMtz-init was being called improperly,
**	    and it's not needed here anyway.
*/

/*
**      ming hints
PROGRAM = qawtl
**
NEEDLIBS = LIBINGRES
**
MODE = SETUID
*/

#include <stdio.h>
#include <stdlib.h>
#include <compat.h>
#include <tmcl.h>
#include <st.h>

#define QA_MAX_MSG 280
#define QA_MAX_DATE 30 
#define QA_BEGIN_MSG "====>"
#define QA_END_MSG   "<===="

main(argc, argv)
int     argc;
char    *argv[];
{
int curr_argn, len;
char msg_buffer [QA_MAX_MSG] ;
char date [QA_MAX_DATE] ;
SYSTIME  stime;
char *qa_logfilename = NULL; 
FILE *QAWTL_fp;
CL_SYS_ERR     sys_err; 
char *arg_ptr, *bptr;

/* If no user message silently exit without writing to log */
if (argc < 2)
{
        exit(1);
}
/* get time stamp */
TMnow(&stime);
TMstr(&stime, date);

/* Begin  message */
IISTprintf(msg_buffer, "\nQAWTL %s", date);
IISTprintf((msg_buffer + STlength(msg_buffer)), " %s ", QA_BEGIN_MSG);

/* Add passed text to message. */
/* Truncate the message if it exceeds the max msg_buffer size */

bptr = msg_buffer + STlength(msg_buffer);

len = sizeof(msg_buffer) - (bptr - msg_buffer) - STlength(QA_END_MSG) - sizeof("\0");
for (curr_argn = 1; curr_argn < argc; curr_argn++)
{
   arg_ptr = argv[curr_argn];
   while ( len > 0 && *arg_ptr )
   {
      *bptr++ = *arg_ptr++;
      len--;
   }
   if ( len )
   {
      *bptr++ = ' ';
      len--;
   }
}
/* End message */
IISTprintf(bptr, "%s\n ", QA_END_MSG);

/* check for an alternate log */
if (getenv("QAWTL_LOG"))
   {   
        qa_logfilename = getenv("QAWTL_LOG");
        if ((QAWTL_fp = fopen(qa_logfilename, "a")))
           {
             SIfprintf(QAWTL_fp,"%s", msg_buffer);
             fclose(QAWTL_fp);
           }
        else
           {
             printf("QAWTL can not write to %s \n", qa_logfilename);
             return(FAIL);
           }
   }
/* Default - write message to errlog.log */
else
   {
       CL_CLEAR_ERR(&sys_err);
       if(ERlog(msg_buffer, STlength(msg_buffer),&sys_err))
         {       
          printf("");
          printf("QAWTL failed writing to errlog.log\n\
Make sure sticky bit is set! (chmod 4755 qawtl)\n"); 
          return(FAIL);
         }
   }
          
          

return(OK);
}
