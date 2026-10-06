# ELO-CONTENT-001 — Constituição do Conteúdo e da Composição de Experiências

## v0.1.0 — design-candidate

> **“Conteúdo não é aquilo que a pessoa abre. É aquilo a partir do qual a experiência é composta.”**

```context-metadata+json
{
  "document": {
    "id": "ELO-CONTENT-001",
    "version": "0.1.0",
    "status": "design-candidate",
    "title": "Constituição do Conteúdo e da Composição de Experiências",
    "date": "2026-10-05"
  },
  "system": {
    "name": "ELO",
    "kind": "offline-first contextual experience system",
    "primary_form_factor": "interactive totem",
    "target_substrate": "Raspberry Pi 5",
    "language": "C++26",
    "ui_candidate": "Qt 6 + QML"
  },
  "depends_on": [
    "ELO-EXPERIENCE-001",
    "ELO-PUBLISHING-001",
    "ELO-ADMIN-001"
  ],
  "primary_consumer": "Gemini / implementation agent"
}
```

---

## 1. Propósito

Este documento define a constituição do conteúdo do ELO e a estrutura mínima para que o sistema componha experiências contextuais em um totem interativo.

O ELO não deve ser construído como uma coleção de páginas, slides, vídeos ou menus.

Ele deve trabalhar com conteúdo estruturado e relacionável, capaz de assumir diferentes papéis conforme a situação:

```text
PASSA
  ↓
PERCEBE
  ↓
DESACELERA
  ↓
PARA
  ↓
INTERAGE
  ↓
APROFUNDA
```

O mesmo núcleo de conteúdo deve poder participar de várias experiências.

---

## 2. Separação fundamental

O sistema deve distinguir quatro dimensões.

```text
TYPE
o que o conteúdo significa

MODALITY
como ele pode ser apresentado

ROLE
qual função cumpre naquele momento

INTERACTION
como a pessoa participa
```

Exemplo:

```text
type        = species
modality    = image + audio
role        = attract
interaction = listen_and_identify
```

Essa separação é constitutiva.

Uma imagem não é um tipo semântico.
Um quiz não é uma modalidade.
`attract` não é um formato de arquivo.

---

# PARTE I — TIPOS DE CONTEÚDO SUPORTADOS

## 3. Taxonomia v0.1

O ELO v0.1 deve suportar os seguintes tipos semânticos de conteúdo:

```text
fact
concept
entity
place
phenomenon
event
micro_story
question
challenge
comparison
sequence
indicator
ambient_scene
```

Tipos mais específicos podem ser expressos por `subtype`.

---

## 4. `fact`

Uma afirmação factual verificável.

Exemplos:

```text
“O Pampa cobre cerca de 63% do território do Rio Grande do Sul.”
“O Cardeal-amarelo alimenta-se prioritariamente de sementes no solo e em arbustos.”
```

Campos importantes:

```yaml
type: fact
statement: "..."
source_ids:
  - source_001
confidence: reviewed
```

Todo fato canônico deve possuir proveniência.

---

## 5. `concept`

Uma ideia ou conceito que pode ser explicado, relacionado ou exemplificado.

Exemplos:

```text
biodiversidade
campo nativo
cadeia alimentar
conservação
sucessão ecológica
```

Um conceito pode apontar para:

- exemplos;
- fatos;
- espécies;
- paisagens;
- fenômenos;
- comparações.

---

## 6. `entity`

Uma entidade identificável do domínio.

Subtipos iniciais possíveis:

```text
species
organism
object
person
institution
cultural_element
```

Exemplo:

```yaml
type: entity
subtype: species
canonical_name: Cardeal-amarelo
scientific_name: Gubernatrix cristata
```

O subtipo `species` será especialmente importante no primeiro corpus do Pampa.

---

## 7. `place`

Um lugar, ambiente, paisagem ou unidade espacial.

Subtipos:

```text
landscape
habitat
region
site
ecosystem_area
```

Exemplos:

```text
campo nativo
banhado
afloramento rochoso
paisagem campestre
```

---

## 8. `phenomenon`

