/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADULCOL.H - Definitions for local collation support
**
** Description:
**	The file contains external definitions needed to use
**	local collation sequence support routines directly.
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
**      04-jan-1993 (stevet)
**          Added function prototypes.
**      19-jan-1993 (stevet)
**          Removed prototype for ad0_llike() for the time being because
**          it depends on ADF.H, which may not be included.
**      03-mar-1993 (stevet)
**          Found all files including aducol.h and added appropriate headers,
**          so added prototype for ad0_llike() back.
**	24-mar-93 (smc)
**	    Fixed aducolinit() function prototype declaration to
**	    correct the alloc function pointer parameter declaration.
**      14-jun-1993 (stevet)
**          Since aducolinit() is called by createdb so we need to have
**          proto and non-proto versions because FE will be linked without
**          proto.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Mar-2009 (kiria01) b121748
**	    Add support for end of string to be marked by pointer/len
**	    instead of just NULL terminated.
**	    Added end pointer estr.
**	    Macro adulstrinit() is joined by adulstrinitn() and
**	    adulstrinitp() to support initialisation using length
**	    or end pointer to terminate buffer.
**	11-May-2009 (kschendel) b122041
**	    Correct aducolinit alloc param to match MEreqmem's signature.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef	struct	_ADULTABLE	ADULTABLE;

/*
**  Defines of other constants.
*/

/*
**	This is the maximum length of a collation sequence name.
*/
# define	MAX_COLNAME	24

/*
**	This is the multiplier between native characters and default
**	weight values.
*/
# define	COL_MULTI	16

/*
**	Arguments to adugcmp
*/

# define	ADUL_BLANKPAD	1
# define	ADUL_SKIPBLANK	2

/*}
** Name: ADULTABLE - Collation sequence table
**
** Description:
**	This is the structure of a collation sequence description.
**	It is intented to be a position independant structure read
**	directly from a file.
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
**      02-Oct-90 (anton)
**          Added ADUL_FINAL to fix bug 31993
**      04-Mar-91 (stevet)
**          Changed testchr from char to u_char to fix collation problem.
*/
struct _ADULTABLE	{
    short	firstchar[256];
# define ADUL_TMULT	0x8000
# define ADUL_TDATA	0x7fff
# define ADUL_SKVAL	ADUL_TDATA
    short	nstate;
    short	magic;
# define ADUL_MAGIC	0x45ce
    struct ADUL_tstate {
	u_char	testchr;
	char	flags;
# define ADUL_LMULT	0x01
# define ADUL_FINAL	0x02
	short	match, nomatch;
    } statetab[1];
    /* VARIABLE LENGTH TABLE */
};

/*}
** Name: ADULcstate - string scan descriptor
**
** Description:
**	Descriptor which holds all state information needed when
**	transforming a string into local collation weight values.
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
**	04-Feb-2009 (kiria01)
**	    Optional end of buffer mark for use without EOS
*/
typedef struct _ADULcstate {
    ADULTABLE	*tbl;
    i2		holdstate;
    i2		nextstate;
    u_char	*nstr;
    u_char	*lstr;
    u_char	*estr;
} ADULcstate;

/*
** Name: adulstrinit	- prepare to convert a string to weight values
**
** Description:
**	Initialize a ADULcstate for a string to weight value convertion.
**
** Inputs:
**	t	Collation table
**	s	string to convert
**	p	pointer to end of string
**	n	buffer length
**
** Outputs:
**	d	pointer to ADULcstate to hold state of convertion
**
**	Returns:
**	    void
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
*/

# define adulstrinitp(t, s, p, d) ((d)->tbl = (t), (d)->estr = (p),\
    (d)->holdstate = (d)->nextstate = -1, (d)->nstr = (d)->lstr = (s))
# define adulstrinit(t, s, d) adulstrinitp(t, s, NULL, d) 
# define adulstrinitn(t, s, n, d) adulstrinitp(t, s, (s)+(n), d)  

