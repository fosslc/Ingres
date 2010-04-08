/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

# include    <compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include    <fe.h>
# include	<er.h>
# include    <cm.h>
# include    <gn.h>
# include    "gnspace.h"
# include    "gnint.h"

/**
** Name:    gnmknam.c - make a new name.
**			GN internal use only
**
** Defines
**		make_name()
*/

extern SPACE *List;
extern i4  Clen;

static bool Bumpdone = FALSE;
static char Bump[256];

static VOID set_bump();
static VOID trunc_name();

/*{
** Name:   make_name - make a new name.
**
** Description:
**
**    make a new name
**
** Inputs:
**
**    sp - SPACE structure for space name goes into.
**    uname - user name.
**
** Outputs:
**    gname - buffer for created name.
**
**    return:
**		OK
**		GNE_NAME
**		GNE_FULL
*/

make_name(sp,uname,gname)
register SPACE *sp;
char *uname;
char *gname;
{
	register i4  i,j;
	register i4  len;
	register char *s;
	PTR *dat;
	bool cont[NAMEBUFFER];
	i4 uend;
	i4 olen;
	i4 bumpfirst;

	STprintf(gname,ERx("%s%s"),sp->pfx,uname);

	if (legalize(sp,gname) != OK)
		return(GNE_NAME);

	/*
	** only compare up to uniqueness length.
	** If we DON'T find name in the hash table, it doesn't
	** collide - return OK
	*/
	Clen = sp->ulen;
	if (IIUGhfHtabFind(sp->htab,gname,&dat) != OK)
	{
		trunc_name(gname,sp->tlen);
		return(OK);
	}

	len = STlength(gname);

	/*
	** while fewer chars than the uniqueness length add numbers
	** and try again.  We won't need to call trunc_name
	** if one of these is acceptable.  If we wind up padding all
	** the way out, we will have padded entirely with 0's
	*/
	while (len <= sp->ulen)
	{
		j = 1;
		olen = len;
		for (i=0; len <= sp->ulen && i < 4; ++i,++len)
			j *= 10;
		for (i=0; i < j; ++i)
		{
			STprintf(gname+olen, ERx("%d"), i);
			if (IIUGhfHtabFind(sp->htab, gname, &dat) != OK)
				return(OK);
		}
		for ( ; olen < len; ++olen)
			gname[olen] = '0';
	}

	/*
	** for the purposes of name modification, we really want to
	** procede BACKWARDS in the string.  Set ubyt to the number
	** of bytes in the unique part of name, cont array set TRUE
	** for each byte which is a character continuation.  uend
	** gets the index for the rightmost unique character.  We
	** don't use the part of cont corresponding to the prefix -
	** it's simply convenient do index the cont array the way we
	** do the name.
	*/
	s = gname + sp->pfxlen;
	for (j=i=sp->pfxlen; (i += CMbytecnt(s)) <= sp->ulen; s = CMnext(s))
	{
		uend = j;
		cont[j] = FALSE;
		for (++j; j < i; ++j)
			cont[j] = TRUE;
	}
	cont[i] = FALSE;

	/*
	** the first "mangling" of the name assures a "0"
	** at the rightmost place.  If we have a lot more
	** truncation length than uniqueness length, slide
	** the non-prefix part of the name over and insert
	** "0"'s to preserve the name.	Otherwise overwrite
	** the rightmost unique character.
	*/
	if (sp->ulen > (sp->tlen - SHIFTLEN))
	{
		gname[uend] = '0';
		for (i=uend+1; cont[i]; ++i)
		{
			cont[i] = FALSE;
			gname[i] = '0';
		}
		uend = i-1;
		cont[i] = FALSE;
	}
	else
	{
		for (i = sp->pfxlen; i < sp->ulen; ++i)
		{
			cont[i] = FALSE;
			gname[i] = '0';
		}

		/* let's not GENERATE a name beginning with a digit */
		if (sp->pfxlen == 0)
			gname[0] = sp->crule == GN_UPPER ? 'A' : 'a';

		uend = sp->ulen - 1;
		cont[i] = FALSE;
		STcopy(uname,gname+sp->ulen);
	}

	/*
	** set up array for incrementing, if we've never done it
	*/
	if (!Bumpdone)
	{
		set_bump();
		Bumpdone = TRUE;
	}

	/*
	** keep bumping unique part of name base 36.  When we bump
	** the first character 36 times, we've cycled through all
	** possible names.
	*/
	bumpfirst = 0;
	while (bumpfirst < 36)
	{
		if (IIUGhfHtabFind(sp->htab,gname,&dat) != OK)
		{
			trunc_name(gname,sp->tlen);
			legalize(sp,gname);		/* force proper case */
			return(OK);
		}

		/*
		** base 36 modification of name.  In most instances this
		** loop executes once.	It repeats with a decrement of
		** i to handle a "carry" following the increment of a 'z'.
		**
		** loop also has to handle incrementing bumpfirst when
		** bumping the first character past the prefix.	 If the
		** prefix is length zero, we also make sure that we don't
		** generate a name beginning with a digit by bumping past
		** those cases (with bumpfirst count) before breaking.
		**
		** non-alphanumerics & multibyte characters may be present.
		** when we work back to them, we simply replace them with
		** 0's.	 At this point multibytes are indicated by the cont
		** array, so it is legitimate to decrement back through the
		** string.  If cont[i] is FALSE, gname[i] is single-byte.
		** when we replace multibyte characters we update the cont
		** status.  We just try the name again after modifying a
		** non-alphanumeric or multi-byte character.
		*/
		i = uend;
		for (;;)
		{
			s = gname+i;

			/* multibyte and non-alphanumeric */
			if (cont[i] || (!CMalpha(s) && !CMdigit(s)))
			{
				for ( ; cont[i]; i--)
				{
					cont[i] = FALSE;
					gname[i] = '0';
				}
				gname[i] = '0';
				break;
			}

			/* if 'z' we do a "carry" */
			if (*s == 'z' || *s == 'Z')
			{
				--i;
				if (i < sp->pfxlen)
				{
					if (sp->pfxlen == 0)
					{
						bumpfirst += 10;
						*s = 'a';
						break;
					}
					*s = '0';
					++bumpfirst;
					break;
				}
				*s = '0';
				continue;
			}

			/* bump character */
			*s = Bump[(unsigned) *s & 0xff];
			if (i == sp->pfxlen)
			{
				++bumpfirst;
				if (sp->pfxlen == 0)
				{
					for (; CMdigit(s); ++bumpfirst)
						*s = Bump[(unsigned) *s & 0xff];
				}
			}
			break;
		}
	}

	return (GNE_FULL);
}

