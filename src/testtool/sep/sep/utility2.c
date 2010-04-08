/*
*/
#include <compat.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <ex.h>
#include <gv.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <si.h>
#include <st.h>
#include <tm.h>


#define utility2_c

#include <sepdefs.h>

typedef struct expression
    {
        char                *exp_str;
        i4                   exp_nat;
        i4                   exp_result;
        STATUS               exp_error;
        f8                   exp_f8;
        char                *exp_operator;
	i4                  exp_oper_code;
        struct expression   *exp_operand1;
        struct expression   *exp_operand2;

    } expression_node;

#include <fndefs.h>

/*
** History:
**	05-jul-1992 (donj)
**	    Added Modularized SEP version code here to be used by all images.
**	    Added expression evaluator which can be used by all images. Added
**	    other SEP CM wrapper functions.
**	05-jul-1992 (donj)
**	    Fix some unix casting problems.
**	06-jul-1992 (donj)
**	    Patch SEP_Exp_Eval() to handle ".if !sepparam_db" meaning 'if
**	    sepparam_db is undefined.
**	10-jul-1992 (donj)
**	    Using explicit ME memory block tags.
**	14-jul-1992 (donj)
**	    Added tracing to SEP_Eval_If(). Found difference between VMS and
**	    UNIX in NMgtAt(). In VMS NMgtAt() returned a NULL ptr for the
**	    result string, in UNIX (su4_u42), NMgtAt() returned an address to
**	    an EOS char. Had to replace "if (cptr1 == NULL)" with
**	    "if (cptr1 == NULL || *cptr1 == EOS)".
**	26-jul-1992 (donj)
**	    Switch MEreqmem() with SEP_MEalloc().
**	30-nov-1992 (donj)
**	    Stripped SEP_CM functions off to their own file. Added "MPE" to
**	    the keywords that the if expression analyzer can handle.
**	04-feb-1993 (donj)
**	    Included lo.h because sepdefs.h now references LOCATION type.
**	    Also added keyword, ALPHA, to the expression evaluator,
**	    SEP_exp_eval(). Changed MPE keyword in SEP_exp_eval() to use
**	    the preprocessor definition, MPE. That way sepdefs.h is the
**	    only place where MPE is evaluated and set.
**	24-Mar-1993 (donj)
**	    Alter SEP_cmd_line() to do both int and float, if appropriate.
**	30-mar-1993 (donj)
**	    Protect SEP_cmd_line() in errors of CVan() and CVaf().
**	31-mar-1993 (donj)
**	    for loop missing ';'.
**      31-mar-1993 (donj)
**          Use a new field in OPTION_LIST structure, arg_required. Used
**          for telling SEP_Cmd_Line() to look ahead for a missing argument.
**	23-apr-1993 (donj)
**	    Fix something that UNIX doesn't like.
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**      20-may-1993 (donj)
**          Fix some embedded comments. They-re illegal in ansi C.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h. Took out unused
**          arg from SEP_cmd_line.
**      14-aug-1993 (donj)
**          changed SEP_cmd_line() from using an array of structs to using a
**	    linked list of structs. Added SEP_alloc_Opt() to create linked list.
**	16-aug-1993 (donj)
**	    Took out calls to MEfree() from SEP_cmd_line(); they were causing
**	    memory writes into unallocated memory. Let SEP_cmd_line() use up a
**	    bit of memory until we can clean it up. Change SEP_Vers_Init() not
**	    to need a hidden globalref, sep_version or sep_platform.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	18-oct-1993 (donj)
**	    Break three functions out of SEP_exp_eval(), the SEPEVAL*()
**	    functions. Also, when processing a quoted string, remove the
**	    quotes in SEPEVAL_alnum().
**	26-oct-1993 (donj)
**	    Broke up SEP_TRACE functions to make them less complex.
**	27-oct-1993 (donj)
**	    Fix a minor casting incompatability with Version.
**	 3-nov-1993 (donj)
**	    Simplied SEP_cmd_line function.
**	 3-dec-1993 (donj)
**	    Added DIFFS and DIFFERENCES variable that allows conditional
**	    expressions to reference if a diff has occurred in the test.
**	    Such as: .if DIFFS > 0
**	14-dec-1993 (donj)
**	    Added STATUS variable that allows conditional expressions to ref-
**	    erence the return status of a spawned command in subsequent
**	    SEP test commands.
**	14-dec-1993 (donj)
**          Add functions to do @VALUE() processing (IS_ValueNotation() and
**          Trans_SEPvar()).
**	20-dec-1993 modified SEPEVAL_alnum() to translate un'dbl quoted misc
**	    strings as environmental vars.
**	30-dec-1993 (donj)
**	    Fix a core dump problem in Trans_SEPvar() when the env_var
**	    specified to be translated is undefined.
**	 4-mar-1994 (donj)
**	    Had to change exp_bool from type bool to type i4, exp_bool was
**	    exp_bool was carrying negative numbers - a no no on AIX. Changed
**	    it's name from exp_bool to exp_result. Also had to change the
**	    types of SEP_Eval_If(), SEP_exp_eval(), SEPEVAL_alnum(),
**	    SEPEVAL_opr_expr(), SEPEVAL_expr_opr_expr(),
**	    SEPEVAL_expr_plus_expr(), SEPEVAL_expr_mult_expr(),
**	    SEPEVAL_expr_div_expr(), SEPEVAL_expr_equ_expr(),
**	    SEPEVAL_expr_lt_expr() and SEPEVAL_expr_minus_expr() for the
**	    sake of clarity.
**	24-may-1994 (donj)
**	    Improve readablitiy. Use SIputrec() instead of SIfprintf() where
**	    no formatting is being done.
**      15-sep-1995 (albany)
**	    Added file pointer to SIputrec() in IS_ValueNotation() to get
**	    rid of acc-vio during tracing.
**	21-Nov-1995 (somsa01)
**	    Added NT_GENERIC to SEPEVAL_alnum() as additional operating system.
**	22-Jan-1996 (somsa01)
**	    Finished adding NT_GENERIC as additional operating system.
**      08-Nov-1996 (mosjo01 & linke01)
**          In case 3, the following statement generate internal compile error
**                     in rs4_us5 :
**                     CMcpychar(cptr3,pair_stack_r[pair_ptr++]);
**          I modify it as the following statements:
**                     CMcpychar(cptr3,pair_stack_r[pair_ptr]); 
**                     ++pair_ptr;                   
**	05-Apr-1999 (kinte01)
**	    Add VAX as a valid SEPEVAL_alnum so VAX specific canons can
**	    be generated
**      02-Aug-1999 (hweho01)
**          Extended the change dated 08-Nov-1996 to ris_u64 (AIX 64-bit).   
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	10-Oct-2001 (devjo01)
**	    Add LP64 as a predefined expresssion to allow conditional
**	    enabling of alternate canons for 64 bit pointer platforms. 
**	02-Feb-2004 (hanje04)
**	    In SEP_exp_eval, case 3 use rs4_us5 case for all platforms
**	    as under double byte (which is now the default build) the line
**	    CMcpychar(cptr3,pair_stack_r[pair_ptr++] increments pair_ptr
**	    twice.
*/

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

GLOBALREF    char         *sep_platform ;
GLOBALREF    i4            testGrade ;
GLOBALREF    i4            Status_Value ;

#define SEP_OPER_INCR       1
#define SEP_OPER_L_AND      2
#define SEP_OPER_L_OR       3
#define SEP_OPER_DECR       4
#define SEP_OPER_D_EQU      5
#define SEP_OPER_N_EQU      6
#define SEP_OPER_PLUS       7
#define SEP_OPER_B_AND      8
#define SEP_OPER_B_OR       9
#define SEP_OPER_MINUS      10
#define SEP_OPER_EQU        11
#define SEP_OPER_L_NOT      12
#define SEP_OPER_LT         13
#define SEP_OPER_GT         14
#define SEP_OPER_MULT       15
#define SEP_OPER_B_NOT      16
#define SEP_OPER_LE         17
#define SEP_OPER_GE         18
#define SEP_OPER_DIV        19
#define SEP_OPER_ATSIGN     20

#define SEP_OPERAND_INIT    -2
#define SEP_OPERAND_F8      -3
#define SEP_OPERAND_NAT     -4
#define SEP_OPERAND_STR     -5
#define SEP_OPERAND_PARAM   -6
#define SEP_OPERAND_FAIL    -7
#define SEP_OPERAND_FUNC    -8

#define EVAL_FIND            0
#define EVAL_CHECK          -1
#define EVAL_FIX            -2
#define EVAL_FINISHED       -3

#ifdef VMS
#define IS_VMS_TorF	ERx("TRUE")
#else
#define IS_VMS_TorF     ERx("FALSE")
#endif
#ifdef ALPHA
#define IS_ALPHA_TorF   ERx("TRUE")
#else
#define IS_ALPHA_TorF   ERx("FALSE")
#endif
#ifdef MPE
#define IS_MPE_TorF     ERx("TRUE")
#else
#define IS_MPE_TorF     ERx("FALSE")
#endif
#ifdef UNIX
#define IS_UNIX_TorF    ERx("TRUE")
#else
#define IS_UNIX_TorF    ERx("FALSE")
#endif
#ifdef NT_GENERIC
#define IS_NT_GENERIC_TorF    ERx("TRUE")
#else
#define IS_NT_GENERIC_TorF    ERx("FALSE")
#endif
#ifdef VAX
#define IS_VAX_TorF	ERx("TRUE")
#else
#define IS_VAX_TorF     ERx("FALSE")
#endif

char              *l_oper_pairs = ERx("({[") ;
char              *r_oper_pairs = ERx(")}]") ;

