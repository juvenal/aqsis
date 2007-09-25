SET(AQSIS_SVNVERSION_SEARCHPATH)

FIND_PROGRAM(AQSIS_SVNVERSION_EXECUTABLE
	svnversion
	PATHS ${AQSIS_XSLTPROC_SEARCHPATH}
	DOC "Location of the svnversion executable"
)

STRING(COMPARE NOTEQUAL ${AQSIS_SVNVERSION_EXECUTABLE} "AQSIS_SVNVERSION_EXECUTABLE-NOTFOUND" AQSIS_SVNVERSION_EXECUTABLE_FOUND)
