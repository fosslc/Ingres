#   Name: iso88599.chs - ISO Coded Character Set 8859/9
#
#   Description:
#
#       Character classifications for ISO Coded Character Set 8859/9
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
#           Add 'a' property to FF (referred to as 'lower case y-diaresis')
#             Changed both comments to the Unicode Character Name
#           Added Character Set Classification Code table comments
#	21-May-2002 (hanje04)
#	    Change 'a' property to 's' for DF and FF to correct peeje01 change.
#       13-June-2002 (gupsh01)
#           Added 'l' property for DF, to retain the alphabet property.
#	13-June-2002 (gupsh01)
#           Added 'l' property for FF, to retain the alphabet property.
#       14-June-2002 (gupsh01)
#           Change 'l' property to 'a' for DF and FF to correct previous change.
#
0-8	c	# iso_8859_9
9-D     cw      # CRs, LFs, FFs, etc.
20      pw      # space
21-22   po      # !"
23-24   pon     # #$
25-2F   po      # various printable operators %&'()*+,-./
30-39   pndx    # digits
3A-3F   po      # various printable operators :;<=>?
40      pon     # "at" sign @

41-46   psux    61      # A-F
47-5A   psu     67      # G-Z

5B-5E   po      # various printable operators    
5F      ps      # underscore
60      po      # grave
61-66   pslx    # a-f
67-7A   psl     # g-z
7B-7E   po      # various printable operators
7F	cw	# delete

82-9F	cw	# control characters
A0	cw	# NBSP
A1-BF	po	# printable characters 
C0-D6	psu	E0 # upper case characters
D7	po	# multiplication symbol
D8-DE	psu	F8 # upper case characters
DF	posa	#  LATIN SMALL LETTER SHARP S (German)
E0-F6   psl	# lower case characters
F7	po	# division symbol
F8-FE	psl	# lower case characters
FF	posa	# LATIN SMALL LETTER Y WITH DIAERESIS
