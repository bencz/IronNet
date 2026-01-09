using System.Runtime.CompilerServices;

namespace System
{
    /// <summary>
    /// Provides constants and static methods for trigonometric, logarithmic, and other common mathematical functions
    /// </summary>
    public static class Math
    {
        public const double PI = 3.14159265358979323846;
        public const double E = 2.7182818284590452354;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Sin(double a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Cos(double a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Sqrt(double d);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Abs(int value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Abs(double value);

        public static long Abs(long value)
        {
            return value < 0 ? -value : value;
        }

        public static float Abs(float value)
        {
            return value < 0 ? -value : value;
        }

        public static int Max(int val1, int val2)
        {
            return val1 > val2 ? val1 : val2;
        }

        public static long Max(long val1, long val2)
        {
            return val1 > val2 ? val1 : val2;
        }

        public static double Max(double val1, double val2)
        {
            return val1 > val2 ? val1 : val2;
        }

        public static int Min(int val1, int val2)
        {
            return val1 < val2 ? val1 : val2;
        }

        public static long Min(long val1, long val2)
        {
            return val1 < val2 ? val1 : val2;
        }

        public static double Min(double val1, double val2)
        {
            return val1 < val2 ? val1 : val2;
        }

        public static double Floor(double d)
        {
            return (double)(long)d - (d < 0 && d != (long)d ? 1 : 0);
        }

        public static double Ceiling(double a)
        {
            return (double)(long)a + (a > 0 && a != (long)a ? 1 : 0);
        }

        public static double Round(double a)
        {
            return Floor(a + 0.5);
        }

        public static double Pow(double x, double y)
        {
            // Simple implementation for integer powers
            if (y == 0) return 1;
            if (y == 1) return x;
            if (y < 0) return 1.0 / Pow(x, -y);
            
            double result = 1;
            int n = (int)y;
            for (int i = 0; i < n; i++)
                result *= x;
            return result;
        }

        public static int Sign(int value)
        {
            if (value < 0) return -1;
            if (value > 0) return 1;
            return 0;
        }

        public static int Sign(double value)
        {
            if (value < 0) return -1;
            if (value > 0) return 1;
            return 0;
        }
    }
}
