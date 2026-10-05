# ELO-EXPERIENCE-001 — Constituição da Experiência do Totem Interativo

## v0.1.0 — design-candidate

> **“Sempre pronto. Sempre incompleto.”**

```context-metadata+json
{
  "document": {
    "id": "ELO-EXPERIENCE-001",
    "version": "0.1.0",
    "status": "design-candidate",
    "title": "Constituição da Experiência do Totem Interativo",
    "date": "2026-10-05"
  },
  "system": {
    "name": "ELO",
    "kind": "offline-first contextual experience system",
    "primary_form_factor": "interactive totem",
    "target_substrate": "Raspberry Pi 5",
    "language": "C++26",
    "ui_candidate": "Qt 6 + QML",
    "consumes": [
      "ente-kernel",
      "JEV"
    ]
  }
}
```

---

## 1. Propósito

O ELO é um sistema **offline-first de experiência contextual** concebido para operar prioritariamente como um **totem interativo físico**.

Seu objetivo não é reproduzir um quiosque convencional baseado em menus, páginas e reprodução passiva de mídia.

O ELO deve:

- perceber a presença e a interação de pessoas no espaço físico;
- construir contexto durante uma sessão;
- utilizar conteúdo próprio e semanticamente estruturado;
- adaptar a experiência ao que acontece diante do sistema;
- registrar acontecimentos relevantes como eventos;
- preservar continuidade entre execuções;
- operar sem dependência constitutiva da nuvem;
- manter separação clara entre experiência, memória, identidade e interface.

O totem é o produto visível.

O ELO é o sistema que compõe a experiência.

O Ente é aquilo que preserva continuidade constitutiva dentro do sistema.

---

## 2. Princípio central

O ELO não deve funcionar como:

```text
pessoa toca na tela
      ↓
abre menu
      ↓
escolhe conteúdo
      ↓
assiste
```

O comportamento desejado é:

```text
ambiente
   │
   ▼
percepção
   │
   ▼
contexto da sessão
   │
   ▼
motor de experiência
   │
   ├── conteúdo
   ├── narrativa
   ├── resposta
   └── adaptação
   │
   ▼
interface física
   │
   ▼
nova interação
   │
   └────→ novo evento JEV
```

A experiência deve ser orientada por contexto e não apenas por navegação.

---

## 3. Definição operacional

> **ELO é um sistema offline-first de experiência contextual que percebe interações no mundo físico, constrói continuidade através de eventos e relações e utiliza conteúdo próprio para compor experiências adaptativas em um totem interativo.**

---

## 4. Arquitetura conceitual

```text
┌──────────────────────────────────────────────┐
│                    ELO                       │
│     sistema de experiência contextual        │
├──────────────────────────────────────────────┤
│ Interface física                             │
│ Qt 6 + QML                                   │
│ imagem · áudio · vídeo · texto · animação    │
├──────────────────────────────────────────────┤
│ Motor de experiência                         │
│ sessão · contexto · narrativa · decisão      │
│ adaptação · composição de conteúdo           │
├──────────────────────────────────────────────┤
│ Percepção                                    │
│ presença · distância · gesto · toque          │
│ atenção · quantidade de pessoas              │
├──────────────────────────────────────────────┤
│ Conteúdo próprio                             │
│ objetos · relações · temas · modalidades     │
├──────────────────────────────────────────────┤
│ Memória / continuidade                       │
│ eventos · relações · persistência · replay   │
├──────────────────────┬───────────────────────┤
│         JEV          │      ente-kernel      │
│ evento/evidência     │ identidade/estado     │
├──────────────────────┴───────────────────────┤
│ Substrato                                    │
│ Linux · Raspberry Pi 5 · câmera · sensores   │
└──────────────────────────────────────────────┘
```

---

## 5. Separação de responsabilidades

### 5.1 Totem

O totem é a manifestação física da experiência.

Ele reúne:

