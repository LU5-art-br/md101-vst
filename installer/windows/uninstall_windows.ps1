<#
.SYNOPSIS
    Md-101 — Desinstalador Windows (VST3 & Standalone)
    Desenvolvido por LU5 (https://lu5.art.br)
#>
param(
    [switch]$Silent,
    [string]$InstallDir = "$env:ProgramFiles\LU5\Md-101",
    [string]$Vst3Dir    = "$env:CommonProgramFiles\VST3",
    [string]$Lang       = ""
)

$ErrorActionPreference = "Stop"

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

if (-not (Test-IsAdmin)) {
    $needsAdmin = (-not (Test-PathWritable $Vst3Dir)) -or (-not (Test-PathWritable $InstallDir))
    if ($needsAdmin) {
        $argList = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"")
        if ($Silent) { $argList += "-Silent" }
        $argList += @("-InstallDir", "`"$InstallDir`"", "-Vst3Dir", "`"$Vst3Dir`"", "-Lang", $Lang)
        $proc = Start-Process -FilePath "powershell.exe" -ArgumentList $argList -Verb RunAs -Wait -PassThru
        exit $proc.ExitCode
    }
}

$targetVst3 = Join-Path $Vst3Dir "Md-101.vst3"
if (Test-Path $targetVst3) {
    Remove-Item -Recurse -Force $targetVst3
}

if ($InstallDir -eq "$env:ProgramFiles\LU5\Md-101") {
    $startMenuDir = Join-Path ([Environment]::GetFolderPath("CommonPrograms")) "Md-101"
    if (Test-Path $startMenuDir) {
        Remove-Item -Recurse -Force $startMenuDir -ErrorAction SilentlyContinue
    }
    $desktopLnk = Join-Path ([Environment]::GetFolderPath("CommonDesktopDirectory")) "Md-101.lnk"
    if (Test-Path $desktopLnk) {
        Remove-Item -Force $desktopLnk -ErrorAction SilentlyContinue
    }
    $regPath = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Md-101"
    if (Test-Path $regPath) {
        Remove-Item -Recurse -Force $regPath -ErrorAction SilentlyContinue
    }
}

if (Test-Path $InstallDir) {
    Remove-Item -Recurse -Force $InstallDir -ErrorAction SilentlyContinue
}

if ($Lang -eq "pt") {
    Write-Host "==> Md-101 removido com sucesso." -ForegroundColor Green
} else {
    Write-Host "==> Md-101 uninstalled successfully." -ForegroundColor Green
}
