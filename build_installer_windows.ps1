<#
.SYNOPSIS
    Md-101 — Master Windows Installer & Package Builder
    Builds:
      1. Standalone GUI/CLI Setup Wizard: dist\Md-101-<version>-Windows-x64-Setup.exe
      2. Portable Archive with Installers: dist\Md-101-<version>-Windows-x64.zip
    Developed by LU5 (https://lu5.art.br)
#>
param(
    [string]$BuildType = "Release",
    [string]$Version   = ""
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $Version) {
    $Version = if ($env:GITHUB_REF_NAME) { $env:GITHUB_REF_NAME } else { "v1.1.2" }
}

$Vst3Bundle    = Join-Path $ProjectRoot "build\Md101_artefacts\$BuildType\VST3\Md-101.vst3"
$StandaloneExe = Join-Path $ProjectRoot "build\Md101_artefacts\$BuildType\Standalone\Md-101.exe"

if (-not (Test-Path $Vst3Bundle) -or -not (Test-Path $StandaloneExe)) {
    Write-Host "==> Release binaries not found in build/. Building Md-101 ($BuildType) first..." -ForegroundColor Cyan
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot "build_windows.ps1") -BuildType $BuildType
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# 1. Ensure Windows .ico icon exists (generate high-DPI 256x256 PNG-backed .ico via System.Drawing)
$IconPath = Join-Path $ProjectRoot "installer\windows\md-101.ico"
if (-not (Test-Path $IconPath)) {
    Write-Host "==> Generating Md-101 Windows application icon ($IconPath)..." -ForegroundColor Cyan
    Add-Type -AssemblyName System.Drawing
    $bmp = New-Object System.Drawing.Bitmap(256, 256)
    $g   = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::FromArgb(13, 17, 23))

    $rect = New-Object System.Drawing.Rectangle(12, 12, 232, 232)
    $bgBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $rect,
        [System.Drawing.Color]::FromArgb(22, 28, 40),
        [System.Drawing.Color]::FromArgb(10, 14, 20),
        45.0
    )
    $g.FillRectangle($bgBrush, $rect)

    $borderPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 173, 215), 8.0)
    $g.DrawRectangle($borderPen, $rect)

    $innerPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(80, 114, 203, 235), 2.0)
    $g.DrawRectangle($innerPen, 24, 24, 208, 208)

    $fontNum = New-Object System.Drawing.Font("Segoe UI", 24, [System.Drawing.FontStyle]::Bold)
    $brushNum = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(114, 203, 235))
    $g.DrawString("101", $fontNum, $brushNum, 30, 26)

    $brushLed = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(163, 210, 70))
    $g.FillEllipse($brushLed, 198, 36, 18, 18)

    $fontSym = New-Object System.Drawing.Font("Segoe UI", 72, [System.Drawing.FontStyle]::Bold)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
    $g.DrawString("Md", $fontSym, [System.Drawing.Brushes]::White, (New-Object System.Drawing.RectangleF(0, 28, 256, 180)), $sf)

    $fontSub = New-Object System.Drawing.Font("Segoe UI", 16, [System.Drawing.FontStyle]::Bold)
    $brushSub = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(220, 173, 215))
    $g.DrawString("SYNTH [258]", $fontSub, $brushSub, (New-Object System.Drawing.RectangleF(0, 192, 256, 36)), $sf)

    $g.Dispose()

    $pngStream = New-Object System.IO.MemoryStream
    $bmp.Save($pngStream, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngBytes = $pngStream.ToArray()
    $pngStream.Dispose()
    $bmp.Dispose()

    # Write standard ICO header + 256x256 PNG frame
    $icoStream = New-Object System.IO.FileStream($IconPath, [System.IO.FileMode]::Create)
    $bw = New-Object System.IO.BinaryWriter($icoStream)
    $bw.Write([uint16]0)                  # Reserved
    $bw.Write([uint16]1)                  # Type = 1 (ICO)
    $bw.Write([uint16]1)                  # Image count = 1
    $bw.Write([byte]0)                    # Width (0 = 256)
    $bw.Write([byte]0)                    # Height (0 = 256)
    $bw.Write([byte]0)                    # Color count
    $bw.Write([byte]0)                    # Reserved
    $bw.Write([uint16]1)                  # Color planes
    $bw.Write([uint16]32)                 # Bits per pixel
    $bw.Write([uint32]$pngBytes.Length)   # Image data size
    $bw.Write([uint32]22)                 # Offset to PNG data
    $bw.Write($pngBytes)
    $bw.Dispose()
    $icoStream.Dispose()
}

# 2. Locate built-in Windows C# Compiler (csc.exe)
$cscCandidates = @(
    "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe",
    "$env:WINDIR\Microsoft.NET\Framework\v4.0.30319\csc.exe"
)
$cscExe = $cscCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $cscExe) {
    throw "Windows .NET Framework csc.exe not found."
}

$DistDir  = Join-Path $ProjectRoot "dist"
$StageDir = Join-Path $ProjectRoot "build\installer_stage"
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
New-Item -ItemType Directory -Path $DistDir  -Force | Out-Null
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

$PayloadDir = Join-Path $StageDir "payload"
$PayloadVst = Join-Path $PayloadDir "VST3\Md-101.vst3"
$PayloadApp = Join-Path $PayloadDir "App"
New-Item -ItemType Directory -Path $PayloadVst -Force | Out-Null
New-Item -ItemType Directory -Path $PayloadApp -Force | Out-Null

