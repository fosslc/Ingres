/*
    include files
*/
#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <si.h>
#include <st.h>

#define getloc_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	07-Nov-1991 (DonJ)
**	    Extracted from utility.c.
**	02-apr-1992 (donj)
**	    Added inclusion of the cm.h cl module. Removed commented out
**	    inclusions that were not needed. Added FIND_SEPstring()
**	    reference. Changed all refs to '\0' to EOS. Changed more char 
**          pointer increments (c++) to CM calls (CMnext(c)). Changed a 
**          "while (*c)" to the standard "while (*c != EOS)". Made 
**	    explicitly declared char arrays dynamically allocated with
**	    MEreqmem(). Used FIND_SEPstring() rather than STbcompare() so
**	    I could handle cases such as: " '@FILE()' " where the pattern
**	    we're looking for is not delimited by whitespace. Implement
**	    other CM routines as per CM documentation.
**	10-jul-1992 (donj)
**	    Added include of sepfiles.h for the ME memory tag constants.
**	    Isolated these MEreqmems to their own tag.
**	14-jul-1992 (donj)
**	    Added tracing to debug a problem with translating '@file()'
**	    function. Found that replacing a "MEreqmem(); STcopy" with a
**	    STtalloc() fixed the problem.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	24-aug-1992 (donj)
**	    Re-worked getLocation() function that translates the "@file()"
**	    sep function. GetLocation() won't try to translate any dev specs.
**	    Also, "@file()" will work in many forms previously not available
**	    i.e., "@file(dev,)" "@file(,dir,dir,)" "@file(,file)" "@file(file)"
**	    and "@file(dev,file)".
**	30-nov-1992 (donj)
**	    Made tracing a little simpler. Added a ME tag to Trans_SEPfile().
**	08-Dec-1992 (donj)
**	    Changed ME tag for tmp_buffers trying to find error on UNIX. Also
**	    Changed Trans_SEPparam() to not try to free memory tagged
**	    "SEP_ME_TAG_NODEL" (nodelete).
**      18-jan-1993 (donj)
**          Wrap LOcompose() with SEP_LOcompose() to fix a UNIX bug.
**	04-feb-1993 (donj)
**	    Rewrote getLocation() for UNIX LO peculiarities and added MPE
**	    logic as per HenryM's specs.
**      15-feb-1993 (donj)
**          Change casting of ME_tag to match casting of the args in the ME
**          functions.
**	 7-may-1993 (donj)
**	    Adding prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**	20-aug-1993 (donj)
**	    Re-worked how "@file()" handles Environmental Vars, "@file(xxxx,)",
**	    in UNIX. First it tries a call to NMgetAt(), if that fails, and the
**	    string isn't mixed case (new function SEP_CMwhatcase), switch cases
**	    and try again.
**	23-sep-1993 (donj)
**	    Changed the ME tag for three vars in Trans_SEPfile(). They were
**	    getting freed with SEP_ME_TAG_GETLOC inside of getLocation(). Then
**	    accessed afterward.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	20-jan-1994 (donj)
**	    Constant string going to LOfroms() causing core dump on ALPHA. Changed it
**	    to a writable location.
**
**	11-sep-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** getLocation
*/

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

