/*
** seplo.c
**
**
** History:
**	23-oct-1992 (DonJ)  Created.
**	08-dec-1992 (donj)
**	    Changed dont_free flag on call to Trans_SEPfile() to TRUE to try
**	    and fix a problem on UNIX.
**      18-jan-1993 (donj)
**          Wrap LOcompose() with SEP_LOcompose() to fix a UNIX bug.
**	04-feb-1993 (donj)
**	    Added UNIX specific code to compensate for UNIX CL LO
**	    peculiarities.
**      15-feb-1993 (donj)
**          Add a ME_tag arg to SEP_LOfroms.
**	17-feb-1993 (donj)
**	    Simplify SEP_LOcompose.
**	02-mar-1993 (donj)
**	    LO functions now return bad syntax error if location type
**	    doesn't match exactly what is given to the function.(LOfroms()).
**	09-mar-1993 (donj)
**	    Add a new function, SEP_LOwhatisit(), that determines what LOCTYPE
**	    to use for a string. Modify SEP_LOfroms() to use SEP_LOwhatisit().
**	    Also remove the Trans_SEPfile() call in SEP_LOfroms(). It's not
**	    used anywhere.
**	05-apr-1993 (donj)
**	    Fixed new function, SEP_LOwhatisit(). Wasn't working right for VMS.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**	16-aug-1993 (donj)
**	    Took out VMS parts. This file is now for unix only.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	11-sep-1995 (somsa01)
**	    Ported to NT_GENERIC platforms.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <si.h>
#include <st.h>

#define seplo_c

#include <sepdefs.h>
#include <fndefs.h>

STATUS
SEP_LOrebuild(LOCATION *locptr)
{
    LOINFORMATION          LocInfo ;
    char                  *loc_str = NULL ;
    i4                     infoflag ;
    i2                     itexists ;
    char                  *curBuf = NULL ;
    LOCATION               curLoc ;

    char                  *reb_dev = NULL ;
    char                  *reb_path = NULL ;
    char                  *reb_fpre = NULL ;
    char                  *reb_fsuf = NULL ;
    char                  *reb_fver = NULL ;
    char                  *cur_dev = NULL ;
    char                  *cur_path = NULL ;
    char                  *cur_fpre = NULL ;
    char                  *cur_fsuf = NULL ;
    char                  *cur_fver = NULL ;
    char                  *tmp_path = NULL ;

    loc_str  = STtalloc(SEP_ME_TAG_SEPLO, locptr->string);

    reb_dev  = SEP_MEalloc(SEP_ME_TAG_SEPLO, LO_DEVNAME_MAX+1,  TRUE, NULL) ;
    reb_path = SEP_MEalloc(SEP_ME_TAG_SEPLO, LO_PATH_MAX+1,     TRUE, NULL) ;
    reb_fpre = SEP_MEalloc(SEP_ME_TAG_SEPLO, LO_FPREFIX_MAX+1,  TRUE, NULL) ;
    reb_fsuf = SEP_MEalloc(SEP_ME_TAG_SEPLO, LO_FSUFFIX_MAX+1,  TRUE, NULL) ;
    reb_fver = SEP_MEalloc(SEP_ME_TAG_SEPLO, LO_FVERSION_MAX+1, TRUE, NULL) ;

    LOdetail(locptr, reb_dev,  reb_path, reb_fpre, reb_fsuf, reb_fver);

    SEP_LOcompose(reb_dev, reb_path, reb_fpre, reb_fsuf, reb_fver, locptr);

    infoflag = LO_I_ALL;
    itexists = (LOinfo(locptr, &infoflag, &LocInfo) == OK);
    LOwhat(locptr,&infoflag);

    MEtfree(SEP_ME_TAG_SEPLO);
    return (OK);
}

bool
SEP_LOexists(LOCATION *aloc)
{
    LOINFORMATION          LocInfo ;
    i4                     infoflag ;

    infoflag = LO_I_TYPE;
    if (LOinfo(aloc, &infoflag, &LocInfo) == OK)
	return (TRUE);
    else
	return (FALSE);

}

STATUS
SEP_LOffind(char *path[],char *filename,char *fileBuffer,LOCATION *fileloc)
{
    STATUS                 return_val ;
    i4                     i ;

    return_val = FAIL;
    for (i=0; ((path[i] != NULL)&&(*path[i] != EOS))
	      &&(return_val != OK); i++)
    {
	STcopy(path[i], fileBuffer);
	LOfroms(PATH, fileBuffer, fileloc);
	LOfstfile(filename, fileloc);
	if (SEP_LOexists(fileloc))
	    return_val = OK;
	else
	    return_val = FAIL;
    }
    return (return_val);
}

STATUS
SEP_LOfroms(LOCTYPE flag,char *string,LOCATION *loc,i2 ME_tag)
{
    STATUS                 return_val ;
    char                  *tmp_ptr = NULL ;


    tmp_ptr = SEP_MEalloc(ME_tag, MAX_LOC+1, TRUE, NULL);
    STcopy(string, tmp_ptr);

    if (flag == 0)
	flag = SEP_LOwhatisit(tmp_ptr);

    return_val = LOfroms(flag, tmp_ptr, loc);

    /* MEfree(string); */
    return (return_val);
}

