
/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

# include       <compat.h>
# include       <si.h>
# include       <lo.h>
# include       <cv.h>
# include       <st.h>
# include       <me.h>
# include       <er.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <uf.h>
# include       <ug.h>
# include       <adf.h>
# include       <afe.h>
# include       <abfcnsts.h>
# include       <fdesc.h>
# include       <abfrts.h>
# include       "keyparam.h"

/**
** Name:        abrtxio.c -   ABF Run-time System External File Procedure.
**
** Description:
**      Contains the routines that handles external file i/o.
**      Defines:
**
**      IIARofOpenFile()
**      IIARcfCloseFile()
**      IIARfrRewindFile()
**      IIARpfPositionFile()
**      IIARrfReadFile()
**      IIARwfWriteFile()
**      IIARffFlushFile()
**      IIARifInquireFile()
**
** History:
**      19-apr-93 (essi)
**              Abstracted from IIARfnd_row.
**      30-jun-93 (essi)
**              Made the definition of 4 variable GLOBALDEF. Not doing so
**              would require the library to be installed on VMS.
**              Also placed two missing return value. Forced requirement
**              of either record or offset for PositionFile. Fixed call to
**              LOdelete. Prevented ftell (in InquireFile) on SI_TXT files
**              due to VMS' SI restriction.
**      14-jul-93 (essi)
**              Revamped based on DaveL's suggestions. Also made it adhere
**              to ingres standards.
**      21-jul-93 (donc)
**              Replaced <dbms.h> with unbundled header files <gl.h>,
**              <sl.h>, and <iicommon.h>
**      23-aug-93 (essi)
**              Fixed bugs 53651 and 53655. Relaxed display of several
**              error messages (returned status only).  Fixed the bug
**              that allowed multiple filenames to be mapped to the
**              same user handle. Forced truncation of longer fields
**              during `text' reads.
**      11-nov-94 (athda01)
**              Added E_AR015A_ warning error message in InquireFile
**              when Offset or Record values are being requested for
**              a Binary type file and the Reclen is not yet known.
**      03-Feb-96 (fanra01)
**              Structure renamed to _iifileinfo as _fileinfo is a defined
**              structure in NT.
**	23-jul-98 (i4jo01)
**		Have to free memory used for readfile line buffer operation.
**		(b91439)
**      29-apr-1999 (hanch04)
**          Changed ftell to return offset.
**      03-may-1999 (hanch04)
**          SIputrec returns EOF on error.
**	03-jun-1999 (hanch04)
**	    Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**	14-feb-2000 (kitch01)
**		In IIARrfReadFile() remove the freeing and allocating of the line
**		buffer mentioned in bugs fixes 91439 and 92444. This buffer area
**		is allocated in the C code generated from the OSQ file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#define         MAX_4GLSTRING           4096
#define         PRIME_HASH_SIZE         37
#define         DEFAULT_REC_SIZE        80

typedef struct _iifileinfo
{
        char    filename[MAX_LOC+1];
        i4      filetype;
#define FI_TEXT_TYPE            1
#define FI_BINARY_TYPE          2
#define FI_STREAM_TYPE          3
        i4      filemode;
#define FI_READ_MODE            1
#define FI_CREATE_MODE          2
#define FI_APPEND_MODE          3
#define FI_UPDATE_MODE          4
        FILE    *fileptr;
        char    delimit[2];     /* Used for delimited text files */
        i4 handle;
        i4 offset;         /* Used for delayed opening of files */
        i4 recnum;         /* Used for delayed opening of files */
        i4      base;           /* Used for delayed opening of files */
        i4      reclen;         /* Record length of binary files */
        bool    seeked;         /* if TRUE a seek is needed before R/W */
} FI;

static          bool    realseek();
static          bool    filookup();
static          i4      compfunc();
static          i4      hashfunc();
static          PTR     prv();
static          PTR     tab;
DB_STATUS       afe_cvinto();
ADF_CB          *FEadfcb();


STATUS          IIUGiheIntHtabEnter();
STATUS          IIUGihiIntHtabInit();
STATUS          IIUGihdIntHtabDel();
STATUS          IIUGihfIntHtabFind();
i4              IIUGihsIntHtabScan();