STATUS
SEP_cmd_setopt(OPTION_LIST *optptr, bool first_look, char *sub_arg, char *c)
{
    STATUS                 ret_val = OK ;
    char                  *cptr = NULL ;
    char                  *tmp2 = NULL ;
    bool                   all_digits = FALSE ;

    if (optptr->o_active != NULL)
    {
	*optptr->o_active = TRUE;
    }
    /* **************************************************** **
    ** If we had to look twice to find a single char UNIX   **
    ** style switch, (i.e. "-u1"), then point sub_arg at it **
    ** also look for and strip off quotations.              **
    ** **************************************************** */
    if (!first_look)
    {
	sub_arg = c;
	CMnext(sub_arg);
    }
    if ((sub_arg != NULL)&&(*sub_arg == EOS))
    {
	sub_arg = NULL;
    }
    if ((sub_arg != NULL)&&(CMcmpcase(sub_arg,DBL_QUO) == 0))
    {
	CMnext(sub_arg);
	tmp2 = SEP_CMlastchar(sub_arg,0);
	if (CMcmpcase(tmp2,DBL_QUO) == 0)
	{
	    *tmp2 = EOS;
	}
    }

    /* **************************************************** */

    if ((sub_arg == NULL) && optptr->arg_required)
    {
    }

    /* ******************************************** */
    /*          First do string argument            */
    /* ******************************************** */
    if (optptr->o_retstr) 
    {
	if (sub_arg)
	{
	    STcopy(sub_arg, optptr->o_retstr);
	}
	else
	{
	    *optptr->o_retstr = EOS;
	}
    }
    /* ******************************************** */
    /*            Next do integer argument          */
    /* ******************************************** */
    if (optptr->o_retnat)
    {
	*optptr->o_retnat = 0;
	if (sub_arg)
	{
	    for (all_digits = TRUE, cptr = sub_arg;
		 (*cptr)&&(all_digits);
		 all_digits = CMdigit(cptr++))
		;

	    if (all_digits)
	    {
		ret_val = CVan(sub_arg,optptr->o_retnat);
	    }
	}
    }
    /* ******************************************** */
    /*            Next do float argument            */
    /* ******************************************** */
    if (optptr->o_retfloat)
    {
	*optptr->o_retfloat = 0;
	if (sub_arg)
	{
	    ret_val = CVaf(sub_arg,'.',optptr->o_retfloat);
	}

    }
    /* ******************************************** */
    return (ret_val);
}

/* **************************************************************** **
** Look ahead and see if we have a disjointed argument, such as:    **
**    -name =frank    or    -name = frank   or   -name= frank  ...  **
** if we do, then find the sub_arg and skip the fragments by incre- **
** menting 'i'.                                                     **
** **************************************************************** */
i4
SEP_cmd_fix_disjointed( i4  i, char *c, char **sub_arg, i4  argc, char **argv,
			char *opt_prefix )
{
    char                  *nc = NULL ;

    if ((*sub_arg = STindex(c,ERx("="),0)) != NULL)
    {
	*(*sub_arg) = EOS;
	CMnext((*sub_arg));
    }

    if (((*sub_arg == NULL)||(*(*sub_arg) == EOS)) && ((i+1) < argc))
    {
	nc = STtalloc(SEP_ME_TAG_NODEL, argv[i+1]);
    
	if (CMcmpcase(nc,opt_prefix) == 0)
	{
	    if (*sub_arg != NULL)
	    {
		*sub_arg = nc;
		i++;
	    }
	    else
	    {
		MEfree(nc);
	    }
	}
	else if (STcompare(nc,ERx("=")) == 0)
	{
	    MEfree(nc);
	    if ((i+=2)<argc)
	    {
		*sub_arg = STtalloc(SEP_ME_TAG_NODEL,argv[i]);
	    }
	}
	else if (CMcmpcase(nc,ERx("=")) == 0)
	{
	    CMnext(nc);
	    *sub_arg = nc;
	    i++;
	}
	else if (*sub_arg != NULL)
	{
	    *sub_arg = nc;
	    i++;
	}
	else
	{
	    MEfree(nc);
	}
    }
    return (i);
}

STATUS
SEP_cmd_line (i4 *argc, char **argv, OPTION_LIST *header, char *opt_prefix)
{
    STATUS                 ret_val = OK;

    bool                   found_it ;
    bool                   first_look ;

    i4                     i ;
    i4                     k ;
    i4                     f ;
    i4                     slen ;

    char                  *c       = NULL ;
    char                  *tmpbuf  = NULL ;
    char                  *tmp     = NULL ;
    char                  *sub_arg = NULL ;

    OPTION_LIST           *optptr ;

    if (header == NULL)
    {
	return (ret_val);
    }

    for (i = 1, k = 0; i < *argc; i++)
    {
	if (tmpbuf)
	{
	    MEfree(tmpbuf);
	}

	c  = tmpbuf  = STtalloc(SEP_ME_TAG_NODEL, argv[i]);

	/* **************************************************************** **
	** If argument isn't a switch do our argument condensing flip and   **
	** continue to next argument.                                       **
	** **************************************************************** */
	if (CMcmpcase(c,opt_prefix))
	{
	    if (++k != i)
	    {
		tmp = argv[k];
		argv[k] = argv[i];
		argv[i] = tmp;
	    }
	    continue;
	}
	/* **************************************************************** **
	** Look ahead and see if we have a disjointed argument, such as:    **
	**    -name =frank    or    -name = frank   or   -name= frank  ...  **
	** if we do, then find the sub_arg and skip the fragments by incre- **
	** menting 'i'.                                                     **
	** **************************************************************** */

	i = SEP_cmd_fix_disjointed(i, CMnext(c), &sub_arg, *argc, argv,
				   opt_prefix);

	/* **************************************************************** **
	** Now, look for the arg in our list of valid options. First look   **
	** for it VMS style ( "-[s]witch=sub_arg" ) then try it UNIX style  **
	** ( "-ssub_arg").                                                  **
	** **************************************************************** */
	found_it = FALSE;
	first_look = TRUE;
	slen = STlength(c);
	for (f = 0; ((f < 2)&&(found_it == FALSE)&&(ret_val == OK)); f++)
	{
	    optptr = header;
	    do
	    {
		if (first_look)
		{
		    found_it = (STbcompare(c,slen,optptr->o_list,slen,TRUE)==0);
		}
		else
		{
		    found_it = (CMcmpcase(c,optptr->o_list) == 0);
		}

		if (found_it)
		{
		    ret_val = SEP_cmd_setopt(optptr, first_look, sub_arg, c);
		}

		optptr = optptr->o_next;
	    }
	    while ((optptr!=header)&&(found_it == FALSE)&&(ret_val == OK));

	    first_look = FALSE;
	}

	if (found_it == FALSE)
	{
	    if (++k != i)
	    {
		tmp = argv[k];
		argv[k] = argv[i];
		argv[i] = tmp;
	    }
	}
    }
    *argc = ++k;
    for (k; k < i; k++)
    {
	argv[k][0] = EOS;
    }

    return (ret_val);
}


i4
SEP_Eval_If(char *Tokenstr,STATUS *err_code,char *err_msg)
{
    static expression_node  exp_head ;

    i4             token_ctr ;
    i4             i ;

    char          *cptr1 = NULL ;
    char          *cptr2 = NULL ;
    char          *token_array [WORD_ARRAY_SIZE] ;

    *err_code = FAIL;

    if ((Tokenstr == NULL)||(*Tokenstr == EOS))
    {
	*err_code = OK;
	return (TRUE);  /* Null expression always true. */
    }
    token_ctr = SEP_CMgetwords(SEP_ME_TAG_EVAL, Tokenstr, WORD_ARRAY_SIZE,
			       token_array);

    if (token_ctr == 0)
    {
	STcopy(ERx("SEP_CMgetwords token_ctr is ZERO."),err_msg);
	return (FAIL);
    }

    /*
    **  Skip "IF" if present as first token.
    */
    if (CMcmpcase((cptr1 = token_array[0]),ERx(".")) == 0)
    {
	CMnext(cptr1);
    }

    while (CMcmpcase(cptr1,ERx(" ")) == 0)
    {
	CMnext(cptr1);
    }

    if (STbcompare(cptr1,0,ERx("IF"),0,TRUE) == 0)
    {
	*token_array[0] = EOS;
    }

    /*
    **  Reassemble into Tokenstr without whitespace.
    */
    cptr2 = cptr1 = SEP_CMpackwords(SEP_ME_TAG_NODEL, token_ctr, token_array);

    (STATUS) Trans_Token( SEP_ME_TAG_NODEL, &cptr1, NULL,
                          TRANS_TOK_FILE | TRANS_TOK_VAR );
    if (cptr2 != cptr1)
	MEfree(cptr2);

    MEtfree(SEP_ME_TAG_EVAL);
    SEP_exp_init(&exp_head);
    exp_head.exp_str = cptr1;
    SEP_exp_eval(&exp_head);

    *err_code = exp_head.exp_error;
    MEtfree(SEP_ME_TAG_EVAL);
    MEfree(cptr1);

    switch (exp_head.exp_result)
    {
       case SEP_OPERAND_FAIL:
       case TRUE:
       case FALSE:
		break;
       case SEP_OPERAND_INIT:
		exp_head.exp_result = SEP_OPERAND_FAIL;
		*err_code = exp_head.exp_error = FAIL;
		break;
       case SEP_OPERAND_F8:
		exp_head.exp_result = (i4)(exp_head.exp_f8  != 0);
		break;
       case SEP_OPERAND_NAT:
		exp_head.exp_result = (i4)(exp_head.exp_nat != 0);
		break;
       case SEP_OPERAND_STR:
		i = STlength(exp_head.exp_str);
		if (STbcompare(exp_head.exp_str,i,ERx("TRUE"), 4,TRUE) == 0)
		{
		    exp_head.exp_result = TRUE;
		}
		else
		if (STbcompare(exp_head.exp_str,i,ERx("FALSE"),5,TRUE) == 0)
		{
		    exp_head.exp_result = FALSE;
		}
		else
		if (STbcompare(exp_head.exp_str,i,ERx("YES"),  3,TRUE) == 0)
		{
		    exp_head.exp_result = TRUE;
		}
		else
		if (STbcompare(exp_head.exp_str,i,ERx("NO"),   2,TRUE) == 0)
		{
		    exp_head.exp_result = FALSE;
		}
		else
		{
		    exp_head.exp_result = SEP_OPERAND_FAIL;
		    *err_code = exp_head.exp_error = FAIL;
		}

		break;
       case SEP_OPERAND_PARAM:
		CVupper(exp_head.exp_str);
		NMgtAt(exp_head.exp_str,&cptr1);
		exp_head.exp_result = (i4)(!((cptr1 == NULL)||(*cptr1 == EOS)));
		if (tracing&TRACE_SEPCM)
		{
		    SIputrec(ERx("\nIn SEP_Eval_If with SEPPARAM\n"),traceptr);
		    SIfprintf(traceptr,ERx("cptr1 = %d, bool = %d\n"),cptr1,
			      exp_head.exp_result);
		    SIputrec(ERx("cptr1 = "),traceptr);
		    SIputrec(cptr1 == NULL ? ERx("NULL") :
			     *cptr1 == EOS ? ERx("EOS")  : cptr1 ,traceptr);
		    SIputrec(ERx("\n"),traceptr);
		    SIflush(traceptr);
		}
		break;
       default:
		break;
    }

    return (exp_head.exp_result);
}