- tela;
- áudio;
- câmera;
- sensores;
- entrada por toque;
- processamento local;
- interface visual;
- presença física no ambiente.

O totem não deve conter lógica constitutiva espalhada em componentes de interface.

---

### 5.2 ELO

O ELO coordena:

- percepção;
- sessões;
- contexto;
- seleção e composição de conteúdo;
- narrativa;
- adaptação;
- relações;
- persistência;
- experiência apresentada.

O ELO não deve redefinir o Ente.

---

### 5.3 Ente

O Ente preserva:

```text
quem sou
+
estado constitutivo
+
continuidade
+
transformações efetivamente constitutivas
```

O Ente não deve:

- controlar câmera;
- desenhar interface;
- armazenar diretamente toda interação;
- responder a cada clique como se fosse transformação constitutiva;
- ser confundido com o próprio ELO.

O método conceitual `transform()` deve permanecer reservado a mudanças constitutivas.

Eventos da experiência não são automaticamente transformações do Ente.

---

### 5.4 JEV

O JEV representa acontecimentos relevantes da experiência.

Exemplos:

```text
presence.enter
presence.leave
attention.detected
content.show
user.touch
gesture.detected
answer.received
narrative.branch
relation.updated
session.start
session.end
```

O JEV registra história.

O Ente preserva continuidade constitutiva.

Um não substitui o outro.

---

## 6. Primitivas da experiência

O ELO deve ser inicialmente organizado em torno de cinco primitivas fundamentais.

### 6.1 Presence

Pergunta:

> Há alguém aqui?

Possíveis sinais:

- presença detectada;
- distância aproximada;
- quantidade de pessoas;
- entrada e saída do campo perceptivo.

---

### 6.2 Attention

Pergunta:

> Há indícios de atenção ou engajamento?

Possíveis sinais:

- aproximação;
- permanência;
- orientação corporal;
- interação com tela;
- repetição de ações;
- tempo de observação.

Attention não exige reconhecimento biométrico.

---

### 6.3 Action

Pergunta:

> O que aconteceu?

Exemplos:

- toque;
- escolha;
- resposta;
- gesto;
- aproximação;
- afastamento;
- interrupção;
- retomada.

---

### 6.4 Context

Pergunta:

> O que significa este acontecimento dentro desta sessão?

O contexto pode considerar:

- acontecimentos anteriores;
- conteúdo já apresentado;
- duração da sessão;
- sequência de escolhas;
- presença individual ou coletiva;
- relações construídas;
- histórico permitido pelo sistema.

---

### 6.5 Content

Pergunta:

> Que matéria-prima o sistema possui para responder agora?

O conteúdo não deve ser tratado apenas como arquivo.

Ele deve possuir estrutura e significado.

---

## 7. Modelo mínimo de experiência

Conceitualmente:

```text
experience =
    f(
        presence,
        attention,
        actions,
        context,
        history,
        available_content
    )
```

A experiência resultante deve ser observável através da interface física.

---

## 8. Conteúdo próprio como componente estrutural

O ELO deve ser projetado para operar com conteúdo próprio.

O conteúdo pode incluir:

```text
content/
├── themes/
├── narratives/
├── characters/
├── images/
├── video/
├── audio/
├── maps/
├── questions/
├── challenges/
├── facts/
└── relations/
```

O sistema não deve depender constitutivamente de:

- YouTube;
- páginas web;
- feeds remotos;
- APIs externas;
- conectividade permanente.

Recursos externos poderão existir futuramente como extensões, nunca como fundamento da experiência.

---

## 9. Objeto de conteúdo

Um objeto de conteúdo deve possuir metadados semânticos.

Exemplo:

```yaml
content_id: species_cardinal_001

subject:
  type: species
  name: Cardeal-amarelo

themes:
  - pampa
  - biodiversity
  - conservation

audiences:
  - children
  - adult
  - specialist

modalities:
  - image
  - narration
  - text
  - quiz

relations:
  habitat: grassland
  conservation_status: threatened
```

