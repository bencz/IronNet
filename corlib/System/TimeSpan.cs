namespace System
{
    /// <summary>
    /// Represents a time interval with a resolution of one hundred nanoseconds.
    /// </summary>
    public partial struct TimeSpan
    {
        public const long TicksPerMillisecond = 10000;
        public const long TicksPerSecond = TicksPerMillisecond * 1000;
        public const long TicksPerMinute = TicksPerSecond * 60;
        public const long TicksPerHour = TicksPerMinute * 60;
        public const long TicksPerDay = TicksPerHour * 24;

        public static readonly TimeSpan Zero = new TimeSpan(0);
        public static readonly TimeSpan MaxValue = new TimeSpan(Int64.MaxValue);
        public static readonly TimeSpan MinValue = new TimeSpan(Int64.MinValue);

        private readonly long _ticks;

        public TimeSpan(long ticks)
        {
            _ticks = ticks;
        }

        public TimeSpan(int hours, int minutes, int seconds) : this(0, hours, minutes, seconds, 0)
        {
        }

        public TimeSpan(int days, int hours, int minutes, int seconds) : this(days, hours, minutes, seconds, 0)
        {
        }

        public TimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
        {
            long totalMilliseconds = ((long)days * 24 * 60 * 60 * 1000) + ((long)hours * 60 * 60 * 1000) +
                                     ((long)minutes * 60 * 1000) + ((long)seconds * 1000) + milliseconds;
            if (totalMilliseconds > Int64.MaxValue / TicksPerMillisecond || totalMilliseconds < Int64.MinValue / TicksPerMillisecond)
            {
                throw new ArgumentOutOfRangeException("milliseconds");
            }

            _ticks = totalMilliseconds * TicksPerMillisecond;
        }

        public long Ticks => _ticks;
        public int Days => (int)(_ticks / TicksPerDay);
        public int Hours => (int)((_ticks / TicksPerHour) % 24);
        public int Minutes => (int)((_ticks / TicksPerMinute) % 60);
        public int Seconds => (int)((_ticks / TicksPerSecond) % 60);
        public int Milliseconds => (int)((_ticks / TicksPerMillisecond) % 1000);

        public double TotalDays => (double)_ticks / TicksPerDay;
        public double TotalHours => (double)_ticks / TicksPerHour;
        public double TotalMinutes => (double)_ticks / TicksPerMinute;
        public double TotalSeconds => (double)_ticks / TicksPerSecond;
        public double TotalMilliseconds => (double)_ticks / TicksPerMillisecond;

        public static TimeSpan FromDays(double value)
        {
            return Interval(value, 86400000);
        }

        public static TimeSpan FromHours(double value)
        {
            return Interval(value, 3600000);
        }

        public static TimeSpan FromMinutes(double value)
        {
            return Interval(value, 60000);
        }

        public static TimeSpan FromSeconds(double value)
        {
            return Interval(value, 1000);
        }

        public static TimeSpan FromMilliseconds(double value)
        {
            return Interval(value, 1);
        }

        public static TimeSpan FromTicks(long value)
        {
            return new TimeSpan(value);
        }

        public TimeSpan Add(TimeSpan value)
        {
            long ticks = _ticks + value._ticks;
            if (((_ticks ^ value._ticks) >= 0) && ((_ticks ^ ticks) < 0))
            {
                throw new OverflowException("TimeSpan overflowed because the duration is too long.");
            }

            return new TimeSpan(ticks);
        }

        public TimeSpan Subtract(TimeSpan value)
        {
            long ticks = _ticks - value._ticks;
            if (((_ticks ^ value._ticks) < 0) && ((_ticks ^ ticks) < 0))
            {
                throw new OverflowException("TimeSpan overflowed because the duration is too long.");
            }

            return new TimeSpan(ticks);
        }

        public TimeSpan Negate()
        {
            if (_ticks == Int64.MinValue)
            {
                throw new OverflowException("Negating the minimum TimeSpan is not representable.");
            }

            return new TimeSpan(-_ticks);
        }

        public TimeSpan Duration()
        {
            if (_ticks == Int64.MinValue)
            {
                throw new OverflowException("The duration of the minimum TimeSpan is not representable.");
            }

            return new TimeSpan(_ticks < 0 ? -_ticks : _ticks);
        }

        public static int Compare(TimeSpan left, TimeSpan right)
        {
            if (left._ticks < right._ticks)
            {
                return -1;
            }

            return left._ticks > right._ticks ? 1 : 0;
        }

        public static bool Equals(TimeSpan left, TimeSpan right)
        {
            return left._ticks == right._ticks;
        }

        public override bool Equals(object value)
        {
            return value is TimeSpan && ((TimeSpan)value)._ticks == _ticks;
        }

        public override int GetHashCode()
        {
            return (int)_ticks ^ (int)(_ticks >> 32);
        }

        public override string ToString()
        {
            bool negative = _ticks < 0;
            ulong magnitude = negative ? 0ul - (ulong)_ticks : (ulong)_ticks;
            ulong days = magnitude / (ulong)TicksPerDay;
            int hours = (int)((magnitude / (ulong)TicksPerHour) % 24);
            int minutes = (int)((magnitude / (ulong)TicksPerMinute) % 60);
            int seconds = (int)((magnitude / (ulong)TicksPerSecond) % 60);
            int fraction = (int)(magnitude % (ulong)TicksPerSecond);

            string result = negative ? "-" : "";
            if (days != 0)
            {
                result += days.ToString() + ".";
            }

            result += hours.ToString().PadLeft(2, '0') + ":" + minutes.ToString().PadLeft(2, '0') + ":" + seconds.ToString().PadLeft(2, '0');
            if (fraction != 0)
            {
                result += "." + fraction.ToString().PadLeft(7, '0');
            }

            return result;
        }

        public static TimeSpan operator +(TimeSpan left, TimeSpan right)
        {
            return left.Add(right);
        }

        public static TimeSpan operator -(TimeSpan left, TimeSpan right)
        {
            return left.Subtract(right);
        }

        public static TimeSpan operator -(TimeSpan value)
        {
            return value.Negate();
        }

        public static bool operator ==(TimeSpan left, TimeSpan right)
        {
            return left._ticks == right._ticks;
        }

        public static bool operator !=(TimeSpan left, TimeSpan right)
        {
            return left._ticks != right._ticks;
        }

        public static bool operator <(TimeSpan left, TimeSpan right)
        {
            return left._ticks < right._ticks;
        }

        public static bool operator >(TimeSpan left, TimeSpan right)
        {
            return left._ticks > right._ticks;
        }

        public static bool operator <=(TimeSpan left, TimeSpan right)
        {
            return left._ticks <= right._ticks;
        }

        public static bool operator >=(TimeSpan left, TimeSpan right)
        {
            return left._ticks >= right._ticks;
        }

        private static TimeSpan Interval(double value, double scale)
        {
            if (Double.IsNaN(value))
            {
                throw new ArgumentException("TimeSpan does not accept NaN.");
            }

            double milliseconds = value * scale;
            double rounded = milliseconds + (value >= 0 ? 0.5 : -0.5);
            if (rounded > (double)(Int64.MaxValue / TicksPerMillisecond) || rounded < (double)(Int64.MinValue / TicksPerMillisecond))
            {
                throw new OverflowException("TimeSpan overflowed because the duration is too long.");
            }

            return new TimeSpan((long)rounded * TicksPerMillisecond);
        }
    }
}
