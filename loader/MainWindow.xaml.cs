using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using VexHub.Loader.Core;

namespace VexHub.Loader;

public sealed record LogLine(string Time, string Text);

public partial class MainWindow : Window
{
    private const string SiteUrl = "https://github.com/Arbuzick34/VexHub";
    private const string SourceUrl = "https://github.com/Arbuzick34/VexHub";
    private const string DocsUrl = "https://github.com/Arbuzick34/VexHub#readme";

    private enum Phase
    {
        Idle,
        Busy,
        Active,
    }

    private readonly LoaderSettings _settings;
    private readonly ObservableCollection<StepViewModel> _steps;
    private readonly ObservableCollection<LogLine> _log = [];

    private string _language;
    private Phase _phase = Phase.Idle;
    private bool _failed;
    private CancellationTokenSource? _session;

    public MainWindow()
    {
        _settings = LoaderSettings.Load();
        _language = App.LanguageOverride is { Length: > 0 } forced ? forced : _settings.Language;
        Loc.Apply(_language);

        InitializeComponent();

        _steps = new ObservableCollection<StepViewModel>
        {
            new("StepClient", Icons.Box),
            new("StepGame", Icons.Target),
            new("StepStart", Icons.Play),
            new("StepReady", Icons.Shield),
        };

        StepsList.ItemsSource = _steps;
        LogList.ItemsSource = _log;

        LangButton.Content = _language == "en" ? "RU" : "EN";

        AddLog("loader ready");
        SyncUi();
    }

    private static Brush Brush(string key) => (Brush)Application.Current.FindResource(key);

    // ── session ──────────────────────────────────────────────────────────────

    private async void Launch_Click(object sender, RoutedEventArgs e) => await LaunchAsync();

    private async Task LaunchAsync()
    {
        if (_phase == Phase.Busy) return;

        if (_phase == Phase.Active)
        {
            Close();
            return;
        }

        _phase = Phase.Busy;
        _failed = false;
        HideError();
        ResetSteps();
        ResetProgress();
        _log.Clear();
        SyncUi();

        _session?.Cancel();
        _session?.Dispose();
        _session = new CancellationTokenSource();

        try
        {
            await RunSessionAsync(_session.Token);
        }
        catch (OperationCanceledException)
        {
            // The window was closed mid-session.
        }
        catch (Exception error)
        {
            _steps[^1].Set(StepState.Error);
            Fail(error.Message);
        }
    }

    private async Task RunSessionAsync(CancellationToken token)
    {
        SetProgress(0.08);
        AddLog("session · starting");

        // 1 · client binary
        _steps[0].Set(StepState.Active);
        AddLog("client · resolving vexhub.exe");
        await Task.Delay(160, token);

        var path = VexHubClient.Resolve(_settings);
        if (path is null)
        {
            _steps[0].Set(StepState.Error);
            Fail(Loc.Get("ErrorVexHubMissing"));
            return;
        }

        var version = ReadVersion(path);
        _steps[0].Set(StepState.Done, "StepClientFound", version);
        AddLog($"client · vexhub.exe v{version}");
        SetProgress(0.34);

        // 2 · game state
        _steps[1].Set(StepState.Active);
        await Task.Delay(160, token);

        using var game = GameClient.Find();
        if (game is null)
        {
            _steps[1].Set(StepState.Warn, "StepGameWarnDetail");
            AddLog("cs2 · not running");
        }
        else
        {
            _steps[1].Set(StepState.Done, "StepGamePid", game.Id);
            AddLog($"cs2 · pid {game.Id}");
        }

        SetProgress(0.58);

        // 3 · start the client
        _steps[2].Set(StepState.Active);
        await Task.Delay(200, token);

        using var running = VexHubClient.FindRunning();
        if (running is not null)
        {
            _steps[2].Set(StepState.Done, "StepStartRunning", running.Id);
            AddLog($"client · already running pid {running.Id}");
        }
        else
        {
            try
            {
                using var started = VexHubClient.Start(path);
                _steps[2].Set(StepState.Done, "StepStartPid", started.Id);
                AddLog($"client · launched pid {started.Id}");
            }
            catch (Exception error)
            {
                _steps[2].Set(StepState.Error);
                Fail(string.Format(Loc.Get("ErrorStart"), error.Message));
                return;
            }
        }

        SetProgress(0.82);

        // 4 · ready
        _steps[3].Set(StepState.Active);
        await Task.Delay(260, token);
        _steps[3].Set(StepState.Done);

        SetProgress(1);
        _phase = Phase.Active;
        SyncUi();
        AddLog("session active · press Insert in-game");
    }

    private static string ReadVersion(string path)
    {
        try
        {
            var info = FileVersionInfo.GetVersionInfo(path);
            var version = info.ProductVersion ?? info.FileVersion;
            return string.IsNullOrWhiteSpace(version) ? "1.2.3" : version.Split('+')[0];
        }
        catch
        {
            return "1.2.3";
        }
    }

