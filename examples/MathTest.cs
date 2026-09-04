using System;

class MathTest
{
    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    static void Main()
    {
        Console.WriteLine("=== Math Test ===");
        
        // Test basic math operations
        double a = 16.0;
        double b = 2.0;
        
        Console.WriteLine("a = 16.0, b = 2.0");
        Console.WriteLine("Sqrt(a) = " + Math.Sqrt(a));
        Console.WriteLine("Pow(b, 10) = " + Math.Pow(b, 10));
        Console.WriteLine("Abs(-42) = " + Math.Abs(-42));
        Console.WriteLine("Max(5, 10) = " + Math.Max(5, 10));
        Console.WriteLine("Min(5, 10) = " + Math.Min(5, 10));
        
        // Test trigonometry
        double angle = Math.PI / 4; // 45 degrees
        Console.WriteLine("Sin(PI/4) = " + Math.Sin(angle));
        Console.WriteLine("Cos(PI/4) = " + Math.Cos(angle));

        Assert(Math.Round(0.5) == 0.0, "Round must use midpoint-to-even for 0.5.");
        Assert(Math.Round(1.5) == 2.0, "Round must use midpoint-to-even for 1.5.");
        Assert(Math.Round(2.5) == 2.0, "Round must use midpoint-to-even for 2.5.");
        Assert(Math.Round(-1.5) == -2.0, "Round must use midpoint-to-even for -1.5.");
        Assert(Math.Round(-2.5) == -2.0, "Round must use midpoint-to-even for -2.5.");
        Assert(double.IsNaN(Math.Max(double.NaN, 1.0)), "Max must propagate NaN from its first argument.");
        Assert(double.IsNaN(Math.Min(1.0, double.NaN)), "Min must propagate NaN from its second argument.");
        Assert(1.0 / Math.Max(-0.0, 0.0) == double.PositiveInfinity, "Max must select positive zero.");
        Assert(1.0 / Math.Min(0.0, -0.0) == double.NegativeInfinity, "Min must select negative zero.");
        Assert(1.0 / Math.Round(-0.1) == double.NegativeInfinity, "Round must preserve negative zero.");

        bool overflow = false;
        try
        {
            Math.Abs(int.MinValue);
        }
        catch (OverflowException)
        {
            overflow = true;
        }

        Assert(overflow, "Abs(Int32.MinValue) did not throw OverflowException.");

        overflow = false;
        try
        {
            Math.Abs(long.MinValue);
        }
        catch (OverflowException)
        {
            overflow = true;
        }

        Assert(overflow, "Abs(Int64.MinValue) did not throw OverflowException.");

        bool arithmetic = false;
        try
        {
            Math.Sign(double.NaN);
        }
        catch (ArithmeticException)
        {
            arithmetic = true;
        }

        Assert(arithmetic, "Sign(NaN) did not throw ArithmeticException.");
        
        Console.WriteLine("=== Math Test Complete ===");
    }
}
