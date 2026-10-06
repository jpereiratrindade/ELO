# ELO-ADMIN-001 — Constituição da Administração Local e Curadoria

## v0.1.0 — design-candidate

> **“O ELO possui um plano de experiência e um plano editorial independentes. O runtime consome conteúdo publicado; o sistema administrativo produz, valida, versiona e ativa esse conteúdo sem modificar o software executável.”**

```context-metadata+json
{
  "document": {
    "id": "ELO-ADMIN-001",
    "version": "0.1.0",
    "status": "design-candidate",
    "title": "Constituição da Administração Local e Curadoria",
    "date": "2026-10-05"
  },
  "system": {
    "name": "ELO",
    "kind": "offline-first contextual experience system",
    "primary_form_factor": "interactive totem",
    "target_substrate": "Raspberry Pi 5",
    "language": "C++26",
    "control_plane": "Unix Domain Socket (/run/elo/control.sock)",
    "admin_backend": "C++26 native HTTP server + Web UI estática"
  },
  "depends_on": [
    "ELO-CONSTITUTION-001",
    "ELO-EXPERIENCE-001",
    "ELO-CONTENT-001",
    "ELO-PUBLISHING-001"
  ]
}
```

---

## 1. Propósito e Filosofia

Este documento estabelece as bases constitucionais da administração local, estúdio editorial e curadoria do sistema ELO em instalações autônomas (como totens em parques, reservas e museus).

O ELO não é administrado pela tela interativa do totem — esta pertence integralmente à experiência pública dos visitantes, sem mouses, teclados ou janelas de configuração. A administração do ELO é concebida como um **sistema editorial local e soberano**, operado via rede local protegida por curadores e educadores a partir de seus próprios dispositivos (smartphones, tablets ou laptops).

---

## 2. Os Três Ciclos de Vida Independentes

O ELO reconhece três dimensões constitutivas que evoluem em ritmos completamente diferentes:

```text
REALIZAÇÃO
Hardware físico, substrato Raspberry Pi 5, câmera fixa, áudio,
instalação física e identidade do totem (elo://...).
       │ (Muda a cada anos / manutenção física)
       ▼
SOFTWARE
Motor de experiência (ExperienceEngine), visão contínua, biometria local,
máquina de estados, drivers e integridade determinística.
       │ (Muda por releases / atualizações de firmware)
       ▼
CONTEÚDO
Curadoria editorial, átomos, narrativas, cantos de aves,
vídeos, perguntas, receitas e relações ecológicas.
       │ (Muda continuamente pela curadoria)
```

A independência destas três camadas impede que atualizações editoriais exijam recompilação de código ou intervenção de desenvolvimento.

---

## 3. Arquitetura de Planos: Experiência vs. Editorial

O sistema divide-se estritamente em dois planos:

```text
                         ELO
                          │
          ┌───────────────┴────────────────┐
          │                                │
      Runtime                         Content Studio
    experiência                       curadoria local
          │                                │
          ▼                                ▼
     elo-kiosk                         elo-admin
  (Qt Quick C++)                    (Nativo C++26)
          │                                │
          │                           draft content
          │                                │
          │                         validate + preview
          │                                │
          │                              publish
          │                                │
          └───────────────┬────────────────┘
                          ▼
                 Content Bundle vN
                          │
              ┌───────────┴───────────┐
              │                       │
           catálogo                 assets
           semântico             imagem/áudio/vídeo
```

### Regra de Ouro do Isolamento
> **O `elo-admin` NUNCA modifica o catálogo ativo diretamente.**

