/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: c_api.c
**
** Description:
**      Entry points for the ICE programming interface.
**
** History:
**      14-Oct-98 (fanra01)
**          Add http agent string into connect message.
**          Reuse attr_out field as the index to current column and use read
**          field as the index to current row.
**      27-Nov-1998 (fanra01)
**          Add flag for indicating column headers processed.
**          Also add fix for adjusting output buffer for null values.
**      18-Jan-1999 (fanra01)
**          Fix compiler warnings on unix.
**      03-Feb-1999 (fanra01)
**          Corrections to the upload protocol.
**          Changed all nat and longnat to i4.
**      08-Mar-1999 (fanra01)
**          Check validity of pointer before trying to free it.  It is possible
**          to have gaps in the array if not all function attributes has been
**          requested.  Attributes may be null if an error has occured or a
**          previous close is called.
**      11-Mar-1999 (fanra01)
**          Correction to length of variables and values.  Remove single
**          increment as end of string is not required.
**          Extra length causes protocol violation in the ice server as an
**          unexpected boundary is sent.
**      23-Mar-1999 (fanra01)
**          Correct statement to use ii_action for variable name during
**          disconnect.
**      24-Mar-1999 (fanra01)
**          Add history to ICE_C_Initialize.
**          Modified return from ICE_C_Initialize() to be consistent with
**          other ICE_STATUS functions.
**          Ensure that initialize is performed only once.
**      29-Apr-1999 (fanra01)
**          Changed test for position parameter as position can be greater
**          than the number of requested output attributes.
**      29-Dec-1999 (fanra01)
**          Bug 99926
**          Corrected mishanding of null values ending a row buffer.
**          Add test for the null indicator.
**      17-Jan-2000 (fanra01)
**          Bug 100044
**          Correct sigbus error generated when checking for null value in
**          result set.
**      03-Feb-2000 (fanra01)
**          Bug 100330
**          Correct bad attribute value conversion due to length
**          miscalculation when length value spans two buffers.
**      28-Feb-2000 (fanra01)
**          Bug 100632
**          Add ICE_C_Terminate function.  This specifies that the connected
**          session with the ice server should be disconnected.
**      02-Mar-2000 (fanra01)
**          Bug 100632
**          Reinitialize the ICE_C_Initialized flag on terminate so that
**          ICE_C_Initialize can be recalled.
**	06-mar-2000 (somsa01)
**	    Changed SIopen() to SIfopen() for compatibility with UNIX.
**      12-Jun-2000 (fanra01)
**          Bug 101345
**          Add empty node string test to ensure no node has been set.
**      26-Jul-2000 (fanra01)
**          Bug 102182
**          Ensure that the the available length for parameters is correctly
**          set for disconnect.
**	27-oct-2000 (somsa01)
**	    Added include of compat.h and st.h .
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add an ICE_C_ConnectAs API entry point.  This sets the API
**          session context to the one specified.  The cookie should be in the
**          form iicookie="ICE#_#########"
**      29-Oct-2001 (fanra01)
**          Bug 106172
**          Add tests for empty cookie strings before trying to update it.
*/
#include <compat.h>
#include <st.h>
#include <ice_c_api.h>
#include <iceclient.h>
#include <erddf.h>

# define HVAR_PASSWORD      ERx("ii_password")
# define HVAR_USERID        ERx("ii_userid")
# define HVAR_HTTP_AGENT    ERx("http_user_agent")
# define SET_COOKIE         ERx("Set-cookie: ")
# define COOKIE_PATH        ERx("path=/")
# define API_AGENT          ERx("ICE_C_API")

# define OUT_PARAM          ERx("out_")
# define MULTIPART          ERx("multipart; boundary=")
# define BOUNDARY           ERx("-------------ICE%x")
# define FORMAT_ATTR        ERx("--%s\r\nContent-Disposition: form-data; name=\"%s%s\"\r\n\r\n%s\r\n")
# define FORMAT_BLOB        ERx("--%s\r\nContent-Disposition: form-data; name=\"%s\";filename=\"%s\"\r\nContent-Type:\r\n\r\n")
# define FORMAT_END         ERx("--%s--\r\n")

typedef struct __ICE_TUPLE
{
    char*              *tab;
    struct __ICE_TUPLE *next;
} ICE_TUPLE;

typedef struct __ICE_C_CLIENT_INTERNAL
{
    char*           node;
    ICEClient       ice_client;
    STATUS          error;
    ICE_C_PARAMS*   params;
    FILE*           file;
    i4              attr_count;     /* # of elements in the param table */
    i4              attr_out;       /* index of current column */
    i4              attr_out_count; /* total number of columns */
    i4              read;           /* index of current row */
    i4              count;          /* total number of rows */
    char*           *attributes;    /* array of column name strings */
    i4              colhead;        /* flag indicates when the column */
                                    /* headings are processed */
    i4*             cross;          /* cross tab of col name and return val */
    ICE_TUPLE*      first;          /* list of tuples */
    ICE_TUPLE*      last;           /* list of tuples */
    char*           buffer;         /* Results buffer */
    i4              position;       /* Index of current position in buffer */
} ICE_C_CLIENT_INTERNAL;

