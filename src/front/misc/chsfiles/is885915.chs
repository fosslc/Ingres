#   Name: is885915.chs - ISO Coded Character Set 8859/15
#
#   Description:
#
#       Character classifications for ISO Coded Character Set 8859/15
#       Character Set Classification Codes:
#        p - printable
#        c - control
#        o - operator
#        x - hex digit
#        d - digit
#        u - upper-case alpha
#        l - lower-case alpha
#        a - non-cased alpha
#        w - whitespace
#        s - name start
#        n - name
#        1 - first byte of double-byte character
#        2 - second byte of double-byte character
#
#   History:
#       15-may-2002 (peeje01)
#           Add 'a' property to DF (referred to as 'Beta')
#           Changed comment to the Unicode Character Name
#           Added Character Set Classification Code table comments
#	21-May-2002 (hanje04)
#	    Change 'a' property to 's' for DF to correct peeje01 change.
#       13-June-2002 (gupsh01)
#           Added 'l' property for DF, to retain the alphabet property.
#       14-June-2002 (gupsh01)
#           Modified 'l' property for DF to 'a', to correct previous change.
#
0-8	c		# ISO 8859-15 (Latin 9)
9-D	cw		# HT, LF, VT, FF, CR
20	pw		# space
21-22	po		# !"
23-24	pon		# #$
25-2F	po		# %&'()*+,-./
30-39	pdxn		# 0-9
3A-3F	po		# :;<=>?
40	pon		# @
41-46	pxus	61	# A-F
47-5A	pus	67	# G-Z
5B-5E	po		# [\]^
5F	ps		# _
60	po		# `
61-66	pxls		# a-f
67-7A	pls		# g-z
7B-7E	po		# {|}~
7F	c		# DEL

A0	cw		# no-break space
A1-A5	po		# inverted exclamation and currency signs
A6	psu	A8	# S w/caron
A7	po		# section sign
A8	psl		# s w/caron
A9-B3	po		# various signs and operators
B4	psu	B8	# Z w/caron
B5-B7	po		# various signs and operators
B8	psl		# z w/caron
B9-BB	po		# various signs and operators
BC	psu	BD	# ligature OE
BD	psl		# ligature oe
BE	psu	FF	# Y w/diaeresis
BF	po		# inverted question mark
C0-D6	psu	E0	# various accented letters
D7	po		# multiplication sign
D8-DE	psu	F8	# various accented letters
DF	posa		# LATIN SMALL LETTER SHARP S (German)
E0-F6	psl		# various accented letters
F7	po		# division sign
F8-FE	psl		# various accented letters
FF	psl		# y w/diaeresis