    private void Fail(string message)
    {
        _phase = Phase.Idle;
        _failed = true;
        ShowError(message);
        AddLog("error · " + message);
        SyncUi();
    }

    private void ResetSteps()
    {
        foreach (var step in _steps) step.Reset();
    }

    // ── ui state ─────────────────────────────────────────────────────────────

    private void SyncUi()
    {
        if (_phase == Phase.Busy)
        {
            Badge.Background = Brush("BadgeBusyBrush");
            Badge.BorderBrush = Brush("BadgeBusyBorderBrush");
            BadgeDot.Fill = Brush("AccentBrightBrush");
            BadgeText.Foreground = Brush("AccentBrightBrush");
            BadgeText.Text = Loc.Get("BadgeRunning");
            ProgressLabel.Text = Loc.Get("ProgressBusy");
            BarFill.Background = Brush("AccentBarBrush");
        }
        else if (_phase == Phase.Active)
        {
            Badge.Background = Brush("BadgeActiveBrush");
            Badge.BorderBrush = Brush("BadgeActiveBorderBrush");
            BadgeDot.Fill = Brush("OkBrush");
            BadgeText.Foreground = Brush("OkBrush");
            BadgeText.Text = Loc.Get("BadgeActive");
            ProgressLabel.Text = Loc.Get("ProgressDone");
            BarFill.Background = Brush("OkBarBrush");
        }
        else if (_failed)
        {
            Badge.Background = Brush("BadgeErrorBrush");
            Badge.BorderBrush = Brush("BadgeErrorBorderBrush");
            BadgeDot.Fill = Brush("DangerBrush");
            BadgeText.Foreground = Brush("DangerTextBrush");
            BadgeText.Text = Loc.Get("BadgeError");
            ProgressLabel.Text = Loc.Get("ProgressFailed");
            BarFill.Background = Brush("AccentBarBrush");
        }
        else
        {
            Badge.Background = Brush("BadgeIdleBrush");
            Badge.BorderBrush = Brush("LineSoftBrush");
            BadgeDot.Fill = Brush("FaintBrush");
            BadgeText.Foreground = Brush("MutedBrush");
            BadgeText.Text = Loc.Get("BadgeIdle");
            ProgressLabel.Text = Loc.Get("ProgressIdle");
            BarFill.Background = Brush("AccentBarBrush");
        }

        string style;
        Geometry icon;
        string label;

        if (_phase == Phase.Busy)
        {
            style = "BusyButton";
            icon = Icons.Play;
            label = "LaunchBusy";
        }
        else if (_phase == Phase.Active)
        {
            style = "SecondaryButton";
            icon = Icons.Power;
            label = "LaunchClose";
        }
        else if (_failed)
        {
            style = "PrimaryButton";
            icon = Icons.Sync;
            label = "LaunchRetry";
        }
        else
        {
            style = "PrimaryButton";
            icon = Icons.Play;
            label = "LaunchStart";
        }

        LaunchButton.Style = (Style)FindResource(style);
        LaunchButton.IsEnabled = _phase != Phase.Busy;
        LaunchIcon.Data = icon;
        LaunchIcon.Visibility = _phase == Phase.Busy ? Visibility.Collapsed : Visibility.Visible;
        LaunchText.Text = Loc.Get(label);
        LaunchSpinner.Visibility = _phase == Phase.Busy ? Visibility.Visible : Visibility.Collapsed;

        if (_phase == Phase.Busy)
        {
            if (!LaunchSpinRotate.HasAnimatedProperties)
            {
                LaunchSpinRotate.BeginAnimation(
                    RotateTransform.AngleProperty,
                    new DoubleAnimation(0, 360, TimeSpan.FromMilliseconds(900)) { RepeatBehavior = RepeatBehavior.Forever });
            }
        }
        else
        {
            LaunchSpinRotate.BeginAnimation(RotateTransform.AngleProperty, null);
            LaunchSpinRotate.Angle = 0;
        }
    }

    private void SetProgress(double value, bool instant = false)
    {
        value = Math.Clamp(value, 0, 1);
        PercentText.Text = $"{(int)Math.Round(value * 100)}%";

        if (instant)
        {
            BarScale.BeginAnimation(ScaleTransform.ScaleXProperty, null);
            BarScale.ScaleX = value;
            return;
        }

        BarScale.BeginAnimation(
            ScaleTransform.ScaleXProperty,
            new DoubleAnimation(value, TimeSpan.FromMilliseconds(380))
            {
                EasingFunction = new CubicEase { EasingMode = EasingMode.EaseOut },
            });
    }

    private void ResetProgress()
    {
        BarScale.BeginAnimation(ScaleTransform.ScaleXProperty, null);
        BarScale.ScaleX = 0;
        PercentText.Text = "0%";
        BarFill.Background = Brush("AccentBarBrush");
    }

    private void ShowError(string message)
    {
        ErrorText.Text = message;
        ErrorPanel.Visibility = Visibility.Visible;
    }

