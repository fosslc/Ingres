/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: gvcl
**
** Description:
**      Header file containing definitions for the version informaion.
**      This file is included from the generated gv.h
**
** History:
**      26-Jan-2004 (fanra01)
**          SIR 111718
**          Created.
**      11-Feb-2004 (fanra01)
**          SIR 111718
**          Add prototype for GVmo_init.
**          Also renamed II_VERSION to ING_VERSION to make consistent with
**          ING_CONFIG which originally conflicted with II_CONFIG a string
**          define of that name.
**      01-Jun-2005 (fanra01)
**          SIR 114614
**          Add build level into the version structure.
**	25-Jul-2007 (drivi01)
**	    Added GVvista function to retrieve version of
**	    the OS and return true or false if OS version
**	    is Vista or not.
**	18-Sep-2007 (drivi01)
**	    Replace return type of GVvista with int.
**	20-Aug-2009 (drivi01)
**	    Define FUNC_EXTERN for GVshobj.
**      03-Sep-2009 (frima01) 122490
**          Add prototypes for GVcnf_term and GVcnf_init
*/

#ifndef GV_included
#define GV_included

# define    GV_M_MAJOR  0x0001
# define    GV_M_MINOR  0x0002
# define    GV_M_GENLVL 0x0004
# define    GV_M_BLDLVL 0x0008
# define    GV_M_BYTE   0x0010
# define    GV_M_HW     0x0020
# define    GV_M_OS     0x0040
# define    GV_M_BLDINC 0x0080

#define     GV_M_ALL    GV_M_MAJOR|GV_M_MINOR|GV_M_GENLVL| \
GV_M_BLDLVL|GV_M_BYTE|GV_M_HW|GV_M_OS|GV_M_BLDINC
    

/*
** Name: ING_VERSION
**
** Description:
**      Structure to hold encoded Ingres version.
**
**      major       Major version number
**      minor       Minor version or point release number
**      genlevel    Generated level
**      bldlevel    Build level
**      byte_type   Compile time character encoding byte type
**      hardware    Hardware platform
**      os          Operating system
**      bldinc      Build increment
**
** History:
**      26-Jan-2004 (fanra01)
**          Created.
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
*/
typedef struct _ing_ver ING_VERSION;

struct _ing_ver
{
    i4 major;
    i4 minor;
    i4 genlevel;
    i4 bldlevel;
    i4 byte_type;
    i4 hardware;
    i4 os;
    i4 bldinc;
};

FUNC_EXTERN STATUS GVmo_init( VOID );
FUNC_EXTERN VOID GVmo_term( VOID );
FUNC_EXTERN STATUS GVver( i4 flags, ING_VERSION* vout );
FUNC_EXTERN int GVvista(VOID);
FUNC_EXTERN VOID GVshobj(char **shPrefix);
FUNC_EXTERN STATUS GVdecode(i4 enc, char* str);

FUNC_EXTERN VOID GVcnf_term(VOID);
FUNC_EXTERN STATUS GVcnf_init(VOID);

FUNC_EXTERN STATUS GVgetverstr(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetenvstr(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetversion(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetpatchlvl(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgettcpportaddr(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetinstance(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetlanportaddr(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetconformance(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetlanguage(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetcharset(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetsystempath(i4, i4, PTR, i4, char *);
FUNC_EXTERN STATUS GVgetnetportaddr(i4, i4, PTR, i4, char *);

#endif /* GV_included */
