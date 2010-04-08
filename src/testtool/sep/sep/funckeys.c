#include <compat.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <si.h>
#include <st.h>
#include <termdr.h>

#define funckeys_c

#include <sepdefs.h>
#include <funckeys.h>
#include <fndefs.h>

/*
** History:
**	##-XXX-1990 (eduardo)
**		Created.
**	24-may-1990 (rog)
**		Remove code duplicated from FTLIB.
**	09-Jul-1990 (owen)
**		Use angle brackets on sep header files in order to search
**		for them in the include search path.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	21-jul-1992 (dkhor)
**   	    (i) In 6.4/01/01 FKcmdtype() (see front/frontcl/ft/funckeys.c) 
**              expects four (4) args.  Update this file accordingly.
**	   (ii) Error mesg output should really use STprintf() instead of
**	        STcopy() when arguments are expected.
**	07-sep-1992 (donj)
**		Trying to be swift and improving on dkhor errmsg change, spelled
**		mapName as mapname. This change fixes it.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**	    Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALDEF    FUNCKEYS     *fkList = NULL;

GLOBALREF    char          terminalType [] ;

char         *Illegal_entry = ERx("ERROR: illegal entry, %s, in %s\n") ;
char         *failed_alloc  = ERx("ERROR: failed allocating memory for FK") ;

char                       mapName [MAX_LOC] ;

STATUS                     open_MapFile      ( FILE **mapPtr, char *Emsg ) ;
STATUS                     process_frscmmds  ( FUNCKEYS **fk, FUNCKEYS **lfk,
					       char *Emsg );
STATUS                     chk_for_legal_values ( char *buff2, char *cptr,
                                                  FUNCKEYS **lfk, char *Emsg ) ;
FUNCKEYS                  *found_in_frscmmds ( char *buff2 ) ;
FUNCKEYS                  *chk_for_others    ( char *buff2, char **cp,
					       STATUS *ioerr, char *Emsg ) ;
i4                         SEP_FKgetword     ( char **ppc, char *buf ) ;

STATUS
readMapFile(char *Emsg)
{
    char                  *cptr = NULL ;
    char                   buff1 [TEST_LINE] ;
    char                   buff2 [TEST_LINE] ;
    FILE                  *mapPtr = NULL ;
    FUNCKEYS              *fk  = NULL ;
    FUNCKEYS              *lfk = NULL ;
    STATUS                 ioerr ;
    bool                   close_mapFile = TRUE;
    bool                   end_of_file   = FALSE;

    if ((ioerr = open_MapFile(&mapPtr, Emsg)) != OK)
    {
	close_mapFile = FALSE;
    }

    ioerr = process_frscmmds(&fk, &lfk, Emsg);

    /*
    ** read MAP file
    */
    while ((ioerr == OK)&&(end_of_file == FALSE))
    {
	if ((ioerr = SIgetrec(buff1, TEST_LINE, mapPtr)) != OK)
	{
	    if (ioerr == ENDFILE)
	    {
		end_of_file = TRUE;
		ioerr = OK;
	    }
	}
	else
	{
            CVlower((cptr = buff1));
            /* 
            ** check left side of equation
            */
            if (SEP_FKgetword(&cptr, buff2) == FOUND)
            {
                /*
                ** check if found in FRS commands table. If not, check
                ** for other possibilities.
                */
                if ((lfk = found_in_frscmmds( buff2 )) == NULL)
                {
                    lfk  = chk_for_others( buff2, &cptr, &ioerr, Emsg );
                }

                if (ioerr == OK)
                {
		    /*
		    ** check for '=' sign
		    */
                    if (FKgetequal(&cptr) != FOUND)
                    {
                        ioerr = FAIL;
                        STprintf(Emsg, Illegal_entry, buff2, mapName );
                    }
                    else
                    /*
                    ** check for right side of equation
                    */
                    if (SEP_FKgetword(&cptr, buff2) != FOUND)
                    {
                        ioerr = FAIL;
                        STprintf(Emsg, Illegal_entry, buff2, mapName );
                    }
                    else
                    {
                        ioerr = chk_for_legal_values( buff2, cptr, &lfk, Emsg );
                    }
                }
            }
	}
    }

    if (close_mapFile)
    {
	SIclose(mapPtr);
    }

    return(ioerr);
}

FUNCKEYS
*found_in_frscmmds( char *buff2 )
{
    FUNCKEYS              *lfk = fkList ;
    FUNCKEYS              *rfk = NULL ;
    i4                     i ;

    /* check if found a FRS command */

    for (i = 0; i < FRSCMMDS; i++)
    {
	if (STcompare(frscmmds[i], buff2) == 0)
	{
	    rfk = lfk;
	    break;
	}
	else if (i != FRSCMMDS - 1)
	{
	    lfk = lfk->next;
	}
    }
    return (rfk);
}

FUNCKEYS
*chk_for_others( char *buff2, char **cp, STATUS *ioerr, char *Emsg )
{
    FUNCKEYS              *fk  = NULL ;
    FUNCKEYS              *lfk ;
    i4                     val1, val ;
    i4                     fk_cmd ;

    for (lfk = fkList; lfk->next; lfk = lfk->next)
	;
    switch (fk_cmd = FKcmdtype(buff2, &val1, &val, *cp))
    {
	case FDMENUITEM:
	case FDFRSK:
		FKgetnum(cp, &val);

		if (fk_cmd == FDMENUITEM)
		{
		    STprintf(buff2, ERx("MI%d"), val);
		}
		else
		{
		    STprintf(buff2, ERx("FRSK%d"), val);
		}

		if ((*ioerr = getNewFKey(&fk, buff2, Emsg)) == OK)
		{
		    lfk->next = fk;
		    lfk = fk;
		}
		break;
	case FDPFKEY: /* trying to disable a key, we don't care about this */
	case FDCTRL:
		break;
	default:
		*ioerr = FAIL;
                STprintf(Emsg, Illegal_entry, buff2, mapName );
		break;
    }
    return (lfk);
}

