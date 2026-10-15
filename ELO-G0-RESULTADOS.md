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
4. `217f358` e complementares — `fix(security): harden studio dom, http parser and presence ui`
   - **Hardening DOM no Studio (`web/admin/app.js`):** substituição de todas as concatenações dinâmicas de `innerHTML` por APIs DOM seguras (`document.createElement`, `textContent`, `.value`); token restrito exclusivamente a chamadas `/api/*` de mesma origem.
   - **Hardening do parser HTTP (`apps/elo-admin/admin_http_server.cpp`):**
     - Limite estrito de 16 KiB para buffer de cabeçalho mesmo antes ou após delimitador `\r\n\r\n` (rejeição com `431 Request Header Fields Too Large`);
     - Validação estrita de `Content-Length` (rejeição de duplicidades, negativos, não numéricos ou > 16 MiB com `400 Bad Request`);
     - Rejeição de `Transfer-Encoding` não suportado (ex: `chunked`) com `501 Not Implemented`;
     - Rejeição de cabeçalhos `Host` duplicados ou divergentes com `400 Bad Request`;
     - Rejeição de bytes nulos em cabeçalhos ou requisições malformadas com `400 Bad Request`;
     - Rejeição de métodos HTTP não suportados com `405 Method Not Allowed`;
     - Cabeçalhos de segurança defensiva: `Content-Security-Policy: default-src 'self' 'unsafe-inline'`, `X-Frame-Options: DENY`, `X-Content-Type-Options: nosniff`.
   - **Alinhamento da UI do Kiosk (`ui/Main.qml`):**
     - Substituição de termos biométricos residuais (`IDENTIDADE RECONHECIDA`, `RECONHECENDO`) por indicadores de presença (`● PRESENÇA DETECTADA`, `● AGUARDANDO VISITANTE`);
     - Substituição de diálogo modal bloqueante por banner flutuante discreto, mantendo o conteúdo navegável.
   - **Validação de XSS via navegador real (`tests/test_studio_browser_xss.py`):**
     - Execução automatizada com Firefox headless e perfil efêmero;
     - Prova de zero execuções de scripts ou manipuladores de eventos (`onerror`, `onload`, `onmouseover`) a partir de dados editoriais.

## 4. Testes e evidências

### Baseline anterior às mudanças

- `cmake --build build -j2`: **PASS** (`ninja: no work to do`).
- `ctest --test-dir build --output-on-failure`: **PASS, 7/7**.
- Limitação: testes antigos confirmavam e exigiam matrícula facial automática; passar
  nessa baseline não demonstrava conformidade com G0.

### Estado final consolidado do G0

- Configuração CMake com modelos locais fixados e download desabilitado: **PASS**.
- Build completo sem warnings de compilação: **PASS**.
- CTest: **PASS, 9/9**, 0 falhas, ~5,0 s.
  1. `ConstitutionalInvariants`: **PASS** (invariantes de desacoplamento e não-biometria garantidos).
  2. `CameraCatalog`: **PASS** (descoberta de câmeras V4L2).
  3. `SqliteStorage`: **PASS** (armazenamento relacional local).
  4. `KioskPresentation`: **PASS** (sessão anônima, ausência de matrícula, início e continuidade sem câmera).
  5. `ContentSystem`: **PASS** (integridade de átomos, relações e receitas).
  6. `ContentPublishing`: **PASS** (geração de pacote assinado por hash e persistência de link).
  7. `ContentDatabase`: **PASS** (operações no banco de conteúdo editorial).
  8. `AdminSecurity`: **PASS** (loopback bind, token obrigatório, 431 em cabeçalhos excessivos, 400 em Content-Length e Host duplicados/inválidos, 501 em Transfer-Encoding chunked, 405 em métodos não suportados, asset containment contra path traversal, persistência segura de conteúdo).
  9. `StudioBrowserXss`: **PASS** (teste dinâmico em navegador real headless Firefox: verificação de que títulos, fatos e metadados com `<script>`, `<img onerror>`, `<svg onload>` são renderizados como texto puro no DOM, com 0 execuções de script).
- `git diff --check`: **PASS** (código limpo sem espaços em branco espúrios ou quebras anômalas de EOF).

