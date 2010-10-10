# History
# 	30-mar-2001 (gupsh01)
#		Modified to work correctly.
#	09-oct-2001 (gupsh01)
#		Modified the awk scripts to include DEC argument 
# 		This enables the decrement of the index also.
#	3-dec-04 (inkdo01)
#		Fix to filkup awk script.
#	23-Mar-2005 (schka24)
#		Rewrite most of this, refer to fi_defn.txt and .awk
#		instead of the old way.
#	31-Jul-2007 (kiria01) b117955
#		Pulled file generation into jam
#		No longer any need to hand edit the extra files or
#		store the intermediates in piccolo (or check the
#		intermediates or integrate them
#	04-Oct-2010 (kiria01) b124065
#		Updated comments to cover removal of adgoptab.roc from
#		the build and its data being managed in fi_defn.txt
#

Adding new operators
--------------------

1.	Background
	----------

	This is a short document detailing how to add a new operator
	to the DBMS.

	The source file for most of this is fi_defn.txt. This file
	contains the operator information and the associated function
	instance information. This file is 'compiled' by the awk script
	fi_defn.awk into 4 separate, related files:

		adgfi_defn.h
		adgfi_defn.c
		adgfi_defn_lu.c
		adgop_defn.c

	Operators are listed in the operator table in 'adop_defn.c'.
	Operators are not just the usual infix operators like + or =,
	but also prefix-notation functions (SUM(), left(), etc);  zero-
	operand functions (dba(), or session_user);  implicit type
	coercions;  and coercions specified by COPY command formats.
	All these things are "operators".

	They map to one or more 'function instances' in the f.i. table
	adgfi_defn.c.  A function instance (FI) is one specific
	implementation of the operator based on operand type.  So, there
	is one FI for int + int, a different FI for float + float, but
	both implement the "ADD" operator ADI_ADD_OP.

	The fi description table in adgfi_defn.c is ordered by operator
	type (comparison, operator, function, etc) and within type,
	by operator ID number.  (There is no particular ordering within
	FI's for one operator, unless some order is useful to type
	resolution.)

	The f.i may be mapped to the underlying function call by a fast
	lookup table in adgfi_defn_lu.c.  This table is indexed by FI
	number.

	By the way, there's no need for a FI for *every* possible type
	combination.  If type resolution doesn't find the FI it wants,
	type coercions will be generated until it arrives at a type
	combination that is implemented by some FI.  If no such coercions
	exist, the operator is not defined for the given type combination.

2.	Links between the various tables
	--------------------------------

	Every operator has one or more entries in adgop_defn.c.
	(Multiple entries are possible if an operator has synonyms,
	for instance "dba" and "_dba" are both the zero-operand
	"return the name of the database owner" function operator.)
	These aliases can be easily added as will be seen from a quick
	read of the fi_defn.txt source.
	Each operator has an operator ID that looks like ADI_xxx_OP.

	Every function instance has one entry in the FI description
	table, adgfi_defn.roc.  This entry contains the FI number,
	which always looks like ADFI_nnn_blah where nnn is the actual
	FI number that the #define translates to.   The FI description
	table also defines the types that the FI applies to, as well
	as some other useful information (the operator that the FI
	implements;  the length and type info for the result; flags;
	various other stuff).

	Every function instance has one entry in the FI lookup
	table, adgfi_defn_lu.c.  This table is ordered by FI number,
	which is convenient for runtime execution.  The FI lookup table
	contains a pointer back to the FI description table, as well
	as routine addresses for the adu or adc functions that implement
	the function instance.  (If there is no function listed, the
	FI is executed by in-line code, either in adeexecute.c or in
	the STCODE executor adehandler.c).
	

	This section illustrates the links between the various tables
	using the function 'left()' as an example.

a)	fi_defn.txt

	This text data file contains all the operator and function
	instance definitions that implement each operator.

	As examples, below are definitions for an infix operator,
	a post fix operator and the more usual prefix function.

-----------------------------------------------
	Example 1 - infix operator <>
-----------------------------------------------
group ADI_COMPARISON
    ...
    operator ADI_NE_OP type ADI_INFIX
        def_flags ADI_F262144_ASSOCIATIVE
        name "<>" support ADI_INGRES_6|ADI_ANSI|ADI_DB2
        alias "!="

	...
        fi ADFI_008_C_NE_C
            description boolean  :  c != c
            complement ADFI_000_C_EQ_C
            lenspec ADI_FIXED DB_BOO_LEN
            null_preinst ADE_3CXI_CMP_SET_SKIP
            result DB_BOO_TYPE
            operands DB_CHR_TYPE DB_CHR_TYPE
	...

	The 'type ADI_INFIX' defines the infix nature of this
	operator and the 'def_flags ADI_F262144_ASSOCIATIVE'
	will ensure that all the fi elements inherit the flag.
	The 'name "<>"' indicates that this is the primary name
	of the operator and it applies to all languages and
	supported by ADI_INGRES_6,ADI_ANSI and ADI_DB2. There is
	also an alias defined by 'alias "!="' for all languages.

	As this infix operator happens to be in the ADI_COMPARISON
	group, the 'complement ADFI_000_C_EQ_C' defines its negative
	peer and the 'lenspec ADI_FIXED DB_BOO_LEN' and the
	'result DB_BOO_TYPE' are required for comparisons.