STATUS
getLocation(char *string,char *newstr,LOCTYPE *typeOfLoc)
{
    STATUS                 ret_val ;
    LOCATION               aLoc ;
    LOCATION               bLoc ;

    LOCATION              *Locptr1 = NULL ;
    LOCATION              *Locptr2 = NULL ;
    LOCATION              *Locptr3 = NULL ;

    char                  *cptr    = NULL ;
    char                  *cptr2   = NULL ;
    char                  *next    = NULL ;
    char                  *start   = NULL ;
    char                  *tmpBuf  = NULL ;
    char                  *tmpBuf2 = NULL ;
    char                  *gl_dev  = NULL ;
    char                  *gl_path [128] ;
    char                  *gl_file = NULL ;
    char                  *resultPtr = NULL ;
    char                  *tmpaLocStr = NULL ;
    char                  *tmpbLocStr = NULL ;

    i4                     i ;

    ret_val = OK;
    for ( i=0; i<128; i++)
	gl_path[i] = NULL;

    tmpaLocStr = SEP_MEalloc(SEP_ME_TAG_GETLOC, MAX_LOC+1, TRUE, (STATUS *)NULL);
    tmpbLocStr = SEP_MEalloc(SEP_ME_TAG_GETLOC, MAX_LOC+1, TRUE, (STATUS *)NULL);

    LOfroms(FILENAME & PATH, tmpaLocStr, &aLoc);
    LOfroms(FILENAME & PATH, tmpbLocStr, &bLoc);

    if (tracing&TRACE_PARM)
    {
	SIfprintf(traceptr, ERx("getLocation00> string = %s\n"), string);
	SIfprintf(traceptr, ERx("getLocation01> newstr = %s\n"), newstr);
    }

    tmpBuf   = SEP_MEalloc(SEP_ME_TAG_GETLOC, MAX_LOC+1, TRUE, (STATUS *)NULL);
    STcopy(string, tmpBuf);

    if (!(start = STindex(tmpBuf, ERx("("), 0)))
    {
	MEtfree(SEP_ME_TAG_GETLOC);
	return(FAIL);
    }

    MEfree(tmpaLocStr);
    MEfree(tmpbLocStr);

    aLoc.string = SEP_MEalloc(SEP_ME_TAG_GETLOC, MAX_LOC+1, TRUE, (STATUS *)NULL);
    bLoc.string = SEP_MEalloc(SEP_ME_TAG_GETLOC, MAX_LOC+1, TRUE, (STATUS *)NULL);

    next = start;
    CMnext(next);
    if (typeOfLoc)
	*typeOfLoc = NULL;

    if (CMcmpcase(next, ERx(",")))
    {
	/* There's a device specification */

	cptr = next;
	if ((next = STindex(cptr, ERx(","), 0)) == NULL)
	{
	    if ((next = STindex(cptr, ERx(")"), 0)) == NULL)
		ret_val = FAIL;
	    else
	    {	/*
		** This is really a file name in the form, @file(filename.type)
		*/
		*next = EOS;
		gl_file = SEP_MEalloc(SEP_ME_TAG_GETLOC, ((i4)(next-cptr))+2,
				      TRUE, (STATUS *)NULL);
		STcopy(cptr, gl_file);

		if (tracing&TRACE_PARM)
		    SIfprintf(traceptr, ERx("getLocation02> file = %s\n"),
			      gl_file);
		if (typeOfLoc)
		    *typeOfLoc = FILENAME;
	    }
	}
	else
	{
	    *next = EOS;
	    gl_dev = SEP_MEalloc(SEP_ME_TAG_GETLOC, ((i4)(next-cptr))+2, TRUE,
				 (STATUS *)NULL);
#ifdef NT_GENERIC
	    STpolycat(1, cptr, gl_dev);

	    cptr2 = gl_dev;
	    SEP_NMgtAt(cptr2, &tmpBuf2, SEP_ME_TAG_GETLOC);
	    if ((tmpBuf2 == NULL)||(*tmpBuf2 == EOS))
	    {
		switch (i = SEP_CMwhatcase(cptr2))
		{
		   case  0: break;
		   case  1: for (cptr2 = gl_dev; *cptr2 != EOS; CMnext(cptr2))
				CMtolower(cptr2,cptr2);
			    break;
		   case -1: for (cptr2 = gl_dev; *cptr2 != EOS; CMnext(cptr2))
				CMtoupper(cptr2,cptr2);
			    break;
		}
		if (i != 0)
		{
		    cptr2 = gl_dev;
		    SEP_NMgtAt(cptr2, &tmpBuf2, SEP_ME_TAG_GETLOC);
		}
	    }

	    if (tmpBuf2 != NULL)
		if (*tmpBuf2 != EOS)
		{
		    MEfree(gl_dev);
		    gl_dev = SEP_MEalloc(SEP_ME_TAG_GETLOC,
					 (STlength(tmpBuf2)*2)+2,
					 TRUE,(STATUS *)NULL);
		    STcopy(tmpBuf2,gl_dev);
		}
#else    
#ifdef UNIX
	    STpolycat(2,ERx("$"),cptr, gl_dev);

	    cptr2 = gl_dev;
	    SEP_NMgtAt(CMnext(cptr2), &tmpBuf2, SEP_ME_TAG_GETLOC);
	    if ((tmpBuf2 == NULL)||(*tmpBuf2 == EOS))
	    {
		switch (i = SEP_CMwhatcase(cptr2))
		{
		   case  0: break;
		   case  1: for (cptr2 = gl_dev; *cptr2 != EOS; CMnext(cptr2))
				CMtolower(cptr2,cptr2);
			    break;
		   case -1: for (cptr2 = gl_dev; *cptr2 != EOS; CMnext(cptr2))
				CMtoupper(cptr2,cptr2);
			    break;
		}
		if (i != 0)
		{
		    cptr2 = gl_dev;
		    SEP_NMgtAt(CMnext(cptr2), &tmpBuf2, SEP_ME_TAG_GETLOC);
		}
	    }

	    if (tmpBuf2 != NULL)
		if (*tmpBuf2 != EOS)
		{
		    MEfree(gl_dev);
		    gl_dev = SEP_MEalloc(SEP_ME_TAG_GETLOC,
					 (STlength(tmpBuf2)*2)+2,
					 TRUE,(STATUS *)NULL);
		    STcopy(tmpBuf2,gl_dev);
		}
#else
#ifdef MPE
            STpolycat(2,ERx("!"),cptr, gl_dev);
            for (cptr2 = gl_dev; *cptr2 != EOS; CMnext(cptr2))
                CMtoupper(cptr2,cptr2);
#else
	    /* Probably VMS */

	    STcopy(cptr, gl_dev);
#endif
#endif
#endif
	    if (tracing&TRACE_PARM)
		SIfprintf(traceptr, ERx("getLocation03> dev = %s\n"), gl_dev);
	    if (typeOfLoc)
		*typeOfLoc = PATH;
	}

    }

    cptr = CMnext(next);
    next = STindex(cptr, ERx(","), 0);

    for (i = 0; next; ) /* do the paths */
    {
	*next = EOS;
	gl_path[i] = SEP_MEalloc(SEP_ME_TAG_GETLOC, (int)(next-cptr+2), TRUE,
				 (STATUS *)NULL);
	STcopy(cptr, gl_path[i]);
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr, ERx("getLocation04> path = %s\n"), gl_path[i]);

	if (typeOfLoc)
	    *typeOfLoc = PATH;

	cptr = CMnext(next);
	next = STindex(cptr, ERx(","), 0);
	i++;
    }
    gl_path[i] = NULL;

    Locptr1 = &aLoc;
    Locptr2 = &bLoc;


