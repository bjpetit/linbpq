	PUBLIC	ENDOFCODE,ENDOFDATA

CODE	SEGMENT	PUBLIC	'CODE'

ENDOFCODE	LABEL	BYTE

CODE	ENDS

DATA	SEGMENT	PUBLIC	'DATA'

	PUBLIC	FREEDATA,ENDBUFFS

FREEDATA        DB      59500 DUP (0)

ENDBUFFS LABEL   BYTE

ENDOFDATA	LABEL	BYTE

DATA	ENDS

	END
