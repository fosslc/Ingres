/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: wtsupld.c
**
** Description:
**      Upload facility.  This module contains the functions to process
**      file uploads from HTML forms according to RFC's 1521 and 1867.
**
**      WTSUploadInit       Initialise the function table.
**      WTSOpenUpLoad       Extract the first boundary string.
**      WTSUpLoad           Handle fule upload.
**      WTSCloseUpLoad      Terminate upload.
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
**      01-Feb-1999 (fanra01)
**          Add handling of incomplete boundaries by adding UE_PBOUND event and
**          FRAGMENT_INCOMPLETE flag.
**          Limit skipping white space to length of a line.
**      02-Mar-1999 (fanra01)
**          Add logging message.
**      28-Apr-1999 (fanra01)
**          Changes to handle the case where HTML close comments '--' could be
**          interpreted as a tranfer boundary.
**      07-May-1999 (fanra01)
**          Fixup compiler warning.
**      29-Mar-2001 (fanra01)
**          Bug 104379
**          Check the additional varcmplt flag when a starting boundary event
**          is processed in the US_VAL state.  If the variable is incomplete
**          set the varname= into the request.
*/
#include <wsf.h>
#include <wts.h>
#include <erwsf.h>
#include <wss.h>
#include <wpsbuffer.h>
#include <wssapp.h>

#include <asct.h>

#include "wtslocal.h"

/*
** Upload states.
*/
enum upld_states
{
    US_IDLE,        /* State of newly allocated upload structure             */
    US_BEGIN,       /* First boundary string located                         */
    US_DISP,        /* Starting boundary string recevied                     */
    US_VAL,         /* form-data with no filename received                   */
    US_FILE,        /* form-data with filename recevied                      */
    US_FWRITE,      /* mime type or encode type received                     */
    US_FPARM,       /* remove new line delimiter to file                     */
    US_END,         /* ended state                                           */
    US_MAXSTATES
};

/*
** Upload events.
*/
enum upld_events
{
    UE_OPEN,        /* Content-type: multipart/form-data, boundary=uuuuuu    */
    UE_FORM,        /* content-dispostion: form-data; name="vvvvvv"          */
    UE_EMBED,       /* Content-Type: multipart/mixed, boundary=wwwwwww       */
    UE_SBOUND,      /* --uuuuuu                                              */
    UE_VALUE,       /* variable value                                        */
    UE_FNAME,       /* content-dispostion: form-data; name="xx" filename="yy"*/
    UE_MIXED,       /* Content-type: multipart/mixed, boundary=zzzzzz        */
    UE_EBOUND,      /* --uuuuuu--                                            */
    UE_MIME,        /* content-Type: abcde/fghijk                            */
    UE_FVALUE,      /* file contents                                         */
    UE_FENC,        /* Content-Transfer-Encoding: lmnopqr                    */
    UE_PBOUND,      /* a partial boundary received                           */
    UE_EXCEPT,      /* unknown event received                                */
    UE_MAXEVENTS
};

/*
** Content attributes.
*/
enum upld_attr
{
    A_TYPE,         /* type */
    A_DISP,         /* disposition */
    A_ENC           /* encoding */
};

/*
** Disposition types.
*/
enum dtype
{
    D_FORM,         /* form-data */
    D_ATTACH        /* attachment */
};

/*
** Name: state-event function table.
**
** Decription:
**      The table represents the states and the events that are handled
**      in that state.
**      The Trans row indicates the next state which will be set having handled
**      the event.
**
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |STATE    |US_IDLE |US_BEGIN|US_DISP |US_VAL  |US_FILE  |US_FPARM|US_FWRITE|
** +---------+        |        |        |        |         |        |         |
** |EVENT    |        |        |        |        |         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_OPEN  |        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |US_BEGIN|        |        |        |         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_FORM  |        |        |Next is |        |         |        |         |
** |         |        |        |Value   |        |         |        |         |
** |Trans    |        |        |US_VAL  |        |         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_EMBED |        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |        |        |        |US_BEGIN|         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_SBOUND|        |        |        |        |         |        |Close    |
** |         |        |        |        |        |         |        |File     |
** |Trans    |        |US_DISP |        |US_DISP |         |        |US_DISP  |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_VALUE |        |        |        |Store   |         |        |         |
** |         |        |        |        |Value   |         |        |         |
** |Trans    |        |        |        |US_VAL  |         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_FNAME |        |        |Open    |        |         |        |         |
** |         |        |        |File    |        |         |        |         |
** |Trans    |        |        |US_FILE |        |         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_MIXED |        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |        |        |        |        |         |        |         |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_EBOUND|        |        |        |        |         |        |Close    |
** |         |        |        |        |        |         |        |File     |
** |Trans    |        |        |US_END  |US_END  |         |        |US_DISP  |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_MIME  |        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |        |        |        |        |US_FPARM |US_FPARM|US_FWRITE|
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_FVALUE|        |        |        |        |         |        |Write    |
** |         |        |        |        |        |         |        |File     |
** |Trans    |        |        |        |        |         |        |US_FWRITE|
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_FENC  |        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |        |        |        |        |US_FPARM |US_FPARM|US_FWRITE|
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_PBOUND|        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |        |NOP     |NOP     |NOP     |         |        |NOP      |
** +---------+--------+--------+--------+--------+---------+--------+---------+
** |UE_EXCEPT|        |        |        |        |         |        |         |
** |         |        |        |        |        |         |        |         |
** |Trans    |US_END  |US_END  |US_END  |US_END  |US_END   |US_END  |US_END   |
** +---------+--------+--------+--------+--------+---------+--------+---------+
*/