VOID
SEP_TRACE_exp_eval(expression_node *exp)
{

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("    str     = "),traceptr);
	SIputrec(exp->exp_str == NULL ? ERx("NULL") :
		 *exp->exp_str == EOS ? ERx("EOS")  : exp->exp_str,traceptr);
	SIputrec(ERx("\n"),traceptr);

	SIfprintf(traceptr,  ERx("    i4      = %d\n"),    exp->exp_nat);
	SEP_TRACE_exp_result(ERx("    result  = "),        exp->exp_result);
	SIfprintf(traceptr,  ERx("    err     = %d\n"),    exp->exp_error);
	SIfprintf(traceptr,  ERx("    f8      = %7.3f \n"),exp->exp_f8);
	SEP_TRACE_exp_oper(  ERx("    opr     = "),        exp->exp_operator,
							   exp->exp_oper_code);
	SEP_TRACE_operand(   ERx("    op1     = "),        exp->exp_operand1);
	SEP_TRACE_operand(   ERx("    op2     = "),        exp->exp_operand2);
	SIflush(traceptr);
    }
}

VOID
SEP_exp_init(expression_node *exp)
{
    exp->exp_str       = NULL;
    exp->exp_nat       = 0;
    exp->exp_result      = SEP_OPERAND_INIT;
    exp->exp_error     = FAIL;
    exp->exp_f8        = 0;
    exp->exp_operator  = NULL;
    exp->exp_oper_code = 0;
    exp->exp_operand1  = NULL;
    exp->exp_operand2  = NULL;
}


