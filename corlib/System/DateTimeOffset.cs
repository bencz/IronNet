namespace System
{
    /// <summary>
    /// Represents a point in time together with its offset from Coordinated Universal Time.
    /// </summary>
    public partial struct DateTimeOffset
    {
        private const long UnixEpochTicks = 621355968000000000;
        private const int MaxOffsetMinutes = 14 * 60;

        public static readonly DateTimeOffset MinValue = new DateTimeOffset(DateTime.MinTicks, TimeSpan.Zero);
        public static readonly DateTimeOffset MaxValue = new DateTimeOffset(DateTime.MaxTicks, TimeSpan.Zero);
        public static readonly DateTimeOffset UnixEpoch = new DateTimeOffset(UnixEpochTicks, TimeSpan.Zero);

        private readonly long _utcTicks;
        private readonly short _offsetMinutes;

        public DateTimeOffset(long ticks, TimeSpan offset)
        {
            int offsetMinutes = ValidateOffset(offset);
            long utcTicks = ticks - offset.Ticks;
            ValidateUtcTicks(utcTicks);

            _utcTicks = utcTicks;
            _offsetMinutes = (short)offsetMinutes;
        }

        public DateTimeOffset(DateTime dateTime)
        {
            long offsetTicks;
            if (dateTime.Kind == DateTimeKind.Utc)
            {
                offsetTicks = 0;
            }
            else
            {
                offsetTicks = CurrentLocalOffsetTicks();
            }

            int offsetMinutes = ValidateOffset(new TimeSpan(offsetTicks));
            long utcTicks = dateTime.Ticks - offsetTicks;
            ValidateUtcTicks(utcTicks);

            _utcTicks = utcTicks;
            _offsetMinutes = (short)offsetMinutes;
        }

        public DateTimeOffset(DateTime dateTime, TimeSpan offset)
        {
            if (dateTime.Kind == DateTimeKind.Utc && offset != TimeSpan.Zero)
            {
                throw new ArgumentException("A UTC DateTime must have a zero offset.");
            }

            int offsetMinutes = ValidateOffset(offset);
            long utcTicks = dateTime.Ticks - offset.Ticks;
            ValidateUtcTicks(utcTicks);

            _utcTicks = utcTicks;
            _offsetMinutes = (short)offsetMinutes;
        }

        public DateTimeOffset(int year, int month, int day, int hour, int minute, int second, TimeSpan offset) :
            this(new DateTime(year, month, day, hour, minute, second), offset)
        {
        }

        public DateTimeOffset(int year, int month, int day, int hour, int minute, int second, int millisecond, TimeSpan offset) :
            this(new DateTime(year, month, day, hour, minute, second, millisecond), offset)
        {
        }

        public static DateTimeOffset Now => new DateTimeOffset(DateTime.Now);
        public static DateTimeOffset UtcNow => new DateTimeOffset(DateTime.UtcNow);

        public DateTime Date => DateTime.Date;
        public DateTime DateTime => new DateTime(Ticks, DateTimeKind.Unspecified);
        public int Day => DateTime.Day;
        public DayOfWeek DayOfWeek => DateTime.DayOfWeek;
        public int DayOfYear => DateTime.DayOfYear;
        public int Hour => DateTime.Hour;
        public DateTime LocalDateTime => UtcDateTime.ToLocalTime();
        public int Millisecond => DateTime.Millisecond;
        public int Minute => DateTime.Minute;
        public int Month => DateTime.Month;
        public TimeSpan Offset => TimeSpan.FromMinutes(_offsetMinutes);
        public int Second => DateTime.Second;
        public long Ticks => _utcTicks + (long)_offsetMinutes * TimeSpan.TicksPerMinute;
        public TimeSpan TimeOfDay => DateTime.TimeOfDay;
        public DateTime UtcDateTime => new DateTime(_utcTicks, DateTimeKind.Utc);
        public long UtcTicks => _utcTicks;
        public int Year => DateTime.Year;

        public DateTimeOffset Add(TimeSpan timeSpan)
        {
            return new DateTimeOffset(DateTime.Add(timeSpan), Offset);
        }

        public DateTimeOffset AddDays(double days)
        {
            return new DateTimeOffset(DateTime.AddDays(days), Offset);
        }

        public DateTimeOffset AddHours(double hours)
        {
            return new DateTimeOffset(DateTime.AddHours(hours), Offset);
        }

        public DateTimeOffset AddMilliseconds(double milliseconds)
        {
            return new DateTimeOffset(DateTime.AddMilliseconds(milliseconds), Offset);
        }

        public DateTimeOffset AddMinutes(double minutes)
        {
            return new DateTimeOffset(DateTime.AddMinutes(minutes), Offset);
        }

        public DateTimeOffset AddMonths(int months)
        {
            return new DateTimeOffset(DateTime.AddMonths(months), Offset);
        }

        public DateTimeOffset AddSeconds(double seconds)
        {
            return new DateTimeOffset(DateTime.AddSeconds(seconds), Offset);
        }

        public DateTimeOffset AddTicks(long ticks)
        {
            return new DateTimeOffset(DateTime.AddTicks(ticks), Offset);
        }

        public DateTimeOffset AddYears(int years)
        {
            return new DateTimeOffset(DateTime.AddYears(years), Offset);
        }

        public DateTimeOffset Subtract(TimeSpan value)
        {
            return new DateTimeOffset(DateTime.Subtract(value), Offset);
        }

        public TimeSpan Subtract(DateTimeOffset value)
        {
            return new TimeSpan(_utcTicks - value._utcTicks);
        }

        public DateTimeOffset ToOffset(TimeSpan offset)
        {
            ValidateOffset(offset);
            long localTicks = _utcTicks + offset.Ticks;
            if (localTicks < DateTime.MinTicks || localTicks > DateTime.MaxTicks)
            {
                throw new ArgumentException("The resulting clock time is outside the supported range.");
            }

            return new DateTimeOffset(localTicks, offset);
        }

        public DateTimeOffset ToLocalTime()
        {
            return ToOffset(new TimeSpan(CurrentLocalOffsetTicks()));
        }

        public DateTimeOffset ToUniversalTime()
        {
            return new DateTimeOffset(_utcTicks, TimeSpan.Zero);
        }

        public long ToUnixTimeMilliseconds()
        {
            return (_utcTicks - UnixEpochTicks) / TimeSpan.TicksPerMillisecond;
        }

        public long ToUnixTimeSeconds()
        {
            return (_utcTicks - UnixEpochTicks) / TimeSpan.TicksPerSecond;
        }

        public static DateTimeOffset FromUnixTimeMilliseconds(long milliseconds)
        {
            const long minimum = -62135596800000;
            const long maximum = 253402300799999;
            if (milliseconds < minimum || milliseconds > maximum)
            {
                throw new ArgumentOutOfRangeException("milliseconds");
            }

            return new DateTimeOffset(UnixEpochTicks + milliseconds * TimeSpan.TicksPerMillisecond, TimeSpan.Zero);
        }

        public static DateTimeOffset FromUnixTimeSeconds(long seconds)
        {
            const long minimum = -62135596800;
            const long maximum = 253402300799;
            if (seconds < minimum || seconds > maximum)
            {
                throw new ArgumentOutOfRangeException("seconds");
            }

            return new DateTimeOffset(UnixEpochTicks + seconds * TimeSpan.TicksPerSecond, TimeSpan.Zero);
        }

        public static int Compare(DateTimeOffset left, DateTimeOffset right)
        {
            if (left._utcTicks < right._utcTicks)
            {
                return -1;
            }

            return left._utcTicks > right._utcTicks ? 1 : 0;
        }

        public static bool Equals(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks == right._utcTicks;
        }

        public bool EqualsExact(DateTimeOffset other)
        {
            return _utcTicks == other._utcTicks && _offsetMinutes == other._offsetMinutes;
        }

        public bool Equals(DateTimeOffset other)
        {
            return _utcTicks == other._utcTicks;
        }

        public override bool Equals(object value)
        {
            return value is DateTimeOffset && ((DateTimeOffset)value)._utcTicks == _utcTicks;
        }

        public override int GetHashCode()
        {
            return (int)_utcTicks ^ (int)(_utcTicks >> 32);
        }

        public override string ToString()
        {
            int totalMinutes = _offsetMinutes;
            char sign = '+';
            if (totalMinutes < 0)
            {
                sign = '-';
                totalMinutes = -totalMinutes;
            }

            int offsetHours = totalMinutes / 60;
            int offsetMinutes = totalMinutes % 60;
            return DateTime.ToString() + " " + sign + offsetHours.ToString().PadLeft(2, '0') + ":" + offsetMinutes.ToString().PadLeft(2, '0');
        }

        public static DateTimeOffset operator +(DateTimeOffset value, TimeSpan duration)
        {
            return value.Add(duration);
        }

        public static DateTimeOffset operator -(DateTimeOffset value, TimeSpan duration)
        {
            return value.Subtract(duration);
        }

        public static TimeSpan operator -(DateTimeOffset left, DateTimeOffset right)
        {
            return left.Subtract(right);
        }

        public static bool operator ==(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks == right._utcTicks;
        }

        public static bool operator !=(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks != right._utcTicks;
        }

        public static bool operator <(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks < right._utcTicks;
        }

        public static bool operator >(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks > right._utcTicks;
        }

        public static bool operator <=(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks <= right._utcTicks;
        }

        public static bool operator >=(DateTimeOffset left, DateTimeOffset right)
        {
            return left._utcTicks >= right._utcTicks;
        }

        private static int ValidateOffset(TimeSpan offset)
        {
            if (offset.Ticks % TimeSpan.TicksPerMinute != 0)
            {
                throw new ArgumentException("The offset must be specified in whole minutes.");
            }

            long minutes = offset.Ticks / TimeSpan.TicksPerMinute;
            if (minutes < -MaxOffsetMinutes || minutes > MaxOffsetMinutes)
            {
                throw new ArgumentOutOfRangeException("offset");
            }

            return (int)minutes;
        }

        private static void ValidateUtcTicks(long utcTicks)
        {
            if (utcTicks < DateTime.MinTicks || utcTicks > DateTime.MaxTicks)
            {
                throw new ArgumentOutOfRangeException("offset");
            }
        }

        private static long CurrentLocalOffsetTicks()
        {
            long offsetTicks = Environment.GetLocalNowTicks() - Environment.GetUtcNowTicks();
            return offsetTicks / TimeSpan.TicksPerMinute * TimeSpan.TicksPerMinute;
        }
    }
}
