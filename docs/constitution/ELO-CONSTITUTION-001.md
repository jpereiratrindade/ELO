# ELO-CONSTITUTION-001 — Constituição Inicial do Sistema ELO

## v0.2.0 — design-candidate

> **“Sempre pronto. Sempre incompleto.”**

```context-metadata+json
{
  "document": {
    "id": "ELO-CONSTITUTION-001",
    "version": "0.2.0",
    "status": "design-candidate",
    "title": "Constituição Inicial do Sistema ELO",
    "date": "2026-10-05"
  },
  "system": {
    "name": "ELO",
    "kind": "offline-first personalized interaction system",
    "language": "C++26",
    "substrate_candidate": "Raspberry Pi 5",
    "ui_candidate": "Qt 6 + QML/Qt Quick",
    "consumes": [
      "ente-kernel",
      "JEV"
    ]
  },
  "epistemic_scope": "system-constitution-hypothesis",
  "privacy_model": "local-first, data-minimizing, consent-driven",
  "operating_model": "offline-first",
  "motto": "Sempre pronto. Sempre incompleto."
}
```

---

# 0. Declaração fundamental

Este documento define a constituição inicial do **ELO**, um novo sistema experimental e independente concebido para oferecer experiências presenciais personalizadas em regime **offline-first**.

O nome ELO expressa a propriedade central buscada pelo sistema:

> **preservar continuidade entre encontros sem exigir que a pessoa entregue ao sistema mais informação do que aquela necessária à experiência.**

O ELO poderá utilizar reconhecimento biométrico facial como uma das fontes de evidência para estabelecer continuidade entre diferentes interações com uma mesma pessoa.

Entretanto:

```text
ELO
≠
ENTE

ELO
≠
JEV

ELO
≠
RECONHECIMENTO FACIAL

ELO
≠
RASPBERRY PI
```

O ELO constitui um sistema autônomo com:

- identidade própria;
- propósito próprio;
- domínio próprio;
- estado próprio;
- armazenamento próprio;
- ciclo de vida próprio;
- regras próprias;
- interface própria;
- experiências próprias.

Para cumprir determinadas propriedades, poderá consumir componentes independentes já existentes.

Na concepção inicial:

```text
ELO
 │
 ├── consome ente-kernel
 │      ↓
 │   identidade e continuidade constitutiva
 │
 ├── consome JEV
 │      ↓
 │   julgamento probabilístico tipado
 │
 ├── consome capacidades de percepção
 │      ↓
 │   câmera
 │   detecção
 │   biometria
 │
 ├── possui domínio de experiência
 │      ↓
 │   conteúdo
 │   personalização
 │   questionários
 │   seleção
 │
 └── possui interface local
        ↓
     Qt 6 / QML
```

Nenhum desses componentes isoladamente define aquilo que o ELO é.

---

# 1. Significado do nome

**ELO** não constitui sigla obrigatória.

O nome representa:

```text
pessoa
  ↓
encontro
  ↓
experiência
  ↓
memória mínima
  ↓
novo encontro
  ↓
continuidade
```

O reconhecimento facial constitui apenas uma forma possível de restabelecer esse elo.

O nome permanece adequado caso futuras realizações empreguem:

- voz;
- dispositivo pessoal;
- token;
- presença;
- contexto;
- outro fator local;
- combinação multimodal.

Assim:

```text
ELO
≠
FACE
```

---

# 2. Princípio de composição

O ELO deve ser construído por **composição de capacidades independentes**.

```text
                 ELO
                  │
        ┌─────────┼──────────┐
        │         │          │
        ▼         ▼          ▼
 ente-kernel     JEV      domínio ELO
                            │
                    ┌───────┼────────┐
                    ▼       ▼        ▼
                 percepção GUI   experiência
```

O sistema adapta-se aos contratos dos componentes que consome.

Os componentes fundamentais não são especializados para acomodar o ELO.

---

# 3. Independência dos componentes consumidos

O ELO não deve exigir alteração específica no `ente-kernel` para cumprir sua função.

Da mesma forma, não deve redefinir o JEV para transformá-lo em mecanismo próprio do ELO.

A relação correta é:

```text
ELO
 ↓
consome contrato
 ↓
componente independente
```

e nunca:

```text
necessidade do ELO
       ↓
modificar ENTE
       ↓
modificar JEV
       ↓
incorporar domínio ELO
```

Caso uma limitação seja encontrada, deve-se primeiro determinar se ela pertence:

```text
ao ELO
ao adaptador
ao contrato
ou realmente ao componente consumido
```

antes de qualquer alteração externa.

---

# 4. Hipótese central

A hipótese investigada é:

> **Um sistema local pode oferecer experiências personalizadas e contínuas utilizando biometria facial apenas como evidência probabilística de recorrência de uma pessoa, preservando simultaneamente autonomia, privacidade, minimização de dados e independência da nuvem.**