Um processo, dinâmica ou fenômeno observável.

Subtipos:

```text
ecological_process
climatic_process
human_impact
seasonal_change
behavior
```

Exemplos:

```text
polinização
pastejo
fogo
migração
conversão de habitat
```

---

## 9. `event`

Um acontecimento localizado no tempo.

Pode representar:

```text
evento histórico
evento ambiental
mudança territorial
descoberta
marco institucional
```

Campos típicos:

```yaml
type: event
date_or_period: "..."
place_ids:
  - place_001
```

---

## 10. `micro_story`

Uma narrativa curta e autocontida.

Pode combinar:

```text
personagem
situação
mudança
descoberta
desfecho parcial
```

A micro-história deve funcionar bem em sessões curtas e ser interrompível.

Ela não precisa encerrar todo o tema.

---

## 11. `question`

Uma pergunta que pode servir tanto para curiosidade quanto para interação.

Subtipos:

```text
open
binary
multiple_choice
identification
prediction
observation
reflection
```

Exemplo:

```yaml
type: question
subtype: identification
prompt: "Quem está cantando?"
answer_ref: species_cardinal_001
```

---

## 12. `challenge`

Uma tarefa curta que exige ação.

Subtipos:

```text
find
match
identify
choose
listen
observe
gesture
collective_decision
```

Exemplos:

```text
encontre a espécie na paisagem
escute e descubra
compare duas imagens
faça uma escolha em grupo
```

---

## 13. `comparison`

Conteúdo que explicita diferenças, semelhanças ou mudanças.

Subtipos:

```text
A_vs_B
before_after
larger_smaller
common_rare
native_transformed
seasonal
```

Uma comparação deve referenciar pelo menos dois conteúdos.

---

## 14. `sequence`

Uma ordenação significativa.

Subtipos:

```text
timeline
process
life_cycle
cause_effect
step_sequence
```

Exemplo:

```text
evento A
  ↓
evento B
  ↓
consequência C
```

---

## 15. `indicator`

Um dado, medida ou indicador que possa ser apresentado de forma compreensível.

Exemplos:

```text
percentual
contagem
tendência
área
tempo
distância
frequência
```

Pode alimentar:

- texto;
- gráfico;
- comparação;
- animação;
- mapa.

Dados devem possuir unidade, contexto e fonte.

---

## 16. `ambient_scene`

Uma composição destinada principalmente à presença ambiental.

Pode reunir:

```text
imagem
som
movimento
texto mínimo
tempo
```

Não exige interação.

Seu objetivo pode ser:

- estabelecer atmosfera;
- atrair atenção;
- comunicar algo em poucos segundos;
- fornecer valor para quem apenas passa.

---

## 17. Extensibilidade

A taxonomia v0.1 não deve ser codificada como um conjunto rígido impossível de ampliar.

Deve permitir futuros tipos como:

```text
testimony
quote
artifact
document_excerpt
map_story
data_story
simulation
procedural_scene
3d_object
```

A implementação deve validar os tipos conhecidos, mas manter o domínio preparado para versionamento do schema.

---

# PARTE II — MODALIDADES DE APRESENTAÇÃO

## 18. Modalidades suportadas

O ELO deve distinguir conteúdo de sua mídia.

Modalidades iniciais:

```text
text
image
audio
video
animation
map
diagram
data_visualization
```

Um mesmo conteúdo pode possuir várias modalidades.

Exemplo:

```yaml
modalities:
  - image
  - audio
  - text
```

---

## 19. `text`

Usos:

- nome;
- frase curta;
- pergunta;
- microfato;
- explicação;
- legenda;
- narrativa.

A interface do totem deve priorizar textos curtos.

---

## 20. `image`

Pode incluir:

- fotografia;
- ilustração;
- detalhe;
- recorte;
- composição;
- imagem comparativa.

Deve possuir créditos e direitos de uso registrados.

---

## 21. `audio`

Pode incluir:

- paisagem sonora;
- vocalização;
- efeito;
- narração;
- entrevista;
- música autorizada.

O áudio deve possuir duração, origem e direitos documentados.

---

## 22. `video`

