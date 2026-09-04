using System;

sealed class FinalizedResource
{
    public static int FinalizedCount;

    ~FinalizedResource()
    {
        FinalizedCount++;
    }
}

class FinalizerTest
{
    static void AllocateFinalizable()
    {
        new FinalizedResource();
    }

    static void AllocateSuppressed()
    {
        FinalizedResource resource = new FinalizedResource();
        GC.SuppressFinalize(resource);
    }

    static void AllocateReregistered()
    {
        FinalizedResource resource = new FinalizedResource();
        GC.SuppressFinalize(resource);
        GC.ReRegisterForFinalize(resource);
    }

    static void Main()
    {
        AllocateFinalizable();
        GC.Collect();
        Console.WriteLine(FinalizedResource.FinalizedCount);

        AllocateSuppressed();
        GC.Collect();
        Console.WriteLine(FinalizedResource.FinalizedCount);

        AllocateReregistered();
        GC.Collect();
        Console.WriteLine(FinalizedResource.FinalizedCount);
    }
}
