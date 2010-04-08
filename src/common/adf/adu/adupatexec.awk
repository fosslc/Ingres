#
# adupatexec.awk - State engine compiler for pattern matching code.
#
# This script implemements a simple parser for a file based on the DOT
# acyclic directed graph format. It recogonises and interprets the state
# information from the implied state graph and generates C code to
# implement the engine.
#
#   Variable params:
#       header      Filename to use to produce header definitions
#       body        Filename to use to produce state machine body
#       out         Filename to output formatted graph to
#       subgraphs   Extract subgrapss if > 0
#       pretty      Re-jig ouput so that graph prints better
#       verbose     Dump extra data
# 
#   Build commands:
#     To build the opcode definition file:
#	nawk -v header=adupatexec_ops.h -f adupatexec.awk
#     To build the state code:
#	nawk -v body=adupatexec_inc.i -f adupatexec.awk
#     To generate the prettied sub-graphs...
#	nawk -v pretty=1 -v out=file -f adupatexec.awk
#
# History:
#	28-May-2008 (kiria01) SIR120473
#	    Created
#	12-Aug-2008 (kiria01) SIR120473
#	    Drop the use of asort() to pretty the constant
#	    list - it a gawk extension not available with
#	    Windows MKS or Solaris awk.
#	30-Dec-2008 (kiria01) SIR120473
#	    Added support for PAT_FNDLIT to allow for more efficient
#	    pattern scans for literals,
#	08-Jan-2009 (kiria01) SIR120473
#	    Revise prior PAT_FNDLIT chnage to stop it skipping text that
#	    had part matched the pattern.
#	30-May-2009 (kiria01) b122128
#	    Allow fast path for PAT_LIT and PAT_FNDLIT when we are in the
#	    end (or only) block. Adds EOFSEEN, PATROOM, CMPWHLPAT decisions
#	    and CHSKPPAT to advance CH the pattern length.
#	    Add optimised instruction ENDLIT when pattern at end of string
#	    with EOFCLOSE and ENDPOS.
#	28-Jul-2009 (kiria01)
#	    Drop spurious emit from sub-graph creation
#       22-Oct-2009 (hanal04) Bug 122777
#           NEXTCH uses REG_bufend which points to the end of the row buffer.
#           If we want to use NEXTCH to move over the pattern buffer we need
#           to temporarily set REG_bufend to ctx->patbufend and then reset it
#           after NEXTCH completes.
BEGIN{
    cur=""
    if(pretty != 1) pretty = 0
    alli=0
    st=-5
    op=-1
    if (header == "") header = "NONE";
    if (body == "") body = "NONE"
    if (out == "") out = "NONE"
    if (list == "") list = "NONE"
    if (verbose == "") verbose = 0
    SFX=".dotfile"
    fname=""
    TEST=1
    ACTION=2
    FORK=3
    TERM=4
    SWITCH=5
}