STATUS
SEP_LOcompose(char *SEPLOdev,char *SEPLOpath,char *SEPLOfpre,char *SEPLOfsuf,char *SEPLOfver,LOCATION *locp)
{
    STATUS  ret_val = OK ;

    char    tmpstr [MAX_LOC+1] ;

    *tmpstr = EOS;

    if (SEPLOdev  != NULL && *SEPLOdev  == EOS) SEPLOdev  = NULL;
    if (SEPLOpath != NULL && *SEPLOpath == EOS) SEPLOpath = NULL;
    if (SEPLOfpre != NULL && *SEPLOfpre == EOS) SEPLOfpre = NULL;
    if (SEPLOfsuf != NULL && *SEPLOfsuf == EOS) SEPLOfsuf = NULL;
    if (SEPLOfver != NULL && *SEPLOfver == EOS) SEPLOfver = NULL;

    if (SEPLOdev  == NULL && SEPLOpath == NULL)
    {
	STcopy(ERx("."),tmpstr);
    }
    else if (SEPLOdev == NULL)
    {
	STcopy(SEPLOpath,tmpstr);
    }
    else if (SEPLOpath == NULL)
    {
	STcopy(SEPLOdev,tmpstr);
    }
#ifdef NT_GENERIC
    else if (*SEPLOpath == '\\')
#else
    else if (*SEPLOpath == '/')
#endif
    {
	STpolycat(2,SEPLOdev,SEPLOpath,tmpstr);
    }
    else
#ifdef NT_GENERIC
	STpolycat(3,SEPLOdev,ERx("\\"),SEPLOpath,tmpstr);
#else
	STpolycat(3,SEPLOdev,ERx("/"),SEPLOpath,tmpstr);
#endif

    if ((ret_val = LOcompose(NULL, tmpstr, SEPLOfpre,
			     SEPLOfsuf, SEPLOfver, locp)) == 2305)
	ret_val = OK;

    return (ret_val);
}


LOCTYPE
SEP_LOwhatisit(char *f_string)
{
    LOCTYPE                what_itis ;
    char                  *cptr1 = NULL ;
    char                  *cptr2 = NULL ;

    if (f_string == NULL || *f_string == EOS)
	return (NULL);

    what_itis = NULL;


    if (STindex(f_string,ERx("/"),0) == NULL)
    {
	if (CMcmpnocase(f_string,ERx("$")) == 0)
	{
	    cptr1 = f_string;
	    CMnext(cptr1);
	    NMgtAt(cptr1,&cptr2);
	    if (cptr2 == NULL || *cptr2 == EOS)
		return (NULL);

	    if ((what_itis = SEP_LOwhatisit(cptr2)) != NULL)
		STcopy(cptr2,f_string);

	    return (what_itis);
	}

	if (SEP_LOisitaPath(f_string))
	    return (PATH);

	return (FILENAME);
    }

    if (CMcmpnocase(SEP_CMlastchar(f_string,0),ERx("/")) == 0)
	return (PATH);

    if (SEP_LOisitaPath(f_string))
	return (PATH);

    return (FILENAME & PATH);

}

bool
SEP_LOisitaPath(char *f_string)
{
    LOCATION               whatlo ;
    char                   whats [MAX_LOC+1] ;
    LOINFORMATION          LocInfo ;
    i4                     infoflag ;

    STcopy(f_string,whats);
    if (LOfroms(PATH, whats, &whatlo) != OK)
	return (FALSE);

    infoflag = LO_I_TYPE;

    if (LOinfo(&whatlo, &infoflag, &LocInfo) != OK)
	return (FALSE);

    if ((infoflag & LO_I_TYPE) == 0 || LocInfo.li_type != LO_IS_DIR)
	return (FALSE);

    return (TRUE);
}
