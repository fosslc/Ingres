/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<cu.h>
# include       <ug.h>
#include	"ercu.h"
#include	"curtc.h"

/**
** Name:	curecord.c -	Copy Utility Record Manipulation Module.
**
** Description:
**	This files contains routines that manipulate curecords.
**
**	cu_recfor	Given a rectype return a CURECORD.
**	IICUltdLineToDescriptor	Convert from buffer to query descriptor.
**
** History:
**	25-jun-1987 (Joe)
**		Initial Version
**	08/12/88 (dkh) - Fixed IICUltdLineToDescriptor to not corrupt stack.
**	05-dec-88 (kenl)
**		Changed FEalloc() calls to FEreqmem().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF CURTC		iiCUrtcRectypeToCurecord[];

char	*cu_gettok();

static STATUS 	cu_convert();
static STATUS 	cu_init();

/*{
** Name:	cu_recfor()	- Given a rectype return a CURECORD.
**
** Description:
**	Given a rectype this finds the CURECORD for it and initializes
**	it.  Any memory allocated is done using FEreqmem, so the caller
**	can declare a tag around this.
**
** Inputs:
**	rectype		{OOID} The record type for which to get a CURECORD.
**
** Returns:
**	{CURECORD *}	A CURECORD for the type, or NULL if the rectype is bad.
**
** Side Effects:
**	May allocate memory using FEreqmem.
**
** History:
**	25-jun-1987 (Joe)
**		First Written
**	25-apr-1991 (jhw)  Corrected 'rectype' to be OOID type.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
CURECORD *
cu_recfor ( rectype )
register OOID	rectype;
{
	register CURTC	*crp;

	for ( crp = iiCUrtcRectypeToCurecord ; crp->curectype != CU_BADTYPE ;
			++crp )
	{
		if ( crp->curectype == rectype )
		{
			if ( ! crp->curec->cuinit )
			{
				cu_init(crp->curec);
			}
			return crp->curec;
		}
	}
	return NULL;
}

/*{
** Name:	cu_kindof() -	Get the kind of a rectype.
**
** Description:
**	Given a rectype, this returns one of:
**
**		CU_OBJECT
**		CU_DETAIL
**		CU_COMPONENT
**		CU_ENCODING
**		CU_BADTYPE
**
**	depending of the kind of rectype
**
** Inputs:
**	rectype		The rectype to get the kind of.
**
** Returns:
**	{nat}	The kind of rectype it is.
**
** History:
**	3-jul-1987 (Joe)
**		Initial Version.
*/
i4
cu_kindof ( rectype )
i4	rectype;
{
    if (rectype > 0)
	return CU_OBJECT;
    rectype = -rectype;
    return ((rectype&CU_MASK) == 0 ? CU_BADTYPE : (rectype&CU_MASK));
}

/*{
** Name:	cu_init		- Initialize a CURECORD
**
** Description:
**	This routine initializes a CURECORD by allocating the argv
**	array.
**
** Inputs:
**	curec		The CURECORD to build the text value for.
**
** Outputs:
**	Returns:
**		OK if succeeded.
**		FAIL if couldn't allocate memory.
**
** Side Effects:
**	Allocates memory using FEreqmem.
**
** History:
**	25-jun-1987 (Joe)
**		First Written
*/
static STATUS
cu_init(curec)
CURECORD	*curec;
{
    STATUS	stat;

    curec->cuargv = (IISQLDA *)FEreqmem((u_i4)0, (u_i4)(IISQDA_HEAD_SIZE +
	(curec->cunoelms * IISQDA_VAR_SIZE)), TRUE, &stat);
    return(stat);
}

