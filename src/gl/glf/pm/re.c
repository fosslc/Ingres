/*
**  re.c -- contains externally callable functions REcompile() and REexec()
**
**	@(#)re.c	1.3 of 18 April 87
**	Copyright (c) 2004 Ingres Corporation
**	Written by Henry Spencer.  Not derived from licensed software.
**
**	Permission is granted to anyone to use this software for any
**	purpose on any computer system, and to redistribute it freely,
**	subject to the following restrictions:
**
**	1. The author is not responsible for the consequences of use of
**		this software, no matter how awful, even if they arise
**		from defects in it.
**
**	2. The origin of this software must not be misrepresented, either
**		by explicit claim or by omission.
**
**	3. Altered versions must be plainly marked as such, and must not
**		be misrepresented as being the original software.
**
**  Beware that some of this code is subtly aware of the way operator
**  precedence is structured in regular expressions.  Serious changes in
**  regular-expression syntax might require a total rethink.
**
**  History:
**	10-20-92 (tyler) 
**		Adapted from the original to be INGRES CL compliant in
**		order to be included in the PM GL module.  Eventually,
**		this code should migrate into a separate GL module (RE?).
**	28-Nov-1992 (terjeb)
**	    Rename strcspn to my_strcspn to avoid confusion with library
**          routines, which may have conflicting prototypes.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	28-mar-95 (peeje01)
**	    Crossintegration of doublebyte label 6500db_su4_us42
**	    Magic number is changed
**	    CMbyteinc replaces a ++ but the rest of the changes
**	    back out usage of CMnext.
**	21-jan-1999 (canor01)
**	    Clean up function prototypes.
**    21-apr-1999 (hanch04)
**        Replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Feb-2004 (fanra01)
**	    In range expressions initialize the range end when encoding the
**	    expression.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	    _error is pretty bogus here, as this might be in backend
**	    context, but at least it compiles clean now...
*/

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <st.h>
# include <me.h>
# include <cm.h>
# include <pc.h>
# include <er.h>
# include <lo.h>
# include <pm.h> /* required since re.h isn't externally includable */
# include "re.h"

/*
 * The "internal use only" fields in re.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * regstart	char that must begin a match; '\0' if none obvious
 * reganch	is the match anchored (at beginning-of-line only)?
 * regmust	string (pointer into program) that match must include, or NULL
 * regmlen	length of regmust string
 *
 * Regstart and reganch permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  Regmust permits fast rejection
 * of lines that cannot possibly match.  The regmust tests are costly enough
 * that REcompile() supplies a regmust only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  Regmlen is
 * supplied because the test in REexec() needs it and REcompile() is computing
 * it anyway.
 */

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH implement concatenation; a "next" pointer with
 * a BRANCH on both ends of it is connecting two alternatives.  (Here we
 * have one of the subtle syntax dependencies:  an individual BRANCH (as
 * opposed to a collection of them) is never concatenated with anything
 * because of operator precedence.)  The operand of some types of node is
 * a literal string; for others, it is a node leading into a sub-FSM.  In
 * particular, the operand of a BRANCH node is the first node of the branch.
 * (NB this is *not* a tree structure:  the tail of the branch connects
 * to the thing following the set of BRANCHes.)  The opcodes are:
 */

/* definition		number	opnd?	meaning */
# define END		0	/* no	End of program. */
# define BOL		1	/* no	Match "" at beginning of line. */
# define EOL		2	/* no	Match "" at end of line. */
# define ANY		3	/* no	Match any one character. */
# define ANYOF		4	/* str	Match any character in this string. */
# define ANYBUT		5	/* str	Match any character not in string. */
# define BRANCH		6	/* node	Match this alternative, or next... */
# define BACK		7	/* no	Match "", "next" ptr points backward. */
# define EXACTLY	8	/* str	Match this string. */
# define NOTHING	9	/* no	Match empty string. */
# define STAR		10	/* node	Match this (simple) thing 0 or more */
# define PLUS		11	/* node	Match this (simple) thing 1 or more */
# define OPEN		20	/* no	Mark this point in input as start */
				/*	OPEN+1 is number 1, etc. */
# define CLOSE		30	/* no	Analogous to OPEN. */

