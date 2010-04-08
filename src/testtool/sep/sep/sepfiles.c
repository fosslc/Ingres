/*
    Include files
*/
#include <compat.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#include <lo.h>
#include <er.h>
#include <me.h>

#define sepfiles_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**	30-Oct-1989 (Eduardo)
**		Creation
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	27-aug-1990 (rog)
**		Correctly handle SEPENDFILE's in SEPFILE files.
**	29-aug-1990 (rog)
**		Change uchar's to u_char's for portability.
**	02-may-91 (kirke)
**		It is unnecessary and incorrect to allocate the FILE
**		structure since SIfopen() does it for us (via fopen()).
**		We don't need to free it either, as SIclose() does
**		that (via fclose()).
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	02-apr-1992 (donj)
**	    Changed all refs to '\0' to EOS. Changed more char pointer 
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**	    to the standard "while (*c != EOS)". Implement other CM routines
**	    as per CM documentation.
**      10-jul-1992 (donj)
**          Added include of sepfiles.h for the ME memory tag constants.
**          Isolated these MEreqmems to their own tag.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	30-nov-1992 (donj)
**	    made SEPresloc and SEPcanloc global.
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**	23-sep-1993 (donj)
**	    Modify SEPtranslate() to protect IISTindex from overrunning a
**	    memory area.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**      14-apr-1994 (donj)
**          Added testcase support.
**	10-may-1994 (donj)
**	    Implemented a "delete file location" function, del_floc(), that's
**	    more efficient than the old del_files() function that re-created
**	    the location structure even if that structure already existed.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**
**	??-oct-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**	04-dec-1997 (kinpa04)
**	    Change to SEPgetc() & SEPgetrec() because SIread() was changed
**	    to return ENDFILE on partial read of buffer at EOF.
**	08-jul-1998 (kosma01)
**	    sgi_us5's compiler is very picky, cast buffer to (PTR) in
**	    SIwrite.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
*/

GLOBALREF    SEPFILE      *sepResults ;
GLOBALREF    SEPFILE      *sepCanons ;
GLOBALREF    SEPFILE      *sepGCanons ;
GLOBALREF    SEPFILE      *sepDiffer ;

GLOBALREF    LOCATION      SEPresloc ;
GLOBALREF    LOCATION      SEPcanloc ;

GLOBALREF    bool          updateMode ;
GLOBALREF    char            tcName [] ;

static char                SEPres [MAX_LOC] ;
static char                SEPcan [MAX_LOC] ;
static char                SEPgcan [MAX_LOC] ;
static char                SEPdiff [MAX_LOC] ;
static char                escapebuf [2] = { SEP_ESCAPE, EOS } ;
static LOCATION            SEPgcanloc ;
static LOCATION            SEPdiffloc ;

STATUS
SEPopen(LOCATION *fileloc,i4 buffSize,SEPFILE	**fileptr)
{
    SEPFILE               *sptr = NULL ;
    FILE                  *fptr = NULL ;
    u_char                *cptr = NULL ;
    STATUS                 ioerr ;
    char                  *cptr1 = NULL ;

    if ((ioerr = SIfopen(fileloc, ERx("rw"), SI_RACC, buffSize, &fptr)) != OK)
	return(ioerr);
	
    cptr1 = SEP_MEalloc(SEP_ME_TAG_SEPFILES, sizeof(SEPFILE), TRUE, &ioerr);
    sptr  = (SEPFILE *)cptr1;

    if (ioerr != OK)
    {
	SIclose(fptr);
	return(ioerr);
    }

    cptr1 = SEP_MEalloc(SEP_ME_TAG_SEPFILES, buffSize, TRUE, &ioerr);
    cptr  = (u_char *)cptr1;
    if (ioerr != OK)
    {
	SIclose(fptr);
	MEfree((PTR)sptr);
	return(ioerr);
    }

    sptr->_buf = cptr;
    sptr->_pos = sptr->_end = sptr->_buf + buffSize;
    sptr->_buffSize = buffSize;
    sptr->_fptr = fptr;

    *fileptr = sptr;

    return(OK);
}

STATUS
SEPclose(SEPFILE *fileptr)
{
    STATUS                 ioerr ;

    ioerr = SIclose(fileptr->_fptr);
    MEfree((PTR)fileptr->_buf);
    MEfree((PTR)fileptr);
    return(ioerr);
}

STATUS
SEPrewind(SEPFILE *fileptr,bool flushing)
{
    if (flushing)
	SIflush(fileptr->_fptr);

    fileptr->_pos = fileptr->_end;
    SIfseek(fileptr->_fptr, 0L, SI_P_START);
    
    return(OK);
}

