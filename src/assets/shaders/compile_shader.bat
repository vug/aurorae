@echo off
where glslc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: glslc not found in PATH!
    echo Please install Vulkan SDK and ensure glslc is in your system PATH
    echo Typically found in: C:\VulkanSDK\<version>\Bin
    exit /b 1
)

if "%~1"=="" (
    echo Usage: compile_shader.bat shader_file [debug^|release]
    echo Examples:
    echo   compile_shader.bat triangle.vert
    echo   compile_shader.bat triangle.vert debug
    exit /b 1
)

set SHADER_FILE=%~1
set CONFIG=release

if not "%~2"=="" (
    if /i "%~2"=="debug" (
        set CONFIG=debug
    ) else if /i "%~2"=="release" (
        set CONFIG=release
    ) else (
        echo Error: Invalid config value '%~2'. Use 'debug' or 'release'
        exit /b 1
    )
)

if %CONFIG%==debug (
set OUTPUT_FILE=debug/%SHADER_FILE%.spv
) else (
set OUTPUT_FILE=release/%SHADER_FILE%.spv
)

echo Compiling %SHADER_FILE% to %OUTPUT_FILE% in %CONFIG% mode...
if %CONFIG%==debug (
    glslc -c -g --target-env=vulkan1.3 --target-spv=spv1.6 -Werror %SHADER_FILE% -o %OUTPUT_FILE%
) else (
    glslc -c -O --target-env=vulkan1.3 --target-spv=spv1.6 -Werror %SHADER_FILE% -o %OUTPUT_FILE%
)

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
    exit /b 1
)