i4
SEP_exp_eval(expression_node *exp)
{
    STATUS         STSerr ;
    char           pair_stack_l [WORD_ARRAY_SIZE] [3] ;
    char           pair_stack_r [WORD_ARRAY_SIZE] [3] ;
    char           tmp_buff1    [256] ;
    char           tmp_buff2    [256] ;
    char           tmp_oper     [5] ;

    char          *cptr1 = NULL ;
    char          *lptr1 = NULL ;
    char          *cptr2 = NULL ;
    char          *cptr3 = NULL ;
    char          *cptr4 = NULL ;
    char          *cptr5 = NULL ;
    char          *ocptr = NULL ;

    bool           oper_yes ;
    bool           oper_l_yes ;
    bool           oper_r_yes ;
    bool           inside_dq ;
    bool           break_it ;

    i4             i ;
    i4             j ;
    i4             pair_ptr ;

    i4             eval_state ;


    eval_state = EVAL_FIND;
    pair_ptr   = 0;

    if ((exp->exp_str == NULL)||(*exp->exp_str == EOS))
    {
	exp->exp_nat      = 0;
	exp->exp_result     = TRUE;
	exp->exp_f8       = 0;
	exp->exp_error    = OK;
	exp->exp_operator = NULL;
	exp->exp_operand1 = NULL;
	exp->exp_operand2 = NULL;

	return(TRUE);
    }

    for (i = 0; i <   5; i++)
    {
	tmp_oper[i]  = EOS;
    }

    for (i = 0; i < 256; i++)
    {
	tmp_buff1[i] = tmp_buff2[i] = EOS;
    }

    inside_dq = FALSE;

    
    for (lptr1 = NULL, ocptr = cptr1 = exp->exp_str;
	 *cptr1 != EOS;
	 lptr1 = cptr1, CMnext(cptr1))
    {
	/* ****************************************** */
	/*         Start init for iteration.          */
	/* ****************************************** */
	oper_yes = oper_l_yes = oper_r_yes = FALSE;
	if (CMoper(cptr1))
	{
	    if (CMcmpcase(cptr1,DBL_QUO) == 0)
	    {
		inside_dq   = (bool)(inside_dq ? FALSE : TRUE);
	    }
	    else
	    {
		oper_yes = TRUE;
		if ((cptr2 = STindex(l_oper_pairs, cptr1, 3)) != NULL)
		{
		    oper_l_yes  = TRUE;
		}
		else if ((cptr2 = STindex(r_oper_pairs, cptr1, 3)) != NULL)
		{
		    oper_r_yes  = TRUE;
		}
	    }
	}

	/*          End init for iteration.           */

	if (!oper_yes||inside_dq)
	{
	    continue;
	}

	/* ****************************************** */
	/* case 1: symbol+expression                  */
	/* ****************************************** */
	if (oper_yes&&(pair_ptr == 0)&&(ocptr != cptr1)&&(!oper_r_yes))
	{
	    switch(eval_state)
	    {
	       case EVAL_CHECK:

		    break_it = FALSE;
		    if (oper_l_yes)
		    {
			if (STindex(r_oper_pairs, lptr1, 3) != NULL)
			{
			    if (SEP_cmp_oper(ERx("*"),tmp_oper) >= 0)
			    {
				continue;
			    }
			}
			break_it = TRUE;
		    }
		    else
		    {
			if (SEP_cmp_oper(cptr1,tmp_oper) >= 0)
			{
			    continue;
			}
		    }

		    if (break_it)
		    {
			break;
		    }

	       case EVAL_FIND:

		    for (cptr4 = tmp_buff1, cptr3 = ocptr; cptr3 < cptr1; )
		    {
			CMcpyinc(cptr3,cptr4);
		    }
		    *cptr4 = EOS;

		    cptr3 = cptr1;
		    cptr4 = tmp_oper;
		    if (oper_l_yes)
		    {
			CMcpychar(ERx("*"),cptr4);
			CMnext(cptr4);
		    }
		    else
		    {
			while (CMoper(cptr3)&&CMcmpcase(cptr3,DBL_QUO)&&
			       (STindex(l_oper_pairs, cptr3, 3) == NULL))
			{
			    CMcpyinc(cptr3,cptr4);
			}
		    }

		    *cptr4 = EOS;

		    for (cptr4 = tmp_buff2; *cptr3 != EOS; )
		    {
			CMcpyinc(cptr3,cptr4);
		    }

		    *cptr4 = EOS;

		    i = SEP_CMstlen(tmp_oper,0);
		    if (i > 1)
		    {
			for (; i>1; i--)
			{
			    CMnext(cptr1);
			}
		    }

		    eval_state = EVAL_CHECK;
		    continue;
	    }
	}
	/*              End of case 1.                */

	/* ****************************************** */
	/* case 2: +expression                        */
	/* ****************************************** */
	if (oper_yes&&(pair_ptr == 0)&&(ocptr == cptr1)&&
	    !(oper_l_yes||oper_r_yes))
	{
	    /* ****************************************** */
	    /* We don't care what eval_state, if ocptr is */
	    /* the same as cptr1, we couldn't be in       */
	    /* EVAL_CHECK.                                */
	    /* ****************************************** */
	    /*
	    switch(eval_state)
	    {
	       case EVAL_CHECK:

	       case EVAL_FIND:
	    */
		    *tmp_buff1 = EOS;
		    cptr3 = cptr1;
		    cptr4 = tmp_oper;
		    while (CMoper(cptr3)&&CMcmpcase(cptr3,DBL_QUO)&&
			   (STindex(l_oper_pairs,cptr3,3)==NULL))
			    CMcpyinc(cptr3,cptr4);
		    *cptr4 = EOS;

		    for (cptr4 = tmp_buff2; *cptr3 != EOS; )
			CMcpyinc(cptr3,cptr4);
		    *cptr4 = EOS;

		    if ((i = SEP_CMstlen(tmp_oper,0)) > 1)
			for (; i>1; i--)
			    CMnext(cptr1);

		    eval_state = EVAL_CHECK;
		    continue;
	    /*
	    }
	    */
	}
	/*              End of case 2.                 */

	/* ******************************************* */
	/* case 3: (expression{expression})expression  */
	/*         ^                                   */
	/* ******************************************* */
	/* case 4: (expression{expression})expression  */
	/*                    ^                        */
	/* ******************************************* */
	if (oper_l_yes)                 /* case 3 & 4. */
	{
	    /* ****************************************** */
	    /* So far, we don't care what eval_state.     */
	    /* ****************************************** */
	    /*
	    switch(eval_state)
	    {
	       case EVAL_CHECK:

	       case EVAL_FIND:
	    */
		    CMcpychar(cptr2,pair_stack_l[pair_ptr]);
		    i = (i4)(l_oper_pairs - cptr2);
		    cptr3 = r_oper_pairs;
		    for (j = 0; j<i; CMnext(cptr3))
			;
		    CMcpychar(cptr3,pair_stack_r[pair_ptr]);
                    ++pair_ptr;
                      
		    continue;
	    /*
	    }
	    */
	}
	/*             End of case 3 & 4.              */

	/* ******************************************* */
	/* case 5: (expression{expression})expression  */
	/*                               ^             */
	/* ******************************************* */
	/* case 6: (expression{expression})expression  */
	/*                                ^            */
	/* ******************************************* */
	/* case 7: (expression{expression})+expression */
	/*                                ^            */
	/* ******************************************* */
	/* case 8: (expression)(expression)            */
	/*                    ^                        */
	/* ******************************************* */
	/* case 9: (expression)                        */
	/*                    ^                        */
	/* ******************************************* */
	if (oper_yes&&oper_r_yes&&(pair_ptr&&(ocptr != cptr1)))
	{
	    if (--pair_ptr)                 /* case 5. */
	    {
		continue;
	    }

	    cptr3 = cptr1;
	    CMnext(cptr3);

	    switch(eval_state)
	    {
	       case EVAL_CHECK:

		    if (*cptr3 == EOS)          /* case 9. */
		    {
			continue;
		    }

		    CMnext(cptr1);

		    if (!CMoper(cptr3)||        /* case 6 & 8. */
			(STindex(l_oper_pairs, cptr3, 3) != NULL))
		    {
			if (SEP_cmp_oper(ERx("*"),tmp_oper) >= 0)
			{
			    continue;
			}
		    }
		    else                        /* case 7. */
		    {
			if (SEP_cmp_oper(cptr3,tmp_oper) >= 0)
			{
			    continue;
			}
		    }

	       case EVAL_FIND:

		    if (!CMoper(cptr3)||            /* case 6 & 9. */
			(STindex(l_oper_pairs, cptr3, 3) != NULL))
		    {                               /* case 8. */
			if (*cptr3 != EOS)          /* case 6. */
			{
			    cptr4 = tmp_oper;
			    CMcpychar(ERx("*"),cptr4);
			    CMnext(cptr4);
			    *cptr4 = EOS;
			}

			cptr4 = ocptr;
			CMnext(cptr4);
			cptr5 = tmp_buff1;
			while (cptr4 < cptr1)
			    CMcpyinc(cptr4, cptr5);

			CMnext(cptr1);
			*cptr5 = EOS;

			for (cptr4 = tmp_buff2; *cptr3 != EOS; )
			    CMcpyinc(cptr3,cptr4);

			*cptr4 = EOS;
			eval_state = EVAL_CHECK;
			continue;
		    }

		    /* case 7. next char is an oper and not a l_paran. */

		    cptr4 = ocptr;
		    CMnext(cptr4);
		    cptr5 = tmp_buff1;
		    while (cptr4 < cptr1)
			CMcpyinc(cptr4, cptr5);
		    *cptr5 = EOS;
		    CMnext(cptr1);

		    cptr4 = tmp_oper;
		    while (CMoper(cptr3)&&CMcmpcase(cptr3,DBL_QUO)&&
			   (STindex(l_oper_pairs, cptr3, 3) == NULL))
			    CMcpyinc(cptr3,cptr4);
		    *cptr4 = EOS;

		    for (cptr4 = tmp_buff2; *cptr3 != EOS; )
			CMcpyinc(cptr3,cptr4);
		    *cptr4 = EOS;

		    if ((i = SEP_CMstlen(tmp_oper,0)) > 1)
			for (; i>1; i--)
			    CMnext(cptr1);

		    eval_state = EVAL_CHECK;
		    continue;
	    }
	}
	/*          End of case 5,6,7,8 & 9.           */

    }

    cptr1 = tmp_oper;
    cptr2 = tmp_buff1;
    cptr3 = tmp_buff2;

    /*
    ** Catch the special case of the ingres platform config str.
    */
    if ((CMcmpcase(cptr1,ERx(".")) == 0)&&(SEP_CMstlen(cptr1,0) == 1)&&
	(SEP_CMstlen(cptr2,0) == 3)&&(SEP_CMstlen(cptr3,0) == 3))
    {
	STpolycat(3,cptr2,cptr1,cptr3,exp->exp_str);
	cptr1 = cptr2 = cptr3 = NULL;
    }

    if ((cptr1 == NULL)||(*cptr1 == EOS))
	exp->exp_operator = NULL;
    else
    {
	exp->exp_operator = tmp_oper;
    }

    if ((cptr2 == NULL)||(*cptr2 == EOS))
    {
	exp->exp_operand1 = NULL;
    }
    else
    {
	exp->exp_operand1 = (expression_node *)
			    SEP_MEalloc(SEP_ME_TAG_EVAL,
					sizeof(expression_node), TRUE,
					&STSerr);
	if (STSerr != OK)
	{
	    return(STSerr);
	}

	SEP_exp_init((expression_node *)exp->exp_operand1 );
	exp->exp_operand1->exp_str = tmp_buff1;

	SEP_exp_eval((expression_node *)exp->exp_operand1 );
    }

    if ((cptr3 == NULL)||(*cptr3 == EOS))
    {
	exp->exp_operand2 = NULL;
    }
    else
    {
	exp->exp_operand2 = (expression_node *)
			    SEP_MEalloc(SEP_ME_TAG_EVAL,
					sizeof(expression_node), TRUE,
					&STSerr);
	if (STSerr != OK)
	{
	    return(STSerr);
	}

	SEP_exp_init((expression_node *)exp->exp_operand2 );
	exp->exp_operand2->exp_str = tmp_buff2;
	SEP_exp_eval((expression_node *)exp->exp_operand2 );
    }

    /*
    ** ****************************************************************
    **             Do the evaluation at this level.
    ** ****************************************************************
    */

    cptr1 = exp->exp_str;
    exp->exp_error = OK;

    if (exp->exp_operator != NULL)
    {
	exp->exp_oper_code = SEP_opr_what(exp->exp_operator,NULL);
    }

    /*
    ** ****************************************
    ** Case 1:  This is the simplest case. No
    **          operators, just a simple alpha-
    **          numeric.
    ** ****************************************
    */

    if ((exp->exp_operator == NULL)&&
	(exp->exp_operand1 == NULL)&&
	(exp->exp_operand2 == NULL))
    {
	SEPEVAL_alnum(exp);
    }
    else
    /*
    ** ****************************************
    ** Case 2:  Almost same as previous. Maybe
    **          just one set of parens, "(vms)"
    ** ****************************************
    */
    if ((exp->exp_operator == NULL)&&
	(((exp->exp_operand1 != NULL)&&(exp->exp_operand2 == NULL))||
	 ((exp->exp_operand1 == NULL)&&(exp->exp_operand2 != NULL))))
    {
	if (exp->exp_operand2)
	{
	    exp->exp_operand1 = exp->exp_operand2;
	    exp->exp_operand2 = NULL;
	}
	exp->exp_result  = exp->exp_operand1->exp_result;
	exp->exp_f8    = exp->exp_operand1->exp_f8;
	exp->exp_nat   = exp->exp_operand1->exp_nat;
	exp->exp_error = exp->exp_operand1->exp_error;
    }
    else
    /*
    ** ****************************************
    ** Case 3:  {oper}{expression}
    ** ****************************************
    */
    if ((exp->exp_operator != NULL)&&
	(((exp->exp_operand1 != NULL)&&(exp->exp_operand2 == NULL))||
	 ((exp->exp_operand1 == NULL)&&(exp->exp_operand2 != NULL))))
    {
	SEPEVAL_opr_expr(exp);
    }
    else
    /*
    ** ****************************************
    ** Case 4:  {expression}{oper}{expression}
    ** ****************************************
    */
    if ((exp->exp_operator != NULL)&&
	(exp->exp_operand1 != NULL)&&
	(exp->exp_operand2 != NULL))
    {
	SEPEVAL_expr_opr_expr(exp);
    }

    /*
    ** ****************************************************************
    **             Done with evaluation.
    ** ****************************************************************
    */

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEP_exp_eval()>\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    return(exp->exp_result);
}

/*
**  SEP_cmp_oper (). Compare operators for precedence.
**
**  Returns 0 if equal, -1 if op1 is less than op2, or 1.
*/

i4
SEP_cmp_oper(char *op1,char *op2)
{
    i4             g1 ;
    i4             g2 ;

    char           to1 [5] ;
    char           to2 [5] ;
    char          *co1 = NULL ;
    char          *co2 = NULL ;

    for (co1=op1,co2=to1;
	 (CMoper(co1)&&CMcmpcase(co1,DBL_QUO)&&
	  (STindex(l_oper_pairs,co1,3) == NULL));
	 CMcpyinc(co1,co2))
	;

    *co2 = EOS;

    SEP_opr_what(to1,&g1);

    for (co1=op2,co2=to2;
	 (CMoper(co1)&&CMcmpcase(co1,DBL_QUO)&&
	  (STindex(l_oper_pairs,co1,3) == NULL));
	 CMcpyinc(co1,co2))
	;

    *co2 = EOS;

    SEP_opr_what(to2,&g2);

    if (g1 == g2)
    {
	return ( 0);
    }
    else
    if (g1 < g2)
    {
	return (-1);
    }
    else
    {
	return ( 1);
    }
}


