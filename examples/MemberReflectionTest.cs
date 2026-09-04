using System;
using System.Reflection;

public delegate void ReflectionCallback(int value);
public delegate void GenericReflectionCallback<T>(T value);

public interface IReflectionBase
{
}

public interface IReflectionMarker : IReflectionBase
{
}

public sealed class ReflectionContainer
{
    public sealed class VisibleNested
    {
    }

    private sealed class HiddenNested
    {
    }

    public static Type HiddenNestedType => typeof(HiddenNested);
}

public sealed class ReflectionPayload
{
    public int Value;

    public ReflectionPayload()
    {
    }

    public ReflectionPayload(int value)
    {
        Value = value;
    }
}

public sealed class ReflectionSample : IReflectionMarker
{
    public const int Answer = 42;
    public const string Greeting = "Olá";
    public const object Empty = null;
    public static string Label;
    public event ReflectionCallback Changed;
    public int Value;
    public int Number { get; set; }

    public int this[int offset]
    {
        get { return Value + offset; }
        set { Value = value - offset; }
    }

    public ReflectionSample(int value)
    {
        Value = value;
    }

    public int Add(int amount)
    {
        return Value + amount;
    }

    public static int Multiply(int left, int right)
    {
        return left * right;
    }

    public static void Increment(ref int value)
    {
        value++;
    }

    public static void ReplaceAfterCollection(ref object value)
    {
        value = new ReflectionPayload(123);
        GC.Collect();

        if (((ReflectionPayload)value).Value != 123)
        {
            throw new Exception("By-reference object was not rooted during collection");
        }
    }

    public static T Identity<T>(T value)
    {
        return value;
    }

    public static int ReferenceConstraint<T>() where T : class
    {
        return 1;
    }

    public static int ValueConstraint<T>() where T : struct
    {
        return 2;
    }

    public static int ConstructorConstraint<T>() where T : new()
    {
        return 3;
    }

    public static int InterfaceConstraint<T>() where T : IReflectionMarker
    {
        return 4;
    }

    public static bool IsTypeObject(object value)
    {
        return value is Type;
    }

    public static void Optional(int number = 17, string text = "default", object value = null)
    {
        if (number == -1 && text == null && value != null)
        {
            throw new Exception("Unreachable optional parameter guard");
        }
    }

    public void RaiseChanged(int value)
    {
        Changed(value);
    }

    private int Hidden()
    {
        return -1;
    }
}

public sealed class GenericReflectionSample<T>
{
    public T Item { get; set; }
    public event GenericReflectionCallback<T> Changed;

    public void RaiseChanged(T value)
    {
        Changed(value);
    }
}

public sealed class GenericConstraintCarrier<T> where T : IReflectionMarker
{
}

public static class MemberReflectionTest
{
    private static int _eventTotal;
    private static int _genericEventValue;

    private static void OnChanged(int value)
    {
        _eventTotal += value;
    }

    private static void OnChangedTwice(int value)
    {
        _eventTotal += value * 2;
    }

    private static void OnGenericChanged(int value)
    {
        _genericEventValue = value;
    }

