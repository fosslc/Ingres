# Copyright (c) 2007, 2008 Ingres Corporation
#
#
#
# This awk program is used to process the FI definition file fi_defn.txt
# into a bunch of output files:
#
# Usage:
#   To build the adgfi_defn.roc file:
#       nawk -vroc_file=adgfi_defn.roc -f fi_defn.awk fi_defn.txt
#   To build the adgfi_defn_lu.roc file:
#       nawk -vroclu_file=adgfi_defn_lu.roc -f fi_defn.awk fi_defn.txt
#   To build the adgfi_defn.h file:
#       nawk -vh_file=adgfi_defn.h -f fi_defn.awk fi_defn.txt
#
# (Use "awk" instead of "nawk" on non-Solaris machines.)
#
# For information on what's in the fi_defn.txt file, refer there.
# A short summary:
# group adi-operator-type
#   operator adi-operator
#	fi description (fi implementing that operator)
#	fi description
#	...
#   operator ...
#   ...
# group
# ....
#
# A FI description entry is a multi line entry which has the
# components:
#	fi		function instance number
#	description	description of fi
#	complement	complement FI id
#	flags		optional FI flags
#	lenspec		FI result length specification
#	workspace	optional workspace size
#	null_preinst	Pre-instruction for nullable data handling
#	result		result data type
#	operands	data type of operands if any
#	routine		FI handler routine
#	stcode		STCODE generation info
#
# For a summary of the relationships among the various operator and function
# instance definition files, see README.txt.
#
# History:
#	23-Mar-2005 (schka24)
#	    Replace clumsy and error-prone manual process with nicer
#	    semi-automated function instance construction.
#	28-Mar-2005 (schka24)
#	    Forgot to error-check entry keywords, do that.
#	31-Jul-2007 (kiria01) b117955
#	    Integrate with jam - much less error prone. Can generate
#	    the output files in one go, complete.
#       18-dec-2008 (joea)
#           Replace READONLY/WSCREADONLY by const.
#	aug-2009 (stephenb)
#		include adudate.h
#

# Routine to issue a marginally useful error and stop.
function fi_error(what)
{
    print "Error defining FI",fi,"in",what;
    print "Aborting at input line",NR;
    exit 1;
}

# Start a FI definition by defaulting all sorts of poop to blank.
function fi_start()
{
    description = "";
    complement = "";
    flags = "";
    lenspec_op = "";
    lenspec_size = "";
    lenspec_ps = ""
    wslen = "";
    null_preinst = "";
    result = "";
    operands = 0;
    operand_dt[0] = operand_dt[1] = operand_dt[2] = operand_dt[3] = "";
    routine = "";
    routine_b = "";
    routine_e = "";
}