O objetivo não é provar quem uma pessoa “é” em sentido absoluto.

O objetivo é sustentar uma afirmação limitada:

> **A pessoa atualmente presente provavelmente corresponde à mesma identidade local que participou anteriormente desta experiência.**

Portanto:

```text
FACE
≠
PESSOA

EMBEDDING
≠
IDENTIDADE ABSOLUTA

MATCH BIOMÉTRICO
≠
CERTEZA

RECONHECIMENTO
=
EVIDÊNCIA PROBABILÍSTICA
DE CONTINUIDADE LOCAL
```

---

# 5. Propósito inicial

O ELO deverá possibilitar experiências presenciais personalizadas cujo conteúdo seja definido pela aplicação concreta.

Exemplos possíveis:

- apresentação de conteúdo individualizado;
- continuidade de uma experiência anterior;
- conteúdos sequenciais;
- pesquisas curtas;
- questionários;
- seleção aleatória de participantes;
- experiências educativas;
- experiências culturais;
- exposições interativas;
- atividades experimentais;
- pesquisas de campo;
- outras aplicações futuras compatíveis.

A primeira realização não precisa conhecer antecipadamente todos os usos futuros.

```text
FUNÇÃO ATUAL
≠
IDENTIDADE DO ELO
```

---

# 6. Princípio basal

> **Reconhecer apenas o suficiente para proporcionar continuidade, conservar apenas o necessário para sustentá-la e jamais exigir da pessoa mais informação do que a experiência legitimamente necessita.**

---

# 7. Sempre pronto. Sempre incompleto.

## 7.1 Sempre pronto

O ELO deve possuir localmente recursos suficientes para:

- iniciar;
- estabelecer e recuperar sua identidade;
- operar sem Internet;
- apresentar sua interface gráfica;
- perceber presença;
- executar fluxos autorizados de reconhecimento;
- apresentar conteúdo;
- realizar interações;
- realizar sorteios locais;
- aplicar questionários;
- persistir apenas o necessário;
- recuperar seu estado após reinicialização;
- representar desconhecimento;
- recusar identificação insuficientemente sustentada.

## 7.2 Sempre incompleto

Sua configuração presente não define os limites das capacidades futuras.

Poderão ser incorporados posteriormente:

- novos conteúdos;
- novos sensores;
- novas interfaces;
- novos modelos perceptivos;
- voz;
- áudio;
- touchscreen;
- dispositivos associados;
- outras formas de identidade;
- novos mecanismos de interação;
- novas políticas de seleção;
- sincronização opcional;
- novos domínios de aplicação.

Essas extensões pertencem ao ELO e não exigem, por princípio, alteração do `ente-kernel` ou do JEV.

---

# 8. Linguagem de implementação

A realização inicial do ELO será concebida em:

```text
C++26
```

C++26 será utilizado para:

- núcleo da aplicação;
- modelos de domínio;
- ciclo de vida;
- integração com `ente-kernel`;
- integração com JEV;
- persistência;
- percepção;
- biometria;
- randomização;
- coordenação da experiência;
- serviços locais;
- integração com a interface.

O projeto deverá evitar dependência desnecessária de linguagens adicionais para a lógica principal.

---

# 9. Interface gráfica

A interface gráfica faz parte da primeira realização do ELO.

A tecnologia candidata inicial é:

```text
Qt 6
+
Qt Quick
+
QML
```

A lógica de domínio permanecerá em C++26.

QML deverá ser utilizado predominantemente para:

- composição visual;
- telas;
- animações;
- componentes gráficos;
- estados de apresentação;
- interação homem-máquina.

A GUI não deve carregar regras constitutivas ou regras críticas de domínio.

```text
QML
=
PRESENTATION

C++26
=
DOMAIN + APPLICATION + INFRASTRUCTURE
```

---

# 10. Interface não define o sistema

Qt não constitui propriedade constitutiva do ELO.

```text
ELO
≠
QT
```

A primeira realização poderá utilizar Qt 6.

Uma realização futura poderá substituir a camada gráfica sem necessariamente produzir uma nova identidade de sistema.

Portanto:

```text
Qt 6
   ↓
outra interface futura

≠
novo ELO automaticamente
```

---

# 11. Perfil inicial de interface

O primeiro ELO será concebido como experiência do tipo:

```text
KIOSK / TOTEM
```

A interface inicial deverá suportar:

```text
monitor
+
mouse
```

e ser arquitetada desde o início para futura compatibilidade com:

```text
touchscreen
teclado
botões físicos
outros dispositivos de entrada
```

A primeira interface deve priorizar:

- tela cheia;
- elementos grandes;
- baixa carga cognitiva;
- poucas escolhas simultâneas;
- navegação previsível;
- ausência de janelas do sistema operacional;
- feedback visual imediato;
- retorno automático ao estado inicial;
- acessibilidade;
- funcionamento integral offline.

