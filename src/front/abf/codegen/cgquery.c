/*
** Copyright (c) 1992, 2008 Ingres Corporation
**      All rights reserved.
*/

#include        <compat.h>
# include       <cv.h>          /* 6-x_PC_80x86 */
#include        <si.h>
#include        <st.h>
#include        <er.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
#include        <fe.h>
#include        <abfcg.h>
#include        <iltypes.h>
#include        <ilops.h>
#include        <erit.h>
/* Interpreted Frame Object definition */
#include        <ade.h>
#include        <frmalign.h>
#include        <fdesc.h>
#include        <abfrts.h>
#include        <ilrffrm.h>

#include        "cggvars.h"
#include        "cgil.h"
#include        "cglabel.h"
#include        "cgout.h"
#include        "cgerror.h"


/**
** Name:        cgquery.c -             Code Generator Query Module.
**
** Description:
**      Generates C code from the IL statements for a query.  The OSL code
**      generator builds a QRY structure, and generates calls on the ABF
**      runtime routines to execute the query.  Defines:
**
**      IICGqryBuildQryGen()
**      IICGqrsQrySingleGen()
**      IICGqrrQryRowchkGen()
**      IICGqrgQrygenGen()
**      IICGqrnQryNextGen()
**      IICGqrbQryBrkGen()
**      IICGqrfQryFreeGen()
**      IICGqrcQryChildGen()
**      IICGqmdChkMDQry()       check for master-detail query.
**      IICGoqfQualfldEle()
**      IICGoqlQualInit()
**      IICGmessage()           generate code for message with pause.
**
** History:
**      Revision 6.4/03
**      09/20/92 (emerson)
**              Created IICGqrrQryRowchkGen and modified IICGqrsQrySingleGen,
**              IICGqrgQrygenGen, IICGqrnQryNextGen, and IICGqryBuildQryGen
**              to handle new QRYROWCHK and QRYROWGET (for bug 39582).
**              Also part of fix for bug 46471 in IICGqrnQryNextGen.
**
**      Revision 6.3/03/01
**      01/09/91 (emerson)
**              Remove 32K limit on IL (allow up to 2G of IL).
**              This entails introducing a modifier bit into the IL opcode,
**              which must be masked out to get the opcode proper.
**      02/26/91 (emerson)
**              Remove 2nd argument to IICGgadGetAddr (cleanup:
**              part of fix for bug 36097).
**      8-apr-92 (davel)
**              Fixed bug 43506 by explicitly sizing the IIqg_argv and IIqg_spec
**              arrays - this works around an HP compiler bug.
**      11-nov-94 (lawst01)
**              Fixed bug 48229 - in the '..start.c' file generated, the
**              'QRY_SPEC' IIqgen... structures all get genned as 'IIqgen1'
**              in the case of multiple occurrances.
**
**      Revision 6.3/03/00  90/05  wong
**      Abstracted 'IICGmessage()'.
**
**      Revision 6.0  87/07  arthur
**      Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

VOID    IICGmessage();
VOID    IICGqrbQryBrkGen();
VOID    IICGoqfQualfldEle();

/*
** Counter used in creating unique names for QRY and QDESC structures.
*/
static i4       qryIndex = 1;
/*
** Flag to indicate master-detail query.
*/
static bool     MdQryBuilt = FALSE;

