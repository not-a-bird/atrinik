@echo off

echo Updating Atrinik installation, please wait...

rem Wait a few seconds to make sure upgrader.exe has finished.
timeout /NOBREAK 2

rem Store the current working directory.
set old_dir=%CD%
rem Go to the patches directory.
cd "%AppData%"\.atrinik\temp

rem Extract all patches.
for %%f in (*.tar.gz) do (
	echo Extracting %%f
	"%old_dir%"\gunzip.exe -c %%f > %%~nf
	"%old_dir%"\tar.exe xvf %%~nf
	del /q %%f
	del /q %%~nf
)

rem Go back to the old directory.
cd "%old_dir%"
rem Copy over the extracted files.
xcopy /s/e/y "%AppData%"\.atrinik\temp\*.* .\
rem Remove the temporary directory.
rmdir /s/q "%AppData%"\.atrinik\temp

rem Start up the client.
start atrinik.exe %*
exit