---

# 12. Interface offline-first

A interface não poderá depender de:

- páginas web remotas;
- CDN;
- fontes externas;
- APIs de Internet;
- JavaScript remoto;
- autenticação em nuvem;
- conteúdo carregado obrigatoriamente pela rede.

Todos os recursos necessários à função primária devem existir localmente.

```text
NETWORK = OFF
```

deve ainda permitir:

```text
GUI
CONTENT
IDENTIFICATION
SURVEY
RANDOMIZATION
PERSISTENCE
```

---

# 13. Substrato candidato inicial

A primeira realização terá como alvo:

```text
Raspberry Pi 5
```

preferencialmente em arquitetura:

```text
ARM64
```

O Raspberry Pi 5 constitui o primeiro substrato.

Não constitui a identidade do ELO.

```text
ELO
≠
RPi 5
```

---

# 14. Papel do ente-kernel

O `ente-kernel` constitui componente consumido pelo ELO.

Sua função não é:

- reconhecer pessoas;
- executar interface;
- armazenar biometria;
- apresentar conteúdo;
- sortear participantes;
- executar questionários.

Sua responsabilidade permanece restrita às primitivas constitutivas para as quais foi criado.

O ELO poderá manter:

```cpp
ente::kernel kernel_;
```

e utilizá-lo como fundamento para:

```text
identidade
continuidade
transformação
estado de existência
```

A presença dessa instância não transforma o ELO em ENTE.

---

# 15. Papel do JEV

O JEV constitui mecanismo independente de **julgamento probabilístico**.

O ELO poderá consumir JEV para produzir julgamentos estruturados sobre estados ou perguntas em situações em que julgamento probabilístico seja útil.

O contrato conceitual deve preservar:

```text
FACT
≠
JUDGMENT
≠
DECISION
≠
ACTION
```

JEV:

```text
observa estado fornecido
      ↓
julga
      ↓
produz saída estruturada
```

O JEV não executa a ação.

O ELO continua responsável pela decisão operacional.

---

# 16. Primitivas de julgamento do JEV

Quando aplicável, o ELO poderá consumir primitivas como:

```text
Choice

Score

Noul
```

representando julgamentos estruturados e probabilísticos segundo o contrato próprio do JEV.

Exemplo conceitual:

```text
estado ELO
   ↓
pergunta tipada
   ↓
JEV
   ↓
Score / Choice / Noul
   ↓
política do ELO
   ↓
decisão
```

Nunca:

```text
JEV
 ↓
ação física direta
```

---

# 17. JEV não é autoridade factual

Uma saída do JEV constitui:

```text
JUDGMENT
```

e não:

```text
OBSERVED FACT
```

O ELO deverá manter essa distinção.

Especialmente em biometria:

```text
câmera detectou face
=
observação

modelo produziu similaridade
=
resultado computacional

JEV avaliou contexto
=
julgamento

ELO permitiu continuidade
=
decisão
```

Essas categorias não devem ser colapsadas.

---

# 18. Componentes não constituem identidade do sistema

O ELO deve poder substituir componentes sem necessariamente perder sua identidade.

Exemplos:

```text
YuNet
   ↓
novo detector
```

```text
SFace
   ↓
novo encoder
```

```text
Qt 6
   ↓
nova interface
```

```text
Raspberry Pi 5
   ↓
substrato futuro
```

```text
mouse
   ↓
touchscreen
```

```text
JEV Provider A
   ↓
JEV Provider B compatível
```

Mudança de componente:

```text
≠
nova identidade automaticamente
```

---

# 19. Espaços de identidade

Devem permanecer explicitamente separados pelo menos quatro espaços.

## 19.1 Identidade do ELO

Representa esta realização específica do sistema.

Exemplo conceitual:

```text
elo://S01
```

Responde:

> **“Qual ELO continua existindo através destas transformações?”**

---

## 19.2 Identidade do substrato

Representa o equipamento físico atual.

```text
device://D04
```

```text
ELO_ID
≠
DEVICE_ID
```

---

## 19.3 Identidade mantida pelo ente-kernel

Pertence à infraestrutura constitutiva.

```text
KERNEL_ID
```

Não constitui o nome semântico do ELO.

```text
KERNEL_ID
≠
ELO_ID
```

---

## 19.4 Identidade perceptual local da pessoa

Representa uma identidade humana localmente distinguida.

```text
person-local://P17
```

Responde:

> **“Esta observação parece corresponder à mesma pessoa anteriormente encontrada por este ELO?”**

Portanto:

```text
ELO_ID
≠
KERNEL_ID
≠
DEVICE_ID
≠
PERSON_LOCAL_ID
```

---

# 20. Princípio da identidade humana mínima

