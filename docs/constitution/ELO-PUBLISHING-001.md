# ELO-PUBLISHING-001 — Protocolo de Empacotamento, Validação e Publicação de Conteúdo

## v0.1.0 — design-candidate

> **“O conteúdo só se torna público se passar pelo mesmo validador que o runtime utiliza. Toda publicação é atômica, versionada e reversível.”**

```context-metadata+json
{
  "document": {
    "id": "ELO-PUBLISHING-001",
    "version": "0.1.0",
    "status": "design-candidate",
    "title": "Protocolo de Empacotamento, Validação e Publicação de Conteúdo",
    "date": "2026-10-05"
  },
  "system": {
    "name": "ELO",
    "kind": "offline-first contextual experience system",
    "primary_form_factor": "interactive totem",
    "target_substrate": "Raspberry Pi 5",
    "language": "C++26"
  },
  "depends_on": [
    "ELO-CONSTITUTION-001",
    "ELO-EXPERIENCE-001",
    "ELO-CONTENT-001",
    "ELO-ADMIN-001"
  ]
}
```

---

## 1. O Conceito de `ContentBundle`

No ELO, o conteúdo não é distribuído como arquivos soltos ou tabelas fragmentadas de banco de dados. Ele é organizado sob a entidade formal de topo: **`ContentBundle`**.

Um `ContentBundle` é uma unidade fechada, imutável, versionada e criptograficamente assinada por hash, contendo:

1. **Manifesto do Pacote (`manifest.json`):** Metadados, tema padrão, versão semântica e hashes.
2. **Catálogo Semântico (`catalog/`):**
   * Átomos de conteúdo (`atoms/`)
   * Variantes de apresentação (`variants/`)
   * Relações ecológicas/narrativas (`relations/`)
   * Receitas de experiência (`recipes/`)
   * Fontes de autoridade (`sources/`)
3. **Mídias e Ativos Físicos (`assets/`):**
   * Arquivos de áudio (cantos de aves, paisagens sonoras PCM WAV/OGG)
   * Imagens e ilustrações científicas (PNG/WEBP/JPG)
   * Vídeos ou animações (quando aplicável)

---

## 2. Estrutura de Diretórios no Totem

No sistema de arquivos do Raspberry Pi, o repositório de conteúdo organiza-se em:

```text
/var/lib/elo/content/
│
├── bundles/
│   ├── elo-content-pampa-0.1.0/
│   │   ├── manifest.json
│   │   ├── catalog/
│   │   └── assets/
│   │
│   ├── elo-content-pampa-0.1.1/
│   │   ├── manifest.json
│   │   ├── catalog/
│   │   └── assets/
│   │
│   └── elo-content-pampa-0.2.0/
│       ├── manifest.json
│       ├── catalog/
│       └── assets/
│
├── staging/
│   └── bundle-candidate-temp/
│
└── current -> bundles/elo-content-pampa-0.1.1/  (Symlink Atômico)
```

O runtime `elo-kiosk` lê exclusivamente a partir de `/var/lib/elo/content/current/` (ou do caminho configurado via `ELO_CONTENT_DIR`).

---

## 3. Manifesto do Bundle (`manifest.json`)

Cada bundle contém obrigatoriamente um manifesto na sua raiz:

```json
{
  "bundle_id": "elo-content-pampa",
  "version": "0.2.0",
  "schema_version": "0.1",
  "name": "Bioma Pampa — Campos Sulinos e Espécies Ameaçadas",
  "description": "Catálogo de aves, herbívoros e relações ecológicas do Pampa gaúcho.",
  "default_theme": "pampa",
  "curation_revision": 18,
  "created_at": "2026-10-05T22:00:00Z",
  "content_hash": "sha256:4a8b79c3d4f8e...",
  "parent_bundle": "sha256:2b1c88e99a7d3...",
  "compatibility": {
    "min_engine_version": "0.2.0",
    "max_engine_version": "0.3.0"
  }
}
```

### Agnosticismo de Tema no Motor
O campo `"default_theme"` liberta o motor em C++ (`ExperienceEngine`) de conhecer temas específicos como `"pampa"`. O motor consulta `bundle.manifest.default_theme`, permitindo que o mesmo totem execute amanhã:
* `elo-content-morfocampo`
* `elo-content-pantanal`
* `elo-content-museu-historico`
sem alterar uma única linha do código compilado.

---

## 4. O Pipeline de Publicação

A publicação de um novo bundle pelo `elo-admin` segue um pipeline de sete estágios:

```text
1. DRAFT
   Curador insere ou edita dados na área de rascunho
     ↓
2. VALIDATE
   ContentValidator nativo C++ executa 100% dos testes semânticos
     ↓
3. PREVIEW
   Curador inspeciona visualmente no Studio antes de autorizar
     ↓
4. BUILD BUNDLE
   Arquivos são consolidados em staging e o hash sha256 é calculado
     ↓
5. SEAL & STORE
   O diretório é movido para bundles/elo-content-<id>-<version>/
     ↓
6. ATOMIC ACTIVATE
   O symlink 'current' é atualizado via rename atômico de link
     ↓
7. IPC RELOAD
   Sinal 'CONTENT_PUBLISHED' é enviado ao elo-kiosk pelo socket Unix
```

### Invariante de Validação (O Princípio "No Broken Bundles")
> **Um bundle JAMAIS é ativado se falhar na verificação de `elo::content::ContentValidator`.**

O validador verifica:
1. Todos os JSONs aderem aos schemas vigentes.
2. Todo átomo referenciado por uma relação existe.
3. Todo arquivo de mídia referenciado em `presentation.media` ou `subject.audio` existe fisicamente em `assets/` e possui integridade de leitura.
4. Toda receita possui pelo menos um átomo que satisfaz suas dependências (`requires`).

---

## 5. Ativação Atômica e Reversibilidade (Rollback)

Em sistemas POSIX/Linux, a troca do symlink `current` é realizada atomicamente utilizando `renameat` ou substituição de link simbólico temporário:

```bash
ln -sfn bundles/elo-content-pampa-0.2.0 current.tmp && mv -T current.tmp current
```

Isso garante que o leitor no totem:
* **Nunca leia um estado intermediário ou incompleto:** ou lê a versão antiga íntegra, ou a versão nova íntegra.
* **Rollback Imediato:** Caso o operador decida reverter a publicação, um comando administrativo reverte o symlink para a versão anterior:
  ```bash
  ln -sfn bundles/elo-content-pampa-0.1.1 current.tmp && mv -T current.tmp current
  ```
  e envia o sinal de reload para o kiosk.

---

## 6. Rastreabilidade de Experiência e Proveniência

Para viabilizar pesquisa científica, auditoria e replay de experiências, cada sessão de visitante registra no seu log de eventos:

```text
Session Record
  ├── session_id: "sess-20261005-0042"
  ├── realization_id: "elo://S01"
  ├── content_bundle_id: "elo-content-pampa"
  ├── content_bundle_hash: "sha256:4a8b79c3d4f8e..."
  ├── recipe_id: "discover_by_sound"
  ├── active_atom_id: "species_cardeal_001"
  └── events: [...]
```

Dessa forma, o sistema pode sempre responder com precisão matemática:
> **“Com qual versão exata do catálogo esta interação ocorreu?”**
