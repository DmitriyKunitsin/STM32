rem Usage BuildInfo.bat
rem Pre-build command line from IAR IDE:
rem "PATH_TO_BUILDINFO_BAT\BuildInfo.bat" $CONFIG_NAME$ $PROJ_FNAME$ "PATH_TO_BUILDINFO_C"
rem "$PROJ_DIR$\..\..\..\Common\BuildInfo.bat" $CONFIG_NAME$ $PROJ_FNAME$ "$PROJ_DIR$\..\Src"

echo // BuildInfo.c																										> %3\BuildInfo.c
echo const char aBuildInfo_Date[]			= "%DATE%";			// "DD.MM.YYYY"								>> %3\BuildInfo.c
echo const char aBuildInfo_Time[]			= "%TIME%";		// "HH:MM:SS.ZZ" or "HH:MM:SS,ZZ"			>> %3\BuildInfo.c
echo const char aBuildInfo_ProjName[]		= "%2";			// IAR Project Name	>> %3\BuildInfo.c
echo const char aBuildInfo_ConfigName[]		= "%1";		// IAR Configuration - "Debug" and similar	>> %3\BuildInfo.c
echo const char aBuildInfo_ComputerName[]		= "%COMPUTERNAME%";			// Host Computer Name											>> %3\BuildInfo.c
echo const char aBuildInfo_SVN_Revision[]		= >> %3\BuildInfo.c
type %3\..\..\Revision.txt		>>%3\BuildInfo.c
echo ;			>> %3\BuildInfo.c
