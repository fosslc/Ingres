/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fileioex.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation Langage of IVM (exec process with redirect input:output)
**
** History:
**
** 07-Jul-2000 (uk$so01)
**    Created for BUG #102010
**/

#if ! defined (FILE_IO_EXEC_REDIRECT_INPUT_OUTPUT)
#define FILE_IO_EXEC_REDIRECT_INPUT_OUTPUT

//
// Excute the process using files or buffer to 
// redirect input/output.
class CaFileIOExecParam;
BOOL FILEIO_ExecCommand (CaFileIOExecParam* pParam);

#define INPUT_ASFILE    0x0001  // Use file as standard input to the child process
#define OUTPUT_ASFILE   0x0002  // Use file as standard output to the child process
class CaFileIOExecParam
{
public:
	CaFileIOExecParam()
	{
		m_strCommand   = _T("");
		m_strInputArg  = _T("");
		m_strOutputArg = _T("");
		m_nFlag = 0;
		m_nErrorCode = 0;
		m_strError = _T("");
	}

	~CaFileIOExecParam(){}


	CString m_strCommand;    // Executable name
	CString m_strInputArg;   // Input argument default as String  (can be empty). Append with '\n' if needed.
	CString m_strOutputArg;  // Output argument default as String (out parameter)

	//
	// m_nFlag is bitwise '|' operation and can be
	//   Default = 0. 
	//   INPUT_ASFILE:  m_strInputArg will be treated as File Name
	//   OUTPUT_ASFILE: m_strOutputArg will be treated as File Name to contain the result
	UINT m_nFlag;

	//
	// m_nErrorCode =
	//  -1: Generic error
	//   0: No error.
	//   1: Cannot access input file.
	//   2: Cannot open input file.
	//   3: Cannot create output file
	//   4: Fail to create pipe
	//   5: Fail to create process
	//   6: Cannot write to pipe.
	//   7: Cannot read from pipe.
	UINT m_nErrorCode;
	CString m_strError; // Additional error message if any.
	
};


#endif // FILE_IO_EXEC_REDIRECT_INPUT_OUTPUT
