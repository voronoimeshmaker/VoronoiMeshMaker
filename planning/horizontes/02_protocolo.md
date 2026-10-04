# Protocolo comum das etapas

Cada P21–P29 incorpora este protocolo por referência. Ler também AGENTS.md,
[sequência anterior](../sequencia_prompts.md), [decisões](../DECISIONS.md),
[linha de base histórica](../P05_BASELINE.md) e as diretrizes atuais em
../../vmm/docs/guidelines.rst.

## Ambiente e preservação

- Usar exclusivamente WSL Ubuntu-26.04-Test, /home/jflavio/Programas/VMM.
- A partir do Windows, chamar sempre wsl -d Ubuntu-26.04-Test.
- Conferir git status antes/depois; preservar alterações do usuário.
- Não fazer commit, tag ou push.
- Validar CMAKE_HOME_DIRECTORY e compilador antes de reutilizar build.
- Não instalar dependências novas sem decisão específica.
- Ler relatórios históricos como evidência histórica, nunca como validação do estado atual.

## Projeto

C++23; composição, concepts, traits e registros abertos. Não introduzir herança, virtual,
enums ou equivalentes de despacho fechado. API sem tipos externos; núcleo sem IO ou CGAL.
Includes: padrão, externos, VMM; ordem alfabética e comentários separadores existentes.
Preservar o contrato estável da série 1.x e o leitor/escritor .vmesh v1.
Não enfraquecer verificações geométricas para fazer casos novos passarem.
A modalidade em camadas não herda uma promessa de ortogonalidade própria de Voronoi irrestrito.

A linha de base contém disposições históricas substituídas: conferir decisões posteriores
sobre compiladores, LTO, arquitetura nativa, escritores e estabilidade. O estado atual não
deve ser inferido apenas de P05 ou do cabeçalho antigo da sequência.

## Iterações e evidência

1. Apresentar plano de iterações por grupos pequenos de classes; seguir revisão do plano anterior.
2. Testar primeiro funções públicas por classe com GTest; depois integração entre classes.
3. Registrar comandos, versão do código, ambiente, resultados, falhas e limitações.
4. Medir cobertura por arquivo segundo R25 (90% linhas / 80% ramos); exceções não são automáticas.
5. Rodar check_requirements, firewall, testes pertinentes e regressões apropriadas.
6. Só depois dos testes adicionar exemplos ao manual (R30).
7. Ao falhar, investigar causa; se o limite de três tentativas do protocolo anterior for
   alcançado, registrar impasse e solicitar decisão. Não inventar evidência.
8. Não iniciar a etapa seguinte sem a revisão prevista na sequência anterior.

## Registro de decisões

Não reabrir instruções confirmadas H01–H10 como se faltasse autorização.
Respostas pendentes Q01–Q10 devem ser registradas literalmente, com consequências.
Novas propostas arquiteturais entram em planning/DECISIONS.md com o próximo número livre
e status PROPOSTA; não reservar números nem alterar entradas históricas.
Este pacote não registra propostas como aprovadas nem altera o registro existente.

## Modelo de relatório

Criar planning/horizontes/relatorios/Pxx_k_relatorio.md somente ao executar a iteração:

- objetivo e requisitos atendidos;
- base do código, estado inicial, ambiente e versões utilizadas;
- arquivos alterados e justificativa;
- decisões confirmadas, propostas e pendências;
- comandos e evidência de testes por classe, integração e regressões;
- cobertura por arquivo e benchmark, quando aplicável;
- limites conhecidos e reprodução de falhas;
- resultado: concluído, parcial ou bloqueado, sem confundir plano com execução.

Entregáveis em português; código e comentários técnicos em inglês.
Revisão adicional em sessão separada é opcional, conforme REV da sequência anterior;
não implica delegação automática.