Pode incluir:

- registro curto;
- cena;
- comportamento;
- transformação;
- depoimento;
- explicação.

Vídeos devem ser otimizados para execução local.

---

## 23. `animation`

Inclui animações produzidas ou procedurais.

Pode ser utilizada para:

- atrair atenção;
- indicar mudança;
- revelar informação;
- ilustrar processo;
- responder à presença.

---

## 24. `map`

Mapas são modalidades espaciais.

Podem representar:

- distribuição;
- localização;
- trajetória;
- comparação espacial;
- mudança territorial.

---

## 25. `diagram`

Representações estruturais:

- relações;
- ciclos;
- processos;
- anatomia;
- fluxo;
- causa e efeito.

---

## 26. `data_visualization`

Representações de dados:

- barras;
- linhas;
- proporções;
- pictogramas;
- contadores;
- escalas.

Devem ser simples o suficiente para leitura em um totem.

---

# PARTE III — PAPÉIS EXPERIENCIAIS

## 27. Roles

Todo conteúdo apresentado deve poder assumir um papel:

```text
ambient
attract
engage
reveal
deepen
```

---

## 28. `ambient`

Para quem passa.

Objetivo:

```text
entregar valor sem exigir parada
```

---

## 29. `attract`

Para provocar desaceleração.

Objetivo:

```text
curiosidade
surpresa
contraste
incompletude
```

---

## 30. `engage`

Para iniciar participação.

Objetivo:

```text
ação simples
resposta rápida
entrada clara
```

---

## 31. `reveal`

Para recompensar a ação.

Objetivo:

```text
mostrar
explicar
confirmar
surpreender
```

---

## 32. `deepen`

Para permitir continuidade.

Objetivo:

```text
relacionar
contextualizar
comparar
contar
aprofundar
```

---

# PARTE IV — FORMAS DE INTERAÇÃO

## 33. Interações suportadas

Primeiro conjunto:

```text
none
touch
choose
true_false
identify
find
compare
listen_and_identify
reveal
explore
follow_relation
gesture
collective_choice
```

Interação é uma propriedade da experiência, não do fato canônico.

---

## 34. Interação sem ação explícita

`none` é legítimo.

O ELO deve suportar experiências:

```text
contemplativas
ambientais
reativas à presença
```

---

## 35. Toque e escolha

Formas simples:

```text
touch
choose
true_false
```

Devem funcionar com poucos elementos grandes e legíveis.

---

## 36. Identificação e descoberta

```text
identify
find
listen_and_identify
```

São centrais para experiências curtas de curiosidade.

---

## 37. Exploração

```text
explore
follow_relation
```

Permitem aprofundamento não linear.

---

## 38. Gesto

`gesture` utiliza percepção corporal sem exigir identificação biométrica.

Exemplo:

```text
levantar a mão
aproximar-se
mover-se para uma região
```

---

## 39. Escolha coletiva

`collective_choice` é utilizada quando mais de uma pessoa está presente.

Exemplo:

```text
“Vocês conseguem chegar a uma decisão juntos?”
```

---

# PARTE V — CONTENT ATOM

## 40. ContentAtom

`ContentAtom` é a menor unidade semanticamente útil para composição.

Estrutura sugerida:

```yaml
schema_version: 0.1
content_id: species_cardinal_001

type: entity
subtype: species

subject:
  canonical_name: Cardeal-amarelo
  scientific_name: Gubernatrix cristata

themes:
  - pampa
  - biodiversity
  - conservation

canonical_facts:
  - fact_id: fact_cardinal_001
    statement: "..."
    source_ids:
      - source_001

modalities:
  image:
    - asset_id: image_cardinal_001
  audio:
    - asset_id: audio_cardinal_001

relations:
  - rel_cardinal_habitat
  - rel_cardinal_conservation

supported_roles:
  - ambient
  - attract
  - engage
  - reveal
  - deepen

audiences:
  - general

provenance:
  reviewed: true
```

---

# PARTE VI — RELATIONS

## 41. Relation

Relações tornam o corpus navegável.

Tipos iniciais:

