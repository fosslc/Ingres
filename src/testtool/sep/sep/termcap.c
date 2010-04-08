/*
**   termcap.c
**
**   This file contains routines to read SEP termcap info
**
**   History:
**
**   March-89	(eduardo) created it
**   June-89	(mca)	  Added septerm stuff
**   7-Nov-1989  (eduardo)
**		Added pathname paramater to load_termcap() routine
**   27-Feb-90   (Eduardo)
**		Added support for function keys
**   28-Feb-90   (Eduardo)
**		Implemented use of termcap variables instead of reading
**		values from termType.sep file
**   09-Jul-1990 (owen)
**		Use angle brackets on sep header files in order to search
**		for them in the include search path.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**	02-apr-1992 (donj)
**	    Removed unneeded references. Changed all refs to '\0' to EOS.Changed
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)". Implement other CM routines
**          as per CM documentation.
**	10-jul-1992 (donj)
**	    Using ME memory tags.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	30-nov-1992 (donj)
**	    Use SEP_LOexists() as a wrapper for LOexists().
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**	 7-may-1993 (donj)
**	    comp_seqs() needs not to be a VOID. I made it a nat.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      13-apr-1994 (donj)
**          Fix some return typing. Making sure that all funtions return an
**          acceptable value in every case.
**	24-may-1994 (donj)
**	    remove the use of a hidden global var "buffer_1" use a passed param
**	    "errmsg" instead.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <me.h>

#define termcap_c

#include <termdr.h>
#include <sepdefs.h>
#include <fndefs.h>

GLOBALDEF    char         *PROMPT_POS = NULL ;
GLOBALDEF    char         *DEL_EOL = NULL ;
GLOBALDEF    char         *ATTR_OFF = NULL ;
GLOBALDEF    char         *REV_VIDEO = NULL ;
GLOBALDEF    char         *BLNK = NULL ;
GLOBALDEF    char         *DRAW_LINES = NULL ;
GLOBALDEF    char         *DRAW_CHAR = NULL ;
GLOBALDEF    char         *REG_CHAR = NULL ;
GLOBALDEF    bool          USINGFK = FALSE ;
GLOBALDEF    KEY_NODE     *termKeys = NULL ;

GLOBALREF    char          buffer_1 [SCR_LINE] ;
GLOBALREF    FUNCKEYS     *fkList ;
GLOBALREF    char          terminalType [] ;

/*
** define positions for hard coded prompts
*/

#define HCP_COL	(i4)0
#define HCP_ROW	(i4)23

/*
**  load_termcap
*/

STATUS
load_termcap( char *pathname, char *errmsg )
{
    char                   keyBuffer [TEST_LINE] ;
    char                   wrkBuffer [TEST_LINE] ;    
    char                   termName [MAX_LOC] ;
    char                   termFileName [MAX_LOC] ;
    char                  *cp = NULL ;
    char                  *colon = NULL ;
    char                  *cptr = NULL ;
    short                  count ;
    STATUS                 ioerr ;
    LOCATION               capLoc ;
    FILE                  *capPtr = NULL ;
    KEY_NODE              *newnode = NULL ;
    
/*
**  read basic keys
**  This is the written representation of the basic keys (esc,bell,^S, etc.)
*/

    STcopy(pathname, wrkBuffer);
    LOfroms(PATH, wrkBuffer, &capLoc);
    LOfstfile(ERx("basickeys.sep"), &capLoc);
    LOtos(&capLoc, &cp);
    STcopy(cp, termFileName);
    if (LOfroms(FILENAME & PATH,termFileName,&capLoc) != OK)
    {
	STprintf(errmsg,
	    ERx("ERROR: could not create location for basic keys file"));
	return(FAIL);
    }
    if (!SEP_LOexists(&capLoc))
    {
	STprintf(errmsg,
	    ERx("ERROR: basic keys file does not exist"));
	return(FAIL);
    }
    if (SIopen(&capLoc,ERx("r"),&capPtr) != OK)
    {
	STprintf(errmsg,
	    ERx("ERROR: could not open basic keys file "));
	return(FAIL);
    }
/*
**  read basic keys file
*/
    while ((ioerr = SIgetrec(keyBuffer,TEST_LINE,capPtr)) == OK)
    {
	keyBuffer[STlength(keyBuffer) - 1] = EOS; /* get rid of nl */
	cp = keyBuffer + 3;
	colon = STindex(cp,ERx(":"),0);
	if (colon == NULL)
	{
	    SIclose(capPtr);
	    STprintf(errmsg,ERx("ERROR: illegal format in basic keys file"));
	    return(FAIL);
	 }
	 cptr = SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(KEY_NODE), TRUE, (STATUS *) NULL);
	 newnode = (KEY_NODE *)cptr;

	 *colon = EOS;
	 CMnext(colon);
	 newnode->key_label = STtalloc(SEP_ME_TAG_NODEL,cp);
	 cp = wrkBuffer;
	 decode(colon,cp);
	 newnode->key_value = STtalloc(SEP_ME_TAG_NODEL,wrkBuffer);
	 newnode->left = newnode->right = NULL;
	 insert_key_node(&termKeys,newnode);
    }
    if (ioerr != ENDFILE)
    {
	STprintf(errmsg,ERx("ERROR: while reading basic keys file"));
	return(FAIL);
    }
    SIclose(capPtr);