i4
SEP_opr_what(char *oper,i4 *grp)
{
    i4              i ;
    i4              clen ;

    clen = SEP_CMstlen(oper, 0);

    if (clen == 1)
    {
	if      (CMcmpcase(ERx("+"), oper) == 0) i = SEP_OPER_PLUS;
	else if (CMcmpcase(ERx("&"), oper) == 0) i = SEP_OPER_B_AND;
	else if (CMcmpcase(ERx("|"), oper) == 0) i = SEP_OPER_B_OR;
	else if (CMcmpcase(ERx("-"), oper) == 0) i = SEP_OPER_MINUS;
	else if (CMcmpcase(ERx("="), oper) == 0) i = SEP_OPER_EQU;
	else if (CMcmpcase(ERx("!"), oper) == 0) i = SEP_OPER_L_NOT;
	else if (CMcmpcase(ERx("<"), oper) == 0) i = SEP_OPER_LT;
	else if (CMcmpcase(ERx(">"), oper) == 0) i = SEP_OPER_GT;
	else if (CMcmpcase(ERx("*"), oper) == 0) i = SEP_OPER_MULT;
	else if (CMcmpcase(ERx("/"), oper) == 0) i = SEP_OPER_DIV;
	else if (CMcmpcase(ERx("~"), oper) == 0) i = SEP_OPER_B_NOT;
	else if (CMcmpcase(ERx("@"), oper) == 0) i = SEP_OPER_ATSIGN;
	else i = 0;
    }
    else
    if (clen == 2)
    {
	if      (STcompare(ERx("++"),oper) == 0) i = SEP_OPER_INCR;
	else if (STcompare(ERx("&&"),oper) == 0) i = SEP_OPER_L_AND;
	else if (STcompare(ERx("||"),oper) == 0) i = SEP_OPER_L_OR;
	else if (STcompare(ERx("--"),oper) == 0) i = SEP_OPER_DECR;
	else if (STcompare(ERx("=="),oper) == 0) i = SEP_OPER_D_EQU;
	else if (STcompare(ERx("!="),oper) == 0) i = SEP_OPER_N_EQU;
	else if (STcompare(ERx("<>"),oper) == 0) i = SEP_OPER_N_EQU;
	else if (STcompare(ERx("><"),oper) == 0) i = SEP_OPER_N_EQU;
	else if (STcompare(ERx("<="),oper) == 0) i = SEP_OPER_LE;
	else if (STcompare(ERx("=<"),oper) == 0) i = SEP_OPER_LE;
	else if (STcompare(ERx("=>"),oper) == 0) i = SEP_OPER_GE;
	else if (STcompare(ERx(">="),oper) == 0) i = SEP_OPER_GE;
	else i = 0;
    }
    else i = 0;

    if (grp != NULL)
    {
	*grp = 0;
	switch (i)
	{
	    /* group 1 */
	   case SEP_OPER_INCR:
	   case SEP_OPER_DECR:
	   case SEP_OPER_L_NOT:
	   case SEP_OPER_B_NOT:
	   case SEP_OPER_B_AND:
	   case SEP_OPER_ATSIGN:
				(*grp)++;
	    /* group 2 */
	   case SEP_OPER_B_OR:
	   case SEP_OPER_MULT:
	   case SEP_OPER_DIV:
				(*grp)++;
	    /* group 3 */
	   case SEP_OPER_PLUS:
	   case SEP_OPER_MINUS:
				(*grp)++;
	    /* group 4 */
	   case SEP_OPER_D_EQU:
	   case SEP_OPER_N_EQU:
	   case SEP_OPER_EQU:
	   case SEP_OPER_LT:
	   case SEP_OPER_GT:
	   case SEP_OPER_LE:
	   case SEP_OPER_GE:
				(*grp)++;
	    /* group 5 */
	   case SEP_OPER_L_AND:
	   case SEP_OPER_L_OR:
				(*grp)++;
		break;

	   default:
		break;
	}
    }

    return (i);
}


VOID
SEP_Vers_Init(char *suffix, char **vers_ion, char **plat_form)
{
/*
** Initialization  of SEP version number. This string is displayed when
** SEP is started in non-batch mode.
*/
    STATUS         Vsts ;
    char          *cptr = NULL ;
    char          *tmp ;

    tmp = SEP_MEalloc(SEP_ME_TAG_NODEL, 255, TRUE, &Vsts);
    STpolycat(4, suffix, ERx(" "), SEP_VER, Version, tmp);

    *vers_ion = STtalloc(SEP_ME_TAG_NODEL, tmp);

    if ((cptr = STindex((char *)Version, ERx("("), 0)) != NULL)
    {
	CMnext(cptr);
	STlcopy(cptr, tmp, 7);
    }

    *plat_form = STtalloc(SEP_ME_TAG_NODEL, tmp);

    MEfree(tmp);

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("Leaving SEP_Vers_Init()>\n"),traceptr);
	SIfprintf(traceptr,ERx("    vers = %s\n"),*vers_ion);
	SIfprintf(traceptr,ERx("    plat = %s\n"),*plat_form);
	SIflush(traceptr);
    }
}


VOID
SEP_alloc_Opt(OPTION_LIST **opthead, char *list, char *retstr, bool *active,
	      i4  *retnat, f8 *retfloat, bool arg_req)
{
    OPTION_LIST     *optptr;
    OPTION_LIST     *opttmp;
    OPTION_LIST     *header = *opthead;
    STATUS           OPTerr;

    optptr = (OPTION_LIST *)SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(OPTION_LIST),
					TRUE, &OPTerr);

    optptr->o_list          = STtalloc(SEP_ME_TAG_NODEL, list);
    optptr->o_retstr        = retstr;
    optptr->o_active        = active;
    optptr->o_retnat        = retnat;
    optptr->o_retfloat      = retfloat;
    optptr->arg_required    = arg_req;
    optptr->o_next          = (OPTION_LIST *)NULL;
    optptr->o_prev          = (OPTION_LIST *)NULL;

    if (*opthead == NULL)
    {
	*opthead = optptr;
    }
    else if (header->o_next == NULL)
    {
	header->o_next = header->o_prev = optptr;
	optptr->o_next = optptr->o_prev = header;
    }
    else
    {
	opttmp         = header->o_prev;
	opttmp->o_next = optptr;
	header->o_prev = optptr;
	optptr->o_next = header;
	optptr->o_prev = opttmp;
    }

    return;
}

i4
SEPEVAL_alnum(expression_node *exp)
{
    char          *cptr1 = exp->exp_str ;
    char          *cptr2 = NULL ;
    i4             i ;
    i4             clen ;

    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_alnum\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    if (CMcmpcase(cptr1,DBL_QUO) == 0)
    {
	/* String Literal */
	exp->exp_result = SEP_OPERAND_STR;
	cptr2 = cptr1;
	CMnext(cptr2);
	while (CMcmpcase(cptr2,DBL_QUO)!=0 )
	{
	    CMcpychar(cptr2,cptr1);
	    CMnext(cptr2);
	    CMnext(cptr1);
	}
	*cptr1 = EOS;
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("VMS"), 0, TRUE) == 0)
    {
#ifdef VMS
	exp->exp_result  = TRUE;
#else
	exp->exp_result  = FALSE;
#endif
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("ALPHA"), 0, TRUE) == 0)
    {
#ifdef ALPHA
	exp->exp_result  = TRUE;
#else
	exp->exp_result  = FALSE;
#endif
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("MPE"), 0, TRUE) == 0)
    {
#ifdef MPE
	exp->exp_result  = TRUE;
#else
	exp->exp_result  = FALSE;
#endif
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("UNIX"), 0, TRUE) == 0)
    {
#ifdef UNIX
	exp->exp_result = TRUE;
#else
	exp->exp_result = FALSE;
#endif
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("NT_GENERIC"), 0, TRUE) == 0)
    {
#ifdef NT_GENERIC
	exp->exp_result = TRUE;
#else
	exp->exp_result = FALSE;
#endif
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("VAX"), 0, TRUE) == 0)
    {
#ifdef VAX
	exp->exp_result = TRUE;
#else
	exp->exp_result = FALSE;
#endif
    }
    else
    if (STbcompare(exp->exp_str, 0, ERx("LP64"), 0, TRUE) == 0)
    {
#ifdef LP64
	exp->exp_result = TRUE;
#else
	exp->exp_result = FALSE;
#endif
    }
    else
    if ((STbcompare(exp->exp_str, 0, ERx("DIFFS"), 0, TRUE) == 0) ||
	(STbcompare(exp->exp_str, 0, ERx("DIFFERENCES"), 0, TRUE) == 0))
    {
	exp->exp_result = SEP_OPERAND_NAT;
	exp->exp_nat  = testGrade ;
	exp->exp_f8   = testGrade ;
    }
    else
    if ((STbcompare(exp->exp_str, 0, ERx("STATUS"), 0, TRUE) == 0))
    {
	exp->exp_result = SEP_OPERAND_NAT;
	exp->exp_nat  = Status_Value ;
	exp->exp_f8   = Status_Value ;
    }
    else
    if ((STbcompare(exp->exp_str, 0, ERx("TRUE"), 0, TRUE) == 0) ||
        (STbcompare(exp->exp_str, 0, ERx("YES"), 0, TRUE) == 0))
    {
	exp->exp_result = TRUE;
    }
    else
    if ((STbcompare(exp->exp_str, 0, ERx("FALSE"), 0, TRUE) == 0) ||
        (STbcompare(exp->exp_str, 0, ERx("NO"), 0, TRUE) == 0))
    {
	exp->exp_result = FALSE;
    }
    else
    {
	clen = SEP_CMstlen(exp->exp_str, 0);
	if ((clen >= 8)&&
	    (STbcompare(exp->exp_str, 8, ERx("SEPPARAM"), 8, TRUE) == 0))
	{
	    exp->exp_result = SEP_OPERAND_PARAM;
	}
	else
	{
	    for (i=0,cptr1 = exp->exp_str; i<3; i++)
	    {
		 CMnext(cptr1);
	    }

	    if ((clen == 7) && (CMcmpcase(cptr1,ERx(".")) == 0))
	    {
		if (STbcompare(sep_platform,0,exp->exp_str,0,TRUE) == 0)
		{
		    exp->exp_result = TRUE;
		}
		else
		{
		    exp->exp_result = FALSE;
		}
	    }
	    else
	    {
		char *c_ptr      = NULL;

		(STATUS) SEP_STclassify( exp->exp_str, &exp->exp_result,
                                        &exp->exp_nat, &exp->exp_f8);

		if (exp->exp_result == SEP_OPERAND_STR)
		{
		    NMgtAt(exp->exp_str,&c_ptr);
		    if ((c_ptr != NULL)&&(*c_ptr != EOS))
		    {
			(STATUS) SEP_STclassify( c_ptr, &exp->exp_result,
                                                &exp->exp_nat, &exp->exp_f8);

			exp->exp_str = STtalloc(SEP_ME_TAG_EVAL, c_ptr);
		    }
		    else
		    {
			*exp->exp_str = EOS;
		    }
		}
	    }
	}
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_alnum\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    return (exp->exp_result);
}