/*
 * Opcode notes:
 *
 * BRANCH	The set of branches constituting a single choice are hooked
 *		together with their "next" pointers, since precedence prevents
 *		anything being concatenated to any individual branch.  The
 *		"next" pointer of the last BRANCH in a choice points to the
 *		thing following the whole choice.  This is also where the
 *		final "next" pointer of each individual branch points; each
 *		branch starts with the operand node of a BRANCH node.
 *
 * BACK		Normal "next" pointers all implicitly point forward; BACK
 *		exists to make loop structures possible.
 *
 * STAR,PLUS	'?', and complex '*' and '+', are implemented as circular
 *		BRANCH structures using BACK.  Simple cases (one character
 *		per match) are implemented with STAR and PLUS for speed
 *		and to minimize recursive plunges.
 *
 * OPEN,CLOSE	...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
# define OP(p)		(*(p))
# define NEXT(p)	(((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
# define OPERAND(p)	((p) + 3)

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */

# ifndef DOUBLEBYTE
# define MAGIC	0234
# else
# define MAGIC	19
# endif /* #ifndef DOUBLEBYTE */

/*
 * Utility definitions.
 */
#ifndef CHARBITS
# define UCHARAT(p)	((i4)*(unsigned char *)(p))
#else
# define UCHARAT(p)	((i4)*(p)&CHARBITS)
#endif

# define _FAIL(m)	{ _error(m); return(NULL); }

# define ISMULT(c)	((c) == '*' || (c) == '+' || (c) == '?')
# define META	"^$.[()|?+*\\"

/*
 * Flags to be passed up and down.
 */
# define HASWIDTH	01	/* Known never to match null string. */
# define SIMPLE		02	/* Simple enough to be STAR/PLUS operand. */
# define SPSTART		04	/* Starts with * or +. */
# define WORST		0	/* Worst case. */

/*
 * Global work variables for REcompile().
 */
static char *regparse;		/* Input-scan pointer. */
static i4  regnpar;		/* () count. */
static char regdummy;
static char *regcode;		/* Code-emit pointer; &regdummy = don't. */
static i4 regsize;		/* Code size. */

# undef reg

/*
 * Forward declarations for REcompile()'s friends.
 */
static char *reg(i4 paren, i4  *flagp);
static char *regbranch(i4 *flagp);
static char *regpiece(i4 *flagp);
static char *regatom(i4 *flagp);
static char *regnode(char op);
static char *regnext(register char *p);
static void regc( char * );
static void reginsert(char op, char *opnd);
static void regtail(char *p, char *val);
static void regoptail(char *p, char *val);

static i4  my_strcspn(char *s1, char *s2);


/*
 * _error() - error handling function
 */
void
_error(char *s)
{
	SIfprintf(stderr, "RE: %s\n", s);
	PCexit( FAIL );
}

/*
 * REcompile - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled RE_EXP.
 */
STATUS
REcompile( char *exp, RE_EXP **re_exp, i4  mem_tag )
{
	register RE_EXP *r;
	register char *scan;
	register char *longest;
	register i4  len;
	i4 flags;
	u_char magic = MAGIC;

	if (exp == NULL)
	{
	    _error("NULL argument");
	    return (FAIL);
	}

	/* First pass: determine size, legality. */
	regparse = exp;
	regnpar = 1;
	regsize = 0L;
	regcode = &regdummy;
	regc( (char *) &magic );
	if (reg(0, &flags) == NULL)
		return( FAIL );

	/* Small enough for pointer-storage convention? */
	if (regsize >= 32767L)		/* Probably could be 65535L. */
	{
	    _error("regular expression too big");
	    return (FAIL);
	}

	/* Allocate space. */
	r = (RE_EXP *) MEreqmem( mem_tag, sizeof(RE_EXP) + (unsigned) regsize,
		FALSE, NULL);
	if (r == NULL)
	{
	    _error("out of space");
	    return (FAIL);
	}

	/* Second pass: emit code. */
	regparse = exp;
	regnpar = 1;
	regcode = r->program;
	regc( (char *) &magic );
	if (reg(0, &flags) == NULL)
		return( FAIL );

	/* Dig out information for optimizations. */
	r->regstart = '\0';	/* Worst-case defaults. */
	r->reganch = 0;
	r->regmust = NULL;
	r->regmlen = 0;
	scan = r->program+1;			/* First BRANCH. */
	if (OP(regnext(scan)) == END) {		/* Only one top-level choice. */
		scan = OPERAND(scan);

		/* Starting-point info. */
		if (OP(scan) == EXACTLY)
			r->regstart = *OPERAND(scan);
		else if (OP(scan) == BOL)
			r->reganch++;

		/*
		 * If there's something expensive in the r.e., find the
		 * longest literal string that must appear and make it the
		 * regmust.  Resolve ties in favor of later strings, since
		 * the regstart check works with the beginning of the r.e.
		 * and avoiding duplication strengthens checking.  Not a
		 * strong reason, but sufficient in the absence of others.
		 */
		if (flags&SPSTART) {
			longest = NULL;
			len = 0;
			for (; scan != NULL; scan = regnext(scan))
				if (OP(scan) == EXACTLY && STlength(OPERAND(scan)) >= len) {
					longest = OPERAND(scan);
					len = STlength(OPERAND(scan));
				}
			r->regmust = longest;
			r->regmlen = len;
		}
	}
	*re_exp = r;
	return( OK );
}

