# include       <te.h>
# include       <lo.h>
# include       <tc.h>
# include       <st.h>

/*
** TEtest is used to test forms programs.
** in_name and out_name are the names of files to open for reading and
** writing.
**
**  History:
**
**      2/85 (ac) -- made stdin nonbuffered.
**
**      8/85 (joe) -- Now uses IIfe_tpin for input.
**
**      Revision 30.3  86/02/06  18:27:18  roger
**      We can only reassign stdin to the keystroke file
**      when testing with AT&T curses, which unconditionally
**      reads therefrom.  This means that calls to TEswitch
**      will fail, and in particular, that the Keystroke
**      File Editor cannot be used with AT&T INGRES.
**
**      03/07/89 (leighb) -- added MSDOS switch.
**
**      20-mar-91 (fraser)
**          Moved TEunbuf from temisc.c
**	16-oct-1995 (canor01)
**	    When redirecting stdout, increment TEok2write to show that
**	    the screen should not be refreshed.
**	05-mar-1997 (canor01)
**	    Changed some GLOBALDEFs to GLOBALREFs and moved definitions
**	    to tedata.c
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/

GLOBALREF FILE   *IIte_fpin;
GLOBALREF int    IIte_use_tc;
GLOBALREF TCFILE *IItc_in;
GLOBALREF FILE   *IIte_fpout;



static VOID TEunbuf(FILE * fp);

i4
TEtest(
char    *in_name,
i4      in_type,
char    *out_name,
i4      out_type)
{
char        in[MAX_LOC];
LOCATION    in_loc;

    if ((in_name) && (*in_name != '\0') && (in_type != TE_NO_FILE))
    {
        switch(in_type)
        {
            case TE_SI_FILE:
                if ((IIte_fpin = freopen(in_name, "r", stdin)) == NULL)
                    return FAIL;
                TEunbuf(IIte_fpin);
                break;
            case TE_TC_FILE:
                 STcopy(in_name, in);
                 LOfroms(FILENAME, in, &in_loc);
                 if (TCopen(&in_loc, "r", &IItc_in) != OK)
                     return(FAIL);
                 IIte_use_tc = 1;
                 break;
        }
    }

    if (out_name && *out_name != '\0')
    {
        IIte_fpout = freopen(out_name, "w", stdout);
        if (IIte_fpout == NULL)
            return FAIL;
    }
    return OK;
}



/* make fp unbuffered */

static VOID
TEunbuf(
FILE *  fp)
{
     setbuf( fp, NULL );
}