/*{
** Name:    IICGqryBuildQryGen() -      Builds structure for OSL query
**
** Description:
**      Generates code for an OSL query by building a QRY structure.
**      The IL statements read as input and the C code generated
**      reflect the QDESC and QRY structures.  (See also the comments
**      in qg.h on QDESCs.)
**
** IL statements:
**      QRY VALUE STRING STRING // QRY var, form name, table field name (if any)
**      QRYSZE INT INT          // no. of fields, no. of query specifications
**      QRYPRED INT INT         // One QRYPRED/TL2ELM combination will
**      TL2ELM STRING STRING    // appear for each qualification function
**        ...                   // in any of the query's where clauses.
**      ENDLIST
**      TL2ELM STRING VALUE     // One for each field retrieved by the query:
**        ...                   //      name, DBV reference for field/column
**      ENDLIST
**      TL2ELM VALUE VALUE      // Query specification list:
**        ...                   //      Constant, predicate, or DBV reference,
**        ...                   //      which type of reference.
**      ENDLIST
**      [IL_QID     strvalQueryName intvalQueryId1 intvalQueryId2]
**      QRYEND INT INT STRING   // DML, query mode, putform string
**        same format for child query, if any
**      [ QRYCHILD VALUE VALUE ]
**
**      In the QRY statement, the first VALUE is the temporary QRY structure
**      allocated to hold the query.  The first STRING contains the name
**      of the form being retrieved into, and the second the table field
**      (if any).  This third STRING may be a NULL (0.)
**
**      In the QRYSZE statement, the first integer operand is the number of
**      QRY_ARGV structures for the retrieve (one for each field or column
**      retrieved by the query;) the second integer operand is the number of
**      QRY_SPEC elements that describe the query.  Both of these counts do not
**      include the NULL valued elements that terminate each array of QRY_ARGVs
**      or QRY_SPECs.  These operands are used only the by the OSL interpreter,
**      and can be ignored by the code generator, except that the second count
**      is being used to work around an HP C compiler bug.
**
**      For each qualification specification in any of the where clauses for the
**      query (including those in aggregates in target list expressions) there
**      will be one QRYPRED statement, followed by as many TL2ELM statements as
**      there are elements within the qualification specification.  The first
**      operand of the QRYPRED statement is the type of qualification; the
**      second is the number of elements within the qualifcation specification
**      (this operand is required by the OSL interpreter, but may be ignored by
**      the code generator.)  In each TL2ELM statement the first operand is the
**      name of a field; the second the database expression on the left-hand
**      side of the qualification element (e.g., "relname.colname".)  The list
**      is terminated by an ENDLIST statement.
**
**      The next list of TL2ELM statements corresponds to the elements in the
**      ARGV for the query.  The first operand in each statement is the name of
**      a field or column, the second the index of the corresponding DBV in the
**      'qg_base' array.  (This value can be decremented to get the actual index
**      within the DBV array for the frame/procedure object.)  This TL2ELM list
**      is terminated by an ENDLIST statement.
**
**      The next list of TL2ELM statements correspond to the QRY_SPEC array for
**      the query.  Each TL2ELM specifies a QRY_SPEC element, which consists of
**      either a constant, query predicate, or DBV reference, and the type of
**      the QRY_SPEC.  This list too is terminated by an ENDLIST.
**
**      In the QRYEND statement, the first INT is the DML (either DB_QUEL or
**      DB_SQL) and the second is the query mode (see "qg.h.")  The STRING is
**      an encoded string that specifies those fields that should be displayed
**      after being retrieved.
**
**      If this is the parent of a master-detail query, then the IL statements
**      describing this query will be followed by those for the detail query.
**      Last will come a QRYCHILD statement, whose VALUEs are the indexes
**      (again, plus one) of the parent and child QRY structures in the DBV
**      array.
**
**      IL_QID was added after support for repeated queries.  If a the IL
**      was generated before IL_QID was added, then it is possible to have a
**      repeat query without a IL_QID.  If this is found, then the query is run
**      non repeated.  This fixes JupBug #10015 since the old way query ids were
**      generated was not unique enough, but with IL_QID, they are generated
**      like the embedded languages.
**
**
** Inputs:
**      stmt    {IL *}  The beginning IL statement for which to generate code.
**
** History:
**      10/89 (jhw) -- Modified to support separate query sections (passed
**              queries only.  This supports the 'look_up()' system frame.)
**      01/90 (jhw) -- Modified to generate query ID using 'iiCGqryID'.
**              JupBug #7899.
**      13-feb-1990 (Joe)
**          Added support for IL_QID to fix JupBug #10015.
**      08/90 (jhw) -- Added support to pass DBDV in QRY structure for
**              arrays.  Bug #32963.
**      10/90 (jhw) -- Added definition (and initialization) of local 'IImode[]'
**              for bug #31929.
**      10/07/91 (emerson)
**              Generate the number of elements in the IIqg_spec array,
**              to work around a bug in the HP C compiler.
**              Previously, we could generate stuff like
**
**                      static QRY_SPEC IIqg_spec[] = {...};
**                      ...
**                      {
**                              ...
**                              static QRY_SPEC IIqg_spec[] = {...};
**                              ...
**                      }
**
**              which caused trouble if the two instances of IIqg_spec
**              were of different sizes.
**      11/22/91 (szeng)
**              Integrated emerson's change to IICGqryBuildQryGen
**              to work around an HP C compiler bug.
**      8-apr-92 (davel)
**              Handle the IIqg_argv array the same way as the above mentioned
**              IIqg_spec array.
**      09/20/92 (emerson)
**              Set qry->qr_version to QR_VERSION instead of QG_VERSION.
**              (Part of fix for bug 39582).
*/
static VOID     QsgenInit();
static VOID     QryArgvEle();
static VOID     QrySpecEle();
/*  11-nov-94   lawst01  48229                                          */
    i4          qry_spec_qgensuff = 1;

