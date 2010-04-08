#   Name: iso88592.chs - ISO Coded Character Set 8859/2
#
#   Description:
#
#       Character classifications for ISO Coded Character Set 8859/2
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
0-8	c	# iso88592.chs
9-D     cw      # CR's, LF's, FF's, etc.
20      pw      # space
21-22   po      # !"
23-24   pon     # #$
25-2F   po      # various printable operators
30-39   pndx     # digits
3A-3F   po      # various printable operators
40      pon     # "at" sign

41-46   psux    61      # A-F
47-5A   psu     67      # G-Z

5B-5E   po      # various printable operators
5F      ps      # underscore
60      po      # grave
61-66   pslx    # a-f
67-7A   psl     # g-z
7B-7E   po      # various printable operators

A0	cw      # NBSP
A1	psu	B1
A2	po	# printable char
A3	psu	B3
A4	po	# printable char
A5-A6	psu	B5
A7-A8	po	# printable chars
A9-AC	psu	B9
AD	po	# printable char
AE-AF	psu	BE
B0	po	# printable char
B1	psl
B2	po	# printable char
B3	psl
B4	po	# printable char
B5-B6	pls
B7-B8	po	# printable char
B9-BC	pls
BD	po	# printable char
BE-BF	pls

C0-D6	psu	E0
D7	po	# multiplication symbol
D8-DE	psu	F8
DF	posa	#  LATIN SMALL LETTER SHARP S (German)
E0-F6	pls
F7	po	# division symbol
F8-FE	pls
