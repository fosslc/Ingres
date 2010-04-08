/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: setenv.h
**
**  Description:
**	Set an Ingres variable (like ingsetenv).
**
**  History:
**	07-jun-2004 (wonst02)
**	    Created.
*/
#pragma once
# include <compat.h>
# include <er.h>
# include <nm.h>

class WcSetEnv
{
public:
	WcSetEnv(void);
	~WcSetEnv(void);
	// Set an Ingres variable (like ingsetenv)
	bool Set(char * name, char * value);
};
