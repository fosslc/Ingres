/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <er.h>
# include       <lo.h>
# include       <ex.h>
# include	<si.h>
# include       <ooclass.h>
# include       <metafrm.h>
# include       <abclass.h>
# include       "framegen.h"
# include       <fglstr.h>
# include       <erfg.h>

/**
** Name:	fglupval.c	-	Produce lookup validation SELECT
**
** Description:
**	Produce the SELECT used for a lookup validation.  That is, do a 
**	singleton select (into simplefields or a tablefield row) which
**	sets the displayed field for a lookup table corresponding to the value
**	in the join field.
**
**	IIFGlupval	Produce lookup validation SELECT
**
** History:
**	20 Jun 1989 (Mike S)	Initial version
**	24 jul 1989 (Mike S)	Select into master join field/column
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
 
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	LSTR	*IIFGlac_lupvalAssignClause();

extern FILE *outfp;
extern METAFRAME *G_metaframe;
extern i4  G_fgqline;
extern char *G_fgqfile;

/* static's */

/*{
** Name:        IIFGlupval      - Generate a 4GL lookup validation
**
** Description:
**      Receive a request from the code generator to produce a lookup
**      validation select.  That is, to select the displayed lookup table
**      data from the database, based on the value in the displayed join column.**      We also receive the left-hand margin string and the current file and
**      line (for error messages).  We generate the SELECT needed and write
**      it to the output file.
**
** Inputs:
**      p_wbuf          char *          left-margin padding
**      infn            char *          template file name
**      line_cnt        i4              current line in template file
**      jindx           i4              index (in MFJOIN) of lookup join
**
** Outputs:
**      none
**
**      Returns:
**              STATUS                  OK      if Select was produced
**                                      FAIL    otherwise
**
**      Exceptions:
**              none
**
** Side Effects:
**              Select is written to output file
**
** History:
**      12 Jun 1989 (Mike S)    Initial version
*/
STATUS
IIFGlupval(p_wbuf, infn, line_cnt, jindx)
char    *p_wbuf;        /* Left-margin padding */
char    *infn;          /* name of input file currently processing */
i4      line_cnt;       /* current line number in input file */
i4      jindx;          /* Join index in MFJOIN */
{
        EX              FEjmp_handler();
        EX_CONTEXT      context;
        char            *form = ((USER_FRAME *)(G_metaframe->apobj))->form->name;
			/* Form name */
        MFJOIN          *jptr = G_metaframe->joins[jindx];	/* Lookup join*/
        bool            intblfld = (  jptr->type == JOIN_DETLUP
				   || (G_metaframe->mode & MFMODE) == MF_MASTAB
				   );
	MFTAB           *ptab = G_metaframe->tabs[jptr->tab_1];
        MFTAB           *ltab = G_metaframe->tabs[jptr->tab_2];
	MFCOL		*mcol = ptab->cols[jptr->col_1];

        /* Set up global variables */
        G_fgqline = line_cnt;
        G_fgqfile = infn;
 
        /* establish exception handler */
        if (EXdeclare(FEjmp_handler, &context) != OK)
        {
                /* Clean up and return failure */
                EXdelete();
                return(FAIL);
        }
 
        /* Output SELECT line */
        SIfprintf(outfp, ERx("%s%s"), p_wbuf, form);

        SIfprintf(outfp, ERx(" := REPEATED SELECT\n"));
 
        /* Output assignment clause */
        IIFGols_outLinkStr(IIFGlac_lupvalAssignClause(ltab, mcol, intblfld), 
		  	   outfp, p_wbuf);
 
        /* Output FROM clause */
        SIfprintf(outfp, ERx("%sFROM %s\n"), p_wbuf, ltab->name);
 
        /* Output WHERE clause */
        SIfprintf(outfp, ERx("%sWHERE "), p_wbuf);
	SIfprintf(outfp, ERx("%s.%s = :"), ltab->name,
		  ltab->cols[jptr->col_2]->name);
	if (intblfld)
		SIfprintf(outfp, ERx("%s."), TFNAME);
	SIfprintf(outfp, ERx("%s;\n"), ptab->cols[jptr->col_1]->alias);

        
        /*
        ** If we returned to here, no error occured.  Clean up and
        ** return success.
        */
        EXdelete();
        return(OK);
}

