/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

static	bool	is_prime();

/*
** Name:   IIUGnpNextPrime - next larger prime number
**
** Description:
**	returns smallest prime number >= its argument.
**
** Inputs:
**	num	number for "floor" of prime.
**
** Outputs:
**
**     return:
**	prime number
**
** Side effects:
**	none
**
** History:
**	4/87 (bobm)	written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4
IIUGnpNextPrime(num)
register i4  num;
{
	if (num <= 2)
		return (2);
	if ((num%2) == 0)
		++num;
	while (! is_prime(num))
		num += 2;
	return (num);
}

/*
** is_prime: only valid for i >= 2.
**
** simply check all factors <= the square root of the number, with
** a minor wrinkle:
**
** we split our checks into two separate chains which cover all
** numbers with no factors of 2 or 3, avoiding many of the non-
** prime factors.  factor1 winds up being all integers = 5 mod 6,
** factor2 all integers >= 7 which = 1 mod 6.  Anything = 0,2,3 or
** 4 mod 6 divides by 2 or 3.
**
** this gives a rather small number of redundant factor checks for
** reasonable sized arguments (say < 10000).  Only for extremely large
** numbers would the extra overhead justify a "smarter" algorithm.
**
** specifically:
**	5 11 17 23 29 35 41 47 53 59 65 71 77 83 89 95
**	7 13 19 25 31 37 43 49 55 61 67 73 79 85 91 97
**
**	are the factors < 100 which are checked.
**
**	25 35 49 55 65 77 85 91 95 are redundant (non-prime)
*/

static bool
is_prime(i)
register i4  i;
{
	register i4  factor1,factor2;

	if (i == 2 || i == 3)
		return(TRUE);

	if ((i%3) == 0 || (i%2) == 0)
		return(FALSE);

	factor1 = 5;
	factor2 = 7;
	while ((factor1 * factor1) <= i)
	{
		if ((i % factor1) == 0)
			return (FALSE);
		if ((i % factor2) == 0)
			return (FALSE);
		factor1 += 6;
		factor2 += 6;
	}

	return (TRUE);
}