static i4   ICE_C_Initialized = FALSE;

GSTATUS ICEQuery(i4  clientType, char*    node, PICEClient client);

GSTATUS
C_APIWriteClient(
    u_i4         len,
    char*       buffer,
    PICEClient  client);

GSTATUS
C_APIReadClient(
    u_i4   *len, 
    char *buffer,
    PICEClient client); 

void
ICE_C_SetError(
    ICE_C_CLIENT    client,
    GSTATUS                *err)
{
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client;

    if (internal != NULL && internal->error == 0)
    {
        GSTATUS local = GSTAT_OK;
        CL_ERR_DESC clError;
        i4  nMsgLen = 0;

        local = GAlloc((PTR*)&internal->buffer, 256, FALSE);
        if (local == GSTAT_OK)
        {
            ERslookup (
                    (*err)->number,
                    NULL,
                    0,
                    NULL,
                    internal->buffer,
                    256,
                    -1,
                    &nMsgLen,
                    &clError,
                    0,
                    NULL);
            internal->buffer[nMsgLen] = EOS;
            internal->error = (*err)->number;
        }
        else
        {
            internal->error = 0;
            DDFStatusFree(TRC_EVERYTIME, &local);
        }
        DDFStatusFree(TRC_EVERYTIME, err);
    }
}

/*
** Name: C_APIInitPage
**
** Description:
**      Function to extract the cookie from the server response.
**
** Inputs:
**      pszCType    Page initialization string.
**      dwWritten   Length of string.
**      client      Client context structure.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK
**
** History:
**      29-Oct-2001 (fanra01)
**          Add history.
**          Add tests to the pointers before trying to update them.
*/
GSTATUS
C_APIInitPage(
    char*   pszCType,
    i4      dwWritten,
    PICEClient client)
{
    GSTATUS err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client->user_info;
    char *begin = NULL;
    char *end = NULL;

    begin  = STindex(pszCType, SET_COOKIE, 0);
    if (begin != NULL)
    {
         begin += STlength(SET_COOKIE);
    }
    end = STindex(pszCType, COOKIE_PATH, 0);
    if (end != NULL)
    {
        end += STlength(COOKIE_PATH);
        *end = EOS;
    }

    if (begin != NULL)
    {
        err = GAlloc(&internal->ice_client.Cookies, STlength(begin) + 1, FALSE);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO(begin, STlength(begin) + 1, internal->ice_client.Cookies);
        }
    }
    return(err);
}

/*
** Name: C_APIInitPageAs
**
** Description:
**      Placeholder function that performs a No Op.
**
** Inputs:
**      pszCType    Page initialization string.
**      dwWritten   Length of string.
**      client      Client context structure.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK
**
** History:
**      18-Apr-2001 (fanra01)
**          Created.
*/
GSTATUS
C_APIInitPageAs(
    char*   pszCType,
    i4      dwWritten,
    PICEClient client)
{
    GSTATUS err = GSTAT_OK;
    return(err);
}

GSTATUS
C_APISendURL (
    char* url,
    i4      dwWritten,
    PICEClient client)
{
    GSTATUS err = GSTAT_OK;
    return(err);
}

/*
** Name: C_APICreateAttribute
**
** Description:
**      Function scans through the received buffer and returns the next value.
**      The value is preceded by a space pre-padded length string.
**      If the length string starts with a 00 the value is a null.
**
** Inputs:
**      current         starting position of string length and value
**      buffer          array of string length/value pairs
**      buffer_length   length of buffer
**
** Outputs:
**      current         next string length and value
**      attribute       output string
**
** Returns:
**      GSTAT_OK        success
**      !0              error number for failure
**
** History:
**      27-Nov-1998 (fanra01)
**          Add history.
**          Check for null value and return <NULL> string.
**      29-Dec-1999 (fanra01)
**          Bug 99926
**          The test for exceeding of buffer checks for
**          current + STR_LENGTH_SIZE.  If the last value is a null this check
**          would fail.  Include a test for the remainder of the buffer being
**          a null value.
**      17-Jan-2000 (fanra01)
**          Copy 2 bytes into an i2 to perform the compare for alignment
**          purposes.
**      03-Feb-2000 (fanra01)
**          Bug 100330
**          Check for the end of the buffer splitting the length value
**          causing an incorrect length conversion.
*/
GSTATUS
C_APICreateAttribute(
    i4         *current,
    char*        buffer,
    i4         buffer_length,
    char*        *attribute) 
{
    GSTATUS err = GSTAT_OK;

    *attribute = NULL;
    /*
    ** if the length of buffer remaining contains at least a null indicator
    ** or a string length value then test for a null value.
    */
    if (((buffer_length - *current) == sizeof(i2)) ||
        (*current + STR_LENGTH_SIZE <= buffer_length))
    {
        char buf[STR_LENGTH_SIZE + 1];
        i4     length;
        i2  nullval = 0x3030;
        i2  val;

        /*
        ** Check for the 00 null value indicator.
        */
        MECOPY_VAR_MACRO(buffer + *current, sizeof(i2), (PTR)&val);

        if (val != nullval)
        {
            /*
            ** If not a null value ensure that the remainder of the buffer
            ** has enough for the complete length value otherwise leave it
            ** for the next read.
            */
            if (*current + STR_LENGTH_SIZE <= buffer_length)
            {
                MECOPY_VAR_MACRO(buffer + *current, STR_LENGTH_SIZE, buf);
                *current += STR_LENGTH_SIZE;
                buf[STR_LENGTH_SIZE] = EOS;
                if (CVan(buf, &length) == OK)
                {
                    if (*current + length <= buffer_length)
                    {
                        err = G_ME_REQ_MEM(0, *attribute, char, length + 1);
                        if (err == GSTAT_OK)
                        {
                            MECOPY_VAR_MACRO(buffer + *current, length, *attribute);
                            (*attribute)[length] = EOS;
                        }
                        *current += length;
                    }
                    else
                        *current -= STR_LENGTH_SIZE;
                }
            }
        }
        else
        {
            /*
            ** Return the null string
            */
            char* nullstr = ERx("<NULL>");
            length = STlength(nullstr);

            err = G_ME_REQ_MEM(0, *attribute, char, length + 1);
            if (err == GSTAT_OK)
            {
                MECOPY_VAR_MACRO(nullstr, length, *attribute);
                (*attribute)[length] = EOS;
            }
            *current += sizeof(i2);
        }
    }
    return(err);
}