    private void HideError()
    {
        ErrorPanel.Visibility = Visibility.Collapsed;
        ErrorText.Text = string.Empty;
    }

    private void AddLog(string text)
    {
        _log.Add(new LogLine(DateTime.Now.ToString("HH:mm:ss"), text));
        while (_log.Count > 40) _log.RemoveAt(0);
        LogScroll.ScrollToEnd();
    }

    // ── chrome ───────────────────────────────────────────────────────────────

    private void Lang_Click(object sender, RoutedEventArgs e)
    {
        _language = _language == "en" ? "ru" : "en";
        Loc.Apply(_language);
        LangButton.Content = _language == "en" ? "RU" : "EN";

        foreach (var step in _steps) step.Refresh();

        SyncUi();
        _settings.Language = _language;
        _settings.Save();
    }

    private void TitleBar_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ButtonState != MouseButtonState.Pressed) return;

        try
        {
            DragMove();
        }
        catch (InvalidOperationException)
        {
            // DragMove throws when the button is released before the call lands.
        }
    }

    private void Minimize_Click(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;

    private void Close_Click(object sender, RoutedEventArgs e) => Close();

    private void ClearLog_Click(object sender, RoutedEventArgs e) => _log.Clear();

    private void Site_Click(object sender, RoutedEventArgs e) => Open(SiteUrl);

    private void Source_Click(object sender, RoutedEventArgs e) => Open(SourceUrl);

    private void Docs_Click(object sender, RoutedEventArgs e) => Open(DocsUrl);

    private static void Open(string url)
    {
        try
        {
            Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
        }
        catch
        {
            // Opening a browser is best-effort.
        }
    }

    protected override void OnClosing(CancelEventArgs e)
    {
        _session?.Cancel();
        _session?.Dispose();
        _session = null;
        base.OnClosing(e);
    }

    // ── screenshot mode ──────────────────────────────────────────────────────

    protected override async void OnContentRendered(EventArgs e)
    {
        base.OnContentRendered(e);
        if (App.ShotPath is null) return;

        try
        {
            ApplyDemoState(App.ShotState);
            await Task.Delay(700);
            SaveShot(App.ShotPath);
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error);
        }
        finally
        {
            Application.Current.Shutdown();
        }
    }

    private void ApplyDemoState(string? state)
    {
        switch ((state ?? "idle").ToLowerInvariant())
        {
            case "busy":
                _phase = Phase.Busy;
                _steps[0].Set(StepState.Done);
                _steps[1].Set(StepState.Active);
                SetProgress(0.42, instant: true);
                AddLog("client · vexhub.exe v1.2.3");
                AddLog("cs2 · scanning processes");
                break;

            case "active":
                _phase = Phase.Active;
                _steps[0].Set(StepState.Done, "StepClientFound", "1.2.3");
                _steps[1].Set(StepState.Done, "StepGamePid", 18244);
                _steps[2].Set(StepState.Done, "StepStartPid", 9012);
                _steps[3].Set(StepState.Done);
                SetProgress(1, instant: true);
                AddLog("client · vexhub.exe v1.2.3");
                AddLog("cs2 · pid 18244");
                AddLog("client · launched pid 9012");
                AddLog("session active · press Insert in-game");
                break;

            case "warn":
                _phase = Phase.Active;
                _steps[0].Set(StepState.Done, "StepClientFound", "1.2.3");
                _steps[1].Set(StepState.Warn, "StepGameWarnDetail");
                _steps[2].Set(StepState.Done, "StepStartPid", 9012);
                _steps[3].Set(StepState.Done);
                SetProgress(1, instant: true);
                AddLog("client · vexhub.exe v1.2.3");
                AddLog("cs2 · not running");
                AddLog("session active · press Insert in-game");
                break;

            case "error":
                _steps[0].Set(StepState.Error);
                SetProgress(0.12, instant: true);
                Fail(Loc.Get("ErrorVexHubMissing"));
                break;

            default:
                SetProgress(0, instant: true);
                AddLog("waiting to launch");
                break;
        }

        SyncUi();
    }

    private void SaveShot(string path)
    {
        ShotBackground.Visibility = Visibility.Visible;
        Surface.UpdateLayout();

        var width = (int)Math.Ceiling(Surface.ActualWidth);
        var height = (int)Math.Ceiling(Surface.ActualHeight);
        if (width <= 0 || height <= 0) return;

        var bitmap = new RenderTargetBitmap(width, height, 96, 96, PixelFormats.Pbgra32);
        bitmap.Render(Surface);

        var encoder = new PngBitmapEncoder();
        encoder.Frames.Add(BitmapFrame.Create(bitmap));

        var full = Path.GetFullPath(path);
        var folder = Path.GetDirectoryName(full);
        if (!string.IsNullOrEmpty(folder)) Directory.CreateDirectory(folder);

        using var stream = File.Create(full);
        encoder.Save(stream);
    }
}
