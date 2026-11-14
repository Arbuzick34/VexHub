using System.Windows;

namespace VexHub.Loader;

public partial class App : Application
{
    public static string? ShotPath { get; private set; }
    public static string? ShotState { get; private set; }
    public static string? LanguageOverride { get; private set; }

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        for (var i = 0; i < e.Args.Length; i++)
        {
            switch (e.Args[i])
            {
                case "--shot" when i + 1 < e.Args.Length:
                    ShotPath = e.Args[++i];
                    break;
                case "--state" when i + 1 < e.Args.Length:
                    ShotState = e.Args[++i];
                    break;
                case "--lang" when i + 1 < e.Args.Length:
                    LanguageOverride = e.Args[++i];
                    break;
            }
        }
    }
}