/*{
** Name:        IIARofOpenFile()        Open external file.
**
** Description:
**      Open the specified file name with the specified mode and type.
**      All parameteres are mandatory.
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** Side effects:
**      To avoid performance degradation, the memory allocated for the hash
**      table is not released.
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
PTR
IIARofOpenFile ( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        DB_DATA_VALUE   *value;
        KEYWORD_PRM     prm_desc;
        FLD_VALUE       fld_value;
        char            filename[MAX_LOC+1];
        char            filemode[FE_MAXNAME+1];
        char            filetype[FE_MAXNAME+1];
        i4              recsize = 0;
        i4              ftype;
        i4              isbyref;
        static i4  genhandle = 0; /* 4GL generated file handle */
        i4         usrhandle;     /* User provided file handle */
        LOCATION        loc;
        STATUS          status;
        FILE            *fileptr;
        FI              *fi;
        FI              *fitmp;
        char            fmode[3];
        char            dma[2];
        register ADF_CB *cb;
        bool            scanflag;
        i4         hashtrash;
        DB_DATA_VALUE   ldbv;
        bool            seeked;
        i4              ifiletype;
        i4              ifilemode;

        /*
        ** Initialize the hash table on the first call to this routine.
        */

        if ( tab == NULL )
        {
                if ( IIUGihiIntHtabInit( PRIME_HASH_SIZE, NULL,
                        NULL, NULL, &tab) == FAIL )
                {
                        _VOID_ iiarUsrErr( E_AR0130_HashInit, 0);
                        return( prv(-1, prm, name) );
                }

                /*
                ** Allocate a single fi block for the scan use.
                */

                if( (fitmp = (FI *) MEreqmem( 0, sizeof(FI), FALSE, NULL )) ==
                        NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0137_AllocFail, 0 );
                        return( prv(-1, prm, name) );
                }
        }

        /* OpenFile requires at least two arguments */

        if ( prm->pr_argcnt < 2 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }

        filename[0] = EOS;
        filemode[0] = EOS;
        filetype[0] = EOS;
        dma[0] = EOS;
        dma[1] = EOS;
        value = NULL;

        seeked = FALSE;

        prm_desc.parent = name;
        prm_desc.pclass = F_AR0006_procedure;
        for ( fp = prm->pr_formals, ap = prm->pr_actuals,
            cnt = prm->pr_argcnt ; fp != NULL && --cnt >= 0 ; ++fp, ++ap )
        {
                char    prm_name[FE_MAXNAME+1];

                if ( *fp == NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0004_POSTOOSL,
                            1, ERget(F_AR0005_frame));
                        return( prv(-1, prm, name) );
                }
                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                _VOID_ STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
                _VOID_ CVlower(prm_name);

                if ( STequal(prm_name, ERx("filename")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)filename,
                            DB_CHR_TYPE, sizeof(filename)-1);
                        _VOID_ STtrmwhite(filename);
                        if ( STlength(filename) == 0 )
                        {
                                _VOID_ iiarUsrErr( E_AR013A_BadFileName,
                                    1, filename );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("filetype")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)filetype,
                            DB_CHR_TYPE, sizeof(filetype)-1);
                        _VOID_ STtrmwhite(filetype);
                        _VOID_ CVlower(filetype);
                        if ( STequal(filetype,ERx("text")) )
                        {
                                ifiletype = FI_TEXT_TYPE;
                                ftype = SI_TXT;
                        }
                        else if ( STequal(filetype,ERx("binary")) )
                        {
                                ifiletype = FI_BINARY_TYPE;
                                ftype = SI_RACC;
                        }
                        else if ( STequal(filetype,ERx("stream")) )
                        {
                                ifiletype = FI_STREAM_TYPE;
                                ftype = SI_RACC;
                        }
                        else
                        {
                                _VOID_ iiarUsrErr( E_AR0132_BadFileType,
                                    1, filetype );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("recordsize")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&recsize,
                            DB_INT_TYPE, sizeof(recsize));
                        if ( recsize <= 0 )
                        {
                                _VOID_ iiarUsrErr( E_AR014A_BadRecSize, 0 );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("filemode")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)filemode,
                            DB_CHR_TYPE, sizeof(filemode)-1);
                        _VOID_ STtrmwhite(filemode);
                        _VOID_ CVlower(filemode);
                        if ( STequal(filemode,ERx("read")) )
                        {
                                ifilemode = FI_READ_MODE;
                                STcopy(ERx("r"), fmode);
                        }
                        else if ( STequal(filemode,ERx("create")) )
                        {
                                ifilemode = FI_CREATE_MODE;
                                STcopy(ERx("w"), fmode);
                        }
                        else if ( STequal(filemode,ERx("append")) )
                        {
                                ifilemode = FI_APPEND_MODE;
                                STcopy(ERx("a"), fmode);
                        }
                        else if ( STequal(filemode,ERx("update")) )
                        {
                                ifilemode = FI_UPDATE_MODE;
                                STcopy(ERx("rw"), fmode);
                        }
                        else
                        {
                                _VOID_ iiarUsrErr( E_AR0133_BadFileMode,
                                    1, filemode );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("delimiter")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)dma,
                            DB_CHR_TYPE, sizeof(dma)-1);
                }
                else if ( STequal(prm_name, ERx("handle")) )
                {
                        cb = FEadfcb();
                        value = prm_desc.value; /* Keep for byref */
                        if ( !afe_tycoerce(cb,DB_INT_TYPE,value->db_datatype) )
                        {
                                _VOID_ iiarUsrErr( E_AR0087_Unmatched_datatype,
                                        3, prm_name, ERx("handle"),
                                        ERx("openfile") );
                                return( prv(-1, prm, name) );
                        }
                        isbyref = ( ap->abpvtype == -DB_DBV_TYPE );
                        if ( !isbyref )
                        {
                                _VOID_ iiarGetValue( &prm_desc, (PTR)&usrhandle,
                                    DB_INT_TYPE, sizeof(usrhandle));

                                /* User supplied handles must be positive. */

                                if ( usrhandle <= 0 )
                                {
                                        _VOID_ iiarUsrErr(
                                                E_AR0135_BadUsrHandle, 0 );
                                        return( prv(-1, prm, name) );
                                }
                        }
                }
                else
                {
                        _VOID_ iiarUsrErr( E_AR0134_BadParam, 1, prm_name );
                        return( prv(-1, prm, name) );
                }
        } /* end for */

        /*
        ** Fail if filemode is read, append or update and the file does
        ** not exist.
        */

        if ( (ifilemode == FI_READ_MODE ||
              ifilemode == FI_APPEND_MODE ||
              ifilemode == FI_UPDATE_MODE ) )
        {
                if ( IIUGvfVerifyFile(filename, UG_IsExistingFile,
                        FALSE, FALSE, UG_ERR_ERROR) != OK )
                {
                        return( prv(-1, prm, name) );
                }
        }

        /*
        ** Issue error if the file is already open with different handle.
        ** We need scan here since we are looking for a filename which may
        ** have been opened with a different handle.
        */

        scanflag = FALSE;       /* start the search */
        while ( IIUGihsIntHtabScan( tab, scanflag, &hashtrash, &fitmp ) > 0 )
        {
                if ( STcompare(fitmp->filename,filename) == 0 )
                {
                        _VOID_ iiarUsrErr( E_AR013C_AlreadyOpen, 2,
                            filename, (PTR)&fitmp->handle );
                        return( prv(-1, prm, name) );
                }
                scanflag = TRUE;        /* continue the search */
        }

        if( (fi = (FI *) MEreqmem( 0, sizeof(FI), FALSE, NULL )) == NULL )
        {
                _VOID_ iiarUsrErr( E_AR0137_AllocFail, 0 );
                return( prv(-1, prm, name) );
        }

        /* Provide defaults and fill the fi structure */

        if ( filemode[0] == EOS )
        {
                fi->filemode = FI_READ_MODE;
                STcopy(ERx("r"), fmode);
        }
        else
        {
                fi->filemode = ifilemode;
        }
        if ( filetype[0] == EOS )
        {
                fi->filetype = FI_TEXT_TYPE;
                ftype = SI_TXT;
        }
        else
        {
                fi->filetype = ifiletype;
        }
        if ( fi->filetype == FI_TEXT_TYPE && fi->filemode == FI_UPDATE_MODE )
        {
                _VOID_ iiarUsrErr( E_AR0153_TextUpdate, 0 );
                return( prv(-1, prm, name) );
        }

        _VOID_ STcopy(filename,fi->filename);
        fi->seeked = seeked;
        fi->delimit[0] = dma[0];
        fi->delimit[1] = dma[1];
        fi->reclen = recsize;

        /*
        ** Record size is a logical property in this file, but on VMS you
        ** cannot open a file with the physical record size of zero. We
        ** default to DEFAULT_REC_SIZE in this case.
        */

        if ( recsize == 0 )
        {
                recsize = DEFAULT_REC_SIZE;
        }

        if ( isbyref )
        {
                /*
                ** Generate a handle. To generate handles; we start with
                ** -1 and keep decrementing for every new request
                */

                genhandle--;
                fi->handle = genhandle;
        }
        else
        {
                /* Use the supplied handle. */

                fi->handle = usrhandle;
        }

        /*
        ** Prevent mapping of a user handle to multiple file names.
        */

        if ( filookup( fi->handle, &fitmp ) )
        {
                _VOID_ iiarUsrErr( E_AR013C_AlreadyOpen, 2,
                    fitmp->filename, (PTR)&fitmp->handle );
                return( prv(-1, prm, name) );
        }

        if ( LOfroms( PATH & FILENAME, filename, &loc ) == FAIL )
        {
                _VOID_ iiarUsrErr( E_AR013B_BadName, 1, filename );
                _VOID_ MEfree( (PTR)fi );
                return( prv(-1, prm, name) );
        }

        if ( (status = SIfopen( &loc, fmode, ftype, recsize, &fileptr )) != OK)
        {
               _VOID_ iiarUsrErr( E_AR0136_OpenFailed, 2,
                       filename, (PTR)&status );
                _VOID_ MEfree( (PTR)fi );
                return( prv(-1, prm, name) );
        }

        fi->fileptr = fileptr;

        if ( IIUGiheIntHtabEnter( tab, fi->handle, (PTR)fi ) == FAIL )
        {
                _VOID_ iiarUsrErr( E_AR0138_HashEntry, 0 );
                _VOID_ MEfree( (PTR)fi );
                return( prv(-1, prm, name) );
        }

        /* Fill the byref variable */

        if ( isbyref )
        {
                ldbv.db_datatype = DB_INT_TYPE;
                ldbv.db_length = sizeof(genhandle);
                ldbv.db_data = (PTR)&genhandle;
                _VOID_ afe_cvinto(FEadfcb(), &ldbv, value);
        }
        return( prv(0, prm, name) );
}


