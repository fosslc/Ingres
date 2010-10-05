SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _UL
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	03-dec-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	19-jun-1996 (sweeney)
**	    Fix typos.
**      09-jul-2003 (stial01)
**          Added E_UL0017_DIAGNOSTIC
**      04-oct-2010 (maspa05) bug 124531
**          Added E_UL0400_BAD_TRACE_DIR and E_UL0401_BAD_TRACE_DIR
*/


#define _UL_CLASS 10


E_UL0000_OK:SS00000_SUCCESS
"successful completion"
E_UL0001_BAD_SYSTEM_LOOKUP:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL0002_BAD_ERROR_LOOKUP:SS5000B_INTERNAL_ERROR
"Could not find message in error message file"
E_UL0003_BADPARM:SS5000B_INTERNAL_ERROR
"Bad parameter to error message lookup routine."
E_UL0004_CORRUPT:SS5000B_INTERNAL_ERROR
"ULM has detected a corrupted memory pool."
E_UL0005_NOMEM:SS5000B_INTERNAL_ERROR
"Either ULM cannot get enough memory from SCF to initialize a memory
 pool, or the memory pool has been exhausted."
E_UL0006_BADPTR:SS5000B_INTERNAL_ERROR
"Bad pointer has been detected by ULM; pointer to piece of memory lies outside
 of the supplied memory pool."
E_UL0007_CANTFIND:SS5000B_INTERNAL_ERROR
"ULM cannot find the block of memory in the pool's allocated list."
E_UL0008_MEM_SEMINIT:SS5000B_INTERNAL_ERROR
"SCF would not initialize semaphore."
E_UL0009_MEM_SEMWAIT:SS5000B_INTERNAL_ERROR
"Error attempting to set semaphore."
E_UL000A_MEM_SEMRELEASE:SS5000B_INTERNAL_ERROR
"Error attempting to release semaphore."
E_UL000B_NOT_ALLOCATED:SS5000B_INTERNAL_ERROR
"Attempt to perform some ULM operation on a memory pool that was not
 allocated by you."
