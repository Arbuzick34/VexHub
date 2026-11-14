using System.Globalization;
using System.Windows;
using System.Windows.Media;

namespace VexHub.Loader.Core;

/// <summary>
/// Parses SVG path data (the same `d` strings the website uses) into WPF geometry.
/// Supports M/L/H/V/C/S/Q/T/A/Z in absolute and relative form.
/// </summary>
public static class SvgPath
{
    public static Geometry Parse(string data)
    {
        var geometry = new PathGeometry { FillRule = FillRule.Nonzero };
        var reader = new Reader(data);

        PathFigure? figure = null;
        var current = new Point(0, 0);
        var start = current;
        var lastCubic = current;
        var lastQuad = current;
        var afterCubic = false;
        var afterQuad = false;

        while (true)
        {
            reader.SkipSeparators();
            if (!reader.HasMore) break;

            if (reader.IsLetter)
            {
                reader.Command = reader.Read();
            }
            else if (reader.Command == '\0')
            {
                break;
            }

            var op = reader.Command;
            var relative = char.IsLower(op);
            var kind = char.ToUpperInvariant(op);
            var previousCubic = afterCubic;
            var previousQuad = afterQuad;
            afterCubic = false;
            afterQuad = false;

            switch (kind)
            {
                case 'M':
                {
                    var point = reader.ReadPoint();
                    if (relative) point = Offset(current, point);
                    if (figure is not null) geometry.Figures.Add(figure);
                    figure = new PathFigure { StartPoint = point, IsFilled = true };
                    start = point;
                    current = point;
                    reader.Command = relative ? 'l' : 'L';
                    break;
                }
                case 'L':
                {
                    var point = reader.ReadPoint();
                    if (relative) point = Offset(current, point);
                    Figure(ref figure, current).Segments.Add(new LineSegment(point, true));
                    current = point;
                    break;
                }
                case 'H':
                {
                    var x = reader.ReadNumber();
                    if (relative) x += current.X;
                    var point = new Point(x, current.Y);
                    Figure(ref figure, current).Segments.Add(new LineSegment(point, true));
                    current = point;
                    break;
                }
                case 'V':
                {
                    var y = reader.ReadNumber();
                    if (relative) y += current.Y;
                    var point = new Point(current.X, y);
                    Figure(ref figure, current).Segments.Add(new LineSegment(point, true));
                    current = point;
                    break;
                }
                case 'C':
                {
                    var control1 = reader.ReadPoint();
                    var control2 = reader.ReadPoint();
                    var point = reader.ReadPoint();
                    if (relative)
                    {
                        control1 = Offset(current, control1);
                        control2 = Offset(current, control2);
                        point = Offset(current, point);
                    }
                    Figure(ref figure, start).Segments.Add(new BezierSegment(control1, control2, point, true));
                    lastCubic = control2;
                    afterCubic = true;
                    current = point;
                    break;
                }
                case 'S':
                {
                    var control2 = reader.ReadPoint();
                    var point = reader.ReadPoint();
                    if (relative)
                    {
                        control2 = Offset(current, control2);
                        point = Offset(current, point);
                    }
                    var control1 = previousCubic ? Reflect(lastCubic, current) : current;
                    Figure(ref figure, start).Segments.Add(new BezierSegment(control1, control2, point, true));
                    lastCubic = control2;
                    afterCubic = true;
                    current = point;
                    break;
                }
                case 'Q':
                {
                    var control = reader.ReadPoint();
                    var point = reader.ReadPoint();
                    if (relative)
                    {
                        control = Offset(current, control);
                        point = Offset(current, point);
                    }
                    Figure(ref figure, start).Segments.Add(new QuadraticBezierSegment(control, point, true));
                    lastQuad = control;
                    afterQuad = true;
                    current = point;
                    break;
                }
                case 'T':
                {
                    var point = reader.ReadPoint();
                    if (relative) point = Offset(current, point);
                    var control = previousQuad ? Reflect(lastQuad, current) : current;
                    Figure(ref figure, start).Segments.Add(new QuadraticBezierSegment(control, point, true));
                    lastQuad = control;
                    afterQuad = true;
                    current = point;
                    break;
                }
                case 'A':
                {
                    var rx = Math.Abs(reader.ReadNumber());
                    var ry = Math.Abs(reader.ReadNumber());
                    var rotation = reader.ReadNumber();
                    var largeArc = reader.ReadFlag();
                    var sweep = reader.ReadFlag();
                    var point = reader.ReadPoint();
                    if (relative) point = Offset(current, point);

                    var target = Figure(ref figure, start);
                    if (rx <= 0 || ry <= 0 || point == current)
                    {
                        target.Segments.Add(new LineSegment(point, true));
                    }
                    else
                    {
                        target.Segments.Add(new ArcSegment(
                            point,
                            new Size(rx, ry),
                            rotation,
                            largeArc,
                            sweep ? SweepDirection.Clockwise : SweepDirection.Counterclockwise,
                            true));
                    }
                    current = point;
                    break;
                }
                case 'Z':
                {
                    if (figure is not null)
                    {
                        figure.IsClosed = true;
                        geometry.Figures.Add(figure);
                        figure = null;
                    }
                    current = start;
                    reader.Command = '\0';
                    break;
                }
                default:
                    throw new FormatException($"Unsupported SVG command '{kind}'.");
            }
        }

        if (figure is not null) geometry.Figures.Add(figure);
        geometry.Freeze();
        return geometry;
    }

