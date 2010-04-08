#include	"telocal.h"

STATICDEF	i2	TEonRngSw = 0;	/* Flag to indicate if on Ring Menu. */
STATICDEF	i2	TElstMouse = 0;	/* 1 -> last "key" was left mouse click. */

/*
** Set flag that indicates whether currently on Ring Menu or not.
** Set mouse speed also.
*/

VOID
TEonRing(
bool sw)
{
	TEonRngSw = sw;
}

i2
TElastKeyWasMouse(void)
{
	return(TElstMouse);
}

i2
TEmousePos(		/* Get Mouse Position */
i4 *	lin,		/* ptr to place to put 0 relative line #	*/
i4 *	col)		/* ptr to place to put 0 relative column #	*/
{			/* returns 1 to indicate mouse present, 0 otherwise */
	return(0);
}