/*{
** Name:	IICUltdLineToDescriptor - Convert from file line to query
**
** Description:
**	Converts from a line read from the file into a query descriptor.
**	For this implementation, the query descriptor is an insert
**	param string and an argv.
**
** Inputs:
**	curec		The CURECORD whose text values are to be filled.
**
**	inbuf		The file line.
**
**	ids		The id(s) for the parent(s).
**
** History:
**	15-jun-1987 (Joe)
**		Initial Version.
**      13-Oct-2008 (hanal04) Bug 120290
**              If cuupdabfsrc is TRUE we are dealing with the first detail
**              line of an OC_AFORMREF record and must update the abf_source
**              held in the first token with the new object_id of the object
**              we are generating.
**      15-Jan-2009 (hanal04) Bug 121512
**              If cuupdabfsrc is TRUE ignore NULL object_id values and
**              use a tmp buffer to replace non-null object_id values
**              which may have used fewer digits than the new ID.
**      13-Jul-2009 (hanal04) Bug 122276
**              If cuupdabfsrc is TRUE check for the case where the old
**              object_id value may have used fewer digits than the new ID.
**              If this is the case a temporary buffer can not be used as
**              the tokenized input buffer is used after exiting this
**              routine. Instead we have to re-tokenise the buffer making
**              space for the new Object ID.
*/
VOID
IICUltdLineToDescriptor(curec, inbuf, ids)
CURECORD	*curec;
char		*inbuf;
IDSTACK		*ids;
{
    char		*FEsalloc();
    /*
    **  Note that the declaration is CU_TOKMAX + 1 due to
    **  the way the loop works below.
    */
    char		*tokens[CU_TOKMAX + 1];
    i4			i;
    i4			numtok;
    IISQLDA		*a;
    CUTLIST		*t;

    /*
    ** This buffer is big enough to hold the insert string since
    ** an element is name=%XX,
    ** where name <= FE_MAXNAME.
    */
    char	buf[(FE_MAXNAME+5)*(CU_TOKMAX+CU_LEVELMAX)];
    char	*bp;
    bool	buildinsstr = FALSE;
    char        tmptok[FE_MAXNAME];

    if (curec->cuinsstr == NULL)
	buildinsstr = TRUE;
    for (cu_gettok(inbuf), i = 0;
	 (tokens[i] = cu_gettok(NULL)) != NULL;
	 i++)
    {
	;
    }

    if(curec->cuupdabfsrc)
    {
        /* Object Id src refs will not be valid on a reload, use the new id */
        if(*tokens[0] != EOS)
        {
            STprintf(tmptok, "%d", ids->idstk[ids->idtop]);
            if(STlength(tokens[0]) < STlength(tmptok))
            {
                char *ptr = tokens[i - 1];
                i4 shift = STlength(tmptok) - STlength(tokens[0]);
                i4 j;
                
                ptr += STlength(tokens[i - 1]);

                if((ptr + shift) > (inbuf + CU_LINEMAX))
                    IIUGerr(E_CU0016_CNVT_TO_QRY_ERR, UG_ERR_FATAL, 0);

                for ( ; ptr >= tokens[1] ; ptr-- )
                {
                    *(ptr + shift) = *ptr;
                }     
                for (j=1 ; j < i ; j++)
                {
                    tokens[j] += shift;
                }
            }
            STprintf((PTR)tokens[0], "%d", ids->idstk[ids->idtop]);
        }
    }

    numtok = i-1;
    for ( ; i < CU_TOKMAX; ++i)
	tokens[i] = ERx("");

    a = curec->cuargv;
    bp = buf;
    if (buildinsstr)
    {	
	a->sqln = curec->cunoelms;
	a->sqld = curec->cunoelms;
	*bp++ = '(';
    }
    for (t = curec->cutlist, a = curec->cuargv, i = 0;
	 i < curec->cunoelms;
	 i++, t++)
    {
	if (buildinsstr)
	{
	    STcopy(t->cuname, bp);
	    bp += STlength(t->cuname);
	    *bp++ = ',';
	}
	if (t->cuinssrc == CUFILE || t->cuinssrc == CUOFILE)
	{
	    cu_convert(tokens[t->cuoffset-1], &(t->cudbv));
	    a->sqlvar[i].sqltype = DB_DBV_TYPE;
	    a->sqlvar[i].sqllen = sizeof(DB_DATA_VALUE);
	    a->sqlvar[i].sqldata = (char *) &t->cudbv;
	}
	else
	{
	    a->sqlvar[i].sqltype = DB_INT_TYPE;
	    a->sqlvar[i].sqllen = sizeof(i4);
	    a->sqlvar[i].sqldata = (char *) &(ids->idstk[t->cuoffset-1]);
	}
    }

    if (buildinsstr)
    {
    	/* build the list of question marks for the dynamic insert */
	*--bp = ')';
	bp++;
	STcopy(ERx(" VALUES ("), bp);
	bp = buf + STlength(buf);
	for (i = 0; i < curec->cunoelms; i++)
	{
	    *bp++ = '?';
	    *bp++ = ',';
	}

	/*
	** Always put out a , after each element.
	** put \0 over last one.
	*/
	*--bp = ')';
	*++bp = '\0';
	curec->cuinsstr = FEsalloc(buf);
    }
}

