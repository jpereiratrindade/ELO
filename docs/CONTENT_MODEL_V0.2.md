# ELO Content Model v0.2

## Decisão arquitetural

O ELO mantém duas dimensões independentes:

- **Átomo**: registro canônico, reutilizável, endereçado por `content_id` imutável.
- **Pacote**: composição editorial versionada que referencia átomos por `atom_ids` e pode ser vinculada a uma `application_id`.

Um átomo pode participar de vários pacotes. Editar um pacote não duplica o átomo. Publicar um pacote cria um snapshot imutável somente com os átomos selecionados, suas variantes e as relações internas à seleção.

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

## Metadados do pacote

```json
{
  "bundle_id": "elo-content-example",
  "version": "1.0.0",
  "application_id": "app.elo.example",
  "atom_ids": ["record_001", "record_002"]
}
```

`atom_ids` vazio em manifestos v0.1 significa “todos os átomos”, garantindo compatibilidade. Em manifestos v0.2, o Studio grava a seleção explícita.

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

Na primeira execução, os JSONs legados são importados automaticamente se o banco estiver vazio. Depois disso, o fluxo é unidirecional: SQLite → snapshot JSON → validação → bundle imutável.

## Publicação e controle remoto

Publicar sempre sela e ativa um pacote. O controle remoto de conteúdo ao vivo (`SHOW_ATOM`) continua operando sobre um átomo do pacote ativo e não altera o snapshot. Uma futura fila remota deve referenciar `{bundle_hash, content_id}` para preservar rastreabilidade.
