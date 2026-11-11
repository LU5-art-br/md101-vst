<#
.SYNOPSIS
    Md-101 — Instalador Windows (VST3 & Standalone)
    Desenvolvido por LU5 (https://lu5.art.br)
#>
param(
    [switch]$Silent,
    [switch]$Vst3Only,
    [switch]$StandaloneOnly,
    [string]$InstallDir = "$env:ProgramFiles\LU5\Md-101",
    [string]$Vst3Dir    = "$env:CommonProgramFiles\VST3",
    [switch]$NoDesktopShortcut,
    [string]$Lang       = ""
)

$ErrorActionPreference = "Stop"
$AppVersion = "1.1.1"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)

if (-not $Lang) {
    $uiCulture = (Get-UICulture).TwoLetterISOLanguageName
    $Lang = if ($uiCulture -eq "pt") { "pt" } else { "en" }
}

function Test-IsAdmin {
    $identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-PathWritable([string]$TargetPath) {
    $parent = $TargetPath
    while ($parent -and -not (Test-Path $parent)) {
        $parent = Split-Path -Parent $parent
    }
    if (-not $parent) { return $false }
    $probe = Join-Path $parent (".md101_write_test_" + [guid]::NewGuid().ToString("N"))
    try {
        [IO.File]::WriteAllText($probe, "ok")
        Remove-Item -Force $probe -ErrorAction SilentlyContinue
        return $true
    } catch {
        return $false
    }
}

# Elevate via UAC if target directories require Administrator privileges
if (-not (Test-IsAdmin)) {
    $needsAdmin = (-not (Test-PathWritable $Vst3Dir)) -or (-not (Test-PathWritable $InstallDir))
    if ($needsAdmin) {
        if ($Lang -eq "pt") {
            Write-Host "==> Solicitando permissao de Administrador (UAC) para instalar em Program Files..." -ForegroundColor Cyan
        } else {
            Write-Host "==> Requesting Administrator privileges (UAC) to install into Program Files..." -ForegroundColor Cyan
        }
        $argList = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"")
        if ($Silent)            { $argList += "-Silent" }
        if ($Vst3Only)          { $argList += "-Vst3Only" }
        if ($StandaloneOnly)    { $argList += "-StandaloneOnly" }
        if ($NoDesktopShortcut) { $argList += "-NoDesktopShortcut" }
        $argList += @("-InstallDir", "`"$InstallDir`"", "-Vst3Dir", "`"$Vst3Dir`"", "-Lang", $Lang)
        $proc = Start-Process -FilePath "powershell.exe" -ArgumentList $argList -Verb RunAs -Wait -PassThru
        exit $proc.ExitCode
    }
}

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host "  [101] Md - Md-101 v$AppVersion (LU5 - https://lu5.art.br)" -ForegroundColor White
if ($Lang -eq "pt") {
    Write-Host "  Instalador Windows - Sintetizador VST3 & Standalone da Tabela Periodica" -ForegroundColor Gray
} else {
    Write-Host "  Windows Installer - Periodic Table VST3 & Standalone Synthesizer" -ForegroundColor Gray
}
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ""

# Locate VST3 and Standalone binaries
$vst3Candidates = @(
    (Join-Path $ScriptDir "Md-101.vst3"),
    (Join-Path $ProjectRoot "build\Md101_artefacts\Release\VST3\Md-101.vst3"),
    (Join-Path $ProjectRoot "build\Md101_artefacts\Debug\VST3\Md-101.vst3")
)
$Vst3Source = $vst3Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

$exeCandidates = @(
    (Join-Path $ScriptDir "Md-101.exe"),
    (Join-Path $ProjectRoot "build\Md101_artefacts\Release\Standalone\Md-101.exe"),
    (Join-Path $ProjectRoot "build\Md101_artefacts\Debug\Standalone\Md-101.exe")
)
$ExeSource = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $StandaloneOnly -and -not $Vst3Source) {
    throw "Md-101.vst3 not found. Run .\build_windows.ps1 first."
}

# 1. Install VST3 Plugin
if (-not $StandaloneOnly -and $Vst3Source) {
    $targetVst3 = Join-Path $Vst3Dir "Md-101.vst3"
    if ($Lang -eq "pt") {
        Write-Host "==> Instalando Plugin VST3 em: $targetVst3" -ForegroundColor Cyan
    } else {
        Write-Host "==> Installing VST3 Plugin to: $targetVst3" -ForegroundColor Cyan
    }
    New-Item -ItemType Directory -Path $Vst3Dir -Force | Out-Null
    if (Test-Path $targetVst3) {
        Remove-Item -Recurse -Force $targetVst3
    }
    Copy-Item -Recurse -Force $Vst3Source $targetVst3
}

