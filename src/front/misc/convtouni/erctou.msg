/*
**  Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/**
** Name:        erctou.msg -      Message file for class CU
**
** Description:
**      This file defines the messages for class CU.
**
** History:
**	29-Apr-2004 (gupsh01)
**	    Created.
**	24-Jun-2004 (gupsh01)
**	    Formatted error messages. 
**/

# define        _CT_CLASS       299

F_CT0001_CONVTOUNI
"CONVTOUNI"

S_CT0005_Completed_CONVTOUNI_a
"\nCONVTOUNI has created executable CTOUEXEC and 
\n sql script %0c. 
\n Please examine the sql file and run 
\n CTOUEXEC to finish conversion to unicode.\n"

S_CT0006_Completed_CONVTOUNI_b
"\nCONVTOUNI has created executable 'ctouexec.bat' and
\n sql script %0c. 
\n Please examine the sql script and run
\n 'ctouexec.bat' to finish conversion to unicode.\n"

S_CT0007_Completed_CONVTOUNI_i
"\nCONVTOUNI has created executable 'ctouexec.ing' and 
\n sql script %0c. 
\n Please examine the sql script and run
\n 'ctouexec.ing' to finish conversion to unicode.\n"

S_CT0008_Executing_script
"\nExecuting convtouni generated script %0c ...\n"

S_CT0010_Execution_completed
"\nExecution of convtouni successful.\n"

E_CT0025_Bad_paramfile
"The -param_file=filename flag should be the only option specified.\n
any other flags should be specified in the parameter file."
/*%
** Explanation:
**    Convtouni can accept parameters from a parameter file containing
**    input parameters listed one per line. When this flag is specified
**    do not provide any other flags or options. All the flags should be
**    in the parameter file.
**
** System Status:
**    Program terminates.
**
** User Action:
**    Remove the extra flags and specify then inside the parameter file.
*/
E_CT0026_Out_of_memory
"Out of memory - attempt to allocate memory failed.\n"
/*
** Explanation:
**    Convtouni failed to acquire memory.
**
** System Status:
**    Program terminates.
**
** User Action:
**    Determine why system resourcees are running low.
*/