/*
 * reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoiding.
 */
static char *
reg(i4 paren, i4  *flagp)
{
	register char *ret;
	register char *br;
	register char *ender;
	register i4  parno;
	i4 flags;

	*flagp = HASWIDTH;	/* Tentatively. */

	/* Make an OPEN node, if parenthesized. */
	if (paren) {
		if (regnpar >= NSUBEXP)
			_FAIL("too many ()");
		parno = regnpar;
		regnpar++;
		ret = regnode(OPEN+parno);
	} else
		ret = NULL;

	/* Pick up the branches, linking them together. */
	br = regbranch(&flags);
	if (br == NULL)
		return(NULL);
	if (ret != NULL)
		regtail(ret, br);	/* OPEN -> first. */
	else
		ret = br;
	if (!(flags&HASWIDTH))
		*flagp &= ~HASWIDTH;
	*flagp |= flags&SPSTART;
	while (*regparse == '|') {
		CMnext( regparse );
		br = regbranch(&flags);
		if (br == NULL)
			return(NULL);
		regtail(ret, br);	/* BRANCH -> BRANCH. */
		if (!(flags&HASWIDTH))
			*flagp &= ~HASWIDTH;
		*flagp |= flags&SPSTART;
	}

	/* Make a closing node, and hook it on the end. */
	ender = regnode((paren) ? CLOSE+parno : END);	
	regtail(ret, ender);

	/* Hook the tails of the branches to the closing node. */
	for (br = ret; br != NULL; br = regnext(br))
		regoptail(br, ender);

	/* Check for proper termination. */
	if (paren && *regparse != ')') {
		_FAIL("unmatched ()");
	}
	if( paren )
		CMnext( regparse );
	if (!paren && *regparse != '\0') {
		if (*regparse == ')') {
			_FAIL("unmatched ()");
		} else
			_FAIL("junk on end");	/* "Can't happen". */
		/* NOTREACHED */
	}

	return(ret);
}

/*
 * regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char *
regbranch(i4 *flagp)
{
	register char *ret;
	register char *chain;
	register char *latest;
	i4 flags;

	*flagp = WORST;		/* Tentatively. */

	ret = regnode(BRANCH);
	chain = NULL;
	while (*regparse != '\0' && *regparse != '|' && *regparse != ')') {
		latest = regpiece(&flags);
		if (latest == NULL)
			return(NULL);
		*flagp |= flags&HASWIDTH;
		if (chain == NULL)	/* First piece. */
			*flagp |= flags&SPSTART;
		else
			regtail(chain, latest);
		chain = latest;
	}
	if (chain == NULL)	/* Loop ran zero times. */
		(void) regnode(NOTHING);

	return(ret);
}