/*{
** Name:        IIARfrRewindFile        Rewind.
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
PTR
IIARfrRewindFile( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        i4         usrhandle;
        FI              *fi;
        STATUS          status;

        if ( prm->pr_argcnt == 0 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }
        prm_desc.parent = name;
        prm_desc.pclass = F_AR0006_procedure;
        for ( fp = prm->pr_formals, ap = prm->pr_actuals,
            cnt = prm->pr_argcnt ; fp != NULL && --cnt >= 0 ; ++fp, ++ap )
        {
                char    prm_name[FE_MAXNAME+1];

                if ( *fp == NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0004_POSTOOSL,
                            1, ERget(F_AR0005_frame));
                        return( prv(-1, prm, name) );
                }
                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                _VOID_ STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
                _VOID_ CVlower(prm_name);

                if ( STequal(prm_name, ERx("handle")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&usrhandle,
                            DB_INT_TYPE, sizeof(usrhandle));
                }
                else
                {
                        _VOID_ iiarUsrErr( E_AR0134_BadParam, 1, prm_name );
                        return( prv(-1, prm, name) );
                }
        } /* end for */

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        /* Cannot rewind APPEND and CREATE mode files */

        if ( fi->filemode == FI_APPEND_MODE || fi->filemode == FI_CREATE_MODE )
        {
                _VOID_ iiarUsrErr( E_AR014B_BadRewind, 0 );
                return( prv(-1, prm, name) );
        }

        if (( status = SIfseek ( fi->fileptr, (i4)0, SI_P_START )) != OK )
        {
                _VOID_ iiarUsrErr( E_AR0141_SeekFailed, 2, fi->filename,
                    (PTR)&status );
                return( prv(-1, prm, name) );
        }

        return( prv(0, prm, name) );
}

/*{
** Name:        IIARffFlushFile         Force data to disk.
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
PTR
IIARffFlushFile( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        i4         usrhandle;
        FI              *fi;
        STATUS          status;

        prm_desc.parent = name;
        prm_desc.pclass = F_AR0006_procedure;
        for ( fp = prm->pr_formals, ap = prm->pr_actuals,
            cnt = prm->pr_argcnt ; fp != NULL && --cnt >= 0 ; ++fp, ++ap )
        {
                char    prm_name[FE_MAXNAME+1];

                if ( *fp == NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0004_POSTOOSL,
                            1, ERget(F_AR0005_frame));
                        return( prv(-1, prm, name) );
                }
                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                _VOID_ STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
                _VOID_ CVlower(prm_name);

                if ( STequal(prm_name, ERx("handle")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&usrhandle,
                            DB_INT_TYPE, sizeof(usrhandle));
                }
                else
                {
                        _VOID_ iiarUsrErr( E_AR0134_BadParam, 1, prm_name );
                        return( prv(-1, prm, name) );
                }
        } /* end for */

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        if ( fi->filemode == FI_READ_MODE )
        {
                _VOID_ iiarUsrErr( E_AR0145_ReadFlush, 0 );
                return( prv(-1, prm, name) );
        }

        if (( status = SIflush ( fi->fileptr )) != OK )
        {
                _VOID_ iiarUsrErr( E_AR0142_FlushFailed, 2, fi->filename,
                    (PTR)&status );
                return( prv(-1, prm, name) );
        }

        return( prv(0, prm, name) );
}

