/*
**Copyright (c) 2004 Ingres Corporation
*/
# include 	<stdlib.h>
#include <stdio.h>

#ifdef pyr
#include <sys/param.h>
#include <sys/mman.h>
#endif

main()
{
	if (caller())
	{
		printf("Dataspace executed incorrectly.\n");
		exit(1);
	}
	else
	{
		printf("Dataspace is executable.\n");
		exit(0);
	}
}

callee()
{
	int	n = 0;

	n += 1;
	n += callx();
	n += 64;

	return (n);
}

callx()
{
	return(128);
}

caller()
{
	int	callee();
	int	(*ptr)();

	register char	*buffer;
	register char	*b;
	register char	*c;
	register int	x;
	int		ret_val = 0;

	/* Setup */
	buffer = malloc(2048);
	b = buffer;
	c = (char *)callee;

	/* Fill in the buffer */
	while (c != (char *)caller)
		*b++ = *c++;

	/* Call the buffer */

#ifdef pyr
	if (mprotect((~(PAGSIZ-1) & (int)buffer), PAGSIZ, PROT_EXEC) != 0)
		return(1);
#endif

	ptr = (int(*)())buffer;
	x = (*ptr)();

	/* This should be 193 */
	if (x == 193)
		ret_val = 0;
	else
		ret_val = 1;

	return(ret_val);
}