-----------------------------------------------
	Example 2 - postfix operator IS NULL
-----------------------------------------------
group ADI_COMPARISON
   ...
   operator ADI_ISNUL_OP type ADI_POSTFIX
        name "is null" support ADI_INGRES_6|ADI_ANSI|ADI_DB2
	...
        fi ADFI_516_ALL_ISNULL
            description boolean  :  all isnull
            complement ADFI_517_ALL_ISNOTNULL
            lenspec ADI_FIXED DB_BOO_LEN
            null_preinst ADE_0CXI_IGNORE
            result DB_BOO_TYPE
            operands DB_ALL_TYPE
	...

	The 'type ADI_POSTFIX' defines the postfix nature of this
	operator and presently these all happen to be comparisons
	albeit single parameter ones.


-----------------------------------------------
	Example 3 - prefix function
-----------------------------------------------
group ADI_NORM_FUNC
    ...
    operator ADI_DATE_OP type ADI_PREFIX
        def_flags ADI_F524288_EXPL_COERCION
        name "date" support ADI_INGRES_6|ADI_DB2
        alias "ingresdate" support ADI_INGRES_6|ADI_ANSI|ADI_DB2
	...
        fi ADFI_125_DATE_C
            description date  :  date(c)
            flags ADI_F16384_CHKVARFI
            lenspec ADI_FIXED 12
            null_preinst ADE_1CXI_SET_SKIP
            result DB_DTE_TYPE
            operands DB_CHR_TYPE
            routine adu_14strtodate

        fi ADFI_280_DATE_CHAR
            description date  :  date(char)
            flags ADI_F16384_CHKVARFI
            lenspec ADI_FIXED 12
            null_preinst ADE_1CXI_SET_SKIP
            result DB_DTE_TYPE
            operands DB_CHA_TYPE
            routine adu_14strtodate
	...

	The 'type ADI_PREFIX' defines the postfix nature of this
	operator and it sets 'def_flags ADI_F524288_EXPL_COERCION' as
	a default flag for the FIs. The 'name "date"' indicates the
	primary name for this operator which also has 'alias "ingresdate"'
	but the absence of ADI_ANSI on date ensures that only ingresdate
	gets appropriatly mapped if ansi enabled.

	On the FIs, these have both got 'flags ADI_F16384_CHKVARFI' which
	is a special case of the ADI_F1_VARFI. These flags control constant
	folding, the latter says do not fold ever and the CHKVARFI says
	fold but ONLY if the params can be treated as real constants. In
	case of DATEs, CHKVARFI asserts that only dates that are constant
	and complete - no missing parts that needed supplimenting from
	current date - will be treated as truly constant and therefore
	foldable.

	These FIs happen to have 'routine adu_14strtodate' which
	indicates that the routine can be called directly without needing
	to be compiled. If no 'routine' is specified, then the function
	will be implemented in-line in ade_execute_cx(). This may also
	be the case if 'routine' is present but then there will simply
	two means of calling the FI.

a)	adgop_defn.c
	------------
	This file is generated from the fi_defn.txt file using fi_defn.awk
	and it builds the table:

		GLOBALDEF const ADI_OPRATION Adi_3RO_operations[];

	This will contain a row for each operator name or alias and defines
	the range of indicies relevant to that operator in the lookup table
	called Adi_fi_lkup defined in adgfi_defn_lu.c:

b)	adgfi_defn_lu.c
	---------------
	This file is also generated from the fi_defn.txt file using
	fi_defn.awk and it builds the lookup table:

		GLOBALCONSTDEF ADI_FI_LOOKUP Adi_fi_lkup[];

	This provides addresses into the main FI table Adi_fis[] and also
	carries the three addresses of the routines related to that FI. The
	first routine will be that of 'routine' or NULL if not specified.
	The 2nd and 3rd relate to routines in support of aggregates.
	The Adi_fis[] table is defined in the file adgfi_defn.c:

c)	adgfi_defn.c
	------------

	This file is also generated from the fi_defn.txt file using
	fi_defn.awk and it builds the FI table itself defining the FI
	specific information:

		GLOBALDEF const ADI_FI_DESC Adi_fis[];

	Besides the FI identifier, it contains the FI ID of any compliment
	FI (comparison only), the type of the FI such as prefix, infix or
	postfix and any flags to apply. In addition the result and parameter
	datatypes are described here along with instrictions about NULL
	handling, how to calculate the result length and whether there are
	any memory workspace requirements.

d)	adgfi_defn.h
	------------
	This is the final product of fi_defn.awk compiling fi_defn.txt.
	It consists of the list of definitions that capture the index
	bounds of each operator within the Adi_fi_lkup[]. The bounds are
	retained in the operator table Adi_3RO_operations[].


3. Adding new operators or FI's
   ----------------------------

