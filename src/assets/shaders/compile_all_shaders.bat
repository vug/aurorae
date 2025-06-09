@echo off
echo Compiling all shaders in debug and release modes...

REM Create debug and release directories if they don't exist
if not exist debug mkdir debug
if not exist release mkdir release

REM Compile all vertex shaders
for %%f in (*.vert) do (
    echo.
    echo Processing %%f...
    call compile_shader.bat %%f debug
    if %ERRORLEVEL% NEQ 0 goto :error
    call compile_shader.bat %%f release
    if %ERRORLEVEL% NEQ 0 goto :error
)

REM Compile all fragment shaders
for %%f in (*.frag) do (
    echo.
    echo Processing %%f...
    call compile_shader.bat %%f debug
    if %ERRORLEVEL% NEQ 0 goto :error
    call compile_shader.bat %%f release
    if %ERRORLEVEL% NEQ 0 goto :error
)

echo.
echo All shaders compiled successfully!
exit /b 0

:error
echo.
echo Error: Shader compilation failed!
exit /b 1