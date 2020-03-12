@echo off

IF NOT EXIST ..\bin mkdir ..\bin
pushd ../bin

REM -nologo -FC -Zi
set commonCompilerFlags=-Gm- -GR- -EHa- -Zo -Oi -Zi -Od
set commonCompilerFlags=-wd4577 %commonCompilerFlags%

cl %commonCompilerFlags% /DDEVELOPER_MODE=1 /DDESKTOP=1 /DNOMINMAX /Fe../bin/easy_engine.exe /I../engine /I ../SDL2 /I../libs /I../libs/gl3w /IE:/include ../src/main.cpp ../libs/gl3w/GL/gl3w.cpp /link ..\bin\SDL2.lib ..\bin\SDL2main.lib Winmm.lib opengl32.lib shlwapi.lib /SUBSYSTEM:WINDOWS ../src/myres.res

popd