/*{
** Name:        IIARifInquireFile()     Get info about external file.
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
**      11-nov-94 (athda01) Bug 62639.
**              Added E_AR015A_BinReclenNotProv error message to both
**              Offset and Record situations where fi->seeked is TRUE
**              indicating a delayed read/write/position in effect due
**              to Binary file record length not provided or derived yet.
**              In this situation the Offset and/or Recnum value being
**              returned are not absolute values as calculated from the
**              beginning of the file, but rather the the last values
**              that may have been realtive values passed with base=.
*/
PTR
IIARifInquireFile( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        char            filename[MAX_LOC+1];
        char            filemode[FE_MAXNAME+1];
        char            filetype[FE_MAXNAME+1];
        DB_DATA_VALUE   *fnvalue;
        DB_DATA_VALUE   *fmvalue;
        DB_DATA_VALUE   *ftvalue;
        DB_DATA_VALUE   *povalue;
        DB_DATA_VALUE   *rnvalue;
        FI              *fi;
        OFFSET_TYPE	offset;
        i4         usrhandle;
        STATUS          status;
        DB_DATA_VALUE   src;
        ADF_CB          *cb;

        if ( prm->pr_argcnt == 0 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }

        filename[0] = EOS;
        filemode[0] = EOS;
        filetype[0] = EOS;

        fnvalue = NULL;
        fmvalue = NULL;
        ftvalue = NULL;
        povalue = NULL;
        rnvalue = NULL;

        cb = FEadfcb();

        prm_desc.parent = name;
        prm_desc.pclass = F_AR0006_procedure;
        for ( fp = prm->pr_formals, ap = prm->pr_actuals,
            cnt = prm->pr_argcnt ; fp != NULL && --cnt >= 0 ; ++fp, ++ap )
        {
                char    prm_name[FE_MAXNAME+1];

                if ( *fp == NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0004_POSTOOSL,
                            1, ERget(F_AR0005_frame));
                        return( prv(-1, prm, name) );
                }
                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                _VOID_ STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
                _VOID_ CVlower(prm_name);

                if ( STequal(prm_name, ERx("handle")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&usrhandle,
                            DB_INT_TYPE, sizeof(usrhandle));
                }
                else if ( STequal(prm_name, ERx("filename")) )
                {
                        fnvalue = prm_desc.value;
                        if (!afe_tycoerce(cb,DB_CHR_TYPE,fnvalue->db_datatype))
                        {
                                _VOID_ iiarUsrErr( E_AR0087_Unmatched_datatype,
                                        3, prm_name, ERx("inquirefile"),
                                        ERx("inquirefile") );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("filetype")) )
                {
                        ftvalue = prm_desc.value;
                        if (!afe_tycoerce(cb,DB_CHR_TYPE,ftvalue->db_datatype))
                        {
                                _VOID_ iiarUsrErr( E_AR0087_Unmatched_datatype,
                                        3, prm_name, ERx("inquirefile"),
                                        ERx("inquirefile") );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("filemode")) )
                {
                        fmvalue = prm_desc.value;
                        if (!afe_tycoerce(cb,DB_CHR_TYPE,fmvalue->db_datatype))
                        {
                                _VOID_ iiarUsrErr( E_AR0087_Unmatched_datatype,
                                        3, prm_name, ERx("inquirefile"),
                                        ERx("inquirefile") );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("offset")) )
                {
                        povalue = prm_desc.value;
                        if (!afe_tycoerce(cb,DB_INT_TYPE,povalue->db_datatype))
                        {
                                _VOID_ iiarUsrErr( E_AR0087_Unmatched_datatype,
                                        3, prm_name, ERx("inquirefile"),
                                        ERx("inquirefile") );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("record")) )
                {
                        rnvalue = prm_desc.value;
                        if (!afe_tycoerce(cb,DB_INT_TYPE,rnvalue->db_datatype))
                        {
                                _VOID_ iiarUsrErr( E_AR0087_Unmatched_datatype,
                                        3, prm_name, ERx("inquirefile"),
                                        ERx("inquirefile") );
                                return( prv(-1, prm, name) );
                        }
                }
                else
                {
                        _VOID_ iiarUsrErr( E_AR0134_BadParam, 1, prm_name );
                        return( prv(-1, prm, name) );
                }
        } /* end for */

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        if ( fnvalue != NULL )
        {
                src.db_datatype = DB_CHR_TYPE;
                src.db_length = STlength(fi->filename);
                src.db_data = (PTR)fi->filename;
                afe_cvinto(FEadfcb(), &src, fnvalue);
        }
        if ( fmvalue != NULL )
        {
                src.db_datatype = DB_CHR_TYPE;
                switch ( fi->filemode )
                {
                        case    FI_READ_MODE:
                                src.db_length = STlength(ERx("read"));
                                src.db_data = ERx("read");
                                break;

                        case    FI_CREATE_MODE:
                                src.db_length = STlength(ERx("create"));
                                src.db_data = ERx("create");
                                break;

                        case    FI_APPEND_MODE:
                                src.db_length = STlength(ERx("append"));
                                src.db_data = ERx("append");
                                break;

                        case    FI_UPDATE_MODE:
                                src.db_length = STlength(ERx("update"));
                                src.db_data = ERx("update");
                                break;
                }
                afe_cvinto(FEadfcb(), &src, fmvalue);
        }
        if ( ftvalue != NULL )
        {
                src.db_datatype = DB_CHR_TYPE;
                switch ( fi->filetype )
                {
                        case    FI_TEXT_TYPE:
                                src.db_length = STlength(ERx("text"));
                                src.db_data = ERx("text");
                                break;

                        case    FI_BINARY_TYPE:
                                src.db_length = STlength(ERx("binary"));
                                src.db_data = ERx("binary");
                                break;

                        case    FI_STREAM_TYPE:
                                src.db_length = STlength(ERx("stream"));
                                src.db_data = ERx("stream");
                                break;
                }
                afe_cvinto(FEadfcb(), &src, ftvalue);
        }

        offset = -1; /* flag to prevent mutiple ftell */
        if ( povalue != NULL )
        {
                /* VMS SI cannot ftell the SI_TXT files */

                if ( fi->filetype == FI_TEXT_TYPE )
                {
                        _VOID_ iiarUsrErr( E_AR014E_PosText, 0 );
                        return( prv(-1, prm, name) );
                }

                if ( fi->seeked )
                {
                        src.db_datatype = DB_INT_TYPE;
                        src.db_length = sizeof(fi->offset);
                        src.db_data = (PTR)&fi->offset;
                        afe_cvinto(FEadfcb(), &src, povalue);

                        /* athda01 : 11-11-94                         */
                        /* b62639 : added msg below to inform program */
                        /* that binary file record length is not      */
                        /* available (zero) and so the absolute offset*/
                        /* or recordnumber cannot be derived. Returned*/
                        /* value may be relative to base. (ie -n)     */

                        _VOID_ iiarUsrErr(E_AR015A_BinReclenNotProv, /*athda01*/
                           1,  fi->filename);                        /*athda01*/
                }
                else
                {
                        if ( (offset = SIftell( fi->fileptr )) == -1 )
                        {
                                _VOID_ iiarUsrErr( E_AR0144_TellFailed, 0 );
                                return( prv(-1, prm, name) );
                        }
                        fi->offset = offset;
                        src.db_datatype = DB_INT_TYPE;
                        src.db_length = sizeof(fi->offset);
                        src.db_data = (PTR)&fi->offset;
                        afe_cvinto(FEadfcb(), &src, povalue);
                }
        }

        if ( rnvalue != NULL )
        {
                if ( fi->filetype != FI_BINARY_TYPE )
                {
                        _VOID_ iiarUsrErr( E_AR014C_RecText, 0 );
                        return( prv(-1, prm, name) );
                }
                if ( fi->seeked )
                {
                        src.db_datatype = DB_INT_TYPE;
                        src.db_length = sizeof(fi->recnum);
                        src.db_data = (PTR)&fi->recnum;
                        afe_cvinto(FEadfcb(), &src, rnvalue);

                        /* athda01 : 11-11-94                         */
                        /* b62639 : added msg below to inform program */
                        /* that binary file record length is not      */
                        /* available (zero) and so the absolute offset*/
                        /* or recordnumber cannot be derived. Returned*/
                        /* value may be relative to base. (ie -n)     */

                        _VOID_ iiarUsrErr(E_AR015A_BinReclenNotProv, /*athda01*/
                           1,  fi->filename);                        /*athda01*/
                }
                else
                {
                        if ( offset == -1 &&  (offset = SIftell( fi->fileptr))
                                == -1 )
                        {
                                _VOID_ iiarUsrErr( E_AR0144_TellFailed, 0 );
                                return( prv(-1, prm, name) );
                        }
                        if ( fi->reclen == 0 )
                        {
                                fi->recnum = 1;
                        }
                        else
                        {
                                fi->recnum = offset / fi->reclen + 1;
                        }
                        src.db_datatype = DB_INT_TYPE;
                        src.db_length = sizeof(fi->recnum);
                        src.db_data = (PTR)&fi->recnum;
                        afe_cvinto(FEadfcb(), &src, rnvalue);
                }
        }

        return( prv(0, prm, name) );
}


