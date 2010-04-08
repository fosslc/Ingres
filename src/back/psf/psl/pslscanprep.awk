# Copyright (c) 2010 Ingres Corporation
#
# pslscanprep.awk - take the guess work out of building the token table.
#
# Reads a token list and formats the Key_string[] and *Key_index[]
# tables to be included by the scanners.
#
# The format of the data file is a simple list of index number and
# token text in lowercase. The index number must correspond to the
# corresponding slot in the main KEYINFO Key_info[] table.
#
# The list must be in token length order and sorted within equal lengths.
# These conditions are checked for!
#
# For Quel:
#	awk -f pslscanprep.awk -vOUT=pslscantoks.h  pslscantoks.dat
# For SQL:
#	awk -f pslscanprep.awk -vOUT=pslsscantoks.h pslsscantoks.dat
#
# History:
#	13-Mar-2010 (kiria01)
#	    Created after conflict found between collate and comment
#	    when adding singleton.
#
BEGIN{
	if (OUT==""){
		print "OUT variable is not defined"
		exit -1
	}
	curlen
	idx=0
	last=""
}
hdrdone!=1{
	hdrdone=1
	print "/*"					>OUT
	print "** "OUT" generated from "FILENAME	>OUT
	print "** using pslscanprep.awk"		>OUT
	print "*/\n\n"					>OUT
	print "static const u_char Key_string[] = {"	>OUT
}
# Support block comments - /* and */ on their own lines
$0=="/*"{	if (incom){
			print "Comment Error at line "NR>OUT
			exit
		}
		incom=1
}
$0=="*/"{	if (!incom){
			print "Comment Error at line "NR>OUT
			exit
		}
		incom=0
		print $0				>OUT
		next
}
incom==1{	print $0				>OUT
		next
}

$0~"^/[*].*[*]/$"{
	if (comment != ""){
		printf "%18s%s\n","",comment		>OUT
	}
	comment = $0
	next
}

NF==2{
	l=length($2)
	if (l != curlen){
		if (l < curlen){
			print "CURLEN mismatch:["NR"] "$0
			exit -1
		}
		curlen = l
		printf "%-16s%3d,\n", "/* "idx" */", 0	>OUT
		if (comment != ""){
			printf "%18s%s\n","",comment	>OUT
			comment = ""
		}
		idx++
		last=""
		starts[curlen]=idx
		printf "%-16s", "/* "idx" */"		>OUT
		if ($1<10)printf " "			>OUT
		if ($1<100)printf " "			>OUT
		printf "%d,", $1			>OUT
	}else if($2 <= last){
		print "Order error: '"last"' vs. '"$2"'"
		exit -1
	}else{
		printf "%-16s", ""			>OUT
		if ($1<10)printf " "			>OUT
		if ($1<100)printf " "			>OUT
		printf "%d,", $1			>OUT
	}
	if (curlen < 11) printf " "			>OUT
	for(i = 1; i <= curlen; i++){
		if (curlen > 16 && i == 16){
			printf "\n%20s",""		>OUT
		}
		printf "'%s',",substr($2,i,1)		>OUT
		if (curlen < 11) printf " "		>OUT
	}
	printf "0,\n"					>OUT
	idx += curlen+2
	last=$2
}
END{
	printf "%-16s%3d\n", "/* "idx" */", 0		>OUT
	if (comment != ""){
		printf "%18s%s\n","",comment		>OUT
		comment = ""
	}
	printf "%18s};\n\n",""				>OUT
	print "/*">OUT
	print "** This table contains pointers into the above Psl_keystr[] array.  The">OUT
	print "** n'th pointer points to the keywords of length n in Psl_keystr (where n">OUT
	print "** begins at zero).">OUT
	print "*/">OUT

	print "\nstatic const u_char *Key_index[] = {"	>OUT
	curlen=0
	while(curlen <= 32){
		printf "\t\t&Key_string[%4d],\t/* Keywords of length%3d */\n",\
				0+starts[curlen],curlen	>OUT
		curlen++
	}
	print "\t\t};"					>OUT
}

