/*
** dummy.h
**
** Description:
**
**	Dummy GLOBALDEF's for listexec and executor.
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**     4-jan-1994 (donj)
**	Added some more dummy GLOBALDEF's that VMS complained
**	about.
**    14-apr-1994 (donj)
**	Added tcName[].
**    10-may-1994 (donj)
**	Added some more dummy GLOBALDEFS to satisfy some of the functions
**	in the SEP libraries even if listexec and executor don't use them.
**    17-may-1994 (donj)
**	Took out an unused SEP_TESTINFO GLOBALDEF that was conflicting on
**	VMS with a Sep_TestInfo declaration in executor and listexec.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALDEF    CMMD_NODE    *sepcmmds = NULL ;
GLOBALDEF    LOCATION      SEPcanloc ;
GLOBALDEF    LOCATION      SEPresloc ;
GLOBALDEF    LOCATION      SEPtcinloc ;
GLOBALDEF    LOCATION      SEPtcoutloc ;
GLOBALDEF    LOCATION      outLoc ;
GLOBALDEF    LOCTYPE       outputDir_type ;
GLOBALDEF    OPTION_LIST  *close_canon_opt = NULL ;
GLOBALDEF    OPTION_LIST  *open_canon_opt = NULL ;
GLOBALDEF    PID           SEPsonpid ;
GLOBALDEF    SEPFILE      *sepCanons = NULL ;
GLOBALDEF    SEPFILE      *sepDiffer = NULL ;
GLOBALDEF    SEPFILE      *sepGCanons = NULL ;
GLOBALDEF    SEPFILE      *sepResults = NULL ;
GLOBALDEF    STATUS        SEPtclink = SEPOFF ;
GLOBALDEF    TCFILE       *SEPtcinptr = NULL ;
GLOBALDEF    TCFILE       *SEPtcoutptr = NULL ;
GLOBALDEF    WINDOW       *coverupW = NULL ;
GLOBALDEF    WINDOW       *mainW = NULL ;
GLOBALDEF    WINDOW       *statusW = NULL ;
GLOBALDEF    bool          SEPdisconnect = FALSE ;
GLOBALDEF    bool          batchMode = FALSE ;
GLOBALDEF    bool          ignore_silent = FALSE ;
GLOBALDEF    bool          in_canon = FALSE ;
GLOBALDEF    bool          outLoc_exists ;
GLOBALDEF    bool          paging = TRUE ;
GLOBALDEF    bool          paging_old = TRUE ;
GLOBALDEF    bool          shellMode = FALSE ;
GLOBALDEF    bool          updateMode = FALSE ;
GLOBALDEF    bool          useSepTerm = TRUE ;
GLOBALDEF    bool          verboseMode = FALSE ;
GLOBALDEF    char          SEPinname [MAX_LOC] ;
GLOBALDEF    char          SEPoutname [MAX_LOC] ;
GLOBALDEF    char          SEPpidstr [6] ;
GLOBALDEF    char          buffer_1 [SCR_LINE] ;
GLOBALDEF    char          buffer_2 [SCR_LINE] ;
GLOBALDEF    char          canonAns = EOS ;
GLOBALDEF    char          cont_char[3] = {'-',EOS};
GLOBALDEF    char          editCAns = EOS ;
GLOBALDEF    char          holdBuf [SCR_LINE] ;
GLOBALDEF    char          old_editor [MAX_LOC] ;   /* old value of EDITOR    */
GLOBALDEF    char          old_visual [MAX_LOC] ;   /* old value of VISUAL    */
GLOBALDEF    char          old_ing_edit [MAX_LOC] ; /* old value of ING_EDIT  */
GLOBALDEF    char          msg [TEST_LINE] ;
GLOBALDEF    char          real_editor [MAX_LOC] ;
GLOBALDEF    char          terminalType [MAX_LOC] ;
GLOBALDEF    char          tcName [MAX_LOC] ;
GLOBALDEF    char         *ErrC       = ERx("ERROR: Could not ") ;
GLOBALDEF    char         *ErrF       = ERx("ERROR: failed while ") ;
GLOBALDEF    char         *ErrOpn     = ERx("ERROR: Could not open ") ;
GLOBALDEF    char         *SEP_IF_EXPRESSION = NULL ;
GLOBALDEF    char         *close_args = NULL ;
GLOBALDEF    char         *close_args_array [WORD_ARRAY_SIZE] ;
GLOBALDEF    char         *creloc     = ERx("create location for ") ;
GLOBALDEF    char         *lineTokens [SCR_LINE] ;
GLOBALDEF    char         *lookfor    = ERx("looking for ") ;
GLOBALDEF    char         *open_args = NULL ;
GLOBALDEF    char         *open_args_array [WORD_ARRAY_SIZE] ;
GLOBALDEF    char         *outputDir = NULL ;
GLOBALDEF    i4            SEPtcsubs = 0 ;
GLOBALDEF    i4            fromLastRec = 0 ;
GLOBALDEF    i4            if_level [15] ;
GLOBALDEF    i4            if_ptr = 0 ;
GLOBALDEF    i4            screenLine = 0 ;
GLOBALDEF    i4            sep_state ;
GLOBALDEF    i4            sep_state_old ;
GLOBALDEF    i4            testLevel = 3 ;
GLOBALDEF    i4            testLine = 0 ;
GLOBALDEF    i4            verboseLevel = 0 ;
GLOBALDEF    i4            waitLimit = 300 ;
GLOBALDEF    LOCATION     *SED_loc = NULL ;
GLOBALDEF    bool          diff_numerics = FALSE ;
GLOBALDEF    bool          grammar_methods = GRAMMAR_LEX;
GLOBALDEF    bool          ignoreCase = FALSE ;
GLOBALDEF    f8            fuzz_factor = 0 ;
GLOBALDEF    int           diffSpaces = 0 ;
GLOBALDEF    long          A_always_match = 0 ;
GLOBALDEF    long          A_word_match   = 0 ;
GLOBALDEF    char          kfebuffer [TERM_LINE] ;
GLOBALDEF    char         *kfe_ptr = kfebuffer ;
GLOBALDEF    char         *rbanner    = ERx("====  RESULT FILE  ====") ;
GLOBALDEF    char         *rbannerend = ERx("====  RESULT FILE END  ====") ;
GLOBALDEF    char         *Con_with   = ERx("Connection with ") ;
GLOBALDEF    char         *cmtFile = NULL ;
GLOBALDEF    LOCATION     *cmtLoc  = NULL ;
GLOBALDEF    char         *cmtName = NULL ;