/*
 * regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char *
regpiece(i4 *flagp)
{
	register char *ret;
	register char op;
	register char *next;
	i4 flags;

	ret = regatom(&flags);
	if (ret == NULL)
		return(NULL);

	op = *regparse;
	if (!ISMULT(op)) {
		*flagp = flags;
		return(ret);
	}

	if (!(flags&HASWIDTH) && op != '?')
		_FAIL("*+ operand could be empty");
	*flagp = (op != '+') ? (WORST|SPSTART) : (WORST|HASWIDTH);

	if (op == '*' && (flags&SIMPLE))
		reginsert(STAR, ret);
	else if (op == '*') {
		/* Emit x* as (x&|), where & means "self". */
		reginsert(BRANCH, ret);			/* Either x */
		regoptail(ret, regnode(BACK));		/* and loop */
		regoptail(ret, ret);			/* back */
		regtail(ret, regnode(BRANCH));		/* or */
		regtail(ret, regnode(NOTHING));		/* null. */
	} else if (op == '+' && (flags&SIMPLE))
		reginsert(PLUS, ret);
	else if (op == '+') {
		/* Emit x+ as x(&|), where & means "self". */
		next = regnode(BRANCH);			/* Either */
		regtail(ret, next);
		regtail(regnode(BACK), ret);		/* loop back */
		regtail(next, regnode(BRANCH));		/* or */
		regtail(ret, regnode(NOTHING));		/* null. */
	} else if (op == '?') {
		/* Emit x? as (x|) */
		reginsert(BRANCH, ret);			/* Either x */
		regtail(ret, regnode(BRANCH));		/* or */
		next = regnode(NOTHING);		/* null. */
		regtail(ret, next);
		regoptail(ret, next);
	}
	CMnext( regparse );
	if (ISMULT(*regparse))
		_FAIL("nested *?+");

	return(ret);
}

/*
 * regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static char *
regatom(i4 *flagp)
{
	register char *ret;
	i4 flags;
	char null_byte = '\0';

	*flagp = WORST;		/* Tentatively. */

	switch (*regparse) {
	case '^':
		CMnext( regparse );
		ret = regnode(BOL);
		break;
	case '$':
		CMnext( regparse );
		ret = regnode(EOL);
		break;
	case '.':
		CMnext( regparse );
		ret = regnode(ANY);
		*flagp |= HASWIDTH|SIMPLE;
		break;
	case '[': {
			char *range_start = NULL;
			bool double_start;
			u_i2 first_u2, last_u2;
			u_char first_u1, last_u1;

			CMnext( regparse );
			if (*regparse == '^') {	/* Complement of range. */
				ret = regnode(ANYBUT);
				CMnext( regparse );
			} else
				ret = regnode(ANYOF);
			if (*regparse == ']' || *regparse == '-') {
				regc( regparse );
				CMnext( regparse );
			}
			while (*regparse != '\0' && *regparse != ']') {
				if (*regparse == '-') {
					char range_op = '-';

					CMnext( regparse );
					if( *regparse == ']' ||
						*regparse == '\0'
					)
						regc( &range_op );
					else {
						char *tmp;
						bool invalid = FALSE;
						bool double_end;

						if( range_start == NULL )
							invalid = TRUE;

						double_end =
							CMdbl1st( regparse );

						if( !invalid &&
							double_end	
							&& !double_start
						)
							invalid = TRUE;

						if( !invalid &&
							double_start
							&& !double_start
						)
							invalid = TRUE;

						if( !invalid &&
							CMcmpcase( range_start,
							regparse ) > 0
						)
							invalid = TRUE;

						if( double_start )
							_FAIL("don't know how to support character classes containing double-byte ranges");

						if( invalid )
							_FAIL("invalid [] range");
						/* no double-byte ranges! */
                        /*
                        ** Initialize the value for the end of the range.
                        */
                        last_u1 = UCHARAT(regparse);
						for (; first_u1 <= last_u1;
							first_u1++
						)
							regc( (char *)
								&first_u1 );

						CMnext( regparse );
					}
				} else {
					range_start = regparse;
					if( CMdbl1st( range_start ) )
					{
						double_start = TRUE;
						first_u2 = *(u_i2 *) range_start;
					}
					else
					{
						double_start = FALSE;
						first_u1 = UCHARAT(range_start);
					}
					regc( regparse );
					CMnext( regparse );
				}
			}
			regc( &null_byte );
			if (*regparse != ']')
				_FAIL("unmatched []");
			CMnext( regparse );
			*flagp |= HASWIDTH|SIMPLE;
		}
		break;
	case '(':
		CMnext( regparse );
		ret = reg(1, &flags);
		if (ret == NULL)
			return(NULL);
		*flagp |= flags&(HASWIDTH|SPSTART);
		break;
	case '\0':
	case '|':
	case ')':
		CMnext( regparse );
		_FAIL("internal urp");	/* Supposed to be caught earlier. */
		break;
	case '?':
	case '+':
	case '*':
		CMnext( regparse );
		_FAIL("?+* follows nothing");
		break;
	case '\\':
		CMnext( regparse );
		if (*regparse == '\0')
			_FAIL("trailing \\");
		ret = regnode(EXACTLY);
		regc( regparse );
		CMnext( regparse );
		regc( &null_byte );
		*flagp |= HASWIDTH|SIMPLE;
		break;
	default: {
			register i4  len;
			register char ender;

			len = my_strcspn(regparse, META);
			if (len <= 0)
				_FAIL("internal disaster");
			ender = *(regparse+len);
			if (len > 1 && ISMULT(ender))
				len--;	/* Back off clear of ?+* operand. */
			*flagp |= HASWIDTH;
			if (len == 1)
				*flagp |= SIMPLE;
			ret = regnode(EXACTLY);
			while (len > 0) {
				regc( regparse );
				CMbytedec( len, regparse );
				CMnext( regparse );
			}
			regc( &null_byte );
		}
		break;
	}

	return(ret);
}