O ELO não deve exigir identidade civil para proporcionar continuidade.

Uma pessoa poderá existir localmente apenas como:

```text
person-local://P17
```

sem associação obrigatória com:

```text
nome
CPF
e-mail
telefone
endereço
documento
conta externa
rede social
```

O princípio é:

> **reconhecer recorrência sem necessariamente conhecer identidade civil.**

---

# 21. Biometria como evidência

A biometria facial deve constituir evidência probabilística.

Nunca certeza absoluta.

```text
observação facial
       ↓
representação biométrica
       ↓
comparação
       ↓
similaridade
       ↓
hipótese
       ↓
decisão local
```

Estados mínimos:

```text
UNKNOWN
CANDIDATE
SUPPORTED
UNCERTAIN
CONTRADICTORY
```

Quando não houver sustentação suficiente:

```text
UNKNOWN
```

é resposta correta.

---

# 22. Privacidade como propriedade do ELO

Privacidade não constitui opção da interface.

Integra o desenho do sistema.

Princípios iniciais:

```text
OFFLINE_FIRST

LOCAL_PROCESSING

DATA_MINIMIZATION

PURPOSE_LIMITATION

EXPLICIT_CONSENT

REVOCABILITY

NO_SILENT_ENROLLMENT

NO_REQUIRED_CLOUD

NO_HIDDEN_CLASSIFICATION

NO_RAW_FACE_RETENTION_BY_DEFAULT
```

---

# 23. Consentimento

Nenhuma identidade biométrica persistente deverá ser criada silenciosamente.

Antes do cadastro, a pessoa deve compreender:

- que biometria facial será utilizada;
- por qual motivo;
- o que será armazenado;
- o que não será armazenado;
- que imagens não serão preservadas por padrão;
- que o processamento é local;
- como desistir;
- como solicitar esquecimento.

O consentimento deve decorrer de ação positiva.

```text
ausência de recusa
≠
consentimento
```

---

# 24. Fluxo inicial de participação

```text
pessoa aproxima-se
        │
        ▼
interface apresenta
a experiência
        │
        ▼
deseja participar?
     ┌──┴──┐
    não   sim
     │     │
     ▼     ▼
 termina  experiência
             │
             ▼
      biometria será utilizada?
          ┌──┴──┐
         não   sim
          │     │
          ▼     ▼
       sessão  explicar
   temporária  finalidade
                 │
                 ▼
            consentimento?
              ┌──┴──┐
             não   sim
              │     │
              ▼     ▼
           sessão  continuidade
        sem memória biométrica
```

---

# 25. Não retenção de imagem por padrão

A imagem deve ser transitória.

```text
câmera
  ↓
RAM
  ↓
detecção
  ↓
alinhamento
  ↓
representação biométrica
  ↓
imagem descartada
```

Por padrão não devem ser persistidos:

- frames;
- fotografias;
- vídeos;
- recortes faciais.

---

# 26. Representação biométrica local

Quando necessária e autorizada, o ELO poderá armazenar uma representação biométrica derivada.

```text
FaceTemplate
├── template_id
├── person_local_id
├── model_id
├── model_version
├── representation
├── quality
├── created_at
└── integrity_digest
```

Essa informação:

- permanece local por padrão;
- deve ser protegida;
- deve poder ser eliminada;
- não deve ser considerada anônima;
- não pode adquirir outra finalidade silenciosamente.

---

# 27. Separação de memória

A biometria deve permanecer separada do estado funcional da experiência.

```text
BiometricStore
      │
      ▼
person-local://P17
      │
      ▼
ExperienceStore
      │
      ├── conteúdos apresentados
      ├── progresso
      ├── preferências autorizadas
      └── estado da experiência
```

O vetor biométrico não constitui identificador universal.

---

# 28. Direito local ao esquecimento

O ELO deve ser capaz de esquecer uma identidade sem Internet.

```text
FORGET(P17)
      ↓
remove associação biométrica
      ↓
remove templates
      ↓
remove ou desvincula
dados associados aplicáveis
```

Depois:

```text
novo encontro
     ↓
UNKNOWN
```

quando não houver outra base autorizada de continuidade.

---

# 29. Offline-first

A ausência de rede não deve impedir a função primária.

```text
network = unavailable
```

deve continuar permitindo:

```text
boot
GUI
percepção
reconhecimento
conteúdo
interação
questionário
randomização
persistência
recuperação
esquecimento
```

Rede constitui extensão.

Não condição de existência.

---

# 30. Rede como capacidade opcional

Uma conexão futura poderá permitir:

- atualização de software;
- obtenção de novos conteúdos;
- sincronização autorizada;
- backup autorizado;
- diagnóstico;
- telemetria consentida.

Mas:

```text
NETWORK_LOSS
≠
ELO_FAILURE
```

---

# 31. Personalização