VOID
IICGqryBuildQryGen ( stmt )
register IL     *stmt;
{
    register IL *next;
    IL          qid_nm;
    IL          qid1;
    IL          qid2;
    IL          mode;
    i4          qualIndex = 1;  /* suffix for ABRT_QUAL struct names */
    register i4  qrySpecs;      /* current no. of query specs. */
    i4          qryClause;      /* current query clause */
    i4          qrySpecIndices[3];/* index for WHERE, FROM, ORDER BY clauses */
    char        qryname[FE_MAXNAME+1];
    char        qdescname[FE_MAXNAME+1];
    char        buf[CGSHORTBUF];
    i4          qargvs;
    i4          qspecs;

#define         MAX_QCLAUSE   (sizeof(qrySpecIndices)/sizeof(qrySpecIndices[0]))

    IICGobbBlockBeg();
    next = IICGgilGetIl(stmt);
    /*
    ** The information in the QRYSZE statement is needed for dynamic
    ** allocation by the interpreter,
    ** but can be ignored by the code generator, except that the second count
    ** (which contains the number of QRY_SPEC structures in IIqg_spec,
    ** not including the dummy "null" structure at the end)
    ** is being used to work around an HP C compiler bug.
    ** Also the first count (which contains the number of QRY_ARGV structures
    ** in IIqg_argv) is used to workaround this same hp compiler bug.
    */
    if ((*next&ILM_MASK) != IL_QRYSZE)
    {
        IICGerrError(CG_ILMISSING, ERx("qrysize"), (char *) NULL);
        return;
    }
    qargvs = (i4) ILgetOperand(next, 1) + 1;
    qspecs = (i4) ILgetOperand(next, 2);

    next = IICGgilGetIl(next);

    /*
    ** Read Qualification Specification.
    */
    while ((*next&ILM_MASK) == IL_QRYPRED)
    {
        IL      *pred_stmt;

        pred_stmt = next;
        IICGostStructBeg(ERx("static ABRT_QFLD"), 
                        STprintf(buf, ERx("IIqfld%d"), qualIndex),
                        0, CG_ZEROINIT
        );

        next = IICGgilGetIl(next);
        if ((*next&ILM_MASK) != IL_TL2ELM)
        {
            IICGerrError(CG_ILMISSING, ERx("tl2elm"), (char *) NULL);
        }
        else do
        {
            IICGoqfQualfldEle(next);
            next = IICGgilGetIl(next);
        } while ((*next&ILM_MASK) == IL_TL2ELM);
        if ((*next&ILM_MASK) != IL_ENDLIST)
            IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
        IICGoqfQualfldEle((IL *) NULL);
        IICGosdStructEnd();
        IICGostStructBeg(ERx("static ABRT_QUAL"), 
                        STprintf(buf, ERx("IIqual%d"), qualIndex),
                        0, CG_INIT
        );
        IICGoqlQualInit(pred_stmt, qualIndex);
        IICGosdStructEnd();
        IICGostStructBeg(ERx("static QSGEN"), 
                        STprintf(buf, ERx("IIqgen%d"), qualIndex),
                        0, CG_INIT
        );
        QsgenInit(qualIndex++);
        IICGosdStructEnd();
        next = IICGgilGetIl(next);
    } /* End qual. spec. while */

    /*
    ** Read in the 2-valued list of value and field name
    ** for the list of QRY_ARGVs.
    */
    IICGostStructBeg(ERx("static QRY_ARGV"), ERx("IIqg_argv"), qargvs, CG_INIT);
    while ((*next&ILM_MASK) == IL_TL2ELM)
    {
        QryArgvEle(next);
        next = IICGgilGetIl(next);
    }
    if ((*next&ILM_MASK) != IL_ENDLIST)
    {
        IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
        return;
    }
    QryArgvEle((IL *) NULL);
    IICGosdStructEnd();

    /*
    ** Read in the 2-valued list of value and type for the list of the QRY_SPEC.
    */
    next = IICGgilGetIl(next);
    qrySpecs = 0;
    qryClause = 0;
    qrySpecIndices[0] = 0;
/*  11-nov-94   lawst01  48229                                          */
    qry_spec_qgensuff = 1;
    IICGostStructBeg(ERx("static QRY_SPEC"), ERx("IIqg_spec"), qspecs, CG_INIT);
    do
    {
        do
        {
            QrySpecEle(next);
            ++qrySpecs;
            next = IICGgilGetIl(next);
        } while ( (*next&ILM_MASK) == IL_TL2ELM );
        if ((*next&ILM_MASK) != IL_ENDLIST)
        {
            IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
            return;
        }
        QrySpecEle((IL *) NULL);
        ++qrySpecs;

        do
        {
                next = IICGgilGetIl(next);
                if ( (*next&ILM_MASK) == IL_TL2ELM || (*next&ILM_MASK) == IL_ENDLIST )
                {
                        if ( qryClause >= MAX_QCLAUSE )
                        {
                                /* Order should be:
                                **
                                **      WHERE clause
                                **      other
                                **      ORDER BY clause
                                */
                                IICGerrError( CG_ILMISSING,
                                        ERx("too many query specs"), (char*)NULL
                                );
                        }
                        qrySpecIndices[qryClause++] = qrySpecs;
                        if ( (*next&ILM_MASK) == IL_ENDLIST )
                        {
                                QrySpecEle((IL *) NULL);
                                ++qrySpecs;
                        }
                }
        } while ( (*next&ILM_MASK) == IL_ENDLIST );
    } while ( (*next&ILM_MASK) == IL_TL2ELM );
    IICGosdStructEnd();

    /*
    ** Declare the QRY and QDESC structures, with unique names.
    */
    IICGostStructBeg(ERx("QDESC"), 
                        STprintf(qdescname, ERx("IIqdesc%d"), qryIndex),
                        0, CG_NOINIT
    );
    IICGostStructBeg(ERx("QRY"), 
                        STprintf(qryname, ERx("IIqry%d"), qryIndex++),
                        0, CG_NOINIT
    );

        /* Generate local 'IImode[]' declaration and initialize it to '\0' */
        IICGocvCVar(ERx("IImode"), DB_CHR_TYPE, 2, CG_NOINIT);
        IICGovaVarAssign(ERx("IImode[0]"), ERx("'\\0'"));

    /*
    ** Initialize the query ID to empty values.
    */
    qid_nm = 0;
    qid1 = 0;
    qid2 = 0;

    /*
    ** At this point next either points to a IL_QID or to an IL_QRYEND.
    ** If it points to an IL_QRYEND, and the query is still marked
    ** as repeat, then it is an old query compiled before IL_QRYEND
    ** was supported, so we can't build a reliable query id.  In this case,
    ** simply remark the query as non repeat.
    ** If it is an IL_QID, then set up the query id.
    */
    if ((*next&ILM_MASK) == IL_QID)
    {
        qid_nm = ILgetOperand(next, 1);
        qid1 = ILgetOperand(next, 2);
        qid2 = ILgetOperand(next, 3);
        next = IICGgilGetIl(next);        /* Get to IL_QRYEND */
    }

    if ((*next&ILM_MASK) != IL_QRYEND)
    {
        IICGerrError(CG_ILMISSING, ERx("qryend"), (char *) NULL);
        return;
    }

    /*
    ** Assign values to the members of the QDESC and QRY structures.
    */
    IICGosmStructMemAssign(qdescname, ERx("qg_new"), _NullPtr);
    IICGosmStructMemAssign(qdescname, ERx("qg_name"),
                                      ILCHARVAL(qid_nm, _NullPtr));
    _VOID_ CVna( QG_VERSION, buf );
    IICGosmStructMemAssign( qdescname, ERx("qg_version"), buf );
    _VOID_ CVna((i4)ILgetOperand(next, 1), buf);
    IICGosmStructMemAssign(qdescname, ERx("qg_dml"), buf);
    /*
    ** Check to see if the mode is repeat but there was not IL_QID.
    ** If so, we have an old query and we can't generate a unique
    ** query id, so mark the query unrepeated for safety.
    */
    mode = (i4) ILgetOperand(next, 2);
    if ((mode & QM_REPEAT) && qid_nm == 0)
        mode &= (~QM_REPEAT);
    _VOID_ CVna(mode, buf);
    IICGosmStructMemAssign(qdescname, ERx("qg_mode"), buf);

    IICGosmStructMemAssign(qdescname, ERx("qg_num[0]"), ILINTVAL(qid1, _Zero));
    IICGosmStructMemAssign(qdescname, ERx("qg_num[1]"), ILINTVAL(qid2, _Zero));

    IICGosmStructMemAssign(qdescname, ERx("qg_argv"), ERx("IIqg_argv"));
    IICGosmStructMemAssign(qdescname, ERx("qg_tlist"), ERx("IIqg_spec"));
    if ( qrySpecIndices[0] != 0 )
    { /* special query specification sections */
        static const char    _SpecRef[]      = ERx("&IIqg_spec[%d]");

        IICGosmStructMemAssign( qdescname, ERx("qg_where"),
                        STprintf(buf, _SpecRef, qrySpecIndices[0])
        );
        IICGosmStructMemAssign( qdescname, ERx("qg_from"),
                        STprintf(buf, _SpecRef, qrySpecIndices[1])
        );
        /* ORDER BY clause is optional */
        IICGosmStructMemAssign( qdescname, ERx("qg_order"),
                                ( qryClause >= MAX_QCLAUSE )
                                    ? STprintf(buf, _SpecRef, qrySpecIndices[2])
                                    : ERx("NULL")
        );
    }
    else
    { /* initialize query descriptor */
        IICGosmStructMemAssign( qdescname, ERx("qg_from"), ERx("NULL") );
        IICGosmStructMemAssign( qdescname, ERx("qg_where"), ERx("NULL") );
        IICGosmStructMemAssign( qdescname, ERx("qg_order"), ERx("NULL") );
    }
    IICGosmStructMemAssign(qdescname, ERx("qg_child"), ERx("NULL"));
    IICGosmStructMemAssign(qdescname, ERx("qg_internal"), ERx("NULL"));
    _VOID_ CVna(QG_BCNT, buf);
    IICGosmStructMemAssign(qdescname, ERx("qg_bcnt"), buf);
    IICGosmStructMemAssign(qdescname, 
                        STprintf(buf, ERx("qg_base[%d]"), ABCG_ORIGBASE),
                        ERx("IIdbdvs")
        );
    IICGosmStructMemAssign(qdescname, 
                        STprintf(buf, ERx("qg_base[%d]"), ABCG_CALLEEBASE),
                        ERx("NULL")
        );

    IICGosmStructMemAssign(qryname, ERx("qr_zero"), _Zero);
    _VOID_ CVna((i4)QR_VERSION, buf);
    IICGosmStructMemAssign(qryname, ERx("qr_version"), buf);
    IICGosmStructMemAssign(qryname, ERx("qr_argv"), ERx("IIqg_argv"));
        IICGosmStructMemAssign(qryname, ERx("qr_putparam"),
                        ILCHARVAL(ILgetOperand(next, 3), _NullPtr)
        );
        if ( ILgetOperand(stmt, 3) <= 0 )
        { /* table field or form . . . */
                /* Note:  Form must be empty string to distinguish
                ** array references below.
                */
                IICGosmStructMemAssign( qryname, ERx("qr_form"),
                                ILCHARVAL(ILgetOperand(stmt, 2), _EmptyStr)
                );
                IICGosmStructMemAssign( qryname, ERx("qr_table"),
                                ILCHARVAL(ILgetOperand(stmt, 3), _EmptyStr)
                );
        }
        else
        { /* array reference . . . */
                IICGosmStructMemAssign( qryname, ERx("qr_form"), _NullPtr );
                IICGosmStructMemAssign( qryname, ERx("qr_table"),
                        STprintf( buf, ERx("(char *)%s"),
                                IICGgadGetAddr(ILgetOperand(stmt, 3))
                        )
                );
        }
    IICGosmStructMemAssign( qryname, ERx("qr_child"), ERx("(QRY *) NULL") );
    IICGosmStructMemAssign( qryname, ERx("qr_qdesc"),
                        STprintf(buf, ERx("&%s"), qdescname)
    );

    /*
    ** Set the db_data element of the DB_DATA_VALUE for the query
    ** to point to the QRY structure just built.
    */
    IICGoaeArrEleAssign(ERx("IIdbdvs"), ERx("db_data"),
                        (i4) (ILgetOperand(stmt, 1) - 1), 
                        STprintf(buf, ERx("_PTR_ &%s"), qryname)
        );

    return;
}

