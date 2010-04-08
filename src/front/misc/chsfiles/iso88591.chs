#   Name: iso88591.chs - ISO Coded Character Set 8859/1
#
#   Description:
#
#       Standard US/English character set
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
#	13-June-2002 (gupsh01)
#	    Added 'l' property for DF, to retain the alphabet property.
#       14-June-2002 (gupsh01)
#           Change 'l' property to 'a' for DF to correct previous change.
#	31-Jul-2002 (hanch04)
#	    Chaned 'po'  property to 'psa' for FF for y-umlaut
#	    Note:  FF could also be EOF and cause problems when casting
#	    a char to int.
#
0-8	c	# iso_8859_1
9-D     cw      # CR's, LF's, FF's, etc.
20      pw      # space
21-22   po      # !"
23-24   pon     # #$
25-2F   po      # various printable operators
30-39   pndx    # digits
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

A0	cw	# NBSP
A1-BF	po	# printable chars

C0-D6	psu	E0
D7	po	# multiplication symbol
D8-DE	psu	F8
DF	posa	# LATIN SMALL LETTER SHARP S (German)
E0-F6	psl
F7	po	# division symbol
F8-FE	psl
FF	psa	# y-umlaut