a) Adding entry to fi_defn.txt
   ------------------------------

   New operator requires:

   - correct new entry at the end of the appropriate 'group'

   - new #define constant ADI_xxx_OP to identify the operator. This will
     be added in common!hdr!hdr adfops.h to the end of ADI_OPS_MACRO
     in the main code line. Once defined, the operator number should
     not be moved.

   - new #define constants ADFI_nnn_yyyy for the function instance(s) that
     implement the operator. These are defined in common!adf!hdr adffiids.h
     or occasionally in common!hdr!hdr adf.h for FIs that need special
     handling in the code.

     If you're adding a FI to an existing operator, you need the new
     ADFI_nnn_yyy definition, but the op definition in adfops.h will be
     already there.

   New operator requires:
   - correct new operator and FI entries (see fi_defn.txt for format)
     Entries must be properly located within group according to operator
     ID number (NOT alphabetically!). Usually this will be at the end of
     the appropriate 'group'

   New FI for existing operator requires:
   - new FI entry under the operator entry in fi_defn.txt

	Fi, description, lenspec, null_preinst, result, and stcode
	entries in the FI definition are required.  The other lines
	are not required (except for complement which is required for
	any ADI_COMPARISON FI).

c) Updating generated tables
   -------------------------

	** YOU MUST DO THIS BY HAND **

	After modifying fi_defn.txt, run fi_defn.awk from jam.

	Never attempt to hand edit data in adgfi_defn.roc or adgfi_defn.h.
	fi_defn.txt must is the primary source of FI and OP definition info.
	Fix things there, or in the awk process.

	** Running fi_defn.awk WAS NOT PART OF THE JAM BUILD PROCESS. **

	We had to do it by hand.  I objected to this, added jam rules, and
	its now automated in the build.

	Note that fi_defn.txt and fi_defn.awk are both under source
	code control. adgfi_defn* and adgop_defn* are NOT under source
	code control as they are just intermediate files.  

d) Add new executor function
   -------------------------

   Requires:

   - new module in common!adf!adu, or add to existing (relevant)
   module

   - add function prototype to common!adf!hdr aduint.h

	FI's that have no executor function must be implemented
	in-line in adeexecute.c, PLUS there must be a specific
	handler (not the generic fallback) for STCODE execution
	in adehandler.c.

	However, even if FI code is implemented in-line in adeexecute.c
	there is good sense to add the 'routine' definition as well if
	the routine is available. This will aid the performance of
	adf_func() enabling it to not need to compile the CX instructions
	needed to execute the inline code.


