using System.Windows.Media;

namespace VexHub.Loader.Core;

public static class Icons
{
    public static Geometry Sync { get; } = SvgPath.Parse(
        "M20 12a8 8 0 0 1-13.7 5.6M4 12a8 8 0 0 1 13.7-5.6M20 5.5V11h-5.5M4 18.5V13h5.5");

    public static Geometry Shield { get; } = SvgPath.Parse(
        "M12 3 5 6v5.2c0 4.6 3 7.9 7 9.8 4-1.9 7-5.2 7-9.8V6l-7-3Z" +
        "m9 11.8 2 2 4-4.2");

    public static Geometry Target { get; } = SvgPath.Parse(
        "M12 4.5a7.5 7.5 0 1 0 0 15a7.5 7.5 0 1 0 0-15" +
        "M12 9.6a2.4 2.4 0 1 0 0 4.8a2.4 2.4 0 1 0 0-4.8" +
        "M12 2.4v4M12 17.6v4M2.4 12h4M17.6 12h4");

    public static Geometry Plug { get; } = SvgPath.Parse(
        "M9 3.2v5M15 3.2v5M6 8.2h12v3.1a6 6 0 0 1-12 0V8.2ZM12 17.3v3.5");

    public static Geometry Play { get; } = SvgPath.Parse(
        "M9 6.4 18 12l-9 5.6V6.4Z");

    public static Geometry Power { get; } = SvgPath.Parse(
        "M12 3v9M7.3 6.4a7.2 7.2 0 1 0 9.4 0");

    public static Geometry Check { get; } = SvgPath.Parse("m5 12.6 4.4 4.4L19 7.4");

    public static Geometry Alert { get; } = SvgPath.Parse(
        "M12 4 3.2 20h17.6L12 4ZM12 9.8v4.6M12 17.6v.2");

    public static Geometry Minus { get; } = SvgPath.Parse("M5 12h14");

    public static Geometry Close { get; } = SvgPath.Parse("m6 6 12 12M18 6 6 18");

    public static Geometry Globe { get; } = SvgPath.Parse(
        "M12 3.5a8.5 8.5 0 1 0 0 17a8.5 8.5 0 1 0 0-17" +
        "M3.5 12h17" +
        "M12 3.5c2.4 2.4 3.7 5.3 3.7 8.5s-1.3 6.1-3.7 8.5c-2.4-2.4-3.7-5.3-3.7-8.5s1.3-6.1 3.7-8.5Z");

    public static Geometry Code { get; } = SvgPath.Parse(
        "m8 9-3 3 3 3m8-6 3 3-3 3m-2-9-4 12");

    public static Geometry Book { get; } = SvgPath.Parse(
        "M4 5.6A2.6 2.6 0 0 1 6.6 3H20v18H6.6A2.6 2.6 0 0 1 4 18.4V5.6Z" +
        "M8 7.2h8M8 11h5.5");

    public static Geometry Box { get; } = SvgPath.Parse(
        "M12 3 4.5 7v10L12 21l7.5-4V7L12 3Z" +
        "M12 3v18M4.5 7l7.5 4 7.5-4");

    public static Geometry Logo { get; } = SvgPath.Parse("M6.2 6.4 10 13.6l3.8-7.2");
}
