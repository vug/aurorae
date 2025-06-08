@echo off
where glslc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: glslc not found in PATH!
    echo Please install Vulkan SDK and ensure glslc is in your system PATH
    echo Typically found in: C:\VulkanSDK\<version>\Bin
    exit /b 1
)

if [%1]==[] (
    echo Usage: compile_shader.bat shader_file
    echo Example: compile_shader.bat triangle.vert
    exit /b 1
)

set SHADER_FILE=%1
set OUTPUT_FILE=%SHADER_FILE%.spv

echo Compiling %SHADER_FILE% to %OUTPUT_FILE%...
REM Enable optimizations -O: Enable optimizations, -g: Include debug info
glslc -O -g --target-env=vulkan1.3 --target-spv=spv1.6 -Werror %SHADER_FILE% -o %OUTPUT_FILE%

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
    exit /b 1
)