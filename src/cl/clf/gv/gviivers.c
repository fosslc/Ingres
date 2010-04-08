/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: gviivers
**
** Description:
**      Returns the encoded values of the version string.
**
** History:
**      26-Jan-2004 (fanra01)
**          SIR 111718
**          Created.
**      10-Feb-2004 (fanra01)
**          SIR 111718
**          Renamed II_VERSION to ING_VERSION
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**	23-Jul-2004 (schka24)
**	    Missing ii-ver extern, now compiles on unix.
**      26-Jul-2004 (lakvi01)
**          Backed-out the change for SIR 11270 to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**  01-Jun-2005 (fanra01)
**      SIR 114614
**      Add GVver_bldlevel function.
*/
# include <compat.h>
# include <gv.h>
# include <me.h>

GLOBALREF ING_VERSION ii_ver;

static VOID GVver_major( ING_VERSION* vout );
static VOID GVver_minor( ING_VERSION* vout );
static VOID GVver_genlevel( ING_VERSION* vout );
static VOID GVver_bldlevel( ING_VERSION* vout );
static VOID GVver_bytetype( ING_VERSION* vout );
static VOID GVver_hw( ING_VERSION* vout );
static VOID GVver_os( ING_VERSION* vout );
static VOID GVver_bldinc( ING_VERSION* vout );

static VOID (*vers_msk[9])( ING_VERSION* vout ) = {
     GVver_major,
     GVver_minor,
     GVver_genlevel,
     GVver_bldlevel,
     GVver_bytetype,
     GVver_hw,
     GVver_os,
     GVver_bldinc,
     NULL
};

static VOID
GVver_major( ING_VERSION* vout )
{
    vout->major = ii_ver.major;
}

static VOID
GVver_minor( ING_VERSION* vout )
{
    vout->minor = ii_ver.minor;
}

static VOID
GVver_genlevel( ING_VERSION* vout )
{
    vout->genlevel = ii_ver.genlevel;
}

static VOID
GVver_bldlevel( ING_VERSION* vout )
{
    vout->bldlevel = ii_ver.bldlevel;
}

static VOID
GVver_bytetype( ING_VERSION* vout )
{
    vout->byte_type = ii_ver.byte_type;
}

static VOID
GVver_hw( ING_VERSION* vout )
{
    vout->hardware = ii_ver.hardware;
}

static VOID
GVver_os( ING_VERSION* vout )
{
    vout->os = ii_ver.os;
}

static VOID
GVver_bldinc( ING_VERSION* vout )
{
    vout->bldinc = ii_ver.bldinc;
}

/*
** Name: GVver
**
** Description:
**      Function returns the specified components of the encoded version
**      string.
**
** Inputs:
**      flags       Flags specifying requested components of the version.
**
** Outputs:
**      vout        Pointer to an ING_VERSION structure to accept results.
**                  The structure is initialized on entry.
**
** Returns:
**      OK          Function successfully completed
**      FAIL        Error.
**
** History:
**      26-Jan-2004 (fanra01)
**          Created.
*/
STATUS
GVver( i4 flags, ING_VERSION* vout )
{
    STATUS  status = FAIL;
    i4      i;
    i4      j = flags;

    if (vout != NULL)
    {
        MEfill( sizeof(ING_VERSION), 0, vout );
        if (j & GV_M_ALL)
        {
            for(i=0; (i < BITS_IN(i4)) && (vers_msk[i] != NULL); i+=1)
            {
                if ((j>>i)&0x00000001)
                {
                    vers_msk[i]( vout );
                }
            }
            status = OK;
        }
    }
    return(status);
}

/*
** Name: GVdecode
**
** Description:
**      Returns the string equivalent of the hardware and OS encodings.
**
** Inputs:
**      enc         encoding
**
** Outputs:
**      str         string representaion
**
** Returns:
**      OK          command successful
**      FAIL
**
** History:
**      26-Jan-2004 (fanra01)
**          Created.
*/
STATUS
GVdecode( i4 enc, char* str )
{
    STATUS  status = OK;
    char    buf[sizeof(i4)+1];
    i4      i;
    i4      j;
    char*   p;
    
    MEfill( sizeof(i4)+1, 0, buf );
    for (i=0, j=0,p = (char*)&enc; i < sizeof(i4); p+=1, i+=1)
    {
        if (*p)
        {    
            j+=1;
        }
    }
    j = (j > 0) ? j-=1 : j;
    for(i=0; i < sizeof(i4); i+=1, j-=1)
    {
        buf[j] = (enc >> (i * BITS_IN(char)))&(char)0xFF;
    }
    STcopy( buf, str );
    return(status);
}
