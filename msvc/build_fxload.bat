@echo off
setlocal

SET PATH=%PATH%;c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcpackages
SET BUILDENV=C:\WINDOWS\Microsoft.NET\Framework\v3.5

echo fxload build started for Release Win32 config...
mkdir Release 2>NUL

%BUILDENV%\MSBuild.exe fxload.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 > Release/Release_win32.log

if %ERRORLEVEL%==0 goto success 
if not %ERRORLEVEL%==0 goto fail 

:fail
echo Build failed for detail check  Release_win32.log config. check logs
goto done

:success
echo Successs

:done
endlocal
pause