    public static void Main()
    {
        Type sampleType = typeof(ReflectionSample);
        if (sampleType.Assembly == null)
        {
            throw new Exception("Type.Assembly failed");
        }
        if (!sampleType.IsClass || !sampleType.IsPublic || !sampleType.IsSealed || sampleType.IsAbstract || sampleType.DeclaringType != null)
        {
            throw new Exception("Type attribute reflection failed");
        }
        Type[] sampleInterfaces = sampleType.GetInterfaces();
        if (sampleInterfaces.Length != 2 || !typeof(IReflectionBase).IsAssignableFrom(sampleType) || !typeof(IReflectionMarker).IsAssignableFrom(sampleType))
        {
            throw new Exception("Type interface reflection failed");
        }
        Type genericSampleType = typeof(GenericReflectionSample<int>);
        Type genericSampleDefinition = typeof(GenericReflectionSample<>);
        Type[] genericArguments = genericSampleType.GetGenericArguments();
        Type[] genericParameters = genericSampleDefinition.GetGenericArguments();
        if (!genericSampleType.IsGenericType || genericSampleType.IsGenericTypeDefinition || genericSampleType.ContainsGenericParameters || genericArguments.Length != 1 || genericArguments[0] != typeof(int) ||
            !genericSampleDefinition.IsGenericTypeDefinition || !genericSampleDefinition.ContainsGenericParameters || genericParameters.Length != 1 || !genericParameters[0].IsGenericParameter ||
            genericSampleType.GetGenericTypeDefinition() != genericSampleDefinition)
        {
            throw new Exception("Generic Type reflection failed");
        }
        if (genericSampleDefinition.MakeGenericType(new Type[] { typeof(int) }) != genericSampleType)
        {
            throw new Exception("Dynamic generic Type construction failed");
        }
        if (genericParameters[0].GenericParameterPosition != 0 || genericParameters[0].GenericParameterAttributes != GenericParameterAttributes.None || genericParameters[0].FullName != null || genericParameters[0].Namespace != null ||
            genericParameters[0].DeclaringType != genericSampleDefinition || genericParameters[0].DeclaringMethod != null || genericParameters[0].GetGenericParameterConstraints().Length != 0)
        {
            throw new Exception("Generic type parameter metadata failed");
        }
        Type openConstructedType = genericSampleDefinition.MakeGenericType(new Type[] { genericSampleDefinition });
        if (!openConstructedType.IsGenericType || !openConstructedType.ContainsGenericParameters || openConstructedType.FullName != null)
        {
            throw new Exception("Open constructed generic Type failed");
        }
        Type intArrayType = typeof(int[]);
        Type matrixType = typeof(int).MakeArrayType(2);
        Type pointerType = typeof(int).MakePointerType();
        Type byRefType = typeof(int).MakeByRefType();
        if (!intArrayType.IsArray || !intArrayType.HasElementType || intArrayType.GetElementType() != typeof(int) || typeof(int).MakeArrayType() != intArrayType ||
            !matrixType.IsArray || matrixType.GetElementType() != typeof(int) || matrixType.GetArrayRank() != 2 || matrixType.FullName != "System.Int32[,]" ||
            !pointerType.IsPointer || pointerType.GetElementType() != typeof(int) || !byRefType.IsByRef || byRefType.GetElementType() != typeof(int))
        {
            throw new Exception("Element Type reflection failed");
        }
        Type containerType = typeof(ReflectionContainer);
        Type[] publicNestedTypes = containerType.GetNestedTypes();
        Type visibleNestedType = containerType.GetNestedType("visiblenested", BindingFlags.Public | BindingFlags.IgnoreCase);
        Type hiddenNestedType = containerType.GetNestedType("HiddenNested", BindingFlags.NonPublic);
        if (publicNestedTypes.Length != 1 || publicNestedTypes[0] != visibleNestedType || visibleNestedType.DeclaringType != containerType || visibleNestedType.FullName != "ReflectionContainer+VisibleNested" ||
            hiddenNestedType != ReflectionContainer.HiddenNestedType || hiddenNestedType.DeclaringType != containerType)
        {
            throw new Exception("Nested Type reflection failed");
        }

        ConstructorInfo constructor = sampleType.GetConstructor(new Type[] { typeof(int) });
        ReflectionSample sample = (ReflectionSample)constructor.Invoke(new object[] { 7 });

        MethodInfo add = sampleType.GetMethod("Add", new Type[] { typeof(int) });
        if (add.ReturnType != typeof(int) || add.GetParameters()[0].Name != "amount")
        {
            throw new Exception("Method metadata failed");
        }
        if ((int)add.Invoke(sample, new object[] { 5 }) != 12)
        {
            throw new Exception("Instance method invocation failed");
        }

        MethodInfo multiply = sampleType.GetMethod("Multiply", new Type[] { typeof(int), typeof(int) });
        if ((int)multiply.Invoke(null, new object[] { 6, 7 }) != 42)
        {
            throw new Exception("Static method invocation failed");
        }

        MethodInfo identityDefinition = sampleType.GetMethod("Identity");
        Type[] identityParameters = identityDefinition.GetGenericArguments();
        MethodInfo identity = identityDefinition.MakeGenericMethod(new Type[] { typeof(int) });
        MethodInfo duplicateIdentity = identityDefinition.MakeGenericMethod(new Type[] { typeof(int) });
        if (!identityDefinition.IsGenericMethod || !identityDefinition.IsGenericMethodDefinition || !identityDefinition.ContainsGenericParameters ||
            identityParameters.Length != 1 || !identityParameters[0].IsGenericParameter || !identity.IsGenericMethod || identity.IsGenericMethodDefinition || identity.ContainsGenericParameters ||
            identity.GetGenericArguments().Length != 1 || identity.GetGenericArguments()[0] != typeof(int) || !identity.GetGenericMethodDefinition().Equals(identityDefinition) || !duplicateIdentity.Equals(identity) ||
            identity.GetHashCode() != duplicateIdentity.GetHashCode() || identity.ReturnType != typeof(int) || identity.GetParameters()[0].ParameterType != typeof(int) ||
            !identity.GetParameters()[0].Equals(duplicateIdentity.GetParameters()[0]) || (int)identity.Invoke(null, new object[] { 77 }) != 77)
        {
            throw new Exception("Generic method reflection failed");
        }
        Type identityParameter = identityParameters[0];
        Type referenceParameter = sampleType.GetMethod("ReferenceConstraint").GetGenericArguments()[0];
        Type valueParameter = sampleType.GetMethod("ValueConstraint").GetGenericArguments()[0];
        Type constructorParameter = sampleType.GetMethod("ConstructorConstraint").GetGenericArguments()[0];
        Type interfaceParameter = sampleType.GetMethod("InterfaceConstraint").GetGenericArguments()[0];
        Type[] interfaceConstraints = interfaceParameter.GetGenericParameterConstraints();
        if (identityParameter.GenericParameterPosition != 0 || identityParameter.DeclaringType != null || identityParameter.DeclaringMethod.Name != "Identity" ||
            (referenceParameter.GenericParameterAttributes & GenericParameterAttributes.ReferenceTypeConstraint) == 0 ||
            (valueParameter.GenericParameterAttributes & GenericParameterAttributes.NotNullableValueTypeConstraint) == 0 ||
            (constructorParameter.GenericParameterAttributes & GenericParameterAttributes.DefaultConstructorConstraint) == 0 ||
            interfaceConstraints.Length != 1 || interfaceConstraints[0] != typeof(IReflectionMarker))
        {
            throw new Exception("Generic parameter constraint metadata failed");
        }
        if (sampleType.GetMethod("ReferenceConstraint").MakeGenericMethod(new Type[] { typeof(string) }) == null ||
            sampleType.GetMethod("ValueConstraint").MakeGenericMethod(new Type[] { typeof(int) }) == null ||
            sampleType.GetMethod("ConstructorConstraint").MakeGenericMethod(new Type[] { typeof(ReflectionPayload) }) == null ||
            sampleType.GetMethod("InterfaceConstraint").MakeGenericMethod(new Type[] { typeof(ReflectionSample) }) == null ||
            !sampleType.GetMethod("InterfaceConstraint").MakeGenericMethod(new Type[] { typeof(GenericConstraintCarrier<>).GetGenericArguments()[0] }).ContainsGenericParameters ||
            !genericSampleDefinition.GetMethod("RaiseChanged").ContainsGenericParameters)
        {
            throw new Exception("Generic method constraint reflection failed");
        }
        ParameterInfo[] optionalParameters = sampleType.GetMethod("Optional").GetParameters();
        if (optionalParameters.Length != 3 || !optionalParameters[0].IsOptional || (int)optionalParameters[0].DefaultValue != 17 ||
            !optionalParameters[1].IsOptional || (string)optionalParameters[1].DefaultValue != "default" || !optionalParameters[2].IsOptional || optionalParameters[2].DefaultValue != null)
        {
            throw new Exception("Optional parameter metadata failed");
        }
        object reflectedType = typeof(int);
        GC.SuppressFinalize(reflectedType);
        GC.ReRegisterForFinalize(reflectedType);
        if (!(reflectedType is Type) || !(reflectedType is object) || reflectedType.GetType() != typeof(Type) || !(bool)sampleType.GetMethod("IsTypeObject").Invoke(null, new object[] { reflectedType }))
        {
            throw new Exception("Runtime Type reference compatibility failed");
        }
        MethodInfo typeToString = typeof(Type).GetMethod("ToString");
        if ((string)typeToString.Invoke(reflectedType, null) != "System.Int32")
        {
            throw new Exception("Runtime Type virtual invocation failed");
        }
        Array reflectedTypes = new Type[1];
        reflectedTypes.SetValue(typeof(string), 0);
        GC.Collect();
        if ((Type)reflectedTypes.GetValue(0) != typeof(string))
        {
            throw new Exception("Runtime Type array compatibility failed");
        }

        object[] incrementArguments = new object[] { 10 };
        sampleType.GetMethod("Increment").Invoke(null, incrementArguments);
        if ((int)incrementArguments[0] != 11)
        {
            throw new Exception("By-reference invocation failed");
        }

        object[] replaceArguments = new object[] { new object() };
        sampleType.GetMethod("ReplaceAfterCollection").Invoke(null, replaceArguments);
        ReflectionPayload replacement = (ReflectionPayload)replaceArguments[0];
        if (replacement.Value != 123)
        {
            throw new Exception("By-reference reference invocation failed");
        }

        FieldInfo valueField = sampleType.GetField("Value");
        valueField.SetValue(sample, 21);
        if ((int)valueField.GetValue(sample) != 21)
        {
            throw new Exception("Instance field reflection failed");
        }

        FieldInfo labelField = sampleType.GetField("Label");
        labelField.SetValue(null, "ready");
        if ((string)labelField.GetValue(null) != "ready")
        {
            throw new Exception("Static field reflection failed");
        }
        if ((int)sampleType.GetField("Answer").GetValue(null) != 42 || (string)sampleType.GetField("Greeting").GetValue(null) != "Olá" || sampleType.GetField("Empty").GetValue(null) != null)
        {
            throw new Exception("Literal field reflection failed");
        }

        PropertyInfo numberProperty = sampleType.GetProperty("Number");
        numberProperty.SetValue(sample, 32);
        if (!numberProperty.CanRead || !numberProperty.CanWrite || numberProperty.PropertyType != typeof(int) || (int)numberProperty.GetValue(sample) != 32)
        {
            throw new Exception("Property reflection failed");
        }

        PropertyInfo indexer = sampleType.GetProperty("Item");
        ParameterInfo[] indexParameters = indexer.GetIndexParameters();
        if (indexParameters.Length != 1 || indexParameters[0].ParameterType != typeof(int))
        {
            throw new Exception("Indexer metadata reflection failed");
        }
        object indexValue = indexer.GetValue(sample, new object[] { 3 });
        if ((int)indexValue != 24)
        {
            throw new Exception("Indexer read reflection failed");
        }
        indexer.SetValue(sample, 50, new object[] { 2 });
        if (sample.Value != 48)
        {
            throw new Exception("Indexer write reflection failed");
        }

        GenericReflectionSample<int> generic = new GenericReflectionSample<int>();
        PropertyInfo genericProperty = genericSampleType.GetProperty("Item");
        genericProperty.SetValue(generic, 91);
        if (genericProperty.PropertyType != typeof(int) || (int)genericProperty.GetValue(generic) != 91)
        {
            throw new Exception("Constructed generic property reflection failed");
        }
        EventInfo genericEvent = genericSampleType.GetEvent("Changed");
        if (genericEvent == null || genericEvent.EventHandlerType != typeof(GenericReflectionCallback<int>))
        {
            throw new Exception("Constructed generic event metadata failed");
        }
        GenericReflectionCallback<int> genericHandler = OnGenericChanged;
        genericEvent.AddEventHandler(generic, genericHandler);
        generic.RaiseChanged(73);
        if (_genericEventValue != 73)
        {
            throw new Exception("Constructed generic event accessor failed");
        }

        EventInfo changedEvent = sampleType.GetEvent("Changed");
        if (changedEvent == null || changedEvent.EventHandlerType != typeof(ReflectionCallback) || changedEvent.GetAddMethod() == null || changedEvent.GetRemoveMethod() == null || changedEvent.GetRaiseMethod() != null)
        {
            throw new Exception("Event metadata reflection failed");
        }
        ReflectionCallback firstHandler = OnChanged;
        ReflectionCallback secondHandler = OnChangedTwice;
        changedEvent.AddEventHandler(sample, firstHandler);
        changedEvent.AddEventHandler(sample, secondHandler);
        GC.Collect();
        sample.RaiseChanged(3);
        changedEvent.RemoveEventHandler(sample, firstHandler);
        sample.RaiseChanged(2);
        if (_eventTotal != 13)
        {
            throw new Exception("Event accessor reflection failed");
        }

        MethodInfo hidden = sampleType.GetMethod("Hidden", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.DeclaredOnly);
        if (hidden == null || !hidden.IsPrivate)
        {
            throw new Exception("Non-public binding failed");
        }
        if (!typeof(object).IsAssignableFrom(sampleType) || !sampleType.IsInstanceOfType(sample))
        {
            throw new Exception("Type compatibility reflection failed");
        }

        Console.WriteLine("Member reflection test passed");
    }
}