/*{
** Name:        IICGqrsQrySingleGen() - A Singleton Query or Query Loop.
**
** Description:
**      Generates code for a query that does not have an attached submenu.
**      (Either a singleton query or the intial fetch for a query loop.)
**
** IL statements:
**      QRYSINGLE VALUE SID
** or
**      QRYROWGET VALUE SID
**
**              In the QRYSINGLE statement, the VALUE is the temporary QRY
**              structure which holds the query.
**              The SID indicates the end of the query loop (NULL for the
**              singleton query.)
**
**              The QRYSINGLE statement with a NULL SID (a singleton)
**              immediately breaks out of the query if there were no errors.
**
** Code generated:
**
**#if (stmt == IL_QRYROWGET)
**      ((QRY *) IIdbdvs[<offset>].db_data)->qr_version |= QR_0_ROWS_OK;
**#else / * stmt == IL_QRYSINGLE * /
**      ((QRY *) IIdbdvs[<offset>].db_data)->qr_version &= ~QR_0_ROWS_OK;
**#endif
**
**      if (SID == 0)
**
**      if (IIARgn1RtsgenV1(((QRY *) IIdbdvs[<offset>].db_data), QI_START) != 0)
**      {
**              IIARgn1RtsgenV1(((QRY *) IIdbdvs[<offset>].db_data), QI_BREAK);
**      }
**
**      else / * SID != 0 * /
**
**      if (IIARgn1RtsgenV1(((QRY *) IIdbdvs[<offset>].db_data), QI_START) == 0)
**      {
**              goto <label>;
**      }
**
** Inputs:
**      stmt    {IL *}  The IL statement for which to generate code.
**
** History:
**      01/09/91 (emerson)
**              Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
**      09/20/92 (emerson)
**              Added logic to handle new QRYROWGET (for bug 39582).
*/
VOID
IICGqrsQrySingleGen (stmt)
register IL     *stmt;
{
        register IL     *next;
        char    buf[CGSHORTBUF];
        IL      *goto_stmt;
        IL      sid = ILgetOperand(stmt, 2);
        char    *qryname = IICGgvlGetVal(ILgetOperand(stmt, 1), DB_QUE_TYPE);

        /*
        ** Emit code to set or clear the QR_0_ROWS_OK bit of the QRY
        ** (depending on whether this is a QRYROWGET or a QRYSINGLE).
        */
        IICGosbStmtBeg();
        IICGoprPrint(qryname);
        if ((*stmt&ILM_MASK) == IL_QRYROWGET)
        {
                IICGoprPrint(ERx("->qr_version |= "));
        }
        else
        {
                IICGoprPrint(ERx("->qr_version &= ~"));
        }
        _VOID_ CVna((i4)QR_0_ROWS_OK, buf);
        IICGoprPrint(buf);
        IICGoseStmtEnd();

        IICGoibIfBeg();
                IICGocbCallBeg(ERx("IIARgn1RtsgenV1"));
                IICGocaCallArg(qryname);
                IICGocaCallArg(ERx("QI_START"));
                IICGoceCallEnd();
        if (ILNULLSID(stmt, sid))
        { /* singleton query */
                IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
                IICGoikIfBlock();
                        IICGosbStmtBeg();
                        IICGocbCallBeg(ERx("IIARgn1RtsgenV1"));
                        IICGocaCallArg(qryname);
                        IICGocaCallArg(ERx("QI_BREAK"));
                        IICGoceCallEnd();
                        IICGoseStmtEnd();
                IICGobeBlockEnd();      /* if block */
                /*
                ** Close the block that was opened to declare structures for this query.
                */
                if (MdQryBuilt)
                { /* close child */
                        IICGobeBlockEnd();      /* child block */
                        MdQryBuilt = FALSE;
                }
                IICGobeBlockEnd();      /* query block */
        }
        else
        { /* query loop */
                goto_stmt = IICGesEvalSid(stmt, sid);
                IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
                IICGoikIfBlock();
                        /* Goto END SID on error */
                        IICGgtsGotoStmt(goto_stmt);
                        IICGlitLabelInsert(goto_stmt,
                                MdQryBuilt ? CGLM_MDBLKEND : CGLM_BLKEND
                        );
                        MdQryBuilt = FALSE;
                IICGobeBlockEnd();      /* if block */
        }

        /* assert:  MdQryBuilt == FALSE */
        return;
}

