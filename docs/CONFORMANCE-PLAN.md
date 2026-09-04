# IronNet: plano de conformidade e continuidade

Atualizado em 2026-09-04. Este documento reconstrói as prioridades acordadas na conversa e registra evidências do código atual.
Não é uma declaração de conformidade completa nem uma revisão manual concluída de todas as linhas do projeto.

## Objetivos e critérios

- Implementar ECMA-335, separando execução válida, verificação de IL e rejeição segura de metadados/IL inválidos.
- Implementar o contrato .NET Standard 2.0 com CoreLib próprio. Também aceitar bibliotecas compiladas contra a referência oficial, sem recompilá-las contra nosso CoreLib.
- Manter backends Windows/POSIX separados e validar Windows/Linux/macOS/AIX, 32/64 bits e little/big endian.
- Tratar warnings como erros, manter até 240 caracteres por linha e preferir código legível, sem implementações artificiais para satisfazer testes.
- Usar testes diferenciais, testes compilados contra a referência oficial, sanitizers e ensaios de concorrência; presença de API não demonstra sua correção.

Referências: [ECMA-335, 6ª edição](https://ecma-international.org/publications-and-standards/standards/ecma-335/) e
[.NET Standard](https://learn.microsoft.com/en-us/dotnet/standard/net-standard).

## Organização do CoreLib

- Pastas acompanham os namespaces públicos: `System/Collections/Generic`, `System/Collections/ObjectModel`, `System/Threading/Tasks` e assim por diante.
- O .NET Standard especifica APIs, não uma árvore obrigatória de fontes. Contratos públicos devem seguir a referência, sem copiar dependências internas específicas do runtime da Microsoft.
- Novos tipos públicos devem preferir arquivos próprios, com auxiliares compartilhados por responsabilidade. Ainda existem arquivos antigos agrupando tipos e até namespaces diferentes; sua separação deve preservar contratos e testes.
- Código gerenciado fica em `corlib`; internal calls e mecanismos da VM ficam no código nativo; operações dependentes de OS/ABI devem usar os backends da plataforma.
- O build usa o SDK com `net8.0`, mas desabilita a biblioteca padrão e as referências implícitas ao framework. Isso não significa que o CoreLib já implementa .NET 8 ou .NET Standard 2.0.
- A separação futura em bibliotecas/facades depende de resolver identidade de assemblies e type forwarding. Namespace, pasta e assembly são conceitos distintos; nem todo o contrato precisa residir no assembly fundamental.

## Inventário reproduzível de APIs

Ferramenta: `tools/ApiAudit`. Referência: `NETStandard.Library` 2.0.3, arquivo `build/netstandard2.0/ref/netstandard.dll` do pacote oficial.

Resultado após a continuidade de Array/coleções: **222 / 2.417 tipos** e **2.004 / 32.559 assinaturas declaradas** presentes.
Faltam 2.195 tipos e 30.555 assinaturas sob os critérios da ferramenta. O número de assinaturas inclui accessors e difere da contagem de APIs da documentação Microsoft.

O relatório percorre todas as definições públicas/protegidas da referência, incluindo tipos aninhados, métodos, campos, propriedades e eventos.
Assinaturas incluem tipos de retorno, argumentos genéricos, parâmetros, byrefs, ponteiros, arrays, modificadores e convenções de chamada.

Limites: não verifica comportamento, herança, implementação de interfaces, constraints, valores constantes/default, todos os atributos ou resolução binária.
Normaliza a identidade do assembly ao comparar nomes de tipos. Membros herdados não são expandidos. Portanto, o relatório é um inventário, não uma certificação.

- Resumo versionado: `docs/API-INVENTORY.md`.
- Relatório detalhado regenerável: `build/conformance/netstandard2.0/api-gaps.json`.
- Instruções e limites da ferramenta: `tools/ApiAudit/README.md`.

## Correções implementadas nesta rodada

- `SynchronizationContext.Current` por thread, `Send`, `Post`, `CreateCopy` e notificações de operações.
- `Task`/`ValueTask.ConfigureAwait` respeitam captura; `await` e `Task.Yield` usam o contexto; builders restauram o contexto do chamador.
- `async void` notifica início/fim e entrega exceções ao contexto capturado.
- `ConfiguredValueTaskAwaitable` está em `System.Runtime.CompilerServices`, conforme o contrato.
- Enumeradores públicos por valor de `List<T>`, `Queue<T>`, `Stack<T>` e `HashSet<T>`, com implementações explícitas de `IEnumerable<T>`.
- Estados de enumeração corrigidos: `Stack` não reinicia após o fim; `Queue`/`Stack` respeitam descarte; `IEnumerator.Current` valida estados inválidos.
- `MemberRef` de tipos genéricos aninhados usa o resolvedor de tipos e o leitor de assinaturas compartilhados, incluindo tokens comprimidos de quatro bytes.
- Dispatch consulta `MethodImpl` antes da busca por nome, preservando implementações explícitas diferentes para contratos diferentes.
- `ldloc`, `ldarg` e `dup` copiam structs; atribuições preservam armazenamento de variáveis referenciadas por managed byrefs.
- Testes novos: `SynchronizationContextTest`, `EnumeratorContractTest`, `ValueCopyTest`.

### Continuidade: cancelamento e handlers aninhados

- `CancellationTokenRegistration` mantém a identidade do registro por referência, inclusive após cópias/descarte, sem reutilização de IDs inteiros.
- Descarte retira callbacks pendentes mesmo após o início de `Cancel`; callbacks já executando são aguardados, exceto no descarte pelo próprio callback.
- Claim/retirada/completude usam o mesmo monitor. A execução do callback ocorre fora dele; todos os descartes aguardando são sinalizados também quando o callback lança exceção.
- Registros liberam delegates/estado após execução, retirada ou abandono por `Cancel(true)`; linked sources não lançam por descarte concorrente já observado pelo callback de propagação.
- `CancelAfter` não cria uma nova thread quando a fonte já está cancelada. O scheduler de timers compartilhados ainda está pendente.
- A VM mantém uma pilha de continuações de `finally`/`fault`; handlers aninhados não sobrescrevem o destino de um `leave` externo.
- Exceções suspensas durante unwind permanecem como roots do GC. Transições de handlers e remoção dessas roots são protegidas pelo lock de execução.
- `leave` longo usa o deslocamento completo de 32 bits e leitura little endian compartilhada; ambos os formatos validam operandos e limites do destino.
- `endfinally` rejeita execução sem handler ativo ou fora dele. Isso não substitui validação de regiões e fronteiras de instruções por um verificador completo.
- Regressões: `CancellationRegistrationTest`, `NestedFinallyTest` e quatro testes nativos de controle de fluxo, incluindo `fault` e deslocamentos maiores que 64 KiB.

Referência de comportamento consultada: [CancellationTokenRegistration no .NET 8](https://github.com/dotnet/runtime/blob/v8.0.0/src/libraries/System.Private.CoreLib/src/System/Threading/CancellationTokenRegistration.cs).

### Continuidade: ObjectModel, exceções e boxing

- `IReadOnlyCollection<T>`, `IReadOnlyList<T>` e `ReadOnlyCollection<T>`; visão viva da lista, interface não genérica, rejeição de alterações, enumeração, lookup e CopyTo com validações/covariância.
- `List<T>.AsReadOnly`, `Array.AsReadOnly` e interfaces read-only de `List<T>`/vetores. A VM adapta os accessors dos vetores, inclusive covariância de referências.
- `AggregateException` movido para arquivo próprio; construtores não serializados, snapshot de entradas, `InnerExceptions` read-only estável, `Handle`, `GetBaseException`, mensagens e `ToString`.
- `Flatten` agora é iterativo e breadth-first, preservando folhas repetidas e evitando duplicação das mensagens de agregados aninhados.
- `Exception.GetBaseException`, `Exception.GetType`, igualdade tipada/operadores de `Type`, `String.Contains(string)` ordinal e `Environment.CurrentManagedThreadId`.
- Boxing real de `Nullable<T>` retorna null ou uma caixa de T; unboxing recompõe o wrapper por seus offsets de campos, sem assumir alinhamento ou tamanho de ponteiro.
- Boxing compartilhado distingue null válido de falha e mantém a origem como root durante alocações. Reflection e `Array.GetValue/SetValue` usam as conversões de nullable.
- Cópias de referências com narrowing em `Array.Copy` preservam o prefixo já copiado quando um elemento posterior é incompatível.
- `EventWaitHandle` para eventos sem nome e `EventResetMode`; `ManualResetEvent`/`AutoResetEvent` compartilham implementação, com Set/Reset/WaitOne, descarte e proteção por monitor.
- Regressões: `ReadOnlyCollectionTest`, `AggregateExceptionTest`, `NullableBoxingTest`, `BasicContractsTest`; consumidores de `InnerExceptions` passaram de Length para Count.

Referências consultadas: [AggregateException](https://github.com/dotnet/runtime/blob/v8.0.0/src/libraries/System.Private.CoreLib/src/System/AggregateException.cs),
[ReadOnlyCollection](https://github.com/dotnet/runtime/blob/v8.0.0/src/libraries/System.Private.CoreLib/src/System/Collections/ObjectModel/ReadOnlyCollection.cs) e
[boxing de nullable](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/builtin-types/nullable-value-types#boxing-and-unboxing).

As APIs assíncronas ainda não possuem `ExecutionContext`/`AsyncLocal` e não equivalem à implementação completa de Tasks no .NET.
O dispatch ainda necessita revisão para slots virtuais, reimplementação/herança de interfaces, variância e outras combinações de MethodImpl.

### Continuidade: Array e coleções não genéricas

- `Array` implementa `IList`, `ICollection` e `ICloneable`; flags, indexador, lookup, tamanho fixo e `SyncRoot` compartilhado com wrappers read-only.
- `IList.Clear` limpa o armazenamento sem alterar o tamanho, inclusive em arrays multidimensionais. O contrato genérico continua rejeitando Clear, conforme o .NET.
- `Clone` reutiliza `MemberwiseClone`, preservando o payload inteiro do array: tipo, dimensões, limites e referências rasas. Testes incluem structs com referências mantidas exclusivamente pelo clone após GC.
- Overloads de `IndexOf` e `Reverse` não genéricos respeitam lower bounds; ranges são validados com intermediários de 64 bits para evitar overflow de índices.
- `CopyTo(Array, int/long)` e correções de `Copy`/`Clear`: índices relativos ao primeiro lower bound, validação de rank e cópia linear entre formas diferentes de mesmo rank.
- `RankException` em arquivo próprio, com construtores não serializados e mapeamento do erro nativo. Falhas de intervalo em Copy distinguem ArgumentOutOfRangeException de ArgumentException.
- `SetValue(null, ...)` zera o elemento de qualquer tipo, incluindo structs e Nullable, sem alocar uma caixa intermediária.
- Regressão: `ArrayCollectionTest`, executada também no .NET 8 e como binário compilado contra a referência .NET Standard 2.0.

Referência de contratos: [System.Array no .NET](https://github.com/dotnet/runtime/blob/v8.0.0/src/libraries/System.Private.CoreLib/src/System/Array.cs).
Esta etapa não completa as conversões entre tipos de `Array.Copy`/`SetValue`, interfaces estruturais, sorting ou todos os overloads de Array.

## Validação desta rodada

- CoreLib e builds MSVC x64/x86/ASan: concluídos com warnings como erros.
- Em cada configuração Windows, após a continuidade de Array/coleções: 29/29 testes nativos e 61/61 exemplos gerenciados com `--gc-threshold 1`.
- O binário de `EnumeratorContractTest` compilado exclusivamente contra a referência .NET Standard 2.0 passou nas três configurações, usando nosso CoreLib na execução.
- `SynchronizationContextTest`, `EnumeratorContractTest` e `ValueCopyTest` passaram também no .NET 8 pelo ConformanceHost.
- `CancellationRegistrationTest` e `NestedFinallyTest` passaram no .NET 8; cancelamento inclui retirada concorrente, múltiplos descartes, falhas, reentrância e corridas de registro/linked sources.
- Repetição de `CancellationRegistrationTest`: 5/5 execuções adicionais em cada build x64/x86/ASan, com GC agressivo e 32 iterações por cenário de corrida em cada execução.
- `NestedFinallyTest` compilado exclusivamente contra .NET Standard 2.0 passou em x64/x86/ASan com nosso CoreLib.
- Os quatro novos testes passaram no .NET 8. Eles e `CancellationRegistrationTest` passaram como binários compilados contra .NET Standard 2.0 em x64/x86/ASan usando nosso CoreLib.
- `ArrayCollectionTest` passou no .NET 8 e na VM x64/x86/ASan, inclusive compilado exclusivamente contra a referência .NET Standard 2.0.
- ApiAudit regenerado: 222/2.417 tipos e 2.004/32.559 assinaturas; ganho de 1 tipo e 17 assinaturas na etapa de Array.
- ApiAudit comparou a referência consigo mesma: 2.417/2.417 tipos e 32.559/32.559 assinaturas, sem diferenças.
- AddressSanitizer não reportou erros nestas execuções. Isso não elimina a necessidade de ampliar os testes de memória/concorrência.
- Linux, macOS, AIX e hardware big endian não foram executados nesta rodada.

## Sequência de implementação

### P0 — segurança da VM, identidade e execução

1. Verificação central de IL: tamanho dos operandos, stack underflow/overflow, `maxstack`, categorias de valores, alvos de branch, entrada em regiões EH e combinações de prefixes.
2. Identidade de assemblies/tipos: `AssemblyRef`, módulos, `ExportedType`, forwarding, colisões de nomes, resolução de dependências e facades .NET Standard.
3. Completar semântica de prefixes e chamadas: `tail.`, `readonly.`, `constrained.`, `volatile.`, `unaligned.`, `calli`, `jmp` e varargs.
4. Semântica por valor: cópias/boxing/retornos, refs interiores e genéricos; expandir regressões além das cópias corrigidas nesta rodada.
5. Auditar GC: roots temporárias durante chamadas nativas, finalização/ressurreição, weak handles, pinning e coordenação entre threads.
6. PE/metadata hostis: limites/overflow, tabelas/coded indices, blobs recursivos e assinaturas truncadas; adicionar corpus negativo e fuzzing.
7. Completar tratamento de exceções em duas passagens: busca/filtros antes de unwind, exceções não tratadas, `rethrow` em catches aninhados e validação de regiões EH.

Evidências atuais:

- Em `src/runtime/exec.c`, `tail_prefix` e `readonly_prefix` são definidos e zerados; ainda não há consumo que implemente sua semântica completa.
- Em `src/runtime/runtime.c`, TypeRef continua procurando por namespace/nome entre assemblies carregados. Identidade completa ainda não está garantida.
- As instruções de variáveis receberam validação de índices/operandos nesta rodada; isso não constitui um verificador de IL completo.
- `MethodImpl` já tinha leitura de metadata, mas não governava o dispatch explícito; a correção desta rodada adicionou essa consulta.
- O loop de `endfinally` causado por continuações sobrepostas foi corrigido. A busca atual ainda intercala descoberta de handlers e unwind; não representa o algoritmo completo de duas passagens da CLI.

### P1 — fundamentos do CoreLib e assincronismo

1. `ExecutionContext`, `AsyncLocal`, captura/supressão/restauração e distinção semântica entre `OnCompleted` e `UnsafeOnCompleted`.
2. ThreadPool e timers compartilhados; TaskScheduler/TaskFactory, opções de criação/continuação, cancelamento, falhas de agendamento e propagação de exceções não tratadas entre threads e host.
3. `CancellationTokenRegistration.Dispose`: retirada/espera/reentrância implementadas e testadas. Próximos contratos: overloads de registro, captura de contextos e integração com timers compartilhados.
4. Builders: estados default, associação de state machines, exceções e completude; otimizar ValueTask síncrono sem sacrificar comportamento.
5. `ExceptionDispatchInfo`, tipos básicos, `Decimal`, `Convert`, parsing/formatting e cultura.
6. DateTime/DateTimeOffset/TimeSpan: calendários, UTC/local, transições DST, parsing/formatting e limites.

O contrato de `AggregateException.InnerExceptions` foi corrigido e validado por execução binária contra a referência oficial.
Permanecem pendentes a infraestrutura de serialização e os dois membros serializados de `AggregateException`, além de eventos nomeados/handles nativos e demais overloads de WaitHandle.
Não confundir o suporte entregue a eventos sem nome com suporte a eventos interprocesso ou a todo o namespace System.Threading.

### P2 — coleções, texto e LINQ

1. Dictionary: enumeradores públicos incluindo Keys/Values, contrato de IDictionary não genérico, CopyTo e validação completa dos argumentos.
2. List/Array: sorting/searching, interfaces read-only, covariância e contratos genéricos/não genéricos; continuar o inventário de overloads.
3. Coleções read-only, object model, não genéricas, especializadas e concorrentes.
4. LINQ: GroupBy/Lookup/Join/GroupJoin e demais overloads; avaliação adiada, descarte e casos vazios/nulos.
5. Encoding/Decoder/Encoder, fallbacks, String/StringBuilder, comparações e Globalization.

O restante de ObjectModel segue pendente. `System.Array` agora expõe os contratos não genéricos e compartilha SyncRoot com wrappers sobre arrays, mas ainda precisa das conversões de elementos completas e das interfaces estruturais.
Próxima etapa de Array: widening de primitivos, boxing/unboxing em Copy, ArrayTypeMismatchException versus InvalidCastException, tipos incompatíveis em cópias vazias e demais overloads de busca/ordenação.

### P3 — I/O, interop e demais namespaces do contrato

1. File/Directory/Path/FileStream, handles seguros, operações assíncronas e erros por plataforma.
2. P/Invoke/ABI/marshalling: cdecl/stdcall, structs, strings, callbacks e last error; distinguir implementações completas de APIs indisponíveis por plataforma.
3. Reflection/attributes/resources, serialização, expressions e emit previstos no contrato.
4. Networking/HTTP/sockets/TLS, criptografia, compressão, XML, regex, componentes e dados.
5. Expandir por dependência a partir do relatório por namespace, incluindo todos os grupos presentes na referência — não apenas System e Collections.

### P4 — matriz de plataformas e conformidade final

1. CI Windows x86/x64, Linux e macOS com GCC/Clang, warnings como erros e testes diferenciais.
2. Execução big endian e arquiteturas adicionais; teste nativo ou emulação adequada, sem inferir validação a partir de helpers de bytes.
3. AIX/POWER: compiladores, atomics/alinhamento, pthreads, clocks, loader e ABI.
4. Consolidar backends: ainda há seleção de OS em `src/core/platform.c`; mover operações nativas para os backends sem dispersar condicionais.
5. Mapear as demais partições ECMA-335: CTS/CLS/VES, metadata, instruções, perfis/bibliotecas e intercâmbio de depuração; registrar critérios verificáveis por seção.
6. Só declarar suporte quando contratos, execução binária e matriz de plataformas estiverem demonstrados.

## Regra de encerramento de cada etapa

Registrar APIs/semânticas entregues, testes realizados, divergências restantes e plataformas realmente executadas.
Atualizar o inventário e este plano. Não substituir APIs ausentes por stubs, nem considerar um `case` de opcode como prova de conformidade.
Não marcar uma revisão manual completa enquanto arquivos e seções ECMA ainda estiverem sem auditoria aprofundada.
