/*
**  Sepfiles.h
**
**  History:
**	##-###-1990 (eduardo)
**		Created.
**	27-aug-1990 (rog)
**		Added SEP_ESCAPE and SEPcheckEnd to correctly handle 
**		SEPENDFILE's embedded inside SEPFILE files.
**	29-aug-1990 (rog)
**		Changed uchar's to u_char's for portability.
**	14-jan-1992 (donj)
**	     Reformatted variable declarations to one per line for clarity.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 7-may-1993 (donj)
**	    Fixed prototyping in this file.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
*/

FUNC_EXTERN  STATUS        SEPfilesTerminate () ;

FUNC_EXTERN  STATUS        SEPopen (LOCATION *fileloc,i4 buffSize,SEPFILE  **fileptr) ;
FUNC_EXTERN  STATUS        SEPclose (SEPFILE *fileptr) ;
FUNC_EXTERN  STATUS        SEPrewind (SEPFILE *fileptr,bool flushing) ;
FUNC_EXTERN  STATUS        SEPputrec (char *buffer,SEPFILE *fileptr) ;
FUNC_EXTERN  STATUS        SEPputc (i4 achar,SEPFILE *fileptr) ;
FUNC_EXTERN  STATUS        SEPgetrec (char *buffer,SEPFILE *fileptr) ;
FUNC_EXTERN  i4            SEPgetc (SEPFILE *fileptr) ;
FUNC_EXTERN  STATUS        SEPfilesInit (char *pidstring,char *message) ;

#define      SEPENDFILE    -1
#define      SEP_ESCAPE    '\\'

#define SEPcheckEnd(buffer)	(buffer[0] == SEP_ESCAPE \
	&& (SEPENDFILE == (-1 - ( (i4)((u_char)-1) - ((u_char)buffer[1])))))
