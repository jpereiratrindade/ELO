# ELO-G0-RESULTADOS — Contenção e Segurança

**Data:** 15/10/2026

**Base examinada:** `main` em `933457597c3ff315cdd3499ccf757c8ac5d41dcd`

**Estado inicial:** worktree limpo; branch alinhada a `origin/main`
**Referência:** ELO-DEV-001 e ELO-RECONCEPCAO-001 v0.1.0

## 1. Decisão

G0 está concluído no escopo de contenção definido em ELO-DEV-001. Não foi encontrado
bloqueio crítico remanescente no fluxo produtivo examinado. O avanço para G1 é
recomendado, sem piloto público e sem reativação de biometria.

Esta decisão não homologa segurança completa, hardware, acessibilidade ou apagamento
físico de dados. Esses pontos permanecem explicitamente fora das evidências executadas.

## 2. Validação dos achados P0

| ID | Resultado da reprodução | Intervenção | Estado após G0 |
|---|---|---|---|
| A-01 | **CONFIRMADO.** `AdminHttpServer::start()` usava `QHostAddress::Any`; todas as rotas administrativas eram alcançáveis sem autenticação. | Bind alterado para `QHostAddress::LocalHost`; token Bearer obrigatório para `/api/*`; configuração ausente ou token menor que 32 bytes impede início. | **CONTIDO E TESTADO.** |
| A-02 | **CONFIRMADO.** Toda resposta anunciava `Access-Control-Allow-Origin: *` e métodos mutáveis. | Cabeçalhos CORS permissivos removidos. Studio opera em mesma origem; token não é liberado para origens externas. | **CORRIGIDO E TESTADO.** |
| A-03 | **CONFIRMADO.** Presença chamava `begin_automatic_continuity()` e autorizava extração biométrica sem decisão voluntária. | Presença cria somente `PRESENCE_DETECTED`; autorização biométrica permanece falsa. | **CORRIGIDO E TESTADO.** |
| A-04 | **CONFIRMADO.** Face desconhecida no fluxo produtivo chegava a `identify_or_enroll_local_face()` e era matriculada. | Apresentação produtiva ignora embeddings e nunca chama identificação/matrícula. APIs internas antigas ficam isoladas como experimentais para tratamento posterior. | **CONTIDO E TESTADO:** zero templates após três embeddings. |
| A-05 | **CONFIRMADO.** `request_forget()` ignorava três resultados e sempre retornava sucesso; não abrangia WAL, backups ou eventos. | Falhas agora são propagadas e UI não anuncia sucesso parcial. Terminologia mudou para desvinculação; garantia de apagamento físico instantâneo foi retirada. | **CONTIDO, NÃO ENCERRADO COMO POLÍTICA DE DADOS.** Transação única, eventos, WAL e backups ficam para G2; nenhum dado existente foi apagado. Como biometria produtiva está desabilitada, isso não bloqueia G1, mas bloqueia qualquer reativação de continuidade. |
| A-06 | **CONFIRMADO.** API anunciava ArcFace-512, HMAC/SHA-256, irreversibilidade e isolamento de hardware; runtime usa OpenCV SFace e não demonstra essas garantias. | Endpoint `/api/biometric/spec` removido; painel e comentários corrigidos; `BiometricKey` marcado como legado experimental e fora do fluxo G0. | **CORRIGIDO E TESTADO:** endpoint retorna 404 autenticado. |

Nenhum achado P0 foi refutado. Nenhum ficou inconclusivo por inspeção estática. Alcance
de exploração remota anterior e propriedades do dispositivo físico continuam
inconclusivos porque não houve teste dinâmico da versão antiga em rede nem ensaio no Pi.

## 3. Alterações rastreáveis

1. `a55036a` — `fix(admin): contain and authenticate studio server`
   - loopback por padrão;
   - autenticação obrigatória e falha fechada;
   - CORS permissivo removido;
   - limites de cabeçalhos e corpo;
   - confinamento canônico de assets;
   - remoção de `pkill -f` como fallback;
   - token aleatório no instalador e acesso remoto somente por túnel SSH;
   - teste de integração administrativa.
2. `248144f` — `fix(kiosk): make presence sessions anonymous`
   - presença desacoplada de biometria;
   - embeddings ignorados no fluxo principal;
   - câmera opcional durante experiência;
   - declarações falsas removidas;
   - teste de sessão anônima e ausência de matrícula.
