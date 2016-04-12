@echo off

REM 
REM drbd rc batch file
REM 

IF "%1" == "start" GOTO start
IF "%1" == "stop"  GOTO stop

@echo on
echo "Usage: <Batchfile> [start|stop] {vhd path for meta}"
goto :eof

REM ------------------------------------------------------------------------
:start

:attach_vhd
for /f "usebackq tokens=*" %%a in (`drbdadm sh-md-idx all`) do (call :sub_attach_vhd %%a)

:start
::echo WDRBD Starting ...

drbdadm -c /etc/drbd.conf adjust all
if %errorlevel% gtr 0 (
	echo Failed to drbdadm adjust
	goto end
)

goto :eof


REM ------------------------------------------------------------------------

:stop

@echo off
echo Stopping all DRBD resources


for /f "usebackq tokens=*" %%a in (`drbdadm sh-resource all`) do (
	drbdadm sh-dev %%a > tmp_vol.txt
REM MVL
REM for /f "usebackq tokens=*"  %%b in (tmp_vol.txt) do ..\mvl\vollock /l %%b:
	del tmp_vol.txt

	drbdsetup %%a  down
)

goto :eof

:sub_attach_vhd
	if exist %1 (echo select vdisk file="%~f1"& echo attach vdisk) > _temp_attach
	if exist _temp_attach (diskpart /s _temp_attach  > nul & del _temp_attach & echo %1 is mounted.)
	goto :eof