### Não executado e fora de escopo (NOT_RUN)

- Teste físico em Raspberry Pi 5 / Touch Display 2: **NOT_RUN** — hardware não disponível no ambiente de desenvolvimento.
- Tentativa de conexão pela LAN a partir de outra máquina: **NOT_RUN** — sem segundo host; verificado via bind explícito `QHostAddress::LocalHost` no teste unitário.
- Pentest externo ou fuzzing massivo de rede: **NOT_RUN**.
- Validação de WAL, backup, restauração e apagamento físico transacional: **NOT_RUN**, explicitamente postergado para G2.
- Acessibilidade e ergonomia presencial em campo: **NOT_RUN**, escopo do G1.

## 5. Componentes

### Removidos do fluxo produtivo

- continuidade biométrica acionada por presença;
- identificação e matrícula facial automáticas;
- autorização automática para geração de embeddings;
- endpoint de garantias biométricas falsas (`/api/biometric/spec`);
- CORS wildcard (`*`);
- bind administrativo em todas as interfaces (`QHostAddress::Any`);
- concatenação direta de dados em `innerHTML` no Studio;
- diálogos bloqueantes de reconhecimento facial na interface do Kiosk;
- fallback de controle por `pkill -f elo-kiosk`.

### Desabilitados ou isolados

- APIs e tipos internos de biometria permanecem compiláveis somente como legado
  experimental isolado; não são chamados pelo caminho de apresentação G0;
  não podem ser reativados sem aprovação prévia em G2;
- administração não inicia sem token válido de pelo menos 32 bytes;
- administração remota direta por LAN está desabilitada (acesso remoto condicionado a túnel SSH).

### Preservados e funcionais

- detecção de presença anônima e contextual;
- fluxo completo do Studio: autenticação via Bearer token, listagem de átomos, edição, publicação de bundles, recuperação de catálogo e inspeção de manifestos;
- renderização do Kiosk e navegação multimodal de conteúdo;
- banco de dados SQLite local com integridade referencial;
- empacotamento, cálculo de hash SHA-256 e alternância atômica de symlink `current`;
- adaptadores de câmera operando em modo não-identificador.

### Pendentes para próximos ciclos

- G1: consolidação da experiência presencial voluntária, sem câmera, sem identificação e offline;
- G2: transação coordenada de desvinculação, política atômica para WAL/backups e remoção definitiva do código legado biométrico.

## 6. Riscos residuais

1. **Parser HTTP manual:** a implementação contém proteções sólidas contra excesso de cabeçalhos (16 KiB), métodos inválidos, duplicidade de `Content-Length`/`Host` e `Transfer-Encoding`, mas permanece um parser manual minimalista. O bind loopback estrito mitiga exposição à rede externa.
2. **Ciclo de vida do token administrativo:** o Bearer token é armazenado em `sessionStorage` e transmitido apenas para a mesma origem (`/api/*`). Não há rotação automática de tokens ou múltiplos perfis de usuário neste ciclo.
3. **Persistência e WAL:** `request_forget()` falha honestamente em caso de erro, mas as operações ainda são sequenciais e não abrangem o WAL/backups. Reativar biometria antes do ciclo de dados de G2 permanece proibido.
4. **Ausência de homologação em hardware físico:** sem validação de tela e performance no Raspberry Pi 5 real, não há garantia de taxa de quadros e estabilidade de drivers V4L2/DRM em campo.

## 7. Gate e Conclusão

**G0: APROVADO PARA CONTENÇÃO DE CÓDIGO E SEGURANÇA BÁSICA.**

- **Evidências:** 9/9 testes CTest aprovados, teste automatizado em navegador Firefox headless confirmando blindagem contra XSS, build estrito, conformidade de diff.
- **Recomendação:** encerrar formalmente o G0, consolidar o branch `review/elo-g0`, abrir o Pull Request para `main` e iniciar os trabalhos preparatórios do G1 conforme os critérios de experiência anônima e presencial. Não avançar para piloto público em campo antes dos ensaios de hardware e acessibilidade de G1. Não reativar continuidade biométrica. Tratar riscos acima como entradas obrigatórias de G1/G2, mantendo resultados `NOT_RUN` visíveis.