E_UL000C_BAD_ACK_MODE:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL000D_NEWBLOCK:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL000E_BAD_DO_BLOCK:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL000F_BAD_BLOCK_TYPE:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL0010_NEED_MORE:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL0011_MORE_TO_SEND:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL0012_NETWORK_UNKNOWN:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL0013_IN_BLOCK_FULL:SS5000B_INTERNAL_ERROR
"ULF error"
E_UL0014_BAD_ULM_PARM:SS5000B_INTERNAL_ERROR
"Bad parameter supplied to one of the ULM routines."
E_UL0015_BAD_STREAM_MARK:SS5000B_INTERNAL_ERROR
"ULM was handed an invalid `memory stream mark'."
E_UL0016_CHAR_INIT:SS5000B_INTERNAL_ERROR
"Error while processing character set attribute file."
E_UL0017_DIAGNOSTIC:SS5000B_INTERNAL_ERROR
"Unexpected error condition %0c."
E_UL0101_TABLE_ID:SS5000B_INTERNAL_ERROR
"Invalid hash table id found in ULH request block."
E_UL0102_TABLE_SIZE:SS5000B_INTERNAL_ERROR
"Invalid hash table size specified."
E_UL0103_NOT_EMPTY:SS5000B_INTERNAL_ERROR
"Trying to close a hash table containing locked or fixed objects."
E_UL0104_FULL_TABLE:SS5000B_INTERNAL_ERROR
"No more space in the hash table."
E_UL0105_FULL_CLASS:SS5000B_INTERNAL_ERROR
"No more space in the specified class."
E_UL0106_OBJECT_ID:SS5000B_INTERNAL_ERROR
"Invalid object id found in ULH request block."
E_UL0107_ACCESS:SS5000B_INTERNAL_ERROR
"Invalid mode of access to the object."
E_UL0108_DUPLICATE:SS5000B_INTERNAL_ERROR
"Duplicate object id."
E_UL0109_NFND:SS5000B_INTERNAL_ERROR
"Requested object/class not found in the hash table."
E_UL010A_MEMBER:SS5000B_INTERNAL_ERROR
"The object is already a member of a class."
E_UL010B_NOT_MEMBER:SS5000B_INTERNAL_ERROR
"The object is not a member of any class."
E_UL010C_UNKNOWN:SS5000B_INTERNAL_ERROR
"Unknown information code."
E_UL010D_TABLE_MEM:SS50003_EXHAUSTED_RESOURCE
"Error in opening a memory stream for the hash table."
E_UL010E_TCB_MEM:SS50003_EXHAUSTED_RESOURCE
"Error in allocating memory space for the hash table."
E_UL010F_BUCKET_MEM:SS50003_EXHAUSTED_RESOURCE
"Error in allocating memory space for hash buckets."
E_UL0110_CHDR_MEM:SS50003_EXHAUSTED_RESOURCE
"Error in allocating memory space for class headers"
E_UL0111_OBJ_MEM:SS50003_EXHAUSTED_RESOURCE
"Error in allocating memory for a hash object"
E_UL0112_INIT_SEM:SS5000B_INTERNAL_ERROR
"Error in initializing a semaphore."
E_UL0113_TABLE_SEM:SS5000B_INTERNAL_ERROR
"Error in locking the hash table semaphore."
E_UL0114_BUCKET_SEM:SS5000B_INTERNAL_ERROR
"Error in locking a bucket semaphore."
E_UL0115_CLASS_SEM:SS5000B_INTERNAL_ERROR
"Error in locking a class semaphore."
E_UL0116_LRU_SEM:SS5000B_INTERNAL_ERROR
"Error in locking the LRU queue semaphore."
E_UL0117_OBJECT_SEM:SS5000B_INTERNAL_ERROR
"Error in locking an object semaphore."
E_UL0118_NUM_ALIASES:SS5000B_INTERNAL_ERROR
"Invalid number of aliases specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error which may only occur on server startup.
**	ulh_init() has been called with a flag indicating that aliases will be
**	allowed, but the maximum number of aliases specified by the caller was
**	non-positive (<=0).
**
**	This should not occur under normal circumstances.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_UL0119_ALIAS_MEM:SS5000B_INTERNAL_ERROR
"Error in allocating memory for alias."
/*%
** Parameters:
**	None.
**
** Explanation:
**	In order to create a new alias in ULH cache, there should be enough
**	memory to accommodate a new alias, and the total number of existing
**	aliases should not exceed some number specified at server startup.
**	If one of these conditions exists, at least one object must be removed
**	from the cache before the new alias can be created.
**
**	This error will occur if no cache entries (with the possible exception
**	of the one for which the alias was being created) could be removed from
**	the cache.
**
** System Status:
**	The statement will be processed, but performance may be degraded.
**
** Recommendation:
**	The server may have to be shut down and restarted to provide for a
**	larger number of synonyms.
*/
E_UL0120_DUP_ALIAS:SS5000B_INTERNAL_ERROR
"Duplicated alias."
E_UL0121_NO_ALIAS:SS5000B_INTERNAL_ERROR
"The hash table does not support alias."
E_UL0122_ALIAS_REDEFINED:SS5000B_INTERNAL_ERROR
"Object alias cannot be redefined."
E_UL0123_NAME_TOO_LONG:SS5000B_INTERNAL_ERROR
"The name string is too long."
E_UL0201_ULM_FAILED:SS50003_EXHAUSTED_RESOURCE
"Unable to obtain memory from ULM to build tree for display."
E_UL0300_BAD_ADC_TMLEN:SS5000B_INTERNAL_ERROR
"Unexpected error while calling adc_tmlen during constant text creation."
E_UL0301_BAD_ADC_TMCVT:SS5000B_INTERNAL_ERROR
"Unexpected error while calling adc_tmcvt during constant text creation."
E_UL0302_BUFFER_OVERFLOW:SS5000B_INTERNAL_ERROR
"Buffer used for constant to text string conversion is too small."
E_UL0303_BQN_BAD_QTREE_NODE:SS5000B_INTERNAL_ERROR
"Unexpected query tree node."
E_UL0304_ADI_OPNAME:SS5000B_INTERNAL_ERROR
"Unexpected error while calling ADI_OPNAME to find PST_COP operator."
E_UL0305_NO_MEMORY:SS50003_EXHAUSTED_RESOURCE
"Unable to obtain memory for tree to text routines."
E_UL0306_EXCEPTION:SS5000B_INTERNAL_ERROR
"Unexpected exception number %0d occurred, but unable to determine the type."
E_UL0307_EXCEPTION:SS5000B_INTERNAL_ERROR
"Unexpected exception occurred in ULF."
E_UL0308_ADF_EXCEPTION:SS5000B_INTERNAL_ERROR
"user created ADF exception."
E_UL0309_ADF_EXCEPTION:SS5000B_INTERNAL_ERROR
"non-user created ADF exception."
E_UL030A_BAD_LANGUAGE:SS5000B_INTERNAL_ERROR
"bad language ID found for query tree to text conversion."
E_UL030B_BAD_QUERYMODE:SS5000B_INTERNAL_ERROR
"consistency check - unexpected query mode."
E_UL0400_BAD_TRACE_DIR:SS5000B_INTERNAL_ERROR
"unable to set trace directory to '%0c'
- check directory exists and is writeable. Trace directory remains unset."
/*%
** Parameters:
**	'%0c' - name of trace directory.
**
** Explanation:
**	This error will occur when SET TRACE RECORD fails if the directory
**      cannot be found or written to, and the directory was previously not set
**
** System Status:
**	The statement fails. The trace directory is not set.
**
** Recommendation:
**	Re-run SET TRACE RECORD to a valid directory.
*/
E_UL0401_BAD_TRACE_DIR:SS5000B_INTERNAL_ERROR
"unable to set trace directory to '%0c'
- check directory exists and is writeable. Trace directory remains '%1c'"
/*%
** Parameters:
**	'%0c' - name of trace directory.
**	'%1c' - name of previous trace directory.
**
** Explanation:
**	This error will occur when SET TRACE RECORD fails if the directory
**      cannot be found or written to, and the trace directory was previously
**      set.
**
** System Status:
**	The statement fails. The trace directory remains unchanged.
**
** Recommendation:
**	Re-run SET TRACE RECORD to a valid directory.
*/

NO_SQLSTATE_MAPPING