# Routine to flush out a FI definition
function fi_flush()
{
    # Check for required things
    if (description == "")
	fi_error("description");
    if (cur_group == "ADI_COMPARISON" && complement == "")
	fi_error("complement (comparison)");
    if (lenspec_op == "")
	fi_error("lenspec");
    if (null_preinst == "")
	fi_error("null pre-instruction");
    if (result == "")
	fi_error("result datatype");
    if (cur_op == "")
	fi_error("missing OPERATOR context");
    # *** add stcode stuff when I get to it

    # Defaults for optional things
    if (complement == "")
	complement = "ADI_NOFI";
    if (flags == "")
	flags = "ADI_F0_NOFIFLAGS";
    if (lenspec_size == "")
    {
	lenspec_size = 0;
	lenspec_ps = 0;
    }
    else if (lenspec_ps == "")
    {
	lenspec_ps = 0;
    }
    if (wslen == "")
	wslen = 0;
    for (i=0; i < 4; i++)
    {
	if (operand_dt[i] == "")
	    operand_dt[i] = "DB_NODT";
    }
    if (routine == "")
	routine = "NULL";
    if (routine_b == "")
	routine_b = "NULL";
    if (routine_e == "")
	routine_e = "NULL"

    # Output FI stuff to FI description file
    if (roc_file != "")
    {
	print "/*" >descfile;
	printf "** [%d]:  %s\n",fidesc_ix, description >descfile;
	print "*/" >descfile;
	printf "{ %s, %s,\n", fi, complement >descfile;
	printf "  %s, %s, %s,\n", cur_group, flags, cur_op >descfile;
	printf "  {%s, %s, %s}, %s, %s,\n", lenspec_op, lenspec_size,
			lenspec_ps, wslen, null_preinst >descfile;
	printf "  %d, %s,\n", operands, result >descfile;
	printf "    %s, %s, %s, %s\n", operand_dt[0], operand_dt[1],
			operand_dt[2], operand_dt[3] >descfile;
	print "}," >descfile;
    }
    # Count another operator FI
    ++cur_op_count;

    # Store FI lookup info for sorted output at the end
    # Extract FI number from ADFI_nnn_blah.
    s = substr(fi,6);
    s = substr(s,1,index(s,"_"));	# Just the nnn part
    fi_no = s + 0;
    l_descix[fi_no] = fidesc_ix;
    l_routine[fi_no] = routine;
    l_routine_b[fi_no] = routine_b;
    l_routine_e[fi_no] = routine_e;
    l_finame[fi_no] = fi;
    if (fi_no > max_fi_no)
	max_fi_no = fi_no;

    # Move on to next FI entry, indicate not in a FI def now
    ++fidesc_ix;
    fi = "";
}


# Flush the current operator, which merely consists of outputting
# the count of FI's for this operator to the h_file file.
function op_flush()
{
    if (cur_op != "ADI_NOOP" && h_file != "")
	printf "#define ADO_%s_CNT\t%d\n",cur_op_root,cur_op_count >incfile;

    cur_op = "";
    cur_op_count = 0;
    cur_op_root = "";
}

