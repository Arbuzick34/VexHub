using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Media;

namespace VexHub.Loader.Core;

public enum StepState
{
    Pending,
    Active,
    Done,
    Warn,
    Error,
}

public sealed class StepViewModel : INotifyPropertyChanged
{
    private readonly string _prefix;
    private StepState _state = StepState.Pending;
    private string? _detailKey;
    private object[] _detailArgs = [];
    private string? _detailRaw;

    public StepViewModel(string prefix, Geometry icon)
    {
        _prefix = prefix;
        Icon = icon;
    }

    public Geometry Icon { get; }

    public string Title => Loc.Get($"{_prefix}Title");

    public string Detail => _detailRaw
        ?? (_detailKey is null ? Loc.Get($"{_prefix}{State}") : string.Format(Loc.Get(_detailKey), _detailArgs));

    public StepState State
    {
        get => _state;
        private set
        {
            if (_state == value) return;
            _state = value;
            Raise(nameof(State));
            Raise(nameof(Detail));
        }
    }

    public void Set(StepState state, string? detailKey = null, params object[] args)
    {
        _detailKey = detailKey;
        _detailArgs = args;
        _detailRaw = null;
        State = state;
        Raise(nameof(Detail));
    }

    public void SetText(StepState state, string detail)
    {
        _detailKey = null;
        _detailArgs = [];
        _detailRaw = detail;
        State = state;
        Raise(nameof(Detail));
    }

    public void Reset() => Set(StepState.Pending);

    public void Refresh()
    {
        Raise(nameof(Title));
        Raise(nameof(Detail));
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Raise([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
