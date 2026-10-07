# ELO Content Model v0.2

## Decisão arquitetural

O ELO mantém duas dimensões independentes:

- **Átomo**: registro canônico, reutilizável, endereçado por `content_id` imutável.
- **Aplicação**: experiência editorial editável que reúne metadados, configuração de apresentação e referências a átomos por `atom_ids`.
- **Bundle**: snapshot técnico e imutável gerado ao publicar uma versão da aplicação. Não é um cadastro independente na interface.

Um átomo pode participar de várias aplicações. Editar uma aplicação não duplica o átomo. Publicar uma aplicação cria um bundle somente com os átomos selecionados, suas variantes e as relações internas à seleção.

## Metadados do átomo

O núcleo v0.2 é independente de domínio:

```json
{
  "schema_version": "0.2",
  "content_id": "record_001",
  "type": "entity",
  "subtype": "",
  "title": "",
  "summary": "",
  "language": "pt-BR",
  "lifecycle_status": "draft",
  "subject": {
    "canonical_name": "",
    "scientific_name": "",
    "type_label": ""
  },
  "themes": [],
  "canonical_facts": [],
  "metadata": {},
  "modalities": { "image": [], "audio": [] },
  "provenance": {
    "creator": "",
    "publisher": "",
    "source_reference": "",
    "created_at": "",
    "modified_at": "",
    "reviewed": false
  },
  "rights": {
    "license": "",
    "rights_holder": "",
    "attribution": ""
  },
  "accessibility": {
    "alt_text": "",
    "transcript": ""
  }
}
```

`subject.scientific_name` permanece somente para compatibilidade com o schema v0.1. Novos domínios devem preferir `subtype`, `summary` e `metadata` para qualificações específicas.

## Metadados da aplicação e manifesto técnico

```json
{
  "bundle_id": "elo-content-example",
  "version": "1.0.0",
  "domain_category": "general",
  "target_audience": "general",
  "atom_ids": ["record_001", "record_002"]
}
```

O campo técnico `bundle_id` permanece no manifesto para compatibilidade e endereçamento das versões publicadas; na interface ele é apresentado como identificador da aplicação. `atom_ids` vazio em manifestos v0.1 significa “todos os átomos”. Em manifestos v0.2, o Studio grava a seleção explícita.

## Persistência e banco de dados

O SQLite em modo **WAL** é a fonte canônica do espaço editorial mutável. Átomos, relações, receitas, planos de pacote, associações e revisões são gravados transacionalmente em `editorial.sqlite3`.

JSON não é mais banco editorial. Ele é uma projeção gerada antes de validar/publicar, porque bundles selados precisam continuar portáteis, determinísticos e independentes do banco de autoria.

Tabelas principais:

- `content_atoms(content_id, document_json, updated_at)`;
- `content_relations(relation_id, document_json, updated_at)`;
- `content_recipes(recipe_id, document_json, updated_at)`;
- `package_plans(bundle_id, document_json, updated_at)`;
- `package_atoms(bundle_id, content_id, position)`;
- `editorial_revisions(sequence, entity_kind, entity_id, operation, occurred_at)`.

A compatibilidade legada é incremental: cada categoria ainda ausente no banco (átomos,
relações, receitas e planos de pacote) pode ser recuperada dos JSONs existentes. Isso
evita que a presença de átomos no SQLite impeça a descoberta de pacotes publicados
antes da migração. Depois da importação, o fluxo editorial é unidirecional: SQLite →
snapshot JSON → validação → bundle imutável.

## Estados e operações da aplicação

Uma aplicação possui três estados complementares, apresentados separadamente no Studio:

- **salva**: existe como rascunho editável no SQLite;
- **publicado**: possui pelo menos uma versão imutável no diretório de bundles;
- **ativo**: uma versão publicada é o manifesto que o Totem está executando.

Salvar nunca publica nem ativa. **Ativar** gera o bundle, valida, publica a versão e
troca o manifesto ativo como uma única operação da API. Uma combinação
`{bundle_id, version}` já publicada não pode ser sobrescrita com conteúdo diferente;
é necessário incrementar a versão semântica. Reativar exatamente o mesmo conteúdo é
permitido. Desativar remove apenas o apontamento de execução: a aplicação e suas versões
publicadas são preservadas. Uma aplicação ativa não pode ser excluída.

## Publicação e controle remoto

Ativar sempre gera, valida e sela uma versão do pacote antes de apontar o Totem para
ela. O controle remoto de conteúdo ao vivo (`SHOW_ATOM`) continua operando sobre um
átomo do pacote ativo e não altera o snapshot. Uma futura fila remota deve referenciar
`{bundle_hash, content_id}` para preservar rastreabilidade.