i4
SEPEVAL_opr_expr(expression_node *exp)
{
    char          *cptr1 = NULL ;

    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_opr_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    if (exp->exp_operand2)
    {
	exp->exp_operand1 = exp->exp_operand2;
	exp->exp_operand2 = NULL;
    }

    switch (exp->exp_oper_code)
    {
       case SEP_OPER_INCR:
	    switch(exp->exp_result = exp->exp_operand1->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat = exp->exp_operand1->exp_nat + 1;
	       case SEP_OPERAND_F8:
		    exp->exp_f8  = exp->exp_operand1->exp_f8  + 1;
		    break;
	    }
	    break;

       case SEP_OPER_DECR:
	    switch(exp->exp_result = exp->exp_operand1->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat = exp->exp_operand1->exp_nat - 1;
	       case SEP_OPERAND_F8:
		    exp->exp_f8  = exp->exp_operand1->exp_f8  - 1;
		    break;
	    }
	    break;

       case SEP_OPER_PLUS:
	    switch(exp->exp_result = exp->exp_operand1->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat = exp->exp_operand1->exp_nat;
	       case SEP_OPERAND_F8:
		    exp->exp_f8  = exp->exp_operand1->exp_f8;
		    break;
	    }
	    break;

       case SEP_OPER_MINUS:
	    switch(exp->exp_result = exp->exp_operand1->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat = (-1)*exp->exp_operand1->exp_nat;
	       case SEP_OPERAND_F8:
		    exp->exp_f8  = (-1)*exp->exp_operand1->exp_f8;
		    break;
	    }
	    break;

       case SEP_OPER_L_NOT:
	    switch(exp->exp_result = exp->exp_operand1->exp_result)
	    {
	       case TRUE: case FALSE:
		    exp->exp_result = exp->exp_operand1->exp_result;
		    SEPEVAL_flip_it(exp);
		    break;

	       case SEP_OPERAND_PARAM:
		    CVupper(exp->exp_operand1->exp_str);
		    NMgtAt(exp->exp_operand1->exp_str,&cptr1);
		    exp->exp_result = (i4)((cptr1 == NULL)||(*cptr1 == EOS));
		    break;
	    }
	    break;
       case SEP_OPER_ATSIGN:
	    break;
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_opr_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    return (exp->exp_result);
}

i4
SEPEVAL_expr_opr_expr(expression_node *exp)
{
    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_opr_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_oper_code)
    {
       case SEP_OPER_PLUS:  SEPEVAL_expr_plus_expr(exp);
			    break;

       case SEP_OPER_MINUS: SEPEVAL_expr_minus_expr(exp);
			    break;

       case SEP_OPER_MULT:  SEPEVAL_expr_mult_expr(exp);
			    break;

       case SEP_OPER_DIV:   SEPEVAL_expr_div_expr(exp);
			    break;

       case SEP_OPER_D_EQU:
       case SEP_OPER_EQU:   SEPEVAL_expr_equ_expr(exp);
			    break;

       case SEP_OPER_LT:    SEPEVAL_expr_lt_expr(exp);
			    break;

       case SEP_OPER_N_EQU: SEPEVAL_expr_equ_expr(exp);
			    SEPEVAL_flip_it(exp);
			    break;

       case SEP_OPER_LE:    if (SEPEVAL_expr_equ_expr(exp) == FALSE)
			    {
				SEPEVAL_expr_lt_expr(exp);
			    }
			    break;

       case SEP_OPER_GT:
			    if (SEPEVAL_expr_equ_expr(exp) == FALSE)
			    {
				SEPEVAL_expr_lt_expr(exp);
			    }
			    SEPEVAL_flip_it(exp);
			    break;

       case SEP_OPER_GE:    if (SEPEVAL_expr_equ_expr(exp) == FALSE)
			    {
				SEPEVAL_expr_lt_expr(exp);
				SEPEVAL_flip_it(exp);
			    }
			    break;

       case SEP_OPER_L_AND:

	    switch (exp->exp_operand1->exp_result)
	    {
	       case TRUE: case FALSE:
		    switch (exp->exp_operand2->exp_result)
		    {
		       case TRUE: case FALSE:
			    exp->exp_result = (i4)(exp->exp_operand1->exp_result &&
					          exp->exp_operand2->exp_result);
			    break;
		    }
		    break;
	    }
	    break;

       case SEP_OPER_L_OR:

	    switch (exp->exp_operand1->exp_result)
	    {
	       case TRUE: case FALSE:
		    switch (exp->exp_operand2->exp_result)
		    {
		       case TRUE: case FALSE:
			    exp->exp_result = (i4)(exp->exp_operand1->exp_result ||
					          exp->exp_operand2->exp_result);
			    break;
		    }
		    break;
	    }
	    break;

       case SEP_OPER_B_AND:

	    switch (exp->exp_operand1->exp_result)
	    {
	       case TRUE: case FALSE:
		    switch (exp->exp_operand2->exp_result)
		    {
		       case TRUE: case FALSE:
			    exp->exp_result = (i4)(exp->exp_operand1->exp_result &
						  exp->exp_operand2->exp_result);
			    break;
		    }
		    break;

	       case SEP_OPERAND_NAT:
		    switch (exp->exp_operand2->exp_result)
		    {
		       case SEP_OPERAND_NAT:
			    exp->exp_result = exp->exp_operand1->exp_result;
			    exp->exp_nat  = exp->exp_operand1->exp_nat &
					    exp->exp_operand2->exp_nat;
			    exp->exp_f8   = (f8) (exp->exp_nat);
			    break;
		    }
		    break;
	    }
	    break;

       case SEP_OPER_B_OR:

	    switch (exp->exp_operand1->exp_result)
	    {
	       case TRUE: case FALSE:
		    switch (exp->exp_operand2->exp_result)
		    {
		       case TRUE: case FALSE:
			    exp->exp_result = (i4)(exp->exp_operand1->exp_result |
						  exp->exp_operand2->exp_result);
			    break;
		    }
		    break;

	       case SEP_OPERAND_NAT:
		    switch (exp->exp_operand2->exp_result)
		    {
		       case SEP_OPERAND_NAT:
			    exp->exp_result = exp->exp_operand1->exp_result;
			    exp->exp_nat  = exp->exp_operand1->exp_nat |
					    exp->exp_operand2->exp_nat;
			    exp->exp_f8   = (f8) (exp->exp_nat);
			    break;
		    }
		    break;
	    }
	    break;
       /*
       ** These cases are not valid and will FAIL.
       case SEP_OPER_B_NOT:
       case SEP_OPER_INCR:
       case SEP_OPER_DECR:
       case SEP_OPER_L_NOT:
       */
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_opr_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }
    return (exp->exp_result);
}

i4
SEPEVAL_expr_minus_expr(expression_node *exp)
{
    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_minus_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_operand1->exp_result)
    {
       case SEP_OPERAND_NAT:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat  = exp->exp_operand1->exp_nat
				  - exp->exp_operand2->exp_nat;
	       case SEP_OPERAND_F8:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  - exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand2->exp_result;
		    break;

	       default:
		    exp->exp_error = FAIL;
		    exp->exp_result  = SEP_OPERAND_FAIL;
		    break;
	    }
	    break;

       case SEP_OPERAND_F8:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_F8:
	       case SEP_OPERAND_NAT:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  - exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand1->exp_result;
		    break;

	       default:
		    exp->exp_error = FAIL;
		    exp->exp_result  = SEP_OPERAND_FAIL;
		    break;
	    }
	    break;

       default:
	    exp->exp_error = FAIL;
	    exp->exp_result  = SEP_OPERAND_FAIL;
	    break;
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_minus_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }
    return (exp->exp_result);
}

i4
SEPEVAL_expr_plus_expr(expression_node *exp)
{
    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_plus_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_operand1->exp_result)
    {
       case SEP_OPERAND_NAT:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat  = exp->exp_operand1->exp_nat
				  + exp->exp_operand2->exp_nat;
	       case SEP_OPERAND_F8:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  + exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand2->exp_result;
		    break;
	    }
	    break;

       case SEP_OPERAND_F8:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_F8:
	       case SEP_OPERAND_NAT:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  + exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand1->exp_result;
		    break;
	    }
	    break;
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_plus_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    return (exp->exp_result);
}

