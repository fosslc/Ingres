/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsslogin.h>
#include <ddfcom.h>
#include <wsf.h>

/*
**
**  Name: wsslogin.c - Basic authentification
**
**  Description:
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      11-Sep-1998 (fanra01)
**          Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static unsigned char aTrans[] =
{
    62,  0,  0,  0, 63, /* +,-./ */
    52, 53, 54, 55, 56, /* 01234 */ 
    57, 58, 59, 60, 61, /* 56789 */
    0,  0,   0,  0,  0, /* :;<=> */
    0,  0,   0,  1,  2, /* ?@ABC */
    3,  4,   5,  6,  7, /* DEFGH */
    8,  9,  10, 11, 12, /* IJKLM */
    13, 14, 15, 16, 17, /* NOPQR */
    18, 19, 20, 21, 22, /* STUVW */
    23, 24, 25,  0,  0, /* XYZ[\ */
    0,  0,  0,   0, 26, /* ]^_`a */
    27, 28, 29, 30, 31, /* bcdef */
    32, 33, 34, 35, 36, /* ghijk */
    37, 38, 39, 40, 41, /* lmnop */
    42, 43, 44, 45, 46, /* qrstu */
    47, 48, 49, 50, 51  /* vwxyz */ 
};

#define TRANSCHAR(c) (aTrans[c - '+'])

/*
** Name: Decode64() - 
**
** Description:
**
** Inputs:
**	char*		In
**
** Outputs:
**	char*		Out
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
Decode64 (
	char* pIn, 
	char* pOut) 
{
    u_i4	which = 0;
    
    while (*pIn && (*pIn >= '+') && (*pIn <= 'z'))
    {
        switch (which)
        {
            case 0:
                /* put all 6 bits in the 6 high output bits
                */
                *pOut = (TRANSCHAR(*pIn++) << 2) & 0xFc;
                break;
            case 1:
                /* the 2 high bits fill up the 1st output byte
                */
                *pOut++ |= (TRANSCHAR(*pIn)) >> 4;
                /* take the remaining 4 bits and put them in the
                ** high 4 bits of the next output byte
                */
                *pOut = (TRANSCHAR(*pIn++) << 4) & 0xF0;
                break;
            case 2:
                /* take the middle 4 bits and put them in the low
                ** 4 bits of the output
                */
                *pOut++ |= (TRANSCHAR(*pIn) & 0x3C) >> 2;
                /* take the remaining 2 bits and put them in the
                ** high 2 bits of the the last output byte
                */
                *pOut = (TRANSCHAR(*pIn++) & 0x03) << 6;
                break;
            case 3:
                /* OR in the lower 6 bits of the input byte */
                *pOut++ |= TRANSCHAR(*pIn++);
                break;
        }
        which = (which + 1) % 4;
    }
    *pOut = EOS;
return (GSTAT_OK);
}

/*
** Name: AllocUserPass() - 
**
** Description:
**	Receive a crypted message and return the corresponding user and password
**
** Inputs:
**	char*		Message
**
** Outputs:
**	char**		User
**	char**		Password
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
AllocUserPass(
	char *message, 
	char **user, 
	char **password) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 length = 0;
	char *tmp = NULL;    
	char *pPass = NULL;
    length = STlength (STR_BASIC_AUTH);
	
    /* We only understand Basic authentication */
    if (STscompare(message, length, STR_BASIC_AUTH, length) == 0) 
	{
        /* Using Basic Authorization, so decode the credentials */
		message += length + 1;
		err = G_ME_REQ_MEM(0, tmp, char, 2*STlength(message));
		if (err == GSTAT_OK)
			err = Decode64 (message, tmp);

		if (err == GSTAT_OK) 
		{
			if (pPass = STindex (tmp, STR_COLON, 0))
				*pPass++ = EOS;
	
			err =  G_ST_ALLOC(*user, tmp);
			if (err == GSTAT_OK && pPass != NULL)
				err =  G_ST_ALLOC(*password, pPass);
		}

		MEfree(tmp);
	}
return(err);
}
