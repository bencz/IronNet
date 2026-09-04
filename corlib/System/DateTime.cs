namespace System
{
    public enum DayOfWeek
    {
        Sunday = 0,
        Monday = 1,
        Tuesday = 2,
        Wednesday = 3,
        Thursday = 4,
        Friday = 5,
        Saturday = 6
    }

    public enum DateTimeKind
    {
        Unspecified = 0,
        Utc = 1,
        Local = 2
    }

    /// <summary>
    /// Represents an instant expressed as a date and time in the Gregorian calendar.
    /// </summary>
    public partial struct DateTime
    {
        public const long MinTicks = 0;
        public const long MaxTicks = 3155378975999999999;

        private const int DaysPerYear = 365;
        private const int DaysPer4Years = DaysPerYear * 4 + 1;
        private const int DaysPer100Years = DaysPer4Years * 25 - 1;
        private const int DaysPer400Years = DaysPer100Years * 4 + 1;

        private static readonly int[] DaysToMonth365 = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
        private static readonly int[] DaysToMonth366 = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

        public static readonly DateTime MinValue = new DateTime(MinTicks, DateTimeKind.Unspecified);
        public static readonly DateTime MaxValue = new DateTime(MaxTicks, DateTimeKind.Unspecified);

        private readonly long _ticks;
        private readonly DateTimeKind _kind;

        public DateTime(long ticks) : this(ticks, DateTimeKind.Unspecified)
        {
        }

        public DateTime(long ticks, DateTimeKind kind)
        {
            ValidateTicks(ticks);
            ValidateKind(kind);
            _ticks = ticks;
            _kind = kind;
        }

        public DateTime(int year, int month, int day) : this(year, month, day, 0, 0, 0, 0, DateTimeKind.Unspecified)
        {
        }

        public DateTime(int year, int month, int day, int hour, int minute, int second) : this(year, month, day, hour, minute, second, 0, DateTimeKind.Unspecified)
        {
        }

        public DateTime(int year, int month, int day, int hour, int minute, int second, DateTimeKind kind) : this(year, month, day, hour, minute, second, 0, kind)
        {
        }

        public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond) : this(year, month, day, hour, minute, second, millisecond, DateTimeKind.Unspecified)
        {
        }

        public DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, DateTimeKind kind)
        {
            ValidateKind(kind);
            _ticks = DateToTicks(year, month, day) + TimeToTicks(hour, minute, second) + ValidateMillisecond(millisecond) * TimeSpan.TicksPerMillisecond;
            _kind = kind;
        }

        public static DateTime Now => new DateTime(Environment.GetLocalNowTicks(), DateTimeKind.Local);
        public static DateTime UtcNow => new DateTime(Environment.GetUtcNowTicks(), DateTimeKind.Utc);
        public static DateTime Today => Now.Date;

        public DateTime Date => new DateTime(_ticks - _ticks % TimeSpan.TicksPerDay, _kind);
        public int Day => GetDatePart(2);
        public DayOfWeek DayOfWeek => (DayOfWeek)((_ticks / TimeSpan.TicksPerDay + 1) % 7);
        public int DayOfYear => GetDatePart(3);
        public int Hour => (int)((_ticks / TimeSpan.TicksPerHour) % 24);
        public DateTimeKind Kind => _kind;
        public int Millisecond => (int)((_ticks / TimeSpan.TicksPerMillisecond) % 1000);
        public int Minute => (int)((_ticks / TimeSpan.TicksPerMinute) % 60);
        public int Month => GetDatePart(1);
        public int Second => (int)((_ticks / TimeSpan.TicksPerSecond) % 60);
        public long Ticks => _ticks;
        public TimeSpan TimeOfDay => new TimeSpan(_ticks % TimeSpan.TicksPerDay);
        public int Year => GetDatePart(0);

        public DateTime Add(TimeSpan value)
        {
            return AddTicks(value.Ticks);
        }

        public DateTime AddDays(double value)
        {
            return AddMilliseconds(value * 86400000);
        }

        public DateTime AddHours(double value)
        {
            return AddMilliseconds(value * 3600000);
        }

        public DateTime AddMilliseconds(double value)
        {
            if (Double.IsNaN(value))
            {
                throw new ArgumentOutOfRangeException("value");
            }

            double rounded = value + (value >= 0 ? 0.5 : -0.5);
            if (rounded <= -315537897600000 || rounded >= 315537897600000)
            {
                throw new ArgumentOutOfRangeException("value");
            }

            return AddTicks((long)rounded * TimeSpan.TicksPerMillisecond);
        }

        public DateTime AddMinutes(double value)
        {
            return AddMilliseconds(value * 60000);
        }

        public DateTime AddSeconds(double value)
        {
            return AddMilliseconds(value * 1000);
        }

        public DateTime AddTicks(long value)
        {
            if (value > MaxTicks - _ticks || value < -_ticks)
            {
                throw new ArgumentOutOfRangeException("value");
            }

            return new DateTime(_ticks + value, _kind);
        }

        public DateTime AddMonths(int months)
        {
            if (months < -120000 || months > 120000)
            {
                throw new ArgumentOutOfRangeException("months");
            }

            int year = Year;
            int month = Month;
            int day = Day;
            int monthIndex = month - 1 + months;
            if (monthIndex >= 0)
            {
                month = monthIndex % 12 + 1;
                year += monthIndex / 12;
            }
            else
            {
                month = 12 + (monthIndex + 1) % 12;
                year += (monthIndex - 11) / 12;
            }

            if (year < 1 || year > 9999)
            {
                throw new ArgumentOutOfRangeException("months");
            }

            int daysInMonth = DaysInMonth(year, month);
            if (day > daysInMonth)
            {
                day = daysInMonth;
            }

            long dateTicks = DateToTicks(year, month, day);
            return new DateTime(dateTicks + _ticks % TimeSpan.TicksPerDay, _kind);
        }

        public DateTime AddYears(int value)
        {
            if (value < -10000 || value > 10000)
            {
                throw new ArgumentOutOfRangeException("value");
            }

            return AddMonths(value * 12);
        }

        public TimeSpan Subtract(DateTime value)
        {
            return new TimeSpan(_ticks - value._ticks);
        }

        public DateTime Subtract(TimeSpan value)
        {
            if (value.Ticks == Int64.MinValue)
            {
                throw new ArgumentOutOfRangeException("value");
            }

            return AddTicks(-value.Ticks);
        }

        public DateTime ToLocalTime()
        {
            if (_kind == DateTimeKind.Local)
            {
                return this;
            }

            long offset = Environment.GetLocalNowTicks() - Environment.GetUtcNowTicks();
            long localTicks = AddTicksClamped(_ticks, offset);
            return new DateTime(localTicks, DateTimeKind.Local);
        }

        public DateTime ToUniversalTime()
        {
            if (_kind == DateTimeKind.Utc)
            {
                return this;
            }

            long offset = Environment.GetLocalNowTicks() - Environment.GetUtcNowTicks();
            long utcTicks = AddTicksClamped(_ticks, -offset);
            return new DateTime(utcTicks, DateTimeKind.Utc);
        }

        public static DateTime SpecifyKind(DateTime value, DateTimeKind kind)
        {
            return new DateTime(value._ticks, kind);
        }

        public static int Compare(DateTime left, DateTime right)
        {
            if (left._ticks < right._ticks)
            {
                return -1;
            }

            return left._ticks > right._ticks ? 1 : 0;
        }

        public static bool Equals(DateTime left, DateTime right)
        {
            return left._ticks == right._ticks;
        }

        public static int DaysInMonth(int year, int month)
        {
            if (year < 1 || year > 9999)
            {
                throw new ArgumentOutOfRangeException("year");
            }

            if (month < 1 || month > 12)
            {
                throw new ArgumentOutOfRangeException("month");
            }

            int[] days = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
            return days[month] - days[month - 1];
        }

        public static bool IsLeapYear(int year)
        {
            if (year < 1 || year > 9999)
            {
                throw new ArgumentOutOfRangeException("year");
            }

            return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        }

        public override bool Equals(object value)
        {
            return value is DateTime && ((DateTime)value)._ticks == _ticks;
        }

        public override int GetHashCode()
        {
            return (int)_ticks ^ (int)(_ticks >> 32);
        }

        public override string ToString()
        {
            return Year.ToString().PadLeft(4, '0') + "-" + Month.ToString().PadLeft(2, '0') + "-" + Day.ToString().PadLeft(2, '0') + " " +
                   Hour.ToString().PadLeft(2, '0') + ":" + Minute.ToString().PadLeft(2, '0') + ":" + Second.ToString().PadLeft(2, '0');
        }

        public static DateTime operator +(DateTime value, TimeSpan duration)
        {
            return value.Add(duration);
        }

        public static DateTime operator -(DateTime value, TimeSpan duration)
        {
            return value.Subtract(duration);
        }

        public static TimeSpan operator -(DateTime left, DateTime right)
        {
            return left.Subtract(right);
        }

        public static bool operator ==(DateTime left, DateTime right)
        {
            return left._ticks == right._ticks;
        }

        public static bool operator !=(DateTime left, DateTime right)
        {
            return left._ticks != right._ticks;
        }

        public static bool operator <(DateTime left, DateTime right)
        {
            return left._ticks < right._ticks;
        }

        public static bool operator >(DateTime left, DateTime right)
        {
            return left._ticks > right._ticks;
        }

        public static bool operator <=(DateTime left, DateTime right)
        {
            return left._ticks <= right._ticks;
        }

        public static bool operator >=(DateTime left, DateTime right)
        {
            return left._ticks >= right._ticks;
        }

        private static long DateToTicks(int year, int month, int day)
        {
            if (year < 1 || year > 9999)
            {
                throw new ArgumentOutOfRangeException("year");
            }

            if (month < 1 || month > 12)
            {
                throw new ArgumentOutOfRangeException("month");
            }

            int[] days = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
            int daysInMonth = days[month] - days[month - 1];
            if (day < 1 || day > daysInMonth)
            {
                throw new ArgumentOutOfRangeException("day");
            }

            int previousYear = year - 1;
            int totalDays = previousYear * 365 + previousYear / 4 - previousYear / 100 + previousYear / 400 + days[month - 1] + day - 1;
            return (long)totalDays * TimeSpan.TicksPerDay;
        }

        private static long TimeToTicks(int hour, int minute, int second)
        {
            if (hour < 0 || hour >= 24)
            {
                throw new ArgumentOutOfRangeException("hour");
            }

            if (minute < 0 || minute >= 60)
            {
                throw new ArgumentOutOfRangeException("minute");
            }

            if (second < 0 || second >= 60)
            {
                throw new ArgumentOutOfRangeException("second");
            }

            return ((long)hour * 3600 + minute * 60 + second) * TimeSpan.TicksPerSecond;
        }

        private int GetDatePart(int part)
        {
            int days = (int)(_ticks / TimeSpan.TicksPerDay);
            int years400 = days / DaysPer400Years;
            days -= years400 * DaysPer400Years;
            int years100 = days / DaysPer100Years;
            if (years100 == 4)
            {
                years100 = 3;
            }

            days -= years100 * DaysPer100Years;
            int years4 = days / DaysPer4Years;
            days -= years4 * DaysPer4Years;
            int years1 = days / DaysPerYear;
            if (years1 == 4)
            {
                years1 = 3;
            }

            if (part == 0)
            {
                return years400 * 400 + years100 * 100 + years4 * 4 + years1 + 1;
            }

            days -= years1 * DaysPerYear;
            if (part == 3)
            {
                return days + 1;
            }

            int year = years400 * 400 + years100 * 100 + years4 * 4 + years1 + 1;
            int[] daysToMonth = IsLeapYear(year) ? DaysToMonth366 : DaysToMonth365;
            int month = (days >> 5) + 1;
            while (days >= daysToMonth[month])
            {
                month++;
            }

            if (part == 1)
            {
                return month;
            }

            return days - daysToMonth[month - 1] + 1;
        }

        private static int ValidateMillisecond(int millisecond)
        {
            if (millisecond < 0 || millisecond >= 1000)
            {
                throw new ArgumentOutOfRangeException("millisecond");
            }

            return millisecond;
        }

        private static void ValidateTicks(long ticks)
        {
            if (ticks < MinTicks || ticks > MaxTicks)
            {
                throw new ArgumentOutOfRangeException("ticks");
            }
        }

        private static void ValidateKind(DateTimeKind kind)
        {
            if (kind < DateTimeKind.Unspecified || kind > DateTimeKind.Local)
            {
                throw new ArgumentException("Invalid DateTimeKind value.");
            }
        }

        private static long AddTicksClamped(long ticks, long value)
        {
            if (value > MaxTicks - ticks)
            {
                return MaxTicks;
            }

            if (value < -ticks)
            {
                return MinTicks;
            }

            return ticks + value;
        }
    }
}
