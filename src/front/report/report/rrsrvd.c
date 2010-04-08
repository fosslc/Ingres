/*
** Copyright (c) 2004 Ingres Corporation
*/
# include       <compat.h>
# include       <st.h>          /* 6-x_PC_80x86 */
# include       <dbms.h>
# include       <fe.h>
# include       <adf.h>
# include       <fmt.h>
# include        <rtype.h>
# include        <rglob.h>
# include       <er.h>
 
/*
**   R_RESERVED:  routine for reserved word checking.
**
**      Parameters:
**              name    name of potential reserved word.
**
**      Returns:
**
**      Side Effects:
**              Negligeable increase in report preparation time.
**
**      Error messages:
**
**      Trace Flags:
**
**      History:
**      24-feb-95 (angusm)        Initial version for bug 51071
**	10-apr-95 (harpa06)
**                Added to OPING1.1 code line from 6.4 where angusm had made the
**	          change
*/
 
 
/*
**      Reserved word table.
**
**      To extend, add entry with status value (FATAL/NONFATAL)
**      to return to caller.
**
**      This is so that in future, all reserved word checking for
**      RW attributes can be brought within a single function.
**
*/
 
struct  res_word
{
        char    *name;                  /* ptr to one allowable name */
        i2      code;                   /* associated constant code */
};
 
static struct   res_word Reswords[] =
{
        ERx("length"), FATAL,
        {0},            {0}
};
 

/*
**   R_GT_RWORD - check word against table of reserved words.
**
**      Parameters:
**              *ctext - name to check. May contain upper or lower
**                      case characters.
**
**      Returns:
**              Code of the constant(FATAL/NONFATAL),
**              if it exists.  Returns NONFATAL if
**              the command is not in the list.
**
**      Side Effects:
**              none.
**
**      Trace Flags:
**
**      History:
**      24-feb-95 (angusm)        Initial version for bug 51071
*/
 
i2
r_gt_rword(ctext)
char    *ctext;
 
{
        /* internal declarations */
 
        register struct res_word        *pc;            /* ptr to name table */
 
        for (pc = Reswords; pc->name; pc++)
        {       /* check next command */
                if ((STbcompare(pc->name, 0,ctext, 0, TRUE) == 0))
                        return(pc->code);
        }
 
        /* not in table */
 
        return (NONFATAL);
}