# 3. Compile Uninstaller executable (Uninstall-Md-101.exe)
$SetupCs = Join-Path $ProjectRoot "installer\windows\SetupProgram.cs"
$UninstOut = Join-Path $PayloadApp "Uninstall-Md-101.exe"

Write-Host "==> Compiling Windows Uninstaller (Uninstall-Md-101.exe)..." -ForegroundColor Cyan
& $cscExe /nologo /target:winexe /platform:x64 /optimize+ `
    /define:UNINSTALLER_ONLY `
    "/win32icon:$IconPath" `
    "/out:$UninstOut" `
    /r:System.dll /r:System.Drawing.dll /r:System.Windows.Forms.dll `
    /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll `
    $SetupCs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# 4. Populate payload directory
Write-Host "==> Staging VST3, Standalone, and Documentation payload..." -ForegroundColor Cyan
Copy-Item -Recurse -Force "$Vst3Bundle\*" $PayloadVst
Copy-Item -Force $StandaloneExe (Join-Path $PayloadApp "Md-101.exe")
Copy-Item -Force $IconPath (Join-Path $PayloadApp "md-101.ico")

foreach ($doc in @("README.md", "README-en.md", "LICENSE")) {
    $src = Join-Path $ProjectRoot $doc
    if (Test-Path $src) {
        Copy-Item -Force $src (Join-Path $PayloadApp $doc)
    }
}
$docsDir = Join-Path $ProjectRoot "docs"
if (Test-Path $docsDir) {
    $destDocs = Join-Path $PayloadApp "docs"
    New-Item -ItemType Directory -Path $destDocs -Force | Out-Null
    Copy-Item -Recurse -Force "$docsDir\*" $destDocs
}

# 5. Compress payload.zip
Add-Type -AssemblyName System.IO.Compression.FileSystem
$PayloadZip = Join-Path $StageDir "payload.zip"
if (Test-Path $PayloadZip) { Remove-Item -Force $PayloadZip }
[System.IO.Compression.ZipFile]::CreateFromDirectory($PayloadDir, $PayloadZip, [System.IO.Compression.CompressionLevel]::Optimal, $false)

# 6. Compile Standalone Windows Setup Wizard (.exe)
$SetupExeOut = Join-Path $DistDir "Md-101-$Version-Windows-x64-Setup.exe"
Write-Host "==> Compiling self-contained Windows Setup Wizard ($SetupExeOut)..." -ForegroundColor Cyan
& $cscExe /nologo /target:winexe /platform:x64 /optimize+ `
    "/win32icon:$IconPath" `
    "/res:$PayloadZip,Md101Setup.payload.zip" `
    "/out:$SetupExeOut" `
    /r:System.dll /r:System.Drawing.dll /r:System.Windows.Forms.dll `
    /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll `
    $SetupCs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# 7. Also build Portable ZIP Archive with 1-click Install/Uninstall scripts
$ZipFolderName = "Md-101-$Version-Windows-x64"
$ZipStageDir   = Join-Path $StageDir $ZipFolderName
New-Item -ItemType Directory -Path $ZipStageDir -Force | Out-Null

Copy-Item -Recurse -Force $Vst3Bundle (Join-Path $ZipStageDir "Md-101.vst3")
Copy-Item -Force $StandaloneExe (Join-Path $ZipStageDir "Md-101.exe")
Copy-Item -Force $IconPath (Join-Path $ZipStageDir "md-101.ico")
Copy-Item -Force (Join-Path $ProjectRoot "installer\windows\install_windows.ps1") (Join-Path $ZipStageDir "install_windows.ps1")
Copy-Item -Force (Join-Path $ProjectRoot "installer\windows\uninstall_windows.ps1") (Join-Path $ZipStageDir "uninstall_windows.ps1")

$installBat = @"
@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_windows.ps1" %*
pause
"@
[IO.File]::WriteAllText((Join-Path $ZipStageDir "Install-Md-101.bat"), $installBat)

$uninstallBat = @"
@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall_windows.ps1" %*
pause
"@
[IO.File]::WriteAllText((Join-Path $ZipStageDir "Uninstall-Md-101.bat"), $uninstallBat)

foreach ($doc in @("README.md", "README-en.md", "LICENSE")) {
    $src = Join-Path $ProjectRoot $doc
    if (Test-Path $src) {
        Copy-Item -Force $src (Join-Path $ZipStageDir $doc)
    }
}
if (Test-Path $docsDir) {
    $destZipDocs = Join-Path $ZipStageDir "docs"
    New-Item -ItemType Directory -Path $destZipDocs -Force | Out-Null
    Copy-Item -Recurse -Force "$docsDir\*" $destZipDocs
}

$PortableZipOut = Join-Path $DistDir "$ZipFolderName.zip"
if (Test-Path $PortableZipOut) { Remove-Item -Force $PortableZipOut }
[System.IO.Compression.ZipFile]::CreateFromDirectory($ZipStageDir, $PortableZipOut, [System.IO.Compression.CompressionLevel]::Optimal, $false)

Remove-Item -Recurse -Force $StageDir -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "==> Windows installers built successfully!" -ForegroundColor Green
Write-Host "    Setup Wizard (.exe) : $SetupExeOut"
Write-Host "    Portable ZIP (.zip) : $PortableZipOut"
