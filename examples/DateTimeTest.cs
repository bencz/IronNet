using System;

internal static class DateTimeTest
{
    private static int _assertions;

    private static void Main()
    {
        TestGregorianCalendar();
        TestArithmetic();
        TestTimeSpan();
        TestDateTimeOffset();
        TestSystemClock();

        Console.Write("Date/time assertions passed: ");
        Console.WriteLine(_assertions);
    }

    private static void TestGregorianCalendar()
    {
        DateTime epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        Assert(epoch.Ticks == 621355968000000000, "Unix epoch ticks");
        Assert(epoch.DayOfWeek == DayOfWeek.Thursday, "Unix epoch weekday");
        Assert(new DateTime(1, 1, 1).DayOfWeek == DayOfWeek.Monday, "Gregorian first weekday");
        Assert(DateTime.IsLeapYear(2000), "Year 2000 is leap");
        Assert(!DateTime.IsLeapYear(1900), "Year 1900 is not leap");
        Assert(DateTime.DaysInMonth(2024, 2) == 29, "Leap February length");

        DateTime leapDay = new DateTime(2024, 2, 29, 23, 58, 57, 321, DateTimeKind.Utc);
        Assert(leapDay.Year == 2024 && leapDay.Month == 2 && leapDay.Day == 29, "Date component extraction");
        Assert(leapDay.DayOfYear == 60, "Leap day ordinal");
        Assert(leapDay.Hour == 23 && leapDay.Minute == 58 && leapDay.Second == 57 && leapDay.Millisecond == 321, "Time component extraction");
        Assert(leapDay.Kind == DateTimeKind.Utc, "DateTime kind");
    }

    private static void TestArithmetic()
    {
        DateTime januaryEnd = new DateTime(2024, 1, 31, 12, 30, 0);
        DateTime februaryEnd = januaryEnd.AddMonths(1);
        Assert(februaryEnd.Year == 2024 && februaryEnd.Month == 2 && februaryEnd.Day == 29, "AddMonths clamps day");
        Assert(februaryEnd.Hour == 12 && februaryEnd.Minute == 30, "AddMonths preserves time");
        Assert(februaryEnd.AddYears(1).Day == 28, "AddYears clamps leap day");

        DateTime nextDay = januaryEnd.AddDays(1);
        Assert(nextDay.Month == 2 && nextDay.Day == 1, "AddDays crosses month");
        Assert(nextDay - januaryEnd == TimeSpan.FromDays(1), "DateTime subtraction");
        Assert(januaryEnd + TimeSpan.FromHours(2) == new DateTime(2024, 1, 31, 14, 30, 0), "DateTime plus TimeSpan");
        Assert(DateTime.Compare(januaryEnd, nextDay) < 0, "DateTime.Compare");
        Assert(DateTime.SpecifyKind(januaryEnd, DateTimeKind.Utc).Kind == DateTimeKind.Utc, "SpecifyKind");
    }

    private static void TestTimeSpan()
    {
        TimeSpan span = new TimeSpan(2, 3, 4, 5, 678);
        Assert(span.Days == 2 && span.Hours == 3 && span.Minutes == 4 && span.Seconds == 5 && span.Milliseconds == 678, "TimeSpan components");
        Assert(span == TimeSpan.FromDays(2) + new TimeSpan(3, 4, 5) + TimeSpan.FromMilliseconds(678), "TimeSpan addition");
        Assert(TimeSpan.FromSeconds(-2).Duration() == TimeSpan.FromSeconds(2), "TimeSpan.Duration");
        Assert(TimeSpan.Compare(TimeSpan.FromMinutes(1), TimeSpan.FromSeconds(59)) > 0, "TimeSpan.Compare");
        Assert(TimeSpan.FromMilliseconds(1.6).Ticks == 20000, "TimeSpan interval rounding");
        Assert(TimeSpan.FromMilliseconds(-1.6).Ticks == -20000, "Negative TimeSpan interval rounding");

        bool overflowed = false;
        try
        {
            TimeSpan.MaxValue.Add(TimeSpan.FromTicks(1));
        }
        catch (OverflowException)
        {
            overflowed = true;
        }

        Assert(overflowed, "TimeSpan addition overflow");
    }

    private static void TestSystemClock()
    {
        DateTime utc = DateTime.UtcNow;
        DateTime local = DateTime.Now;
        Assert(utc.Kind == DateTimeKind.Utc, "UtcNow kind");
        Assert(local.Kind == DateTimeKind.Local, "Now kind");
        Assert(utc.Year >= 2026 && utc.Year <= 9999, "UtcNow calendar range");

        long offsetHours = (local.Ticks - utc.Ticks) / TimeSpan.TicksPerHour;
        Assert(offsetHours >= -14 && offsetHours <= 14, "Local clock offset range");
        Assert(utc.ToLocalTime().Kind == DateTimeKind.Local, "ToLocalTime kind");
        Assert(local.ToUniversalTime().Kind == DateTimeKind.Utc, "ToUniversalTime kind");
    }

    private static void TestDateTimeOffset()
    {
        DateTimeOffset epoch = DateTimeOffset.FromUnixTimeSeconds(0);
        Assert(epoch == DateTimeOffset.UnixEpoch, "DateTimeOffset Unix epoch");
        Assert(epoch.ToUnixTimeMilliseconds() == 0, "Unix epoch milliseconds");

        TimeSpan offset = TimeSpan.FromHours(-3);
        DateTimeOffset localEpoch = epoch.ToOffset(offset);
        Assert(localEpoch.Year == 1969 && localEpoch.Month == 12 && localEpoch.Day == 31 && localEpoch.Hour == 21, "DateTimeOffset clock conversion");
        Assert(localEpoch.Offset == offset, "DateTimeOffset offset");
        Assert(localEpoch.UtcDateTime == new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc), "DateTimeOffset UTC projection");
        Assert(localEpoch.Equals(epoch), "DateTimeOffset instant equality");
        Assert(!localEpoch.EqualsExact(epoch), "DateTimeOffset exact equality");
        Assert(localEpoch.AddDays(1).ToUnixTimeSeconds() == 86400, "DateTimeOffset arithmetic");
        Assert(DateTimeOffset.FromUnixTimeMilliseconds(1234).ToUnixTimeMilliseconds() == 1234, "Unix millisecond roundtrip");
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            Console.Write("Assertion failed: ");
            Console.WriteLine(message);
            throw new Exception(message);
        }

        _assertions++;
    }
}