O objetivo é permitir que o mesmo objeto participe de diferentes experiências.

---

## 10. Conteúdo não é sequência

Uma fotografia, vídeo, áudio ou texto não deve possuir apenas uma função fixa.

Um mesmo objeto pode atuar como:

- introdução;
- pergunta;
- pista;
- comparação;
- pano de fundo;
- explicação;
- lembrança;
- desafio;
- consequência de uma decisão.

Assim, uma coleção pequena de conteúdo pode gerar múltiplos percursos.

---

## 11. Experiência contextual

Exemplo conceitual:

```text
presence.detected
distance ≈ 2.1 m
persons = 1
identity = UNKNOWN
```

O sistema pode apenas modificar discretamente a interface.

Com aproximação:

```text
distance ≈ 0.8 m
engagement_probability ↑
```

O sistema apresenta:

> Você conhece este lugar?

A resposta altera a experiência seguinte.

Não apenas a página seguinte.

---

## 12. Sessão desconhecida por padrão

A primeira relação com uma pessoa deve assumir:

```text
identity = UNKNOWN
```

UNKNOWN não significa erro.

UNKNOWN é um estado legítimo da experiência.

O sistema deve ser útil mesmo sem identificar pessoas.

Reconhecimento persistente, quando existir, deverá ser tratado como capacidade adicional e não como requisito constitutivo.

---

## 13. Experiência individual e coletiva

O ELO deve admitir pelo menos:

```text
persons = 0
persons = 1
persons > 1
```

Uma sessão coletiva pode alterar:

- linguagem;
- perguntas;
- desafios;
- tempo;
- narrativa;
- necessidade de consenso;
- forma de apresentação.

Exemplo:

> Agora vocês precisarão decidir juntos.

---

## 14. Experiência corporal

O totem pode utilizar percepção física sem exigir identificação biométrica.

Exemplos:

```text
levante a mão
      ↓
gesture.detected
      ↓
experiência responde
```

```text
duas pessoas se aproximam
      ↓
session.mode = collective
      ↓
narrativa muda
```

A presença física deve ser explorada como parte nativa da experiência.

---

## 15. Eventos e história da experiência

Exemplo de sequência JEV:

```text
14:02:11 session.start
14:02:11 presence.enter
14:02:13 attention.detected
14:02:15 content.show:pampa_intro
14:02:21 user.touch:know_place=yes
14:02:22 narrative.branch:experienced
14:02:30 audio.play:bird_17
14:02:37 user.answer:cardeal
14:02:38 result.correct
14:02:41 relation.strength:pampa += 1
14:03:04 presence.leave
14:03:05 session.end
```

O objetivo não é armazenar apenas cliques.

O objetivo é registrar experiência.

---

## 16. Memória e reconstrução

Uma seed não reconstrói o mundo.

Ela permite reproduzir decisões pseudoaleatórias internas quando essas decisões forem determinísticas.

A reconstrução exige:

```text
estado inicial
     +
identidade constitutiva
     +
sequência ordenada de eventos JEV
     +
entradas externas relevantes
     +
seeds/decisões determinísticas
     ↓
reexecução / verificação
     ↓
estado reconstruído
```

Seed e JEV possuem funções distintas.

### Seed

Representa parte da proveniência determinística.

### JEV

Representa parte da história vivida.

---

## 17. Identidade e hardware

A identidade do Ente não deve ser equivalente à identidade física do Raspberry Pi.

O hardware pode fornecer uma âncora de substrato.

Conceitualmente:

```text
installation_identity
+
hardware_anchor
+
ente_identity
+
private_secret
```

Esses componentes devem permanecer distinguíveis.

A substituição de hardware não deve implicar automaticamente a morte ou criação de outro Ente.

---

## 18. Interface gráfica

A interface candidata é:

```text
Qt 6 + QML
```

Requisitos iniciais:

- execução local;
- tela cheia;
- suporte a Raspberry Pi 5;
- baixo acoplamento com ente-kernel;
- comunicação com o motor de experiência;
- animações fluidas;
- áudio;
- vídeo;
- imagem;
- texto;
- resposta imediata à percepção.

A GUI é uma superfície da experiência.

Ela não deve ser o sistema.

---

## 19. Linguagem e runtime

O núcleo candidato permanece:

```text
C++26
```

Objetivos:

- previsibilidade;
- desempenho local;
- controle de recursos;
- portabilidade para Raspberry Pi 5;
- integração nativa com sensores e mídia;
- separação clara entre runtime, domínio e interface.

---

## 20. Alvo físico

Primeiro substrato:

```text
Raspberry Pi 5
```

Configuração-alvo inicial:

```text
Raspberry Pi 5
├── Linux
├── display
├── touch
├── câmera
├── áudio
├── armazenamento local
└── sensores opcionais
```

O Raspberry Pi 5 deve passar de alvo nominal a plataforma experimental validada.

---

## 21. Primeiro domínio de conteúdo

O primeiro protótipo deve operar com um domínio pequeno e controlado.

Candidato:

```text
Pampa
```

Coleção inicial sugerida:

```text
10 espécies
5 paisagens
5 fenômenos ecológicos
5 histórias
```

Total aproximado:

```text
20–30 objetos de conteúdo
```

---

## 22. Cinco experiências iniciais

### 22.1 Descobrir

O sistema introduz algo desconhecido através de curiosidade e revelação.

### 22.2 Explorar

A pessoa navega por relações e temas sem depender de uma sequência fixa.

### 22.3 Comparar

Dois ou mais objetos são apresentados de forma relacional.

### 22.4 Desafiar

O sistema formula perguntas, reconhecimento de padrões, gestos ou escolhas.

### 22.5 Contemplar

O sistema reduz exigência de interação e valoriza imagem, som, ambiente e permanência.

---

## 23. Primeira narrativa

A primeira narrativa deve ser simples o suficiente para testar toda a arquitetura.

Exemplo:

```text
uma presença surge
      ↓
ELO percebe aproximação
      ↓
apresenta uma paisagem
      ↓
formula pergunta contextual
      ↓
recebe resposta
      ↓
seleciona conteúdo relacionado
      ↓
registra JEV
      ↓
atualiza contexto
      ↓
continua ou encerra sessão
```

---

## 24. Primeiro corte vertical

O próximo marco do projeto não deve ser o crescimento horizontal da arquitetura.

Deve ser um corte vertical completo.

### EXP-ELO-001 — Primeira experiência persistente

Objetivo:

> demonstrar que o ELO consegue perceber, responder, registrar, persistir e continuar.

Fluxo:

1. Raspberry Pi inicia.
2. ELO cria ou recupera o mesmo Ente.
3. Uma presença é detectada.
4. A sessão nasce como UNKNOWN.
5. Um objeto de conteúdo é apresentado.
6. Uma interação ocorre.
7. Um evento JEV é registrado.
8. O contexto da sessão muda.
9. O estado necessário é persistido.
10. O sistema é encerrado.
11. O dispositivo reinicia.
12. O mesmo Ente é recuperado.
13. A história relevante permanece disponível.
14. Replay verifica aspectos declarados determinísticos.

---

## 25. Critérios de sucesso do EXP-ELO-001

O experimento é considerado bem-sucedido se:

- o sistema executa integralmente no Raspberry Pi 5;
- a GUI Qt/QML funciona em modo totem;
- presença simulada ou real chega ao motor de experiência;
- uma sessão UNKNOWN é criada;
- ao menos um conteúdo semanticamente estruturado é utilizado;
- uma resposta da pessoa altera a experiência;
- eventos JEV são persistidos;
- reinicialização não destrói a continuidade do Ente;
- replay reproduz o que foi declarado determinístico;
- experiência e transformação constitutiva permanecem conceitualmente separadas.

---

## 26. Regra de foco