/*{
** Name:        IICGqrrQryRowchkGen() - Checks to see if a row was returned.
**
** Description:
**      Generates code to branch if a preceding QRYROWGET returned zero rows.
**
** IL statements:
**      QRYROWCHK VALUE SID
**
**              The VALUE is the temporary QRY structure which holds the query.
**              The SID indicates the end of the query.
**
** Code generated:
**
**      if ((((QRY*)IIdbdvs[<offset>].db_data)->qr_version&QR_ROWS_FOUND) == 0)
**              goto <label>;
**      }
**
** Inputs:
**      stmt    {IL *}  The IL statement for which to generate code.
**
** History:
**      09/20/92 (emerson)
**              Created (for bug 39582).
*/
VOID
IICGqrrQryRowchkGen (stmt)
register IL     *stmt;
{
        char    buf[CGSHORTBUF];
        IL      *goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 2));
        char    *qryname = IICGgvlGetVal(ILgetOperand(stmt, 1), DB_QUE_TYPE);

        IICGoibIfBeg();
                IICGoprPrint(ERx("("));
                IICGoprPrint(qryname);
                IICGoprPrint(ERx("->qr_version & "));
                _VOID_ CVna((i4)QR_ROWS_FOUND, buf);
                IICGoprPrint(buf);
                IICGoprPrint(ERx(")"));
        IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
        IICGoikIfBlock();
                /* Goto END SID on no rows returned */
                IICGgtsGotoStmt(goto_stmt);
                IICGlitLabelInsert(goto_stmt,
                        MdQryBuilt ? CGLM_MDBLKEND : CGLM_BLKEND
                );
                MdQryBuilt = FALSE;
        IICGobeBlockEnd();      /* if block */

        return;
}

