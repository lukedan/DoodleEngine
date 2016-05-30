@echo off
rd headers
md headers
copy *.h headers\*.h
md headers\Engine
copy Engine\*.h headers\Engine\*.h
pause