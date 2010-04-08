#   Name: inst1252.chs - Windows install Code Page 1252
#
#   Description:
#
#       Character classifications for Windows install Code Page 1252
#       Character Set Classification Codes:
#        p - printable
#        c - control
#        o - operator
#        x - hex digit
#        d - digit
#        u - upper-case alpha
#        l - lower-case alpha
#        a - non-cased alpha/path control
#        w - whitespace
#        s - name start
#        n - name
#        1 - first byte of double-byte character
#        2 - second byte of double-byte character
#	 H - hostname character
#	 P - path character
#	 U - username character
#
#   History:
#       25-Jun-2002 (fanra01)
#           Sir 108122
#           Created based on WIN1252.
#           s and n are used in combination with HPU.
#           a is used to define path operators that can be in a path component.
#	01-Dec-2004 (wansh01) 
#	    Bug 113521
#	    Allowed hostname start with digit.	
#	    Added s attribute for entry 30-39. 
#   	14-Feb-2005 (fanra01)
#           Sir 113890
#           Remove accented characters as valid path characters.
#           Allow square brackets as path characters.
#	31-May-2005 (drivi01)
#	    Modified character support for user names, hostnames,
#	    and directory names.
#	22-Jun-2005 (drivi01)
#	    Removed 's' for definition for digits. Validate
#	    first character using CMdigit function instead.
#	26-Sep-2006 (wanfr01)
#	    Sir 116722
#	    Allow Underscores to be allowed for hostnames.
#
0-8	c		# Windows 1252 (Latin 1) code page
9-D	cw		# HT, LF, VT, FF, CR
20	pwP		# space
21-22	po		# !"
23-24	ponU		# #$
25-2C	po		# %&'()*+,-
2D	poanHP		# -
2E	poaP		# .
2F	po		# /
30-39	pdxnHPU	# 0-9
3A	poa		# :
3B-3F	po		# ;<=>?
40	ponU		# @
41-46	pxusHPU	61	# A-F
47-5A	pusHPU	67	# G-Z
5B	poaP		# [
5C	poP		# \
5D	poaP		# ]
5E	po		# ^
5F	psHPU		# _
60	po		# `
61-66	pxlsHPU		# a-f
67-7A	plsHPU		# g-z
7B-7D	po		# {|}
7E	poaP		# ~

7F	c		# DEL
80	po		# Euro sign

82-89	po		# various printable operators
8A	psu	9A	# S w/caron
8B	po		# single left-pointing angle quote
8C	psu	9C	# ligature OE

8E	psu	9E	# Z w/caron

91-99	po		# various printable operators
9A	psl		# s w/caron
9B	po		# single right-pointing angle quote
9C	psl		# ligature oe

9E	psl		# z w/caron
9F	psu	FF	# Y w/diaeresis
A0	cw		# no-break space
A1-BF	po		# various signs and operators
C0-D6	psu	E0	# various accented letters
D7	po		# multiplication sign
D8-DE	psu	F8	# various accented letters
DF	posa		# LATIN SMALL LETTER SHARP S (German)
E0-F6	psl		# various accented letters
F7	po		# division sign
F8-FE	psl		# various accented letters
FF	psl		# y w/diaeresis
