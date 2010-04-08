SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _URS
**
**    History:
**	16-Jan-1998 wonst02
**	    Original Frontier Application Server messages
** 	30-Mar-1998 wonst02
** 		Added interface layer and data dictionary services.
*/

#define _UR_CLASS 233

/*
**  Exit errors -- these error codes are returned by specific URS
**  routines on failure.
**	0100 - 01FF
*/
E_UR0100_APPL_STARTUP:SS5000B_INTERNAL_ERROR
"The URSM could not start the application"

E_UR0101_ADD_PARM:SS5000B_INTERNAL_ERROR
"The URSM could not add a parameter to the application data set"

E_UR0102_BEGIN_PARM:SS5000B_INTERNAL_ERROR
"The URSM could not begin the application parameter data set"

E_UR0103_URD_DDG_INIT:SS5000B_INTERNAL_ERROR
"The URSM could not initialize data dictionary services"

E_UR0104_URD_DDG_DESCR_NULL:SS5000B_INTERNAL_ERROR
"The ursm_ddg_descr pointer is NULL."

E_UR0105_APPL_SHUTDOWN:SS5000B_INTERNAL_ERROR
"The URSM application encountered an error in urs_free_dll()\n
Status=%0d (0x%0x)"

E_UR0106_APPL_NOT_DEFINED:SS5000B_INTERNAL_ERROR
"No applications defined for the application server."

E_UR0107_URD_DDG_SESSION_NULL:SS5000B_INTERNAL_ERROR
"The data dictionary session pointer is NULL."

E_UR0110_INTFC_STARTUP:SS5000B_INTERNAL_ERROR
"The URSM application could not start the interface"

E_UR0115_INTFC_SHUTDOWN:SS5000B_INTERNAL_ERROR
"The URSM application encountered an error in AS_Terminate()\n
Status=%0d (0x%0x)"

E_UR0116_NO_INTFC_DEFINED:SS5000B_INTERNAL_ERROR
"No interfaces defined for the application, name=%0c"

E_UR0130_SRVR_SHUTDOWN:SS5000B_INTERNAL_ERROR
"The URSM application server did not shut down properly."

E_UR0131_SRVR_STARTUP:SS5000B_INTERNAL_ERROR
"The URSM application server did not start up properly."

E_UR0140_AS_ERROR:SS5000B_INTERNAL_ERROR
"The URSM interface encountered an error in AS_ExecuteMethod().\n
Status=%0d (0x%0x)"

E_UR0141_AS_UNKNOWN_INTERFACE:SS5000B_INTERNAL_ERROR
"The URSM interface encountered an error in AS_ExecuteMethod().\n
Unknown interface name=%0c."

E_UR0142_AS_UNKNOWN_METHOD:SS5000B_INTERNAL_ERROR
"The URSM interface encountered an error in AS_ExecuteMethod().\n
Unknown method name=%0c."

E_UR0143_AS_MEMORY_FAILURE:SS5000B_INTERNAL_ERROR
"The URSM interface encountered a memory failure in AS_ExecuteMethod()."

E_UR0144_AS_EXECUTE_ERROR:SS5000B_INTERNAL_ERROR
"The URSM interface could not execute the method via AS_ExecuteMethod()."

E_UR0150_METHOD_LOOKUP:SS5000B_INTERNAL_ERROR
"The URSM could not look up the method."

E_UR0151_NO_METHOD_DEFINED:SS5000B_INTERNAL_ERROR
"No methods defined for the interface, name=%0c"

E_UR0160_ARG_LOOKUP:SS5000B_INTERNAL_ERROR
"The URSM could not look up the argument."

E_UR0170_APPL_LOOKUP:SS5000B_INTERNAL_ERROR
"The URSM could not look up the application."

E_UR0180_INTFC_LOOKUP:SS5000B_INTERNAL_ERROR
"The URSM could not look up the interface."

/*
**  URS errors -- these errors are returned by generic URS routines on
**  failure.
**	0200 - 02FF
*/
E_UR0200_URS_INIT_ERROR:SS5000B_INTERNAL_ERROR
"The URSM did not initialize properly"