#ifndef MPE
    if (gl_dev || gl_path[0])
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr, ERx("getLocation05> SEP_LOcompose()\n"));

	ret_val = SEP_LOcompose(gl_dev, gl_path[0], NULL, NULL, NULL, Locptr1);
#ifdef UNIX
	cptr2 = SEP_CMlastchar(Locptr1->string,0);
	if (*cptr2 == '/') *cptr2 = EOS;
	Locptr1->path = Locptr1->string;
#else
#ifdef NT_GENERIC
	cptr2 = SEP_CMlastchar(Locptr1->string,0);
	if (*cptr2 == '\\') *cptr2 = EOS;
	Locptr1->path = Locptr1->string;
#endif
#endif
    }

    for (i=1; (gl_path[i] != NULL)&&(ret_val == OK); i++)
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr, ERx("getLocation06> LOfaddpath()\n"));

	if ((ret_val = LOfaddpath(Locptr1, gl_path[i], Locptr2)) != OK)
	    continue;

	Locptr3 = Locptr1;
	Locptr1 = Locptr2;
	Locptr2 = Locptr3;
    }
#endif

    if ((gl_file != NULL)&&(ret_val == OK))
    {
	/*
	** This first case catches the instance of: @file(filenane.type).
	** This was caught above in looking for a device (gl_dev).
	*/
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr, ERx("getLocation07> LOfroms()\n"));

	ret_val = LOfroms( FILENAME, gl_file, Locptr1);
    }
    else
    if ((CMcmpcase(cptr, ERx(")")) != 0)&&(ret_val == OK))
    {
	/* There is a filename */

	next = STindex(cptr, ERx(")"), 0);

	*next = EOS;
	gl_file = SEP_MEalloc(SEP_ME_TAG_GETLOC, (int)(next-cptr+2), TRUE,
			      (STATUS *)NULL);
	STcopy(cptr, gl_file);
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr, ERx("getLocation08> file = %s\n"), gl_file);
	if (typeOfLoc)
	    *typeOfLoc = PATH & FILENAME;
	if (gl_dev || gl_path[0])
	{
#ifdef MPE
	    if (gl_dev)
	    {
		if (tracing&TRACE_PARM)
		    SIfprintf(traceptr, ERx("getLocation09> MPE's LOfroms ()\n"));

	        char *tmpMPE = NULL ;

		tmpMPE = SEP_MEalloc(SEP_ME_TAG_GETLOC,
				     (STlength(gl_file)+STlength(gl_dev)+1)*2,
				     TRUE, (STATUS *)NULL);

		STpolycat(3, gl_file, ERx("."), gl_dev, tmpMPE);
		LOfroms(FILENAME & PATH, tmpMPE, Locptr1);
	    }
#else
	    if (tracing&TRACE_PARM)
		SIfprintf(traceptr, ERx("getLocation09> LOfstfile()\n"));

	    LOfstfile(gl_file, Locptr1);
#endif
	}
	else
	{
	    if (tracing&TRACE_PARM)
		SIfprintf(traceptr, ERx("getLocation10> LOfroms()\n"));

	    LOfroms( FILENAME, gl_file, Locptr1);
	}
    }

    if (ret_val == OK)
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr, ERx("getLocation11> LOtos()\n"));

	LOtos(Locptr1, &resultPtr);
	STcopy(resultPtr, newstr);
    }
    MEtfree(SEP_ME_TAG_GETLOC);

    if (tracing&TRACE_PARM)
    {
	if (ret_val != OK)
	    SIfprintf(traceptr,ERx("getLocation12> ret_val = %d\n"),ret_val);
	else
	    SIfprintf(traceptr,ERx("getLocation13> newstr = %s\n"),newstr);
    }

    return(ret_val);
}