/*{
** Name:        IICGqrgQrygenGen() -    Begin an attached query
**
** Description:
**      Generates code to get the first tuple for a query with an attached
**      submenu or a query loop.  At runtime, a message will be output to the
**      user if no rows were retrieved.
**
** IL statements:
**      QRYGEN VALUE INT SID
**
**              The VALUE is the temp QRY structure that holds the query.
**              The INT is TRUE if the query should display a message when
**              no rows are retrieved.
**              The SID indicates the end of the entire query (including
**              its attached submenu).
**
** Code generated:
**
**      ((QRY *) IIdbdvs[<offset>].db_data)->qr_version &= ~QR_0_ROWS_OK;
**
**      if (IIiqset(1,0,(char *)0) != 0)
**      {
**              IIiqfsio((short *)0,1,32,1,IImode,21,(char *)0,0);
**      }
**
**      if (IIARgn1RtsgenV1(((QRY *)IIdbdvs[<offset>].db_data), -QI_START) == 0)
**      {
**#if INT
**              IImessage("No rows retrieved");
**              IIsleep(3);
**#endif
**              goto <label>;
**      }
**
** Inputs:
**      stmt    {IL *}  The IL statement for which to generate code.
**
** History:
**      87/07  agh  Written.
**      90/10  jhw  Moved local 'IImode[]' definition to 'IICGqryBuildQryGen()'
**              and moved save of forms mode before query execution, and also,
**              use -QI_START for query mode for bug #31929.  Also, no longer
**              set form mode here since 'IIARgn1RtsgenV1()' will do it; #34239.
**      01/09/91 (emerson)
**              Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
**      09/20/92 (emerson)
**              Added logic to emit code to clear the QR_0_ROWS_OK bit
**              of the QRY (for bug 39582).
*/
VOID
IICGqrgQrygenGen (stmt)
register IL     *stmt;
{
        char    buf[CGSHORTBUF];
        IL      *goto_stmt;
        bool    msg = (bool)ILgetOperand(stmt, 2);
        char    *qryname = IICGgvlGetVal(ILgetOperand(stmt, 1), DB_QUE_TYPE);

        /*
        ** Emit code to clear the QR_0_ROWS_OK bit of the QRY.
        */
        IICGosbStmtBeg();
        IICGoprPrint(qryname);
        IICGoprPrint(ERx("->qr_version &= ~"));
        _VOID_ CVna((i4)QR_0_ROWS_OK, buf);
        IICGoprPrint(buf);
        IICGoseStmtEnd();

        /*
        ** Note:  When in a sub-menu, forms mode must be "update".
        **
        ** Output code to inquire forms mode ('IIARgn1RtsgenV1()' will change
        ** the form mode after the first row is retrieved but before it is
        ** displayed.)
        **
        **      if (IIiqset(1,0,(char *)0) != 0)
        **      {
        **              IIiqfsio((short *)0,1,32,1,IImode,21,(char *)0,0);
        **      }
        */
        IICGoibIfBeg();
                IICGocbCallBeg(ERx("IIiqset"));
                IICGocaCallArg(_One);
                IICGocaCallArg(_Zero);
                IICGocaCallArg(_NullPtr);
                IICGoceCallEnd();
        IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
        IICGoikIfBlock();
                /* Inquire forms mode */
                IICGosbStmtBeg();
                IICGocbCallBeg(ERx("IIiqfsio"));
                IICGocaCallArg(_ShrtNull);
                IICGocaCallArg(_One);
                IICGocaCallArg(ERx("32"));
                IICGocaCallArg(_One);
                IICGocaCallArg(ERx("IImode"));
                IICGocaCallArg(ERx("21"));
                IICGocaCallArg(_NullPtr);
                IICGocaCallArg(_Zero);
                IICGoceCallEnd();
                IICGoseStmtEnd();
        IICGobeBlockEnd();

        IICGoibIfBeg();
                IICGocbCallBeg(ERx("IIARgn1RtsgenV1"));
                IICGocaCallArg(qryname);
                IICGocaCallArg(ERx("-QI_START"));
                IICGoceCallEnd();
        IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);

        IICGoikIfBlock();
                if (msg)
                {
                        IICGmessage(S_IT001C_No_rows_retrieved);
                }
                goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 3));
                IICGgtsGotoStmt(goto_stmt);
                IICGlitLabelInsert(goto_stmt,
                                MdQryBuilt ? CGLM_MDBLKEND : CGLM_BLKEND
                );
                MdQryBuilt = FALSE;
        IICGobeBlockEnd();

        return;
}

/*{
** Name:        IICGqrnQryNextGen() -   Get the next tuple for a query
**
** Description:
**      Generates code to get the next tuple for a query (usually within an
**      attached submenu or query loop.)
**
** IL statements:
**      QRYNEXT VALUE INT SID
**
**              The VALUE is the temporary QRY structure which holds the query.
**              This VALUE is NULL if the query has been passed in from another
**              frame (as opposed to fully contained within the current frame.)
**              The INT specifies whether a message is to be output when no more
**              rows are retrieved.
**              The SID indicates where to branch on success (if the
**              ILM_BRANCH_ON_SUCCESS modifier bit is set) or on failure.
**              For success, the SID indicates the top of the SELECT loop.
**              For failure, the SID indicates the end of the entire query
**              (including its attached submenu).
**
** Inputs:
**      stmt    {IL *}  The IL statement for which to generate code.
**
** History:
**      01/09/91 (emerson)
**              Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
**      09/20/92 (emerson)
**              Added logic to emit code to clear QR_0_ROWS_OK before calling
**              IIARgn1RtsgenV1 (for bug 39582).  Also support new opcode
**              modifier ILM_BRANCH_ON_SUCCESS (for bug 46761).
*/
VOID
IICGqrnQryNextGen (stmt)
register IL     *stmt;
{
        char    buf[CGSHORTBUF];
        bool    msg = (bool)ILgetOperand(stmt, 2);

        if (ILNULLVAL(ILgetOperand(stmt, 1)))
        { /* query passed as parameter */
                IICGoibIfBeg();
                IICGocbCallBeg(ERx("abrtsnext"));
                IICGocaCallArg(ERx("rtsfparam"));
                IICGoceCallEnd();
                IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
                IICGoikIfBlock();
                        IICGmessage(S_IT001D_No_query_currently_ac);
                IICGobeBlockEnd();
                IICGoeiElseIf();
                        IICGocbCallBeg(ERx("abrtsgen"));
                        IICGocaCallArg(ERx("rtsfparam->pr_oldprm->pr_qry"));
                        IICGocaCallArg(ERx("QI_NMASTER"));
                        IICGoceCallEnd();
                IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
                IICGoikIfBlock();
                        if (msg)
                        {
                                IICGmessage(S_IT001E_No_more_rows);
                        }
                IICGobeBlockEnd();
        }
        else
        {
                IL      *goto_stmt;
                char    *qryname = IICGgvlGetVal( ILgetOperand(stmt, 1),
                                                  DB_QUE_TYPE );
                /*
                ** Emit code to clear the QR_0_ROWS_OK bit of the QRY.
                */
                IICGosbStmtBeg();
                IICGoprPrint(qryname);
                IICGoprPrint(ERx("->qr_version &= ~"));
                _VOID_ CVna((i4)QR_0_ROWS_OK, buf);
                IICGoprPrint(buf);
                IICGoseStmtEnd();

                goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 3));
                IICGoibIfBeg();
                IICGocbCallBeg(ERx("IIARgn1RtsgenV1"));
                IICGocaCallArg(qryname);
                IICGocaCallArg(ERx("QI_NMASTER"));
                IICGoceCallEnd();
                IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
                IICGoikIfBlock();
                        if (msg)
                        {
                                IICGmessage(S_IT001E_No_more_rows);
                        }
                if (*stmt & ILM_BRANCH_ON_SUCCESS)
                {
                        IICGobeBlockEnd();
                        IICGoebElseBlock();
                }
                        IICGgtsGotoStmt(goto_stmt);
                        IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
                IICGobeBlockEnd();
        }
        return;
}

