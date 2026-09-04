# ApiAudit

Inventaria tipos e assinaturas declaradas públicas/protegidas em duas assemblies, usando `System.Reflection.Metadata` sem carregar ou executar as assemblies inspecionadas.
Executa no SDK .NET 8 ou superior e não depende de pacotes externos. O CoreLib analisado continua sendo o CoreLib próprio do IronNet.

## Referência fixa

Use o pacote oficial `NETStandard.Library` 2.0.3:

https://api.nuget.org/v3-flatcontainer/netstandard.library/2.0.3/netstandard.library.2.0.3.nupkg

SHA-256 do pacote usado nesta revisão: `3EB87644F79BCFFB3C0331DBDAC3C7837265F2CDF58A7BFD93E431776F77C9BA`.
Extraia o arquivo `build/netstandard2.0/ref/netstandard.dll`. Não substitua pela facade netstandard.dll de um runtime nem pela referência 2.1.

## Executar

```powershell
dotnet build corlib/corlib.csproj -c Debug
dotnet run --project tools/ApiAudit -- <reference/netstandard.dll> corlib/bin/Debug/net8.0/corlib.dll build/conformance/netstandard2.0
```

Saídas: `summary.md` (totais e namespaces) e `api-gaps.json` (cada tipo e assinatura ausente, hashes SHA-256 das entradas).
Código de saída 0 significa que o inventário foi gerado, não que o CoreLib seja conforme. Entradas sem TypeDefs visíveis são rejeitadas para evitar comparar facades vazias.

O self-check compara a referência consigo mesma: deve encontrar 2.417 tipos e 32.559 assinaturas, sem diferenças.

## Escopo da comparação

- Inclui métodos, construtores, campos, propriedades e eventos; accessors também aparecem como métodos.
- Distingue retorno, parâmetros, genericidade, instance/static, arrays, ponteiros, byrefs, modificadores e convenção de chamada.
- Identifica tipos aninhados pela cadeia completa dos tipos externos.
- Compara membros declarados, sem expandir herança. Presença do nome de um tipo não valida seus atributos, bases ou interfaces.
- Não valida constraints, constantes/defaults, todos os atributos, comportamento, ABI nem binding entre assemblies.
- Normaliza nomes entre a referência e corlib, ignorando a identidade do assembly apenas para este inventário. A VM ainda precisa preservar essa identidade.
- Não usa a contagem como percentual de conformidade .NET Standard.

## Comparação de comportamento

`tools/ConformanceHost` executa um exemplo contra o .NET instalado:

```powershell
dotnet run --project tools/ConformanceHost
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/EnumeratorContractTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/ValueCopyTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/CancellationRegistrationTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/NestedFinallyTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/ReadOnlyCollectionTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/AggregateExceptionTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/NullableBoxingTest.cs
dotnet run --project tools/ConformanceHost -p:TestSource=../../examples/BasicContractsTest.cs
```

O exemplo padrão é SynchronizationContextTest. Compare com a execução do mesmo fonte no IronNet.
Para verificar binding de contratos, compile também exemplos com csc usando exclusivamente `-nostdlib -reference:<reference/netstandard.dll>` e execute no IronNet com nosso corlib.
Esse teste binário é diferente de recompilar o exemplo contra as assinaturas oferecidas pelo próprio CoreLib.
