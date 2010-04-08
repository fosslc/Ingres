/*
**      Copyright (c) 1983, 1986 Ingres Corporation
**
**  Name: silogfil.c
**
**  History:
**
**      3-mar-1993 (fraser)
**          Changed SIlogInit: 1) allow the log file to include a full
**          path name (if no path is given, SIlogInit still opens a
**          file in the FILES directory); 2) eliminate use of NMf_open
**          (NMf_open should not be used).
**          Also changed "NULL" to "0" in MessageBox calls to eliminate
**          compiler warnings.  The first parameter to MessageBox is an
**          HWND, not a pointer.  NULL is defined as a i4 in compat.h.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	19-dec-2000 (somsa01)
**	    SIftell() now returns the offset.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

# include       <compat.h>
# include       <si.h>
# include       <cm.h>
# include       <cv.h>
# include       <lo.h>
# include       <nm.h>
# include       <pc.h>
# include       <st.h>
# include       <varargs.h>
# include       "silocal.h"


#define W4GL_MAX_LOG_SIZE 32768          /* jkronk,B5-w4gl00 Runtime stuff */

static  FILE*   W4glErrorLog = NULL;    /* jkronk, B5-w4gl00 Runtime stuff */
static  char*   W4glLogName  = NULL;    /* jkronk, B5-w4gl00 Runtime stuff */
static  char*   W4glErrorLogBuf = NULL; /* jkronk, B5-w4gl00 Runtime stuff */

/*
**  jkronk, B5-w4gl00 runtime stuff.
**
**  If the error log has not already been opened, then open it and honor the
**  append state requested by the user on the command line.
*/

VOID
SIlogInit(
char *  LogFile,
i2      AppendFlg)
{
        char        buf[MAX_LOC];
        STATUS      ret;
        LOCATION    loc;

        if( LogFile )
        {

            if ( W4glLogName == NULL )
                 W4glLogName =  STalloc( LogFile );

            if( !W4glErrorLog )
            {
                /*
                **  Make a LOCATION for the log file.
                */
                if (strchr(W4glLogName, '\\') == NULL)
                {
                    ret = NMloc(FILES, FILENAME, W4glLogName, &loc);
                }
                else
                {
                    strcpy(buf, W4glLogName);
                    ret = LOfroms(PATH & FILENAME, buf, &loc);
                }

                /*
                **  Open the Window4GL error log file
                */
                if (ret == OK)
                    ret = SIopen(&loc, AppendFlg ? "a" : "w", &W4glErrorLog);

                if (ret != OK)
                {
                        i2 Retc;

                        W4glErrorLog = NULL;
                        Retc = MessageBox( 0, "Unable to open the log file. Do you want to continue ?", "Warning",
                                  MB_DEFBUTTON2 | MB_ICONQUESTION | MB_YESNO );
                        MEfree(W4glLogName);
                        W4glLogName = NULL;
                        if ( Retc ==IDNO )
                        {
                                PCexit( FAIL );
                        }
                }
            }
        }

}

/*
** Write string to log file if it is open.
*/

VOID
SIlogWrite(
char *  str)
{
        if (W4glErrorLog)
        {
            i4          FilePos;
            i4          NumRead;

            SIputrec(str, W4glErrorLog);
            SIflush(W4glErrorLog);

            /*
            ** We only let our log file grow to W4GL_MAX_LOG_SIZE ( initally 32K )
            ** because we want to be able to view it in notepad, which can only
            ** handle  ~45K bytes of info. If the log grows past our limit, we
            ** then truncate the log to W4GL_MAX_LOG_SIZE / 2. This is done so
            ** the user never loses recent information.
            */

            if( ( FilePos = SIftell( W4glErrorLog ) ) >= 0 )
            {
                if( FilePos > W4GL_MAX_LOG_SIZE )
                {
                    if( !W4glErrorLogBuf )
                    {
                        W4glErrorLogBuf = MEreqmem( NULL, W4GL_MAX_LOG_SIZE / 2 + 1, FALSE, NULL );
                        if( !W4glErrorLogBuf )
                        {
                            MessageBox( 0, "Unable to allocate memory for the w4gl.log file management. Windows 4GL will be shutdown.",NULL,
                                                MB_OK );
                            PCexit( FAIL );
                        }
                    }
                    if( SIfseek( W4glErrorLog, W4GL_MAX_LOG_SIZE / 2, SI_P_START ) != OK )
                    {
                        MessageBox( 0, "Unable to seek to desired position in the w4gl.log file. Windows 4GL will be shutdown.",NULL,
                                                MB_OK );
                        PCexit( FAIL );
                    }
                    SIread( W4glErrorLog, W4GL_MAX_LOG_SIZE, &NumRead, W4glErrorLogBuf );
                    SIclose( W4glErrorLog );
                    if ( NMf_open( "rw", W4glLogName, &W4glErrorLog ) != OK )
                    {
                        i2 Retc;

                        W4glErrorLog = NULL;
                        Retc = MessageBox( 0, "Unable to open the w4gl.log file. Do you want to continue ?", "Warning",
                                                MB_DEFBUTTON2 | MB_ICONQUESTION | MB_YESNO );
                        MEfree(W4glLogName);
                        W4glLogName = NULL;
                        if ( Retc ==IDNO )
                        {
                            PCexit( FAIL );
                        }
                    }
                    else
                    {
                        i4      count;

                        SIwrite( NumRead, W4glErrorLogBuf, &count, W4glErrorLog );
                    }
                }
            }
        }

}

/*
** Close log file if it is open.
*/

VOID
SIlogClose(void)
{
        if (W4glErrorLog)
        {
                SIclose(W4glErrorLog);
                W4glErrorLog = NULL;
        }
}