VOID
SEPtranslate(char *buffer,i4 buff_len)
{
    char                  *tempbuf = NULL ;
    char                  *tptr = NULL ;
    char                  *cptr = NULL ;

    bool                   not_first_time ;

    i4                     tmp_buf_len ;

    cptr = buffer;

    if (NULL != STindex(buffer, escapebuf, buff_len))
    {
	if (buff_len)
	    tmp_buf_len = buff_len;
	else
	    tmp_buf_len = STlength(buffer);

	tempbuf = tptr = SEP_MEalloc(SEP_ME_TAG_SEPFILES, tmp_buf_len, FALSE,
                                     (STATUS *) NULL);
	not_first_time = FALSE;
	do
	{
	    if (not_first_time)
		CMnext(cptr);
	    else
		not_first_time = TRUE;

	    if (*cptr == SEP_ESCAPE)
	    {
		CMnext(cptr);
		switch(*cptr)
		{
		    case SEP_ESCAPE:
			break;
		    default:
			*cptr = -1 - ( (i4)((u_char)-1) - ((u_char)*cptr));
			break;
		}
	    }
	    *tptr = *cptr;
	    CMnext(tptr);
	} while (*cptr != EOS);
	STcopy(tempbuf, buffer);
	MEfree(tempbuf);
    }
}

STATUS
SEPputrec(char *buffer,SEPFILE *fileptr)
{
    char                  *tempbuf = NULL ;
    char                  *tptr = NULL ;
    char                  *cptr = NULL ;
    i4                     len ;
    i4                     count ;
    STATUS                 status ;

    bool                   not_first_time ;

    count = 0;
    cptr = buffer;

    len = STlength(buffer);
    if (NULL != STindex(buffer, escapebuf, 0))
    {
	not_first_time = FALSE;
	do
	{
	    if (not_first_time)
		CMnext(cptr);
	    else
		not_first_time = TRUE;

	    if (*cptr == SEP_ESCAPE)
		count++;
	} while (*cptr != EOS);

	tempbuf = tptr = SEP_MEalloc(SEP_ME_TAG_SEPFILES, len+count+1, FALSE,
				     (STATUS *) NULL);
	cptr = buffer;

	not_first_time = FALSE;

	do
	{
	    if (not_first_time)
		CMnext(cptr);
	    else
		not_first_time = TRUE;

	    if (*cptr == SEP_ESCAPE)
	    {
		*tptr = SEP_ESCAPE;
		CMnext(tptr);
	    }

	    *tptr = *cptr;
	    CMnext(tptr);
	} while (*cptr != EOS);

	status = SIwrite(len+count, tempbuf, &count, fileptr->_fptr);
	MEfree(tempbuf);
	return(status);
    }
    else
	return(SIwrite(len, buffer, &count, fileptr->_fptr));
}

STATUS
SEPputc(i4 achar,SEPFILE *fileptr)
{
    u_char                 buffer [2] ;
    i4                     count ;

    if (achar != (i4)((u_char) achar) || achar == SEP_ESCAPE)
    {
	buffer[0] = SEP_ESCAPE;
	buffer[1] = EOS;
	SIwrite(1, (PTR)buffer, &count, fileptr->_fptr);
    }

    buffer[0] = achar;
    buffer[1] = EOS;

    return(SIwrite(1, (PTR)buffer, &count, fileptr->_fptr));
}

STATUS
SEPgetrec(char *buffer,SEPFILE *fileptr)
{
    STATUS                 ioerr ;
    i4                     left ;
    i4                     count ;
    bool                   final ;
    register char         *dptr = buffer ;
    register u_char       *sptr = fileptr->_pos ;

    for (;;)
    {
	while (!(final = (sptr == fileptr->_end)) && *sptr != '\n' && 
	       !SEPcheckEnd(sptr))
	    CMcpyinc(sptr,dptr);

	if (!final && *sptr == '\n')
	{
	    CMcpyinc(sptr,dptr);
	    *dptr = EOS;
	    fileptr->_pos = sptr;
	    ioerr = OK;
	    break;
	}

	if (!final && SEPcheckEnd(sptr))
	{
	    if (sptr == fileptr->_pos)
		ioerr = ENDFILE;
	    else
	    {
		*dptr = '\n';
		CMnext(dptr);
		*dptr = EOS;
		fileptr->_pos = sptr;
		ioerr = OK;
	    }
	    break;
	}
    
	fileptr->_pos = sptr = fileptr->_buf;
	if ((ioerr = SIread(fileptr->_fptr, fileptr->_buffSize, &count, sptr))
	    != OK)
#ifdef VMS
	    break;
#endif
#if defined(UNIX) || defined(NT_GENERIC)
	{
	/*
	**	UNIX returns ENDFILE if less than buffSize characters
	**	remain in file
	*/
	    if (ioerr == ENDFILE && count > 0)
	    {
		fileptr->_buf[count++] = SEP_ESCAPE;
		fileptr->_buf[count] = SEPENDFILE;
	    }
	    else
		break;
	}
#endif
    }
    
    SEPtranslate(buffer, fileptr->_buffSize);
    return(ioerr);
}


    bool                   final ;
static bool                doescape = TRUE ;