function dumpheader(){
    if (header != "NONE"){
	print "#ifndef __ADUPATEXEC_OPS_H_INC">header
	print "#define _ADUPATEXEC_OPS_H_INC">header
	print "/*">header
	print "** adupatexec_ops.h - Generated from",FILENAME>header
	print "**">header
	print "** This include file defines the constants for the">header
	print "** pattern matching state engine and its interface.">header
	print "**">header
	print "** The _DEFINEOP extra parameters are used by the">header
	print "** pat_load raw interface for decoding the optional">header
	print "** parameters and by adu_pat_disass() called by the">header
	print "** pat_dump code.">header
	print "*/">header
	print "">header
	print "#define PAT_OPCODES \\">header
    }
    for (i=0; i<op;i++){
	if (stateopn[i] in ops){
	    opclass=stateopn[i];sub("^PAT_","",opclass);sub("_.*$","",opclass)
	    v1="_";v2="_"
	    s1=substr(stateopn[i],length(opclass)+6,1)
	    s2=substr(stateopn[i],length(opclass)+8,1)
	    if (s1 ~ "[01]") v1 = s1
	    if (s1 ~ "[W]"||s2 ~ "[W]") v2 = "W"
	    if (s1 == ""||s1 ~ "[01W]") s1 = "_"
	    if (s2 == ""||s2 ~ "[W]") s2 = "_"
	    s3="_"
	    if (opclass=="LIT"||opclass=="FNDLIT"||opclass=="COMMENT")s3="L"
	    if (opclass=="JUMP"||opclass=="LABEL"||
		opclass=="FOR"||opclass=="NEXT")s3="J"
	    if (opclass=="CASE")s3="C"
	    if (opclass=="SET")s3="S"
	    if (opclass=="BSET")s3="B"
	    if (opclass=="NSET")s3="NS"
	    if (opclass=="NBSET")s3="NB"
	    if (header != "NONE")printf("_DEFINEOP(%-20s, %3s,%8s, %s, %s,%2s, %s, %s)\\\n",
			stateopn[i], i, opclass, s1, s2, s3, v1, v2)>header
	}else{
	    if (header != "NONE")printf("_DEFINEEX(%-20s, %3s)\\\n", stateopn[i], i)>header
	}
	jumpto[stateopn[i]]--
	swto[stateopn[i]]++
	opclasses[opclass]=opclass
	if(!(opclass in opidx))opidx[opclass]=i
    }
    if (header != "NONE"){
	print "_ENDDEFINE">header
	print "">header
	print "#define PAT_OPTYPES \\">header
	ii=0
	for (i in opclasses){
	    printf("_DEFINE(%-10s, %2d, %2d)\\\n", opclasses[i],ii++,opidx[opclasses[i]])>header
	}
	print "_ENDDEFINE">header
	print "">header
	print "#endif /*_ADUPATEXEC_OPS_H_INC*/">header
    }
}
function emit(){
    if(cur!=""){
	#if(pretty) print "subgraph \"cluster" cur "\" { label=\""cur"\""sgl[cur]"}"
	curSFX=cur SFX
	if(pretty) print "}">curSFX
	if (list!="NONE") print curSFX>list
    }
    cur=""
}
function printb(){
    if (body !="NONE") print $1>body
}

function gettag(tags,tag){
    if (tags~tag){
	val=tags
	if(val~tag"[ 	]*=[ 	]*\""){
	    sub(".*"tag"[ 	]*=[ 	]*\"","",val)
	    sub("\".*$","",val)
	}else{
	    sub(".*"tag"[ 	]*=[ 	]*","",val)
	    sub("[^a-zA-Z0-9_-].*$","",val)
	}
    }else{
	val=""
    }
    return val
}

# Reset work variable for each line
{
    if (FILENAME != fname){
	if (FILENAME ~ "[.]dot$") SFX=".dot"
	if (FILENAME ~ "[.]dotty$") SFX=".dotty"
	fname=FILENAME
    }
    p1="";p2="";node=0;link=0;nodetype=TEST;label="";comments=""
}

# Save lines marked with comment ALL so that it can be output with all subgraphs
$0~"^/\\*ALL"{
    sub("/[*]ALL[*]/[ 	]*","")
    all[alli]=$0
    alli++
}

# Pull apart link records    A->B or A->B [tags]
$0~"->"{
    l=$0
    gsub("/[*].*[*]/","",l)
    gsub("^[ 	]*","",l)
    p1=l;sub("[ 	]*->.*","",p1)
    p2=l;sub("^.*->[ 	]*","",p2);sub("[ 	]*[[].*$","",p2)
    link=1
    tags=$0
    if(tags~"[[]"){
	sub("^[^[]*[[]","",tags);sub("][^]]*$","",tags)
	comments=gettag(tags,"comment")
	label=gettag(tags,"label")
    }
}

# Pull apart node records    A [tags]
# Node shape determines node type
$0!~"->"&&$0~"[[]"{
    l=$0
    gsub("/[*].*[*]/[ 	]*","",l)
    p1=l;sub("^[ 	]*","",p1);sub("[^a-zA-Z0-9_].*$","",p1)
    if (p1 != "graph" && p1 != "node" && p1 != "edge"){
	p2=""
	node=1
	tags=$0
	if(tags~"[[]"){
	    sub("^[^[]*[[]","",tags);sub("][^]]*$","",tags)
	    comments=gettag(tags,"comment")		
	    label=gettag(tags,"label")
	    shape=gettag(tags,"shape")
	    if(shape=="box") nodetype=ACTION
	    if(shape=="circle") nodetype=TERM
	    if(shape=="triangle") nodetype=FORK
	    if(shape=="hexagon"){
		nodetype=SWITCH
		switchnode=p1
	    }
	}
    }
}