/*{
** Name:        IIARpfPositionFile
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
PTR
IIARpfPositionFile( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        i4         usrhandle;
        char            base[FE_MAXNAME+1];
        i4         offset;
        i4         curpos;
        i4         endpos;
        i4         recnum;
        bool            record;
        bool            off;
        i4              baseindex;
        DB_DATA_VALUE   *value;
        FI              *fi;
        STATUS          status;

        if ( prm->pr_argcnt < 2 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }

        base[0] = EOS;
        offset = 0;
        recnum = 0;
        off = FALSE;
        record = FALSE;

        prm_desc.parent = name;
        prm_desc.pclass = F_AR0006_procedure;
        for ( fp = prm->pr_formals, ap = prm->pr_actuals,
            cnt = prm->pr_argcnt ; fp != NULL && --cnt >= 0 ; ++fp, ++ap )
        {
                char    prm_name[FE_MAXNAME+1];

                if ( *fp == NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0004_POSTOOSL,
                            1, ERget(F_AR0005_frame));
                        return( prv(-1, prm, name) );
                }
                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                _VOID_ STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
                _VOID_ CVlower(prm_name);

                if ( STequal(prm_name, ERx("offset")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&offset,
                            DB_INT_TYPE, sizeof(offset) );
                        off = TRUE;
                }
                else if ( STequal(prm_name, ERx("handle")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&usrhandle,
                            DB_INT_TYPE, sizeof(usrhandle));
                }
                else if ( STequal(prm_name, ERx("base")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)base,
                            DB_CHR_TYPE, sizeof(base)-1);
                        _VOID_ STtrmwhite(base);
                        _VOID_ CVlower(base);
                        if ( STequal(base,ERx("start")) )
                        {
                                baseindex = SI_P_START;
                        }
                        else if ( STequal(base,ERx("end")) )
                        {
                                baseindex = SI_P_END;
                        }
                        else if ( STequal(base,ERx("current")) )
                        {
                                baseindex = SI_P_CURRENT;
                        }
                        else
                        {
                                _VOID_ iiarUsrErr( E_AR013D_BadValue, 2,
                                    prm_name, base );
                                return( prv(-1, prm, name) );
                        }
                }
                else if ( STequal(prm_name, ERx("record")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&recnum,
                            DB_INT_TYPE, sizeof(recnum) );

                        /* record number can be anything but zero */

                        if ( recnum == 0 )
                        {
                                _VOID_ iiarUsrErr( E_AR0150_BadRecNum, 0 );
                                return( prv(-1, prm, name) );
                        }
                        record = TRUE;
                }
                else
                {
                        _VOID_ iiarUsrErr( E_AR0134_BadParam, 1, prm_name );
                        return( prv(-1, prm, name) );
                }
        } /* end for */

        /* Provide default */
        if ( base[0] == EOS )
                baseindex = SI_P_START;

        if ( !record && !off )
        {
                _VOID_ iiarUsrErr( E_AR0154_RecOrOff, 0 );
                return( prv(-1, prm, name) );
        }
        if ( record && off )
        {
                _VOID_ iiarUsrErr( E_AR014F_MutualXor, 0 );
                return( prv(-1, prm, name) );
        }

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        if ( fi->filetype == FI_STREAM_TYPE && record )
        {
                _VOID_ iiarUsrErr( E_AR0155_NonRecFile, 0 );
                return( prv(-1, prm, name) );
        }
        if ( fi->filetype == FI_TEXT_TYPE )
        {
                _VOID_ iiarUsrErr( E_AR014E_PosText, 0 );
                return( prv(-1, prm, name) );
        }
        fi->offset = offset;
        fi->recnum = recnum;
        fi->base = baseindex;
        fi->seeked = FALSE;

        /*
        ** If this is a seek to a binary file for which we don't know the
        ** record size yet; save the offset and note that a seek is required
        ** prior to read/write.
        */

        if ( fi->filetype == FI_BINARY_TYPE && fi->reclen == 0 )
        {
                fi->seeked = TRUE;
                return( prv(0, prm, name) );
        }

        if ( !realseek( fi ) )
        {
                return( prv(-1, prm, name) );
        }

        return( prv(0, prm, name) );
}

/*{
** Name:        IIARcfCloseFile
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
PTR
IIARcfCloseFile( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        i4         usrhandle;
        char            disp[FE_MAXNAME+1];
        LOCATION        loc;
        FI              *fi;
        STATUS          status;

        if ( prm->pr_argcnt < 1 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }

        prm_desc.parent = name;
        prm_desc.pclass = F_AR0006_procedure;
        for ( fp = prm->pr_formals, ap = prm->pr_actuals,
            cnt = prm->pr_argcnt ; fp != NULL && --cnt >= 0 ; ++fp, ++ap )
        {
                char    prm_name[FE_MAXNAME+1];

                if ( *fp == NULL )
                {
                        _VOID_ iiarUsrErr( E_AR0004_POSTOOSL,
                            1, ERget(F_AR0005_frame));
                        return( prv(-1, prm, name) );
                }
                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                _VOID_ STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
                _VOID_ CVlower(prm_name);

                if ( STequal(prm_name, ERx("handle")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)&usrhandle,
                            DB_INT_TYPE, sizeof(usrhandle));
                }
                else if ( STequal(prm_name, ERx("disposition")) )
                {
                        _VOID_ iiarGetValue( &prm_desc, (PTR)disp,
                            DB_CHR_TYPE, sizeof(disp)-1);
                        _VOID_ STtrmwhite(disp);
                        _VOID_ CVlower(disp);
                        if (STcompare(disp,ERx("keep")) != 0 &&
                            STcompare(disp,ERx("delete")) != 0)
                        {
                                _VOID_ iiarUsrErr( E_AR013D_BadValue, 2,
                                    prm_name, disp );
                                return( prv(-1, prm, name) );
                        }
                }
                else
                {
                        _VOID_ iiarUsrErr( E_AR0134_BadParam, 1, prm_name );
                        return( prv(-1, prm, name) );
                }
        } /* end for */

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        if (( status = SIclose ( fi->fileptr )) != OK )
        {
                return( prv(-1, prm, name) );
        }

        /* Remove entry from hash table */
        if ( IIUGihdIntHtabDel( tab, fi->handle ) == FAIL )
        {
                _VOID_ iiarUsrErr( E_AR0143_HashDelete, 0 );
                return( prv(-1, prm, name) );
        }

        _VOID_ MEfree( (PTR)fi );

        if ( STequal(disp,ERx("delete")) )
        {
                _VOID_ LOfroms( PATH & FILENAME, fi->filename, &loc );
                _VOID_ LOdelete( &loc );
        }

        return( prv(0, prm, name) );
}

