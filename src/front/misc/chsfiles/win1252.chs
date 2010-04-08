#   Name: win1252.chs - Windows Code Page 1252
#
#   Description:
#
#       Character classifications for Windows Code Page 1252
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
#           Change 'l' property to 'a' for DF to correct previous change.
#
0-8	c		# Windows 1252 (Latin 1) code page
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
