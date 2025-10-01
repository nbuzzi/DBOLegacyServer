using System;
using System.Globalization;
using System.IO;
using System.Windows.Data;

namespace DBOServerMonitor.Converters
{
    public class FileNameConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            var s = value as string;
            if (string.IsNullOrEmpty(s)) return value;
            try { return Path.GetFileName(s); } catch { return s; }
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            // one-way
            return System.Windows.Data.Binding.DoNothing;
        }
    }
}