# Initialize the current operator count, and output files.
BEGIN {
	cur_op = "";
	cur_op_count = 0;
# Defaults
	if (roc_file == "" && h_file == "" && roclu_file == "")
	{
	    descfile = "newfi_defn.roc";
	    lkupfile = "newfi_defn_lu.roc";
	    incfile = "newfi_defn.h";
	    roc_file = "";
	    roclu_file = "";
	    h_file = "";
        }
	else
	{
	    if (roc_file != "") descfile = roc_file;
	    if (h_file != "") incfile = h_file;
	    if (roclu_file != "") lkupfile = roclu_file;
	}
	fidesc_ix = 0;
	fi = "";
	max_fi_no = -1;
	if (h_file != "")
	{
	    #
	    # Generate header for 'incfile'
	    #
	    print "/**" >incfile;
	    print "** Name: ADFFI_DEFN.H - Contains number of fi's for each op and op type" >incfile;
	    print "**" >incfile;
	    print "** Description:" >incfile;
	    print "**	This file is generated by an awk script which walks through the" >incfile;
	    print "**	function instance table.  There are three types of #def'd constants" >incfile;
	    print "**	here:" >incfile;
	    print "**	    1) ADZ_<fi_type>_TY_FIIDX represents an index into the function" >incfile;
	    print "**		instance table where this class of function instances begins." >incfile;
	    print "**	    2) ADO_<op>_CNT gives the number of function instances which have" >incfile;
	    print "**		ADI_<op>_OP as their operation (they are assumed to be grouped" >incfile;
	    print "**		together in the function instance table)." >incfile;
	    print "**	    3) ADZ_<op>_FIIDX represents an index into the function instance" >incfile;
	    print "**	        table specifying where the first function instance which has" >incfile;
	    print "**		ADI_<op>_OP as its operation is." >incfile;
	    print "**" >incfile;
	    print "**	** DO NOT MAKE MANUAL CHANGES TO THIS FILE **" >incfile;
	    print "**" >incfile;
	    print "**	All function instances are defined in adf!adg!fi_defn.txt." >incfile;
	    print "**	That file is processed by fi_defn.awk.  To make changes to a" >incfile;
	    print "**	function instance, modify fi_defn.txt and run fi_defn.awk;" >incfile;
	    print "**/" >incfile;
	    print "" >incfile;
	    print "/* The following operations have no associated function instances */" >incfile;
	    print "#define ADO_EXIST_CNT 0" >incfile;
	    print "#define ADZ_EXIST_FIIDX ADZ_NOFIIDX" >incfile;
	    print "" >incfile;
	    print "#define ADO_NXIST_CNT 0" >incfile;
	    print "#define ADZ_NXIST_FIIDX ADZ_NOFIIDX" >incfile;
	}
	#
	# Generate header for 'descfile'
	#
    	if (roc_file != "")
	{
	    print "/*" >descfile;
	    print "** Copyright (c) 2007 Ingres Corporation" >descfile;
	    print "*/" >descfile;
	    print "#include <compat.h>" >descfile;
	    print "#include <gl.h>" >descfile;
	    print "#include <iicommon.h>" >descfile;
	    print "#include <adf.h>" >descfile;
	    print "#include <adfops.h>" >descfile;
	    print "#include <ade.h>" >descfile;
	    print "#include <adp.h>" >descfile;
	    print "#include <ulf.h>" >descfile;
	    print "#include <adfint.h>" >descfile;
	    print "#include <adffiids.h>" >descfile;
	    print "#include <adudate.h>" >descfile;
	    print "#include <aduint.h>" >descfile;
	    print "" >descfile;
	    print "/**" >descfile;
	    print "**" >descfile;
	    print "**  Name: ADGFI_DEFN.ROC - ADF's function instance table." >descfile;
	    print "**" >descfile;
	    print "**  Description:" >descfile;
	    print "**	This file contains the function instance table initialization which is a" >descfile;
	    print "**	table of ADI_FI_DESC structures, one per f.i.  These structures hold" >descfile;
	    print "**	everything ADF callers will need to see for any f.i. This file used to" >descfile;
	    print "**	also initialize the function instance lookup table, an ADF internal" >descfile;
	    print "**	structure used for quick access to a particular function instance" >descfile;
	    print "**	given the function instance ID" >descfile;
	    print "**" >descfile;
	    print "**	** DO NOT MAKE MANUAL CHANGES TO THIS FILE **" >descfile;
	    print "**" >descfile;
	    print "**	This file is generated from fi_defn.txt by the AWK program fi_defn.awk." >descfile;
	    print "**	To change function instance info, modify fi_defn.txt and run fi_defn.awk;" >descfile;
	    print "*/" >descfile;
	    print "" >descfile;
	    print "GLOBALDEF const ADI_FI_DESC Adi_fis[]= {" >descfile;
	    print "    /*" >descfile;
	    print "    ** This is the definition of ADF's function instance table.  Each element" >descfile;
	    print "    ** of the table is an ADI_FI_DESC structure which carries all information" >descfile;
	    print "    ** visible to the outside world for its corresponding function instance." >descfile;
	    print "    **" >descfile;
	    print "    **      You will notice that some function instances have "DB_ALL_TYPE" as" >descfile;
	    print "    **  their input or result datatype.  This is valid only for non-coercion" >descfile;
	    print "    **  function instances.  If a function instance has an input type of" >descfile;
	    print "    **  DB_ALL_TYPE, all types are valid for this function instance although" >descfile;
	    print "    **  this function instance will be less preferable than those which" >descfile;
	    print "    **  specifically mention the input types.  If a function instance has a" >descfile;
	    print "    **  result type of DB_ALL_TYPE, this means that the result datatype could be" >descfile;
	    print "    **  anything and must be determined by calling adi_res_type()." >descfile;
	    print "    */" >descfile;
	    print "" >descfile;
	    print "/*----------------------------------------------------------------------------*/" >descfile;
	    print "/*  Each array element in the function instance table looks like this:        */" >descfile;
	    print "/*----------------------------------------------------------------------------*/" >descfile;
	    print "/* /* [nnnn]:  Comment describing the function instance ...                   */" >descfile;
	    print "/*                                                                            */" >descfile;
	    print "/*  {   fi_ID,                          complment_fi_ID,                      */" >descfile;
	    print "/*          type_of_fi,         fi_flags,               op_ID,                */" >descfile;
	    print "/*              {     length_spec    },   aggr_w/s_dv_length, null_pre-instr, */" >descfile;
	    print "/*                  #_args,  result_dt,                                       */" >descfile;
	    print "/*                           arg_#1_dt,                                       */" >descfile;
	    print "/*                           arg_#2_dt,                                       */" >descfile;
	    print "/*                           arg_#3_dt,                                       */" >descfile;
	    print "/*                           arg_#4_dt    }                                   */" >descfile;
	    print "/*                                                                            */" >descfile;
	    print "/*----------------------------------------------------------------------------*/" >descfile;
	    print "" >descfile;
	}
	#
	# Generate header for 'descfile'
	#
    	if (roclu_file != "")
	{
	    print "/*" >lkupfile;
	    print "** Copyright (c) 2007 Ingres Corporation" >lkupfile;
	    print "*/" >lkupfile;
	    print "/* Jam hints" >lkupfile;
	    print "**" >lkupfile;
	    print "LIBRARY = IMPADFLIBDATA" >lkupfile;
	    print "**" >lkupfile;
	    print "*/" >lkupfile;
	    print "#include <compat.h>" >lkupfile;
	    print "#include <gl.h>" >lkupfile;
	    print "#include <iicommon.h>" >lkupfile;
	    print "#include <adf.h>" >lkupfile;
	    print "#include <adp.h>" >lkupfile;
	    print "#include <ulf.h>" >lkupfile;
	    print "#include <adfint.h>" >lkupfile;
	    print "#include <aduint.h>" >lkupfile;
	    print "#include <adudate.h>" >lkupfile;
	    print "" >lkupfile;
	    print "/**" >lkupfile;
	    print "**" >lkupfile;
	    print "**  Name: ADGFI_DEFN_LU.ROC - ADF's function instance lookup table." >lkupfile;
	    print "**" >lkupfile;
	    print "**  Description:" >lkupfile;
	    print "**	This file initializes the function instance lookup table, an ADF" >lkupfile;
	    print "**	internal structure used for quick access to a particular function" >lkupfile;
	    print "**	instance given the function instance ID." >lkupfile;
	    print "**" >lkupfile;
	    print "**	** DO NOT MAKE MANUAL CHANGES TO THIS FILE **" >lkupfile;
	    print "**" >lkupfile;
	    print "**	This file is generated from fi_defn.txt by the AWK program fi_defn.awk." >lkupfile;
	    print "**	To change function instance info, modify fi_defn.txt and run fi_defn.awk;" >lkupfile;
	    print "*/" >lkupfile;
	    print "" >lkupfile;
	    print "" >lkupfile;
	    print "/*" >lkupfile;
	    print "** Definition of all global variables used by this file." >lkupfile;
	    print "*/" >lkupfile;
	    print "" >lkupfile;
	    print "# ifdef NT_GENERIC" >lkupfile;
	    print "FACILITYREF ADI_FI_DESC Adi_fis[];" >lkupfile;
	    print "# else /* NT_GENERIC */" >lkupfile;
	    print "GLOBALREF ADI_FI_DESC Adi_fis[]; /* Actual Adf fi descriptors */" >lkupfile;
	    print "# endif" >lkupfile;
	    print "" >lkupfile;
	    print "GLOBALCONSTDEF ADI_FI_LOOKUP Adi_fi_lkup[] = {" >lkupfile;
	    print "/*" >lkupfile;
	    print "** This table is the function instance lookup table, and ADF internal" >lkupfile;
	    print "** structure used for quick access to a particular f.i. given the f.i. ID." >lkupfile;
	    print "** It also holds pieces of information for each f.i. that are not visible" >lkupfile;
	    print "** to the outside world.  The index into this table is equal to the f.i. ID" >lkupfile;
	    print "** of the f.i. corresponding to that row." >lkupfile;
	    print "**" >lkupfile;
	    print "** By having this level of indirection, we gain the ability to add new" >lkupfile;
	    print "** f.i.'s with new f.i. ID's, keep the f.i. table in the proper 'grouped'" >lkupfile;
	    print "** order, and not change existing f.i. ID's.  Therefore, once a f.i. ID" >lkupfile;
	    print "** has been established, it will not change.  This will allow f.i. IDs to" >lkupfile;
	    print "** be stored in the database in the future." >lkupfile;
	    print "*/" >lkupfile;
	    print "" >lkupfile;
	    print "/*----------------------------------------------------------------------------*/" >lkupfile;
	    print "/*  Each array element in the function instance lookup table looks like this: */" >lkupfile;
	    print "/*----------------------------------------------------------------------------*/" >lkupfile;
	    print "/*                                                                            */" >lkupfile;
	    print "/*  { pointer into      & f.i.          & agbgn         & agend        /* f.i.*/" >lkupfile;
	    print "/*     f.i. table,       routine,        routine,        routine }     /* ID  */" >lkupfile;
	    print "/*                                                                            */" >lkupfile;
	    print "/*----------------------------------------------------------------------------*/" >lkupfile;
	    print "" >lkupfile;
	}
}

