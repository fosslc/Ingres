/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : lexical.h
//
//    
//
********************************************************************/

#ifndef LEXICAL_HEADER
#define LEXICAL_HEADER

#include "dba.h"

/*
//
// OBJECT TYPES ARE THE FOLLOWING TYPES:
//
// OT_UNKNOWN
// OT_TABLE 
// OT_DATABASE
//
*/



BOOL IsObjectNameValid (const char* object, int object_type);
BOOL IsIntegerNumber (const char* stringNumber, long Min, long Max);
int GetString (int pos, const char* aString, char* buf);


#endif

