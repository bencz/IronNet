using System;

public static unsafe class UnsafeOpcodeTest
{
    private struct Pair
    {
        public long Left;
        public int Right;
    }

    private static int Double(int value)
    {
        return value * 2;
    }

    private static bool LocalAllocationSurvivesCall()
    {
        int* values = stackalloc int[4];
        values[0] = 3;
        values[1] = 5;
        values[2] = 7;
        values[3] = 11;

        GC.Collect();
        return values[0] + values[1] + values[2] + values[3] == 26;
    }

    private static bool SizeMatchesLayout()
    {
        Pair value;

        value.Left = 1;
        value.Right = 2;
        return sizeof(Pair) == 16 && value.Left + value.Right == 3;
    }

    private static bool ManagedCalliWorks()
    {
        delegate* managed<int, int> function = &Double;
        return function(21) == 42;
    }

    public static int Main()
    {
        bool allocationResult = LocalAllocationSurvivesCall();
        bool sizeResult = SizeMatchesLayout();
        bool calliResult = ManagedCalliWorks();

        Console.WriteLine(allocationResult);
        Console.WriteLine(sizeResult);
        Console.WriteLine(calliResult);

        return allocationResult && sizeResult && calliResult ? 0 : 1;
    }
}
