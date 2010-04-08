;/*
;** Copyright (C) 1996, 1998 Ingres Corporation All Rights Reserved.
;*/
;/*
;** Name: ingmsg.mc
;**
;** Description:
;**         Message file for service specific message to appear in the event
;**         log.
;**
;** History:
;**         16-feb-98 (mcgem01)
;**             Created.
;**         29-Nov-2001 (fanra01)
;**             Bug 106529
;**             Add warning and information messages for use when reporting
;**             events from ERsend.
;*/

FacilityNames=(
    System=0x0FF
    OpenIngres_Database=0x5FF
    )
LanguageNames=(English=1:MSG00001)


MessageId = 200
Severity = error
Facility = OpenIngres_Database
SymbolicName = E_ING_FAILED

Language=English
An operation has failed. %1
%2
.

MessageId =
Severity = success
Facility = OpenIngres_Database
SymbolicName = E_ING_SUCCEDED

Language=English
Operation has completed. %1
.

MessageId =
Severity = warning
Facility = OpenIngres_Database
SymbolicName = I_ING_WARN

Language=English
Message: %1
.
MessageId =
Severity = informational
Facility = OpenIngres_Database
SymbolicName = I_ING_INFO

Language=English
Message: %1
.