```text
is_a
part_of
located_in
inhabits
depends_on
interacts_with
threatened_by
causes
affects
contrasts_with
similar_to
precedes
follows
example_of
related_to
```

Exemplo:

```yaml
relation_id: rel_cardinal_grassland
type: inhabits
from: species_cardinal_001
to: habitat_grassland_001
```

---

# PARTE VII — VARIANTS

## 42. Variant

Uma variante adapta o mesmo núcleo sem alterar o fato.

Exemplo:

```yaml
variant_id: species_cardinal_001_attract_01
content_id: species_cardinal_001
role: attract
audience: general

presentation:
  text: "Você consegue descobrir quem está cantando?"
  media:
    - audio_cardinal_001

interaction:
  type: listen_and_identify

duration_hint_seconds: 8
```

---

## 43. Regra de fidelidade

Variantes:

- podem mudar forma;
- podem mudar comprimento;
- podem mudar dificuldade;
- podem mudar vocabulário;
- não podem alterar o núcleo factual.

---

# PARTE VIII — RECIPES

## 44. Recipe

Recipe é uma estrutura reutilizável de experiência.

Receitas iniciais:

```text
discover_by_sound
discover_by_image
compare_two
true_or_false
find_in_scene
before_after
follow_relation
micro_story
collective_choice
contemplate
```

Exemplo:

```yaml
recipe_id: discover_by_sound

requires:
  - subject.audio
  - subject.image
  - subject.name

steps:
  - play_audio
  - ask_identification
  - reveal_image
  - reveal_name
  - show_micro_fact
  - offer_deepen
```

---

# PARTE IX — EXPERIENCE STATE

## 45. ExperienceState

Representa somente o estado da experiência atual.

```yaml
session_id: session_001

presence:
  persons: 1
  distance_band: near

attention:
  level: engaged

experience:
  role: engage
  depth: 2

current_content:
  content_id: species_cardinal_001
  variant_id: species_cardinal_001_engage_01

history:
  seen:
    - landscape_grassland_001
```

Não confundir com estado constitutivo do Ente.

---

# PARTE X — CONTENT PERFORMANCE

## 46. Métricas

Quatro métricas centrais:

```text
ATTRACTION
passou → percebeu

STOP
percebeu → parou

ENGAGEMENT
parou → interagiu

DEPTH
interagiu → aprofundou
```

Exemplo agregado:

```yaml
content_id: species_cardinal_001

metrics:
  passages: 100
  attention: 43
  stops: 27
  interactions: 19
  deep_sessions: 12
```

Não requer identificação pessoal.

---

# PARTE XI — GERAÇÃO DE CONTEÚDO

## 47. Três níveis

```text
PRE-PRODUCED
PRE-GENERATED
RUNTIME-COMPOSED
```

### PRE-PRODUCED

- fatos;
- dados;
- imagens;
- áudio;
- vídeo;
- mapas;
- fontes;
- créditos.

### PRE-GENERATED

- perguntas;
- chamadas;
- textos curtos;
- desafios;
- variantes por papel;
- versões por público;
- micro-histórias baseadas em fontes.

### RUNTIME-COMPOSED

O ELO escolhe:

```text
content
+
variant
+
recipe
+
relation
+
timing
```

A v0.1 não depende de geração textual online em tempo real.

---

# PARTE XII — CORPUS INICIAL DO PAMPA

## 48. Meta inicial

O primeiro corpus deve ser pequeno, confiável e densamente relacionado.

Sugestão:

```text
10 entidades / espécies
5 lugares ou paisagens
5 fenômenos ou processos
10 fatos centrais
10 perguntas
5 desafios
5 comparações
5 micro-histórias
3 sequências
5 indicadores
5 cenas ambientais
```

Recursos multimídia associados:

```text
12+ imagens
8+ áudios
vídeos curtos quando disponíveis
mapas quando úteis
```

---

## 49. Regra de seleção

Todo item candidato deve responder:

```text
Como funciona para quem PASSA?
Como funciona para quem PARA?
Como funciona para quem INTERAGE?
Como funciona para quem APROFUNDA?
```

---