/*
** Name: adulptr	- return pointer to current converted character
**
** Description:
**	Return pointer to currect character.  Should only be used to
**	determine if the end of a string has been reached durring
**	convertions to weight values.
**
** Inputs:
**	d	ADULcstate of string being converted
**
** Outputs:
**	none
**
**	Returns:
**	    pointer to character
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
*/

# define adulptr(d)	((d)->lstr)

/*
** Name: adulnext	- move to next weight value
**
** Description:
**	Advance to next weight value after haveing translated previous
**	values.  Should be followed by a call to adulptr to determine
**	end of string.
**
** Inputs:
**	d	ADULcstate of string being converted
**
** Outputs:
**	none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    advances to next weight value
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
*/
# define adulnext(d)  (((d)->nextstate = (d)->holdstate) == -1 ? \
			(((d)->lstr = (d)->nstr), 0) : \
			((d)->holdstate = -1))


/*
** Name: adulcmp	- Compare weight values of two strings
**
** Description:
**	Compare the next weight values of two strings
**
** Inputs:
**	ad	ADULcstate of first string being converted
**	bd	ADULcstate of second string being converted
**
** Outputs:
**	none
**
**	Returns:
**	    i4  indicating <,>, or = by sign of value
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    translates characters to weight values allowing adulnext
**	    to be possible.
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
*/

# define adulcmp(ad, bd)	(adultrans(ad) - adultrans(bd))

/*
** Name: adulspace	- check if next weight value is a space value
**
** Description:
**	test if next value is a space according the collation
**	sequence of the string being tested.
**
** Inputs:
**	d	ADULcstate of string being converted
**
** Outputs:
**	none
**
**	Returns:
**	    i4  indicating <,>, or = to space by sign of value
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jun-89 (anton)
**          Moved to ADU from CL
*/

# define adulspace(d)	(!adulccmp(d, (u_char *)" "))

/*
** Name: adul0widcol	- check if character has zero collation width
**
** Description:
**	test if next value has zero width  according the collation
**	sequence of the string being tested.
**
** Inputs:
**	t	Collation table
**	p	pointer to char
**
** Outputs:
**	none
**
**	Returns:
**	    i4  indicating TRUE or FALSE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Mar-2009 (kiria01) b121748
**          Added to identify zero collation width characters
*/

# define adul0widcol(t, d) ((p) && ((ADULTABLE *)(t))->firstchar[*p] == ADUL_SKVAL)

/*
** Name: adulisnull	- check if next character would be a null
**
** Description:
**	test if next value would be generated from a null character
**
** Inputs:
**	d	ADULcstate of string being converted
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE    is null
**	    FALSE   is not null
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-Jun-89 (anton)
**          Moved to ADU from CL
*/

# define    adulisnull(d)	((d)->lstr == '\0')

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN	STATUS	aducolinit(char		*col,
				   PTR		(*alloc)(
							 u_i2	tag,
							 SIZE_TYPE size,
							 bool	clear,
							 STATUS	*status
							),
				   ADULTABLE	**tbl,
				   CL_ERR_DESC	*syserr);

FUNC_EXTERN	i4	adultrans(ADULcstate  *d);

FUNC_EXTERN     i4      adulccmp(ADULcstate  *d,
				 u_char      *c);

FUNC_EXTERN	i4	adugcmp(ADULTABLE  *tbl,
			        i4         flags,
				u_char     *bsa, 
				u_char     *bsb,
				u_char     *esa, 
				u_char     *esb);

FUNC_EXTERN i4  ad0_3clexc_pm(ADULcstate  *str1,
			      u_char      *endstr1,
			      ADULcstate  *str2,
			      u_char      *endstr2,
			      bool        bpad,
			      bool        bignore);

FUNC_EXTERN DB_STATUS ad0_llike(ADF_CB      *adf_scb,
                                ADULcstate  *sst,
                                u_char      *ends,
                                ADULcstate  *pst,
                                u_char      *endp,
                                ADULcstate  *est,
                                bool        bignore,
                                i4          *rcmp);