/*
**  set SEP termcap variables
*/
    if ((STcompare(terminalType, ERx("septerm")) != 0) &&
        (STcompare(terminalType, ERx("vt100")) != 0))
       USINGFK = TRUE;

    cp = TDtgoto(CM, HCP_COL, HCP_ROW);
    PROMPT_POS = STtalloc(SEP_ME_TAG_NODEL,cp);

    DEL_EOL = CE;
    ATTR_OFF = EA;
    REV_VIDEO = RV;
    BLNK = BL;
    DRAW_LINES = LD;
    DRAW_CHAR = LS;
    REG_CHAR = LE;

    if (!USINGFK)
	return(OK);
/*
**  read function keys
*/
    return(readMapFile(errmsg));
}

/*
**  insert_key_node
*/

STATUS
insert_key_node(KEY_NODE **master,KEY_NODE *anode)
{
    STATUS                 ret_val = OK ;
    KEY_NODE              *head = NULL ;
    i4                     cmp ;

    if (*master == NULL)
    {
        *master = anode;
    }
    else
    {
        for (head = *master;;)
        {
            cmp = STcompare(anode->key_label,head->key_label);
            if (cmp < 0)
            {
                if (head->left)
                {
                    head = head->left;
                    continue;
                }
                else
                {
                    head->left = anode;
                    break;
                }
            }
            else
            {
                if (head->right)
                {
                    head = head->right;
                    continue;
                }
                else
                {
                    head->right = anode;
                    break;
                }
            }
        } /* end of for */
    } /* end of main if */
    return (ret_val);
}

/*
**  decode
*/

STATUS
decode(char *source,char *dest)
{
    STATUS                 ret_val = OK ;
    char                   c ;
    char                  *dp = NULL ;
    char                  *src = source ;
    char                  *dst = dest ;
    i4                     i ;

    while ((c = *src) != EOS)
    {
	CMnext(src);
	switch(c)
	{
	    case '^':
		c = *src & 037;
		CMnext(src);
		break;
	    case '\\':
		dp = ERx("E\033^^\\\\::n\nr\rt\tb\bf\f");
		c = *src;
		CMnext(src);
		do
		{
		    if (*dp == c)
		    {
			CMnext(dp);
			c = *dp;
			CMnext(dp);
			break;
		    }
		    CMnext(dp);
		    CMnext(dp);
		}
		while (*dp != EOS);
		if(CMdigit(&c))
		{
		    c -= '0';
		    i = 2;
		    do
		    {
			c <<= 3;
			c |= *src - '0';
			CMnext(src);
		    }
		    while (--i && CMdigit(src));
		}
		break;

	} /* end of switch */

	*dst = c;
	CMnext(dst);

    } /* end of while */

    *dst = EOS;
    return (ret_val);
}


/*
**  comp_seqs
*/

i4
comp_seqs(char *seq1,char *seq2)
{
   register unsigned char *ap = (unsigned char *) seq1 ;
   register unsigned char *bp = (unsigned char *) seq2 ;
   register i4             result = 0 ;
   register i4             len = 0 ;


	len = STlength((char *)ap);
        while (len-- && (result = CMcmpcase(ap,bp)) == 0)
        {
                if (*ap == EOS || *bp == EOS)
                        break;
                CMnext(ap);
                CMnext(bp);
        }
        if (result < 0)
                return (-1);
        if (result > 0)
                return (1);
        return(0);
}