APPENDIX:
==============
Historically notes taken from when adgfitab.roc was in piccolo
capturing useful background information,

  History:
      23-jun-86 (thurston)    
          Initial creation.
      26-jun-86 (thurston)
          Finished adding all but the copy command coercions to the function
          instance and function instance lookup table.
      30-jun-86 (thurston)
          Added the ADI_PRIME_AG, ADI_NOPRIME_AG, and ADI_DONT_CARE constants
          to each aggregate function instance description.
      01-jul-86 (thurston)
          Added the six coercion f.i.'s for coercing a datatype to itself.
          This is needed when the datatype is OK, but the length needs to be
          adjusted.
      08-jul-86 (thurston)
          Reformatted the f.i. table so that it is easier to read.  Also,
          made use of the symbolic constants defined for each f.i. ID in the
          <adffiids.h> header file.
      27-jul-86 (ericj)
          Converted for new ADF error handling.
      05-aug-86 (thurston)
          Bug fix to the f.i. table:  F.i. for date(c) was coded as date(text)
          giving a duplicate date(text).
      07-aug-86 (thurston)
          Changed the lenspec for sum(i) and sumu(i) from { ADI_O1, 0 } to
          { ADI_FIXED, 4 } so that we do not have unnecessary integer overflow
          when summing i1 or i2 columns.
      20-aug-86 (thurston)
          Split the function instance lookup table off into the file
          "ADGFILKUP.ROC" because the front ends wish to link non-execution
          programs without pulling in all of the ADF function instance
          routines.
      10-sep-86 (thurston)
          Changed all of the length-specifications for the comparison
          function instances to be { ADI_FIXED, 0 } instead of { 0, 0 }.
      10-sep-86 (thurston)
          Added four new function instances for the "+" operator.  These deal
          with the frontends' DB_DYC_TYPE datatype (dynamic strings).
      14-oct-86 (thurston)
          Added in a WHOLE BUNCH of function instances to deal with
          CHAR and VARCHAR, and the needed coercions for LONGTEXT.
      15-oct-86 (thurston)
          Added all of the comparison function instances that handle
          the dynamic string datatype, and all of the function
          instances for the HEXIN, HEXOUT, and IFNULL functions.
      11-nov-86 (thurston)
          Adjusted fitab for proper definitions of the "hex" function
          (formerly called the "hexout" function) and the "x" operator
          (formerly called the "hexin" function).
      10-sep-86 (thurston)
          Changed all of the length-specifications for the comparison
          function instances (once again) to be { ADI_FIXED, sizeof(bool) }
          instead of { ADI_FIXED, 0 }.
      30-dec-86 (thurston)
          Set the value of the .adi_agwsdv_len field to DB_CHAR_MAX for
	    min/max of char, and DB_MAXSTRING+DB_CNTSIZE for min/max of varchar.
          These had been mistakenly left as 0.
      07-jan-87 (thurston)
          Removed 8 function instances because they caused problems with
          the datatype resolution algorithms.  The fi's removed are:
               74     money  :  f * money
               75     money  :  i * money
               76     money  :  money * f
               77     money  :  money * i
               81     money  :  f / money
               82     money  :  i / money
               83     money  :  money / f
               84     money  :  money / i
          Also removed 2 function instances because the parser now handles
          them directly in the grammar.  These are:
              569     varchar  :  x'text'
              570     varchar  :  x'varchar'
      29-jan-87 (thurston)
          Removed function instance 265 (dyc  :  c + c) because it caused
          ambiguity problems with function instance 064 (c  :  c + c).
	10-feb-87 (thurston)
	    Made use of the DB_MAXSTRING, DB_CNTSIZE, and DB_CHAR_MAX constants
	    instead of hard wiring 2000, 2, and 255, respectively.
	18-feb-87 (thurston)
	    Fixed bug whereby the "money * money" and "money / money" function
	    instances were not being seen by the DBMS.  The bug was created when
	    the fi's like "money * i" and "f / money" were removed to solve a
	    previous bug.  The problem with this was that these fi's were
	    removed from the middle of the `*' and `/' groups.  Therefore, the
	    adi_fitab() routine was returning a non-contiguous table.
	26-feb-87 (thurston)
	    Made changes to use server CB instead of GLOBALDEFs.
	10-mar-87 (thurston)
	    Made many changes.  Totally removed all function instances for the
	    avgu(), countu(), sumu(), equel_length(), and equel_type() INGRES
	    functions.  (The ...u() aggregates no longer concern ADF, and the
	    equel_length() and equel_type() functions are no longer being
	    supported.)  Furthermore, I added function instances for the new
	    `is null' and `is not null' operators, and one for SQL's count(*)
	    aggregate function.
	11-mar-87 (thurston)
	    Replaced the removed .adi_agprime field with the new .adi_npre_instr
	    field.
	17-mar-87 (thurston)
	    Replaced all of the ADE_1CXI_SET_SKIP's with ADE_3CXI_CMP_SET_SKIP's
	    for the COMPARISON function instances.
	18-mar-87 (thurston)
	    Fixed bug with result len specs for any() and int1() function
	    instances; they had been inadvertently changed from {FIXED, 1} to
	    {FIXED, sizeof(bool)} and needed to be changed back.
	05-apr-87 (thurston)
	    Added `varchar : dbmsinfo(varchar)' and `varchar : typename(i)'.
	13-apr-87 (thurston)
	    Increased the output size of the dba() and username() functions to
	    be 24 from 2 and 12 respectively.  This was done because dba() now
	    returns username (usercodes have gone away), and usernames are now
	    24 characters long.
	04-may-87 (thurston)
	    Changed many of lenspec codes so as to avoid creating too long
	    intermediate values.  Also changed { fixed length, 24 } to be
	    { fixed length, DB_MAXNAME } for the dba() and username() FI's.
	22-may-87 (thurston)
	    Changed `varchar  :  typename(i)' to be `char  :  iitypename(i)'.
	08-jun-87 (thurston)
	    Added the following function instances:
		char  :  iistructure(i)

		i  :  iiuserlen(i,i)

		char  :  iichar12(c)
		char  :  iichar12(text)
		char  :  iichar12(char)
		char  :  iichar12(varchar)

		char  :  date_gmt(date)

		char  :  charextract(c,i)
		char  :  charextract(text,i)
		char  :  charextract(char,i)
		char  :  charextract(varchar,i)
	09-jun-87 (thurston)
	    Added back in the eight fi IDs for:
			f * money		f / money
			i * money		i / money
			money * f		money / f
			money * i		money / i
	    because we were causing an `f' to be coerced into a `money' before
	    the multiplication happened, thereby loosing a lot of precision.
	    Furthermore, I had to add four new fi IDs for:
			f * i			f / i
			i * f			i / f
	    in order to avoid the ambiguity problems that caused us to remove
	    the above eight fi IDs in the first place.
	10-jun-87 (thurston)
	    Added the following function instances:
		text  :  ii_notrm_txt(char)
		varchar  :  ii_notrm_vch(char)
	25-jun-87 (thurston)
	    Added the COPY COERCIONS.
	03-aug-87 (thurston)
	    Added fi's for the following copy coercions ... need to take the
	    right most chars, not the left most, to be consistent with 5.0.
				money -copy-> c
				money -copy-> char
				money -copy-> text
				money -copy-> varchar
	01-sep-87 (thurston)
	    Added fi's for `iihexint(text)' and `iihexint(varchar)'.
	02-sep-87 (thurston)
	    Added fi's for `money(money)' and `date(date)'.
	14-sep-87 (thurston)
	    Changed the lenspec code for the `ii_notrm_txt' and `ii_notrm_vch'
          functions to be ADI_RSLNO1CT instead of ADI_O1CT because this is how
          they are to be used by OSL; that is, more like a coercion.
	30-sep-87 (thurston)
	    Added fi's for `_bintim()', `ii_tabid_di(i,i)', and
	    `ii_di_tabid(char)'.
	15-jan-88 (thurston)
	    Removed *LOTS* of function instances to conform to the adopted LRC
	    string proposal.  Things like all string comparisons where the two
	    datatypes are different.
	15-jan-88 (thurston)
	    Added function instances for:
			text  :  squeeze(c)
			text  :  pad(c)
			varchar  :  pad(char)
	    ... and changed the semantics of `squeeze(char)' so that it
	    produces a `varchar' instead of a `char':
			varchar  :  squeeze(char)
	20-jan-88 (thurston)
	    Re-numbered all of the remaining function instances in order to
	    compress the FI lookup table.
	  **********************************************************************
	  **                                                                  **
	  **                      W A R N I N G ! ! !                         **
	  **                      ===================                         **
	  **  THIS CANNOT BE DONE ONCE THE SYSTEM IS IN PRODUCTION WITHOUT    **
	  **  MAKING ALL EXISTING CUSTOMERS RECOMPILE THEIR 4GL APPLICATIONS  **
	  **                                                                  **
	  **********************************************************************
	21-jan-88 (thurston)
	    Added another field to the function instance definition: the `flags'
	    field.  Currently this only holds information about whether the FI
	    is a `VARIABLE FUNCTION INSTANCE' (denoted by ADI_F1_VARFI) or not.
	    A function instance is variable if it can return different results
	    given the same inputs at different times.
	    Examples:
		1) `date  :  date(char)'
			If the `char' string "now" is passed in twice, an hour
			apart, the two results will be different.
		2) `text  :  text(i)'
			If two sessions are sharing the same query plan with
			this FI in it, one session has the -i4 flag set to 30,
			while the other session has it set to 12, the results
			of the function applied to the same i4 value will be
			different.  One session will produce a 30 character
			string, while the other will produce a 12 character one.
		3) `i  :  date_part(c,date)'
			If the two sessions are using different time zones, this
			FI can produce different results with the same set of
			inputs.
	08-feb-88 (thurston)
	    Changed the lenspec for `char -> char' to be ADI_RESLEN instead of
	    ADI_O1.
	25-feb-88 (thurston)
	    Added the following eight function instances to cure the 5.0
	    compatibility issue of removal of trailing blanks for text and
	    varchar as a 1st argument, and not causing other combo's to
	    erroneously select one of the hybrid function instances:
		text  :  concat(text,char)	    text  :  text + char
		text  :  concat(text,varchar)	    text  :  text + varchar
		char  :  concat(char,varchar)	    char  :  char + varchar
		text  :  concat(varchar,text)	    text  :  varchar + text
	26-feb-88 (thurston)
	    Added the FI's for the new `ii_dv_desc()' function.
	01-sep-88 (thurston)
	    Changed the constants DB_MAXSTRING and DB_CHAR_MAX to be
	    DB_GW4_MAXSTRING, since this is now the absolute largest INGRES
	    string ADF can handle.
	28-sep-88 (thurston)
	    Changed the result length for the `ii_tabid_di()' function from 24
	    to 8.
	04-nov-88 (thurston)
	    Added the FI for the new `avg(date)' aggregate function.
	09-nov-88 (jrb)
	    Added FI table entry for sum(date) aggregate.
	28-feb-89 (fred)
	    Added initial size variable for user-defined ADT support.
	07-Mar-89 (fred)
	    Changed sort order of the table.  The Function Instance Description
	    table must now be sorted, withing function instance type, by
	    operator id.  That is, in addition to being grouped by operator id,
	    these operator id groups must be sorted numerically by operator id.
	    This change facilitates speedier addition of user defined function
	    instances.
	21-mar-89 (jrb)
	    Added new field to adi_lenspec initializer for decimal project.
	22-may-89 (fred)
	    Added new instances for ii_ext_{type,length}().  These are the last
	    normal functions (at the moment), so the [copy_]coercions all got
	    shifted down 2 places.
	01-jun-89 (fred)
	    Removed duplicate definition for char(c).
	06-jun-89 (mikem)
	    Fixed "table_key()" and "object_key()" function instances to point
	    to real operators, and moved them into correct sorted order.
	08-jun-89 (jrb)
	    Changed result type of integer arithmetic (+, -, *, and /) to i4.
	    This was a decision made by the LRC last year which was upheld after
	    being reviewed recently.
	26-jun-89 (mikem)
	    Added logical key <-> longtext conversions.
	17-jul-89 (fred)
	    Corrected fitab entries for ii_ext{type,length}() to skip over the
	    instructions when nulls are present.
	15-aug-89 (mikem)
	    Set correct "result length" for the min/max aggregates on logical
	    keys.
	28-sep-89 (jrb)
	    Added 66 function instances for decimal datatype.
	02-feb-1991 (jennifer)
            Fix bug 33553 by adding isnotnull and isnull for security label.
	11-feb-1992 (stevet)
	    Moved decimal function instances, security label function instances
	    and logical key function instances  so that it is in sorted order
	    within ADI_NORM_FUNC type.
	09-mar-92 (jrb, merged by stevet)
	    Added 6 new function instaces: 4 for conversion of NULL value to
	    string type, 1 for locate(all,all) (bug fix), and 1 for ii_iftrue()
	    (outer join project).
	09-mar-92 (jrb, merged by stevet)
            Added four new fi's for resolve_table function.
	29-apr-1992 (fred)
	    Added coercions and operators for peripheral datatypes.
  	13-jul-92 (stevet)
  	    Removed fi entries for direct support of resolve_table for
  	    CHAR and C datatypes.  Changed resolve_table function to 
  	    accept 2 arguements.
  	04-aug-92 (stevet)
  	    Added eight new fi's for LOGKEY/TABKEY <-> VARCHAR/TEXT.
	29-sep-1992 (fred)
	    Added FI entries for BIT and BIT VARYING datatypes.
	13-oct-1992 (stevet)
  	    Added internal function ii_row_count() and DB_ALL_TYPE for
            isnull and isnotnull operators.  Also removed isnull and
            isnotnull FI for 65 datatypes.
	03-dec-1992 (stevet)
            Added fi's for one and two arguments versions of CHAR(), VARCHAR(),
            TEXT() and C() functions using DB_ALL_TYPE.  Removed FI for
            these functions for 6.5 datatypes.  Also added FI for sesssion_user
            initial_user and system_user.
	09-dec-1992 (rmuth, merged by stevet)
            Add internal functions iitotal_allocated_pages and
            iitotal_overflow_pages
	08-Jan-1993 (fred)
            Added long varchar function instances for left, right,
            uppercase, lowercase, concat, +, size, and length.
	28-jan-1993 (stevet)
            Added alltype fi for ii_dv_desc() to support FIPS constraint.
	09-Apr-1993 (fred)
            Added byte datatype references.  Cleaned up bit ones.
	14-jul-93 (ed)
	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
	25-aug-93 (ed)
	    remove dbdbms.h
  	14-sep-1993 (stevet)
  	    Add money*dec and money/dec function instance.
	14-Oct-1993 (fred)
            Fixed result length for varchar->varbyte coercion.
	10-feb-1994 (stevet)
  	    Moved FI for ADI_SESSUSER_OP, ADI_SYSUSER_OP and ADI_INITUSER_OP
            so that this table is in correct sort order.  Wrong sort order
            prevent overloaded UDF to work correctly. (B59663)
	07-apr-1994 (stevet)
            Added FI for vbyte -> longtext.  (B58991)
	12-may-94 (robf)
            Reintegrated Secure 2.0 functions.
	28-jun-94 (robf/arc)
            Correct seclabel_disjoint() definition after integration problem.
	14-dec-1994 (shero03)
	    Removed iixyzzy()
	11-jan-1995 (shero03)
	    Added iixyzzy()
	5-jul-1996 (angusm)
		Add new function iipermittype(), shuffle down COPY_COERCION and
		COERCION functions.
	17-jul-96 (inkdo01)
	    Slight reorganization of "hex" entries, to permit DB_ALL_TYPE to
	    overload the function.
	11-nov-1996 (shero03)
	    Put Security_label in the proper sequence.
	19-feb-1997 (shero03)
	    Added soundex function.
	9-april-1997(angusm)
	    Previous change causes problems with calc of result columns
	    in expressions e.g. "select hex(left(right(col6, 1),1))...." 
	    (bug 81454)
	29-jul-97 (stephenb)
	    Add coercion from varchar, c, char and text to decimal and 
	    vice-versa
	10-oct-97 (inkdo01)
	    Add FI's for byte/varbyte functions on DB_ALL_TYPE to support
	    data types with no explicit entries.
	06-oct-1998 (shero03)
	    Add bitwise functions: adu_bit_add, _and, _not, _or, _xor
	    ii_ipaddr.  Removd xyzzy
	23-oct-98 (kitch01) Bug 92735.
	    Add _date4() function.
	12-Jan-1999 (shero03)
	    Add hash function.
	15-Jan-1999 (shero03)
	    Add Random functions.
	21-jan-1999 (hanch04)
	    replace nat and longnat with i4
	25-Jan-1999 (shero03)
	    Add UUID functions.
	16-Feb-1999 (shero03)
	    Support up to 4 operands
	08-Jun-1999 (shero03)
	    Add unhex function.
	28-oct-99 (inkdo01)
	    Added numerous functions for OLAP aggregates.
	15-dec-00 (inkdo01)
	    Overhaul aggregate FI's to support inline aggregation.
	15-jan-01 (inkdo01)
	    Use DB_ALL_TYPE for certain f.i.'s (count, any, isnull, 
	    isnotnull) where data type is immaterial.
	01-mar-2001 (gupsh01)
            Added function instances for new data types nchar and 
	    nvarchar for unicode support.
	15-mar-2001 (gupsh01)
	    Modified function instances for nchar and nvarchar 
	    type, added the coercion functions. 
	27-mar-2001 (gupsh01)
            Added new functions for nchar and nvarchar type. 
	28-mar-2001 (gupsh01)
            Added coercion functions for nchar and cvarchar coercion
            to byte, varbyte, lbyte, and ltext type.
	03-apr-2001 (gupsh01)
            Added support for long nvarchar type.
	16-apr-2001 (stial01)
            Added support for iitableinfo function
	20-apr-2001 (gupsh01)
            Added support for long nvarchar -> long nvarchar
            coercion. Fixed indexes numbers skewed by a previous change.
	20-june-01 (inkdo01)
	    Fix comments on unary OLAP aggregates.
	18-sept-01 (inkdo01)
	    Variety of fixes for Unicode - replace ADI_FIXED's (they're 
	    all wrong) and make result of pad, trim, squeeze nvarchar.
	05-sep-01 ( gupsh01)
	    Modified the comments with function instace numbers to correcly 
	    change function instance numbers for upto 4 digits, while adding 
	    and removing function instances. Removed the function instances
	    for coercion between unicode nchar and nvarchar and char and 
	    varchars.
	08-jan-02 (inkdo01)
	    Added 42 entries for ANSI char_length, octet_length, bit_length,
	    position and trim functions.
	18-mar-02 (gupsh01)
	    Added new function instances for ifnull function for nchar and
	    nvarchar data types.
	05-apr-02 (gupsh01)
	    Add missing entries for ifnull function.
      27-mar-2002 (gupsh01)
          Added new function instances to coerce the character datatypes
          char and varchar to unicode datatypes nchar and nvarchar and
          vice-versa.
	16-june-2002 (gupsh01)
	    Modified the result lengths for the coercion functions from character
	    datatypes to unicode datatypes.
	27-may-02 (inkdo01)
	    Added ADI_F32_NOTVIRGIN flag to random's/uuid_create.
	01-aug-2002 (gupsh01)
	    Modified the result lengths for the coercion functions from byte 
	    datatypes to unicode datatypes.
	08-aug-02 (inkdo01)
	    Changed NCHR like pattern operand to NVCHR to enable proper pattern
	    normalization for Unicode data.
	30-sep-02 (drivi01)
	    Changed ADI_CHLEN_OP, ADI_OCLEN_OP, ADI_BLEN_OP, ADI_POS_OP, 
	    ADI_ATRIM_OP operation flags to ADI_DISABLED because the above
	    operations aren't defined and force the recovery server to exit
	    with an exception when udts are being used. Functions that have
	    been altered contain original operation in the comments.
	25-mar-03 (gupsh01)
	    Added new flag ADI_F64_ALLSKIPUNI to function instances which 
	    support DB_ALL_TYPE as input parameter. This flag will disable 
	    calling using these function instances for unicode input 
	    parameters, nchar and nvarchar date types. 
      04-apr-2003 (gupsh01)
          Renamed the constants ADFI_998_NCHAR_LIKE_NCHAR to
          ADFI_998_NCHAR_LIKE_NVCHAR, and ADFI_999_NCHAR_NOTLIKE_NCHAR
          to ADFI_999_NCHAR_NOTLIKE_NVCHAR. This will remove ambiguity 
          between the parameters in adgfitab.roc and the function 
          instance ID.
	09-apr-2003 (gupsh01)
	    Fix the complementary function instances in nchar_like_nvarchar 
	    and nchar_notlike_nvarchar.
      10-apr-2003 (gupsh01)
          Remove the function instances for concat between unicode and non
	    unicode datatypes.Added function instance to concat between nchar
	    and nvarchar.
	19-apr-2003 (gupsh01)
	    Rework the previous change.
	24-apr-2003 (gupsh01)
	    isnull and isnotnull functions should have ADI_F0_NOFIFLAGS flag 
	    and not ADI_F64_ALLSKIPUNI set, For unicode input parameters we 
	    wish to go through the function with DB_ALL_TYPE parameter, for 
	    unicode input parameter.
	28-apr-2003 (gupsh01)
	    Remove the function instance for nchar like nvarchar, and 
	    nchar notlike nvarchar. These operations should follow from 
	    nvarchar like nvarchar or nvarchar notlike nvarchar operation.
      19-may-2003 (gupsh01)
          Added new function instances for long_byte() and long_varchar()
          functions. The long_varchar and long_byte coercions are no longer
          mentioned explicitly in this table, The entries for these have now
          been moved to point to the new functions for long_byte and
          long_varchar().
    04-Sep-2003 (hanje04)
        BUG 110855 - INGDBA2507
        Make dbmsinfo() return varchar(64) instead of varchar(32) to stop
        long (usually fully qualified) hostnames from being truncated.
	15-dec-2003 (gupsh01)
	    Added new function instances for nchar, nvarchar, iiutf8_to_utf16,
	    iiutf16_to_utf8 and for copy coercion of nchar/nvarchar to/from
	    UTF8 internal datatype.
	16-dec-03 (inkdo01)
	    Change DISABLED back to CHLEN/OCLEN/BLEN/POS/ATRIM_OPs.
	12-apr-04 (inkdo01)
	    Added FIs for int8() coercion (for BIGINT support).
	13-apr-2004 (gupsh01)
	    Fixed function instance entries > 1007 they were out of sync.
	20-apr-04 (drivi01)
		Added FIs for unicode cercion for c, text, date, money, 
      security_label, logical_key, secid, table_key.
	05-may-2004 (gupsh01)
	    Fixed result length calculation for functions nchar() and 
	    nvarchar().
	12-may-2004 (gupsh01)
	    Added support for nchar and nvarchar types to float4, float8, int1
	    int2, int4, int8 and decimal functions.
	14-may-2004 (gupsh01)
	    Fixed float8(varchar) function, it had wrong entry for function 
	    instance.
	19-may-2004 (gupsh01)
	    Added support for nchar and nvarchar to functions char(),varchar()
	    ascii(), text(), date(), money().
	14-jun-2004 (gupsh01)
	    Added support for copy coercion and normal coercion between 
	    char/varchar and UTF8 types. 
	20-july-2004 (gupsh01)
	    Fixed length calculation for varbyte(all).
	29-july-04 (inkdo01)
	    Minor change to security_label removal to keep fitab/filkup 
	    consistent.
	12-feb-2004 (gupsh01)	
	    Added coercion between char/varchar -> long varchar. Fixed this 
	    table for function instance numbers > 1000 not matching to 
	    adgfilkup.roc.
	28-feb-05 (inkdo01)
	    Added length(NCLOB) and size(NCLOB).
	23-Mar-2005 (schka24)
	    Replace table with one generated by fi_defn.txt, .awk machinery.
      08-Jun-2005 (gupsh01)
	    Removed ADFI_998_NCHAR_LIKE_NVCHAR and ADFI_999_NCHAR_NOTLIKE_NVCHAR.
	    This is breaking up nchar like varchar case, and these are correctly
	    handled by other function instances.
	13-june-05 (inkdo01)
	    Change result length of ii_dv_desc() to 64 for collations.
	14-jun-2005 (gupsh01)
	    Removed function instances ADFI_1002_CONCAT.. to ADFI_1008_CONCAT..
	17-aug-05 (inkdo01)
	    Change sum(int) to 8 byte result to make room.
	13-apr-06 (dougi)
	    Added entry for isdst().
	30-May-2006 (jenjo02)
	    Add second input parm to adu_17structure.
	07-june-06 (dougi)
	    Added coercions between int/flt and char/varchar/nchar/nvchar.
	03-july-06 (dougi)
	    Changed RESLEN to new CTO48 lenspec for string to int/flt above.
	04-aug-06 (stial01)
	    Added entries for substring, position for blobs
	19-aug-06 (gupsh01)
	    Added entries for ansidate, time_wo_tz, time_with_tz, 
	    timestamp_wo_tz, timestamp_with_tz, time_local, 
	    timestamp_local, And datetime constant functions
	    CURRENT_DATE, CURRENT_TIME, CURRENT_TIMESTAMP, LOCAL_TIME
	    LOCAL_TIMESTAMP.
	05-sep-06 (gupsh01)
	    Added interval_dtos and interval_ytom.
	13-sep-2006 (dougi)
	    Dropped entries for string/numeric coercions - found a better way.
	13-sep-2006 (gupsh01)
	    Fixed entries for ANSI date/time types.
	18-oct-2006 (dougi)
	    Added entries for year(), quarter(), month(), week(), week_iso(), 
	    day(), hour(), minute(), second(), microsecond() functions for date 
	    extraction.
	23-oct-2006 (stial01)
	    Back out Ingres2007 change to iistructure()
	16-oct-2006 (stial01)
          Added locator functions.
	24-oct-2006 (dougi)
	    Reordered entries - opid must be sorted within each category; i.e. 
	    the operator entries must be sorted on the opid.
	25-oct-2006 (gupsh01)
	    Fixed length calculation for ADFI_233_DATE_TO_DATE to now follow
	    ADI_DATE2DATE directive in correctly resolving the dates for the
	    data type family.
	14-nov-2006 (dougi)
	    Change result length of _version() to 32.
	10-dec-2006 (gupsh01)
	    Added function instances for Date arithmetic.
	08-dec-2006 (stial01)
	    Added/updated POSition funcions
	19-dec-2006 (dougi)
	    Added LTXT to LTXT coercion & "=" compare to allow unions of NULLs.
      19-dec-2006 (stial01)
          Added ADFI_1273_NVCHAR_ALL_I for nvarchar(all,i)
	02-jan-2007 (stial01)
	    Fixed integration of NVCHAR_ALL_I.
	08-jan-2007 (gupsh01)
	    Added function instances for Adding ANSI date/time tyeps.
	22-jan-2007 (gupsh01)
	    Added coercion between nvarchar to long varchar and varchar
	    to long nvarchar.
	25-jan-2007 (stial01)
	    Fixed ascii,char,text,varchar,varbyte fi's
	01-feb-2007 (dougi)
	    Changed date_part() result to 4 byte integer to sccomodate 
	    microsecond and nanosecond options.
	12-feb-2007 (stial01)
	    Undo some of 25-jan-2007 fi cleanup, requires upgradedb -trees
	08-mar-2007 (dougi)
	    Added FI entry for round() function.
	13-mar-2007 (gupsh01)
	    Added lower/upper support for long nvarchar.
	13-apr-2007 (dougi)
	    Added entries for ceil(), floor(), trun(), chr(), ltrim() and
	    rtrim().
	16-apr-2007 (dougi)
	    Added entries for lpad(), rpad() and replace().
	02-may-2007 (dougi)
	    And tan(), atan2(), acos(), asin(), pi() and sign().
