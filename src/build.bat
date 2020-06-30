@echo off


IF NOT EXIST ..\bin mkdir ..\bin
pushd ..\bin

REM compile the icon into a .res file
REM uncomment line below to compile the file
REM RC ../src/myres.rc


set commonCompilerFlags=-nologo -FC -Zi -Gm- -GR- -EHa- -Zo -Oi -Zi -Od
set commonCompilerFlags=-wd4577 %commonCompilerFlags%

set commonLinkLibs=..\bin\SDL2.lib ..\bin\SDL2main.lib Winmm.lib opengl32.lib shlwapi.lib Shell32.lib Kernel32.lib Ole32.lib

cl %commonCompilerFlags% /DDEVELOPER_MODE=1 /DDESKTOP=1 /DNOMINMAX /Fe../bin/easy_engine.exe /I../engine /I ../SDL2 /I../libs /I../libs/gl3w /IE:/include ../src/main.cpp ../libs/gl3w/GL/gl3w.cpp /link %commonLinkLibs% /SUBSYSTEM:WINDOWS ../src/myres.res

popd