STATUS
chk_for_legal_values( char *buff2, char *cptr, FUNCKEYS **lfk, char *Emsg )
{
    STATUS                 ioerr = OK ;
    i4                     val1, val ;

    switch (FKcmdtype(buff2, &val1, &val, cptr))
    {
	case FDPFKEY:
	    FKgetnum(&cptr, &val);
	    if ((val < 1)||(val > 40))
	    {
		ioerr = FAIL;
		STprintf(Emsg, Illegal_entry, buff2, mapName );
	    }
	    else
	    {
		(*lfk)->string = STtalloc(SEP_ME_TAG_NODEL, *fktermcap[val-1]);
	    }
	    break;
	case FDCTRL:
	    cptr = SEP_MEalloc(SEP_ME_TAG_NODEL, 3, TRUE, (STATUS *) NULL);
	    switch (val)
	    {
		case CTRL_ESC:
		    cptr[0] = '\33';
		    break;
		case CTRL_DEL:
		    cptr[0] = '\177';
		    break;
		default:
		    cptr[0] = ('A' + val) & 037;
		    break;
	    }
	    (*lfk)->string = cptr;
	    break;
	case FDOFF: /*
		    ** user is disabling a key, we don't care about this
		    */
	    break;
	default:
	    ioerr = FAIL;
	    STprintf(Emsg, Illegal_entry, buff2, mapName );
	    break;
    }

    return (ioerr);
}


STATUS
getNewFKey(FUNCKEYS **newfk, char *label, char *Emsg)
{
    STATUS                 ioerr ;
    FUNCKEYS              *fk    = NULL ;
    char                  *cptr  = NULL ;

    *newfk = NULL;
    fk = (FUNCKEYS *)SEP_MEalloc(SEP_ME_TAG_NODEL,sizeof(FUNCKEYS),TRUE,&ioerr);
    if (ioerr != OK)
    {
	STcopy(failed_alloc,Emsg);
    }
    else
    {
	cptr = SEP_MEalloc(SEP_ME_TAG_NODEL, STlength(label)+3, TRUE, &ioerr);
	if (ioerr != OK)
	{
	    STcopy(failed_alloc,Emsg);
	}
	else
	{
	    STpolycat(3, ERx("`"), label, ERx("'"), cptr);
	    CVupper(cptr);
	    fk->label = cptr;
	    fk->string = NULL;
	    fk->next = NULL;
	    *newfk = fk;
	}
    }

    return(ioerr);
}

STATUS
open_MapFile(FILE **mapPtr, char *Emsg)
{
    STATUS                 ret_val = OK ;
    LOCATION               mapLoc ;
    char                  *cptr = NULL ;


    /* put together MAP file name */

    NMgtAt(ERx("II_SYSTEM"), &cptr);
    STcopy(cptr, mapName);
    LOfroms(PATH, mapName, &mapLoc);
    LOfaddpath(&mapLoc, ERx("ingres"), &mapLoc);
    LOfaddpath(&mapLoc, ERx("files"), &mapLoc);
    LOfstfile(terminalType, &mapLoc);
    LOtos(&mapLoc, &cptr);
    STcopy(cptr, mapName);
    STcat(mapName, ERx(".map"));

    /* open MAP file */

    LOfroms(PATH & FILENAME, mapName, &mapLoc);
    if ((ret_val = SIopen(&mapLoc, ERx("r"), mapPtr)) != OK)
    {
        STprintf(Emsg, ERx("ERROR: could not open %s\n"), mapName);
    }
    return (ret_val);
}

STATUS
process_frscmmds(FUNCKEYS **fk, FUNCKEYS **lfk, char *Emsg)
{
    STATUS                 ioerr = OK ;
    i4                     i ;

    /*
    ** Start creating list of function keys supported by adding
    ** FRS commands
    */

    for (i = 0;(i < FRSCMMDS)&&(ioerr == OK); i++)
    {
        if ((ioerr = getNewFKey(fk, frscmmds[i], Emsg)) == OK)
        {
            if (fkList == NULL)
            {
                fkList = *lfk = *fk;
            }
            else
            {
                (*lfk)->next = *fk;
                *lfk = *fk;
            }
        }
    }
    return (ioerr);
}

/*
**  Find a string of alphabetic characters, ignoring any leading
**  blanks or comments.
*/

i4
SEP_FKgetword( char **ppc, char *buf )
{
    i4                     retval = FOUND;
    char                  *cp;
    char                  *bp = buf;

    *bp = EOS;
    if (FKfindbeg(ppc) == ALLCOMMENT)
    {
        retval = NOTFOUND;
    }
    else
    {
        cp = *ppc;

	if (CMnmstart(cp))
	{
	    CMcpyinc(cp, bp);
	    while (CMnmchar(cp)&&(!CMdigit(cp)))
	    {
		CMcpyinc(cp, bp);
	    }
	    *bp = EOS;
	    if (STcompare(buf,ERx("control")) == 0)
	    {
		char *cp2 = cp;
		CMnext(cp2);

		if ((*cp != ' ')&&(*cp2 == ' '))
		{
		    CMcpyinc(cp, bp);
		}
	    }
	}

        *bp = EOS;
        if (bp == buf)
        {
            retval = NOTFOUND;
        }
        else
        {
            *ppc = cp;
        }
    }
    return(retval);
}