/*
** Name: C_APIWriteClient
**
** Description:
**      Function scans through the received buffer and retrieves the next
**      value.
**      The loop termii4es when the buffer is exhausted.
**
** Inputs:
**      len             length of buffer
**      buffer          array of string length/value pairs
**      client          ICE client structure
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK        success
**      !0              error number for failure
**
** History:
**      27-Nov-1998 (fanra01)
**          Add check to scan column headings only
*/
GSTATUS
C_APIWriteClient(
    u_i4        len,
    char*       buffer,
    PICEClient  client)
{
    GSTATUS                 err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL*  internal = (ICE_C_CLIENT_INTERNAL*) client->user_info;

    if (internal->error != OK)
        return(err);

    if (internal->file != NULL)
    {
        STATUS status = SIwrite(len, buffer, (i4*)&len, internal->file);
        G_ASSERT(status, status);
    }
    else
    {
        i4      current = 0;
        char*   value = NULL;

        err = GAlloc(&internal->buffer, internal->position + len, TRUE);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO(buffer, len,
                internal->buffer + internal->position);

            internal->position += len;

            if (internal->attr_out_count == -1)
            {
                char buf[NUM_MAX_INT_STR+1];

                if (current + NUM_MAX_INT_STR <= internal->position)
                {
                    MECOPY_VAR_MACRO(internal->buffer, NUM_MAX_INT_STR, buf);
                    current += NUM_MAX_INT_STR ;
                    buf[NUM_MAX_INT_STR ] = EOS;

                    CVan(buf, &internal->error);
                    if (internal->error != OK)
                    {
                        CLEAR_STATUS(C_APICreateAttribute(&current,
                                                         internal->buffer,
                                                         internal->position,
                                                         &value));
                        MEfree((PTR)internal->buffer);
                        internal->buffer = value;
                        internal->position = 0;
                        return(DDFStatusAlloc(internal->error));
                    }
                }
                if (current + NUM_MAX_INT_STR <= internal->position) 
                {
                    MECOPY_VAR_MACRO(internal->buffer + current, NUM_MAX_INT_STR, buf);
                    current += NUM_MAX_INT_STR ;
                    buf[NUM_MAX_INT_STR ] = EOS;
                    CVan(buf, &internal->attr_out_count);
                }
            }
            if (internal->colhead == 0)
            {
                while (err == GSTAT_OK &&
                    internal->attr_out < internal->attr_out_count)
                {
                    err = C_APICreateAttribute(&current,
                        internal->buffer,
                        internal->position,
                        &value);
                    if (err == GSTAT_OK)
                    {
                        if (value == NULL)
                        {
                            break;
                        }
                        else
                        {
                            i4  i = 0;
                            for (; i < internal->attr_count; i++)
                            {
                                if (internal->attributes[i] != NULL &&
                                    STcompare(internal->attributes[i], value)
                                        == 0)
                                {
                                    internal->cross[i] = internal->attr_out++;
                                    break;
                                }
                            }
                            MEfree((PTR)value);
                        }
                    }
                }
                internal->colhead = 1;
            }

            while (    err == GSTAT_OK)
            {
                err = C_APICreateAttribute(&current,
                    internal->buffer,
                    internal->position,
                    &value);
                if (err == GSTAT_OK)
                {
                    if (value == NULL)
                    {
                        break;
                    }
                    else
                    {
                        ICE_TUPLE *tuple = internal->last;
                        /*
                        ** If column count matches requested column count
                        ** this is the first time in for this row.
                        */
                        if (internal->attr_out == internal->attr_out_count)
                        {
                            internal->attr_out = 0; /* reset column count */
                            err = G_ME_REQ_MEM(0, tuple, ICE_TUPLE, 1);
                            if (err == GSTAT_OK)
                                err = G_ME_REQ_MEM(0, tuple->tab, char*, internal->attr_out_count);
                            if (err == GSTAT_OK)
                            {
                                if (internal->first == NULL)
                                {
                                    internal->first = tuple;
                                    internal->last = tuple;
                                }
                                else
                                {
                                    internal->last->next = tuple;
                                    internal->last = tuple;
                                }
                                internal->count++; /* bump row count */
                            }
                        }
                        if (err == GSTAT_OK)
                        {
                            tuple->tab[internal->attr_out++] = value;
                            value = NULL;
                        }
                        else
                            MEfree((PTR)value);
                    }
                }
            }

            if (current < internal->position)
            {
                MECOPY_VAR_MACRO(internal->buffer + current,
                                                 internal->position - current,
                                                 internal->buffer);
                internal->position -= current;
            }
            else
                internal->position = 0;
        }
    }
    return(err);
}