E_UR0201_URS_TERM_ERROR:SS5000B_INTERNAL_ERROR
"The URSM did not terminate properly"

E_UR0202_URS_APPL_FREE_ERROR:SS5000B_INTERNAL_ERROR
"The URSM did not free application memory properly"

E_UR0203_URS_FREE_PARM_ERR:SS5000B_INTERNAL_ERROR
"The URSM did not free application parameter list memory properly"

E_UR0204_URS_INIT_ALREADY:SS5000B_INTERNAL_ERROR
"Attempt to initialize the URSM more than once (urs_startup())\n
Urs_mgr_cb=0x%0x."

E_UR0205_URS_ADD_PARM_ERR:SS5000B_INTERNAL_ERROR
"The URSM did not add to the application parameter list properly"

E_UR0206_URS_BEGIN_PARM_ERR:SS5000B_INTERNAL_ERROR
"The URSM did not begin the application parameter list properly"

/*
**  Other facility errors -- error codes indicating failure of specific
**  function requests from other facilities.
**	0300 - 03FF
*/
E_UR0300_SCU_MALLOC_ERROR:SS5000B_INTERNAL_ERROR
"URSM error requesting memory (SCU_MALLOC); size requested was %0d pages."

E_UR0301_SCU_MFREE_ERROR:SS5000B_INTERNAL_ERROR
"URSM error freeing memory (SCU_MFREE)."

E_UR0303_MEFREE_ERROR:SS5000B_INTERNAL_ERROR
"URSM error freeing memory (MEfree)."

E_UR0310_ULM_STARTUP_ERROR:SS5000B_INTERNAL_ERROR
"URSM error on call to ulm_startup().  Size of pool requested is %0d bytes."

E_UR0311_ULM_SHUTDOWN_ERROR:SS5000B_INTERNAL_ERROR
"URSM error on call to ulm_shutdown()."

E_UR0312_ULM_OPENSTREAM_ERROR:SS5000B_INTERNAL_ERROR
"URSM error opening memory stream (ulm_openstream()).\n
memleft = %0d, sizepool = %1d, blocksize = %2d."

E_UR0313_ULM_CLOSESTREAM_ERROR:SS5000B_INTERNAL_ERROR
"URSM error closing memory stream (ulm_closestream()).  memleft = %0d."

E_UR0314_ULM_PALLOC_ERROR:SS5000B_INTERNAL_ERROR
"URSM error allocating memory (ulm_palloc()).\n
memleft = %0d, sizepool = %1d, blocksize = %2d, psize = %3d."

E_UR0315_ULM_OPENSTREAM_PALLOC:SS5000B_INTERNAL_ERROR
"URSM error opening memory stream and allocating memory (ulm_openstream()).\n
memleft = %0d, sizepool = %1d, blocksize = %2d, psize = %3d."

E_UR0330_CSP_SEMAPHORE_ERROR:SS5000B_INTERNAL_ERROR
"URSM error obtaining semaphore (CSp_semaphore())."

E_UR0331_CSV_SEMAPHORE_ERROR:SS5000B_INTERNAL_ERROR
"URSM error releasing semaphore (CSv_semaphore())."

E_UR0350_DDG_INIT_ERROR:SS5000B_INTERNAL_ERROR
"URSM error initializing data dictionary services (DDGInitialize()).\n
%0c"

E_UR0351_DDG_TERM_ERROR:SS5000B_INTERNAL_ERROR
"URSM error terminating data dictionary services (DDGTerminate()).\n
%0c"

E_UR0352_DDG_CONNECT_ERROR:SS5000B_INTERNAL_ERROR
"URSM error connecting to data dictionary services (DDGConnect()).\n
%0c"

E_UR0353_DDG_SELECT_ERROR:SS5000B_INTERNAL_ERROR
"URSM error selecting from data dictionary services (urs_get_appl()).\n
%0c"

E_UR0354_DDG_DISCONNECT_ERROR:SS5000B_INTERNAL_ERROR
"URSM error disconnecting from data dictionary services (DDGDisconnect()).\n
%0c"

