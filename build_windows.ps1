param(
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

# Auto-locate Visual Studio 2022 installation
$vsBase = "C:\Program Files\Microsoft Visual Studio\2022"
$edition = Get-ChildItem $vsBase -Directory | Select-Object -First 1 -ExpandProperty FullName
if (-not $edition) {
    throw "Visual Studio 2022 not found under $vsBase"
}

$cmakeExe = Join-Path $edition "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ninjaExe = Join-Path $edition "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
$msvcDir  = Get-ChildItem (Join-Path $edition "VC\Tools\MSVC") -Directory | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty FullName

$kitsRoot = "C:\Program Files (x86)\Windows Kits\10"
$sdkVer   = Get-ChildItem (Join-Path $kitsRoot "Include") -Directory | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name

$msvcBin = Join-Path $msvcDir "bin\Hostx64\x64"
$sdkBin  = Join-Path $kitsRoot "bin\$sdkVer\x64"

$env:PATH = "$msvcBin;$sdkBin;$(Split-Path $cmakeExe);$(Split-Path $ninjaExe);$env:PATH"
$env:INCLUDE = @(
    (Join-Path $msvcDir "include"),
    (Join-Path $kitsRoot "Include\$sdkVer\ucrt"),
    (Join-Path $kitsRoot "Include\$sdkVer\shared"),
    (Join-Path $kitsRoot "Include\$sdkVer\um"),
    (Join-Path $kitsRoot "Include\$sdkVer\winrt"),
    (Join-Path $kitsRoot "Include\$sdkVer\cppwinrt")
) -join ";"

$env:LIB = @(
    (Join-Path $msvcDir "lib\x64"),
    (Join-Path $kitsRoot "Lib\$sdkVer\ucrt\x64"),
    (Join-Path $kitsRoot "Lib\$sdkVer\um\x64")
) -join ";"

$env:CC  = Join-Path $msvcBin "cl.exe"
$env:CXX = Join-Path $msvcBin "cl.exe"
$env:RC  = Join-Path $sdkBin  "rc.exe"
$env:MT  = Join-Path $sdkBin  "mt.exe"
$env:_CL_ = "/Z7"

$localTmp = Join-Path $PSScriptRoot "build\tmp"
New-Item -ItemType Directory -Path $localTmp -Force | Out-Null
$env:TEMP = $localTmp
$env:TMP  = $localTmp

if (Test-Path "external\JUCE\CMakeLists.txt") {
    Write-Host "==> Pre-configuring JUCE helper tools (juceaide)..." -ForegroundColor Cyan
    & $cmakeExe -S external\JUCE -B build\external\JUCE\tools -G Ninja `
        "-DCMAKE_BUILD_TYPE=Debug" `
        "-DJUCE_BUILD_HELPER_TOOLS=ON" `
        "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
        "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW" `
        "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "==> Configuring Md-101 ($BuildType) by LU5 (https://lu5.art.br) with Ninja + MSVC..." -ForegroundColor Cyan
& $cmakeExe -B build -G Ninja `
    "-DCMAKE_BUILD_TYPE=$BuildType" `
    "-DCMAKE_MAKE_PROGRAM=$ninjaExe" `
    "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW" `
    "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded"

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Building Md-101 VST3, Standalone, and Verification Suite..." -ForegroundColor Cyan
& $cmakeExe --build build --parallel
for ($retry = 1; $retry -le 3 -and $LASTEXITCODE -ne 0; $retry++) {
    Write-Host "==> Retrying final link steps sequentially (-j 1, attempt $retry/3)..." -ForegroundColor Yellow
    Start-Sleep -Seconds 2
    & $cmakeExe --build build -j 1
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Build succeeded! (Md-101 by LU5 - https://lu5.art.br)" -ForegroundColor Green
Write-Host "    VST3 Plugin : build\Md101_artefacts\$BuildType\VST3\Md-101.vst3"
Write-Host "    Standalone  : build\Md101_artefacts\$BuildType\Standalone\Md-101.exe"