/*
 * regnode - emit a node
 */
static char *			/* Location. */
regnode(char op)
{
	register char *ret;
	register char *ptr;

	ret = regcode;
	if (ret == &regdummy) {
		regsize += 3;
		return(ret);
	}

	ptr = ret;
	*ptr++ = op;
	*ptr++ = '\0';		/* Null "next" pointer. */
	*ptr++ = '\0';
	regcode = ptr;

	return(ret);
}

/*
 * regc - emit (if appropriate) a byte of code
 */
static void
regc( char *b )
{
	if (regcode != &regdummy) {
		CMcpychar( b, regcode );
		CMnext( regcode );
	} else
# ifndef DOUBLEBYTE
		regsize++;
# else
		CMbyteinc(regsize, b);
# endif /* #ifndef DOUBLEBYTE */
}

/*
 * reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void
reginsert(char op, char *opnd)
{
	register char *src;
	register char *dst;
	register char *place;

	if (regcode == &regdummy) {
		regsize += 3;
		return;
	}

	src = regcode;
	regcode += 3;
	dst = regcode;
	while (src > opnd)
		*--dst = *--src;

	place = opnd;		/* Op node, where operand used to be. */
	*place++ = op;
	*place++ = '\0';
	*place++ = '\0';
}

/*
 * regtail - set the next-pointer at the end of a node chain
 */
static void
regtail(char *p, char *val)
{
	register char *scan;
	register char *temp;
	register i4  offset;

	if (p == &regdummy)
		return;

	/* Find last node. */
	scan = p;
	for (;;) {
		temp = regnext(scan);
		if (temp == NULL)
			break;
		scan = temp;
	}

	if (OP(scan) == BACK)
		offset = scan - val;
	else
		offset = val - scan;
	*(scan+1) = (offset>>8)&0377;
	*(scan+2) = offset&0377;
}

/*
 * regoptail - regtail on operand of first argument; nop if operandless
 */
static void
regoptail(char *p, char *val)
{
	/* "Operandless" and "op != BRANCH" are synonymous in practice. */
	if (p == NULL || p == &regdummy || OP(p) != BRANCH)
		return;
	regtail(OPERAND(p), val);
}

/*
 * REexec and friends
 */

/*
 * Global work variables for REexec().
 */
static char *reginput;		/* String-input pointer. */
static char *regbol;		/* Beginning of input, for ^ check. */
static char **regstartp;	/* Pointer to startp array. */
static char **regendp;		/* Ditto for endp. */

/*
 * Forwards.
 */
static bool regtry(RE_EXP *prog, char *string);
static i4  regmatch(char *prog);
static i4  regrepeat(char *p);

#ifdef xDEBUG
GLOBALDEF i4  regnarrate = 0;
void regdump();
static char *regprop();
#endif

/*
 * REexec - match a RE_EXP against a string
 */