A personalização deve utilizar apenas dados compatíveis com sua finalidade.

Exemplos:

```text
conteúdo já apresentado
progresso
preferências fornecidas
participação anterior
respostas autorizadas
estado de sequência
```

O ELO não deve produzir perfis adicionais apenas porque tecnicamente pode fazê-lo.

---

# 32. Não inferência silenciosa de características sensíveis

O reconhecimento facial não deve ser reutilizado para inferir silenciosamente:

- sexo;
- gênero;
- raça;
- etnia;
- saúde;
- religião;
- orientação sexual;
- posição política;
- condição socioeconômica;
- personalidade.

O componente facial existe para:

```text
continuidade de identidade local
```

e não:

```text
classificação humana geral
```

---

# 33. Seleção aleatória de participantes

Uma aplicação ELO poderá selecionar participantes aleatoriamente.

```text
pessoa elegível
      ↓
randomização local
      ↓
selecionada?
   ┌──┴──┐
  não   sim
   │     │
   ▼     ▼
segue   convite
```

A randomização deve ser independente da aparência facial.

---

# 34. Estratificação de amostra

Caso uma pesquisa exija grupos específicos, esses grupos não devem ser inferidos biometricamente.

Quando metodologicamente necessário:

```text
participante
     ↓
autodeclaração
     ↓
regra metodológica
     ↓
sorteio
```

A informação deverá ser utilizada exclusivamente segundo o propósito declarado.

---

# 35. Questionários

Uma primeira aplicação poderá utilizar:

```text
monitor
+
mouse
```

com questionários curtos.

Princípios:

- poucas perguntas;
- uma pergunta por tela;
- linguagem clara;
- poucas alternativas;
- botões grandes;
- confirmação explícita;
- possibilidade de desistência;
- ausência de rolagem quando possível.

---

# 36. Separação entre identificação e resposta

A aplicação deve declarar se a resposta necessita permanecer ligada à identidade local.

## 36.1 Resposta vinculada

```text
P17
 ↓
resposta
```

somente quando necessário e autorizado.

## 36.2 Resposta desvinculada

```text
P17
 ↓
participação validada
 ↓
vínculo removido
 ↓
resposta
```

A desvinculação deve ser preferida quando o objetivo não exigir acompanhamento longitudinal.

---

# 37. Continuidade da experiência

O reconhecimento biométrico existe prioritariamente para evitar que cada encontro seja tratado como primeiro encontro.

```text
DIA 1

P17
 ↓
conteúdo A
 ↓
estado salvo
```

Posteriormente:

```text
DIA 8

observação
 ↓
compatível com P17
 ↓
conteúdo A já apresentado
 ↓
conteúdo B
```

O valor está na continuidade.

Não no conhecimento da identidade civil.

---

# 38. Arquitetura conceitual

```text
                        PESSOA
                           │
                           ▼
                    interface ELO
                           │
                           ▼
                    percepção local
                           │
                           ▼
                 representação facial
                           │
                           ▼
                  comparação biométrica
                           │
                           ▼
                   Identity Resolver
                           │
                    ┌──────┴──────┐
                    │             │
                 UNKNOWN       P17 provável
                                    │
                                    ▼
                              Experience State
                                    │
                     ┌──────────────┼──────────────┐
                     ▼              ▼              ▼
                  conteúdo      questionário    julgamento
                                                   │
                                                   ▼
                                                  JEV
                                    │
                                    ▼
                               decisão ELO
                                    │
                                    ▼
                                Qt / QML
```

Em paralelo:

```text
                       ELO
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
    ente-kernel        JEV         domínio
```

---

# 39. Arquitetura de software candidata

```text
elo/
│
├── CMakeLists.txt
│
├── cmake/
│
├── docs/
│
├── include/
│   └── elo/
│       ├── core/
│       ├── identity/
│       ├── perception/
│       ├── biometric/
│       ├── experience/
│       ├── judgment/
│       ├── survey/
│       ├── storage/
│       ├── platform/
│       └── ui/
│
├── src/
│   ├── core/
│   ├── identity/
│   ├── perception/
│   ├── biometric/
│   ├── experience/
│   ├── judgment/
│   ├── survey/
│   ├── storage/
│   ├── platform/
│   └── ui/
│
├── ui/
│   ├── Main.qml
│   ├── screens/
│   ├── components/
│   └── assets/
│
├── tests/
│
├── apps/
│   └── elo-kiosk/
│
└── external/
    ├── ente-kernel/
    └── jev/
```

A estrutura é candidata.

Não normativa nesta versão.

---

# 40. Fronteira C++26 / QML

A interface deverá comunicar-se com o domínio através de fronteiras explícitas.

Exemplo:

```text
QML
 ↓
PresentationModel
 ↓
Application Service
 ↓
Domain
```

Nunca:

```text
QML
 ↓
SQLite diretamente

QML
 ↓
modelo biométrico diretamente

QML
 ↓
ente-kernel diretamente

QML
 ↓
JEV diretamente
```

A GUI não deve conhecer os detalhes internos dessas dependências.

---

# 41. Estado visual

A interface poderá refletir estados de aplicação como:

```text
IDLE

PRESENCE_DETECTED

INFORMATION_PRESENTED

CONSENT_PENDING

NON_BIOMETRIC_SESSION

BIOMETRIC_SESSION

IDENTITY_UNKNOWN

IDENTITY_CANDIDATE

IDENTITY_SUPPORTED

IDENTITY_UNCERTAIN

CONTENT_ACTIVE

SURVEY_ELIGIBLE

SURVEY_SELECTED

SURVEY_ACTIVE

SESSION_COMPLETE
```

A máquina de estados pertence ao ELO.

A tela apenas representa seu estado atual.

---

# 42. Estados explícitos de ausência de conhecimento

O sistema deverá representar:

```text
UNKNOWN

UNCERTAIN

CONTRADICTORY

UNAVAILABLE

NOT_CONSENTED

NOT_APPLICABLE
```

Nenhum deverá ser silenciosamente transformado em sucesso.

---

# 43. Persistência local

A persistência deverá distinguir pelo menos:

```text
ELO State

Kernel State

Biometric State

Experience State

Survey State

Configuration
```

Possível organização:

```text
elo.db

biometric.db

experience.db

survey.db

configuration.db
```

A forma final permanece aberta.

---

# 44. Liveness

Reconhecimento facial não deve ser tratado como autenticação forte isoladamente.

Uma foto, vídeo ou tela pode produzir evidência facial.

Por isso o ELO deverá poder representar:

```text
LIVENESS_NOT_VERIFIED
```

Mecanismos futuros poderão fortalecer a evidência de presença.

---

# 45. Identidade probabilística

O ELO não deverá afirmar:

> “Esta pessoa é definitivamente P17.”

A formulação operacional correta é:

> **“A observação atual apresenta sustentação suficiente, segundo os critérios desta aplicação, para ser tratada como continuidade da identidade local P17.”**

---

# 46. Autonomia da pessoa

A pessoa deve permanecer superior à conveniência da experiência.

```text
participar
é opcional

usar biometria
é opcional quando a finalidade permitir

ser lembrada
é opcional

responder
é opcional

continuar
é opcional

solicitar esquecimento
deve ser possível
```

---

# 47. Primeira aplicação experimental candidata

A primeira realização poderá funcionar como um totem de conteúdo personalizado.

```text
1. ELO inicia offline

2. interface entra em IDLE

3. presença é detectada

4. experiência é apresentada

5. pessoa decide participar

6. consentimento aplicável é verificado

7. reconhecimento local ocorre quando autorizado

8. identidade local resulta em:
      UNKNOWN
      ou
      Pxx

9. conteúdo é selecionado

10. elegibilidade para pesquisa é verificada

11. sorteio local é realizado

12. participante selecionado
    recebe poucas perguntas

13. respostas são registradas
    segundo política declarada

14. sessão é encerrada

15. imagens transitórias são descartadas

16. interface retorna a IDLE
```

---

# 48. Aleatoriedade

Sorteios devem utilizar fonte de aleatoriedade local adequada.

Não devem influenciar a seleção:

- aparência;
- pose;
- iluminação;
- qualidade facial;
- similaridade biométrica;
- frequência de reconhecimento.

O ELO poderá consumir mecanismo próprio de aleatoriedade ou componente adequado sem vincular essa responsabilidade ao modelo facial.

---

# 49. Logs

Evitar:

```text
face.jpg
embedding=[...]
raw_frame=...
```

Preferir:

```text
session=...
identity_state=SUPPORTED
model_version=...
decision=...
```

Identificadores locais também deverão ser omitidos quando não forem necessários.

---

# 50. Invariantes iniciais candidatos

## E1 — SYSTEM_AUTONOMY

O ELO possui identidade, propósito e constituição próprios.

## E2 — ENTE_INDEPENDENCE

O ELO consome `ente-kernel` sem incorporar seu domínio ao kernel.

## E3 — JEV_INDEPENDENCE

O ELO consome JEV sem redefini-lo.

## E4 — JUDGMENT_IS_NOT_ACTION

Uma saída do JEV não constitui autoridade automática para ação.

## E5 — OFFLINE_PRIMARY_OPERATION

A função primária permanece disponível sem rede.

## E6 — LOCAL_GUI_AVAILABILITY

A interface principal funciona integralmente sem Internet.

## E7 — PRESENTATION_DOMAIN_SEPARATION

A GUI não contém lógica crítica de domínio, biometria ou identidade.

## E8 — PERSON_AUTONOMY

