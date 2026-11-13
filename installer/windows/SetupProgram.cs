// ==============================================================================
// Md-101 — Native Windows GUI & CLI Setup / Uninstaller (x64)
// Periodic Table Polyphonic VST3 & Standalone Synthesizer
// Developed by LU5 (https://lu5.art.br)
// Compiles with built-in Windows .NET Framework csc.exe (zero external deps)
// ==============================================================================
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

[assembly: AssemblyTitle("Md-101 Setup")]
[assembly: AssemblyDescription("Md-101 - Periodic Table VST3 & Standalone Synthesizer Installer")]
[assembly: AssemblyCompany("LU5 (https://lu5.art.br)")]
[assembly: AssemblyProduct("Md-101")]
[assembly: AssemblyCopyright("Copyright (c) 2026 LU5")]
[assembly: AssemblyVersion("1.1.2.0")]
[assembly: AssemblyFileVersion("1.1.2.0")]

namespace Md101Installer
{
    internal sealed class InstallOptions
    {
        public bool Silent = false;
        public bool Uninstall = false;
        public bool ElevatedAutoRun = false;
        public bool InstallVst3 = true;
        public bool InstallStandalone = true;
        public bool CreateStartMenu = true;
        public bool CreateDesktop = true;
        public string Lang = "pt";
        public string InstallDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), @"LU5\Md-101");
        public string Vst3Dir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "VST3");
    }

    internal static class Program
    {
        [STAThread]
        public static int Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            InstallOptions opts = ParseArgs(args);

#if UNINSTALLER_ONLY
            opts.Uninstall = true;
#endif
            string exeName = Path.GetFileNameWithoutExtension(Application.ExecutablePath) ?? "";
            if (exeName.StartsWith("Uninstall", StringComparison.OrdinalIgnoreCase))
            {
                opts.Uninstall = true;
            }

            if (opts.Silent)
            {
                return RunSilent(opts);
            }

            if (opts.Uninstall)
            {
                Application.Run(new UninstallForm(opts));
                return 0;
            }

            Application.Run(new SetupForm(opts));
            return 0;
        }

        private static InstallOptions ParseArgs(string[] args)
        {
            InstallOptions opts = new InstallOptions();
            bool isPt =
                string.Equals(CultureInfo.CurrentUICulture.TwoLetterISOLanguageName, "pt", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(CultureInfo.CurrentCulture.TwoLetterISOLanguageName, "pt", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(CultureInfo.InstalledUICulture.TwoLetterISOLanguageName, "pt", StringComparison.OrdinalIgnoreCase);
            opts.Lang = isPt ? "pt" : "en";

            for (int i = 0; i < args.Length; i++)
            {
                string a = args[i] ?? "";
                if (a.Equals("/S", StringComparison.OrdinalIgnoreCase) ||
                    a.Equals("--silent", StringComparison.OrdinalIgnoreCase) ||
                    a.Equals("-s", StringComparison.OrdinalIgnoreCase))
                {
                    opts.Silent = true;
                }
                else if (a.Equals("/UNINSTALL", StringComparison.OrdinalIgnoreCase) ||
                         a.Equals("--uninstall", StringComparison.OrdinalIgnoreCase))
                {
                    opts.Uninstall = true;
                }
                else if (a.Equals("--elevated-run", StringComparison.OrdinalIgnoreCase))
                {
                    opts.ElevatedAutoRun = true;
                }
                else if (a.StartsWith("/DIR=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.InstallDir = a.Substring(5).Trim('"');
                    opts.CreateStartMenu = false;
                    opts.CreateDesktop = false;
                }
                else if (a.StartsWith("--dir=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.InstallDir = a.Substring(6).Trim('"');
                }
                else if (a.StartsWith("/VST3DIR=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.Vst3Dir = a.Substring(9).Trim('"');
                }
                else if (a.StartsWith("--vst3dir=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.Vst3Dir = a.Substring(10).Trim('"');
                }
                else if (a.StartsWith("--lang=", StringComparison.OrdinalIgnoreCase))
                {
                    string l = a.Substring(7).Trim().ToLowerInvariant();
                    opts.Lang = (l == "pt") ? "pt" : "en";
                }
                else if (a.StartsWith("--vst3=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.InstallVst3 = a.Substring(7) == "1";
                }
                else if (a.StartsWith("--standalone=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.InstallStandalone = a.Substring(13) == "1";
                }
                else if (a.StartsWith("--startmenu=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.CreateStartMenu = a.Substring(12) == "1";
                }
                else if (a.StartsWith("--desktop=", StringComparison.OrdinalIgnoreCase))
                {
                    opts.CreateDesktop = a.Substring(10) == "1";
                }
            }
            return opts;
        }

        private static int RunSilent(InstallOptions opts)
        {
            try
            {
                string normApp, normVst;
                if (!InstallerEngine.NormalizeAndValidateDirectory(opts.InstallDir, out normApp))
                    throw new ArgumentException("Invalid installation directory path.");
                if (!InstallerEngine.NormalizeAndValidateDirectory(opts.Vst3Dir, out normVst))
                    throw new ArgumentException("Invalid VST3 directory path.");
                opts.InstallDir = normApp;
                opts.Vst3Dir = normVst;

                if (opts.Uninstall)
                {
                    InstallerEngine.LoadManifestIfPresent(opts);
                    if (InstallerEngine.RequiresElevation(opts) && !InstallerEngine.IsAdministrator())
                    {
                        return InstallerEngine.RelaunchElevated(opts, true);
                    }
                    InstallerEngine.PerformUninstall(opts, delegate(string msg, int pct) { });
                    return 0;
                }
                else
                {
                    if (InstallerEngine.RequiresElevation(opts) && !InstallerEngine.IsAdministrator())
                    {
                        return InstallerEngine.RelaunchElevated(opts, true);
                    }
                    InstallerEngine.PerformInstall(opts, delegate(string msg, int pct) { });
                    return 0;
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("Md-101 Installer Error: " + ex.Message);
                return 1;
            }
        }
    }

    internal static class InstallerEngine
    {
        private static readonly char[] ForbiddenPathChars = new char[]
        {
            '"', '&', '|', '<', '>', ';', '%', '*', '?', '\r', '\n', '\0'
        };

        private const uint PROCESS_QUERY_INFORMATION = 0x0400;
        private const uint TOKEN_DUPLICATE = 0x0002;
        private const uint TOKEN_ASSIGN_PRIMARY = 0x0001;
        private const uint TOKEN_QUERY = 0x0008;
        private const uint MAXIMUM_ALLOWED = 0x02000000;
        private const int SecurityImpersonation = 2;
        private const int TokenPrimary = 1;
        private const int MOVEFILE_DELAY_UNTIL_REBOOT = 0x00000004;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct STARTUPINFOW
        {
            public int cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public int dwX;
            public int dwY;
            public int dwXSize;
            public int dwYSize;
            public int dwXCountChars;
            public int dwYCountChars;
            public int dwFillAttribute;
            public int dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }

        [DllImport("user32.dll")]
        private static extern IntPtr GetShellWindow();

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool OpenProcessToken(IntPtr ProcessHandle, uint DesiredAccess, out IntPtr TokenHandle);

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool DuplicateTokenEx(
            IntPtr hExistingToken,
            uint dwDesiredAccess,
            IntPtr lpTokenAttributes,
            int ImpersonationLevel,
            int TokenType,
            out IntPtr phNewToken);

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool CreateProcessWithTokenW(
            IntPtr hToken,
            int dwLogonFlags,
            string lpApplicationName,
            string lpCommandLine,
            int dwCreationFlags,
            IntPtr lpEnvironment,
            string lpCurrentDirectory,
            ref STARTUPINFOW lpStartupInfo,
            out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool MoveFileExW(string lpExistingFileName, string lpNewFileName, int dwFlags);

        public static bool NormalizeAndValidateDirectory(string rawPath, out string normalized)
        {
            normalized = null;
            if (string.IsNullOrWhiteSpace(rawPath))
                return false;

            string trimmed = rawPath.Trim();
            if (trimmed.IndexOfAny(ForbiddenPathChars) >= 0)
                return false;

            // Reject UNC paths or device namespaces (\\server\share, \\?\...)
            if (trimmed.StartsWith("\\\\", StringComparison.Ordinal))
                return false;

            try
            {
                if (!Path.IsPathRooted(trimmed))
                    return false;

                string full = Path.GetFullPath(trimmed).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                if (string.IsNullOrEmpty(full))
                    return false;

                string root = Path.GetPathRoot(full);
                if (string.IsNullOrEmpty(root))
                    return false;

                string cleanRoot = root.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                if (string.Equals(full, cleanRoot, StringComparison.OrdinalIgnoreCase))
                    return false;

                string[] protectedDirs = new string[]
                {
                    Environment.GetFolderPath(Environment.SpecialFolder.Windows),
                    Environment.GetFolderPath(Environment.SpecialFolder.System),
                    Environment.GetFolderPath(Environment.SpecialFolder.SystemX86),
                    Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                    Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
                    Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles),
                    Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFilesX86),
                    Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                    Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData),
                    Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),
                    Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory)
                };

                for (int i = 0; i < protectedDirs.Length; i++)
                {
                    string p = protectedDirs[i];
                    if (string.IsNullOrEmpty(p)) continue;
                    string cleanP = Path.GetFullPath(p).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                    if (string.Equals(full, cleanP, StringComparison.OrdinalIgnoreCase))
                        return false;
                }

                string userRoot = Path.GetDirectoryName(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
                if (!string.IsNullOrEmpty(userRoot))
                {
                    string cleanUserRoot = Path.GetFullPath(userRoot).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                    if (string.Equals(full, cleanUserRoot, StringComparison.OrdinalIgnoreCase))
                        return false;
                }

                normalized = full;
                return true;
            }
            catch
            {
                return false;
            }
        }

        private static bool IsReparsePoint(string path)
        {
            try
            {
                if (!Directory.Exists(path) && !File.Exists(path))
                    return false;
                return (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0;
            }
            catch
            {
                return true;
            }
        }

        private static void SafeDeleteDirectory(string path)
        {
            if (!Directory.Exists(path) || IsReparsePoint(path))
                return;
            Directory.Delete(path, true);
        }

        private static void DeleteDirectoryContentsExceptSelf(string dirPath, string currentExeFullPath)
        {
            if (!Directory.Exists(dirPath) || IsReparsePoint(dirPath))
                return;

            foreach (string file in Directory.GetFiles(dirPath))
            {
                if (!string.Equals(Path.GetFullPath(file), currentExeFullPath, StringComparison.OrdinalIgnoreCase))
                {
                    try { File.Delete(file); } catch { }
                }
            }

            foreach (string subDir in Directory.GetDirectories(dirPath))
            {
                if (IsReparsePoint(subDir))
                    continue;
                DeleteDirectoryContentsExceptSelf(subDir, currentExeFullPath);
                try
                {
                    if (Directory.GetFiles(subDir).Length == 0 && Directory.GetDirectories(subDir).Length == 0)
                        Directory.Delete(subDir, false);
                }
                catch { }
            }
        }

        private static bool IsSafeToDeleteInstallDir(string dirPath)
        {
            string norm;
            if (!NormalizeAndValidateDirectory(dirPath, out norm))
                return false;
            if (!Directory.Exists(norm) || IsReparsePoint(norm))
                return false;

            return File.Exists(Path.Combine(norm, "Md-101.exe")) ||
                   File.Exists(Path.Combine(norm, "Uninstall-Md-101.exe")) ||
                   File.Exists(Path.Combine(norm, "install-manifest.cfg"));
        }

        public static bool LaunchAsDesktopUser(string exePath)
        {
            if (string.IsNullOrEmpty(exePath) || !File.Exists(exePath))
                return false;

            if (!IsAdministrator())
            {
                try
                {
                    Process.Start(exePath);
                    return true;
                }
                catch
                {
                    return false;
                }
            }

            IntPtr hShellProc = IntPtr.Zero;
            IntPtr hShellToken = IntPtr.Zero;
            IntPtr hPrimaryToken = IntPtr.Zero;
            try
            {
                IntPtr shellHwnd = GetShellWindow();
                if (shellHwnd == IntPtr.Zero)
                    return false;

                uint shellPid;
                GetWindowThreadProcessId(shellHwnd, out shellPid);
                if (shellPid == 0)
                    return false;

                hShellProc = OpenProcess(PROCESS_QUERY_INFORMATION, false, shellPid);
                if (hShellProc == IntPtr.Zero)
                    return false;

                if (!OpenProcessToken(hShellProc, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY, out hShellToken))
                    return false;

                if (!DuplicateTokenEx(hShellToken, MAXIMUM_ALLOWED, IntPtr.Zero, SecurityImpersonation, TokenPrimary, out hPrimaryToken))
                    return false;

                STARTUPINFOW si = new STARTUPINFOW();
                si.cb = Marshal.SizeOf(typeof(STARTUPINFOW));
                PROCESS_INFORMATION pi;
                string workDir = Path.GetDirectoryName(exePath);
                bool ok = CreateProcessWithTokenW(
                    hPrimaryToken,
                    0,
                    exePath,
                    "\"" + exePath + "\"",
                    0,
                    IntPtr.Zero,
                    workDir,
                    ref si,
                    out pi);

                if (ok)
                {
                    if (pi.hThread != IntPtr.Zero) CloseHandle(pi.hThread);
                    if (pi.hProcess != IntPtr.Zero) CloseHandle(pi.hProcess);
                }
                return ok;
            }
            catch
            {
                return false;
            }
            finally
            {
                if (hPrimaryToken != IntPtr.Zero) CloseHandle(hPrimaryToken);
                if (hShellToken != IntPtr.Zero) CloseHandle(hShellToken);
                if (hShellProc != IntPtr.Zero) CloseHandle(hShellProc);
            }
        }
        public static bool IsAdministrator()
        {
            try
            {
                WindowsIdentity id = WindowsIdentity.GetCurrent();
                WindowsPrincipal principal = new WindowsPrincipal(id);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
            catch
            {
                return false;
            }
        }

        public static bool IsDirectoryWritable(string dirPath)
        {
            try
            {
                string current = Path.GetFullPath(dirPath);
                while (!string.IsNullOrEmpty(current) && !Directory.Exists(current))
                {
                    current = Path.GetDirectoryName(current);
                }
                if (string.IsNullOrEmpty(current)) return false;

                string probe = Path.Combine(current, ".md101_probe_" + Guid.NewGuid().ToString("N"));
                File.WriteAllText(probe, "ok");
                File.Delete(probe);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public static bool RequiresElevation(InstallOptions opts)
        {
            if (opts.InstallVst3 && !IsDirectoryWritable(opts.Vst3Dir))
                return true;
            if (opts.InstallStandalone && !IsDirectoryWritable(opts.InstallDir))
                return true;
            return false;
        }

        public static int RelaunchElevated(InstallOptions opts, bool silent)
        {
            string normApp, normVst;
            if (!NormalizeAndValidateDirectory(opts.InstallDir, out normApp) ||
                !NormalizeAndValidateDirectory(opts.Vst3Dir, out normVst))
            {
                throw new ArgumentException("Invalid target directory path.");
            }
            string safeLang = (opts.Lang == "pt") ? "pt" : "en";

            List<string> argParts = new List<string>();
            if (silent) argParts.Add("/S");
            if (opts.Uninstall) argParts.Add("/UNINSTALL");
            argParts.Add("--elevated-run");
            argParts.Add("--lang=" + safeLang);
            argParts.Add("--dir=\"" + normApp + "\"");
            argParts.Add("--vst3dir=\"" + normVst + "\"");
            argParts.Add("--vst3=" + (opts.InstallVst3 ? "1" : "0"));
            argParts.Add("--standalone=" + (opts.InstallStandalone ? "1" : "0"));
            argParts.Add("--startmenu=" + (opts.CreateStartMenu ? "1" : "0"));
            argParts.Add("--desktop=" + (opts.CreateDesktop ? "1" : "0"));

            ProcessStartInfo psi = new ProcessStartInfo();
            psi.FileName = Application.ExecutablePath;
            psi.Arguments = string.Join(" ", argParts.ToArray());
            psi.Verb = "runas";
            psi.UseShellExecute = true;

            Process proc = Process.Start(psi);
            if (proc != null)
            {
                proc.WaitForExit();
                return proc.ExitCode;
            }
            return 1;
        }

        public static void PerformInstall(InstallOptions opts, Action<string, int> progress)
        {
            bool pt = opts.Lang == "pt";
            progress(pt ? "Preparando pacotes do Md-101..." : "Preparing Md-101 packages...", 5);

            string normApp, normVst;
            if (opts.InstallStandalone)
            {
                if (!NormalizeAndValidateDirectory(opts.InstallDir, out normApp))
                    throw new ArgumentException("Invalid installation directory path.");
                opts.InstallDir = normApp;
            }
            if (opts.InstallVst3)
            {
                if (!NormalizeAndValidateDirectory(opts.Vst3Dir, out normVst))
                    throw new ArgumentException("Invalid VST3 directory path.");
                opts.Vst3Dir = normVst;
            }

            Assembly asm = Assembly.GetExecutingAssembly();
            Stream zipStream = asm.GetManifestResourceStream("Md101Setup.payload.zip");
            if (zipStream == null)
            {
                throw new InvalidOperationException("Embedded resource Md101Setup.payload.zip not found.");
            }

            using (zipStream)
            using (ZipArchive archive = new ZipArchive(zipStream, ZipArchiveMode.Read))
            {
                int totalEntries = archive.Entries.Count;
                int processed = 0;

                if (opts.InstallVst3)
                {
                    string targetVst3Root = Path.Combine(opts.Vst3Dir, "Md-101.vst3");
                    progress(pt ? ("Instalando VST3 em " + targetVst3Root + "...") : ("Installing VST3 to " + targetVst3Root + "..."), 15);
                    if (IsAdministrator())
                    {
                        VerifySafePreexistingDirectoryOwner(opts.Vst3Dir);
                        VerifySafePreexistingDirectoryOwner(targetVst3Root);
                    }
                    if (Directory.Exists(targetVst3Root))
                    {
                        try { SafeDeleteDirectory(targetVst3Root); } catch { }
                    }
                    Directory.CreateDirectory(targetVst3Root);
                    if (IsAdministrator())
                    {
                        HardenElevatedDirectoryAcl(targetVst3Root);
                    }
                }

                if (opts.InstallStandalone)
                {
                    progress(pt ? ("Instalando Standalone e Manuais em " + opts.InstallDir + "...") : ("Installing Standalone & Manuals to " + opts.InstallDir + "..."), 40);
                    if (IsAdministrator())
                    {
                        VerifySafePreexistingDirectoryOwner(opts.InstallDir);
                    }
                    Directory.CreateDirectory(opts.InstallDir);
                    if (IsAdministrator())
                    {
                        HardenElevatedDirectoryAcl(opts.InstallDir);
                    }
                }

                foreach (ZipArchiveEntry entry in archive.Entries)
                {
                    processed++;
                    string fullName = entry.FullName.Replace('\\', '/');

                    if (fullName.StartsWith("VST3/Md-101.vst3/", StringComparison.OrdinalIgnoreCase))
                    {
                        if (!opts.InstallVst3) continue;
                        string rel = fullName.Substring("VST3/".Length);
                        string destPath = Path.Combine(opts.Vst3Dir, rel.Replace('/', Path.DirectorySeparatorChar));
                        ExtractEntry(entry, destPath, Path.Combine(opts.Vst3Dir, "Md-101.vst3"));
                    }
                    else if (fullName.StartsWith("App/", StringComparison.OrdinalIgnoreCase))
                    {
                        if (!opts.InstallStandalone) continue;
                        string rel = fullName.Substring("App/".Length);
                        if (string.IsNullOrEmpty(rel)) continue;
                        string destPath = Path.Combine(opts.InstallDir, rel.Replace('/', Path.DirectorySeparatorChar));
                        ExtractEntry(entry, destPath, opts.InstallDir);
                    }

                    int pct = 15 + (int)((processed * 65.0) / Math.Max(1, totalEntries));
                    progress(pt ? ("Extraindo: " + entry.Name) : ("Extracting: " + entry.Name), pct);
                }
            }

            if (opts.InstallStandalone)
            {
                string manifestPath = Path.Combine(opts.InstallDir, "install-manifest.cfg");
                File.WriteAllLines(manifestPath, new string[]
                {
                    "InstallDir=" + opts.InstallDir,
                    "Vst3Dir=" + opts.Vst3Dir,
                    "Lang=" + (opts.Lang == "pt" ? "pt" : "en")
                });

                if (opts.CreateStartMenu)
                {
                    progress(pt ? "Criando atalhos no Menu Iniciar..." : "Creating Start Menu shortcuts...", 85);
                    CreateStartMenuShortcuts(opts);
                }

                if (opts.CreateDesktop)
                {
                    progress(pt ? "Criando atalho na \u00C1rea de Trabalho..." : "Creating Desktop shortcut...", 90);
                    CreateDesktopShortcut(opts);
                }

                RegisterUninstallerInRegistry(opts);
            }

            progress(pt ? "Instala\u00E7\u00E3o conclu\u00EDda com sucesso!" : "Installation completed successfully!", 100);
        }

        private static void VerifySafePreexistingDirectoryOwner(string dirPath)
        {
            if (string.IsNullOrEmpty(dirPath) || !Directory.Exists(dirPath))
                return;

            string fullPath = Path.GetFullPath(dirPath);
            if (Path.GetDirectoryName(fullPath) == null)
                return;

            DirectorySecurity sec = Directory.GetAccessControl(fullPath, AccessControlSections.Owner);
            SecurityIdentifier owner = sec.GetOwner(typeof(SecurityIdentifier)) as SecurityIdentifier;
            if (owner == null)
                throw new UnauthorizedAccessException("Unable to verify owner of existing directory: " + fullPath);

            SecurityIdentifier admins = new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null);
            SecurityIdentifier system = new SecurityIdentifier(WellKnownSidType.LocalSystemSid, null);
            SecurityIdentifier trustedInstaller = new SecurityIdentifier("S-1-5-80-956008885-3418522649-1831038044-1853292631-2271478464");

            bool ownerTrusted = owner.Equals(admins) || owner.Equals(system) || owner.Equals(trustedInstaller);
            if (!ownerTrusted)
            {
                using (WindowsIdentity current = WindowsIdentity.GetCurrent())
                {
                    if (current != null && current.User != null && owner.Equals(current.User))
                        ownerTrusted = true;
                }
            }

            if (!ownerTrusted)
            {
                throw new UnauthorizedAccessException(
                    "Refusing elevated install into pre-existing directory owned by an untrusted account: " + fullPath);
            }
        }

        private static void HardenElevatedDirectoryAcl(string dirPath)
        {
            if (string.IsNullOrEmpty(dirPath) || !Directory.Exists(dirPath))
                return;

            DirectorySecurity sec = new DirectorySecurity();
            sec.SetAccessRuleProtection(true, false);

            InheritanceFlags inherit = InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit;
            sec.AddAccessRule(new FileSystemAccessRule(
                new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null),
                FileSystemRights.FullControl, inherit, PropagationFlags.None, AccessControlType.Allow));
            sec.AddAccessRule(new FileSystemAccessRule(
                new SecurityIdentifier(WellKnownSidType.LocalSystemSid, null),
                FileSystemRights.FullControl, inherit, PropagationFlags.None, AccessControlType.Allow));
            sec.AddAccessRule(new FileSystemAccessRule(
                new SecurityIdentifier(WellKnownSidType.BuiltinUsersSid, null),
                FileSystemRights.ReadAndExecute | FileSystemRights.Synchronize, inherit, PropagationFlags.None, AccessControlType.Allow));

            Directory.SetAccessControl(dirPath, sec);
        }

        private static void ExtractEntry(ZipArchiveEntry entry, string destPath, string baseRootDir)
        {
            string fullBase = Path.GetFullPath(baseRootDir).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
            string fullDest = Path.GetFullPath(destPath);
            if (!fullDest.StartsWith(fullBase, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException("Blocked invalid archive entry path.");
            }

            if (string.IsNullOrEmpty(entry.Name))
            {
                Directory.CreateDirectory(fullDest);
                return;
            }
            string parentDir = Path.GetDirectoryName(fullDest);
            if (!string.IsNullOrEmpty(parentDir))
            {
                Directory.CreateDirectory(parentDir);
            }
            using (Stream inStream = entry.Open())
            using (FileStream outStream = new FileStream(fullDest, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                inStream.CopyTo(outStream);
            }
        }

        private static void CreateShortcut(string lnkPath, string targetPath, string workingDir, string description, string iconPath)
        {
            try
            {
                Type shellType = Type.GetTypeFromProgID("WScript.Shell");
                if (shellType == null) return;
                object shell = Activator.CreateInstance(shellType);
                object shortcut = shellType.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { lnkPath });
                Type scType = shortcut.GetType();
                scType.InvokeMember("TargetPath", BindingFlags.SetProperty, null, shortcut, new object[] { targetPath });
                if (!string.IsNullOrEmpty(workingDir))
                    scType.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, shortcut, new object[] { workingDir });
                if (!string.IsNullOrEmpty(description))
                    scType.InvokeMember("Description", BindingFlags.SetProperty, null, shortcut, new object[] { description });
                if (!string.IsNullOrEmpty(iconPath) && File.Exists(iconPath))
                    scType.InvokeMember("IconLocation", BindingFlags.SetProperty, null, shortcut, new object[] { iconPath });
                scType.InvokeMember("Save", BindingFlags.InvokeMethod, null, shortcut, null);
            }
            catch { }
        }

        private static void CreateStartMenuShortcuts(InstallOptions opts)
        {
            try
            {
                string programsDir = IsAdministrator()
                    ? Environment.GetFolderPath(Environment.SpecialFolder.CommonPrograms)
                    : Environment.GetFolderPath(Environment.SpecialFolder.Programs);
                string groupDir = Path.Combine(programsDir, "Md-101");
                Directory.CreateDirectory(groupDir);

                string exePath = Path.Combine(opts.InstallDir, "Md-101.exe");
                string icoPath = Path.Combine(opts.InstallDir, "md-101.ico");
                string uninstPath = Path.Combine(opts.InstallDir, "Uninstall-Md-101.exe");
                string manualPt = Path.Combine(opts.InstallDir, @"docs\manual.html");
                string manualEn = Path.Combine(opts.InstallDir, @"docs\manual-en.html");

                CreateShortcut(Path.Combine(groupDir, "Md-101.lnk"), exePath, opts.InstallDir,
                    "Md-101 - Periodic Table Polyphonic Synthesizer by LU5", icoPath);

                if (File.Exists(manualPt))
                    CreateShortcut(Path.Combine(groupDir, "Md-101 - Manual (PT).lnk"), manualPt, Path.GetDirectoryName(manualPt), "Manual do Usu\u00E1rio Md-101", null);
                if (File.Exists(manualEn))
                    CreateShortcut(Path.Combine(groupDir, "Md-101 - Manual (EN).lnk"), manualEn, Path.GetDirectoryName(manualEn), "Md-101 User Manual", null);
                if (File.Exists(uninstPath))
                    CreateShortcut(Path.Combine(groupDir, "Uninstall Md-101.lnk"), uninstPath, opts.InstallDir, "Uninstall Md-101", icoPath);
            }
            catch { }
        }

        private static void CreateDesktopShortcut(InstallOptions opts)
        {
            try
            {
                string desktopDir = IsAdministrator()
                    ? Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory)
                    : Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
                string exePath = Path.Combine(opts.InstallDir, "Md-101.exe");
                string icoPath = Path.Combine(opts.InstallDir, "md-101.ico");
                CreateShortcut(Path.Combine(desktopDir, "Md-101.lnk"), exePath, opts.InstallDir,
                    "Md-101 - Periodic Table Polyphonic Synthesizer by LU5", icoPath);
            }
            catch { }
        }

        private static void RegisterUninstallerInRegistry(InstallOptions opts)
        {
            try
            {
                if (!IsAdministrator()) return;
                string keyPath = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\Md-101";
                using (RegistryKey key = Registry.LocalMachine.CreateSubKey(keyPath))
                {
                    if (key == null) return;
                    string uninstExe = Path.Combine(opts.InstallDir, "Uninstall-Md-101.exe");
                    string iconPath = Path.Combine(opts.InstallDir, "md-101.ico");
                    key.SetValue("DisplayName", "Md-101 - Periodic Table Synthesizer (VST3 & Standalone)");
                    key.SetValue("DisplayVersion", "1.1.2");
                    key.SetValue("Publisher", "LU5");
                    key.SetValue("URLInfoAbout", "https://lu5.art.br");
                    key.SetValue("InstallLocation", opts.InstallDir);
                    key.SetValue("Vst3InstallDir", opts.Vst3Dir);
                    key.SetValue("DisplayIcon", File.Exists(iconPath) ? iconPath : Path.Combine(opts.InstallDir, "Md-101.exe"));
                    key.SetValue("UninstallString", "\"" + uninstExe + "\"");
                    key.SetValue("QuietUninstallString", "\"" + uninstExe + "\" /S");
                    key.SetValue("NoModify", 1, RegistryValueKind.DWord);
                    key.SetValue("NoRepair", 1, RegistryValueKind.DWord);
                    key.SetValue("EstimatedSize", 11500, RegistryValueKind.DWord);
                }
            }
            catch { }
        }

        public static void LoadManifestIfPresent(InstallOptions opts)
        {
            try
            {
                // Prefer protected HKLM registry metadata when present
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall\Md-101", false))
                {
                    if (key != null)
                    {
                        string regApp = key.GetValue("InstallLocation") as string;
                        string regVst = key.GetValue("Vst3InstallDir") as string;
                        string normRegApp, normRegVst;
                        if (NormalizeAndValidateDirectory(regApp, out normRegApp))
                            opts.InstallDir = normRegApp;
                        if (NormalizeAndValidateDirectory(regVst, out normRegVst))
                            opts.Vst3Dir = normRegVst;
                        if (!string.IsNullOrEmpty(regApp))
                            return;
                    }
                }

                // Only read install-manifest.cfg from an actual installed app directory containing Md-101.exe
                string exeDir = Path.GetDirectoryName(Application.ExecutablePath) ?? "";
                string manifest = null;
                if (!string.IsNullOrEmpty(exeDir) &&
                    File.Exists(Path.Combine(exeDir, "Md-101.exe")) &&
                    File.Exists(Path.Combine(exeDir, "install-manifest.cfg")))
                {
                    manifest = Path.Combine(exeDir, "install-manifest.cfg");
                }
                else if (File.Exists(Path.Combine(opts.InstallDir, "Md-101.exe")) &&
                         File.Exists(Path.Combine(opts.InstallDir, "install-manifest.cfg")))
                {
                    manifest = Path.Combine(opts.InstallDir, "install-manifest.cfg");
                }

                if (manifest != null && !IsReparsePoint(manifest))
                {
                    foreach (string line in File.ReadAllLines(manifest))
                    {
                        if (line.StartsWith("InstallDir="))
                        {
                            string candidate = line.Substring("InstallDir=".Length).Trim();
                            string norm;
                            if (NormalizeAndValidateDirectory(candidate, out norm))
                                opts.InstallDir = norm;
                        }
                        else if (line.StartsWith("Vst3Dir="))
                        {
                            string candidate = line.Substring("Vst3Dir=".Length).Trim();
                            string norm;
                            if (NormalizeAndValidateDirectory(candidate, out norm))
                                opts.Vst3Dir = norm;
                        }
                    }
                }
            }
            catch { }
        }

        public static void PerformUninstall(InstallOptions opts, Action<string, int> progress)
        {
            bool pt = opts.Lang == "pt";
            LoadManifestIfPresent(opts);

            progress(pt ? "Removendo plugin VST3..." : "Removing VST3 plugin...", 20);
            string normVst;
            if (NormalizeAndValidateDirectory(opts.Vst3Dir, out normVst))
            {
                string vst3Bundle = Path.Combine(normVst, "Md-101.vst3");
                if (Directory.Exists(vst3Bundle))
                {
                    try { SafeDeleteDirectory(vst3Bundle); } catch { }
                }
            }

            progress(pt ? "Removendo atalhos e registros..." : "Removing shortcuts and registry entries...", 50);
            try
            {
                string commonGroup = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonPrograms), "Md-101");
                if (Directory.Exists(commonGroup)) SafeDeleteDirectory(commonGroup);
            }
            catch { }
            try
            {
                string userGroup = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), "Md-101");
                if (Directory.Exists(userGroup)) SafeDeleteDirectory(userGroup);
            }
            catch { }
            try
            {
                string commonDesk = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory), "Md-101.lnk");
                if (File.Exists(commonDesk)) File.Delete(commonDesk);
            }
            catch { }
            try
            {
                string userDesk = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "Md-101.lnk");
                if (File.Exists(userDesk)) File.Delete(userDesk);
            }
            catch { }
            try
            {
                if (IsAdministrator())
                {
                    Registry.LocalMachine.DeleteSubKeyTree(@"Software\Microsoft\Windows\CurrentVersion\Uninstall\Md-101", false);
                }
            }
            catch { }

            progress(pt ? "Removendo arquivos do aplicativo..." : "Removing application files...", 80);
            if (IsSafeToDeleteInstallDir(opts.InstallDir))
            {
                string normInstallDir;
                NormalizeAndValidateDirectory(opts.InstallDir, out normInstallDir);
                string currentExe = Path.GetFullPath(Application.ExecutablePath);
                DeleteDirectoryContentsExceptSelf(normInstallDir, currentExe);

                try
                {
                    SafeDeleteDirectory(normInstallDir);
                }
                catch
                {
                    // Schedule locked self-executable & now-empty directory cleanup without cmd.exe /C
                    try { MoveFileExW(currentExe, null, MOVEFILE_DELAY_UNTIL_REBOOT); } catch { }
                    try { MoveFileExW(normInstallDir, null, MOVEFILE_DELAY_UNTIL_REBOOT); } catch { }

                    try
                    {
                        string psExe = Path.Combine(Environment.SystemDirectory, @"WindowsPowerShell\v1.0\powershell.exe");
                        if (File.Exists(psExe))
                        {
                            string escFile = currentExe.Replace("'", "''");
                            string escDir = normInstallDir.Replace("'", "''");
                            string script =
                                "Start-Sleep -Seconds 2; " +
                                "if (Test-Path -LiteralPath '" + escFile + "') { Remove-Item -LiteralPath '" + escFile + "' -Force -ErrorAction SilentlyContinue }; " +
                                "if ((Test-Path -LiteralPath '" + escDir + "') -and ((Get-ChildItem -LiteralPath '" + escDir + "' -Force -ErrorAction SilentlyContinue).Count -eq 0)) { Remove-Item -LiteralPath '" + escDir + "' -Force -ErrorAction SilentlyContinue }";
                            string encoded = Convert.ToBase64String(Encoding.Unicode.GetBytes(script));

                            ProcessStartInfo psi = new ProcessStartInfo(psExe,
                                "-NoProfile -NonInteractive -WindowStyle Hidden -EncodedCommand " + encoded);
                            psi.WindowStyle = ProcessWindowStyle.Hidden;
                            psi.CreateNoWindow = true;
                            psi.UseShellExecute = false;
                            Process.Start(psi);
                        }
                    }
                    catch { }
                }
            }

            progress(pt ? "Md-101 removido com sucesso!" : "Md-101 uninstalled successfully!", 100);
        }
    }

    internal sealed class ElementBadgeControl : Control
    {
        private readonly Font _numFont = new Font("Segoe UI", 11f, FontStyle.Bold, GraphicsUnit.Pixel);
        private readonly Font _symFont = new Font("Segoe UI", 26f, FontStyle.Bold, GraphicsUnit.Pixel);

        public ElementBadgeControl()
        {
            this.SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.OptimizedDoubleBuffer, true);
            this.Size = new Size(64, 64);
            this.BackColor = Color.FromArgb(14, 18, 26);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;

            Rectangle borderRect = new Rectangle(1, 1, this.Width - 3, this.Height - 3);
            using (Pen borderPen = new Pen(Color.FromArgb(220, 173, 215), 2f))
            {
                g.DrawRectangle(borderPen, borderRect);
            }
            using (SolidBrush ledBrush = new SolidBrush(Color.FromArgb(163, 210, 70)))
            {
                g.FillEllipse(ledBrush, this.Width - 13, 6, 7, 7);
            }

            TextRenderer.DrawText(g, "101", _numFont, new Point(5, 4), Color.FromArgb(114, 203, 235));
            TextRenderer.DrawText(g, "Md", _symFont, new Rectangle(0, 18, this.Width, 42), Color.White,
                TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                _numFont.Dispose();
                _symFont.Dispose();
            }
            base.Dispose(disposing);
        }
    }

    internal sealed class SetupForm : Form
    {
        private readonly InstallOptions _opts;
        private readonly Font _fontRegular = new Font("Segoe UI", 13f, FontStyle.Regular, GraphicsUnit.Pixel);
        private readonly Font _fontBold    = new Font("Segoe UI", 13.5f, FontStyle.Bold, GraphicsUnit.Pixel);
        private readonly Font _fontTitle   = new Font("Segoe UI", 20f, FontStyle.Bold, GraphicsUnit.Pixel);

        private Button _btnLang;
        private Label _lblTitle;
        private Label _lblSubtitle;
        private Label _lblComponentsHeader;
        private CheckBox _chkVst3;
        private CheckBox _chkStandalone;
        private CheckBox _chkStartMenu;
        private CheckBox _chkDesktop;
        private Label _lblDirTitle;
        private TextBox _txtInstallDir;
        private Button _btnBrowse;
        private Label _lblVst3PathInfo;
        private ProgressBar _progressBar;
        private Label _lblStatus;
        private CheckBox _chkLaunchAfter;
        private Button _btnInstall;
        private Button _btnCancel;
        private bool _installFinished = false;

        public SetupForm(InstallOptions opts)
        {
            _opts = opts;
            InitializeUi();
            ApplyLocalization();

            if (_opts.ElevatedAutoRun)
            {
                this.Shown += delegate { StartInstallation(); };
            }
        }

        private void InitializeUi()
        {
            this.AutoScaleMode = AutoScaleMode.None;
            this.Font = _fontRegular;
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.StartPosition = FormStartPosition.CenterScreen;
            this.Text = "Md-101 v1.1.2 \u2014 Setup (LU5)";
            this.BackColor = Color.FromArgb(13, 17, 23);
            this.ForeColor = Color.FromArgb(230, 237, 243);
            this.ClientSize = new Size(640, 480);
            try { this.Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

            // Header Panel
            Panel header = new Panel();
            header.Location = new Point(0, 0);
            header.Size = new Size(640, 92);
            header.BackColor = Color.FromArgb(20, 26, 36);
            this.Controls.Add(header);

            Panel divider = new Panel();
            divider.Location = new Point(0, 91);
            divider.Size = new Size(640, 1);
            divider.BackColor = Color.FromArgb(45, 58, 76);
            header.Controls.Add(divider);

            ElementBadgeControl badge = new ElementBadgeControl();
            badge.Location = new Point(20, 14);
            header.Controls.Add(badge);

            _lblTitle = new Label();
            _lblTitle.Location = new Point(98, 16);
            _lblTitle.Size = new Size(420, 30);
            _lblTitle.Font = _fontTitle;
            _lblTitle.ForeColor = Color.White;
            _lblTitle.BackColor = Color.Transparent;
            header.Controls.Add(_lblTitle);

            _lblSubtitle = new Label();
            _lblSubtitle.Location = new Point(100, 50);
            _lblSubtitle.Size = new Size(430, 24);
            _lblSubtitle.Font = _fontRegular;
            _lblSubtitle.ForeColor = Color.FromArgb(163, 199, 215);
            _lblSubtitle.BackColor = Color.Transparent;
            _lblSubtitle.UseMnemonic = false;
            header.Controls.Add(_lblSubtitle);

            _btnLang = new Button();
            _btnLang.Text = "PT | EN";
            _btnLang.Size = new Size(80, 30);
            _btnLang.Location = new Point(536, 16);
            _btnLang.FlatStyle = FlatStyle.Flat;
            _btnLang.BackColor = Color.FromArgb(30, 39, 54);
            _btnLang.ForeColor = Color.FromArgb(114, 203, 235);
            _btnLang.FlatAppearance.BorderColor = Color.FromArgb(61, 158, 229);
            _btnLang.Cursor = Cursors.Hand;
            _btnLang.Click += delegate
            {
                _opts.Lang = (_opts.Lang == "pt") ? "en" : "pt";
                ApplyLocalization();
            };
            header.Controls.Add(_btnLang);

            // Components Section
            _lblComponentsHeader = new Label();
            _lblComponentsHeader.Location = new Point(24, 108);
            _lblComponentsHeader.Size = new Size(592, 24);
            _lblComponentsHeader.Font = _fontBold;
            _lblComponentsHeader.ForeColor = Color.FromArgb(220, 173, 215);
            this.Controls.Add(_lblComponentsHeader);

            _chkVst3       = CreateCheckBox(28, 138, _opts.InstallVst3);
            _chkStandalone = CreateCheckBox(28, 168, _opts.InstallStandalone);
            _chkStartMenu  = CreateCheckBox(28, 198, _opts.CreateStartMenu);
            _chkDesktop    = CreateCheckBox(28, 228, _opts.CreateDesktop);

            _chkStandalone.CheckedChanged += delegate
            {
                bool en = _chkStandalone.Checked;
                _chkStartMenu.Enabled = en;
                _chkDesktop.Enabled = en;
                _txtInstallDir.Enabled = en;
                _btnBrowse.Enabled = en;
            };

            // Directory Section
            _lblDirTitle = new Label();
            _lblDirTitle.Location = new Point(24, 268);
            _lblDirTitle.Size = new Size(592, 22);
            _lblDirTitle.Font = _fontBold;
            _lblDirTitle.ForeColor = Color.FromArgb(114, 203, 235);
            this.Controls.Add(_lblDirTitle);

            _txtInstallDir = new TextBox();
            _txtInstallDir.Location = new Point(24, 294);
            _txtInstallDir.Size = new Size(494, 26);
            _txtInstallDir.BackColor = Color.FromArgb(22, 27, 34);
            _txtInstallDir.ForeColor = Color.White;
            _txtInstallDir.BorderStyle = BorderStyle.FixedSingle;
            _txtInstallDir.Text = _opts.InstallDir;
            this.Controls.Add(_txtInstallDir);

            _btnBrowse = new Button();
            _btnBrowse.Location = new Point(526, 292);
            _btnBrowse.Size = new Size(90, 28);
            _btnBrowse.FlatStyle = FlatStyle.Flat;
            _btnBrowse.BackColor = Color.FromArgb(30, 39, 54);
            _btnBrowse.ForeColor = Color.White;
            _btnBrowse.FlatAppearance.BorderColor = Color.FromArgb(80, 100, 120);
            _btnBrowse.Click += OnBrowseClick;
            this.Controls.Add(_btnBrowse);

            _lblVst3PathInfo = new Label();
            _lblVst3PathInfo.Location = new Point(24, 326);
            _lblVst3PathInfo.Size = new Size(592, 22);
            _lblVst3PathInfo.ForeColor = Color.FromArgb(140, 160, 180);
            this.Controls.Add(_lblVst3PathInfo);

            // Progress Section
            _progressBar = new ProgressBar();
            _progressBar.Location = new Point(24, 356);
            _progressBar.Size = new Size(592, 22);
            _progressBar.Minimum = 0;
            _progressBar.Maximum = 100;
            this.Controls.Add(_progressBar);

            _lblStatus = new Label();
            _lblStatus.Location = new Point(24, 384);
            _lblStatus.Size = new Size(592, 24);
            _lblStatus.ForeColor = Color.FromArgb(163, 210, 70);
            this.Controls.Add(_lblStatus);

            _chkLaunchAfter = CreateCheckBox(24, 424, true);
            _chkLaunchAfter.Width = 340;
            _chkLaunchAfter.Visible = false;

            // Bottom Action Buttons
            _btnInstall = new Button();
            _btnInstall.Location = new Point(380, 420);
            _btnInstall.Size = new Size(130, 38);
            _btnInstall.FlatStyle = FlatStyle.Flat;
            _btnInstall.BackColor = Color.FromArgb(61, 158, 229);
            _btnInstall.ForeColor = Color.White;
            _btnInstall.Font = _fontBold;
            _btnInstall.FlatAppearance.BorderSize = 0;
            _btnInstall.Cursor = Cursors.Hand;
            _btnInstall.Click += delegate { OnInstallOrFinishClick(); };
            this.Controls.Add(_btnInstall);

            _btnCancel = new Button();
            _btnCancel.Location = new Point(518, 420);
            _btnCancel.Size = new Size(98, 38);
            _btnCancel.FlatStyle = FlatStyle.Flat;
            _btnCancel.BackColor = Color.FromArgb(30, 39, 54);
            _btnCancel.ForeColor = Color.FromArgb(200, 210, 220);
            _btnCancel.FlatAppearance.BorderColor = Color.FromArgb(70, 80, 95);
            _btnCancel.Cursor = Cursors.Hand;
            _btnCancel.Click += delegate { this.Close(); };
            this.Controls.Add(_btnCancel);
        }

        private CheckBox CreateCheckBox(int x, int y, bool checkedState)
        {
            CheckBox chk = new CheckBox();
            chk.Location = new Point(x, y);
            chk.Size = new Size(588, 24);
            chk.Checked = checkedState;
            chk.ForeColor = Color.FromArgb(230, 237, 243);
            this.Controls.Add(chk);
            return chk;
        }

        private void ApplyLocalization()
        {
            bool pt = _opts.Lang == "pt";
            _lblTitle.Text = pt
                ? "Md-101 v1.1.2 \u2014 Instalador"
                : "Md-101 v1.1.2 \u2014 Setup Wizard";
            _lblSubtitle.Text = pt
                ? "Sintetizador Polif\u00F4nico VST3 & Standalone da Tabela Peri\u00F3dica \u2022 LU5"
                : "Periodic Table Polyphonic VST3 & Standalone Synthesizer \u2022 LU5";
            _lblComponentsHeader.Text = pt
                ? "Selecione os componentes para instalar:"
                : "Select components to install:";
            _chkVst3.Text = pt
                ? "Plugin VST3 (64-bit) para DAWs (Reaper, Ableton, FL Studio, Bitwig...)"
                : "VST3 Plugin (64-bit) for DAWs (Reaper, Ableton, FL Studio, Bitwig...)";
            _chkStandalone.Text = pt
                ? "Aplicativo Independente (Standalone Md-101.exe) + Manuais (HTML/MD)"
                : "Standalone Desktop Application (Md-101.exe) + User Manuals (HTML/MD)";
            _chkStartMenu.Text = pt
                ? "Criar atalhos no Menu Iniciar do Windows"
                : "Create Windows Start Menu shortcuts";
            _chkDesktop.Text = pt
                ? "Criar atalho na \u00C1rea de Trabalho (Desktop)"
                : "Create Desktop shortcut";
            _lblDirTitle.Text = pt
                ? "Pasta de instala\u00E7\u00E3o do aplicativo Standalone:"
                : "Standalone application destination folder:";
            _btnBrowse.Text = pt ? "Procurar..." : "Browse...";
            _lblVst3PathInfo.Text = (pt ? "Pasta VST3 do sistema: " : "System VST3 folder: ") + Path.Combine(_opts.Vst3Dir, "Md-101.vst3");
            _chkLaunchAfter.Text = pt
                ? "Iniciar o sintetizador Md-101 agora"
                : "Launch Md-101 synthesizer now";

            if (_installFinished)
            {
                _btnInstall.Text = pt ? "Concluir" : "Finish";
                _lblStatus.Text = pt ? "\u2714 Md-101 instalado com sucesso!" : "\u2714 Md-101 installed successfully!";
            }
            else
            {
                _btnInstall.Text = pt ? "Instalar" : "Install";
                _btnCancel.Text = pt ? "Cancelar" : "Cancel";
                _lblStatus.Text = pt ? "Pronto para instalar." : "Ready to install.";
            }
        }

        private void OnBrowseClick(object sender, EventArgs e)
        {
            using (FolderBrowserDialog fbd = new FolderBrowserDialog())
            {
                fbd.SelectedPath = _txtInstallDir.Text;
                if (fbd.ShowDialog(this) == DialogResult.OK)
                {
                    _txtInstallDir.Text = Path.Combine(fbd.SelectedPath, "Md-101");
                }
            }
        }

        private void OnInstallOrFinishClick()
        {
            if (_installFinished)
            {
                if (_chkLaunchAfter.Visible && _chkLaunchAfter.Checked)
                {
                    string exePath = Path.Combine(_opts.InstallDir, "Md-101.exe");
                    InstallerEngine.LaunchAsDesktopUser(exePath);
                }
                this.Close();
                return;
            }
            StartInstallation();
        }

        private void StartInstallation()
        {
            _opts.InstallVst3 = _chkVst3.Checked;
            _opts.InstallStandalone = _chkStandalone.Checked;
            _opts.CreateStartMenu = _chkStartMenu.Checked;
            _opts.CreateDesktop = _chkDesktop.Checked;
            _opts.InstallDir = _txtInstallDir.Text.Trim();

            if (!_opts.InstallVst3 && !_opts.InstallStandalone)
            {
                MessageBox.Show(this,
                    _opts.Lang == "pt" ? "Selecione pelo menos um componente." : "Please select at least one component.",
                    "Md-101", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            string normApp;
            if (_opts.InstallStandalone)
            {
                if (!InstallerEngine.NormalizeAndValidateDirectory(_opts.InstallDir, out normApp))
                {
                    MessageBox.Show(this,
                        _opts.Lang == "pt" ? "Caminho de pasta de instala\u00E7\u00E3o inv\u00E1lido ou protegido." : "Invalid or protected installation directory path.",
                        "Md-101", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
                _opts.InstallDir = normApp;
            }

            string normVst;
            if (_opts.InstallVst3)
            {
                if (!InstallerEngine.NormalizeAndValidateDirectory(_opts.Vst3Dir, out normVst))
                {
                    MessageBox.Show(this,
                        _opts.Lang == "pt" ? "Caminho de pasta VST3 inv\u00E1lido ou protegido." : "Invalid or protected VST3 directory path.",
                        "Md-101", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
                _opts.Vst3Dir = normVst;
            }

            if (InstallerEngine.RequiresElevation(_opts) && !InstallerEngine.IsAdministrator())
            {
                try
                {
                    this.Hide();
                    InstallerEngine.RelaunchElevated(_opts, false);
                    this.Close();
                    return;
                }
                catch
                {
                    this.Show();
                    return;
                }
            }

            _btnInstall.Enabled = false;
            _btnCancel.Enabled = false;
            _chkVst3.Enabled = false;
            _chkStandalone.Enabled = false;
            _chkStartMenu.Enabled = false;
            _chkDesktop.Enabled = false;
            _txtInstallDir.Enabled = false;
            _btnBrowse.Enabled = false;

            Thread worker = new Thread(delegate()
            {
                try
                {
                    InstallerEngine.PerformInstall(_opts, delegate(string msg, int pct)
                    {
                        this.BeginInvoke((MethodInvoker)delegate
                        {
                            _progressBar.Value = Math.Min(100, Math.Max(0, pct));
                            _lblStatus.Text = msg;
                        });
                    });

                    this.BeginInvoke((MethodInvoker)delegate
                    {
                        _installFinished = true;
                        _btnInstall.Enabled = true;
                        _btnInstall.BackColor = Color.FromArgb(163, 210, 70);
                        _btnInstall.ForeColor = Color.FromArgb(13, 17, 23);
                        _btnInstall.Text = (_opts.Lang == "pt") ? "Concluir" : "Finish";
                        if (_opts.InstallStandalone)
                        {
                            _chkLaunchAfter.Visible = true;
                        }
                    });
                }
                catch (Exception ex)
                {
                    this.BeginInvoke((MethodInvoker)delegate
                    {
                        _btnInstall.Enabled = true;
                        _btnCancel.Enabled = true;
                        MessageBox.Show(this, ex.Message, "Md-101 Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    });
                }
            });
            worker.IsBackground = true;
            worker.Start();
        }
    }

    internal sealed class UninstallForm : Form
    {
        private readonly InstallOptions _opts;
        private readonly Font _fontRegular = new Font("Segoe UI", 13f, FontStyle.Regular, GraphicsUnit.Pixel);
        private readonly Font _fontBold    = new Font("Segoe UI", 14f, FontStyle.Bold, GraphicsUnit.Pixel);

        private Label _lblPrompt;
        private ProgressBar _progressBar;
        private Label _lblStatus;
        private Button _btnUninstall;
        private Button _btnCancel;
        private bool _done = false;

        public UninstallForm(InstallOptions opts)
        {
            _opts = opts;
            InstallerEngine.LoadManifestIfPresent(_opts);
            InitializeUi();

            if (_opts.ElevatedAutoRun)
            {
                this.Shown += delegate { StartUninstall(); };
            }
        }

        private void InitializeUi()
        {
            bool pt = _opts.Lang == "pt";
            this.AutoScaleMode = AutoScaleMode.None;
            this.Font = _fontRegular;
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.StartPosition = FormStartPosition.CenterScreen;
            this.Text = pt ? "Desinstalar Md-101" : "Uninstall Md-101";
            this.BackColor = Color.FromArgb(13, 17, 23);
            this.ForeColor = Color.FromArgb(230, 237, 243);
            this.ClientSize = new Size(490, 235);
            try { this.Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

            _lblPrompt = new Label();
            _lblPrompt.Location = new Point(24, 24);
            _lblPrompt.Size = new Size(442, 54);
            _lblPrompt.Font = _fontBold;
            _lblPrompt.Text = pt
                ? "Deseja remover completamente o sintetizador Md-101 (VST3 e Standalone) do seu computador?"
                : "Do you want to completely remove the Md-101 synthesizer (VST3 and Standalone) from your computer?";
            this.Controls.Add(_lblPrompt);

            _progressBar = new ProgressBar();
            _progressBar.Location = new Point(24, 92);
            _progressBar.Size = new Size(442, 22);
            this.Controls.Add(_progressBar);

            _lblStatus = new Label();
            _lblStatus.Location = new Point(24, 122);
            _lblStatus.Size = new Size(442, 24);
            _lblStatus.ForeColor = Color.FromArgb(114, 203, 235);
            this.Controls.Add(_lblStatus);

            _btnUninstall = new Button();
            _btnUninstall.Location = new Point(242, 170);
            _btnUninstall.Size = new Size(120, 36);
            _btnUninstall.FlatStyle = FlatStyle.Flat;
            _btnUninstall.BackColor = Color.FromArgb(220, 80, 80);
            _btnUninstall.ForeColor = Color.White;
            _btnUninstall.Font = _fontBold;
            _btnUninstall.FlatAppearance.BorderSize = 0;
            _btnUninstall.Text = pt ? "Desinstalar" : "Uninstall";
            _btnUninstall.Click += delegate
            {
                if (_done) { this.Close(); return; }
                StartUninstall();
            };
            this.Controls.Add(_btnUninstall);

            _btnCancel = new Button();
            _btnCancel.Location = new Point(370, 170);
            _btnCancel.Size = new Size(96, 36);
            _btnCancel.FlatStyle = FlatStyle.Flat;
            _btnCancel.BackColor = Color.FromArgb(30, 39, 54);
            _btnCancel.ForeColor = Color.White;
            _btnCancel.Text = pt ? "Cancelar" : "Cancel";
            _btnCancel.Click += delegate { this.Close(); };
            this.Controls.Add(_btnCancel);
        }

        private void StartUninstall()
        {
            string normApp, normVst;
            if (!InstallerEngine.NormalizeAndValidateDirectory(_opts.InstallDir, out normApp) ||
                !InstallerEngine.NormalizeAndValidateDirectory(_opts.Vst3Dir, out normVst))
            {
                MessageBox.Show(this,
                    _opts.Lang == "pt" ? "Caminho de instala\u00E7\u00E3o inv\u00E1lido ou protegido." : "Invalid or protected installation directory path.",
                    "Md-101", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            _opts.InstallDir = normApp;
            _opts.Vst3Dir = normVst;

            if (InstallerEngine.RequiresElevation(_opts) && !InstallerEngine.IsAdministrator())
            {
                try
                {
                    this.Hide();
                    InstallerEngine.RelaunchElevated(_opts, false);
                    this.Close();
                    return;
                }
                catch
                {
                    this.Show();
                    return;
                }
            }

            _btnUninstall.Enabled = false;
            _btnCancel.Enabled = false;

            Thread worker = new Thread(delegate()
            {
                InstallerEngine.PerformUninstall(_opts, delegate(string msg, int pct)
                {
                    this.BeginInvoke((MethodInvoker)delegate
                    {
                        _progressBar.Value = Math.Min(100, Math.Max(0, pct));
                        _lblStatus.Text = msg;
                    });
                });

                this.BeginInvoke((MethodInvoker)delegate
                {
                    _done = true;
                    _btnUninstall.Enabled = true;
                    _btnUninstall.BackColor = Color.FromArgb(61, 158, 229);
                    _btnUninstall.Text = (_opts.Lang == "pt") ? "Fechar" : "Close";
                });
            });
            worker.IsBackground = true;
            worker.Start();
        }
    }
}
