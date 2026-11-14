using System.Diagnostics;
using System.IO;

namespace VexHub.Loader.Core;

public static class VexHubClient
{
    public static string? Resolve(LoaderSettings settings)
    {
        var candidates = new[]
        {
            settings.VexHubPath,
            Path.Combine(AppContext.BaseDirectory, "vexhub.exe"),
            Path.Combine(Environment.CurrentDirectory, "vexhub.exe"),
            Path.Combine(Path.GetTempPath(), "vexhub", "vexhub.exe"),
        };

        foreach (var candidate in candidates)
        {
            if (!string.IsNullOrWhiteSpace(candidate) && File.Exists(candidate)) return candidate;
        }

        return null;
    }

    public static Process? FindRunning() => Process.GetProcessesByName("vexhub").FirstOrDefault();

    public static Process Start(string path)
    {
        var startInfo = new ProcessStartInfo(path)
        {
            UseShellExecute = true,
            WorkingDirectory = Path.GetDirectoryName(path) ?? AppContext.BaseDirectory,
        };

        return Process.Start(startInfo) ?? throw new InvalidOperationException("Process.Start returned null.");
    }
}

public static class GameClient
{
    public static Process? Find() => Process.GetProcessesByName("cs2").FirstOrDefault();
}
