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
} elseif ($Lang -ne "pt" -and $Lang -ne "en") {
    $Lang = "en"
}

function Resolve-SafeDirectory([string]$TargetPath) {
    if ([string]::IsNullOrWhiteSpace($TargetPath)) {
        throw "Directory path cannot be empty."
    }
    $trimmed = $TargetPath.Trim()
    if ($trimmed.IndexOfAny([char[]]@('"', '&', '|', '<', '>', ';', '%', '*', '?', "`r", "`n", [char]0)) -ge 0) {
        throw "Directory path contains forbidden characters: $trimmed"
    }
    if ($trimmed.StartsWith("\\")) {
        throw "UNC or device paths are not allowed: $trimmed"
    }
    if (-not [IO.Path]::IsPathRooted($trimmed)) {
        throw "Directory path must be absolute: $trimmed"
    }
    $full = [IO.Path]::GetFullPath($trimmed).TrimEnd('\', '/')
    $root = [IO.Path]::GetPathRoot($full).TrimEnd('\', '/')
    if ($full -ieq $root) {
        throw "Drive root cannot be used as target directory: $full"
    }
    $protected = @(
        $env:SystemRoot,
        [Environment]::GetFolderPath("System"),
        [Environment]::GetFolderPath("ProgramFiles"),
        [Environment]::GetFolderPath("ProgramFilesX86"),
        [Environment]::GetFolderPath("CommonProgramFiles"),
        [Environment]::GetFolderPath("CommonProgramFilesX86"),
        [Environment]::GetFolderPath("UserProfile"),
        [Environment]::GetFolderPath("ApplicationData"),
        [Environment]::GetFolderPath("LocalApplicationData"),
        [Environment]::GetFolderPath("CommonApplicationData"),
        (Split-Path -Parent ([Environment]::GetFolderPath("UserProfile")))
    ) | Where-Object { $_ } | ForEach-Object { [IO.Path]::GetFullPath($_).TrimEnd('\', '/') }
    foreach ($p in $protected) {
        if ($full -ieq $p) {
            throw "Protected system directory cannot be used: $full"
        }
    }
    return $full
}

function Test-IsReparsePoint([string]$PathToCheck) {
    if (-not (Test-Path -LiteralPath $PathToCheck)) { return $false }
    $item = Get-Item -LiteralPath $PathToCheck -Force -ErrorAction SilentlyContinue
    if (-not $item) { return $false }
    return (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)
}

$InstallDir = Resolve-SafeDirectory $InstallDir
$Vst3Dir    = Resolve-SafeDirectory $Vst3Dir

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
        $psExe = Join-Path $env:SystemRoot "System32\WindowsPowerShell\v1.0\powershell.exe"
        $proc = Start-Process -FilePath $psExe -ArgumentList $argList -Verb RunAs -Wait -PassThru
        exit $proc.ExitCode
    }
}

$targetVst3 = Join-Path $Vst3Dir "Md-101.vst3"
if ((Test-Path -LiteralPath $targetVst3) -and (-not (Test-IsReparsePoint $targetVst3))) {
    Remove-Item -LiteralPath $targetVst3 -Recurse -Force
}

if ($InstallDir -eq "$env:ProgramFiles\LU5\Md-101") {
    $startMenuDir = Join-Path ([Environment]::GetFolderPath("CommonPrograms")) "Md-101"
    if ((Test-Path -LiteralPath $startMenuDir) -and (-not (Test-IsReparsePoint $startMenuDir))) {
        Remove-Item -LiteralPath $startMenuDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    $desktopLnk = Join-Path ([Environment]::GetFolderPath("CommonDesktopDirectory")) "Md-101.lnk"
    if (Test-Path -LiteralPath $desktopLnk) {
        Remove-Item -LiteralPath $desktopLnk -Force -ErrorAction SilentlyContinue
    }
    $regPath = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Md-101"
    if (Test-Path $regPath) {
        Remove-Item -Recurse -Force $regPath -ErrorAction SilentlyContinue
    }
}

if ((Test-Path -LiteralPath $InstallDir) -and (-not (Test-IsReparsePoint $InstallDir))) {
    $hasMarker = (Test-Path -LiteralPath (Join-Path $InstallDir "Md-101.exe")) -or
                 (Test-Path -LiteralPath (Join-Path $InstallDir "uninstall_windows.ps1")) -or
                 (Test-Path -LiteralPath (Join-Path $InstallDir "install-manifest.cfg"))
    if ($hasMarker) {
        Remove-Item -LiteralPath $InstallDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

if ($Lang -eq "pt") {
    Write-Host "==> Md-101 removido com sucesso." -ForegroundColor Green
} else {
    Write-Host "==> Md-101 uninstalled successfully." -ForegroundColor Green
}