bool
REexec( RE_EXP *prog, char *string)
{
	register char *s;

	/* Be paranoid... */
	if (prog == NULL || string == NULL) {
		_error("NULL parameter");
		return( FALSE );
	}

	/* Check validity of program. */
	if (UCHARAT(prog->program) != MAGIC) {
		_error("corrupted program");
		return( FALSE );
	}

	/* If there is a "must appear" string, look for it. */
	if (prog->regmust != NULL) {
		s = string;
		while ((s = STchr(s, *prog->regmust)) != NULL) {
			if (STncmp( s, prog->regmust, prog->regmlen ) == 0
			)
				break;	/* Found it. */
			CMnext( s );
		}
		if (s == NULL)	/* Not present. */
			return( FALSE );
	}

	/* Mark beginning of line for ^ . */
	regbol = string;

	/* Simplest case:  anchored match need be tried only once. */
	if (prog->reganch)
		return(regtry(prog, string));

	/* Messy cases:  unanchored match. */
	s = string;
	if (prog->regstart != '\0')
		/* We know what char it must start with. */
		while ((s = STchr(s, prog->regstart)) != NULL) {
			if (regtry(prog, s))
				return( TRUE );
			CMnext( s );
		}
	else
		/* We don't -- general case. */
		while( TRUE ) {
			if (regtry(prog, s))
				return( TRUE );
			if( *s == '\0' )
				break;
			CMnext( s );	
		}
# ifndef DOUBLEBYTE
		CMnext( s );
# else
/*		CMnext( s ); */
# endif /* #ifndef DOUBLEBYTE */

	/* Failure. */
	return( FALSE );
}

/*
 * regtry - try match at specific point
 */
static bool			/* 0 failure, 1 success */
regtry(RE_EXP *prog, char *string)
{
	register i4  i;
	register char **sp;
	register char **ep;

	reginput = string;
	regstartp = prog->startp;
	regendp = prog->endp;

	sp = prog->startp;
	ep = prog->endp;
	for (i = NSUBEXP; i > 0; i--) {
		*sp = NULL;
# ifndef DOUBLEBYTE
		CMnext( *sp );
# else
		sp++;
# endif /* #ifndef DOUBLEBYTE */
		*ep = NULL;
# ifndef DOUBLEBYTE
		CMnext( *ep );
# else
		ep++;
# endif /* #ifndef DOUBLEBYTE */
	}
	if (regmatch(prog->program + 1)) {
		prog->startp[0] = string;
		prog->endp[0] = reginput;
		return( TRUE );
	} else
		return( FALSE );
}

/*
 * regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
static i4			/* 0 failure, 1 success */
regmatch(char *prog)
{
	register char *scan;	/* Current node. */
	char *next;		/* Next node. */

	scan = prog;
#ifdef xDEBUG
	if (scan != NULL && regnarrate)
		SIfprintf(stderr, "%s(\n", regprop(scan));
#endif
	while (scan != NULL) {
#ifdef xDEBUG
		if (regnarrate)
			SIfprintf(stderr, "%s...\n", regprop(scan));
#endif
		next = regnext(scan);

		switch (OP(scan)) {
		case BOL:
			if (reginput != regbol)
				return(0);
			break;
		case EOL:
			if (*reginput != '\0')
				return(0);
			break;
		case ANY:
			if (*reginput == '\0')
				return(0);
			CMnext( reginput );
			break;
		case EXACTLY: {
				register i4  len;
				register char *opnd;

				opnd = OPERAND(scan);
				/* Inline the first character, for speed. */
				if (*opnd != *reginput)
					return(0);
				len = STlength(opnd);
				if (len > 1 && STncmp( opnd, reginput,
					len ) != 0
				)
					return(0);
				reginput += len;
			}
			break;
		case ANYOF:
			if (*reginput == '\0' || STchr(OPERAND(scan),
				*reginput) == NULL
			)
				return(0);
			CMnext( reginput );
			break;
		case ANYBUT:
			if (*reginput == '\0' || STchr(OPERAND(scan),
				*reginput) != NULL
			)
				return(0);
			CMnext( reginput );
			break;
		case NOTHING:
			break;
		case BACK:
			break;
		case OPEN+1:
		case OPEN+2:
		case OPEN+3:
		case OPEN+4:
		case OPEN+5:
		case OPEN+6:
		case OPEN+7:
		case OPEN+8:
		case OPEN+9: {
				register i4  no;
				register char *save;

				no = OP(scan) - OPEN;
				save = reginput;

				if (regmatch(next)) {
					/*
					 * Don't set startp if some later
					 * invocation of the same parentheses
					 * already has.
					 */
					if (regstartp[no] == NULL)
						regstartp[no] = save;
					return(1);
				} else
					return(0);
			}
			break;
		case CLOSE+1:
		case CLOSE+2:
		case CLOSE+3:
		case CLOSE+4:
		case CLOSE+5:
		case CLOSE+6:
		case CLOSE+7:
		case CLOSE+8:
		case CLOSE+9: {
				register i4  no;
				register char *save;

				no = OP(scan) - CLOSE;
				save = reginput;

				if (regmatch(next)) {
					/*
					 * Don't set endp if some later
					 * invocation of the same parentheses
					 * already has.
					 */
					if (regendp[no] == NULL)
						regendp[no] = save;
					return(1);
				} else
					return(0);
			}
			break;
		case BRANCH: {
				register char *save;

				if (OP(next) != BRANCH)		/* No choice. */
					next = OPERAND(scan);	/* Avoid recursion. */
				else {
					do {
						save = reginput;
						if (regmatch(OPERAND(scan)))
							return(1);
						reginput = save;
						scan = regnext(scan);
					} while (scan != NULL && OP(scan) == BRANCH);
					return(0);
					/* NOTREACHED */
				}
			}
			break;
		case STAR:
		case PLUS: {
				register char nextch;
				register i4  no;
				register char *save;
				register i4  minx;

				/*
				 * Lookahead to avoid useless match attempts
				 * when we know what character comes next.
				 */
				nextch = '\0';
				if (OP(next) == EXACTLY)
					nextch = *OPERAND(next);
				minx = (OP(scan) == STAR) ? 0 : 1;
				save = reginput;
				no = regrepeat(OPERAND(scan));
				while (no >= minx) {
					/* If it could work, try it. */
					if (nextch == '\0' || *reginput == nextch)
						if (regmatch(next))
							return(1);
					/* Couldn't or didn't -- back up. */
					no--;
					reginput = save + no;
				}
				return(0);
			}
			break;
		case END:
			return(1);	/* Success! */
			break;
		default:
			_error("memory corruption");
			return(0);
			break;
		}

		scan = next;
	}

	/*
	 * We get here only if there's trouble -- normally "case END" is
	 * the terminating point.
	 */
	_error("corrupted pointers");
	return(0);
}