/*
** Name: C_APIReadClient
**
** Description:
**      Format a buffer for sending to the server.  Used to mimic the
**      multipart file tranmission from a HTTP server.
**
** Inputs:
**      len         available size of buffer
**      buffer      start of the buffer
**      client      ice client parameters
**
** Outputs:
**      len         remaining length of buffer
**
** Returns:
**      GSTAT_OK    successfull completion
**      other       failed
**
** History:
**      03-Feb-1999 (fanra01)
**          Corrections to upload protocol and length calculations.
*/
GSTATUS
C_APIReadClient(
    u_i4 *len,
    char *buffer,
    PICEClient client) 
{
    GSTATUS                 err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL*  internal;
    char*                   tmp = buffer;
    i4                      available = *len;
    i4                      length = 0;
    STATUS                  status;

    internal = (ICE_C_CLIENT_INTERNAL*) client->user_info;

    /*
    ** If the file handle is still valid we are still sending the blob
    ** file.  Continue reading.
    */
    if (internal->file != NULL)
    {
        status = SIread(internal->file, available, &length, buffer);
        if (status == ENDFILE)
        {
            SIclose(internal->file);
            internal->file = NULL;
            available -= length;
            tmp += length;
            /*
            ** append a carriage return and line feed character as
            ** "\r\n--" indicates starting boundary and file ended.
            */
            STprintf (tmp, "\r\n");
            tmp+= 2;
        }
        else
        {
            /*
            ** Read buffer must be completely filled.
            */
            available = 0;
            G_ASSERT(status, status);
        }
    }

    if (available > 0)
    {
        char        boundary[256];
        char*        name = NULL;
        char*        value = NULL;
        i4             type;

        i4      formatLength = 0;

        /*
        ** Format parameters according to RFC's 1521 and 1867.
        */
        STprintf(boundary, BOUNDARY, internal);
        formatLength += STlength(boundary);
        while (internal->params->name != NULL)
        {
            /*
            ** Take values for current param entry
            */
            type = internal->params->type;
            name = internal->params->name;
            if (internal->params->value != NULL)
                value = internal->params->value;
            else
                value = "";

            /*
            ** Bump to next entry
            */
            internal->params++;
            length = 0;

            /*
            ** Calculate length of formatted attribute
            */
            if ((type & ICE_IN) || (type & ICE_BLOB))
            {
                length += formatLength + STlength(name) + STlength(value);
                length += (type & ICE_BLOB) ? STlength (FORMAT_BLOB) :
                    STlength (FORMAT_ATTR);
            }
            if (type & ICE_OUT)
            {
                length += formatLength + STlength(name) + STlength(OUT_PARAM) +
                    STlength (FORMAT_ATTR);
            }

            if (length > available)
            {
                /*
                ** Put parameter back as not added to buffer yet.
                */
                internal->params--;
                break;
            }
            else
            {
                if (type & ICE_OUT)
                {
                    STprintf(tmp, FORMAT_ATTR, boundary, OUT_PARAM, name, "");
                    length = STlength(tmp);
                    available -= length;
                    tmp += length;
                }

                if (type & ICE_BLOB)
                {
                    LOCATION    loc;
                    STprintf(tmp, FORMAT_BLOB, boundary, name, value);
                    length = STlength(tmp);
                    available -= length;
                    tmp += length;
                    LOfroms(PATH & FILENAME, value, &loc);
                    status = SIfopen(&loc, "r", SI_BIN, 0, &internal->file);
                    G_ASSERT(status, status);
                    *len -= available;
                    err = C_APIReadClient((u_i4*)&available, tmp, client);
                    *len += available;
                    return(err);
                }
                else
                {
                    STprintf(tmp, FORMAT_ATTR,  boundary, "", name, value);
                    length = STlength(tmp);
                    available -= length;
                    tmp += length;
                }
            }
        }

        if (internal->params->name == NULL)
        {
            length =    STlength(boundary) + STlength(FORMAT_END);

            if (length < available)
            {
                STprintf(tmp, FORMAT_END, boundary);
                available -= length;
                tmp += length;
            }
        }
    }
    *len -= available;
    return(err);
}