# PARTE XIII — ARMAZENAMENTO

## 50. Estrutura sugerida

```text
content/
├── catalog/
│   ├── atoms/
│   ├── relations/
│   ├── variants/
│   └── recipes/
├── assets/
│   ├── images/
│   ├── audio/
│   ├── video/
│   ├── maps/
│   └── diagrams/
├── sources/
│   ├── bibliography/
│   └── provenance/
├── locales/
│   └── pt-BR/
└── validation/
    ├── schemas/
    └── reports/
```

---

# PARTE XIV — IMPLEMENTAÇÃO PARA O GEMINI

## 51. Objetivo técnico v0.1

Implementar um núcleo de conteúdo capaz de:

1. carregar o catálogo local;
2. validar schema;
3. carregar todos os tipos v0.1;
4. resolver relações;
5. filtrar por tipo, tema, modalidade e papel;
6. resolver variantes;
7. executar receitas;
8. entregar ações de apresentação à GUI;
9. registrar eventos JEV;
10. explicar por que selecionou determinado conteúdo;
11. operar sem rede;
12. permitir replay determinístico quando aplicável.

---

## 52. Tipos C++26 candidatos

```cpp
enum class ContentType;
enum class ContentRole;
enum class Modality;
enum class InteractionType;

struct ContentAtom;
struct ContentRelation;
struct ContentVariant;
struct ExperienceRecipe;
struct ExperienceState;
struct SelectionReason;

class ContentCatalog;
class ContentValidator;
class ContentSelector;
class RecipeExecutor;
class ContentPerformanceStore;
```

Evitar acoplamento direto com Qt no domínio.

---

## 53. Fronteira com Qt/QML

```text
ContentCatalog
      ↓
ExperienceEngine
      ↓
ContentSelector
      ↓
RecipeExecutor
      ↓
PresentationAction
      ↓
Qt/QML
```

A GUI apresenta.

O domínio decide.

---

## 54. Ações de apresentação iniciais

```text
ShowText
ShowImage
PlayAudio
PlayVideo
ShowMap
ShowDiagram
ShowData
Animate
AskChoice
AskTrueFalse
WaitForTouch
WaitForGesture
Reveal
OfferDeepen
Clear
```

---

## 55. Validação

Erros que devem ser detectados antes do runtime:

```text
duplicate content_id
unknown type
unknown subtype
broken relation
missing asset
missing source
invalid modality
invalid role
invalid interaction
canonical fact without provenance
recipe with unsatisfied requirement
```

---

## 56. Seleção explicável

Exemplo:

```yaml
selected:
  content_id: species_cardinal_001
  variant_id: species_cardinal_001_attract_01
  recipe_id: discover_by_sound

reason:
  requested_role: attract
  theme_match: pampa
  not_seen_in_session: true
  audio_available: true
  recipe_requirements_met: true
```

---

## 57. JEV

Eventos mínimos associados a conteúdo:

```text
content.selected
content.presented
content.revealed
interaction.started
interaction.completed
recipe.started
recipe.completed
relation.followed
experience.depth_changed
```

---

## 58. Critérios de aceite

A implementação v0.1 é suficiente quando:

- os 13 tipos semânticos v0.1 carregam;
- as 8 modalidades são representáveis;
- os 5 roles funcionam;
- as interações iniciais são modeláveis;
- relações são navegáveis;
- duas receitas funcionam ponta a ponta;
- o ELO consegue selecionar conteúdo por contexto;
- toda seleção retorna uma razão;
- o catálogo é totalmente local;
- reinício não exige reconstrução manual do corpus.

---

## 59. Regra de foco

Toda nova abstração deve responder:

> **Isso aumenta nossa capacidade de compor uma experiência significativa no totem?**

Se não, não é prioridade do ciclo atual.

---

## 60. Síntese

```text
CONTEÚDO ELO
=
significado
+
mídia
+
relações
+
variantes
+
papel
+
interação
+
proveniência
```

O ELO não navega por páginas.

Ele compõe experiências a partir de conteúdo estruturado.

---

**Fim — ELO-CONTENT-001 v0.1.0**