/*
 * regrepeat - repeatedly match something simple, report how many
 */
static i4
regrepeat(char *p)
{
	register i4  count = 0;
	register char *scan;
	register char *opnd;

	scan = reginput;
	opnd = OPERAND(p);
	switch (OP(p)) {
	case ANY:
		count = STlength(scan);
		scan += count;
		break;
	case EXACTLY:
		while (*opnd == *scan) {
			CMbyteinc( count, scan );
			CMnext( scan );
		}
		break;
	case ANYOF:
		while (*scan != '\0' && STchr(opnd, *scan) != NULL) {
			CMbyteinc( count, scan );
			CMnext( scan );
		}
		break;
	case ANYBUT:
		while (*scan != '\0' && STchr(opnd, *scan) == NULL) {
			CMbyteinc( count, scan );
			CMnext( scan );
		}
		break;
	default:		/* Oh dear.  Called inappropriately. */
		_error("internal foulup");
		count = 0;	/* Best compromise. */
		break;
	}
	reginput = scan;

	return(count);
}

/*
 * regnext - dig the "next" pointer out of a node
 */
static char *
regnext(register char *p)
{
	register i4  offset;

	if (p == &regdummy)
		return(NULL);

	offset = NEXT(p);
	if (offset == 0)
		return(NULL);

	if (OP(p) == BACK)
		return(p-offset);
	else
		return(p+offset);
}

#ifdef xDEBUG

static char *regprop();

/*
 * regdump - dump a RE_EXP onto stdout in vaguely comprehensible form
 */
void
regdump(r)
RE_EXP *r;
{
	register char *s;
	register char op = EXACTLY;	/* Arbitrary non-END op. */
	register char *next;


	s = r->program + 1;
	while (op != END) {	/* While that wasn't END last time... */
		op = OP(s);
		SIprintf("%2d%s", s-r->program, regprop(s)); /* Where, what. */
		next = regnext(s);
		if (next == NULL)		/* Next ptr. */
			SIprintf("(0)");
		else 
			SIprintf("(%d)", (s-r->program)+(next-s));
		s += 3;
		if (op == ANYOF || op == ANYBUT || op == EXACTLY) {
			/* Literal string, where present. */
			while (*s != '\0') {
				SIputc( *s, stdout );
				CMnext( s );
			}
			CMnext( s );
		}
		SIputc( '\n', stdout );
	}

	/* Header fields of naterest. */
	if (r->regstart != '\0')
		SIprintf("start `%c' ", r->regstart);
	if (r->reganch)
		SIprintf("anchored ");
	if (r->regmust != NULL)
		SIprintf("must have \"%s\"", r->regmust);
	SIprintf("\n");
}