i4
SEPEVAL_expr_mult_expr(expression_node *exp)
{
    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_mult_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_operand1->exp_result)
    {
       case SEP_OPERAND_NAT:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat  = exp->exp_operand1->exp_nat
				  * exp->exp_operand2->exp_nat;
	       case SEP_OPERAND_F8:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  * exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand2->exp_result;
		    break;
	    }
	    break;

       case SEP_OPERAND_F8:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_F8:
	       case SEP_OPERAND_NAT:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  * exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand1->exp_result;
		    break;
	    }
	    break;
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_mult_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }
    return (exp->exp_result);
}

i4
SEPEVAL_expr_div_expr(expression_node *exp)
{
    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_div_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_operand1->exp_result)
    {
       case SEP_OPERAND_NAT:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_nat  = exp->exp_operand1->exp_nat
				  / exp->exp_operand2->exp_nat;
	       case SEP_OPERAND_F8:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  / exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand2->exp_result;
		    break;
	    }
	    break;

       case SEP_OPERAND_F8:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_F8:
	       case SEP_OPERAND_NAT:
		    exp->exp_f8   = exp->exp_operand1->exp_f8
				  / exp->exp_operand2->exp_f8;
		    exp->exp_result = exp->exp_operand1->exp_result;
		    break;
	    }
	    break;
    }

    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_div_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }
    return (exp->exp_result);
}

i4
SEPEVAL_expr_equ_expr(expression_node *exp)
{
    char          *cptr1 = NULL ;
    char          *cptr2 = NULL ;

    i4             i ;
    i4             j ;

    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_equ_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_operand1->exp_result)
    {
       case TRUE: case FALSE:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case TRUE: case FALSE:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_result ==
				          exp->exp_operand2->exp_result );
		    break;
	    }
	    break;

       case SEP_OPERAND_NAT:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_nat ==
				          exp->exp_operand2->exp_nat );
		    break;

	       case SEP_OPERAND_F8:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_f8  ==
				          exp->exp_operand2->exp_f8 );
		    break;
	    }
	    break;

       case SEP_OPERAND_F8:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
	       case SEP_OPERAND_F8:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_f8  ==
				          exp->exp_operand2->exp_f8 );
		    break;
	    }
	    break;

       case SEP_OPERAND_STR:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_STR:
		    exp->exp_result = (i4)
			  (STbcompare(exp->exp_operand1->exp_str,0,
				      exp->exp_operand2->exp_str,0,
				      FALSE) == 0);
		    break;

	       case SEP_OPERAND_PARAM:
		    CVupper(exp->exp_operand2->exp_str);
		    NMgtAt(exp->exp_operand2->exp_str,&cptr1);
		    if ((cptr1 == NULL)||(*cptr1 == EOS))
		    {
			exp->exp_result = FALSE;
		    }
		    else
		    {
			i = STlength(cptr1);
			cptr2 = exp->exp_operand1->exp_str;
			if (CMcmpcase(cptr2,DBL_QUO) == 0)
			{
			    CMnext(cptr2);
			    j = STlength(cptr2);
			    if (CMcmpcase(SEP_CMlastchar(cptr2,0),DBL_QUO) == 0)
			    {
				j--;
			    }

			    i = max(i,j);
			}
			else
			{
			    i = j = 0;
			}

			exp->exp_result = (i4)(STbcompare(cptr2,i,cptr1,i,TRUE) == 0);
		    }
		    break;
	    }
	    break;

       case SEP_OPERAND_PARAM:
	    CVupper(exp->exp_operand1->exp_str);
	    NMgtAt(exp->exp_operand1->exp_str,&cptr1);
	    if ((cptr1 == NULL)||(*cptr1 == EOS))
	    {
		exp->exp_result = FALSE;
		break;
	    }
	    i = STlength(cptr1);

	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_STR:
		    cptr2 = exp->exp_operand2->exp_str;
		    if (CMcmpcase(cptr2,DBL_QUO) == 0)
		    {
			CMnext(cptr2);
			j = STlength(cptr2);
			if (CMcmpcase(SEP_CMlastchar(cptr2,0), DBL_QUO) == 0)
			{
			    j--;
			}

			i = max(i,j);
		    }
		    else
		    {
			i = j = 0;
		    }

		    exp->exp_result = (i4)(STbcompare(cptr1,i,cptr2,i, TRUE) == 0);
		    break;

	       case SEP_OPERAND_PARAM:
		    CVupper(exp->exp_operand2->exp_str);
		    NMgtAt(exp->exp_operand2->exp_str,&cptr2);
		    if ((cptr2 == NULL)||(*cptr2 == EOS))
		    {
			exp->exp_result = FALSE;
		    }
		    else
		    {
			exp->exp_result = (i4)(STbcompare(cptr1,0,cptr2, 0,TRUE)==0);
		    }
		    break;
	    }
	    break;
    }
    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_equ_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }
    return (exp->exp_result);
}

i4
SEPEVAL_expr_lt_expr(expression_node *exp)
{
    exp->exp_result  = SEP_OPERAND_FAIL;

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nIn SEPEVAL_expr_lt_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }

    switch (exp->exp_operand1->exp_result)
    {
       case SEP_OPERAND_NAT:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_nat <
				          exp->exp_operand2->exp_nat );
		    break;
	       case SEP_OPERAND_F8:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_f8  <
				          exp->exp_operand2->exp_f8 );
		    break;
	    }
	    break;

       case SEP_OPERAND_F8:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_NAT:
	       case SEP_OPERAND_F8:
		    exp->exp_result = (i4)(exp->exp_operand1->exp_f8  <
				          exp->exp_operand2->exp_f8 );
		    break;
	    }
	    break;

       case SEP_OPERAND_STR:
	    switch (exp->exp_operand2->exp_result)
	    {
	       case SEP_OPERAND_STR:
		    exp->exp_result = (i4)
			  (STbcompare(exp->exp_operand1->exp_str,0,
				      exp->exp_operand2->exp_str,0,
				      FALSE) < 0);
		    break;
	    }
	    break;
    }
    if (exp->exp_result == SEP_OPERAND_FAIL)
    {
	exp->exp_error = FAIL;
    }

    if (tracing&TRACE_SEPCM)
    {
	SIputrec(ERx("\nLeaving SEPEVAL_expr_lt_expr\n"),traceptr);
	SEP_TRACE_exp_eval(exp);
    }
    return (exp->exp_result);
}

VOID
SEP_TRACE_exp_result(char *prefix, i4  exp_result)
{
    if (tracing&TRACE_SEPCM)
    {
	if (prefix)
	{
	    SIputrec(prefix,traceptr);
	}

	switch (exp_result)
	{
	   case SEP_OPERAND_INIT:  SIputrec(ERx("INIT"),traceptr);
				   break;
	   case SEP_OPERAND_F8:    SIputrec(ERx("F8"),traceptr);
				   break;
	   case SEP_OPERAND_NAT:   SIputrec(ERx("NAT"),traceptr);
				   break;
	   case SEP_OPERAND_STR:   SIputrec(ERx("STR"),traceptr);
				   break;
	   case SEP_OPERAND_PARAM: SIputrec(ERx("PARAM"),traceptr);
				   break;
	   case SEP_OPERAND_FAIL:  SIputrec(ERx("FAIL"),traceptr);
				   break;
	   case TRUE:              SIputrec(ERx("TRUE"),traceptr);
				   break;
	   case FALSE:             SIputrec(ERx("FALSE"),traceptr);
				   break;
	   default:                SIfprintf(traceptr,ERx("%d"),exp_result);
				   break;
	}
    
	if (prefix)
	{
	    SIputrec(ERx("\n"),traceptr);
	}
	SIflush(traceptr);
    }
}

VOID
SEP_TRACE_exp_oper(char *prefix, char *exp_operator, i4  exp_oper_code)
{
    if (tracing&TRACE_SEPCM)
    {
	if (prefix)
	{
	    SIputrec(prefix,traceptr);
	}

	if ( exp_operator == NULL)
	{
	    SIputrec(ERx("NULL"),traceptr);
	}
	else if (*exp_operator == EOS)
	{
	    SIputrec(ERx("EOS"),traceptr);
	}
	else
	{
	    SIfprintf(traceptr,ERx("%s    ("),exp_operator);
	    switch (exp_oper_code)
	    {
	       case SEP_OPER_INCR:  SIputrec(ERx("INCR"),traceptr);
				    break;
	       case SEP_OPER_L_AND: SIputrec(ERx("L_AND"),traceptr);
				    break;
	       case SEP_OPER_L_OR:  SIputrec(ERx("L_OR"),traceptr);
				    break;
	       case SEP_OPER_DECR:  SIputrec(ERx("DECR"),traceptr);
				    break;
	       case SEP_OPER_D_EQU: SIputrec(ERx("D_EQU"),traceptr);
				    break;
	       case SEP_OPER_N_EQU: SIputrec(ERx("N_EQU"),traceptr);
				    break;
	       case SEP_OPER_PLUS:  SIputrec(ERx("PLUS"),traceptr);
				    break;
	       case SEP_OPER_B_AND: SIputrec(ERx("B_AND"),traceptr);
				    break;
	       case SEP_OPER_B_OR:  SIputrec(ERx("B_OR"),traceptr);
				    break;
	       case SEP_OPER_MINUS: SIputrec(ERx("MINUS"),traceptr);
				    break;
	       case SEP_OPER_EQU:   SIputrec(ERx("EQU"),traceptr);
				    break;
	       case SEP_OPER_L_NOT: SIputrec(ERx("L_NOT"),traceptr);
				    break;
	       case SEP_OPER_LT:    SIputrec(ERx("LT"),traceptr);
				    break;
	       case SEP_OPER_GT:    SIputrec(ERx("GT"),traceptr);
				    break;
	       case SEP_OPER_MULT:  SIputrec(ERx("MULT"),traceptr);
				    break;
	       case SEP_OPER_B_NOT: SIputrec(ERx("B_NOT"),traceptr);
				    break;
	       case SEP_OPER_LE:    SIputrec(ERx("LE"),traceptr);
				    break;
	       case SEP_OPER_GE:    SIputrec(ERx("GE"),traceptr);
				    break;
	       case SEP_OPER_DIV:   SIputrec(ERx("DIV"),traceptr);
				    break;
	       default:         SIfprintf(traceptr,ERx("?%d?"),exp_oper_code);
				break;
	    }
	    SIputrec(ERx(")"),traceptr);
	}

	if (prefix)
	{
	    SIputrec(ERx("\n"),traceptr);
	}
	SIflush(traceptr);
    }
}

