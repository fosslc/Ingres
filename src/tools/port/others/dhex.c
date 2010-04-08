/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: dhex
**
** Description:
**      Utility to display the first and last 50000 hexadecimal octexts
**      of a file.
**
** Usage:
**      dhex [filename]
**
PROGRAM=dhex
NEEDLIBS=COMPATLIB
**
** History:
**      08-May-98 (fanra01)
**          Created.
**      08-Jun-98 (fanra01)
**          Updated not to use BOOL type.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# include <compat.h>
# include <cm.h>
# include <lo.h>
# include <me.h>
# include <si.h>
# include <st.h>

# define DUMP_SIZE          50000
# define HEX_TO_ASC(x)      (x > 9) ? (x - 9 + '@') : (x + '0')

i4              nLine = 0;  /* offset in file */

/*
** Name: DumpHex
**
** Description:
**      Reads nDumpSize of bytes from a file and displays its offset, ascii
**      representation of hex octets and any printable characters.
**
** Inputs:
**      fd          file descriptor
**      nDumpSize   number of octets to display
**      buffer      to use as storage
**
** Output:
**      none
**
** Return:
**      OK          completed successfully
**      !OK         read error
*/
STATUS
DumpHex (FILE* fd, i4  nDumpSize, char* pBuf)
{
    char            szHex[160] = { "\0" };
    char            szChr[80]  = { "\0" };
    char            szForm[80] = { "\0" };
    i4              nRead;
    STATUS          status;
    char*           pTmp;
    i4              i;
    i4              j;
    i4              k;
    bool            blPrint;

    if ((status = SIread (fd, nDumpSize, &nRead, pBuf)) == OK)
    {
        for (pTmp=pBuf, i=0, j=0, k=0; i < nDumpSize;)
        {
            /*
            ** Display 16 hex octets per line
            */
            if (k < 16)
            {
                blPrint = FALSE;
                szHex[j++] = HEX_TO_ASC(((*pTmp >> 4) & 0x0F));
                szHex[j++] = HEX_TO_ASC((*pTmp & 0x0F));
                szHex[j++] = ' ';
                szHex[j] = '\0';
                szChr[k++] =  (CMprint(pTmp)) ? *pTmp : '.';
                szChr[k] = '\0';
                i++;
                pTmp++;
            }
            else
            {
                blPrint = TRUE;
                SIprintf ("%08x   %s    %s\n", nLine, szHex, szChr);
                nLine+=k;
                j = k = 0;
                szHex[0] = '\0';
                szChr[0] = '\0';
                szForm[0] = '\0';
            }
        }
        if (blPrint == FALSE)
        {
            MEfill ((16 - k) * 3, ' ', szForm);

            SIprintf ("%08x   %s%s    %s\n", nLine, szHex, szForm, szChr);
        }
        SIprintf ("\n");
    }
    else
        SIfprintf (stderr, "Read error\n");

    return (status);
}

STATUS
main (i4 nArgc, char** pArgv)
{
    LOCATION        sLoc;
    LOINFORMATION   sLOinfo;
    char            szPath[MAX_LOC+1] = { "\0" };
    STATUS          status = OK;
    char*           pszPath;
    FILE*           fd;
    i4              nDumpSize = DUMP_SIZE;
    char*           pReadBuf;
    bool            blRewind = TRUE;
    i4              nFlags = LO_I_SIZE;
    if (nArgc == 2)
    {
        STcopy (pArgv[1], szPath);
        status = LOfroms (PATH & FILENAME, szPath, &sLoc);
        if (status == OK)
        {
            if ((status = LOinfo (&sLoc, &nFlags, &sLOinfo)) == OK)
            {
                if (sLOinfo.li_size < (DUMP_SIZE * 2))
                {
                    nDumpSize = sLOinfo.li_size;
                    blRewind = FALSE;
                }
                pReadBuf = MEreqmem (0, nDumpSize, TRUE, &status);

                if ((pReadBuf != NULL) &&
                    ((status = SIfopen (&sLoc, "r", SI_BIN, 0, &fd)) == OK))
                {
                    if ((status = DumpHex (fd, nDumpSize, pReadBuf)) == OK)
                    {
                        if (blRewind)
                        {
                            status = SIfseek (fd, ~nDumpSize+1, 2);
                            nLine  = sLOinfo.li_size - nDumpSize;
                            SIprintf ("-------------\n");
                            status = DumpHex (fd, nDumpSize, pReadBuf);
                        }
                    }
                    SIclose (fd);
                }
                else
                    SIfprintf (stderr, "Unable to open file %s\n", szPath);
            }
            else
                SIfprintf (stderr, "Cannot find file %s\n", szPath);
        }

    }
    else
    {
        SIfprintf (stderr, "Usage dhex [filename]\n");
    }
}
