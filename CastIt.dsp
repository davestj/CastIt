# Microsoft Developer Studio Project File - Name="CastIt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=CastIt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CastIt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CastIt.mak" CFG="CastIt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CastIt - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CastIt - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CastIt - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "libxml2-2.4.9/include" /I "mysql/include" /I "ResizableLib" /I "curl/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 curl/lib/curlstatic.lib libxml2-2.4.9/win32/libxml2/libxml2_a/libxml2.lib mysql/lib/opt/mysqlclient.lib ResizableLib/Release/ResizableLib.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "CastIt - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "libxml2-2.4.9/include" /I "mysql/include" /I "ResizableLib" /I "curl/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 curl/lib/curlstatic.lib libxml2-2.4.9/win32/libxml2/libxml2_a/libxml2.lib mysql/lib/opt/mysqlclient.lib ResizableLib/Debug/ResizableLib.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CastIt - Win32 Release"
# Name "CastIt - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AddServer.cpp
# End Source File
# Begin Source File

SOURCE=.\BCMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\BtnST.cpp
# End Source File
# Begin Source File

SOURCE=.\CastIt.cpp
# End Source File
# Begin Source File

SOURCE=.\CastIt.rc
# End Source File
# Begin Source File

SOURCE=.\CastItDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigurationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\DB.cpp
# End Source File
# Begin Source File

SOURCE=.\FTPInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\HistoryEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\HyperLink.cpp
# End Source File
# Begin Source File

SOURCE=.\log.cpp
# End Source File
# Begin Source File

SOURCE=.\scastd.cpp
# End Source File
# Begin Source File

SOURCE=.\socket.cpp
# End Source File
# Begin Source File

SOURCE=.\STabCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SystemTray.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskbarNotifier.cpp
# End Source File
# Begin Source File

SOURCE=.\WinXPButtonST.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AddServer.h
# End Source File
# Begin Source File

SOURCE=.\BCMenu.h
# End Source File
# Begin Source File

SOURCE=.\BtnST.h
# End Source File
# Begin Source File

SOURCE=.\CastIt.h
# End Source File
# Begin Source File

SOURCE=.\CastItDlg.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\ConfigurationDialog.h
# End Source File
# Begin Source File

SOURCE=.\DB.h
# End Source File
# Begin Source File

SOURCE=.\FTPInfo.h
# End Source File
# Begin Source File

SOURCE=.\HistoryEdit.h
# End Source File
# Begin Source File

SOURCE=.\HyperLink.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\STabCtrl.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SystemTray.h
# End Source File
# Begin Source File

SOURCE=.\TaskbarNotifier.h
# End Source File
# Begin Source File

SOURCE=.\urlopen.h
# End Source File
# Begin Source File

SOURCE=.\WinXPButtonST.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\NEWICONS\add_server.ico
# End Source File
# Begin Source File

SOURCE=.\res\add_server1.ico
# End Source File
# Begin Source File

SOURCE=.\res\add_server2.ico
# End Source File
# Begin Source File

SOURCE=.\res\allcastlogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\apache88x31.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap13.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap14.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap15.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap17.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CastIt.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\castit.ico
# End Source File
# Begin Source File

SOURCE=.\res\CastIt.rc2
# End Source File
# Begin Source File

SOURCE=.\res\castit88x31logo.bmp
# End Source File
# Begin Source File

SOURCE=".\res\castitlogo-newbmp.bmp"
# End Source File
# Begin Source File

SOURCE=.\res\castitlogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\checkboxes.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Classes1_32x32x16.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\configure.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\delete_log.ico
# End Source File
# Begin Source File

SOURCE=.\res\edit.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\exit.ico
# End Source File
# Begin Source File

SOURCE=.\res\geniusmedialogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Hand.cur
# End Source File
# Begin Source File

SOURCE=.\res\help.bmp
# End Source File
# Begin Source File

SOURCE=.\res\help.ico
# End Source File
# Begin Source File

SOURCE=.\res\help_16x16.ico
# End Source File
# Begin Source File

SOURCE=.\res\home.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\home.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\icecast2.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icecast2_1.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icecast2_32x32.ico
# End Source File
# Begin Source File

SOURCE=.\res\icelogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\res\logo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\media_blbg.bmp
# End Source File
# Begin Source File

SOURCE=.\res\media_blbg.ico
# End Source File
# Begin Source File

SOURCE=.\res\media_blk_bg.bmp
# End Source File
# Begin Source File

SOURCE=.\res\media_blk_bg.ico
# End Source File
# Begin Source File

SOURCE=.\res\mysql.bmp
# End Source File
# Begin Source File

SOURCE=.\res\network.ico
# End Source File
# Begin Source File

SOURCE=.\res\No3_32x32x256.ico
# End Source File
# Begin Source File

SOURCE=.\res\oddsocklogo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\php4.bmp
# End Source File
# Begin Source File

SOURCE=.\res\rrdtool.bmp
# End Source File
# Begin Source File

SOURCE=.\res\satalite.ico
# End Source File
# Begin Source File

SOURCE=".\res\sc-thumb.bmp"
# End Source File
# Begin Source File

SOURCE=.\res\sc_serv_1.ico
# End Source File
# Begin Source File

SOURCE=.\res\sc_serv_32X32.ico
# End Source File
# Begin Source File

SOURCE=.\res\server.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\shoutcast.ico
# End Source File
# Begin Source File

SOURCE=.\res\skinmsn.bmp
# End Source File
# Begin Source File

SOURCE=".\res\small-grey-ogg.bmp"
# End Source File
# Begin Source File

SOURCE=.\res\start_castit.ico
# End Source File
# Begin Source File

SOURCE=.\res\statlite1.ico
# End Source File
# Begin Source File

SOURCE=.\res\stop.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\systray.ico
# End Source File
# Begin Source File

SOURCE=.\res\systray.ico
# End Source File
# Begin Source File

SOURCE=.\res\systray2.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\timer.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Tools4.ico
# End Source File
# Begin Source File

SOURCE=.\res\NEWICONS\update.ico
# End Source File
# Begin Source File

SOURCE=.\res\winmysqladmin_2.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