/*
**  decode_seqs
*/

VOID
decode_seqs(char *source,char *dest)
{
    char                   buffer [10] ;
    char                  *key = NULL ;
    char                  *string1 = NULL ;
    char                  *string2 = NULL ;
    char                  *bracket = NULL ;
    FUNCKEYS              *fk = NULL ;

    string1 = source;
    string2 = dest;

    while ((*string1 != EOS) && (bracket = STindex(string1,ERx("`"),0)))
    {
        while (string1 != bracket)
            CMcpyinc(string1,string2);

        key = buffer;
        while (*bracket != '\'')
	    CMcpyinc(bracket,key);

	CMcpyinc(bracket,key);

        *key = EOS;
        string1 = bracket;

	if (USINGFK)
	{
		for (fk = fkList; fk != NULL; fk = fk->next)
			if (STbcompare(buffer, STlength(buffer),
				       fk->label, STlength(fk->label),
				       FALSE) == 0)
				break;

		if (fk != NULL)
		{
			if (fk->string)
				STcopy(fk->string, buffer);
			else
				STcopy(fk->label, buffer);
		}
		else
			get_key_value(termKeys,buffer);
	}
	else
	    get_key_value(termKeys,buffer);

	STcopy(buffer,string2);
	string2 += STlength(buffer);
    }
    while (*string1 != EOS)
        CMcpyinc(string1,string2);

    *string2 = EOS;
}

/*
**  encode_seqs
*/

VOID
encode_seqs(char *source,char *dest)
{
    char                   buffer [10] ;
    char                  *string1 = NULL ;
    char                  *string2 = NULL ;
    int                    len ;
    int                    len1 ;
    FUNCKEYS              *fk = NULL ;

    string1 = source;
    string2 = dest;

    while (*string1 != EOS)
    {
	if (CMalpha(string1))
	    CMcpyinc(string1,string2);
	else
	{
	    if (USINGFK && !CMalpha(string1))
	    {
		len1 = STlength(string1);
		for (fk = fkList; fk != NULL; fk = fk->next)
		{
			if (fk->string == NULL)
				continue;

			len = STlength(fk->string);
			if (STbcompare(fk->string, len, string1, len1, FALSE)
			    == 0)
			{
				string1 += len;
				break;
			}
		}
		if (fk != NULL)
		{
			STcopy(fk->label, string2);
			string2 += STlength(fk->label);
			continue;
		}
	    }
	    if ((!CMalpha(string1)) &&
		    (get_key_label(termKeys,&string1,buffer) == OK))
	    {
	    	STcopy(buffer,string2);
	    	string2 += STlength(buffer);
	    }
	    else
	        CMcpyinc(string1,string2);
	}
    }
    *string2 = EOS;
}

/*
**  get_key_value
*/

STATUS
get_key_value(KEY_NODE *master,char *label)
{
    KEY_NODE              *head = NULL ;
    i4                     cmp ;

    if (master == NULL || label == NULL)
        return(FAIL);

    for (head = master;;)
    {
	cmp = STcompare(label,head->key_label);
	if (cmp == 0)
	{
	    STcopy(head->key_value,label);
	    return(OK);
	}
	if (cmp < 0)
	{
	    if (head->left)
	    {
		head = head->left;
		continue;
	    }
	    else
		return(FAIL);
	}
	else
	{
	    if (head->right)
	    {
		head = head->right;
		continue;
	    }
	    else
		return(FAIL);
	}
    }
}

/*
**  get_key_label
*/

STATUS
get_key_label(KEY_NODE *master,char **value,char *label)
{
    char                  *achar = NULL ;

    if (master == NULL)
	return(FAIL);

    if (comp_seqs((char *)master->key_value,(char *)(*value)) == 0)
    {
	*value += STlength(master->key_value);
	STcopy(master->key_label,label);
	return(OK);
    }
    if (get_key_label(master->left,value,label) == OK)
	return(OK);

    return(get_key_label(master->right,value,label));
}