Participação depende de decisão da pessoa.

## E9 — NO_SILENT_ENROLLMENT

Não existe cadastro biométrico persistente silencioso.

## E10 — RAW_IMAGE_EPHEMERALITY

Imagens são transitórias por padrão.

## E11 — MINIMUM_PERSISTENCE

Somente informações necessárias são persistidas.

## E12 — BIOMETRIC_IS_EVIDENCE

Correspondência facial constitui evidência probabilística.

## E13 — UNKNOWN_IS_VALID

Desconhecimento constitui estado válido.

## E14 — LOCAL_FORGETTING

A continuidade biométrica pode ser eliminada localmente.

## E15 — IDENTITY_SPACE_SEPARATION

Identidade do ELO, do kernel, do hardware e das pessoas permanecem distintas.

## E16 — NO_SENSITIVE_TRAIT_INFERENCE

Biometria facial não é utilizada para classificação silenciosa de características sensíveis.

## E17 — RANDOMIZATION_INDEPENDENCE

Sorteios não dependem da aparência facial.

## E18 — PURPOSE_BOUND_DATA

Informações permanecem ligadas à finalidade declarada.

## E19 — REVOCABLE_PARTICIPATION

A pessoa pode interromper sua participação.

## E20 — CAPABILITY_MUTABILITY

O ELO pode adquirir novas capacidades sem exigir especialização do ENTE ou do JEV.

## E21 — SUBSTRATE_INDEPENDENCE

Raspberry Pi 5 constitui primeiro alvo, não definição do ELO.

## E22 — UI_REPLACEABILITY

Qt 6 constitui primeira realização gráfica, não identidade do sistema.

---

# 51. Critérios de falha constitucional

O ELO viola esta constituição se:

- passar a ser tratado como uma versão especializada do ENTE;
- alterar o `ente-kernel` apenas para incorporar lógica do ELO;
- modificar o JEV apenas para acomodar domínio específico do ELO;
- tratar julgamento JEV como fato observado;
- permitir que JEV execute diretamente uma ação;
- exigir nuvem para sua função primária;
- tornar a interface dependente de conteúdo remoto obrigatório;
- colocar lógica biométrica diretamente em QML;
- armazenar imagens silenciosamente;
- cadastrar biometria sem consentimento;
- inferir atributos sensíveis a partir da face;
- exigir identidade civil sem necessidade;
- tratar baixa confiança como identificação positiva;
- impedir esquecimento local;
- reutilizar informações para finalidade incompatível;
- confundir identidade do ELO com identidade da pessoa;
- confundir identidade do hardware com identidade do ELO.

---

# 52. Critérios iniciais de sucesso experimental

A primeira realização deverá demonstrar:

```text
S1
build em C++26

S2
execução em Raspberry Pi 5 ARM64

S3
boot completamente offline

S4
GUI Qt 6/QML disponível offline

S5
modo fullscreen/kiosk

S6
interação por mouse

S7
identidade do ELO preservada
com auxílio do ente-kernel

S8
ente-kernel não modificado
para o domínio ELO

S9
JEV consumido por contrato

S10
JEV não executa ações

S11
detecção facial local

S12
representação biométrica local

S13
criação consentida de identidade local

S14
reconhecimento posterior da mesma pessoa

S15
UNKNOWN diante de evidência insuficiente

S16
experiência personalizada por continuidade

S17
sorteio local independente da biometria

S18
questionário curto pela GUI

S19
descarte de imagens

S20
reinicialização com recuperação

S21
esquecimento local

S22
funcionamento integral sem Internet
```

---

# 53. Experimentos iniciais candidatos

## EXP-ELO-001 — Offline Boot

Rede ausente desde o início.

Esperado:

```text
ELO boots
GUI available
PRIMARY_FUNCTION_AVAILABLE
```

---

## EXP-ELO-002 — First Encounter

Pessoa desconhecida aproxima-se.

Esperado:

```text
UNKNOWN
```

Nenhuma identidade persistente é criada automaticamente.

---

## EXP-ELO-003 — Consented Enrollment

A pessoa autoriza continuidade biométrica.

Esperado:

```text
person-local://P01
```

sem necessidade de identidade civil.

---

## EXP-ELO-004 — Return

A pessoa retorna.

Esperado:

```text
candidate P01
      ↓
supported P01
```

e continuidade da experiência.

---

## EXP-ELO-005 — Ambiguous Match

Mais de uma identidade apresenta evidência relevante.

Esperado:

```text
UNCERTAIN
```

---

## EXP-ELO-006 — Network Loss

Rede desaparece durante a sessão.

Esperado:

```text
PRIMARY_FUNCTION_AVAILABLE
```

---

## EXP-ELO-007 — Forget Me

Pessoa solicita exclusão da memória biométrica.

Esperado:

```text
association removed
```