# Ignore empty lines, comment lines
/^#/ {
	next;
}
/^[ 	]*$/ {
	next;
}

# group line ends FI and operator, starts a new group, outputs current
# FI index as the group's starting index for the h_file file.

$1 == "group" {
	if (fi != "")
	    fi_flush();
	if (cur_op != "")
	    op_flush();
    
	cur_group = $2;
	# Split off leading ADI
	s = substr(cur_group,index(cur_group,"_"));
	if (h_file != "")
	{
	    printf "\n#define ADZ%s_TY_FIIDX\t%d\n",s,fidesc_ix >incfile;
	}
	next;
}


# operator line ends FI and operator, starts a new operator, outputs
# current FI index as the operator's starting index for the h_file file.
$1 == "operator" {
	if (fi != "")
	    fi_flush();
	if (cur_op != "")
	    op_flush();
    
	cur_op = $2;
	if (cur_op != "ADI_NOOP")
	{
	    # Split off leading ADI_ and trailing _OP.
	    # A couple stupid special cases for operators not named
	    # ADI_blah_OP:
	    if (cur_op == "ADI_LONG_VARCHAR")
		cur_op_root = "LVARCH";
	    else if (cur_op == "ADI_LONG_BYTE")
		cur_op_root = "LBYTE";
	    else
	    {
		s = substr(cur_op,index(cur_op,"_")+1);
		cur_op_root = substr(s,1,index(s,"_OP")-1);
	    }
	    if (h_file != "")
	    {
		printf "\n#define ADZ_%s_FIIDX\t%d\n",cur_op_root,fidesc_ix >incfile;
	    }
	}
	next;
}