Toda nova capacidade deve responder:

> **Isso melhora diretamente a experiência do totem?**

Se a resposta for não, a capacidade não deve ocupar prioridade no ciclo atual.

A arquitetura existe para servir à experiência.

---

## 27. Prioridade atual

A prioridade passa a ser:

```text
TOTEM
  │
  ├── percepção
  ├── experiência
  ├── conteúdo próprio
  ├── interface
  │
  ▼
 JEV
  │
  ▼
memória
  │
  ▼
 Ente
```

Não se trata de abandonar a arquitetura anterior.

Trata-se de fazer a arquitetura servir explicitamente ao produto.

---

## 28. Não objetivos imediatos

Não são objetivos constitutivos da primeira versão:

- assistente conversacional geral;
- dependência de LLM em nuvem;
- reconhecimento biométrico obrigatório;
- navegador web como interface principal;
- agregação indiscriminada de conteúdo externo;
- múltiplos domínios de conteúdo simultâneos;
- personalização opaca impossível de explicar;
- transformação do Ente a cada evento trivial;
- construção de uma arquitetura abstrata sem demonstração física.

---

## 29. Princípios de projeto

### P1 — Offline-first

O sistema deve permanecer funcional sem conectividade.

### P2 — Physical-first

A experiência nasce da presença no espaço físico.

### P3 — Context before menu

Contexto deve preceder navegação convencional sempre que fizer sentido.

### P4 — Content as structure

Conteúdo deve ser semanticamente compreensível pelo sistema.

### P5 — Unknown is valid

Não conhecer a identidade da pessoa não é falha.

### P6 — Events are history

Eventos registram a experiência.

### P7 — Ente is continuity

O Ente não deve ser confundido com interface, sessão ou log.

### P8 — Explainable adaptation

A adaptação deve poder ser relacionada a eventos e contexto observáveis.

### P9 — Small corpus, rich experience

Poucos objetos de conteúdo devem ser capazes de participar de muitos percursos.

### P10 — Architecture serves experience

Nenhuma abstração deve se tornar objetivo por si mesma.

---

## 30. Pergunta arquitetural central

O primeiro protótipo deve permitir responder empiricamente:

> **O que precisa sobreviver para que o Ente continue sendo ele e para que o ELO continue uma experiência?**

Essa pergunta conecta:

- identidade;
- memória;
- persistência;
- experiência;
- replay;
- hardware;
- conteúdo;
- continuidade.

---

## 31. Estado desta versão

### Especificado

- foco no totem interativo;
- experiência contextual;
- conteúdo próprio;
- primitives Presence, Attention, Action, Context e Content;
- separação ELO / Ente / JEV;
- Raspberry Pi 5 como substrato;
- C++26 como linguagem candidata;
- Qt 6 + QML como GUI candidata;
- EXP-ELO-001 como próximo corte vertical.

### Ainda a implementar ou validar

- persistência real;
- percepção física;
- esquema definitivo de conteúdo;
- storage JEV;
- replay;
- continuidade do Ente entre boots;
- integração total no Raspberry Pi 5;
- primeiro pacote de conteúdo;
- primeira experiência ponta a ponta.

---

## 32. Direção

O ELO não deve crescer primeiro em número de módulos.

Deve crescer em **qualidade de experiência demonstrável**.

O próximo avanço relevante não será uma nova camada abstrata.

Será quando uma pessoa se aproximar do primeiro totem, algo acontecer por causa dessa presença, o sistema compreender o contexto suficiente para responder de forma coerente e essa experiência deixar uma história persistente.

---

## 33. Síntese

```text
ELO
=
percepção
+
contexto
+
conteúdo próprio
+
experiência
+
eventos
+
continuidade
```

O objetivo não é construir uma máquina que apenas exiba informação.

O objetivo é construir um sistema que **perceba que algo está acontecendo e consiga transformar conteúdo em experiência**.

---

**Fim — ELO-EXPERIENCE-001 v0.1.0**
