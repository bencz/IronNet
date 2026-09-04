using System;

public static class OpcodeCoverageTest
{
    private static int filterEvaluationCount;
    private static int finallyTrace;

    private static int SelectValue(int value)
    {
        switch (value)
        {
            case 0:
                return 10;
            case 1:
                return 20;
            case 2:
                return 30;
            case 3:
                return 40;
            case 4:
                return 50;
            default:
                return -1;
        }
    }

    private static bool CheckedAddThrows()
    {
        try
        {
            int value = int.MaxValue;
            value = checked(value + 1);
            return value == 0;
        }
        catch (OverflowException exception)
        {
            GC.Collect();
            return exception.Message.Length != 0;
        }
    }

    private static bool CheckedMultiplyThrows()
    {
        try
        {
            long value = long.MaxValue;
            value = checked(value * 2);
            return value == 0;
        }
        catch (OverflowException)
        {
            return true;
        }
    }

    private static bool CheckedConversionThrows()
    {
        try
        {
            long value = 256;
            byte converted = checked((byte)value);
            return converted == 0;
        }
        catch (OverflowException)
        {
            return true;
        }
    }

    private static bool UncheckedArithmeticWraps()
    {
        int maximum32 = int.MaxValue;
        int minimum32 = int.MinValue;
        long maximum64 = long.MaxValue;
        long minimum64 = long.MinValue;

        int sum32 = unchecked(maximum32 + 1);
        int product32 = unchecked(maximum32 * 2);
        int negated32 = unchecked(-minimum32);
        int shifted32 = 1 << 31;
        long sum64 = unchecked(maximum64 + 1);
        long product64 = unchecked(maximum64 * 2);
        long negated64 = unchecked(-minimum64);
        long shifted64 = 1L << 63;
        float remainder32 = 5.5f % 2.0f;
        double remainder64 = -5.5 % 2.0;

        return sum32 == minimum32 && product32 == -2 && negated32 == minimum32 && shifted32 == minimum32 &&
               sum64 == minimum64 && product64 == -2 && negated64 == minimum64 && shifted64 == minimum64 &&
               remainder32 == 1.5f && remainder64 == -1.5;
    }

    private static bool PendingExceptionSurvivesFinally()
    {
        bool finallyRan = false;

        try
        {
            try
            {
                throw new InvalidOperationException("pending-finally");
            }
            finally
            {
                GC.Collect();
                finallyRan = true;
            }
        }
        catch (InvalidOperationException exception)
        {
            return finallyRan && exception.Message == "pending-finally";
        }
    }

    private static void ThrowThroughFinally()
    {
        try
        {
            throw new InvalidOperationException("cross-frame-finally");
        }
        finally
        {
            GC.Collect();
            finallyTrace = finallyTrace * 10 + 3;
        }
    }

    private static int ReturnThroughNestedFinally()
    {
        try
        {
            try
            {
                return 7;
            }
            finally
            {
                finallyTrace = finallyTrace * 10 + 1;
            }
        }
        finally
        {
            finallyTrace = finallyTrace * 10 + 2;
        }
    }

    private static bool FinallyUnwindWorks()
    {
        finallyTrace = 0;
        try
        {
            ThrowThroughFinally();
        }
        catch (InvalidOperationException exception)
        {
            if (finallyTrace != 3 || exception.Message != "cross-frame-finally")
            {
                return false;
            }
        }

        finallyTrace = 0;
        return ReturnThroughNestedFinally() == 7 && finallyTrace == 12;
    }

    private static bool EvaluateFilter(InvalidOperationException exception, bool accept)
    {
        filterEvaluationCount++;
        GC.Collect();
        return accept && exception.Message == "filtered";
    }

    private static bool ThrowingFilter(InvalidOperationException exception)
    {
        filterEvaluationCount++;
        GC.Collect();
        throw new Exception(exception.Message);
    }

    private static bool ExceptionFiltersWork()
    {
        filterEvaluationCount = 0;
        try
        {
            throw new InvalidOperationException("filtered");
        }
        catch (InvalidOperationException exception) when (ThrowingFilter(exception))
        {
            return false;
        }
        catch (InvalidOperationException exception) when (EvaluateFilter(exception, false))
        {
            return false;
        }
        catch (InvalidOperationException exception) when (EvaluateFilter(exception, true))
        {
            return filterEvaluationCount == 3 && exception.Message == "filtered";
        }
    }

    public static int Main()
    {
        bool switchResult = SelectValue(3) == 40 && SelectValue(9) == -1;
        bool addResult = CheckedAddThrows();
        bool multiplyResult = CheckedMultiplyThrows();
        bool conversionResult = CheckedConversionThrows();
        bool uncheckedResult = UncheckedArithmeticWraps();
        bool finallyResult = PendingExceptionSurvivesFinally();
        bool unwindResult = FinallyUnwindWorks();
        bool filterResult = ExceptionFiltersWork();

        Console.WriteLine(switchResult);
        Console.WriteLine(addResult);
        Console.WriteLine(multiplyResult);
        Console.WriteLine(conversionResult);
        Console.WriteLine(uncheckedResult);
        Console.WriteLine(finallyResult);
        Console.WriteLine(unwindResult);
        Console.WriteLine(filterResult);

        return switchResult && addResult && multiplyResult && conversionResult && uncheckedResult && finallyResult && unwindResult && filterResult ? 0 : 1;
    }
}