# fi line ends FI, starts a new function instance definition.
$1 == "fi" {
	if (fi != "")
	    fi_flush();
	fi_start();
	fi = $2;
	next;
}

# The next batch of keywords simply gather up FI definition stuff.

$1 == "description" {
	if (description != "")
	    fi_error("double description");
	# Everything from the first nonblank after "description"
	s = substr($0,index($0,"description")+12);
	description = substr(s,match(s,"[^ \t]"));
	next;
}

$1 == "complement" {
	if (complement != "")
	    fi_error("double complement");
	complement = $2;
	next;
}


$1 == "flags" {
	if (flags != "")
	    fi_error("double flags");
	flags = $2;
	next;
}

$1 == "lenspec" {
	if (lenspec_op != "")
	    fi_error("double lenspec");
	if (NF < 2 || NF > 4)
	    fi_error("lenspec must have 1, 2, or 3 items following")
	lenspec_op = $2;
	if ($3 != "")
	    lenspec_size = $3;
	if ($4 != "")
	    lenspec_ps = $4;
	next;
}

$1 == "workspace" {
	if (wslen != "")
	    fi_error("double workspace");
	wslen = $2;
	next;
}

$1 == "null_preinst" {
	if (null_preinst != "")
	    fi_error("double null_preinst");
	null_preinst = $2;
	next;
}