/*
** IS_FileNotation
*/

bool
IS_FileNotation(char *filenm)
{
    char                  *tmp_ptr = NULL ;

    if (tracing&TRACE_PARM)
    {
	SIfprintf(traceptr,ERx("IS_FileNotation> filenm = %s\n"),filenm);
    }

    if ((tmp_ptr = FIND_SEPstring(filenm,ERx("@FILE("))) == NULL)
    {
	if (tracing&TRACE_PARM)
	     SIfprintf(traceptr,ERx("IS_FileNotation> return FALSE\n"));

	return (FALSE);
    }
    if (tracing&TRACE_PARM)
    {
	SIfprintf(traceptr,ERx("IS_FileNotation> return TRUE\n"));
    }
    return (TRUE);
}

STATUS
Trans_SEPfile(char **Token,LOCTYPE *typeOfLoc,bool Dont_free,i2 ME_tag)
{
    STATUS                 syserr ;
    char                  *newstr = NULL ;
    char                  *temp_ptr = NULL ;
    char                  *tmp_buffer1 = NULL ;
    char                  *tmp_buffer2 = NULL ;
    char                  *tmp_ptr1 = NULL ;
    char                  *tmp_ptr2 = NULL ;
    char                  *tmp_ptr3 = NULL ;
    char                  *token_ptr = NULL ;
    char                  *tmp_token = NULL ;

    if (ME_tag == SEP_ME_TAG_NODEL && Dont_free != TRUE)
	Dont_free = TRUE;

    if (tracing&TRACE_PARM)
    {
	SIfprintf(traceptr,ERx("Trans_SEPfile> *Token = %s\n"),*Token);
	if (Dont_free)
	    SIfprintf(traceptr,ERx("                Dont_free = TRUE\n"));
	else
	    SIfprintf(traceptr,ERx("                Dont_free = FALSE\n"));
    }

    tmp_token = *Token;
    if ((token_ptr = FIND_SEPstring(tmp_token,ERx("@FILE("))) == NULL)
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr,ERx("Trans_SEPfile> FIND_SEPstring is FALSE\n"));

	return (FALSE);
    }
    if (tracing&TRACE_PARM)
        SIfprintf(traceptr,ERx("Trans_SEPfile> FIND_SEPstring is TRUE\n"));

    tmp_buffer1 = SEP_MEalloc(SEP_ME_TAG_MISC, 128, TRUE, (STATUS *) NULL);
    tmp_buffer2 = SEP_MEalloc(SEP_ME_TAG_MISC, 128, TRUE, (STATUS *) NULL);

    if (token_ptr != tmp_token)
    {
	for (tmp_ptr1 = tmp_buffer1, tmp_ptr2 = tmp_token;
	     tmp_ptr2 != token_ptr; )
	    CMcpyinc(tmp_ptr2,tmp_ptr1);
    }
    else
    {
	tmp_ptr2 = token_ptr;
    }

    for (tmp_ptr3 = tmp_buffer2; CMcmpcase(tmp_ptr2,ERx(")")); )
	CMcpyinc(tmp_ptr2,tmp_ptr3);

    CMcpychar(tmp_ptr2,tmp_ptr3);
    CMnext(tmp_ptr2);
    newstr = SEP_MEalloc(SEP_ME_TAG_MISC, 128, TRUE, (STATUS *) NULL);

    if ((syserr = getLocation(tmp_buffer2,newstr,typeOfLoc)) == OK)
    {
	if (tracing&TRACE_PARM)
	{
	    SIfprintf(traceptr,ERx("Trans_SEPfile> newstr = %s\n"),newstr);
	    SIfprintf(traceptr,ERx("Trans_SEPfile> tmp_buffer1 = %s\n"),
                      tmp_buffer1);
            SIfprintf(traceptr,ERx("Trans_SEPfile> tmp_buffer2 = %s\n"),
                      tmp_buffer2);
	}
	STcat(tmp_buffer1, newstr);
	STcat(tmp_buffer1, tmp_ptr2);

        if (tracing&TRACE_PARM)
            SIfprintf(traceptr,ERx("Trans_SEPfile> tmp_buffer1 = %s\n"),
                      tmp_buffer1);

	temp_ptr = *Token;
	*Token = STtalloc(ME_tag,tmp_buffer1);
        if (Dont_free != TRUE)
	    MEfree(temp_ptr);
    }
    MEfree(tmp_buffer1);
    MEfree(tmp_buffer2);
    MEfree(newstr);

    if (tracing&TRACE_PARM)
        SIfprintf(traceptr,ERx("Trans_SEPfile> *Token = %s\n"),*Token);
 
    return (syserr);
}
