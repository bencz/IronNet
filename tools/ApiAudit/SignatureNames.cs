using System.Collections.Immutable;
using System.Reflection.Metadata;

namespace IronNet.ApiAudit;

internal sealed class SignatureNames : ISignatureTypeProvider<string, object?>
{
    public string GetArrayType(string elementType, ArrayShape shape)
    {
        return elementType + "[rank=" + shape.Rank + ";sizes=" + string.Join(",", shape.Sizes) + ";bounds=" + string.Join(",", shape.LowerBounds) + "]";
    }

    public string GetByReferenceType(string elementType) => elementType + "&";

    public string GetFunctionPointerType(MethodSignature<string> signature) => "fnptr " + FormatMethod(signature);

    public string GetGenericInstantiation(string genericType, ImmutableArray<string> typeArguments)
    {
        return genericType + "<" + string.Join(",", typeArguments) + ">";
    }

    public string GetGenericMethodParameter(object? genericContext, int index) => "!!" + index;

    public string GetGenericTypeParameter(object? genericContext, int index) => "!" + index;

    public string GetModifiedType(string modifier, string unmodifiedType, bool isRequired)
    {
        return unmodifiedType + (isRequired ? " modreq(" : " modopt(") + modifier + ")";
    }

    public string GetPinnedType(string elementType) => elementType + " pinned";

    public string GetPointerType(string elementType) => elementType + "*";

    public string GetPrimitiveType(PrimitiveTypeCode typeCode)
    {
        return "System." + typeCode;
    }

    public string GetSZArrayType(string elementType) => elementType + "[]";

    public string GetTypeFromDefinition(MetadataReader reader, TypeDefinitionHandle handle, byte rawTypeKind)
    {
        TypeDefinition type = reader.GetTypeDefinition(handle);
        string name = reader.GetString(type.Name);
        TypeDefinitionHandle parent = type.GetDeclaringType();
        if (!parent.IsNil)
        {
            return GetTypeFromDefinition(reader, parent, 0) + "+" + name;
        }

        string typeNamespace = reader.GetString(type.Namespace);
        return typeNamespace.Length == 0 ? name : typeNamespace + "." + name;
    }

    public string GetTypeFromReference(MetadataReader reader, TypeReferenceHandle handle, byte rawTypeKind)
    {
        TypeReference type = reader.GetTypeReference(handle);
        string name = reader.GetString(type.Name);
        if (type.ResolutionScope.Kind == HandleKind.TypeReference)
        {
            return GetTypeFromReference(reader, (TypeReferenceHandle)type.ResolutionScope, 0) + "+" + name;
        }

        string typeNamespace = reader.GetString(type.Namespace);
        return typeNamespace.Length == 0 ? name : typeNamespace + "." + name;
    }

    public string GetTypeFromSpecification(MetadataReader reader, object? genericContext, TypeSpecificationHandle handle, byte rawTypeKind)
    {
        return reader.GetTypeSpecification(handle).DecodeSignature(this, genericContext);
    }

    internal static string FormatMethod(MethodSignature<string> signature)
    {
        string receiver = signature.Header.IsInstance ? "instance " : "static ";
        return receiver + signature.Header.CallingConvention + " generic=" + signature.GenericParameterCount + " " + signature.ReturnType +
               "(" + string.Join(",", signature.ParameterTypes) + ") required=" + signature.RequiredParameterCount;
    }
}
