using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace VexHub.Loader.Core;

public sealed class LoaderSettings
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
    };

    public string Language { get; set; } = "en";
    public string? VexHubPath { get; set; }

    public static string Folder => Path.Combine(Path.GetTempPath(), "vexhub");
    public static string FilePath => Path.Combine(Folder, "loader.json");

    public static LoaderSettings Load()
    {
        try
        {
            if (File.Exists(FilePath))
            {
                var loaded = JsonSerializer.Deserialize<LoaderSettings>(File.ReadAllText(FilePath), JsonOptions);
                if (loaded is not null) return loaded;
            }
        }
        catch
        {
            // A broken settings file must not block the loader.
        }

        var language = System.Globalization.CultureInfo.CurrentUICulture.TwoLetterISOLanguageName;
        return new LoaderSettings { Language = language == "ru" ? "ru" : "en" };
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Folder);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(this, JsonOptions));
        }
        catch
        {
            // Persisting settings is best-effort.
        }
    }
}