/*
** Name: ICE_C_Initialize
**
** Description:
**      Perform initialization from the interface.  This function should only
**      be called once in the application.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK          successful completion
**      !0          error status.
**
** History:
**      24-Mar-1999 (fanra01)
**          Add history.
**          Ensure that initialize is performed only once.
*/
ICE_STATUS
ICE_C_Initialize()
{
    ICE_STATUS status = OK;
    bool clientstatus;

    if (ICE_C_Initialized == FALSE)
    {
        ICE_C_Initialized = TRUE;
        if ((clientstatus = ICEClientInitialize()) == FALSE)
        {
            status = FAIL;
        }
    }
    return (status);
}

/*
** Name: ICE_C_Connect
**
** Description:
**      Connect to the specified node using the provided user name and
**      password.
**      Forms an action string for connect and issues the query.
**
** Inputs:
**      node        Contains the ice defined node to connect to.
**                  a NULL node will connect to the default ice database.
**      user        db user
**      password    db user password
**
** Outputs:
**      None.
**
** Returns:
**      OK          successful completion
**      !0          error status.
**
** History:
**      12-Jun-2000 (fanra01)
**          Bug 101345
**          Add empty string check to node.
*/
ICE_STATUS
ICE_C_Connect(
    II_CHAR*        node,
    II_CHAR*        user,
    II_CHAR*        password,
    ICE_C_CLIENT    *client)
{
    GSTATUS err = GSTAT_OK;
    i4  length = 0;
    ICE_C_CLIENT_INTERNAL *internal = NULL;

    if (user == NULL || user[0] == EOS)
        err = DDFStatusAlloc( E_DF0044_DDS_BAD_USER );

    if (err == GSTAT_OK && password == NULL)
        err = DDFStatusAlloc( E_DF0043_DDS_BAD_PASSWORD );

    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM(0, internal, ICE_C_CLIENT_INTERNAL, 1);
        if (err == GSTAT_OK)
        {
            internal->attr_out_count = -1;
            if ((node != NULL) && (*node != EOS))
            {
                length = STlength(node) + 1;
                err = G_ME_REQ_MEM(0, internal->node, char, length);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(node, length, internal->node);
                }
            }
            internal->ice_client.user_info = (PTR) internal;
            internal->ice_client.Host = "C client";
            internal->ice_client.gatewayType = CGI_TYPE;
            internal->ice_client.agent = API_AGENT;

            internal->ice_client.initPage = C_APIInitPage;
            internal->ice_client.sendURL = C_APISendURL;
            internal->ice_client.write = C_APIWriteClient;
            internal->ice_client.read = C_APIReadClient;

            length = STlength (user) + STlength (HVAR_USERID) +
                STlength (password) + STlength (HVAR_PASSWORD) + 25;

            if (err == GSTAT_OK)
                err = G_ME_REQ_MEM(0, internal->ice_client.QueryString, char, length);
            if (err == GSTAT_OK)
            {
                STprintf(internal->ice_client.QueryString,
                    "ii_action=connect&%s=%s&%s=%s",
                    HVAR_USERID,
                    user,
                    HVAR_PASSWORD,
                    password);
                err = ICEQuery (C_API, internal->node, &internal->ice_client);
                MEfree((PTR)internal->ice_client.QueryString);
                internal->ice_client.QueryString = NULL;
            }
        }
    }

    *client = (ICE_C_CLIENT) internal;
    if (err != GSTAT_OK)
    {
        STATUS number = err->number;
        ICE_C_SetError(*client, &err);
        return(number);
    }
    return(OK);
}

/*
** Name: ICE_C_ConnectAs
**
** Description:
**      Connect to the specified node using the provided user name and
**      password.
**      Forms an action string for connect and issues the query.
**
** Inputs:
**      node        Contains the ice defined node to connect to.
**                  a NULL node will connect to the default ice database.
**      user        db user
**      password    db user password
**
** Outputs:
**      None.
**
** Returns:
**      OK          successful completion
**      !0          error status.
**
** History:
**      18-Apr-2001 (fanra01)
**          Created
*/
ICE_STATUS
ICE_C_ConnectAs(
    II_CHAR*        node,
    II_CHAR*        cookie,
    ICE_C_CLIENT    *client)
{
    GSTATUS err = GSTAT_OK;
    i4  length = 0;
    ICE_C_CLIENT_INTERNAL *internal = NULL;

    if (err == GSTAT_OK && cookie == NULL)
        err = DDFStatusAlloc( E_DF0065_DDS_INVALID_SESSION );

    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM(0, internal, ICE_C_CLIENT_INTERNAL, 1);
        if (err == GSTAT_OK)
        {
            internal->attr_out_count = -1;
            if ((node != NULL) && (*node != EOS))
            {
                length = STlength(node) + 1;
                err = G_ME_REQ_MEM(0, internal->node, char, length);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(node, length, internal->node);
                }
            }
            internal->ice_client.user_info = (PTR) internal;
            internal->ice_client.Host = "C client";
            internal->ice_client.gatewayType = CGI_TYPE;
            internal->ice_client.agent = API_AGENT;

            err = GAlloc( &internal->ice_client.Cookies,
                STlength( cookie ) + 1, FALSE );
            if (err == GSTAT_OK)
            {
                MECOPY_VAR_MACRO(cookie, STlength( cookie )+1,
                    internal->ice_client.Cookies);
            }
            internal->ice_client.initPage = C_APIInitPageAs;

            internal->ice_client.sendURL = C_APISendURL;
            internal->ice_client.write = C_APIWriteClient;
            internal->ice_client.read = C_APIReadClient;
        }
    }

    *client = (ICE_C_CLIENT) internal;
    if (err != GSTAT_OK)
    {
        STATUS number = err->number;
        ICE_C_SetError(*client, &err);
        return(number);
    }
    return(OK);
}

