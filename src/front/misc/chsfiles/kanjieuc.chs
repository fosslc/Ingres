#
#   Description:
#
#       Kanjieuc character set (CP932)
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
#
#       24-Oct-2006 (kiria01)
#           Change A1 to be marked as whitespace as well as 1&2.
#           This is to support doublebyte space changes in cmcl.h
#	16-Jun-2008 (gupsh01)
#	    Reinstate the classification 'a' for characters between
#	    A1 to FE range. Without it error is given when creating
#	    names with Japanese characters. (bug 120499)


0-8	c	# kanjieuc
9-D     cw      # CR's, LF's, FF's, etc.
E-1F    c
20      pw      # space
21-22   po      # !"
23-24   pon     # #$
25-2F   po      # various printable operators (%&'()*+,~./)
30-39   pndx    # digits (0-9)
3A-3F   po      # various printable operators (:;<=>?)
40      pon     # "at" sign

41-46   psux   61      # A-F
47-5A   psu    67      # G-Z

5B-5E   po     # various printable operators ([\]^)
5F      ps     # underscore
60      po     # grave
61-66   pslx   # a-f
67-7A   psl    # g-z
7B-7E   po     # various printable operators

7F      c       #

80      p      #
81	p	#

82-8D	ps    #
8E-8F	ps1a   #
90-9F	ps    #

A0      c      #
A1      p12wsa    #
A2-FE   p12sa    #


FF      c