/*{
** Name:        IICGqrbQryBrkGen() -    Close a query
**
** Description:
**      Breaks out of a query.
**
** IL statements:
**      QRYBRK VALUE
**
**              The VALUE is the temp QRY structure which holds the query.
**
** Inputs:
**      stmt    {IL *}  The IL statement for which to generate code.
*/
VOID
IICGqrbQryBrkGen (stmt)
register IL     *stmt;
{
        if (ILNULLVAL(ILgetOperand(stmt, 1)))
        { /* query passed as a parameter */
                IICGoibIfBeg();
                {
                        IICGoicIfCond(ERx("rtsfparam"), CGRO_NE, ERx("NULL"));
                        IICGoikIfBlock();

                        IICGosbStmtBeg();
                        IICGocbCallBeg(ERx("abrtsgen"));
                        IICGocaCallArg(ERx("rtsfparam->pr_oldprm->pr_qry"));
                        IICGocaCallArg(ERx("QI_BREAK"));
                        IICGoceCallEnd();
                        IICGoseStmtEnd();
                }
                IICGobeBlockEnd();
        }
        else
        {
                IICGosbStmtBeg();
                IICGocbCallBeg(ERx("IIARgn1RtsgenV1"));
                IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_QUE_TYPE));
                IICGocaCallArg(ERx("QI_BREAK"));
                IICGoceCallEnd();
                IICGoseStmtEnd();
        }
        return;
}

/*{
** Name:    IICGqrfQryFreeGen() -       Free up space used for a query
**
** Description:
**      A no-op for the code generator, since the memory used for
**      a query is gotten statically (as an automatic variable
**      declared within a C block) rather than dynamically.
**
** IL statements:
**      QRYFREE VALUE
**
**              The VALUE is the temp QRY structure which holds the query.
**
** Inputs:
**      stmt    The IL statement for which to generate code.
**
*/
VOID
IICGqrfQryFreeGen(stmt)
register IL     *stmt;
{
        return;
}

/*{
** Name:    IICGqrcQryChildGen() -      Connect master-detail queries
**
** Description:
**      Generates code to tie together master-detail queries.
**      The 2 QRY structures (each pointing to a QDESC structure) have
**      already been built by calls on IICGqryBuildQryGen().
**      This routine makes the 'child' elements of the master QRY and
**      QDESC point to the detail QRY and QDESC respectively.
**
** IL statements:
**      QRYCHILD VALUE VALUE
**
**              The first VALUE is the master query's QRY structure,
**              the second the detail's QRY.
**
** Inputs:
**      stmt    The IL statement for which to generate code.
*/
VOID
IICGqrcQryChildGen(stmt)
register IL     *stmt;
{
    char        parent[CGSHORTBUF];
    char        child[CGSHORTBUF];
    char        lhs[CGSHORTBUF];
    char        rhs[CGSHORTBUF];

    STcopy(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_QUE_TYPE), parent);
    STcopy(IICGgvlGetVal(ILgetOperand(stmt, 2), DB_QUE_TYPE), child);
    IICGoibIfBeg();
    {
        char    buf[CGSHORTBUF];

        IICGoicIfCond(parent, CGRO_NE, ERx("NULL"));
        IICGoikIfBlock();

        IICGovaVarAssign(STprintf(lhs, ERx("(%s)->qr_child"), parent), child);
        IICGovaVarAssign(STprintf(lhs, ERx("(%s)->qr_qdesc->qg_child"), parent),
                        STprintf(rhs, ERx("(%s)->qr_qdesc"), child)
        );
    }
    IICGobeBlockEnd();
    /*
    ** Set flag to indicate master-detail query.
    */
    MdQryBuilt = TRUE;
    return;
}

/*{
** Name:    IICGqmdChkMDQry() - Check for Master-Detail Query.
**
** Description:
**      Check if a master-detail query was generated by looking at the
**      'MdQryBuilt' flag.  Return the value of this flag and clear it.
**
** Called By:
**      'IICGcalCallfGen()'
**
** Side Effects:
**      Clears 'MDqryBuilt'.
**
** History:
**      07/87 (jhw) -- Written.
*/
bool
IICGqmdChkMDQry ()
{
    bool    mdqry;

    mdqry = MdQryBuilt;
    MdQryBuilt = FALSE;

    return mdqry;
}

