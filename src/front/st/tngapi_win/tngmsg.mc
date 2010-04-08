;/*
;** Copyright (C) 1996, 2005 Ingres Corporation
;*/
;/*
;** Name: tngmsg.mc
;**
;** Description:
;**     Message file for error messages that appear in the pre-installation
;**     check api.
;**
;** History:
;**     26-Jun-2002 (fanra01)
;**         Sir 108122
;**         Created.
;**     09-Sep-2004 (fanra01)
;**         Add message for path length exceeded.
;**     20-Jan-2005 (fanra01)
;**         Sir 113777
;**         Add error messages for Ingres status message retrieval.
;**     17-Oct-2005 (fanra01)
;**         Sir 115396
;**         Add error messages for invalid instance code.
;*/

FacilityNames=(
    System=0x0FF
    Ingres_Database=0x5FF
    )
LanguageNames=(English=1:MSG00001)


MessageId = 2000
Severity = error
Facility = Ingres_Database
SymbolicName = E_NULL_PARAM

Language=English
Invalid (NULL) parameter passed to II_PrecheckInstallation
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_NO_INSTALL_PATH

Language=English
II_SYSTEM not defined in the response file
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_OS_NOT_MIN_VER

Language=English
The operating system does not meet the minimum requirements
.
MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_BAD_PATH

Language=English
The installation path cannot be found
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_PATH_NOT_DIR

Language=English
The installation path is not a directory
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_PATH_CANNOT_WRITE

Language=English
The user does not have write permission on the installation path
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_INVAL_CHARS_IN_PATH

Language=English
The installation path contains characters that are not supported
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_NO_CHAR_MAP

Language=English
charmap field is not defined in the response file
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_CHARMAP_ERROR

Language=English
Error accessing the requested character map
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_GET_COMPUTER_FAIL

Language=English
Unable to retrieve computer name
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_GET_HOST_FAIL

Language=English
Unable to retrieve host name
.

MessageId =
Severity = warning
Facility = Ingres_Database
SymbolicName = E_UNMATCHED_NAME

Language=English
Configured computer name does not match the host name
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_INVALID_HOST

Language=English
The configured computer name contains characters that are not supported
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_GET_USER_FAIL

Language=English
Unable to retrieve user name
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_INVALID_USER

Language=English
The configured user name contains characters that are not supported
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_PATH_LEN_EXCEEDED

Language=English
The length of the installation path has exceeded the permitted maximum
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_INSUFFICIENT_BUFFER

Language=English
An insufficient message area has been provided for an Ingres status message
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_GET_MESSAGE_ERROR

Language=English
An error occurred during the retrieval of an Ingres status message
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_INVALID_INSTANCE

Language=English
An invalid character or sequence of characters has been provided as an instance code.
.

MessageId =
Severity = error
Facility = Ingres_Database
SymbolicName = E_INSTANCE_ERROR

Language=English
An error occurred reading the instance code.
.