/*
 * regprop - printable representation of opcode
 */
static char *
regprop(op)
char *op;
{
	register char *p;
	static char buf[50];

	(void) strcpy(buf, ":");

	switch (OP(op)) {
	case BOL:
		p = "BOL";
		break;
	case EOL:
		p = "EOL";
		break;
	case ANY:
		p = "ANY";
		break;
	case ANYOF:
		p = "ANYOF";
		break;
	case ANYBUT:
		p = "ANYBUT";
		break;
	case BRANCH:
		p = "BRANCH";
		break;
	case EXACTLY:
		p = "EXACTLY";
		break;
	case NOTHING:
		p = "NOTHING";
		break;
	case BACK:
		p = "BACK";
		break;
	case END:
		p = "END";
		break;
	case OPEN+1:
	case OPEN+2:
	case OPEN+3:
	case OPEN+4:
	case OPEN+5:
	case OPEN+6:
	case OPEN+7:
	case OPEN+8:
	case OPEN+9:
		STprintf(buf+STlength(buf), "OPEN%d", OP(op)-OPEN);
		p = NULL;
		break;
	case CLOSE+1:
	case CLOSE+2:
	case CLOSE+3:
	case CLOSE+4:
	case CLOSE+5:
	case CLOSE+6:
	case CLOSE+7:
	case CLOSE+8:
	case CLOSE+9:
		STprintf(buf+STlength(buf), "CLOSE%d", OP(op)-CLOSE);
		p = NULL;
		break;
	case STAR:
		p = "STAR";
		break;
	case PLUS:
		p = "PLUS";
		break;
	default:
		_error("corrupted opcode");
		break;
	}
	if (p != NULL)
		(void) STcat(buf, p);
	return(buf);
}
#endif

/*
 * The following is provided for those people who do not have strcspn() in
 * their C libraries.  They should get off their butts and do something
 * about it; at least one public-domain implementation of those (highly
 * useful) string routines has been published on Usenet.
 */

/*
 * my_strcspn - find length of initial segment of s1 consisting entirely
 * of characters not from s2
 *
 * Modified to return length in bytes (tyler)
 */

static i4
my_strcspn(char *s1, char *s2)
{
	register char *scan1;
	register char *scan2;
	register i4  count;

	count = 0;
	for (scan1 = s1; *scan1 != '\0'; CMnext( scan1 ) ) {
		for (scan2 = s2; *scan2 != '\0'; ) {	/* ++ moved down. */
			if (*scan1 == *scan2)
			{
				CMnext( scan2 );
				return(count);
			}
			CMnext( scan2 );
		}
		CMbyteinc( count, scan1 );	
	}
	return(count);
}

#ifdef OMIT_UNTIL_MADE_CL_COMPLIANT

/*
** REsub() needs to be made CM compliant before it can be uncommented and
** used. (tyler)
** 
** Specifically, this function needs to be changed to use CM calls. 
*/
void
REsub(prog, source, dest)
RE_EXP *prog;
char *source;
char *dest;
{
	register char *src;
	register char *dst;
	register char c;
	register i4  no;
	register i4  len;

	if (prog == NULL || source == NULL || dest == NULL) {
		_error("NULL parm to REsub");
		return;
	}
	if (UCHARAT(prog->program) != MAGIC) {
		_error("damaged RE_EXP fed to REsub");
		return;
	}

	src = source;
	dst = dest;
	while ((c = *src) != '\0') {
		CMnext( src );
		if (c == '&')
			no = 0;
		else if (c == '\\' && '0' <= *src && *src <= '9') {
			no = *src - '0';
			CMnext( src );
		} else
			no = -1;

		if (no < 0) {	/* Ordinary character. */
			if (c == '\\' && (*src == '\\' || *src == '&')) {
				c = *src;
				CMnext( src );
			}
			*dst = c;
			CMnext( dst );
		} else if (prog->startp[no] != NULL && prog->endp[no] != NULL) {
			len = prog->endp[no] - prog->startp[no];
			STncpy( prog->startp[no], dst, len);
			prog->startp[ no + len ] = EOS;
			dst += len;
			if (len != 0 && *(dst-1) == '\0') {
				/* STLcopy() hit NULL. */
				_error("damaged match string");
				return;
			}
		}
	}
	CMnext( src );
	*dst = '\0';
	CMnext( dst );
}

# endif /* OMIT_UNTIL_MADE_CL_COMPLIANT */