# 2. Install Standalone App, Documentation & Uninstaller
if (-not $Vst3Only -and $ExeSource) {
    if ($Lang -eq "pt") {
        Write-Host "==> Instalando Aplicativo Standalone e Manuais em: $InstallDir" -ForegroundColor Cyan
    } else {
        Write-Host "==> Installing Standalone Application & Manuals to: $InstallDir" -ForegroundColor Cyan
    }
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Copy-Item -Force $ExeSource (Join-Path $InstallDir "Md-101.exe")

    foreach ($docFile in @("README.md", "README-en.md", "LICENSE")) {
        foreach ($base in @($ScriptDir, $ProjectRoot)) {
            $candidate = Join-Path $base $docFile
            if (Test-Path $candidate) {
                Copy-Item -Force $candidate (Join-Path $InstallDir $docFile)
                break
            }
        }
    }

    foreach ($base in @($ScriptDir, $ProjectRoot)) {
        $docsCandidate = Join-Path $base "docs"
        if (Test-Path $docsCandidate) {
            $destDocs = Join-Path $InstallDir "docs"
            New-Item -ItemType Directory -Path $destDocs -Force | Out-Null
            Copy-Item -Recurse -Force "$docsCandidate\*" $destDocs
            break
        }
    }

    $icoCandidates = @(
        (Join-Path $ScriptDir "md-101.ico"),
        (Join-Path $ProjectRoot "installer\windows\md-101.ico")
    )
    $icoSource = $icoCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($icoSource) {
        Copy-Item -Force $icoSource (Join-Path $InstallDir "md-101.ico")
    }

    $uninstCandidates = @(
        (Join-Path $ScriptDir "uninstall_windows.ps1"),
        (Join-Path $ProjectRoot "installer\windows\uninstall_windows.ps1")
    )
    $uninstSource = $uninstCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($uninstSource) {
        Copy-Item -Force $uninstSource (Join-Path $InstallDir "uninstall_windows.ps1")
    }

    # Create Start Menu & Desktop Shortcuts when installing to standard Program Files
    if ($InstallDir -eq "$env:ProgramFiles\LU5\Md-101") {
        try {
            $wsh = New-Object -ComObject WScript.Shell
            $startMenuDir = Join-Path ([Environment]::GetFolderPath("CommonPrograms")) "Md-101"
            New-Item -ItemType Directory -Path $startMenuDir -Force | Out-Null

            $lnkApp = $wsh.CreateShortcut((Join-Path $startMenuDir "Md-101.lnk"))
            $lnkApp.TargetPath = Join-Path $InstallDir "Md-101.exe"
            $lnkApp.WorkingDirectory = $InstallDir
            $lnkApp.Description = "Md-101 - Periodic Table Polyphonic Synthesizer by LU5"
            if (Test-Path (Join-Path $InstallDir "md-101.ico")) {
                $lnkApp.IconLocation = Join-Path $InstallDir "md-101.ico"
            }
            $lnkApp.Save()

            if (Test-Path (Join-Path $InstallDir "docs\manual.html")) {
                $lnkDoc = $wsh.CreateShortcut((Join-Path $startMenuDir "Md-101 - Manual.lnk"))
                $lnkDoc.TargetPath = Join-Path $InstallDir "docs\manual.html"
                $lnkDoc.Save()
            }

            if (-not $NoDesktopShortcut) {
                $desktopDir = [Environment]::GetFolderPath("CommonDesktopDirectory")
                $lnkDesk = $wsh.CreateShortcut((Join-Path $desktopDir "Md-101.lnk"))
                $lnkDesk.TargetPath = Join-Path $InstallDir "Md-101.exe"
                $lnkDesk.WorkingDirectory = $InstallDir
                $lnkDesk.Description = "Md-101 - Periodic Table Polyphonic Synthesizer by LU5"
                if (Test-Path (Join-Path $InstallDir "md-101.ico")) {
                    $lnkDesk.IconLocation = Join-Path $InstallDir "md-101.ico"
                }
                $lnkDesk.Save()
            }
        } catch {
            Write-Host "Note: Could not create shell shortcuts: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
}

Write-Host ""
if ($Lang -eq "pt") {
    Write-Host "==> Md-101 v$AppVersion instalado com sucesso!" -ForegroundColor Green
    if (-not $StandaloneOnly) { Write-Host "    VST3       : $(Join-Path $Vst3Dir 'Md-101.vst3')" }
    if (-not $Vst3Only)       { Write-Host "    Standalone : $(Join-Path $InstallDir 'Md-101.exe')" }
} else {
    Write-Host "==> Md-101 v$AppVersion installed successfully!" -ForegroundColor Green
    if (-not $StandaloneOnly) { Write-Host "    VST3       : $(Join-Path $Vst3Dir 'Md-101.vst3')" }
    if (-not $Vst3Only)       { Write-Host "    Standalone : $(Join-Path $InstallDir 'Md-101.exe')" }
}
Write-Host ""
