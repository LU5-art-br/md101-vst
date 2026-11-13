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
$AppVersion = "1.1.2"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)

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

function Assert-SafeDirectoryOwner([string]$DirPath) {
    if ([string]::IsNullOrEmpty($DirPath) -or -not (Test-Path -LiteralPath $DirPath)) {
        return
    }
    $full = [IO.Path]::GetFullPath($DirPath)
    if (-not [IO.Path]::GetDirectoryName($full)) {
        return
    }
    $sec = [IO.Directory]::GetAccessControl($full, [Security.AccessControl.AccessControlSections]::Owner)
    $owner = $sec.GetOwner([Security.Principal.SecurityIdentifier])
    if (-not $owner) {
        throw "Unable to verify owner of existing directory: $full"
    }
    $trustedSids = @(
        (New-Object Security.Principal.SecurityIdentifier([Security.Principal.WellKnownSidType]::BuiltinAdministratorsSid, $null)),
        (New-Object Security.Principal.SecurityIdentifier([Security.Principal.WellKnownSidType]::LocalSystemSid, $null)),
        (New-Object Security.Principal.SecurityIdentifier("S-1-5-80-956008885-3418522649-1831038044-1853292631-2271478464"))
    )
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent().User
    if ($currentUser) {
        $trustedSids += $currentUser
    }
    $isTrusted = $false
    foreach ($sid in $trustedSids) {
        if ($owner.Equals($sid)) {
            $isTrusted = $true
            break
        }
    }
    if (-not $isTrusted) {
        throw "Refusing elevated install into pre-existing directory owned by an untrusted account: $full"
    }
}

function Set-HardenedDirectoryAcl([string]$DirPath) {
    if ([string]::IsNullOrEmpty($DirPath) -or -not (Test-Path -LiteralPath $DirPath)) {
        return
    }
    $sec = New-Object Security.AccessControl.DirectorySecurity
    $sec.SetAccessRuleProtection($true, $false)
    $inherit = [Security.AccessControl.InheritanceFlags]::ContainerInherit -bor [Security.AccessControl.InheritanceFlags]::ObjectInherit
    $noneProp = [Security.AccessControl.PropagationFlags]::None
    $allow = [Security.AccessControl.AccessControlType]::Allow

    $sec.AddAccessRule((New-Object Security.AccessControl.FileSystemAccessRule(
        (New-Object Security.Principal.SecurityIdentifier([Security.Principal.WellKnownSidType]::BuiltinAdministratorsSid, $null)),
        [Security.AccessControl.FileSystemRights]::FullControl, $inherit, $noneProp, $allow)))
    $sec.AddAccessRule((New-Object Security.AccessControl.FileSystemAccessRule(
        (New-Object Security.Principal.SecurityIdentifier([Security.Principal.WellKnownSidType]::LocalSystemSid, $null)),
        [Security.AccessControl.FileSystemRights]::FullControl, $inherit, $noneProp, $allow)))
    $sec.AddAccessRule((New-Object Security.AccessControl.FileSystemAccessRule(
        (New-Object Security.Principal.SecurityIdentifier([Security.Principal.WellKnownSidType]::BuiltinUsersSid, $null)),
        ([Security.AccessControl.FileSystemRights]::ReadAndExecute -bor [Security.AccessControl.FileSystemRights]::Synchronize),
        $inherit, $noneProp, $allow)))

    [IO.Directory]::SetAccessControl($DirPath, $sec)
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
        $psExe = Join-Path $env:SystemRoot "System32\WindowsPowerShell\v1.0\powershell.exe"
        $proc = Start-Process -FilePath $psExe -ArgumentList $argList -Verb RunAs -Wait -PassThru
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
    if (Test-IsAdmin) {
        Assert-SafeDirectoryOwner $Vst3Dir
        Assert-SafeDirectoryOwner $targetVst3
    }
    New-Item -ItemType Directory -Path $Vst3Dir -Force | Out-Null
    if (Test-Path -LiteralPath $targetVst3) {
        if (Test-IsReparsePoint $targetVst3) {
            throw "Refusing to delete reparse point at $targetVst3"
        }
        Remove-Item -LiteralPath $targetVst3 -Recurse -Force
    }
    New-Item -ItemType Directory -Path $targetVst3 -Force | Out-Null
    if (Test-IsAdmin) {
        Set-HardenedDirectoryAcl $targetVst3
    }
    Copy-Item -Recurse -Force "$Vst3Source\*" $targetVst3
}

# 2. Install Standalone App, Documentation & Uninstaller
if (-not $Vst3Only -and $ExeSource) {
    if ($Lang -eq "pt") {
        Write-Host "==> Instalando Aplicativo Standalone e Manuais em: $InstallDir" -ForegroundColor Cyan
    } else {
        Write-Host "==> Installing Standalone Application & Manuals to: $InstallDir" -ForegroundColor Cyan
    }
    if (Test-IsAdmin) {
        Assert-SafeDirectoryOwner $InstallDir
    }
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    if (Test-IsAdmin) {
        Set-HardenedDirectoryAcl $InstallDir
    }
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