3. `7efcc41` — `fix(privacy): stop reporting partial forgetting as success`
   - erros de cada armazenamento propagados;
   - UI deixa de anunciar esquecimento completo;
   - documentação distingue desvinculação de apagamento físico.

## 4. Testes e evidências

### Baseline anterior às mudanças

- `cmake --build build -j2`: **PASS** (`ninja: no work to do`).
- `ctest --test-dir build --output-on-failure`: **PASS, 7/7**.
- Limitação: testes antigos confirmavam e exigiam matrícula facial automática; passar
  nessa baseline não demonstrava conformidade com G0.

### Estado final

- Configuração CMake com modelos locais fixados e download desabilitado: **PASS**.
- Build completo: **PASS**.
- CTest: **PASS, 8/8**, 0 falhas, 2,13 s.
- `AdminSecurity`: **PASS** para falha fechada sem token, bind loopback, `401` em mutação
  não autenticada, ausência de CORS wildcard, remoção da especificação biométrica e
  bloqueio de traversal de asset.
- `KioskPresentation`: **PASS** para sessão anônima, ausência de identidade, ausência de
  templates após embeddings, início de conteúdo sem identificação e continuidade do
  conteúdo após falha da câmera.
- Demais sete suítes de domínio, armazenamento, câmera, conteúdo e publicação: **PASS**.
- `bash -n scripts/install_rpi5.sh`: **PASS**.
- `git diff --check`: **PASS** antes dos commits.

### Não executado

- Teste físico em Raspberry Pi 5 / Touch Display 2: **NOT_RUN** — hardware não estava no escopo disponível.
- Tentativa de conexão pela LAN a partir de outra máquina: **NOT_RUN** — sem segundo host; propriedade de bind foi verificada por API Qt no teste.
- Pentest/fuzzing do parser HTTP: **NOT_RUN**.
- Migração, exclusão ou inspeção de banco biométrico real: **NOT_RUN** por restrição explícita de preservação.
- Validação de WAL, backup, restauração e apagamento físico: **NOT_RUN**, destinado a G2.

## 5. Componentes

### Removidos do fluxo produtivo

- continuidade biométrica acionada por presença;
- identificação e matrícula facial automáticas;
- autorização automática para geração de embeddings;
- endpoint de garantias biométricas;
- CORS wildcard;
- bind administrativo em todas as interfaces;
- fallback de controle por `pkill -f elo-kiosk`.

### Desabilitados ou isolados

- APIs e tipos internos de biometria permanecem compiláveis somente como legado
  experimental; não são chamados pelo caminho de apresentação G0;
- administração não inicia sem token válido;
- administração remota direta por LAN está desabilitada.

### Preservados

- detecção opcional e não identificadora de presença;
- experiência, conteúdo multimodal, receitas e seleção contextual;
- SQLite e registros existentes;
- publicação, bundles, rollback e IPC autenticado indiretamente pelo servidor local;
- câmera como adaptador opcional.

### Pendentes

- substituir parser HTTP manual por componente mantido ou submetê-lo a testes negativos amplos;
- timeouts explícitos por conexão e limites mais granulares por rota/upload;
- transação coordenada de desvinculação e política para eventos, WAL e backups;
- remover ou migrar definitivamente código biométrico experimental;
- revisar documentos constitucionais históricos, que ainda descrevem desenho anterior;
- gestão operacional mais forte de segredo e sessão administrativa;
- ensaio físico, CI e análise dinâmica de segurança.

## 6. Riscos residuais

1. Parser HTTP continua manual. Limites reduzem exposição, mas não equivalem a parser
   maduro ou prova contra request smuggling e negação de serviço.
2. Bearer token fica na sessão do navegador. Loopback e túnel SSH reduzem alcance;
   rotação, expiração, usuários e auditoria ainda não existem.
3. `request_forget()` agora falha honestamente, porém operações continuam sequenciais,
   não atômicas. Reativar biometria antes de G2 é proibido.
4. Código biométrico legado ainda compila e testes de baixo nível ainda o exercitam.
   Isso preserva dados e dependências, mas exige remoção/módulo segregado posterior.
5. Ausência de ensaio físico e teste dinâmico impede alegar prontidão para piloto.

## 7. Gate

**G0: PASS para contenção de código; evidência física NOT_RUN.**

**Recomendação:** avançar para G1 em desenvolvimento controlado. Não avançar para piloto
público. Não reativar continuidade biométrica. Tratar riscos acima como entradas
obrigatórias de G1/G2, mantendo resultados `NOT_RUN` visíveis.**
