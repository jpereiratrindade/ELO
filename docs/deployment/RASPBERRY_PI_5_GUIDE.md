# Guia de Instalação e Operação no Raspberry Pi 5 (ELO Appliance)

Este documento orienta a instalação e execução do sistema **ELO (Totem Soberano Offline & Studio de Conteúdo)** no **Raspberry Pi 5** (utilizando Raspberry Pi OS 64-bit / Bookworm ou Debian 13 aarch64).

---

## 1. Requisitos do Sistema

* **Hardware:** Raspberry Pi 5 (4GB ou 8GB de RAM recomendados).
* **Display:** Monitor ou TV conectado via micro-HDMI (HDMI-0 ou HDMI-1).
* **Câmera:** Câmera USB UVC padrão (ex: Logitech C920/C922) ou Raspberry Pi Camera Module v2/v3.
* **Sistema Operacional:** Raspberry Pi OS Lite ou Desktop (64-bit).

---

## 2. Instalação Automatizada em 1 Comando

Para configurar o Raspberry Pi 5 do zero (instalação de dependências, compilação de alta performance, configuração de permissões de hardware e serviços de inicialização automática no boot), execute no terminal do Raspberry Pi:

```bash
cd ~/ELO
chmod +x scripts/install_rpi5.sh
./scripts/install_rpi5.sh
```

O script realizará automaticamente:
1. Instalação de todas as dependências do Qt6 (QML, Quick, Network), OpenCV, SQLite3, DRM/KMS e V4L2.
2. Atribuição de permissões de hardware ao usuário (`video`, `render`, `input`, `audio`, `tty`).
3. Compilação otimizada com Ninja e detecção de modelos neurais biométricos.
4. Instalação dos binários em `/usr/local/bin` e assets em `/usr/local/share/elo`.
5. Registro e habilitação dos serviços Systemd (`elo-admin` e `elo-kiosk`) para início automático no boot.
6. Instalação do utilitário de controle de linha de comando: `elo-ctl`.

Após a primeira instalação, reinicie o Raspberry Pi para aplicar as permissões de GPU/vídeo:
```bash
sudo reboot
```

---

## 3. Gerenciamento com a Ferramenta CLI `elo-ctl`

Após a instalação, o utilitário `elo-ctl` estará disponível globalmente no terminal:

| Comando | Descrição |
| :--- | :--- |
| `elo-ctl status` | Exibe o estado do Admin Studio, do Totem Kiosk, do Socket IPC e câmeras |
| `elo-ctl start` | Inicia o Admin Studio e o Totem Kiosk |
| `elo-ctl stop` | Interrompe o Totem Kiosk e o Admin Studio |
| `elo-ctl restart` | Reinicia todos os módulos do ELO |
| `elo-ctl reload-content` | Notifica o Totem Kiosk para recarregar o catálogo imediatamente |
| `elo-ctl logs-kiosk` | Acompanha os logs em tempo real do Totem (face recognition, rendering) |
| `elo-ctl logs-admin` | Acompanha os logs de requisições HTTP e publicações do Admin Studio |
| `elo-ctl camera-check` | Testa e lista os dispositivos `/dev/video*` e permissões |
| `elo-ctl admin-url` | Mostra o endereço IP local para abrir o painel web no navegador |

---

## 4. Acessando o Sovereign Content Studio via Rede Local

Com o serviço ativo, acesse o painel administrativo de qualquer computador, tablet ou celular conectado na mesma rede:

```text
http://<IP_DO_RASPBERRY_PI>:8080
```
*(Exemplo: `http://10.163.4.146:8080`)*

No Studio Web você pode:
* Criar e editar Átomos de Conteúdo e variantes multimodais (Texto, Imagens, Áudios).
* Vincular relações de contexto e criar receitas interativas.
* Gerenciar Projetos e Aplicações soberanas ativas.
* Iniciar, encerrar ou reiniciar o Totem remotamente com 1 clique.
* Visualizar métricas agregadas anônimas de engajamento do público.

---

## 5. Modos de Execução Manual do Totem (Kiosk)

Se preferir rodar manualmente fora do systemd:

### A) Modo Console TTY Direto (Sem ambiente desktop)
O ELO Kiosk possui detecção automática de hardware e roda diretamente na GPU via **EGLFS (KMS/DRM)**:
```bash
/usr/local/bin/elo-kiosk
```
*(Ou forçando o backend explicitamente: `/usr/local/bin/elo-kiosk -platform eglfs`)*

### B) Modo Desktop (Sessão Wayland / Labwc / Wayfire)
Se o Raspberry Pi estiver logado na interface gráfica do Desktop:
```bash
export WAYLAND_DISPLAY=wayland-0
export XDG_RUNTIME_DIR=/run/user/$(id -u)
/usr/local/bin/elo-kiosk
```

### C) Modo Desktop X11
```bash
export DISPLAY=:0
/usr/local/bin/elo-kiosk
```