$1 == "result" {
	if (result != "")
	    fi_error("double result datatype");
	result = $2;
	next;
}

$1 == "operands" || $1 == "operand" {
	if (operands != 0)
	    fi_error("double operands");
	operands = NF - 1;
	if (operands > 4)
	    fi_error("too many operands");
	for (i=0; i<operands; i++)
	    operand_dt[i] = $(i+2);
	next;
}

$1 == "routine" {
	if (routine != "")
	    fi_error("double routine");
	if (cur_group == "ADI_AGG_FUNC")
	{
	    if (NF != 4)
		fi_error("aggs require 3 routines");
	}
	else
	{
	    if (NF != 2)
		fi_error("non-aggs only allow 1 routine");
	}
	routine = $2;
	if (NF > 2)
	{
	    routine_b = $3;
	    routine_e = $4;
	}
	next;
}

# stcode stuff to go here

# Anything else is an error line.
{
	fi_error("Unknown keyword " $1);
}

# At EOF, flush current FI, current operator, then output the lookup
# table according to FI number.
END {
	if (fi != "")
	    fi_flush();
	if (cur_op != "")
	    op_flush();

	# Tie off the h_file file, we're done with it
	if (h_file != "")
	{
		close (incfile);
	}

	# Append an "end" entry to the FI description file, then
	# close it.
    	if (roc_file != "")
	{
	    print "/*" >descfile;
	    print "** End-of-table null entry" >descfile
	    print "*/" >descfile;
	    print "{ ADFI_ENDTAB, ADFI_ENDTAB," >descfile;
	    print "  0, 0, ADI_NOOP," >descfile;
	    print "  {0, 0, 0}, 0, 0," >descfile;
	    print "  0, DB_NODT," >descfile;
	    print "    DB_NODT, DB_NODT, DB_NODT, DB_NODT" >descfile;
	    print "}" >descfile;
	    print "};" >descfile;
	    print "" >descfile;
	    print "GLOBALDEF const i4 Adi_fis_size = sizeof(Adi_fis);" >descfile;
	    close(descfile);
	}
        if (roclu_file != "")
	{
	    # Write out the FI lookup table entries in order
    
	    for (i = 0; i <= max_fi_no; i++)
	    {
		if (l_descix[i] == "")
		    printf "{ NULL, NULL, NULL, NULL },\t/* No FI %d */\n",i >lkupfile;
		else
		    printf "{ &Adi_fis[%d], %s, %s, %s },\t/* %s */\n", l_descix[i],
			    l_routine[i], l_routine_b[i], l_routine_e[i],
			    l_finame[i] >lkupfile;
	    }
	    # Finish with a closing entry and the size declaration.
	    print "{ NULL, NULL, NULL, NULL }" >lkupfile;
	    print "};" >lkupfile;
	    print "" >lkupfile;
	    print "GLOBALDEF const i4 Adi_fil_size = sizeof(Adi_fi_lkup);" >lkupfile;
	    close(lkupfile);
	}
}
