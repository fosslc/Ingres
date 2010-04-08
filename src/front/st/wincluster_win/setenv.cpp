/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: setenv.cpp
**
**  Description:
**	Set an Ingres variable (like ingsetenv).
**
**  History:
**	07-jun-2004 (wonst02)
**	    Created.
*/
#include ".\setenv.h"

WcSetEnv::WcSetEnv(void)
{
}

WcSetEnv::~WcSetEnv(void)
{
}

// Set an Ingres variable (like ingsetenv)
bool WcSetEnv::Set(char * name, char * value)
{
	return (NMstIngAt(name, value) == OK);
}