static i4  endofline (PWTS_UPLOAD load, char* buf, i4  len);
static GSTATUS HandlerError( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS MessageIncomplete( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Begin_SBound( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Disp_Form( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Disp_Fname( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Disp_EBound( WTS_SESSION* sess, char* line, i4  linelen,
    i4* pos );
static GSTATUS Val_SBound( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Val_Embed( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Val_Value( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS Val_EBound( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS File_Mime( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS File_Fenc( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS FParm_Mime( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS FParm_Fenc( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS FWrite_SBound( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS FWrite_EBound( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );
static GSTATUS FWrite_Fvalue( WTS_SESSION* sess, char* line, i4  linelen,
    i4 * pos );


static char* attribute[]=
{
    "type:", "disposition:", "transfer-encoding:", NULL
};
static char* disptype[]={ "form-data;", "attachment;", NULL };

/*
** State-event function table
*/
GSTATUS (*handler[US_MAXSTATES][UE_MAXEVENTS])(WTS_SESSION*, char*, i4 , i4 *);

/*
** Name: HandlerError
**
** Decription:
**      Function is catch all for states where events are unexpected.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
HandlerError (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    return (DDFStatusAlloc(E_WS0088_WTS_INVALID_UPLOAD));
}

/*
** Name: HandlerException
**
** Decription:
**      Function is is to catch unknown or new events.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
HandlerException (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    PWTS_UPLOAD load= sess->load;

    load->state = US_END;

    return (DDFStatusAlloc(E_WS0088_WTS_INVALID_UPLOAD));
}

/*
** Name: MessageIncomplete
**
** Decription:
**      Function handles the case where the boundary is incomplete.
**      Sets the indication that an incomplete boundary has been received.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.  Always 0.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      01-Feb-1999 (fanra01)
**          Created.
*/
static GSTATUS
MessageIncomplete (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;

    load->fragment |= FRAGMENT_INCOMPLETE;

    return (err);
}

/*
** Name: Begin_SBound
**
** Decription:
**      Received the starting boundary.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Begin_SBound (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;

    load->state = US_DISP;
    return (err);
}

/*
** Name: Disp_Form
**
** Decription:
**      Disposition: form-data; recevied with no filename.  Implies a page
**      variable name.
**      Extract the variable and store it in the upload structure for use
**      later.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Disp_Form (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    char*       p = line + *pos;
    char*       varname;
    i4          remain = linelen - *pos;
    i4          i = 0;
    i4          j = 0;

    if (STbcompare ("name", 4, p, 0, TRUE) == 0)
    {
        /*
        ** Find the first quote after name
        */
        while ((*p != '\"') && (i < remain))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        CMbyteinc(i,p);     /* skip the quote */
        CMnext(p);
        varname= p;
        while ((*p != '\"') && (i < remain))
        {
            j+=CMbytecnt(p);
            CMbyteinc(i,p);
            CMnext(p);
        }
        CMbyteinc(i,p);     /* skip the quote */
        CMnext(p);
        /*
        ** Allocate space for the variable name a '=' character and an EOS
        */
        if ((err = GAlloc (&load->varname, j+2, FALSE)) == GSTAT_OK)
        {
            MEfill (j+2, 0, load->varname);
            MECOPY_VAR_MACRO (varname, j, load->varname);
            STcat (load->varname, "=");
        }
        *pos += i;
        load->state = US_VAL;
    }
    else
    {
        HandlerException (sess, line, linelen, pos);
    }
    return (err);
}

/*
** Name: Disp_Fname
**
** Decription:
**      Takes the string
**        content-disposition: form-data; name="xxxx" filename="yyyy"
**      opens an ice temporary file and sets items in the variable string
**      as xxxx=icetempfile.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Disp_Fname (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS         err = GSTAT_OK;
    PWTS_UPLOAD     load= sess->load;
    char*           p = line + *pos;
    char*           name;
    i4              remain = linelen - *pos;
    i4              i = 0;
    i4              j = 0;
    UPLOADED_FILE*  upload;

    if (STbcompare ("name", 4, p, 0, TRUE) == 0)
    {
        /*
        ** Find the first quote after name
        */
        while ((*p != '\"') && (i < remain))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        CMbyteinc(i,p);     /* skip the quote */
        CMnext(p);
        name= p;
        while ((*p != '\"') && (i < remain))
        {
            j+=CMbytecnt(p);
            CMbyteinc(i,p);
            CMnext(p);
        }
        CMbyteinc(i,p);
        CMnext(p);          /* skip the quote */
        while ((*p != ';') && (i < remain))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        CMbyteinc(i,p);
        CMnext(p);          /* skip the semi colon */
        /*
        ** Allocate space for the variable name a '=' character and an EOS
        */
        if ((err = GAlloc (&load->varname, j+2, FALSE)) == GSTAT_OK)
        {
            MEfill (j+2, 0, load->varname);
            MECOPY_VAR_MACRO (name, j, load->varname);
            STcat (load->varname, "=");
        }
        while (CMwhite(p) && (i < remain))
        {
            CMbyteinc(i, p);
            CMnext(p);
        }
        if (STbcompare ("filename", 8, p, 0, TRUE) == 0)
        {
            /*
            ** Find the first quote after filename
            */
            while ((*p != '\"') && (i < remain))
            {
                CMbyteinc(i,p);
                CMnext(p);
            }
            CMbyteinc(i,p);
            CMnext(p);          /* skip the quote */
            name= p;
            while ((*p != '\"') && (i < remain))
            {
                j+=CMbytecnt(p);
                CMbyteinc(i,p);
                CMnext(p);
            }
            CMbyteinc(i,p);
            CMnext(p);          /* skip the quote */
            /*
            ** Allocate space for the filename and an EOS
            */
            if ((err = GAlloc (&load->filename, j+1, FALSE)) == GSTAT_OK)
            {
                MEfill (j+1, 0, load->filename);
                MECOPY_VAR_MACRO (name, j, load->filename);
                asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
                    "Upload session=%s file=%s",
                    ((sess->act_session != NULL) ?
                        ((sess->act_session->user_session) ?
                            sess->act_session->user_session->name : "None")
                    : "None"),
                    load->filename );
            }
            *pos += i;
            err = G_ME_REQ_MEM(0, upload, UPLOADED_FILE, 1);
            if (err == GSTAT_OK)
            {
                bool status;

                err = WCSRequest( 0, NULL, SYSTEM_ID, NULL, WSF_ICE_SUFFIX,
                    &upload->file, &status);
                if (err == GSTAT_OK)
                {
                    CLEAR_STATUS(WCSLoaded(upload->file, TRUE));
                    err = WCSOpen (upload->file, "w", SI_BIN, 0);
                    if (err == GSTAT_OK)
                    {
                        i4  nlen = STlength (load->varname);
                        i4  flen = STlength(upload->file->info->path);

                        err = GAlloc(&load->icefname, flen + 1, FALSE);
                        if (err == GSTAT_OK)
                        {

                            MECOPY_VAR_MACRO( upload->file->info->path, flen,
                                load->icefname);

                            err = GAlloc((PTR*)&sess->variable,
                                sess->vlen + nlen + flen + 2, TRUE);
                            if (err == GSTAT_OK && sess->variable != NULL)
                            {
                                if (sess->vlen > 0)
                                {
                                    sess->variable[sess->vlen++] = '&';
                                }
                                MECOPY_VAR_MACRO(load->varname, nlen,
                                    sess->variable + sess->vlen);
                                sess->vlen += nlen;
                                if (flen)
                                {
                                    MECOPY_VAR_MACRO(load->icefname, flen,
                                        sess->variable + sess->vlen);
                                    sess->vlen += flen;
                                    upload->next = sess->list;
                                    sess->list = upload;
                                }
                                sess->variable[sess->vlen] = EOS;
                            }
                        }
                    }
                    else
                    {
                        CLEAR_STATUS(WCSRelease(&upload->file));
                    }
                }
            }
            load->state = US_FILE;
        }
        else
        {
            HandlerException (sess, line, linelen, pos);
        }
    }
    else
    {
        HandlerException (sess, line, linelen, pos);
    }
    return (err);
}

/*
** Name: Disp_EBound
**
** Decription:
**      A terminating boundary has been recevied.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Disp_EBound (WTS_SESSION* sess, char* line, i4  linelen, i4* pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;

    load->state = US_END;
    return (err);
}

/*
** Name: Val_SBound
**
** Decription:
**      A starting boundary recevied whilst expecting a value.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
**      29-Mar-2001 (fanra01)
**          Add test of varcmplt flag otherwise an empty variable would be
**          ignored.
*/
static GSTATUS
Val_SBound (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;

    /*
    ** Received a starting boundary indicator in the US_VAL state.
    ** If the varcmplt flag is not set the variable has no value.
    ** The name value pair should still be copied to the variable list.
    */
    if ((load->state == US_VAL) && (load->varcmplt == 0))
    {
        err = Val_Value( sess, line, 0, pos );
    }

    load->state = US_DISP;
    load->varcmplt = 0;
    *pos = linelen;
    return (err);
}

/*
** Name: Val_Embed
**
** Decription:
**      A content type indicating multipary mixed was received whilst expecting
**      a value.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Val_Embed (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;

    load->state = US_BEGIN;
    return (err);
}

/*
** Name: Val_Value
**
** Decription:
**      A value should be extracted from the buffer and appended to the
**      variables list.
**      Format of the variable list is variable=value&variable=value.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Val_Value (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    i4          nlen = STlength (load->varname);

    err = GAlloc((PTR*)&sess->variable,
        sess->vlen + nlen + linelen + 2, TRUE);
    if (err == GSTAT_OK && sess->variable != NULL)
    {
        if (sess->vlen > 0)
        {
            sess->variable[sess->vlen++] = '&';
        }
        MECOPY_VAR_MACRO(load->varname, nlen, sess->variable + sess->vlen);
        sess->vlen += STlength (load->varname);
        if (linelen)
        {
            MECOPY_VAR_MACRO(line, linelen,
                sess->variable + sess->vlen);
            sess->vlen += linelen;
            load->varcmplt = 1;
        }
        sess->variable[sess->vlen] = EOS;
    }
    load->state = US_VAL;
    *pos = linelen;
    return (err);
}

/*
** Name: Val_EBound
**
** Decription:
**      A terminating boundary has been recevied.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
Val_EBound (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;

    load->state = US_END;
    *pos = linelen;
    return (err);
}

/*
** Name: File_Mime
**
** Decription:
**      The mime type of the following file.  Extracted an saved for use in
**      in the future.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
File_Mime (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    i4          remain = linelen - *pos;
    char*       p = line + *pos;
    char*       mime = NULL;
    i4          i=0;

    if ((err = GAlloc (&mime, remain + 1, FALSE)) == GSTAT_OK)
    {
        load->type = mime;
        while ((remain > 0) && CMwhite(p))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        while (i < remain)
        {
            CMbyteinc(i,p);
            CMcpyinc (p, mime);
        }
        load->state = US_FPARM;
    }
    *pos += i;
    return (err);
}

/*
** Name: File_Fenc
**
** Decription:
**      An encoding description for the following file.  Stored for use in
**      the furture.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
File_Fenc (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    i4          remain = linelen - *pos;
    char*       p = line + *pos;
    char*       enc = NULL;
    i4          i=0;

    if ((err = GAlloc (&enc, remain + 1, FALSE)) == GSTAT_OK)
    {
        load->encode = enc;
        while (CMwhite(p))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        while (i < remain)
        {
            CMbyteinc(i,p);
            CMcpyinc (p, enc);
        }
        load->state = US_FPARM;
    }
    *pos += i;
    return (err);
}

/*
** Name: FWrite_Mime
**
** Decription:
**      The mime type of the following file.  Extracted an saved for use in
**      in the future.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
FParm_Mime (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    i4          remain = linelen - *pos;
    char*       p = line + *pos;
    char*       mime = NULL;
    i4          i=0;


    if ((err = GAlloc (&mime, remain + 1, FALSE)) == GSTAT_OK)
    {
        load->type = mime;
        while (CMwhite(p))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        while (i < remain)
        {
            CMbyteinc(i, p);
            CMcpyinc (p, mime);
        }
        load->state = US_FPARM;
    }
    *pos += i;
    return (err);
}

/*
** Name: FWrite_Fenc
**
** Decription:
**      An encoding description for the following file.  Stored for use in
**      the furture.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
FParm_Fenc (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    i4          remain = linelen - *pos;
    char*       p = line + *pos;
    char*       enc = NULL;
    i4          i=0;

    if ((err = GAlloc (&enc, remain + 1, FALSE)) == GSTAT_OK)
    {
        load->encode = enc;
        while (CMwhite(p))
        {
            CMbyteinc(i,p);
            CMnext(p);
        }
        while (i < remain)
        {
            CMbyteinc(i,p);
            CMcpyinc (p, enc);
        }
        load->state = US_FPARM;
    }
    *pos += i;
    return (err);
}

/*
** Name: FWrite_SBound
**
** Decription:
**      A delimiting boundary is recevied during upload of a file.  Indicating
**      the file has completed and should now be closed.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
FWrite_SBound (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    UPLOADED_FILE*  upload = sess->list;

    if (upload != NULL)
    {
        sess->list = upload->next;
        CLEAR_STATUS(WCSClose(upload->file));
        MEfree ((PTR)upload);
    }
    load->state = US_DISP;
    return (err);
}

/*
** Name: FWrite_EBound
**
** Decription:
**      A terminating boundary is recevied during upload of a file.  Indicating
**      the file has completed and should now be closed.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
FWrite_EBound (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    UPLOADED_FILE*  upload = sess->list;

    if (upload != NULL)
    {
        sess->list = upload->next;
        CLEAR_STATUS(WCSClose(upload->file));
        MEfree ((PTR)upload);
    }
    load->state = US_END;
    return (err);
}

/*
** Name: FWrite_Fvalue
**
** Decription:
**      The input buffer should be written to file.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
*/
static GSTATUS
FWrite_Fvalue (WTS_SESSION* sess, char* line, i4  linelen, i4 * pos)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD load= sess->load;
    i4          count =0;

    if ((err = WCSWrite(linelen, line, &count, sess->list->file)) == GSTAT_OK)
    {
        load->state = US_FWRITE;
    }
    else
    {
        UPLOADED_FILE*  upload = sess->list;

        if (upload != NULL)
        {
            sess->list = upload->next;
            CLEAR_STATUS(WCSClose(upload->file));
            MEfree ((PTR)upload);
        }
    }
    *pos += count;
    return (err);
}

/*
** Name: WTSUploadInit
**
** Decription:
**      Initialise the function matrix.
**
** Inputs:
**      sess        ice session structure
**      line        pointer to the current line
**      linelen     length of the line
**
** Outputs:
**      pos         number of characters handled.
**
** Return:
**      GSTAT_OK    success
**      other       failure
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
**      01-Feb-1999 (fanra01)
**          Add MessageIncomplete handlers for US_BEGIN, US_DISP, US_VAL and
**          US_WRITE states.
*/
GSTATUS
WTSUploadInit ()
{
    GSTATUS status = GSTAT_OK;
    i4  i;
    i4  j;

    for (i=0; i < US_MAXSTATES; i++)
    {
        for (j=0; j < UE_MAXEVENTS; j++)
        {
            handler[i][j] = HandlerError;
        }
    }
    handler[US_BEGIN][UE_SBOUND]    = Begin_SBound;
    handler[US_BEGIN][UE_PBOUND]    = MessageIncomplete;
    handler[US_DISP][UE_FORM]       = Disp_Form;
    handler[US_DISP][UE_FNAME]      = Disp_Fname;
    handler[US_DISP][UE_EBOUND]     = Disp_EBound;
    handler[US_DISP][UE_PBOUND]     = MessageIncomplete;
    handler[US_VAL][UE_SBOUND]      = Val_SBound;
    handler[US_VAL][UE_VALUE]       = Val_Value;
    handler[US_VAL][UE_EBOUND]      = Val_EBound;
    handler[US_VAL][UE_PBOUND]      = MessageIncomplete;
    handler[US_FILE][UE_MIME]       = File_Mime;
    handler[US_FILE][UE_FENC]       = File_Fenc;
    handler[US_FPARM][UE_MIME]      = FParm_Mime;
    handler[US_FPARM][UE_FENC]      = FParm_Fenc;
    handler[US_FWRITE][UE_FVALUE]   = FWrite_Fvalue;
    handler[US_FWRITE][UE_SBOUND]   = FWrite_SBound;
    handler[US_FWRITE][UE_EBOUND]   = FWrite_EBound;
    handler[US_FWRITE][UE_PBOUND]   = MessageIncomplete;

    handler[US_IDLE][UE_EXCEPT]     = HandlerException;
    handler[US_BEGIN][UE_EXCEPT]    = HandlerException;
    handler[US_DISP][UE_EXCEPT]     = HandlerException;
    handler[US_VAL][UE_EXCEPT]      = HandlerException;
    handler[US_FILE][UE_EXCEPT]     = HandlerException;
    handler[US_FWRITE][UE_EXCEPT]   = HandlerException;

    return (status);
}

/*
** Name: WTSOpenUpLoad() -
**
** Description:
**
** Inputs:
**      contentType
**      session
**
** Outputs:
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
WTSOpenUpLoad(
    char        *contentType,
    WTS_SESSION *session)
{
    GSTATUS     err;
    i4          length = STlength(MULTIPART);
    PWTS_UPLOAD load = NULL;
    char*       bound;
    i4          blen;

    if (contentType != NULL &&
        STscompare(contentType, length, MULTIPART, length) == 0)
    {
        session->status = UPLOAD;
        if ((err = GAlloc ((char**)&load, sizeof(WTS_UPLOAD),
            FALSE)) == GSTAT_OK)
        {
            session->load = load;
            bound = STindex (contentType, BOUNDARY, 0) + STlength(BOUNDARY);
            blen  = STlength (bound);

            /*
            ** Allocate space for a start boundary and and en boundary
            ** each having the delimiters and null terminator.
            */
            if (((err = GAlloc (&load->start, blen + 3, FALSE)) == GSTAT_OK) &&
                ((err = GAlloc (&load->end, blen + 5, FALSE)) == GSTAT_OK))
            {
                STcopy ("--",load->start);
                STlcopy (bound, load->start + 2, blen);
                STcopy (load->start, load->end);
                STcat (load->end, "--");
            }
            load->state = US_BEGIN;
        }
    }
    else
    {
        session->status = NO_UPLOAD;
        session->load   = NULL;
    }
    session->list = NULL;
    session->variable = NULL;
    session->vlen = 0;

    return(GSTAT_OK);
}

/*
** Name: endofline
**
** Decription:
**      Function returns the number of bytes to a set of new line characters.
**      The state is used to add additional checking when in transfer mode
**      where the contents of the file may also contain new line characters.
**
** Inputs:
**      load
**      buf
**      len
**
** Outputs:
**      load.endfile
**      load.fragment
**
** Return:
**      length of line to the end of buffer or newline or boundary.
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
**      28-Apr-1999 (fanra01)
**          Add length check prior to string compare to ensure a comparison
**          of the correct number of characters.
**      07-May-1999 (fanra01)
**          Add cast to remove warning.
*/
static i4
endofline (PWTS_UPLOAD load, char* buf, i4  len)
{
    i4      i;
    char    c;
    bool    delim = FALSE;

    for (i=0; (i < len) && (delim == FALSE); i+=1)
    {
        if ((c= buf[i]) == '\r')
        {
            if (buf[i+1] == '\n')
                delim = TRUE;
        }
        else if (c == '\n')
        {
            if (buf[i+1] == '\r')
                delim = TRUE;
        }
        /*
        ** If in transfer state check that we have reached the end delimiter by
        ** testing the 2 characters after the "\r\n" for start of boundary.
        */
        if (delim == TRUE)
        {
            if (load->state == US_FWRITE)
            {
                if (load->endfile == FALSE)
                {
                    /*
                    ** if boundary marker found continue else switch off delim
                    */
                    if ((buf[i+2] == '-') && (buf[i+3] == '-') &&
                        ((len - i) > (i4)STlength(load->start)) &&
                        (STbcompare (load->start, STlength(load->start),
                         &buf[i+2], 0, TRUE) == 0))
                    {
                        load->endfile = TRUE;
                        delim = TRUE;
                        break;  /* leave the loop */
                    }
                    else
                    {
                        delim = FALSE;
                    }
                }
            }
            else
            {
                break;  /* leave the loop */
            }
        }
    }
    if (delim != TRUE)
    {
        if (load->state != US_FWRITE)
        {
            load->fragment |= FRAGMENT_HEAD;
        }
        else
        {
            load->fragment &= ~FRAGMENT_HEAD;
        }
    }
    else
    {
        if (load->fragment & FRAGMENT_HEAD)
        {
            /*
            ** Use exclusive OR to disable FRAGMENT_HEAD and enable
            ** FRAGMENT_TAIL
            */
            load->fragment ^= (FRAGMENT_HEAD | FRAGMENT_TAIL);
        }
        else
        {
            load->fragment &= ~FRAGMENT_TAIL;
        }
    }
    return (i);
}

/*
** Name:
**
** Decription:
**
** Inputs:
**      load
**      buf
**      len
**
** Outputs:
**      count
**
** Return:
**
** History:
**      23-Oct-98 (fanra01)
**          Created.
**      01-Feb-1999 (fanra01)
**          Add data length parameter to determine incomplete messages.
**      27-Apr-1999 (fanra01)
**          Update the incomplete message handling to account for '--'
**          characters that don't define a boundary.
*/
static i4
getstrevent (PWTS_UPLOAD load, char *buf, i4  len, i4 datalen, i4 * count)
{
    i4      eventnum = 0;
    i4      i;
    i4      j;
    char*   p = buf;
    i4      pos = 0;
    i4      boundlen;

    if ((buf[0]=='-') && (buf[1] == '-'))
    {
        /*
        ** If the remaining string is less than the length of boundary string
        ** there is likely more to follow.  Set the event for a partial
        ** boundary.
        */
        boundlen = STlength(load->end);
        if ((boundlen < datalen) &&
            (STbcompare (load->end, boundlen, p, 0, TRUE) == 0))
        {
            eventnum = UE_EBOUND;
            pos+=STlength(load->end);
        }
        else
        {
            boundlen = STlength(load->start);
            if ((boundlen < datalen) &&
                (STbcompare (load->start, boundlen, p, 0, TRUE) == 0))
            {
                eventnum = UE_SBOUND;
                pos+=STlength(load->start);
            }
            else
            {
                if (datalen < boundlen)
                {
                    eventnum = UE_PBOUND;
                }
            }
        }
    }

    if ((eventnum == 0) && (*p) &&
        (STbcompare (ERx("content-"), 8, p, 0, TRUE) == 0))
    {
        pos+=8;
        p+=pos;
        /*
        ** Scan list of content attributes
        */
        for (i=0; (attribute[i] != NULL) && (eventnum == 0); i++)
        {
            if ((STbcompare (attribute[i], STlength(attribute[i]), p, 0,
                TRUE)) == 0)
            {
                p += STlength(attribute[i]);
                pos+=STlength(attribute[i]);
                /*
                ** Ensure we don't go beyond the line
                */
                while ((pos < len) && CMwhite(p))
                {
                    CMbyteinc(pos, p);
                    CMnext(p);
                }

                switch (i)
                {
                    case A_TYPE:
                        if ((STbcompare (ERx("multipart/mixed"), 15, p, 0,
                            TRUE)) == 0)
                        {
                            pos+=15;
                            p += 15;
                            eventnum = UE_EMBED;
                        }
                        else
                        {
                            eventnum = UE_MIME;
                        }
                        break;

                    case A_DISP:
                        for (j=0; (disptype[j] != NULL) && (eventnum == 0);
                            j++)
                        {
                            if ((STbcompare (disptype[j],
                                STlength(disptype[j]), p, 0, TRUE)) == 0)
                            {
                                pos+=STlength(disptype[j]);
                                p += STlength(disptype[j]);
                                /*
                                ** Ensure we don't go beyond the line
                                */
                                while ((pos < len) && CMwhite(p))
                                {
                                    CMbyteinc(pos, p);
                                    CMnext(p);
                                }
                                switch (j)
                                {
                                    case D_FORM:
                                        if (STstrindex(p, "filename=",
                                            len - pos, TRUE) !=NULL)
                                        {
                                            eventnum = UE_FNAME;
                                        }
                                        else
                                        {
                                            eventnum = UE_FORM;
                                        }
                                        break;
                                    case D_ATTACH:
                                        eventnum = UE_FNAME;
                                        break;
                                }
                            }
                        }
                        if (eventnum == 0) eventnum = UE_EXCEPT;
                        break;

                    case A_ENC:
                        eventnum = UE_FENC;
                        break;
                }
            }
        }
        if (eventnum == 0) eventnum = UE_EXCEPT;
    }
    else
    {
        if (eventnum == 0)
        {
            /*
            ** Unnamed line - determine event on previous state
            */
            switch (load->state)
            {
                case US_VAL:
                    eventnum = UE_VALUE;
                    break;
                case US_FWRITE:
                    eventnum = UE_FVALUE;
                    break;
                default:
                    eventnum = UE_EXCEPT;
                    break;
            }
        }
    }
    *count+=pos;
    return (eventnum);
}

/*
** Name: WTSUpLoad() -
**
** Description:
**	read the following format.
**		1) --
**		2) boundary if there is a new parameter.
**		3) "Content-Disposition: form-data"
**		4) name and filename (if it is a file) of the parameter + '\r\n'
**		5) if it is a file, the mime type (Content-Type) '\r\n'
**		6) '\r\n'
**		7) the value + '\r\n'
**
** Inputs:
**
** Outputs:
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
**	8-Sep-98 (marol01)
**          fix issue with the upload feature. The format expected by the
**          function is incorrect when the system try to read a file with
**          a supported mime type.
*/
GSTATUS
WTSUpLoad(
    char*       data,
    i4          *datalength,
    WTS_SESSION *session)
{
    GSTATUS     err = GSTAT_OK;
    i4          linelen;
    i4          count = 0;
    i4          eventnum;
    char*       p = data;
    char*       work;
    PWTS_UPLOAD load = session->load;

    load->fragment &= ~FRAGMENT_INCOMPLETE;

    /*
    ** Terminate processing loop if incomplete message is encountered.
    ** This will be recalled once more data has been read.
    */
    while ((err == GSTAT_OK) && (*datalength > 0) &&
           ((load->fragment & FRAGMENT_INCOMPLETE) == 0))
    {
        count = 0;
        linelen = endofline (load, p, *datalength);
        if ((load->fragment != 0) && (load->state != US_FWRITE))
        {
            if ((load->fragment & FRAGMENT_MASK) == FRAGMENT_HEAD)
            {
                load->fraglen = linelen;
                err = GAlloc(&load->buf, load->fraglen, FALSE);
                MECOPY_VAR_MACRO(p, linelen, load->buf);
                work = NULL;
                p+=linelen;
                *datalength-=linelen;
            }
            else if ((load->fragment & FRAGMENT_MASK) == FRAGMENT_TAIL)
            {
                err = GAlloc(&load->buf, load->fraglen + linelen, TRUE);
                MECOPY_VAR_MACRO(p, linelen, load->buf + load->fraglen);
                work = load->buf;
                p+=linelen;
                *datalength-=linelen;
                linelen = load->fraglen + linelen;
            }
        }
        else
        {
            work = p;
        }
        if (work != NULL)
        {
            if (linelen != 0)
            {
                eventnum = getstrevent(load, work, linelen, *datalength,
                    &count);
                err = handler[load->state][eventnum](session, work, linelen,
                    &count);
                if (err == GSTAT_OK)
                {
                    if ((load->fragment & FRAGMENT_MASK) != FRAGMENT_TAIL)
                    {
                        /*
                        ** Processed whole line skip '\r\n'
                        */
                        count += ((count==linelen) &&
                            (*datalength>linelen+2)) ? 2 : 0;
                        *datalength -= count;
                        p+=count;
                    }
                    else
                    {
                        load->fragment = 0;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (load->state == US_FPARM)
                {
                    load->state = US_FWRITE;
                }
                count+=2;
                *datalength -= count;
                p+=count;
            }
        }
    }
    return(err);
}

/*
** Name: WTSCloseUpLoad() - 
**
** Description:
**
** Inputs:
**
** Outputs:
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
WTSCloseUpLoad(
	WTS_SESSION	*session)
{
    GSTATUS     err = GSTAT_OK;
    PWTS_UPLOAD next = session->load;
    PWTS_UPLOAD load;
    UPLOADED_FILE*  upload;

    session->load = NULL;
    while (next != NULL)
    {
        load = next;
        if (load->state != US_END)
        {
            upload = session->list;
            if (upload != NULL)
            {
                session->list = upload->next;
                CLEAR_STATUS(WCSClose(upload->file));
                MEfree ((PTR)upload);
            }
            /*
            **  Trace untimely end
            */
        }
        if (load->buf     ) MEfree (load->buf     );
        if (load->start   ) MEfree (load->start   );
        if (load->end     ) MEfree (load->end     );
        if (load->varname ) MEfree (load->varname );
        if (load->icefname) MEfree (load->icefname);
        if (load->filename) MEfree (load->filename);
        if (load->type    ) MEfree (load->type    );
        if (load->encode  ) MEfree (load->encode  );
        next = load->next;
        MEfree ((PTR)load);
    }
    return(err);
}
