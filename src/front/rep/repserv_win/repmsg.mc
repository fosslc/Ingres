;/*
;** Copyright (c) 1996, 1997 Ingres Corporation
;*/
;/*
;** Name: repmsg.mc
;**
;** Description:
;**         Message file for replicator specific message to appear in the event
;**         log.
;**
;** History:
;**         25-Jun-97 (fanra01)
;**             Created.
;**         06-Aug-97 (fanra01)
;**             Modified to output error text on failure.
;*/

FacilityNames=(
    System=0x0FF
    OpenIngres=0x5FF
    )
LanguageNames=(English=1:MSG00001)


MessageId = 100
Severity = error
Facility = OpenIngres
SymbolicName = E_REP_FAILED

Language=English
An operation has failed. %1
%2
.

MessageId =
Severity = success
Facility = OpenIngres
SymbolicName = E_REP_SUCCEDED

Language=English
Operation has completed. %1
.
