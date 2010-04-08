
#include <achilles.h>

#ifdef NT_GENERIC
char log_actions[][16] = {
#else
GLOBALDEF char log_actions[][16] = {
#endif /* END #ifdef NT_GENERIC */
	"STARTED",		/* C_START     */
	"NO START",		/* C_NO_START  */
	"INTERRUPTED",		/* C_INT       */
	"NO INTERRUPT",		/* C_NO_INT    */
	"KILLED",		/* C_KILL      */
	"NO KILL",		/* C_NO_KILL   */
	"EXITED",		/* C_EXIT      */
	"TERMINATED",		/* C_TERM      */
	"ABORTED",		/* C_ABORT     */
	"NOT FOUND"		/* C_NOT_FOUND */
};