VOID
SEP_TRACE_operand(char *prefix, expression_node *exp)
{
    if (tracing&TRACE_SEPCM)
    {
	if (prefix)
	{
	    SIputrec(prefix,traceptr);
	}

	SIputrec(exp == NULL ? ERx("(ptr)NULL")     :
		 exp->exp_str == NULL ? ERx("NULL") :
		 *exp->exp_str == EOS ? ERx("EOS")  : exp->exp_str, traceptr);

	if (prefix)
	{
	    SIputrec(ERx("\n"),traceptr);
	}
	SIflush(traceptr);
    }
}

/*
** IS_ValueNotation
*/

bool
IS_ValueNotation(char *varnm)
{
    bool                   ret_bool ;
    char                  *tmp_ptr ;

    if (tracing&TRACE_PARM)
    {
	SIfprintf(traceptr,ERx("IS_ValueNotation> varnm = %s\n"),varnm);
	SIflush(traceptr);
    }

    ret_bool = ((tmp_ptr = FIND_SEPstring(varnm,ERx("@VALUE("))) != NULL);

    if (tracing&TRACE_PARM)
    {
	SIputrec(ERx("IS_ValueNotation> return "),traceptr);
	SIputrec(ret_bool ? ERx("TRUE\n") : ERx("FALSE\n"),traceptr);
	SIflush(traceptr);
    }

    return (ret_bool);
}

STATUS
Trans_SEPvar(char **Token, bool Dont_free, i2 ME_tag)
{
    STATUS                 syserr = OK ;
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
	SIfprintf(traceptr,ERx("Trans_SEPvar> *Token = %s\n"),*Token);
	SIputrec(ERx("                Dont_free = "),traceptr);
	SIputrec(Dont_free ? ERx("TRUE\n") : ERx("FALSE\n"),traceptr);
	SIflush(traceptr);
    }

    tmp_token = *Token;

    syserr = ((token_ptr = FIND_SEPstring(tmp_token,ERx("@VALUE("))) == NULL)
	   ? FAIL : OK;

    if (tracing&TRACE_PARM)
    {
	SIputrec(ERx("Trans_SEPvar> FIND_SEPstring is "),traceptr);
	SIputrec(syserr == FAIL ? ERx("FALSE\n") : ERx("TRUE\n"),traceptr);
	SIflush(traceptr);
    }

    if (syserr == FAIL)
    {
	return(syserr);
    }

    tmp_buffer1 = SEP_MEalloc(SEP_ME_TAG_MISC, 128, TRUE, (STATUS *) NULL);
    tmp_buffer2 = SEP_MEalloc(SEP_ME_TAG_MISC, 128, TRUE, (STATUS *) NULL);

    if (token_ptr != tmp_token)
    {
	for (tmp_ptr1 = tmp_buffer1, tmp_ptr2 = tmp_token;
	     tmp_ptr2 != token_ptr; CMcpyinc(tmp_ptr2,tmp_ptr1)) {} ;
    }
    else
    {
	tmp_ptr2 = token_ptr;
    }

    for ( ; CMcmpcase(tmp_ptr2, ERx("(")); CMnext(tmp_ptr2)) {} ;
    CMnext(tmp_ptr2);

    for (tmp_ptr3 = tmp_buffer2; CMcmpcase(tmp_ptr2,ERx(")"));
	 CMcpyinc(tmp_ptr2,tmp_ptr3)) {} ;
    CMnext(tmp_ptr2);

    newstr = SEP_MEalloc(SEP_ME_TAG_MISC, 128, TRUE, (STATUS *) NULL);

    /*
    ** **************************************************************** **
    */
    if (STbcompare(tmp_buffer2, 0, ERx("VMS"), 0, TRUE) == 0)
    {
	STcopy(IS_VMS_TorF,newstr);
    }
    else
    if (STbcompare(tmp_buffer2, 0, ERx("ALPHA"), 0, TRUE) == 0)
    {
	STcopy(IS_ALPHA_TorF,newstr);
    }
    else
    if (STbcompare(tmp_buffer2, 0, ERx("MPE"), 0, TRUE) == 0)
    {
	STcopy(IS_MPE_TorF,newstr);
    }
    else
    if (STbcompare(tmp_buffer2, 0, ERx("UNIX"), 0, TRUE) == 0)
    {
	STcopy(IS_UNIX_TorF,newstr);
    }
    else
    if (STbcompare(tmp_buffer2, 0, ERx("NT_GENERIC"), 0, TRUE) == 0)
    {
	STcopy(IS_NT_GENERIC_TorF,newstr);
    }
    else
    if (STbcompare(tmp_buffer2, 0, ERx("VAX"), 0, TRUE) == 0)
    {
	STcopy(IS_VAX_TorF,newstr);
    }
    else
    if ((STbcompare(tmp_buffer2, 0, ERx("DIFFS"), 0, TRUE) == 0) ||
	(STbcompare(tmp_buffer2, 0, ERx("DIFFERENCES"), 0, TRUE) == 0))
    {
	STprintf(newstr,ERx("%d"), testGrade);
    }
    else
    if ((STbcompare(tmp_buffer2, 0, ERx("STATUS"), 0, TRUE) == 0))
    {
	STprintf(newstr,ERx("%d"), Status_Value);
    }
    else
    {
	MEfree(newstr);
	SEP_NMgtAt(tmp_buffer2, &newstr, SEP_ME_TAG_MISC);
    }
    /*
    ** **************************************************************** **
    */

    if (tracing&TRACE_PARM)
    {
	SIputrec(ERx("Trans_SEPvar> newstr = "),traceptr);
	SIputrec(newstr ? newstr : ERx("NULL"),traceptr);
	SIputrec(ERx("\n"),traceptr);

	SIfprintf(traceptr,ERx("Trans_SEPvar> tmp_buffer1 = %s\n"),tmp_buffer1);
        SIfprintf(traceptr,ERx("Trans_SEPvar> tmp_buffer2 = %s\n"),tmp_buffer2);
	SIflush(traceptr);
    }
    if (newstr)
    {
	STcat(tmp_buffer1, newstr);
    }

    STcat(tmp_buffer1, tmp_ptr2);

    if (tracing&TRACE_PARM)
    {
        SIfprintf(traceptr,ERx("Trans_SEPvar> tmp_buffer1 = %s\n"),tmp_buffer1);
	SIflush(traceptr);
    }

    temp_ptr = *Token;
    *Token = STtalloc(ME_tag,tmp_buffer1);
    if (Dont_free != TRUE)
    {
	MEfree(temp_ptr);
    }

    MEfree(tmp_buffer1);
    MEfree(tmp_buffer2);
    if (newstr)
    {
	MEfree(newstr);
    }

    if (tracing&TRACE_PARM)
    {
        SIfprintf(traceptr,ERx("Trans_SEPvar> *Token = %s\n"),*Token);
	SIflush(traceptr);
    }
 
    return (syserr);
}
STATUS
SEP_STclassify( char *str, i4  *result_ptr, i4  *nat_ptr, f8 *f8_ptr )
{
    STATUS ret_val = OK ;
    char *cptr1 ;

    bool  has_digits = FALSE;
    bool  has_alphas = FALSE;
    bool  has_others = FALSE;
    char *dot_ptr    = NULL;
    char *e_ptr      = NULL;

    for (cptr1 = str; *cptr1 != EOS; CMnext(cptr1))
    {
	if ((!has_digits)&&(CMdigit(cptr1)))
            has_digits = TRUE;
        else
        if ((!has_alphas)&&(CMalpha(cptr1)))
            has_alphas = TRUE;
        else
        if (!has_others)
            has_others = TRUE;

        if (CMcmpcase(cptr1,ERx(".")) == 0)
            dot_ptr = cptr1;
        else
        if ((CMcmpcase(cptr1,ERx("e")) == 0)&&
            (dot_ptr != NULL)&&has_digits)
            e_ptr = cptr1;
    }

    if ((e_ptr != NULL)||
        ((dot_ptr != NULL)&&has_digits&&
        (!(has_alphas||has_others))))
    {
	if (result_ptr != NULL)
            *result_ptr = SEP_OPERAND_F8;

        if (f8_ptr != NULL)
	    CVaf(str,'.',f8_ptr);
    }
    else
    if (has_digits&&(!(has_alphas||has_others)))
    {
        if (result_ptr != NULL)
            *result_ptr = SEP_OPERAND_NAT;

        if (nat_ptr != NULL)
            CVan(str,nat_ptr);

        if (f8_ptr != NULL)
            CVaf(str,'.',f8_ptr);
    }
    else
    {
        if (result_ptr != NULL)
            *result_ptr = SEP_OPERAND_STR;
    }
    return (ret_val);
}
