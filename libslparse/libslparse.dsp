# Microsoft Developer Studio Project File - Name="libslparse" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libslparse - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libslparse.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libslparse.mak" CFG="libslparse - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libslparse - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libslparse - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "libslparse"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libslparse - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Library\Release"
# PROP Intermediate_Dir "..\Object\Release\libslparse"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\libslparse" /I "..\libaqsistypes" /I "..\libaqsistypes\win32\intel" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Library\Debug"
# PROP Intermediate_Dir "..\Object\Debug\libslparse"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\libslparse" /I "..\libaqsistypes" /I "..\libaqsistypes\win32\intel" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libslparse - Win32 Release"
# Name "libslparse - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\funcdef.cpp
# End Source File
# Begin Source File

SOURCE=.\libslparse.cpp
# End Source File
# Begin Source File

SOURCE=.\optimise.cpp
# End Source File
# Begin Source File

SOURCE=.\parsenode.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.y

!IF  "$(CFG)" == "libslparse - Win32 Release"

# Begin Custom Build - Building Parser from $(InputPath)
IntDir=.\..\Object\Release\libslparse
InputPath=.\parser.y

BuildCmds= \
	bison --no-lines --defines -o$(IntDir)\parser.cpp $(InputPath)

"$(IntDir)\parser.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(IntDir)\parser.cpp.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

# Begin Custom Build - Building Parser from $(InputPath)
IntDir=.\..\Object\Debug\libslparse
InputPath=.\parser.y

BuildCmds= \
	bison --no-lines --defines -o$(IntDir)\parser.cpp $(InputPath)

"$(IntDir)\parser.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(IntDir)\parser.cpp.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\scanner.l

!IF  "$(CFG)" == "libslparse - Win32 Release"

# Begin Custom Build - Building Lexical Scanner from $(InputPath)
IntDir=.\..\Object\Release\libslparse
InputPath=.\scanner.l

"$(IntDir)\scanner.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -o$(IntDir)\scanner.cpp $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

# Begin Custom Build - Building Lexical Scanner from $(InputPath)
IntDir=.\..\Object\Debug\libslparse
InputPath=.\scanner.l

"$(IntDir)\scanner.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -o$(IntDir)\scanner.cpp $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\typecheck.cpp
# End Source File
# Begin Source File

SOURCE=.\vardef.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\funcdef.h
# End Source File
# Begin Source File

SOURCE=.\ifuncdef.h
# End Source File
# Begin Source File

SOURCE=.\iparsenode.h
# End Source File
# Begin Source File

SOURCE=.\ivardef.h
# End Source File
# Begin Source File

SOURCE=.\libslparse.h
# End Source File
# Begin Source File

SOURCE=.\parsenode.h
# End Source File
# Begin Source File

SOURCE=.\vardef.h
# End Source File
# End Group
# Begin Group "Generated Files"

# PROP Default_Filter ""
# Begin Group "Release"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Object\Release\libslparse\parser.cpp

!IF  "$(CFG)" == "libslparse - Win32 Release"

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Object\Release\libslparse\parser.cpp.h

!IF  "$(CFG)" == "libslparse - Win32 Release"

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Object\Release\libslparse\scanner.cpp

!IF  "$(CFG)" == "libslparse - Win32 Release"

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Debug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Object\Debug\libslparse\parser.cpp

!IF  "$(CFG)" == "libslparse - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Object\Debug\libslparse\parser.cpp.h

!IF  "$(CFG)" == "libslparse - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Object\Debug\libslparse\scanner.cpp

!IF  "$(CFG)" == "libslparse - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libslparse - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# End Group
# End Target
# End Project