i4
SEPgetc(SEPFILE	*fileptr)
{
    STATUS                 ioerr ;
    i4                     left ;
    i4                     count ;
    i4                     toreturn ;

    left = fileptr->_end - fileptr->_pos;

    if (left > 0)
	toreturn = *fileptr->_pos++;
    else
    {
	fileptr->_pos = fileptr->_buf;
	if ((ioerr = SIread(fileptr->_fptr, fileptr->_buffSize, &count, fileptr->_pos)) != OK)
	{
#ifdef VMS
	    return(SEPENDFILE);
#endif
#if defined(UNIX) || defined(NT_GENERIC)
/*
**	UNIX returns ENDFILE if less than buffSize in files
*/
	    if (ioerr == ENDFILE && count > 0)
	    {
		fileptr->_buf[count++] = SEP_ESCAPE;
		fileptr->_buf[count] = SEPENDFILE;
		toreturn = *fileptr->_pos++;
	    }
	    else
		return(SEPENDFILE);
#endif
	}
	else
	    toreturn = *fileptr->_pos++;
    }
    if (doescape && toreturn == SEP_ESCAPE)
    {
	doescape = FALSE;
	switch(toreturn = SEPgetc(fileptr))
	{
	   case SEP_ESCAPE:
		break;
	   default:
		toreturn = -1 - ( (i4)((u_char)-1) - ((u_char)toreturn));
		break;
	}
	doescape = TRUE;
    }
    return(toreturn);
}

STATUS
SEPfilesInit(char *pidstring,char *message)
{
    FILE                  *fptr = NULL ;
    STATUS                 ioerr ;

    STprintf(SEPres,  ERx("re%s.stf"), pidstring);
    STprintf(SEPcan,  ERx("ca%s.stf"), pidstring);
    STprintf(SEPdiff, ERx("df%s.stf"), pidstring);
    LOfroms(FILENAME & PATH, SEPres, &SEPresloc);
    LOfroms(FILENAME & PATH, SEPcan, &SEPcanloc);
    LOfroms(FILENAME & PATH, SEPdiff, &SEPdiffloc);

    STprintf(tcName,  ERx("tc%s.stf"), pidstring);

    /*
    **	we create the files using this undesirable way because
    **  of CL SI inability to create a RACC file if open as "rw"
    **	so we have to create the file opening it as "w" and then
    **  reopen it as "rw"
    */

    /*	create file */

    if ((ioerr = SIfopen(&SEPresloc, ERx("w"), SI_RACC, SCR_LINE, &fptr)) != OK)
    {
	STcopy(ERx("ERROR: could not create RESULTS files"), message);
	return(ioerr);
    }
    else
	SIclose(fptr);

    if ((ioerr = SIfopen(&SEPcanloc, ERx("w"), SI_RACC, SCR_LINE, &fptr)) != OK)
    {
	STcopy(ERx("ERROR: could not create CANONS files"), message);
	return(ioerr);
    }
    else
	SIclose(fptr);

    if ((ioerr = SIfopen(&SEPdiffloc, ERx("w"), SI_RACC, TEST_LINE, &fptr)) != OK)
    {
	STcopy(ERx("ERROR: could not create DIFFS files"), message);
	return(ioerr);
    }
    else
	SIclose(fptr);

    /*	open files as "rw"  */

    if (SEPopen(&SEPresloc, SCR_LINE, &sepResults) != OK)
    {
	STcopy(ERx("ERROR: could not open RESULTS file"),message);
	return(FAIL);
    }
    if (SEPopen(&SEPcanloc, SCR_LINE, &sepCanons) != OK)
    {
	STcopy(ERx("ERROR: could not open CANONS file"),message);
	return(FAIL);
    }
    if (SEPopen(&SEPdiffloc, TEST_LINE, &sepDiffer) != OK)
    {
	STcopy(ERx("ERROR: could not open DIFFS file"),message);
	return(FAIL);
    }
    
    /*
    **	if updating a test create file to accumulate alternate
    **	canons
    */

    if (updateMode)
    {
	STprintf(SEPgcan, ERx("gc%s.stf"), pidstring);
	LOfroms(FILENAME & PATH, SEPgcan, &SEPgcanloc);
	if ((ioerr = SIfopen(&SEPgcanloc, ERx("w"), SI_RACC, SCR_LINE, &fptr)) != OK)
	{
	    STcopy(ERx("ERROR: could not create GCANONS files"), message);
	    return(ioerr);
	}
	else
	    SIclose(fptr);
	if (SEPopen(&SEPgcanloc, SCR_LINE, &sepGCanons) != OK)
	{
	    STcopy(ERx("ERROR: could not open GCANONS file"),message);
	    return(FAIL);
	}
    }

    return(OK);
}

STATUS
SEPfilesTerminate()
{
    SEPclose(sepResults);
    SEPclose(sepCanons);
    SEPclose(sepDiffer);
    del_floc(&SEPresloc);
    del_floc(&SEPcanloc);
    del_floc(&SEPdiffloc);
    if (updateMode)
    {
	SEPclose(sepGCanons);
	del_floc(&SEPgcanloc);
    }
    return(OK);
}