/*
** Name: ICE_C_Close
**
** Description:
**      Frees the resources created for the tuple data during fetch.
**
** Inputs:
**      client      pointer to the internal client structure.
**
** Outputs:
**      None.
**
** Returns:
**      OK          successful completion
**      !0          error status.
**
** History:
**      08-Mar-1999 (fanra01)
**          Check validity of pointer before trying to free it.  It is possible
**          to have gaps in the array if not all function attributes has been
**          requested.
*/
ICE_STATUS
ICE_C_Close(
    ICE_C_CLIENT    client)
{
    GSTATUS err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client;
    ICE_TUPLE            *tuple;
    i4                             i;

    if (internal->attributes != NULL)
    {
        for (i = 0; i < internal->attr_count; i++)
        {
            if (internal->attributes[i] != NULL)
            {
                MEfree((PTR)internal->attributes[i]);
            }
        }
        MEfree((PTR)internal->attributes);
    }
    internal->attributes = NULL;
    MEfree((PTR)internal->cross);
    internal->cross = NULL;
    MEfree((PTR)internal->buffer);
    internal->buffer = NULL;

    internal->last = NULL;
    while (internal->first != NULL)
    {
        tuple = internal->first;
        internal->first = internal->first->next;
        for (i = 0; i < internal->attr_out_count; i++)
            MEfree((PTR)tuple->tab[i]);

        MEfree((PTR)tuple->tab);
        MEfree((PTR)tuple);
    }

    internal->colhead = 0;
    internal->attr_count = 0;
    internal->attr_out = 0;
    internal->attr_out_count = -1;
    internal->read = 0;
    internal->count = 0;
    internal->error = 0;
return(0);
}

/*
** Name: ICE_C_Disconnect
**
** Description:
**      Issues a disconnect action to the server and frees resources.
**
** Inputs:
**      client      pointer to the internal client structure.
**
** Outputs:
**      None.
**
** Returns:
**      OK          successful completion
**      !0          error status.
**
** History:
**      23-Mar-1999 (fanra01)
**          Correct statement to use ii_action for variable name.
**      26-Jul-2000 (fanra01)
**          Ensure that the parameter length is set to 0 as
**          upload will have set the length.
*/
ICE_STATUS
ICE_C_Disconnect(
    ICE_C_CLIENT    *client)
{
    GSTATUS err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) *client;

    if (internal != NULL)
    {
        char statement[] = "ii_action=disconnect";
        internal->error = 0;
        internal->ice_client.cbAvailable = 0;
        internal->ice_client.QueryString = statement;
        CLEAR_STATUS( ICEQuery (C_API, internal->node, &internal->ice_client) );
        ICE_C_Close(*client);
        internal->ice_client.QueryString = NULL;
        MEfree((PTR)internal->node);
        MEfree((PTR)internal->ice_client.Cookies);
    }
    *client = NULL;

    if (err != GSTAT_OK)
    {
        STATUS number = err->number;
        ICE_C_SetError(*client, &err);
        return(number);
    }
    return(OK);
}


GSTATUS
ICE_C_ExecuteDownload(
    ICE_C_CLIENT_INTERNAL *internal,
    ICE_C_PARAMS    tab[])
{
    GSTATUS err = GSTAT_OK;
    i4         count = 0;
    char*        target = NULL;
    char        params[256];
    params[0] = EOS;

    while (tab[count].name != NULL)
    {
        if (STcompare(tab[count].name, "unit") == 0)
            STprintf(params, "ii_unit=%s", tab[count].value);
        else if (STcompare(tab[count].name, "document") == 0)
            internal->ice_client.PathInfo = tab[count].value;
        else if (STcompare(tab[count].name, "target") == 0)
                target = tab[count].value;
        count++;
    }

    if (target != NULL)
    {
        LOCATION    loc;
        STATUS    status;

        LOfroms(PATH & FILENAME, target, &loc);
        status = SIfopen(&loc, "w", SI_BIN, 0, &internal->file);
        G_ASSERT(status, status);

        STprintf(params, "%s&ii_action=download", params);
        internal->ice_client.cbAvailable = STlength(params);
        internal->ice_client.TotalBytes = internal->ice_client.cbAvailable;
        internal->ice_client.Data = params;
        err = ICEQuery (C_API, internal->node, &internal->ice_client);
        internal->read = 0;
        internal->ice_client.cbAvailable = 0;
        internal->ice_client.TotalBytes = 0;
        internal->ice_client.PathInfo = NULL;
        internal->ice_client.Data = NULL;

        SIclose(internal->file);
        internal->file = NULL;
    }
    return(OK);
}