# Trace opportunity to see what file noise there is
link==0&&node==0{
    #print "HERE",$0
}

# If link is involved with a switch then the targets will
# be case labels and the selector value will be the OPCODE
# value.
switchnode != "" && p1==switchnode && link==1 && comments!="default"{
    ops[p2]=p2
    casenode_line[p2]=$0
    o=0+comments
    if (o in stateopn){
	print "ERROR op id clash:",stateopn[o],"vs",p2
    }else{
	stateopn[o]=p2
	stateopv[p2]=o
    }
    if (o>=op)op=o+1
    
}

$0==""{
    if(pretty) print $0>out
    next
}
$0~"^/\\*"{
    if(pretty) print $0>out
    next
}

#
# What to do with a node
#
node==1{
    if (p1 in ops){
	emit()
	cur=p1
	tmp=all[0]
	sub("\"PatMat\"","\""cur"\"",tmp)
	curSFX=cur SFX
	if(pretty) print tmp>curSFX
	for(i =1; i<alli;i++){
	    if(pretty) print all[i]>curSFX
	}
	if(pretty) print "label=\"Subgraph for "cur".\"">curSFX
	if(pretty) print "size=\"5,5\"">curSFX
	if(pretty) print "rankdir=LR">curSFX
	if(pretty) print casenode_line[cur]>curSFX
    }
    sgl[cur]=sgl[cur]";"p1
    if(pretty) print $0>out
    curSFX=cur SFX
    if(pretty) print $0>curSFX
    if (p1 in states){
	print "ERROR - redefined state:"p1
	print $0
    }else{
	states[p1]=p1
	if (!(p1 in statev)){
	    staten[st]=p1
	    statev[p1]=st++
	}
	if (nodetype==ACTION || nodetype==TERM){
	    stateaction[p1]=comments
	    if (comments=="REFER")
		statetype[p1]=ACTION
	    else
		statetype[p1]=nodetype
	}else if(nodetype==TEST){
	    statetype[p1]=nodetype
	    statetest[p1]=comments
	    if((comments=="CHAV"||comments=="EOFCLOSE") && !(p1 in stateopv)){
		stateopn[op]=p1
		stateopv[p1]=op
		op++
	    }
	}else{
	    statetype[p1]=nodetype
	}
    }
    next
}

#
# What to do with a link
#
link==1{
    jumpto[p2]++
    if(statetype[p1]==ACTION) stateyes[p1]=p2
    else if(label=="yes") stateyes[p1]=p2
    else if(label=="no") stateno[p1]=p2
    else if(label=="parent") stateparent[p1]=p2
    else if(label=="child") statechild[p1]=p2
    else if(label=="More pat") stateno[p1]=p2
    else if(statetype[p1]==TERM){
	print"ERROR in link - link for TERM?:",p1,p2
    }else if((label!=""||statetype[p1]!=""||comments!="")&&statetype[p1]!=SWITCH){
	print"ERROR in link:",p1,p2
	print"label",label
	print"type",statetype[p1]
    }
}

#
# Certain graph forms present better if some of the links
# are reversed and the arrows interchanged.
#
pretty && link==1 && comments=="CYC"{
    cyc=p2
    label=$0
    sub("^.*label=","[comment=\"REV\",arrowhead=none,arrowtail=normal,headlabel=",label)
    sub("^.*comment=\"CYC\"","[comment=\"REV\",arrowhead=none,arrowtail=normal",label)
    print p2"->"p1 label>out
    curSFX=cur SFX
    print p2"->"p1 label>curSFX
    next
}
pretty && cyc!="" && p1==cyc && link==1{
    label="[comment=\"REV\",arrowhead=none,arrowtail=normal]"
    print p2"->"p1 label>out
    curSFX=cur SFX
    print p2"->"p1 label>curSFX
    cyc=""
    next
}

