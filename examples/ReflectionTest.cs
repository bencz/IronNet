using System;
using System.Reflection;

class ReflectionTest
{
    static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new Exception(message);
        }
    }

    static Assembly GetCaller()
    {
        return Assembly.GetCallingAssembly();
    }

    static int Main()
    {
        Assembly executing = Assembly.GetExecutingAssembly();
        Assembly entry = Assembly.GetEntryAssembly();
        Assembly declaring = Assembly.GetAssembly(typeof(ReflectionTest));
        Assembly corlib = Assembly.GetAssembly(typeof(string));

        Require(executing != null, "Assembly.GetExecutingAssembly returned null.");
        Require(executing == entry, "The executing and entry assemblies should be identical in Main.");
        Require(executing == declaring, "Assembly.GetAssembly returned the wrong declaring assembly.");
        Require(GetCaller() == executing, "Assembly.GetCallingAssembly returned the wrong assembly.");
        Require(corlib != null && corlib != executing, "The corlib assembly was not resolved independently.");

        AssemblyName assemblyName = executing.GetName();
        Require(assemblyName.Name == "ReflectionTest", "AssemblyName did not parse the simple name.");
        Require(assemblyName.Version != null && assemblyName.Version.ToString() == "0.0.0.0", "AssemblyName did not parse the version.");
        Require(assemblyName.FullName == executing.FullName, "AssemblyName did not preserve the full identity.");
        Require(executing.FullName != null && executing.FullName.IndexOf(',') > 0, "Assembly.FullName is incomplete.");
        Require(executing.Location != null && executing.Location.Length > 0, "A file-backed assembly returned an empty location.");

        Require(executing.GetType("ReflectionTest") == typeof(ReflectionTest), "Assembly.GetType failed an exact lookup.");
        Require(executing.GetType("reflectiontest", false, true) == typeof(ReflectionTest), "Assembly.GetType failed an ignore-case lookup.");
        Require(executing.GetType("Missing.Type") == null, "Assembly.GetType returned an unknown type.");

        Type[] types = executing.GetTypes();
        bool found = false;
        for (int index = 0; index < types.Length; index++)
        {
            if (types[index] == typeof(ReflectionTest))
            {
                found = true;
            }
        }

        Require(found, "Assembly.GetTypes omitted the program type.");
        Console.WriteLine("Reflection test passed");
        return 0;
    }
}
