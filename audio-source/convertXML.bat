@echo off
..\tools\xml2struct\xml2struct.exe config.xml > samv71-ucs\src\default_config.c
if %ERRORLEVEL% EQU 0 echo Conversion was successful
pause