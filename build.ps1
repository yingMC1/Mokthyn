# Mokthyn 一键构建脚本（Release / Debug）
# 用法：
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Config Release   # 构建 Release
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Config Debug     # 构建 Debug
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Clean            # 清理两个构建目录
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Config Release -Configure  # 强制重新配置
# 说明：缓存已存在时直接复用生成器（MinGW Makefiles），不做 -G 重新配置，
# 避免 `-G "MinGW Makefiles"` 在 cmd / VSCode 任务拼接时引号被剥离导致 "Could not create named generator MinGW"。
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Config = 'Release',
    [switch]$Clean,
    [switch]$Configure
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$cmake = "$env:ProgramFiles\CMake\bin\cmake.exe"
$make = 'D:/mingw64/bin/mingw32-make.exe'
$compiler = 'D:/mingw64/bin/x86_64-w64-mingw32-g++.exe'
$buildDir = Join-Path $root ("build/cmake-" + $Config.ToLower())
$cacheFile = Join-Path $buildDir 'CMakeCache.txt'

if ($Clean) {
    foreach ($c in 'cmake-release', 'cmake-debug') {
        $dir = Join-Path $root "build/$c"
        if (Test-Path -LiteralPath (Join-Path $dir 'CMakeCache.txt')) {
            & $cmake --build $dir --target clean
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
    Write-Host "已清理构建产物"
    exit 0
}

if (-not (Test-Path -LiteralPath $cacheFile) -or $Configure) {
    # 首次配置：参数用数组传递，PowerShell 自动处理含空格生成器名，无引号问题
    & $cmake -S $root -B $buildDir -G 'MinGW Makefiles' -DCMAKE_MAKE_PROGRAM=$make `
        -DCMAKE_BUILD_TYPE=$Config -DCMAKE_CXX_COMPILER=$compiler
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
& $cmake --build $buildDir --config $Config --parallel 1
exit $LASTEXITCODE