#
# Normal record output
#
{
    if(pretty) print $0>out
    curSFX=cur SFX
    if(cur!="" && pretty) print $0>curSFX
}
END{
    # Make sure all child runon states are
    # 'cased' & end in the opcode list
    for(i = -5; i < st; i++){
	s=staten[i]
	if(statetype[s]==FORK){
	    if (!(statechild[s] in stateopv)){
		stateopn[op]=statechild[s]
		stateopv[statechild[s]]=op
		op++
	    }
	}else if(statetype[s]==SWITCH){
	    jumpto[stateno[s]]++
	}
    }
    if (header !="NONE"){
	dumpheader()
    }else{
	for (i=0; i<op;i++){
	    jumpto[stateopn[i]]--
	    swto[stateopn[i]]++
	}
    }

    if (body != "NONE"){
	print "/*">body
	print "** adupatexec_inc.i - Generated from",FILENAME>body
	print "**">body
	print "** This include file implements the state machine for">body
	print "** pattern matching.">body
	print "*/">body
	print "">body
	for(i = 0; i < st; i++){
	    s=staten[i]
	    if (swto[s]>0) {
		print "\ncase " s ":">body
		if (s in ops){
		    opclass=s;sub("^PAT_","",opclass);sub("_.*$","",opclass)
		    s1=substr(s,length(opclass)+6,1)
		    s2=substr(s,length(opclass)+8,1)
		    if (verbose > 0)print opclass,s1,s2
		    if (opclass=="ANY"||
			opclass=="SET"||opclass=="NSET"||
			opclass=="BSET"||opclass=="NBSET"){
			if (s1=="N") {
			    print "\tREG_N = GETNUM(REG_PC);">body
			}
			if (s2=="M") {
			    print "\tREG_M = GETNUM(REG_PC);">body
			}
			if (opclass=="SET") {
			    print "\ttmp = GETNUM(REG_PC);">body
			    print "\tREG_litset = REG_PC;">body
			    print "\tREG_PC += tmp;">body
			    print "\tREG_litsetend = REG_PC;">body
			} else if (opclass=="NSET") {
			    print "\ttmp = GETNUM(REG_PC);">body
			    print "\tREG_litset = REG_PC;">body
			    print "\tREG_PC += tmp;">body
			    print "\tREG_O = GETNUM(REG_litset);">body
			    print "\tREG_litsetend = REG_PC;">body
			} else if (opclass=="BSET"||opclass=="NBSET") {
			    print "\tREG_O = GETNUM(REG_PC);">body
			    print "\tREG_L = GETNUM(REG_PC);">body
			    print "\tREG_litset = REG_PC;">body
			    print "\tREG_PC += REG_L;">body
			    print "\tREG_litsetend = REG_PC;">body
			}
		    }
		}
	    }
	    if (jumpto[s]>1 ||
		    jumpto[s]==1 &&
			stateyes[staten[statev[s]-1]]!=s &&
			swto[s]==0 ||
		    jumpto[s]==1 &&
			swto[s]==1) {
		print "/**/_" s ":">body
	    }
	    if (statetest[s]!=""){
		if (stateno[s]!=""){
		    print "\t/*"statetest[s]"(_" stateno[s]")*/">body
		}else{
		    print "\t/*"statetest[s]"*/">body
		}
	    }
	    if (statetype[s]==TEST && (statetest[s]=="CHAV" || statetest[s]=="EOFCLOSE"))
		print "\tctx->state = " s ";">body
	    if(statetype[s]==TEST){
		if (statetest[s] == "CHAV"){
		    #print "\tif (!(tst" statetest[s] "(ctx))) goto _" stateno[s]";">body
		    print "\tif (REG_CH >= REG_bufend){">body
		    print "\t\tif (ctx->parent->at_eof) goto _" stateno[s]";">body
		    print "\t\tgoto _STALL;">body
		    print "\t}">body
		}else if (statetest[s] == "EOFCLOSE"){
		    # Note that presently we can't support collation this way
		    print "#ifdef LITPRIME">body
		    print "\tgoto _" stateno[s]";">body
		    print "#endif">body
		    print "\t\t/* If space too small fail */">body
		    print "\tif (da_ctx->eof_offset - da_ctx->seg_offset <">body
		    print "\t\tREG_CH - sea_ctx->buffer + REG_L)goto _" stateno[s]";">body
		    print "\t\t/* If not in expected segment - stall */">body
		    print "\tif (da_ctx->eof_offset - da_ctx->seg_offset >">body
		    print "\t\tda_ctx->under_dv.db_length - sizeof(i2) + REG_L)goto _STALL;">body
		}else if (statetest[s] == "MOREPAT"){
		    print "\tif (REG_litset >= REG_litsetend)goto _" stateno[s]";">body
		}else if (statetest[s] == "PATROOM"){
		    print "\tif (REG_CH+REG_L > REG_bufend)goto _" stateno[s]";">body
		}else if (statetest[s] == "CMPWHLPAT"){
		    print "#ifndef MECMP_LOOP">body
		    print "\tif (MEcmp(REG_CH, REG_litset,REG_L))goto _" stateno[s]";">body
		    print "#else">body
		    print "\t{">body
		    print "\t    register CHAR *CHp = (CHAR*)REG_CH;">body
		    print "\t    while (REG_litset<REG_litsetend){">body
		    print "\t\tif (*CHp != *(CHAR*)REG_litset){">body
		    print "\t\t    REG_litset = REG_litsetend-REG_L;">body
		    print "\t\t    goto _" stateno[s]";">body
		    print "\t\t}">body
		    print "\t\tCHp++;">body
		    print "\t\tREG_litset += sizeof(CHAR);">body
		    print "\t    }">body
		    print "\t}">body
		    print "#endif /*MEMCMP_INTRINSIC*/">body
		}else if (statetest[s] == "CHINBSET"){
		    print "\tch = *(CHAR*)REG_CH - REG_O;">body
		    print "\tif ((CHAR)ch>=8*REG_L ||">body
		    print "\t\t!((1<<(ch&7))&REG_litset[ch>>3])) goto _" stateno[s]";">body
		}else if (statetest[s] == "CHNINBSET"){
		    print "\tch = *(CHAR*)REG_CH - REG_O;">body
		    print "\tif ((CHAR)ch<8*REG_L &&">body
		    print "\t\t(1<<(ch&7))&REG_litset[ch>>3]) goto _" stateno[s]";">body
		}else if (statetest[s] == "CHINSET"){
		    print "#ifdef " statetest[s]>body
		    print "\t"statetest[s]"(_" stateno[s]")">body
		    print "#else">body
		    print "\tif ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _"stateno[s]";">body
		    print "\tfor (; ; ADV(t)){">body
		    print "\t    if (HAVERANGE(t)){">body
		    print "\t	if(COMPARECH(t)<0){">body
		    print "\t	    ADV(t);">body
		    print "\t	}else{">body
		    print "\t	    ADV(t);">body
		    print "\t	    if(COMPARECH(t)<=0) break;">body
		    print "\t	}">body
		    print "\t    }else if (!COMPARECH(t)) break;">body
		    print "\t    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _"stateno[s]";">body
		    print "\t}">body
		    print "#endif /* " statetest[s]" */">body
		}else if (statetest[s] == "CHNINSET"){
		    print "#ifdef " statetest[s]>body
		    print "\t"statetest[s]"(_" stateno[s]")">body
		    print "#else">body
		    print "\tfor (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){">body
		    print "\t    if (HAVERANGE(t)){">body
		    print "\t	if(COMPARECH(t)<0){">body
		    print "\t	    ADV(t);">body
		    print "\t	}else{">body
		    print "\t	    ADV(t);">body
		    print "\t	    if(COMPARECH(t)<=0) break;">body
		    print "\t	}">body
		    print "\t    }else if (!COMPARECH(t)) break;">body
		    print "\t    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _"stateno[s]";">body
		    print "\t}">body
		    print "\tfor (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){">body
		    print "\t    if (HAVERANGE(t)){">body
		    print "\t	if(COMPARECH(t)<0){">body
		    print "\t	    ADV(t);">body
		    print "\t	}else{">body
		    print "\t	    ADV(t);">body
		    print "\t	    if(COMPARECH(t)<=0) goto _"stateno[s]";">body
		    print "\t	}">body
		    print "\t    }else if (!COMPARECH(t)) goto _"stateno[s]";">body
		    print "\t    if (last_diff < 0) break;">body
		    print "\t}">body
		    print "#endif /* " statetest[s]" */">body
		}else if (statetest[s] == "EOFSEEN"){
		    # Note that presently we can't support collation this way
		    print "#ifdef LITPRIME">body
		    print "\tgoto _" stateno[s]";">body
		    print "#endif">body
		    print "\tif (!sea_ctx->at_eof)goto _" stateno[s]";">body
		}else if (statetest[s] == "NGT0"){
		    print "\tif (REG_N <= 0) goto _" stateno[s]";">body
		}else if (statetest[s] == "MGT0"){
		    print "\tif (REG_M <= 0) goto _" stateno[s]";">body
		}else if (statetest[s] == "DIFFPAT"){
		    print "\tDIFFPAT(_" stateno[s]")">body
		}else{
		    print "\tif (!(tst" statetest[s] "(ctx))) goto _" stateno[s]";">body
		}
		if (statev[s]+1==statev[stateyes[s]]){
		    #print "\t/*NEXT*/"
		}else{
		    print "\tgoto _" stateyes[s]";">body
		}
	    }else if(statetype[s]==FORK){
		print "\tREG_STORE(ctx)">body
		print "\tFORK(ctx,"statechild[s]");">body
		print "\t/* REG_SYNC(ctx) */">body
		if (statev[s]+1==statev[stateparent[s]]){
		    #print "\t/*NEXT*/">body
		}else{
		    print "\tgoto _" stateparent[s]";">body
		}
	    }else if(statetype[s]==ACTION){
		if (stateaction[s] == "N0"){
		    #print "\tREG_N = GETNUM(REG_PC);">body
		}else if (stateaction[s] == "M0"){
		    #print "\tREG_M = GETNUM(REG_PC);">body
		}else if (stateaction[s] == "NM0"){
		    #print "\tREG_N = GETNUM(REG_PC);">body
		    #print "\tREG_M = GETNUM(REG_PC);">body
		}else if (stateaction[s] == "CHINC"){
		    print "\tNEXTCH">body
		}else if (stateaction[s] == "CHINC_PATINC"){
		    print "\tNEXTPAT">body
		    print "\tNEXTCH">body
		}else if (stateaction[s] == "PATINC"){
		    print "\tNEXTPAT">body
		}else if (stateaction[s] == "NDEC_CHINC"){
		    print "\tREG_N--;">body
		    print "\tNEXTCH">body
		}else if (stateaction[s] == "MDEC"){
		    print "\tREG_M--;">body
		}else if (stateaction[s] == "REFER"){
		    # print "\t/*SKIP*/">body
		}else if (stateaction[s] == "PATINIT"){
		    print "\tREG_L = GETNUM(REG_PC);">body
		    print "\tREG_litset = REG_PC;">body
		    print "\tREG_PC += REG_L;">body
		    print "\tREG_litsetend = REG_PC;">body
		    print "#ifdef LITPRIME">body
		    print "\tLITPRIME">body
		    print "#endif">body
		}else if (stateaction[s] == "CHSKPPAT"){
		    print "\tREG_CH += REG_L;">body
		}else if (stateaction[s] == "ENDPOS"){
		    print "\t/* Position CH at where ENDLIT must start */">body
		    print "\tREG_CH = da_ctx->eof_offset - da_ctx->seg_offset +">body
		    print "\t\t\t\tsea_ctx->buffer - REG_L;">body
		}else if (stateaction[s] == "PATRESET"){
		    # Optimised containings scan.
		    # The added complexity here is due to what happens
		    # after a partial match is spotted but fails at some point.
		    # We need to wind bach the CH ptr to the next char prior
		    # to then match. However, that might be in the previous
		    # segment and gone. All is not lost, the matched data is in
		    # the litset buffer so we sub-scan that for another match
		    # just in case and then resume with pointers left intact.
		    print "\t{">body
		    print "\t    register u_i1 *PATHOLD = REG_litset;">body
		    print "\t    REG_litset = REG_litsetend - REG_L;">body
		    print "\t    if (PATHOLD > REG_litset)">body
		    print "\t    {">body
		    print "\t\tregister u_i1 *CHHOLD = REG_CH;">body
		    print "\t\tregister u_i1 *CHp = REG_litset;">body
		    print "#ifdef CHPRIME">body
                    print "\t\tregister u_i1 *REG_save;">body
		    print "\t\tCHSAVE(hold)">body
		    print "\t\tREG_CH = CHp;">body
		    print "\t\tCHPRIME">body
		    print "\t\tCHp = REG_CH;">body
		    print "\t\tCHSAVE(pat)">body
		    print "#endif">body
		    print "\t\tfor(;;) {/* rescan points of pre-match */">body
		    print "#ifdef LITPRIME">body
		    print "\t\t    LITPRIME">body
		    print "#endif">body
		    print "#ifdef CHPRIME">body
		    print "\t\t    CHRESTORE(pat)">body
		    print "\t\t    REG_save = REG_bufend;">body
                    print "\t\t    REG_bufend = ctx->patbufend;">body
		    print "#endif">body
		    print "\t\t    REG_CH = CHp;">body
		    print "\t\t    NEXTCH">body
		    print "\t\t    CHp = REG_CH;">body
		    print "#ifdef CHPRIME">body
                    print "\t\t    REG_bufend = REG_save;">body
		    print "\t\t    CHSAVE(pat)">body
		    print "#endif">body
		    print "\t\t    for(;;) {">body
		    print "\t\t\tif (REG_CH >= PATHOLD) {">body
		    print "\t\t\t    REG_CH = CHHOLD;">body
		    print "#ifdef CHPRIME">body
		    print "\t\t\t    CHRESTORE(hold)">body
		    print "#endif">body
		    print "\t\t\t    goto _"s"loop;">body
		    print "\t\t\t}">body
		    print "\t\t\tDIFFPAT(_"s"exit)">body
		    print "\t\t\tNEXTCH">body
		    print "\t\t\tNEXTPAT">body
		    print "\t\t    }">body
		    print "_"s"exit:">body
		    print "\t\t    REG_litset = REG_litsetend - REG_L;">body
		    print "\t\t}">body
		    print "\t    } else {">body
		    print "#ifdef LITPRIME">body
		    print "\t\tLITPRIME">body
		    print "#endif">body
		    print "\t\tNEXTCH">body
		    print "\t    }">body
		    print "\t}">body
		    print "_"s"loop:">body
		}else if (stateaction[s] == "CASE"){
		    print "\ttmp = GETNUM(REG_PC);">body
		    print "\tREG_STORE(ctx)">body
		    print "\tFORKN(ctx, (u_i4)tmp);">body
		    print "\tREG_SYNC(ctx)">body
		}else if (stateaction[s] == "JUMP"){
		    print "\ttmp = GETNUM(REG_PC);">body
		    print "\tREG_PC += tmp;">body
		}else if (stateaction[s] == "JUMP" ||
			stateaction[s] == "COMMENT"){
		    print "\ttmp = GETNUM(REG_PC);">body
		    print "\tREG_PC += tmp;">body
		}else if (stateaction[s] == "LABEL"){
		    print "\ttmp = GETNUM(REG_PC);">body
		}else if (stateaction[s] == "FOR"){
		    print "\t/*FRAMEPUSH*/">body
		    print "\tctx->sp++;">body
		    print "\t/*Get args*/">body
		    print "\tctx->frame[ctx->sp].N = GETNUM(REG_PC);">body
		    print "\tctx->frame[ctx->sp].M = GETNUM(REG_PC);">body
		    print "\ttmp = GETNUM(REG_PC);">body
		    print "\tREG_PC += tmp;">body
		}else if (stateaction[s] == "NEXT"){
		    print "\t/*Get arg*/">body
		    print "\ttmp = GETNUM(REG_PC);">body
		    print "\t/*Check N count expired*/">body
		    print "\tif (ctx->frame[ctx->sp].N > 0){">body
		    print "\t    /*Decr & do 1 iter*/">body
		    print "\t    ctx->frame[ctx->sp].N--;">body
		    print "\t    REG_PC += tmp;">body
		    print "\t} else {">body
		    print "\t    /*Check M count expired*/">body
		    print "\t    if (ctx->frame[ctx->sp].M > 0){">body
		    print "\t        /*Decr & do 1 iter FORKED*/">body
		    print "\t        ctx->frame[ctx->sp].M--;">body
		    print "\t        REG_STORE(ctx)">body
		    print "\t        FORKB(ctx,tmp);">body
		    print "\t        REG_SYNC(ctx)">body
		    print "\t    }">body
		    print "\t    /*FRAMEPOP & cont next instr*/">body
		    print "\t    ctx->sp--;">body
		    print "\t}">body
		}else if (stateaction[s] == "NEXT_W"){
		    print "\t/*Get arg*/">body
		    print "\ttmp = GETNUM(REG_PC);">body
		    print "\t/*Check N count expired*/">body
		    print "\tif (ctx->frame[ctx->sp].N > 0){">body
		    print "\t    /*Decr & do 1 iter*/">body
		    print "\t    ctx->frame[ctx->sp].N--;">body
		    print "\t    REG_PC += tmp;">body
		    print "\t} else {">body
		    print "\t    REG_STORE(ctx)">body
		    print "\t    FORKB(ctx,tmp);">body
		    print "\t    REG_SYNC(ctx)">body
		    print "\t    /*FRAMEPOP & cont next instr*/">body
		    print "\t    ctx->sp--;">body
		    print "\t}">body
		}else if (stateaction[s] == "PAT_TRACE"){
		    print "\tctx->parent->trace = ADU_pat_debug;">body
		}else if (stateaction[s] == "PAT_NOTRACE"){
		    print "\tctx->parent->trace = 0;">body
		}else{
		    print "\tact" stateaction[s] "(ctx);">body
		}
		if (statev[s]+1==statev[stateyes[s]]){
		    #print "\t/*NEXT*/">body
		}else{
		    print "\tgoto _" stateyes[s]";">body
		}
	    }else if(statetype[s]==TERM){
		print "\t" stateaction[s] ";">body
		print "\tbreak;">body
	    }else if(statetype[s]==SWITCH){
		print "if (REG_PC >= ctx->patbufend) goto _"stateno[s]";">body
		print "DISASS(ctx)">body
		print "ctx->state = *REG_PC++;">body
		print "_restart_from_stall:">body
		print "switch(ctx->state){">body
	    }else{
		print "ERROR in dumping">body
	    }
	}
	print "}">body
    }
    if (verbose > 0){
	OFS="\t"
	for(i = -5; i < st; i++){
	    s=staten[i]
	    if(s in stateopv)
		printf "*"
	    else
		printf " "
	    if(statetype[s]==TEST){
		if (statev[s]+1==statev[stateyes[s]])stateyes[s]="*NEXT*"
		printf "%-19s %3d %2d %2d %-8s %-19s %-19s %s\n", s,\
		i,jumpto[s]+0,swto[s]+0,"TEST",statetest[s],stateyes[s],stateno[s]
	    }else if(statetype[s]==FORK){
		#if (statev[s]+1==statev[stateparent[s]])stateparent[s]="*NEXT*"
		printf "%-19s %3d %2d %2d %-8s %-19s %-19s %s\n", s,\
		i,jumpto[s]+0,swto[s]+0,"FORK","-",stateparent[s],statechild[s]
	    }else if(statetype[s]==ACTION){
		ns=stateyes[s]
		c=" "
		if (jumpto[ns]==1) c="*"
		if (jumpto[ns]==2 && ns in ops) c="#"
		if (statev[s]+1==statev[stateyes[s]])stateyes[s]="*NEXT*"
		printf "%-19s %3d %2d %2d %-8s %-19s%1s%-19s %s\n", s,\
		i,jumpto[s]+0,swto[s]+0,"ACTION",stateaction[s],c,stateyes[s],"-"
	    }else if(statetype[s]==TERM){
		printf "%-19s %3d %2d %2d %-8s %-19s %-19s %s\n", s,\
		i,jumpto[s]+0,swto[s]+0,"TERM",stateaction[s],"-","-"
	    }else if(statetype[s]==SWITCH){
		printf "%-19s %3d %2d %2d %-8s %-19s %-19s %s\n", s,\
		i,jumpto[s]+0,swto[s]+0,"SWITCH","-",stateno[s],"-"
	    }else{
		print "ERROR in dumping"
	    }
	}
    }
}