/*{
** Name:	cu_convert	-  Convert from token to data.
**
** Description:
**	Since the current implementation only understands the simple
**	types, this routines converts from a token in the line buffer
**	to a data element.
**
**	If the type of the element is character, then is simply sets
**	the data pointer to point at the line buffer.  Otherwise it
**	converts from the line buffer to the int or float value.
**
** Inputs:
**	token		The token from the line buffer.
**
**	dbv		The DB_DATA_VALUE to convert to.
**
**	Returns:
**		OK if succeeded.
**		otherwise a conversion error.
**
** History:
**	3-jul-1987 (Joe)
**		Initial Version
*/
static STATUS
cu_convert(token, dbv)
char		*token;
DB_DATA_VALUE	*dbv;
{
    STATUS		rval;
    DB_TEXT_STRING	*ts;
    i4			len;

    switch (dbv->db_datatype)
    {
      case DB_VCH_TYPE:
	IICUbsdBSDecode(token);
	len = STlength(token);

	/* yes, there's actually one extra byte - needed for STcopy */
	ts = (DB_TEXT_STRING *) FEreqmem((u_i4) 0,
				sizeof(DB_TEXT_STRING) + len, TRUE, &rval);

	if (ts == NULL)
		return rval;

	dbv->db_data = (PTR) ts;
	dbv->db_length = len+2;
	ts->db_t_count = len;
	STcopy(token,ts->db_t_text);
	break;

      case DB_CHR_TYPE:
	dbv->db_data = (PTR) token;
	dbv->db_length = STlength(token);
	break;

      case DB_INT_TYPE:
	if (dbv->db_data == NULL)
	{
	    if ((dbv->db_data = (PTR)FEreqmem((u_i4)0, (u_i4)sizeof(i4), TRUE,
		&rval)) == NULL)
	    {
		return rval;
	    }
	}
	/*
	** Since old copy files could have empty strings for
	** zero we check for a NULL token.  If it is then
	** make it zero.  Also, if we defaulted a non-existent
	** token, we have an empty string.
	*/
	if (token == NULL || *token == EOS)
	{
	    token = ERx("0");
	}

	/*
	** !!!HACK!!!
	**
	** We really should convert to the appropriate length here, but
	** for now just stuff the length so the query we eventually run will
	** work.
	*/
	dbv->db_length = sizeof(i4);
	return CVal(token, (i4 *)dbv->db_data);
	break;

      case DB_FLT_TYPE:
	if (dbv->db_data == NULL)
	{
	    if ((dbv->db_data = (PTR)FEreqmem((u_i4)0, (u_i4)sizeof(f8), TRUE,
		&rval)) == NULL)
	    {
		return rval;
	    }
	}
	/*
	** See comment above.
	*/
	if (token == NULL || *token == EOS)
	{
	    token = ERx("0.0");
	}
	return CVaf(token, '.', (f8 *)dbv->db_data);
	break;
    }
    return OK;
}
