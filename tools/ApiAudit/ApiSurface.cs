using System.Reflection;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;

namespace IronNet.ApiAudit;

internal sealed record ApiType(string Name, string Namespace, HashSet<string> Members);

internal static class ApiSurface
{
    internal static Dictionary<string, ApiType> Read(string path)
    {
        using FileStream stream = File.OpenRead(path);
        using PEReader pe = new PEReader(stream);
        MetadataReader reader = pe.GetMetadataReader();
        SignatureNames names = new SignatureNames();
        Dictionary<string, ApiType> types = new Dictionary<string, ApiType>(StringComparer.Ordinal);

        foreach (TypeDefinitionHandle handle in reader.TypeDefinitions)
        {
            if (!IsVisible(reader, handle))
            {
                continue;
            }

            TypeDefinition definition = reader.GetTypeDefinition(handle);
            string name = names.GetTypeFromDefinition(reader, handle, 0);
            TypeDefinition outer = definition;
            while (!outer.GetDeclaringType().IsNil)
            {
                outer = reader.GetTypeDefinition(outer.GetDeclaringType());
            }

            ApiType type = new ApiType(name, reader.GetString(outer.Namespace), new HashSet<string>(StringComparer.Ordinal));
            types.Add(name, type);

            foreach (MethodDefinitionHandle methodHandle in definition.GetMethods())
            {
                MethodDefinition method = reader.GetMethodDefinition(methodHandle);
                if (IsVisible(method.Attributes))
                {
                    MethodSignature<string> signature = method.DecodeSignature(names, (object?)null);
                    type.Members.Add("method " + reader.GetString(method.Name) + " " + SignatureNames.FormatMethod(signature));
                }
            }

            foreach (FieldDefinitionHandle fieldHandle in definition.GetFields())
            {
                FieldDefinition field = reader.GetFieldDefinition(fieldHandle);
                FieldAttributes access = field.Attributes & FieldAttributes.FieldAccessMask;
                if (access is FieldAttributes.Public or FieldAttributes.Family or FieldAttributes.FamORAssem)
                {
                    string storage = (field.Attributes & FieldAttributes.Static) != 0 ? " static " : " instance ";
                    type.Members.Add("field " + reader.GetString(field.Name) + storage + field.DecodeSignature(names, (object?)null));
                }
            }

            foreach (PropertyDefinitionHandle propertyHandle in definition.GetProperties())
            {
                PropertyDefinition property = reader.GetPropertyDefinition(propertyHandle);
                PropertyAccessors accessors = property.GetAccessors();
                if (IsVisible(reader, accessors.Getter) || IsVisible(reader, accessors.Setter))
                {
                    MethodSignature<string> signature = property.DecodeSignature(names, (object?)null);
                    type.Members.Add("property " + reader.GetString(property.Name) + " " + SignatureNames.FormatMethod(signature));
                }
            }

            foreach (EventDefinitionHandle eventHandle in definition.GetEvents())
            {
                EventDefinition eventDefinition = reader.GetEventDefinition(eventHandle);
                EventAccessors accessors = eventDefinition.GetAccessors();
                if (IsVisible(reader, accessors.Adder) || IsVisible(reader, accessors.Remover) || IsVisible(reader, accessors.Raiser))
                {
                    string eventType = eventDefinition.Type.Kind switch
                    {
                        HandleKind.TypeDefinition => names.GetTypeFromDefinition(reader, (TypeDefinitionHandle)eventDefinition.Type, 0),
                        HandleKind.TypeReference => names.GetTypeFromReference(reader, (TypeReferenceHandle)eventDefinition.Type, 0),
                        HandleKind.TypeSpecification => names.GetTypeFromSpecification(reader, null, (TypeSpecificationHandle)eventDefinition.Type, 0),
                        _ => throw new BadImageFormatException("Invalid event type handle.")
                    };
                    type.Members.Add("event " + reader.GetString(eventDefinition.Name) + " " + eventType);
                }
            }
        }

        if (types.Count == 0)
        {
            throw new InvalidDataException("The input has no visible type definitions; supply a contract assembly, not a forwarding facade: " + path);
        }

        return types;
    }

    private static bool IsVisible(MetadataReader reader, TypeDefinitionHandle handle)
    {
        TypeDefinition type = reader.GetTypeDefinition(handle);
        TypeAttributes access = type.Attributes & TypeAttributes.VisibilityMask;
        if (access == TypeAttributes.Public)
        {
            return true;
        }

        if (access is TypeAttributes.NestedPublic or TypeAttributes.NestedFamily or TypeAttributes.NestedFamORAssem)
        {
            return IsVisible(reader, type.GetDeclaringType());
        }

        return false;
    }

    private static bool IsVisible(MetadataReader reader, MethodDefinitionHandle handle)
    {
        return !handle.IsNil && IsVisible(reader.GetMethodDefinition(handle).Attributes);
    }

    private static bool IsVisible(MethodAttributes attributes)
    {
        MethodAttributes access = attributes & MethodAttributes.MemberAccessMask;
        return access is MethodAttributes.Public or MethodAttributes.Family or MethodAttributes.FamORAssem;
    }
}