Encontro futuro:

```text
UNKNOWN
```

---

## EXP-ELO-008 — Random Selection

Pessoa elegível participa de sorteio.

Esperado:

```text
resultado independente
da aparência facial
```

---

## EXP-ELO-009 — Restart

Sistema é reiniciado.

Esperado:

```text
mesma identidade ELO
+
estado autorizado recuperado
```

---

## EXP-ELO-010 — Kernel Independence

O módulo facial é substituído.

Esperado:

```text
ente-kernel unchanged
```

---

## EXP-ELO-011 — JEV Independence

Uma capacidade exclusiva do ELO muda.

Esperado:

```text
JEV unchanged
```

---

## EXP-ELO-012 — UI Replacement

Uma tela QML é completamente substituída.

Esperado:

```text
domain behavior unchanged
```

---

## EXP-ELO-013 — Mouse Interaction

Questionário é concluído integralmente utilizando apenas mouse.

Esperado:

```text
SURVEY_COMPLETE
```

---

## EXP-ELO-014 — UI Recovery

Aplicação retorna ao estado inicial após abandono ou término da sessão.

Esperado:

```text
SESSION_COMPLETE
      ↓
IDLE
```

sem exposição de dados da pessoa anterior.

---

# 54. Hipótese física inicial

```text
Raspberry Pi 5
│
├── Linux ARM64
│
├── aplicação ELO
│   │
│   ├── C++26 Core
│   │
│   ├── ente-kernel
│   │
│   ├── JEV Adapter
│   │
│   ├── Perception
│   │
│   ├── Biometrics
│   │
│   ├── Identity Resolver
│   │
│   ├── Experience Engine
│   │
│   ├── Survey Engine
│   │
│   ├── Randomization
│   │
│   ├── Local Storage
│   │
│   └── Qt 6 / QML UI
│   │
│   └── fullscreen / kiosk
│
├── câmera
├── monitor
└── mouse
```

Rede:

```text
OPCIONAL
```

---

# 55. Stack candidato inicial

```text
LANGUAGE
C++26

BUILD
CMake
Ninja

GUI
Qt 6
Qt Quick
QML

PLATFORM
Linux ARM64
Raspberry Pi 5

CAMERA
libcamera / interface compatível

VISION
OpenCV / backend substituível

MODEL EXECUTION
ONNX Runtime ou backend substituível

PERSISTENCE
local
formato ainda não normativo

IDENTITY FOUNDATION
ente-kernel

PROBABILISTIC JUDGMENT
JEV
```

Nenhum item, exceto quando explicitamente promovido em versão futura desta constituição, constitui dependência ontológica irreversível.

---

# 56. Questões deliberadamente abertas

Esta versão ainda não define:

- modelo facial definitivo;
- threshold biométrico definitivo;
- algoritmo definitivo de liveness;
- estrutura criptográfica final;
- política temporal definitiva de retenção;
- banco de dados definitivo;
- conteúdo da primeira experiência;
- metodologia definitiva de sorteio;
- quantidade definitiva de perguntas;
- aparência visual final;
- resolução de tela;
- câmera definitiva;
- touchscreen;
- mecanismo de atualização;
- backup;
- sincronização;
- relacionamento entre múltiplos ELOs;
- migração de hardware;
- continuidade entre dispositivos;
- outras capacidades futuras.

Essas lacunas são deliberadas.

> **Não definir o que ainda não precisa ser definido também é uma propriedade do desenho.**

---

# 57. Declaração final provisória

O ELO não existe para identificar pessoas indiscriminadamente.

Também não existe para demonstrar o ENTE ou o JEV.

Esses componentes são meios.

Não são seu propósito.

O ELO existe para investigar uma forma de proporcionar experiências locais contínuas e personalizadas nas quais uma pessoa possa ser reconhecida, quando assim desejar, sem precisar entregar ao sistema mais informação do que a experiência necessita.

A biometria facial constitui apenas uma capacidade possível de restabelecer o elo entre encontros.

O `ente-kernel` fornece uma capacidade constitutiva consumida pelo sistema.

O JEV fornece julgamento probabilístico consumido segundo seu próprio contrato.

Qt fornece a primeira superfície gráfica.

Raspberry Pi 5 fornece o primeiro substrato físico.

Nenhum deles, isoladamente, define o ELO.

```text
ENTE
não é ELO

JEV
não é ELO

FACE
não é ELO

QT
não é ELO

RASPBERRY PI
não é ELO
```

O ELO deve permanecer capaz de incorporar, substituir e abandonar capacidades específicas sem perder desnecessariamente sua identidade.

A pessoa nunca deve ser tratada como propriedade do sistema.

Ela participa da experiência sob seus próprios termos.

O elo existe para servi-la.

Não para aprisioná-la.

---

> **Sempre pronto. Sempre incompleto.**