E_UR0371_DLL_UNLOAD:SS5000B_INTERNAL_ERROR
"URSM error unloading application server DLL.\n
DLunload(), status=%0d (0x%0x), handle=0x%1x."

E_UR0372_DLL_LOAD:SS5000B_INTERNAL_ERROR
"URSM error loading application server DLL.\n
DLprepare(), status=%0d (0x%0x), DLL=%1c."

E_UR0373_DLL_BIND:SS5000B_INTERNAL_ERROR
"URSM error binding entry point for DLL=%2c.\n
DLbind(), status=%0d (0x%0x), name=%1c."

E_UR0380_API_CNVT_MEMORY:SS5000B_INTERNAL_ERROR
"Memory allocation failure calling cnvtDataValue2GCA()."

/*
**  Bad parameter errors -- used for sanity checks on parameters, control block
**  values, etc.  These are usually coding errors.
**	0400 - 04FF
*/
E_UR0400_UNKNOWN_DATATYPE:SS50000_MISC_ERRORS
"The GCA datatype %0d (0x%0x) is unknown to URSM BuildParm"

E_UR0401_URS_ADD_PARM_USAGE:SS50000_MISC_ERRORS
"Null parameter passed to urs_add_parm().\n
ursb->ursb_parm = 0x%0x, ursb->ursb_parm->input_dataset = 0x%1x"

E_UR0402_URS_GET_TYPE_NULL:SS50000_MISC_ERRORS
"Null parameter passed to urs_get_type().\n
ursb->ursb_parm = 0x%0x, ursb->ursb_parm->output_dataset = 0x%1x"

E_UR0403_URS_GET_TYPE_USAGE:SS50000_MISC_ERRORS
"Invalid parameter passed to urs_get_type()."

W_UR0413_UNDEF_REPOSITORY:SS01000_WARNING
"The dictionary database name is not defined in config.dat:\n
%0c"

E_UR0414_UNDEF_DLL_NAME:SS50000_MISC_ERRORS
"The dictionary driver name is not defined in config.dat:\n
%0c"

/*
**  Informational -- used to return information to caller.
**	0500 - 05FF
*/

/*
**  Miscellaneous -- internally detected bugs, etc.
**	0600 - 06FF
*/
E_UR0600_NO_MEM:SS50003_EXHAUSTED_RESOURCE
"Not enough memory for URSM call"

E_UR0601_MEM_ERROR:SS5000B_INTERNAL_ERROR
"Error (other than not enough memory) allocating memory for URSM"


/*
**  User errors -- non-fatal errors to be returned to the user.
**	0700 - 07FF
*/
E_UR0700_URS_METHOD_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Method %0c not found in interface %1c"

E_UR0701_URS_WRONG_ARGCOUNT:SS50000_MISC_ERRORS
"Wrong number of arguments for method %0c.\n
Number in request=%1d, number in data dictionary=%2d"

E_UR0702_URS_ADD_PARM_MISMATCH:SS50000_MISC_ERRORS
"The parameter in the procedure call did not match the repository description.\n
Parameter number=%0d, name=%1c;\n
Procedure call db_datatype=%2d, db_length=%3d, db_prec=%4d;\n
Description ds_nullable=%5d, ds_dataType=%6d, ds_length=%7d, ds_precision=%8d, ds_columnType=%9d."

E_UR0703_URS_METHOD_DISABLED:SS50000_MISC_ERRORS
"Method %0c is disabled in interface %1c."

E_UR0704_URS_TOO_MANY_ARGS:SS50000_MISC_ERRORS
"Too many arguments for method %0c.\n
Number in request=%1d, maximum number allowed=%2d."

E_UR0710_URS_ARG_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"%2c argument %0d not found in method %1c."

E_UR0715_URS_METHOD_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Application ID=%0d/interface ID=%1d/method ID=%2d not found for argument %3c."

E_UR0720_URS_INTFC_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Interface %0c not found."

E_UR0721_URS_INTFC_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Application ID=%0d/interface ID=%1d not found for method %2c."

E_UR0730_URS_APPL_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"Application ID=%0d not found for interface %1c."

NO_SQLSTATE_MAPPING