/*{
** Name:        IIARwfWriteFile         Write data item(s) to file.
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
**      26-jan-1994 (mgw) Bug #59131
**              Pass the errant parameter name to iiarUsrErr when reporting
**              error E_AR0134_BadParam.
**      16-mar-1994 (donc) Bug #60262
**              Place trailing \n in the proper string position and then
**              NULL terminate
*/
PTR
IIARwfWriteFile ( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        char            buf[MAX_4GLSTRING+1];
        char            field[MAX_4GLSTRING+1];
        i4         usrhandle;
        i4         offset;
        i4         size;
        i4              i;
        i4              elms;
        i4              slen;
        i4         retval;
        DB_DATA_VALUE   *value;
        DB_DATA_VALUE   **items;        /* keep itemlist in this array */
        FI              *fi;
        LOCATION        loc;
        STATUS          status;
        bool            mismatch;

        if ( prm->pr_argcnt < 2 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }

        fp = prm->pr_formals;
        ap = prm->pr_actuals;
        cnt = prm->pr_argcnt;

        /* Allocate memory for itemlist */

        if( (items = (DB_DATA_VALUE **) MEreqmem( (u_i4)0,
            cnt*sizeof(DB_DATA_VALUE *),
            FALSE, NULL )) == NULL )
        {
                _VOID_ iiarUsrErr( E_AR0137_AllocFail, 0 );
                return( prv(-1, prm, name) );
        }

        i = 0;
        elms = 0;
        size = 0;
        mismatch = FALSE;

        while ( --cnt >= 0 )
        {
                char    prm_name[FE_MAXNAME+1];

                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                if ( *fp == NULL )
                {
                        items[i] = (DB_DATA_VALUE *)ap->abpvvalue;
                        size += items[i]->db_length;
                        if (abs(items[i]->db_datatype) != DB_CHA_TYPE &&
                            abs(items[i]->db_datatype) != DB_VCH_TYPE &&
                            abs(items[i]->db_datatype) != DB_TXT_TYPE &&
                            abs(items[i]->db_datatype) != DB_CHR_TYPE )
                        {
                                mismatch = TRUE;
                        }
                        i++;
                }
                else
                {
                        _VOID_ STlcopy( *fp, prm_name,
                            (u_i4)sizeof(prm_name)-1 );
                        _VOID_ CVlower(prm_name);
                        if ( STequal(prm_name, ERx("handle")) )
                        {
                                _VOID_ iiarGetValue( &prm_desc,
                                    (PTR)&usrhandle,
                                    DB_INT_TYPE, sizeof(usrhandle));
                        }
                        else
                        {
                                _VOID_ iiarUsrErr( E_AR0134_BadParam, 1,
                                                   prm_name );
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                }
                fp++;
                ap++;
        }

        elms = i;
        buf[0] = EOS;

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ MEfree( (PTR)items );
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        /* Prevent writing to 'read' files. */

        if ( fi->filemode == FI_READ_MODE )
        {
                _VOID_ iiarUsrErr( E_AR013E_WriteToRead, 1, fi->filename);
                _VOID_ MEfree( (PTR)items );
                return( prv(-1, prm, name) );
        }


        /* Itemlist of text files should only be of string data types */

        if ( fi->filetype == FI_TEXT_TYPE && mismatch )
        {
                _VOID_ iiarUsrErr( E_AR0151_BadItemList, 0 );
                _VOID_ MEfree( (PTR)items );
                return( prv(-1, prm, name) );
        }

        if ( fi->filetype == FI_BINARY_TYPE )
        {
                /*
                ** Check to see if the given recordsize in openfile (if any)
                ** matches the actual size.
                */

                if ( fi->reclen != 0 && fi->reclen != size )
                {
                        _VOID_ iiarUsrErr( E_AR014D_SizeMismatch, 0 );
                        _VOID_ MEfree( (PTR)items );
                        return( prv(-1, prm, name) );
                }
                else
                {
                        fi->reclen = size;
                }

                /* Find out whether we need to do a seek (was delayed) */

                if ( fi->seeked )
                {
                        if ( !realseek( fi ) )
                        {
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                        fi->seeked = FALSE;
                }
                for (i=0; i < elms; i++)
                {
                        if ( SIwrite( (i4)items[i]->db_length,
                            (char *)items[i]->db_data,
                            &retval, fi->fileptr )  != OK )
                        {
                                _VOID_ iiarUsrErr( E_AR0152_WriteFailed, 1,
                                        fi->filename );
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                }
        }
        else if ( fi->filetype == FI_STREAM_TYPE )
        {
                for (i=0; i < elms; i++)
                {
                        if ( SIwrite( (i4)items[i]->db_length,
                            (char *)items[i]->db_data,
                            &retval, fi->fileptr )  != OK )
                        {
                                iiarUsrErr( E_AR0152_WriteFailed, 1,
                                        fi->filename );
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                }
        }
        else    /* text file */
        {
                if ( size > MAX_4GLSTRING )
                {
                        _VOID_ iiarUsrErr( E_AR0157_MaxRecSize, 0);
                        _VOID_ MEfree( (PTR)items );
                        return( prv(-1, prm, name) );
                }
                for (i=0; i < elms; i++)
                {
                        if ( AFE_NULLABLE_MACRO(items[i]->db_datatype) &&
                            AFE_ISNULL(items[i]) )
                        {
                                _VOID_ iiarUsrErr( E_AR0149_NullData, 0);
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                        prm_desc.value = items[i];
                        _VOID_ iiarGetValue( &prm_desc, (PTR)field,
                            DB_CHR_TYPE, sizeof(field)-1);
                        _VOID_ STcat(buf, field);
                        _VOID_ STcat(buf, fi->delimit);
                }
                slen = STlength(buf);
                buf[ slen ] =  '\n';
                buf[ slen + 1] =  EOS;
                if ( SIputrec( buf, fi->fileptr ) == EOF )
                {
                        iiarUsrErr( E_AR0152_WriteFailed, 1,
                                fi->filename );
                        _VOID_ MEfree( (PTR)items );
                        return( prv(-1, prm, name) );
                }
        }

        _VOID_ MEfree( (PTR)items );
        return( prv(0, prm, name) );
}

/*{
** Name:        IIARrfReadFile          Read data item(s) from file.
**
** Description:
**
**
** Inputs:
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**      proc    {ABRTSPRO *}  The procedure object.
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
**      21-apr-94 (connie) Bug #58274
**              Added error message E_AR012F_ReadOnAppend
**	23-jul-98 (i4jo01)
**		Make sure that we free previous line buffer before
**		trying to allocate a new one.
**	19-aug-98 (i4jo01)
**		Amend previous fix, need to check whether data area
**		actually allocated before MEfree call. (b92444).
**	14-feb-2000 (kitch01)
**	    Remove completely the freeing and allocating of the line buffer
**		described by the previous 2 changes. This buffer area is allocated
**		in the C code generated from the OSQ file. There is no need to free
**		and reallocate it in this code.
*/
PTR
IIARrfReadFile( prm, name, proc )
ABRTSPRM        *prm;
char            *name;
ABRTSPRO        *proc;
{
        register char           **fp;           /* pointer to formal name */
        register ABRTSPV        *ap;            /* pointer to actuals */
        register i4             cnt;

        KEYWORD_PRM     prm_desc;
        char            buf[MAX_4GLSTRING+1];
        char            oldbuf[MAX_4GLSTRING+1];
        char            field[MAX_4GLSTRING+1];
        char            dn[FE_MAXNAME+1];
        i4         usrhandle;
        i4         size;
        i4         offset;
        i4         curpos;
        i4         recnum;
        DB_DATA_VALUE   *value;
        DB_DATA_VALUE   *lastvalue;
        DB_DATA_VALUE   **items;
        i4         retval;
        i4              i;
        i4              elms;
        i4              len;
        FI              *fi;
        bool            mismatch;
        bool            binfile;
        STATUS          status;
        LOCATION        loc;

        if ( prm->pr_argcnt < 2 )
        {
                _VOID_ iiarUsrErr( E_AR0131_MissingParam, 0);
                return( prv(-1, prm, name) );
        }

        fp = prm->pr_formals;
        ap = prm->pr_actuals;
        cnt = prm->pr_argcnt;

        /* Allocate memory for the itemlist */

        if( (items = (DB_DATA_VALUE **) MEreqmem( (u_i4)0,
            cnt*sizeof(DB_DATA_VALUE *),
            FALSE, NULL )) == NULL )
        {
                _VOID_ iiarUsrErr( E_AR0137_AllocFail, 0 );
                return( prv(-1, prm, name) );
        }

        buf[0] = EOS;
        dn[0] = EOS;
        i=0;
        elms = 0;
        size = 0;
        mismatch = FALSE;

        while ( --cnt >= 0 )
        {
                char    prm_name[FE_MAXNAME+1];

                prm_desc.name = *fp;
                prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
                if ( *fp == NULL )
                {
                        items[i] = (DB_DATA_VALUE *)ap->abpvvalue;
                        size += items[i]->db_length;
                        if (abs(items[i]->db_datatype) != DB_CHA_TYPE &&
                            abs(items[i]->db_datatype) != DB_VCH_TYPE &&
                            abs(items[i]->db_datatype) != DB_TXT_TYPE &&
                            abs(items[i]->db_datatype) != DB_CHR_TYPE )
                        {
                                mismatch = TRUE;
                        }
                        i++;
                }
                else
                {
                        _VOID_ STlcopy( *fp, prm_name,
                            (u_i4)sizeof(prm_name)-1 );
                        _VOID_ CVlower(prm_name);
                        if ( STequal(prm_name, ERx("handle")) )
                        {
                                _VOID_ iiarGetValue( &prm_desc,
                                    (PTR)&usrhandle,
                                    DB_INT_TYPE, sizeof(usrhandle));
                        }
                }
                ap++;
                fp++;
        }

        elms = i;

        if ( !filookup( usrhandle, &fi ) )
        {
                _VOID_ MEfree( (PTR)items );
                _VOID_ iiarUsrErr( E_AR0140_LookupFailed, 1, (PTR)&usrhandle );
                return( prv(-1, prm, name) );
        }

        /* Prevent reading from 'create' and 'append' files. */

        if ( fi->filemode == FI_CREATE_MODE )
        {
                _VOID_ iiarUsrErr( E_AR013F_ReadOnWrite, 1, fi->filename);
                _VOID_ MEfree( (PTR)items );
                return( prv(-1, prm, name) );
        }

        if ( fi->filemode == FI_APPEND_MODE )
        {
                _VOID_ iiarUsrErr( E_AR012F_ReadOnAppend, 1, fi->filename);
                _VOID_ MEfree( (PTR)items );
                return( prv(-1, prm, name) );
        }

        /* Itemlist of text files should only be on string data types */

        if ( fi->filetype == FI_TEXT_TYPE && mismatch )
        {
                _VOID_ iiarUsrErr( E_AR0151_BadItemList, 0 );
                _VOID_ MEfree( (PTR)items );
                return( prv(-1, prm, name) );
        }

        if ( fi->filetype == FI_BINARY_TYPE )
        {
                /*
                ** Check to see if the given recordsize in openfile (if any)
                ** matches the actual size.
                */

                if ( fi->reclen != 0 && fi->reclen != size )
                {
                        _VOID_ iiarUsrErr( E_AR014D_SizeMismatch, 0 );
                        _VOID_ MEfree( (PTR)items );
                        return( prv(-1, prm, name) );
                }
                else
                {
                        fi->reclen = size;
                }

                /* Find out whether we need to do a seek */

                if ( fi->seeked )
                {
                        if ( !realseek( fi ) )
                        {
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                        fi->seeked = FALSE;
                }
                for (i=0; i < elms; i++)
                {
                        lastvalue = items[i];
                        status = SIread( fi->fileptr,
                                (i4)items[i]->db_length,
                                &retval, items[i]->db_data );
                        if ( status != OK && status != ENDFILE )
                        {
                                _VOID_ iiarUsrErr( E_AR0156_Exception, 0);
                                items[i] = lastvalue;
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                        if ( status  == ENDFILE )
                        {
                                items[i] = lastvalue;
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                }
                _VOID_ MEfree( (PTR)items );
                return( prv(0, prm, name) );
        }
        else if ( fi->filetype == FI_STREAM_TYPE )
        {
                for (i=0; i < elms; i++)
                {
                        lastvalue = items[i];
                        status = SIread( fi->fileptr,
                                (i4)items[i]->db_length,
                                &retval, items[i]->db_data );
                        if ( status != OK && status != ENDFILE )
                        {
                                _VOID_ iiarUsrErr( E_AR0156_Exception, 0);
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                        if ( status  == ENDFILE )
                        {
                                items[i] = lastvalue;
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }
                }
                _VOID_ MEfree( (PTR)items );
                return( prv(0, prm, name) );
        }
        else
        {
                /*
                ** For delimited text files; read the whole record and
                ** break it into pieces according to the delimiter.
                */

                char    *bufp;
                char    *fieldp;

                if (SIgetrec( buf, (i4)MAX_4GLSTRING, fi->fileptr ) != OK)
                {
                        /* Restore the last item in itemlist. */
                        _VOID_ STcopy(oldbuf, buf);
                        _VOID_ MEfree( (PTR)items );
                        return( prv(-1, prm, name) );
                }

                /* Save the last item in itemlist. */
                _VOID_ STcopy(buf, oldbuf);

                bufp = buf;
                field[0] = EOS;
                for (i=0; i < elms; i++)
                {
                        fieldp = field;
                        while ( *bufp != '\n' )
                        {
                                if ( *bufp == fi->delimit[0] )
                                {
                                        bufp++;
                                        *fieldp=EOS;
                                        break;
                                }
                                *fieldp = *bufp;
                                fieldp++;
                                bufp++;
                        }
                        if ( *bufp == '\n' )
                        {
                                *fieldp=EOS;
                        }

                        /*
                        ** If we have more than one items and the length of
                        ** field is equal to length of buffer; something
                        ** (most probably the delimiter) is wrong.
                        */

                        if ( elms > 1 && STlength(field) == (STlength(buf)-1) )
                        {
                                _VOID_ iiarUsrErr( E_AR0159_UnseenDelim, 1,
                                        fi->delimit);
                                _VOID_ MEfree( (PTR)items );
                                return( prv(-1, prm, name) );
                        }

                        /* Convert from C string to DB_DATA_VALUE. */

                        if ( abs(items[i]->db_datatype) == DB_VCH_TYPE ||
                            abs(items[i]->db_datatype) == DB_TXT_TYPE )
                        {
                                i4      alen;
                                DB_TEXT_STRING  *txt;

                                /*
                                ** Shorten the field if longer than the
                                ** defined size.
                                */
                                len = STlength( field );
                                alen = items[i]->db_length;
                                if ( AFE_NULLABLE_MACRO(items[i]->db_datatype) )
                                {
                                        alen--;
                                }
                                if ( len > (alen - DB_CNTSIZE ))
                                {
                                        len = alen - DB_CNTSIZE;
                                        *(field + len) = EOS;
                                }
                                /* Copy without padding */
                                txt = ( DB_TEXT_STRING * )items[i]->db_data;
                                txt->db_t_count = len;
                                _VOID_ MEcopy( (PTR)field, txt->db_t_count+1,
                                    (PTR)txt->db_t_text );
                        }
                        else
                        {
                                i4      alen;
                               /*
                                ** Shorten the field if longer than the
                                ** defined size.
                                */
                                len = STlength( field );
                                alen = items[i]->db_length;
                                if ( AFE_NULLABLE_MACRO(items[i]->db_datatype) )
                                {
                                        alen--;
                                }
                                if ( len > alen )
                                {
                                        len = alen;
                                }
                                *(field + len) = EOS;

                                /*
                                ** Copy and blank pad to the defined size.
                                */
                                _VOID_ STmove(field, ' ',items[i]->db_length,
                                    items[i]->db_data);
                                *(items[i]->db_data + items[i]->db_length)=EOS;
                        }
                }
        }
        _VOID_ MEfree( (PTR)items );
        return( prv(0, prm, name) );
}

/*{
** Name:        realseek()      Perform the actual seek.
**
** Description:
**      Do the actual seek. This piece of code overcomes the problems with
**      seeks by 1) Converting SI_P_CURRENT and SI_P_END to SI_P_START since
**      SI_P_CURRENT is not reliable. 2) Checking the boundary errors before
**      doing the seek. This is because seeking beyond EOF succeds ( on read-
**      open files; seeks to that position, on write-open files, if followed by
**      an actual write, it will extend the file ). Also seeking ahead of BOF
**      fails with the side effect of positioning to EOF.
**
**
** Inputs:
**      fi      {FI *}  The file info structure.
**
** Returns:
**      TRUE if seek succeeds, FALSE otherwise.
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
static
bool
realseek( fi )
FI      *fi;
{
        OFFSET_TYPE    curpos;
        OFFSET_TYPE    endpos;
        STATUS     status;

        /*
        ** Convert record number specification to byte offset. Here is how
        ** the record number plays its role in respect to BASE.
        **
        **      BASE    RECORD  RESULT
        **      ------- ------  ------------------------------------------
        **      start     <=0   Illegal
        **      start     > 0   Position to the RECORD number
        **      current   = 0   Illegal
        **      current   !=0   Position RECORD relative to current record
        **      end       >=0   Illegal
        **      end       < 0   Position RECORD before the last record in file
        **
        **
        ** Example:     Assume RECORDSIZE is 41
        **
        **              +----------+
        **      0       | record 1 |
        **      41      | record 2 |
        **      82      | record 3 |
        **      123     | record 4 |
        **      164     | record 5 | <- Current File Pointer (CFP)
        **      205     | record 6 |
        **      246     | record 7 |
        **      287     | record 8 |
        **              +----------+
        **
        **      BASE='start',   RECORD=1  Moves CFP to 'record 1' OFFSET 0
        **      BASE='start',   RECORD=-1 Causes error
        **      BASE='start',   RECORD=7  Moves CFP to 'record 7' OFFSET 246
        **
        **      BASE='current', RECORD=1  Moves CFP to 'record 6' OFFSET 205
        **      BASE='current', RECORD=-1 Moves CFP to 'record 4' OFFSET 123
        **
        **      BASE='end',     RECORD=1  Causes error
        **      BASE='end',     RECORD=-1 Moves CFP to 'record 7' OFFSET 246
        **      BASE='end',     RECORD=-5 Moves CFP to 'record 3' OFFSET 82
        */

        if ( fi->recnum != 0 )
        {
                switch( fi->base )
                {
                case SI_P_START:
                        if ( fi->recnum < 0 )
                        {
                                /* Trying to position ahead of BOF */

                                _VOID_ iiarUsrErr( E_AR0147_SeekAheadBOF, 0 );
                                return(FALSE);
                        }
                        fi->offset = (fi->recnum - 1) * fi->reclen;
                        break;
                case SI_P_CURRENT:
                        fi->offset = fi->recnum * fi->reclen;
                        break;
                case SI_P_END:
                        if ( fi->recnum > 0 )
                        {
                                /* Trying to position past EOF */

                                _VOID_ iiarUsrErr( E_AR0146_SeekPastEOF, 0 );
                                return(FALSE);
                        }
                        fi->offset = fi->recnum * fi->reclen;
                        break;
                }
                fi->recnum = 0;
        }

        switch( fi->base )
        {
        case SI_P_START:
                {
                        if ( fi->offset < 0 )
                        {
                                /* Trying to position ahead of BOF */

                                _VOID_ iiarUsrErr( E_AR0147_SeekAheadBOF, 0 );
                                return(FALSE);
                        }
                        curpos = SIftell( fi->fileptr );
                        _VOID_ SIfseek( fi->fileptr, (i4)0, SI_P_END );
                        endpos = SIftell( fi->fileptr );
                        if ( fi->offset > endpos )
                        {
                                /* Trying to position past EOF */

                                _VOID_ iiarUsrErr( E_AR0146_SeekPastEOF, 0 );
                                _VOID_ SIfseek ( fi->fileptr, curpos,
                                        SI_P_START );
                                return(FALSE);
                        }
                        break;
                }
        case SI_P_CURRENT:
                {
                        /* Convert 'current' to 'start' */

                        curpos = SIftell( fi->fileptr );
                        fi->offset += curpos;
                        fi->base = SI_P_START;
                        if ( fi->offset < 0 )
                        {
                                /* Trying to position ahead of BOF */

                                _VOID_ iiarUsrErr( E_AR0147_SeekAheadBOF, 0 );
                                return(FALSE);
                        }
                        _VOID_ SIfseek( fi->fileptr, (i4)0, SI_P_END );
                        endpos = SIftell( fi->fileptr );
                        if ( fi->offset > endpos )
                        {
                                /* Trying to position past EOF */

                                _VOID_ iiarUsrErr( E_AR0146_SeekPastEOF, 0 );
                                _VOID_ SIfseek ( fi->fileptr, curpos,
                                        SI_P_START );
                                return(FALSE);
                        }
                        break;
                }
        case SI_P_END:
                {
                        /* Convert 'end' to 'start' */

                        curpos = SIftell( fi->fileptr );
                        _VOID_ SIfseek ( fi->fileptr, (i4)0, SI_P_END );
                        endpos = SIftell( fi->fileptr );
                        fi->offset += endpos;
                        fi->base = SI_P_START;
                        if ( fi->offset < 0 )
                        {
                                /* Trying to position ahead of BOF */

                                _VOID_ iiarUsrErr( E_AR0147_SeekAheadBOF, 0 );
                                _VOID_ SIfseek ( fi->fileptr, curpos,
                                        SI_P_START );
                                return(FALSE);
                        }
                        if ( fi->offset > endpos )
                        {
                                /* Trying to position past EOF */

                                _VOID_ iiarUsrErr( E_AR0146_SeekPastEOF, 0 );
                                _VOID_ SIfseek ( fi->fileptr, curpos,
                                        SI_P_START );
                                return(FALSE);
                        }
                        break;
                }
        }

        /*
        ** Make sure the offset is at the begining of the record for
        ** binary files.
        */
        if ( fi->filetype == FI_BINARY_TYPE &&
             (fi->offset % fi->reclen) != 0 )
        {
                fi->offset = fi->offset - (fi->offset % fi->reclen);
        }

        if (( status = SIfseek ( fi->fileptr, fi->offset, fi->base )) != OK )
        {
                _VOID_ iiarUsrErr( E_AR0141_SeekFailed, 2, fi->filename,
                    (PTR)&status );
                return(FALSE);
        }
        fi->offset = 0;
        return(TRUE);
}

/*{
** Name:        filookup        Look for a handle in hash table.
**
** Description:
**      Lookup handle in the FI hash table.
**
** Inputs:
**      usrhandle {i4}   User supplied handle.
**      fi      {FI **}        Pointer to pointer to file info block.
**
** Returns:
**      TRUE for success, FALSE for failure.
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
static
bool
filookup( usrhandle, fi )
i4         usrhandle;
FI              **fi;
{
        if ( tab == NULL )
        {
                _VOID_ iiarUsrErr( E_AR0139_NoOpenFile, 0);
                return( FALSE );
        }
        if ( IIUGihfIntHtabFind( tab, usrhandle, (PTR *)fi ) > 0 )
        {
                return( FALSE );
        }
        return( TRUE );
}
/*{
** Name:        prv()           Return status for callproc to caller.
**
** Description:
**      Return status value to the user's callproc statement.
**
**
** Inputs:
**      retval  {i4}     The value to return. 0 = success, -1 = fail.
**      prm     {ABRTSPRM *}  The parameter structure for the call.
**      name    {char *}      Procedure name
**
** Returns:
**      NULL
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
static
PTR
prv(retval, prm, name)
i4         retval;
ABRTSPRM        *prm;
char            *name;
{
        DB_DATA_VALUE   retvalue;

        retvalue.db_data = (PTR)&retval;
        retvalue.db_length = sizeof(retval);
        retvalue.db_datatype = DB_INT_TYPE;
        IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );
        return ( NULL );
}