Todas as alterações ocorrem no espaço de trabalho editorial (`draft/staging`). O conteúdo só se torna público após validação rigorosa e ativação atômica pelo protocolo de publicação ([ELO-PUBLISHING-001](file:///home/jpereiratrindade/dev/cpp/ELO/docs/constitution/ELO-PUBLISHING-001.md)).

---

## 4. Estados Editoriais de Conteúdo

O ciclo de vida de uma entidade de conteúdo (átomo, receita ou bundle) obedece à máquina de estados editorial:

```text
DRAFT
  ↓
VALID
  ↓
PREVIEWED
  ↓
PUBLISHED
  ↓
ACTIVE
  ↓
SUPERSEDED
```

1. **DRAFT:** O curador redige textos, vincula mídias ou importa rascunhos.
2. **VALID:** O conteúdo passou integralmente pelas verificações semânticas e de mídia de `elo::content::ContentValidator`.
3. **PREVIEWED:** O curador testou a pré-visualização da experiência no navegador ou simulador.
4. **PUBLISHED:** O bundle recebeu hash de conteúdo (`sha256`) e foi selado no diretório de bundles.
5. **ACTIVE:** O link atômico `current` aponta para este bundle e o runtime o está consumindo.
6. **SUPERSEDED:** Substituído por uma nova versão, permanecendo em disco para fins de histórico ou rollback.

---

## 5. Control Plane Local via IPC

A comunicação entre o `elo-admin` e o `elo-kiosk` acontece de forma privada e determinística no nível do sistema operacional:

```text
[elo-admin] ──(Unix Domain Socket: /run/elo/control.sock)──► [elo-kiosk]
```

### Mensagens do Control Plane:
* `CONTENT_PUBLISHED { bundle_id, hash }`: Notifica que um novo bundle foi ativado atomicamente.
* `CONTENT_RELOAD`: Solicita ao kiosk recarregar o catálogo em tempo de execução sem piscar a tela ou reiniciar o processo.
* `STATUS_REQUEST / STATUS_REPORT`: Verifica saúde dos subsistemas (visão, áudio, armazenamento, geração de kernel).
* `AUDIT_EVENT { event }`: Registra ações administrativas no log soberano.

Nenhuma porta de rede ou endpoint HTTP é aberto pelo `elo-kiosk`. Ele apenas ouve no socket Unix local.

---

## 6. Distinção Constitucional: Julgamento (JEV) vs. Event Log vs. Métricas

Para evitar ambiguidades semânticas:

```text
JEV (Judgment Event)
= Julgamento puro do domínio (Contexto → Julgamento → Score | Choice | Noul)

ExperienceEventStore (Event Log)
= Registro factual e cronológico do que aconteceu (presence.detected, recipe.completed...)

ExperienceMetrics
= Agregações e indicadores analíticos soberanos locais
```

O painel de monitoramento do `elo-admin` consome `ExperienceMetrics` e `ExperienceEventStore`, sem confundir fatos da experiência com eventos ontológicos do JEV.

---

## 7. Decisão Tecnológica: Runtime Unificado em C++26

O backend do `elo-admin` é implementado em **C++26 nativo**, compartilhando as mesmas bibliotecas de domínio do ELO (`libelo_core.a`):

1. **Mesmo Validador:** Reutiliza diretamente `elo::content::ContentValidator` e `elo::content::ContentCatalog`.
2. **Sem Sobrecarga no Raspberry Pi:** Elimina a necessidade de empacotar runtimes extras como Python/FastAPI/uvicorn e suas árvores de dependências voláteis no totem embarcado.
3. **Web UI Estática e Rica:** Servida pelo servidor C++ como assets puros (HTML5, Vanilla CSS com tipografia e cores do design system ELO, JavaScript moderno sem build steps pesados).

---

## 8. Invariantes de Segurança da Administração

| Invariante | Definição |
|---|---|
| **A1 — Disabled by Default** | O serviço administrativo pode ser pausado ou desabilitado por jumper físico ou chave de configuração, permanecendo dormente quando o totem estiver sem curador. |
| **A2 — Local Network Only** | O servidor administrativo nunca escuta em interfaces públicas de internet; liga-se estritamente à rede local (WLAN/LAN privada do totem). |
| **A3 — Local Authentication** | Toda sessão administrativa exige autenticação local com credenciais provisionadas na instalação física. |
| **A4 — Audit Log** | Toda alteração de conteúdo ou publicação é registrada no log soberano com carimbo de tempo e identificador de curador. |
| **A5 — Kiosk Sovereignty** | Uma falha, crash ou indisponibilidade do `elo-admin` NUNCA afeta a operação do `elo-kiosk`. O kiosk continua operando a partir do bundle ativo. |