/*
** Name: ICE_C_ExecuteFunction
**
** Description:
**
**
**
** Inputs:
**      internal    API working parameters
**      name        function name
**      tab         table of function parameters
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    successfull completion
**      other       failed
**
** History:
**      03-Feb-1999 (fanra01)
**          Corrections to upload protocol and length calculations.
**      11-Mar-1999 (fanra01)
**          Correction to length of variables and values.  Remove single
**          increment as end of string is not required.
**          Extra length causes protocol violation in the ice server as an
**          unexpected boundary is sent.
*/
GSTATUS
ICE_C_ExecuteFunction(
    ICE_C_CLIENT_INTERNAL   *internal,
    char*                   name,
    ICE_C_PARAMS            tab[])
{
    GSTATUS     err = GSTAT_OK;
    i4          count = 0;
    bool        withBlob = FALSE;
    i4          length = 0;

    /*
    ** Determine if a blob is included in the current request
    */
    internal->attr_out_count = -1;
    while (tab[count].name != NULL)
    {
        if (tab[count].type & ICE_BLOB)
            withBlob = TRUE;
        count++;
    }
    internal->attr_count = count;

    if (withBlob == TRUE)
    {
        char        boundary[256];
        char*       name = NULL;
        char*       value = NULL;

        /*
        ** Calculate the length of message to send to the server.
        */
        i4      attrLength = STlength(FORMAT_ATTR) - 8;
        i4      blobLength = STlength(FORMAT_BLOB) - 6;

        STprintf(boundary, BOUNDARY, internal);
        length = STlength(boundary);
        attrLength += STlength(boundary);
        blobLength += STlength(boundary);
        length += STlength(MULTIPART) + 1;
        err = G_ME_REQ_MEM(0, internal->ice_client.ContentType, char, length);
        if (err == GSTAT_OK)
        {
            STprintf(internal->ice_client.ContentType, "%s%s",
                MULTIPART, boundary);

            length = STlength(boundary) + STlength(FORMAT_END);
            count = 0;
            while (count < internal->attr_count)
            {
                if (tab[count].type & ICE_OUT)
                {
                    length += attrLength + STlength(name) +
                        STlength(OUT_PARAM) + 1;
                }
                if (tab[count].type & ICE_IN)
                {
                    name = tab[count].name;
                    if (tab[count].value != NULL)
                        value = tab[count].value;
                    else
                        value = "";

                    if (tab[count].type & ICE_BLOB)
                    {
                        if (tab[count].value != NULL)
                        {
                            LOINFORMATION   loinf;
                            i4              flagword;
                            LOCATION        loc;

                            LOfroms(PATH & FILENAME, tab[count].value, &loc);
                            flagword = (LO_I_SIZE);
                            if (LOinfo(&loc, &flagword, &loinf) == OK)
                            {
                                length += blobLength +
                                    STlength(name) +
                                    STlength(value) +
                                    loinf.li_size;
                            }
                        }
                    }
                    else
                        length += attrLength + STlength(name) + STlength(value);
                }
                count++;
            }

            /*
            ** Read and format the first block the API will send
            */
            err = G_ME_REQ_MEM(0, internal->buffer, char, BUFFER_SIZE);
            if (err == GSTAT_OK)
            {
                internal->ice_client.cbAvailable = BUFFER_SIZE;
                internal->params = tab;
                err = C_APIReadClient((u_i4 *)&internal->ice_client.cbAvailable,
                    internal->buffer,
                    &internal->ice_client);
            }
        }
    }
    else
    {
        count = 0;
        while (count < internal->attr_count)
        {
            if (tab[count].type & ICE_IN)
            {
                length += STlength(tab[count].name) + 2;
                if (tab[count].value != NULL)
                    length += STlength(tab[count].value);
            }
            if (tab[count].type & ICE_OUT)
                length += STlength(tab[count].name) + 2 + STlength(OUT_PARAM);
            count++;
        }

        err = G_ME_REQ_MEM(0, internal->buffer, char, length);
        if (err == GSTAT_OK)
        {
            count = 0;
            while (err == GSTAT_OK && count < internal->attr_count)
            {
                if (tab[count].type & ICE_IN)
                {
                    if (internal->buffer[0] != EOS)
                        STcat(internal->buffer, "&");

                    STprintf ( internal->buffer,
                        "%s%s=%s",
                        internal->buffer,
                        tab[count].name,
                        (tab[count].value) ? tab[count].value : "");
                }

                if (tab[count].type & ICE_OUT)
                {
                    if (internal->buffer[0] != EOS)
                        STcat(internal->buffer, "&");

                    STprintf ( internal->buffer,
                        "%s%s%s=",
                        internal->buffer,
                        OUT_PARAM,
                        tab[count].name);
                }
                count++;
            }
        }
        if (length > 0) length--;
        internal->ice_client.cbAvailable = length;
    }

    if (err == GSTAT_OK)
    {
        err = G_ME_REQ_MEM(0, internal->attributes, char*, count);
        if (err == GSTAT_OK)
            err = G_ME_REQ_MEM(0, internal->cross, i4 , count);
        if (err == GSTAT_OK)
        {
            count = 0;
            while (err == GSTAT_OK && count < internal->attr_count)
            {
                internal->cross[count] = -1;
                if (tab[count].type & ICE_OUT)
                {
                    i4  nlen = STlength(tab[count].name) + 1;
                    err = G_ME_REQ_MEM(0, internal->attributes[count], char, nlen);
                    if (err == GSTAT_OK)
                        MECOPY_VAR_MACRO(tab[count].name,
                            nlen,
                            internal->attributes[count]);
                }
                count++;
            }
        }
    }

    if (err == GSTAT_OK)
    {
        internal->ice_client.TotalBytes = length;
        internal->ice_client.PathInfo = name;
        internal->ice_client.Data = internal->buffer;
        err = ICEQuery (C_API, internal->node, &internal->ice_client);
        internal->read = 0;
        MEfree((PTR)internal->ice_client.ContentType);
        internal->ice_client.ContentType = NULL;
        internal->ice_client.cbAvailable = 0;
        internal->ice_client.TotalBytes = 0;
        internal->ice_client.PathInfo = NULL;
        internal->ice_client.Data = NULL;
        internal->params = NULL;

    }
    return(err);
}

