@echo off
set /p target=enter target directory:
md %target%
copy *.h %target%\*.h
md %target%\Engine
copy Engine\*.h %target%\Engine\*.h
pause