    private static Point Offset(Point origin, Point delta) => new(origin.X + delta.X, origin.Y + delta.Y);

    private static Point Reflect(Point control, Point origin) => new(2 * origin.X - control.X, 2 * origin.Y - control.Y);

    private static PathFigure Figure(ref PathFigure? figure, Point anchor)
    {
        figure ??= new PathFigure { StartPoint = anchor, IsFilled = true };
        return figure;
    }

    private sealed class Reader(string text)
    {
        private readonly string _text = text;
        private int _index;

        public char Command { get; set; }

        public bool HasMore => _index < _text.Length;

        public bool IsLetter
        {
            get
            {
                SkipSeparators();
                return HasMore && char.IsLetter(_text[_index]);
            }
        }

        public char Read()
        {
            SkipSeparators();
            if (!HasMore) throw new FormatException("Unexpected end of SVG path.");
            return _text[_index++];
        }

        public void SkipSeparators()
        {
            while (HasMore && (char.IsWhiteSpace(_text[_index]) || _text[_index] == ',')) _index++;
        }

        public Point ReadPoint() => new(ReadNumber(), ReadNumber());

        public double ReadNumber()
        {
            SkipSeparators();
            var start = _index;
            if (HasMore && (_text[_index] == '+' || _text[_index] == '-')) _index++;
            while (HasMore && char.IsDigit(_text[_index])) _index++;
            if (HasMore && _text[_index] == '.')
            {
                _index++;
                while (HasMore && char.IsDigit(_text[_index])) _index++;
            }
            if (HasMore && (_text[_index] == 'e' || _text[_index] == 'E'))
            {
                _index++;
                if (HasMore && (_text[_index] == '+' || _text[_index] == '-')) _index++;
                while (HasMore && char.IsDigit(_text[_index])) _index++;
            }
            if (_index == start) throw new FormatException("Expected a number in SVG path.");
            return double.Parse(_text.AsSpan(start, _index - start), CultureInfo.InvariantCulture);
        }

        public bool ReadFlag()
        {
            SkipSeparators();
            if (!HasMore) throw new FormatException("Expected an arc flag.");
            var value = _text[_index] switch
            {
                '0' => false,
                '1' => true,
                _ => throw new FormatException("Arc flags must be 0 or 1."),
            };
            _index++;
            return value;
        }
    }
}
