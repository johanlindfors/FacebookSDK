@echo off
setlocal

REM Add TextTransform to our path based on the default location for VS2017 Enterprise Edition
set _TT=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\
set PATH=%_TT%;%PATH%;

REM Verify that we can find TextTransform
where /q TextTransform.exe
if "%ErrorLevel%" == "0" goto :Found

REM Add TextTransform to our path based on the default location for VS2017
set _TT=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\IDE\
set PATH=%_TT%;%PATH%;

REM Verify that we can find TextTransform
where /q TextTransform.exe
if not "%ErrorLevel%" == "0" goto :NotFound

:Found
if not exist Generated md Generated
REM cpp files
call :GenFile Graph.FBCursors.cpp Graph.FBCursors.cpp.tt
call :GenFile Graph.FBGroup.cpp Graph.FBGroup.cpp.tt
call :GenFile Graph.FBPage.cpp Graph.FBPage.cpp.tt
REM call :GenFile FBPageCategory.cpp FBPageCategory.cpp.tt
REM call :GenFile FBPaging.cpp FBPaging.cpp.tt
REM call :GenFile FBPermission.cpp FBPermission.cpp.tt
call :GenFile Graph.FBUser.cpp Graph.FBUser.cpp.tt
call :GenFile Graph.FBProfilePicture.cpp Graph.FBProfilePicture.cpp.tt
call :GenFile Graph.FBProfilePictureData.cpp Graph.FBProfilePictureData.cpp.tt
call :GenFile Graph.FBAppRequest.cpp Graph.FBAppRequest.cpp.tt
REM call :GenFile FBObject.cpp FBObject.cpp.tt

REM header files
call :GenFile Graph.FBCursors.h Graph.FBCursors.h.tt
call :GenFile Graph.FBGroup.h Graph.FBGroup.h.tt
call :GenFile Graph.FBPage.h Graph.FBPage.h.tt
REM call :GenFile FBPageCategory.h FBPageCategory.h.tt
REM call :GenFile FBPaging.h FBPaging.h.tt
REM call :GenFile FBPermission.h FBPermission.h.tt
call :GenFile Graph.FBUser.h Graph.FBUser.h.tt
call :GenFile Graph.FBProfilePicture.h Graph.FBProfilePicture.h.tt
call :GenFile Graph.FBProfilePictureData.h Graph.FBProfilePictureData.h.tt
call :GenFile Graph.FBAppRequest.h Graph.FBAppRequest.h.tt
REM call :GenFile FBObject.h FBObject.h.tt

REM definition files
call :GenFile Graph.FBPage.idl Graph.FBPage.idl.tt
call :GenFile Graph.FBAppRequest.idl Graph.FBAppRequest.idl.tt
call :GenFile Graph.FBUser.idl Graph.FBUser.idl.tt
call :GenFile Graph.FBProfilePicture.idl Graph.FBProfilePicture.idl.tt
call :GenFile Graph.FBProfilePictureData.idl Graph.FBProfilePictureData.idl.tt
call :GenFile Graph.FBCursors.idl Graph.FBCursors.idl.tt
call :GenFile Graph.FBGroup.idl Graph.FBGroup.idl.tt

goto End

:NotFound
echo Could not find TextTransform, please add it to your PATH or have it available in %CommonProgramFiles(x86)%\Microsoft Shared\TextTemplating\14.0 (VS2015) or %CommonProgramFiles(x86)%\Microsoft Shared\TextTemplating\12.0 (VS2013)

:End
exit /b 0

REM call :GenFile GeneratedFileName TemplateFileName
:GenFile
setlocal
set CurrentGeneratedFile=%1
set CurrentTemplateFile=%2
REM check the generated file and its .tt equivalent
if not exist Generated\%CurrentGeneratedFile% goto :RunGenFile
call :ShouldRegen %CurrentGeneratedFile% %CurrentTemplateFile%
if %errorlevel% EQU 1 goto :RunGenFile
REM check the .xml file
for %%i in (%CurrentGeneratedFile%) do set BaseName=%%~ni
call :ShouldRegen %CurrentGeneratedFile% %BaseName%.xml
if %errorlevel% EQU 1 goto :RunGenFile
REM check if we need to also check a .ttinclude file
set Ext=""
set TTIncludeFile="x"
for %%i in (%CurrentGeneratedFile%) do set Ext=%%~xi
if "%Ext%" EQU ".cpp" set TTIncludeFile=FBGraphObjectImplementation.ttinclude
if "%Ext%" EQU ".h" set TTIncludeFile=FBGraphObjectHeader.ttinclude
if "%Ext%" EQU ".idl" set TTIncludeFile=FBGraphObjectDefinition.ttinclude
if "%TTIncludeFile%" EQU "x" exit /b 0
call :ShouldRegen %CurrentGeneratedFile% %TTIncludeFile%
if %errorlevel% EQU 1 goto :RunGenFile
exit /b 0

:RunGenFile
echo Generating Generated\%CurrentGeneratedFile%
TextTransform.exe -out Generated\%CurrentGeneratedFile% %CurrentTemplateFile%
exit /b 0


REM call :ShouldRegen GeneratedFileName TemplateFileName
REM returns 1 if should regen, 0 otherwise
:ShouldRegen
setlocal
set CurrentGeneratedFile=%1
set CurrentTemplateFile=%2
set CompareTool="..\..\build\CompareFileDates.exe"
%CompareTool% "%CD%\Generated\%CurrentGeneratedFile%" "%CD%\%CurrentTemplateFile%"
if %errorlevel% EQU 1 exit /b 1 else exit /b 0