/*
** truncate name to length, not chopping off part of a multibyte character
*/
static VOID
trunc_name(s,len)
register char *s;
register i4  len;
{
	register i4  i;

	for (i=0; *s != EOS && (i += CMbytecnt(s)) <= len; s = CMnext(s))
		;
	*s = EOS;
}

/*
** this looks silly as the devil, but it IS independent of
** character set.
*/
static VOID
set_bump()
{
	Bump[ (unsigned) '0' & 0xff ] = '1';
	Bump[ (unsigned) '1' & 0xff ] = '2';
	Bump[ (unsigned) '2' & 0xff ] = '3';
	Bump[ (unsigned) '3' & 0xff ] = '4';
	Bump[ (unsigned) '4' & 0xff ] = '5';
	Bump[ (unsigned) '5' & 0xff ] = '6';
	Bump[ (unsigned) '6' & 0xff ] = '7';
	Bump[ (unsigned) '7' & 0xff ] = '8';
	Bump[ (unsigned) '8' & 0xff ] = '9';
	Bump[ (unsigned) '9' & 0xff ] = 'a';
	Bump[ (unsigned) 'a' & 0xff ] = 'b';
	Bump[ (unsigned) 'b' & 0xff ] = 'c';
	Bump[ (unsigned) 'c' & 0xff ] = 'd';
	Bump[ (unsigned) 'd' & 0xff ] = 'e';
	Bump[ (unsigned) 'e' & 0xff ] = 'f';
	Bump[ (unsigned) 'f' & 0xff ] = 'g';
	Bump[ (unsigned) 'g' & 0xff ] = 'h';
	Bump[ (unsigned) 'h' & 0xff ] = 'i';
	Bump[ (unsigned) 'i' & 0xff ] = 'j';
	Bump[ (unsigned) 'j' & 0xff ] = 'k';
	Bump[ (unsigned) 'k' & 0xff ] = 'l';
	Bump[ (unsigned) 'l' & 0xff ] = 'm';
	Bump[ (unsigned) 'm' & 0xff ] = 'n';
	Bump[ (unsigned) 'n' & 0xff ] = 'o';
	Bump[ (unsigned) 'o' & 0xff ] = 'p';
	Bump[ (unsigned) 'p' & 0xff ] = 'q';
	Bump[ (unsigned) 'q' & 0xff ] = 'r';
	Bump[ (unsigned) 'r' & 0xff ] = 's';
	Bump[ (unsigned) 's' & 0xff ] = 't';
	Bump[ (unsigned) 't' & 0xff ] = 'u';
	Bump[ (unsigned) 'u' & 0xff ] = 'v';
	Bump[ (unsigned) 'v' & 0xff ] = 'w';
	Bump[ (unsigned) 'w' & 0xff ] = 'x';
	Bump[ (unsigned) 'x' & 0xff ] = 'y';
	Bump[ (unsigned) 'y' & 0xff ] = 'z';
	Bump[ (unsigned) 'A' & 0xff ] = 'B';
	Bump[ (unsigned) 'B' & 0xff ] = 'C';
	Bump[ (unsigned) 'C' & 0xff ] = 'D';
	Bump[ (unsigned) 'D' & 0xff ] = 'E';
	Bump[ (unsigned) 'E' & 0xff ] = 'F';
	Bump[ (unsigned) 'F' & 0xff ] = 'G';
	Bump[ (unsigned) 'G' & 0xff ] = 'H';
	Bump[ (unsigned) 'H' & 0xff ] = 'I';
	Bump[ (unsigned) 'I' & 0xff ] = 'J';
	Bump[ (unsigned) 'J' & 0xff ] = 'K';
	Bump[ (unsigned) 'K' & 0xff ] = 'L';
	Bump[ (unsigned) 'L' & 0xff ] = 'M';
	Bump[ (unsigned) 'M' & 0xff ] = 'N';
	Bump[ (unsigned) 'N' & 0xff ] = 'O';
	Bump[ (unsigned) 'O' & 0xff ] = 'P';
	Bump[ (unsigned) 'P' & 0xff ] = 'Q';
	Bump[ (unsigned) 'Q' & 0xff ] = 'R';
	Bump[ (unsigned) 'R' & 0xff ] = 'S';
	Bump[ (unsigned) 'S' & 0xff ] = 'T';
	Bump[ (unsigned) 'T' & 0xff ] = 'U';
	Bump[ (unsigned) 'U' & 0xff ] = 'V';
	Bump[ (unsigned) 'V' & 0xff ] = 'W';
	Bump[ (unsigned) 'W' & 0xff ] = 'X';
	Bump[ (unsigned) 'X' & 0xff ] = 'Y';
	Bump[ (unsigned) 'Y' & 0xff ] = 'Z';
}
