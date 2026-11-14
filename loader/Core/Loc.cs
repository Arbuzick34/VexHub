using System.Windows;

namespace VexHub.Loader.Core;

public static class Loc
{
    public static string Get(string key) => Application.Current?.TryFindResource(key) as string ?? key;

    public static void Apply(string language)
    {
        var dictionary = new ResourceDictionary
        {
            Source = new Uri($"pack://application:,,,/Resources/Lang.{language}.xaml", UriKind.Absolute),
        };

        var merged = Application.Current.Resources.MergedDictionaries;
        if (merged.Count == 0) merged.Add(dictionary);
        else merged[0] = dictionary;
    }
}
