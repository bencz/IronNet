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
        public static extern double Tan(double a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Sqrt(double d);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Log(double d);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Log10(double d);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Exp(double d);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int Abs(int value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Abs(double value);

        public static long Abs(long value)
        {
            if (value == long.MinValue)
            {
                throw new OverflowException();
            }

            return value < 0 ? -value : value;
        }

        public static float Abs(float value)
        {
            if (value == 0)
            {
                return 0;
            }

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
            if (double.IsNaN(val1))
            {
                return val1;
            }
            if (double.IsNaN(val2))
            {
                return val2;
            }
            if (val1 == 0 && val2 == 0)
            {
                return IsNegative(val1) ? val2 : val1;
            }

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
            if (double.IsNaN(val1))
            {
                return val1;
            }
            if (double.IsNaN(val2))
            {
                return val2;
            }
            if (val1 == 0 && val2 == 0)
            {
                return IsNegative(val1) ? val1 : val2;
            }

            return val1 < val2 ? val1 : val2;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Floor(double d);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Ceiling(double a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Round(double a);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern double Pow(double x, double y);

        public static int Sign(int value)
        {
            if (value < 0)
            {
                return -1;
            }
            if (value > 0)
            {
                return 1;
            }

            return 0;
        }

        public static int Sign(double value)
        {
            if (double.IsNaN(value))
            {
                throw new ArithmeticException("Function does not accept floating point Not-a-Number values.");
            }
            if (value < 0)
            {
                return -1;
            }
            if (value > 0)
            {
                return 1;
            }

            return 0;
        }

        private static unsafe bool IsNegative(double value)
        {
            ulong bits = *((ulong*)&value);
            return (bits & 0x8000000000000000ul) != 0;
        }
    }
}