ICE_STATUS
ICE_C_Execute(
    ICE_C_CLIENT    client,
    II_CHAR*        name,
    ICE_C_PARAMS    tab[])
{
    GSTATUS err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client;

    if (client != NULL)
    {
        ICE_C_Close(client);
        internal->error = 0;
        if (STcompare(name, "download") == 0)
            err = ICE_C_ExecuteDownload(internal, tab);
        else
            err = ICE_C_ExecuteFunction(internal, name, tab);
        if (err != GSTAT_OK)
        {
            STATUS number = err->number;
            ICE_C_SetError(client, &err);
            return(number);
        }
    }
    return(OK);
}

ICE_STATUS
ICE_C_Fetch(
    ICE_C_CLIENT    client,
    II_INT4*        end)
{
    GSTATUS err = GSTAT_OK;
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client;

    if (internal == NULL)
        return(OK);

    if (internal->error != OK)
        return(internal->error);

    if (internal->read < internal->count)
    {
        if (internal->read > 0)
        {
            ICE_TUPLE    *tmp;
            i4  i;

            tmp = internal->first;
            internal->first = internal->first->next;
            for (i = 0; i < internal->attr_out_count; i++)
                MEfree((PTR)tmp->tab[i]);
            MEfree((PTR)tmp->tab);
            MEfree((PTR)tmp);
        }
        internal->read++;
        *end = FALSE;
    }
    else
        *end = TRUE;

    if (err != GSTAT_OK)
    {
        STATUS number = err->number;
        ICE_C_SetError(client, &err);
        return(number);
    }
    return(OK);
}

/*
** Name: ICE_C_GetAttribute
**
** Description:
**      Retrieve an individual value from the result set.
**
** Inputs:
**      client      pointer to the internal client structure.
**      position    ICE_C_PARAMS array index of the output parameter.
**
** Outputs:
**      None.
**
** Returns:
**      NULL        no value available
**      !NULL       requested value.
**
**                  There is no error return.
**
** History:
**      29-Apr-1999 (fanra01)
**          Changed test for position parameter
*/
char*
ICE_C_GetAttribute(
    ICE_C_CLIENT    client,
    II_INT4         position)
{
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client;

    if (internal == NULL && internal->error != OK)
        return(NULL);

    /*
    ** Check the requested position is within array bounds and is a requested
    ** output value.
    */
    if ((position < internal->attr_count) &&
        (internal->attributes[position] != NULL) &&
        (*(internal->attributes[position]) != EOS))
    {
        i4  cross_position = internal->cross[position];
        if (cross_position != -1)
            return(internal->first->tab[cross_position]);
        else
            return(NULL);
    }
    else
        return(NULL);
}

/*
** Name: ICE_C_LastError
**
** Description:
**      Returns the text message for the last error.
**
** Inputs:
**      client      pointer to the internal client structure.
**
** Outputs:
**      None.
**
** Returns:
**      NULL        no error text available
**      !NULL       error text
**
** History:
**      28-Feb-2000 (fanra01)
**          Commented.
*/
char*
ICE_C_LastError(
    ICE_C_CLIENT    client)
{
    ICE_C_CLIENT_INTERNAL *internal = (ICE_C_CLIENT_INTERNAL *) client;

    if (internal != NULL && internal->error != 0)
        return(internal->buffer);
    else
        return(NULL);
}

/*
** Name: ICE_C_Terminate
**
** Description:
**      Function invokes client termination to disconnect active sessions.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      28-Feb-2000 (fanra01)
**          Created.
**      02-Mar-2000 (fanra01)
**          Reinitialise the ICE_C_Initialized flag.
**
*/
VOID
ICE_C_Terminate()
{
    if (ICE_C_Initialized == TRUE)
    {
        ICEClientTerminate();
        ICE_C_Initialized = FALSE;
    }
    return;
}
