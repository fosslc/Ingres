/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    qg.msg -	Query Generator Module Error Message File.
**
** Description:
**	Contains the diagnostic error message definitions for the QG module.
**
**	Note:  Except all errors less than E_QG0010 are DIAGNOSTIC messages
**	and do not need to be translated.
**
** History:
**	Revision 6.0  87/08/04  wong
**	Initial revision.
**	23-oct-1990 (mgw)
**		corrected a misspelling in a message.
**	16-jan-94 (davel)
**		Added E_QG0012_Unsupported to flag unsupprted QG features.
**/

# define	_QG_CLASS	134

E_QG0001_No_Internal	"Internal program error:  Missing data structures.\n
The QG module is missing an internal data structure.
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	The query generator module was called to break out of a query
**	for which no internal data structure had been set-up.  This is
**	an internal programming problem.
**
** System Status:
**	No action.
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_QG0002_NoStart	"Internal program error:  No QG_START operation.\n
The QG module was called to fetch a row without a query having been started.
This is an internal problem resulting from incorrect query generator usage.
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	The query generator module was called to fetch a row without a query
**	having been started.  The query generator module should be called to
**	start a query before calling it to fetch a row.  This is an internal
**	programming problem.
**
** System Status:
**	No rows will be fetched.
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_QG0003_NoMemory	"Internal program error:  Memory allocation failure.\n
The QG module could not allocate any more memory.
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	The query generator module attempted to allocate some memory needed to
**	execute a query or fetch the data for a query, but could not.  This is
**	an internal programming problem.
**
** System Status:
**	The query will not be executed or no data will be fetched for the query.
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_QG0004_NoSpec		"Internal program warning:\n
Function did not generate any query specifications.
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	A function that the query generator module called to build a dynamic
**	query specification did not generate a query specification.  This is
**	usually not an error, but if you see the error message this is because
**	of an internal programming error.
**
** System Status:
**	An always true predicate will be generated, "1=1".
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_QG0005_BadVersion	"Internal program error:  Incompatible versions.\n
The QG module was called to execute a query that was incompatible with
the version of the module.  This is an internal problem resulting from
incorrect query generator usage.
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	The query generator module detected a version incompatibility in its
**	usage.  In particular, a version 0 query with '%a' format (meaning
**	DBDV data type) in its parameter list was passed to the QG module.
**	Queries with DBDV elements should be version 1, only.  This is an
**	internal programming problem.
**
** System Status:
**	The query will not be executed.
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_QG0006_BadSpec	"Internal program error:  Bad query specification.\n
The QG module found a bad query specification for the query being executed.
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	The query generator module detected an invalid query specification in
**	a query it was attempting to execute.  This is an internal programming
**	problem.
**
** System Status:
**	The query will not be executed.
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_QG0010_TempFile	"Could not open temporary file.\n
The QG module could not open a temporary file needed to execute a query.  Please
check that you have access rights and sufficient disk space available in the
temporary directory before reporting this as an internal problem to your
technical representative."

/*%
** Explanation:
**	The query generator module could not open a temporary file used to hold
**	data fetched for the query being executed.  This could be for any number
**	of reasons, but is most likely a disk space or access problem.
**
** System Status:
**	No data will be fetched for the query.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory and
**	sufficient disk space before reporting this internal problem to your
**	technical representative.
*/

E_QG0011_WriteFile	"Could not write to temporary file.\n
The QG module could not write to a temporary file needed to execute a query.\ 
Please check your temporary directory access rights and disk space before
reporting this as an internal problem to your technical representative."

/*%
** Explanation:
**	The query generator module could not write to a temporary file used to
**	hold data fetched for the query being executed.  This could be for any
**	number of reasons, but is most likely a disk space problem.
**
** System Status:
**	Query execution will be terminated with an error status.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory and
**	sufficient disk space before reporting this internal problem to your
**	technical representative.
*/

E_QG0012_Unsupported	"Unsupported Select syntax in Windows4GL.\n
The QG module encountered the internal Select option '%0c', which is 
unsupported.  Please report this as an internal problem to your technical 
representative."

/*%
** Explanation:
**	The query generator module was asked to perform executions that have
**	been removed for Windows4GL.
**
** System Status:
**	Query execution will be terminated with an error status.
**
** User Action:
**	Make sure that you are using documented Select statement syntax in
**	your Windows4GL application, and also please report this to your
**	technical representative.
*/