/*{
** Name:    IICGoqfQualfldEle() -       Generate element of an ABRT_QFLD array
**
** Description:
**      Generates an element of an array of ABRT_QFLDs.
**
** Inputs:
**      stmt    A TL2ELM statement with information about an element
**              of a qualification function, from which can be built an
**              ABRT_QFLD.
**              The first operand is the name of a field.
**              The second operand is the left-hand side (database expression)
**              of the qualification function element.
**              If the 'stmt' argument is NULL, then this routine generates
**              a final element of the array with NULL members.
**
*/
VOID
IICGoqfQualfldEle(stmt)
register IL     *stmt;
{
    IICGosbStmtBeg();
    if (stmt == NULL)
    {
        IICGoprPrint(ERx("NULL, NULL"));
        IICGoeeEndEle(CG_LAST);
    }
    else
    {
        IICGoprPrint(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
        IICGoprPrint(ERx(", "));
        IICGoprPrint(IICGgvlGetVal(ILgetOperand(stmt, 2), DB_CHR_TYPE));
        IICGoeeEndEle(CG_NOTLAST);
    }
    return;
}

/*{
** Name:    IICGoqlQualInit() - Generate initialization of an ABRT_QUAL
**
** Description:
**      Generates an initialization of an ABRT_QUAL.
**
** Inputs:
**      pred_stmt       The QRYPRED statement for this ABRT_QUAL.
**                      First operand is the type of ABRT_QUAL; second operand
**                      is the number of elements in the array pointed to
**                      by qu_elms.
**      suffix          Suffix of the name of the ABRT_QFLD array.
**
*/
IICGoqlQualInit(pred_stmt, suffix)
register IL     *pred_stmt;
i4      suffix;
{
        char    buf[CGSHORTBUF];

        IICGosbStmtBeg();
        IICGoprPrint(STprintf(buf, ERx("%d, %s, %d, IIqfld%d"),
                        (i4) ILgetOperand(pred_stmt, 1), _FormVar,
                        (i4) ILgetOperand(pred_stmt, 2), suffix)
        );
        IICGoeeEndEle(CG_LAST);
        return;
}

/*{
** Name:    QsgenInit() -       Generate an initialization of a QSGEN
**
** Description:
**      Generates an initialization of a QSGEN.
**
** Inputs:
**      suffix          Suffix of the name of the QUAL structure.
**
*/
static VOID
QsgenInit(suffix)
i4      suffix;
{
    char        buf[CGSHORTBUF];

    IICGosbStmtBeg();
    IICGoprPrint(STprintf(buf,ERx("_PTR_ &IIqual%d, IIARrpgRtsPredGen"),suffix));
    IICGoeeEndEle(CG_LAST);
    return;
}

/*{
** Name:   QryArgvEle() -       Generate an element of a QRY_ARGV array
**
** Description:
**      Generates an element of an array of QRY_ARGVs.
**
** Inputs:
**      stmt    A TL2ELM statement with information about the QRY_ARGV.
**              The first operand is the name of a field, the second
**              the index of the corresponding DB_DATA_VALUE.
**              If this argument is NULL, then this routine generates
**              a final element of the array with NULL members.
*/
static VOID
QryArgvEle(stmt)
register IL     *stmt;
{
    char        buf[CGSHORTBUF];

    IICGosbStmtBeg();
    if (stmt == NULL)
    {
        IICGoprPrint(ERx("{(DB_DATA_VALUE *) NULL, 0, 0, (char *) NULL}"));
        IICGoeeEndEle(CG_LAST);
    }
    else
    {
        /*
        ** The value for these QRY_ARGVs is the index
        ** of the DB_DATA_VALUE in the qg_base array.
        */
        IICGoprPrint(STprintf(buf, ERx("{(DB_DATA_VALUE *) NULL, %d, %d, %s}"),
                        (ILgetOperand(stmt, 2) - 1), ABCG_ORIGBASE,
                        IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE)
                    )
        );
        IICGoeeEndEle(CG_NOTLAST);
    }
    return;
}

/*{
** Name:    QrySpecEle() -      Generate an element of a QRY_SPEC array
**
** Description:
**      Generates an element of an array of QRY_SPECs.
**
** Inputs:
**      stmt    A TL2ELM statement with information about the QRY_SPEC.
**              The first operand is the value, the second the type
**              of QRY_SPEC element.
**              If this argument is NULL, then this routine generates
**              a closing element of the array.
**
*/
/*      11-nov-94 (lawst01)  remove following and replace above         **
static i4       suffix = 1;      * suffix for QUAL struct names * 
**      11-nov-94 (lawst01)  remove following and replace above         */

static VOID
QrySpecEle (stmt)
register IL     *stmt;
{
    char        buf[CGSHORTBUF];

    IICGosbStmtBeg();
    if (stmt == NULL)
    {
        IICGoprPrint(STprintf(buf, ERx("{_PTR_ NULL, 0, 0, %d},"), QS_END));
        IICGoeeEndEle(CG_LAST);
/*      suffix = 1;             11-nov-94   lawst01  48229              */
    }
    else
    {
        IL      val;
        i4      type;

        val = ILgetOperand(stmt, 1);
        type = (i4) ILgetOperand(stmt, 2);
        switch (type)
        {
          case QS_TEXT:
            _VOID_ STprintf(buf, ERx("{_PTR_ %s, 0, 0, %d}"),
                                IICGgvlGetVal(val, DB_CHR_TYPE), type
            );
            break;

          case QS_QRYVAL:
          case QS_VALUE:
            /*
            ** The value for these QRY_SPECs is the index of the
            ** DB_DATA_VALUE in the qg_base[ABCG_ORIGBASE] array.
            */
            _VOID_ STprintf(buf, ERx("{_PTR_ NULL, %d, %d, %d}"),
                        (val - 1), ABCG_ORIGBASE, (type | QS_BASE)
            );
            break;

          case QS_VALGEN:
            _VOID_ STprintf(buf, ERx("{_PTR_ &IIqgen%d, 0, 0, %d}"),
                        qry_spec_qgensuff++,type        /* 11-nov-94    */
/* 11-nov-94            suffix++, type                  48229           */
            );
            break;

          case QS_END:
          default:
            return;             /* not an element */
        }
        IICGoprPrint(buf);
        IICGoeeEndEle(CG_NOTLAST);
    }
    return;
}

/*{
** Name:        IICGmessage() - Generate Code for Message with Pause.
**
** Description:
**      Generates the C code for a message statement followed by a (sleep)
**      pause.
**
** Code generated:
**      IImessage("<msg>");
**      IIsleep(3);
**
** Input:
**      msg     {ER_MSGID}  The ID of the message to be printed.
**
** History:
**      05/90 (jhw) -- Abstracted.
*/
VOID
IICGmessage ( msg )
ER_MSGID        msg;
{
        char    buf[CGSHORTBUF];

        IICGosbStmtBeg();
        IICGocbCallBeg(ERx("IImessage"));
        IICGocaCallArg(STprintf(buf, "\"%s\"", ERget(msg)));
        IICGoceCallEnd();
        IICGoseStmtEnd();
        IICGosbStmtBeg();
        IICGocbCallBeg(ERx("IIsleep"));
        IICGocaCallArg(ERx("3"));
        IICGoceCallEnd();
        IICGoseStmtEnd();
}
