#!/usr/bin/env bash
# ==============================================================================
# ELO — Autonomous Sovereign Appliance Installer for Raspberry Pi 5
# Tested on: Raspberry Pi OS (64-bit / Bookworm / Debian 13)
# ==============================================================================
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${BLUE}${BOLD}================================================================${NC}"
echo -e "${GREEN}${BOLD}   ELO — Instalador do Sistema para Raspberry Pi 5             ${NC}"
echo -e "${GREEN}${BOLD}   Totem Soberano Offline & Studio Local de Gestão de Conteúdo   ${NC}"
echo -e "${BLUE}${BOLD}================================================================${NC}"

# Detect current user
CURRENT_USER="${SUDO_USER:-$USER}"
if [ "$CURRENT_USER" = "root" ] && [ -n "$SUDO_USER" ]; then
    CURRENT_USER="$SUDO_USER"
fi
USER_HOME=$(eval echo "~${CURRENT_USER}")

echo -e "Usuário alvo para execução: ${GREEN}${CURRENT_USER}${NC} (Home: ${USER_HOME})"
echo ""

# 1. Checagem de privilégios para instalação de pacotes
if [ "$(id -u)" -ne 0 ]; then
    echo -e "${YELLOW}Aviso: Para instalar dependências e serviços de sistema, serão solicitados privilégios sudo.${NC}"
    SUDO_CMD="sudo"
else
    SUDO_CMD=""
fi

# 2. Instalação de Dependências do Sistema no Raspberry Pi OS / Debian
echo -e "${BLUE}>>> [1/6] Atualizando repositórios e instalando dependências do sistema...${NC}"
$SUDO_CMD apt-get update -y

$SUDO_CMD apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    curl \
    v4l-utils \
    libsqlite3-dev \
    libopencv-dev \
    libdrm-dev \
    libgbm-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libxkbcommon-dev \
    libxcb-cursor0 \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-declarative-dev-tools \
    libqt6quick6 \
    libqt6qml6 \
    libqt6network6 \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window \
    qml6-module-qtcore \
    qml6-module-qtqml

# 3. Configuração de Grupos e Permissões de Hardware (Câmera, GPU e DRM/KMS)
echo -e "${BLUE}>>> [2/6] Configurando grupos de acesso a vídeo, GPU e entrada para '${CURRENT_USER}'...${NC}"
for grp in video render input audio tty; do
    if getent group "$grp" >/dev/null 2>&1; then
        $SUDO_CMD usermod -aG "$grp" "$CURRENT_USER" || true
    fi
done

# 4. Compilação Otimizada
echo -e "${BLUE}>>> [3/6] Compilando binários do ELO com suporte nativo a ARM64...${NC}"
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

mkdir -p build
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DELO_DOWNLOAD_VISION_MODELS=ON

ninja -C build

# 5. Instalação de Binários, Ferramentas CLI e Assets
echo -e "${BLUE}>>> [4/6] Instalando binários e ferramentas em /usr/local/bin e /usr/local/share...${NC}"
$SUDO_CMD ninja -C build install 2>/dev/null || {
    # Fallback se install target não estiver ativo
    $SUDO_CMD cp build/apps/elo-kiosk/elo-kiosk /usr/local/bin/
    $SUDO_CMD cp build/apps/elo-admin/elo-admin /usr/local/bin/
}

# Instalar utilitário CLI elo-ctl e elo-kiosk-launcher
$SUDO_CMD cp deploy/scripts/elo-ctl /usr/local/bin/
$SUDO_CMD cp deploy/scripts/elo-kiosk-launcher /usr/local/bin/
$SUDO_CMD chmod +x /usr/local/bin/elo-ctl
$SUDO_CMD chmod +x /usr/local/bin/elo-kiosk-launcher
$SUDO_CMD chmod +x /usr/local/bin/elo-kiosk
$SUDO_CMD chmod +x /usr/local/bin/elo-admin

# Instalar arquivos estáticos da web de administração
$SUDO_CMD mkdir -p /usr/local/share/elo/admin
$SUDO_CMD mkdir -p /usr/local/share/elo/web/admin
$SUDO_CMD cp -r web/admin/* /usr/local/share/elo/admin/
$SUDO_CMD cp -r web/admin/* /usr/local/share/elo/web/admin/

# 6. Inicialização do Espaço de Dados do Usuário (Catálogo Bioma Pampa & Seeds)
echo -e "${BLUE}>>> [5/6] Preparando diretório soberano de conteúdo em '${USER_HOME}/.local/share/elo'...${NC}"
CONTENT_DIR="${USER_HOME}/.local/share/elo/content"
mkdir -p "${CONTENT_DIR}/catalog/atoms"
mkdir -p "${CONTENT_DIR}/catalog/relations"
mkdir -p "${CONTENT_DIR}/catalog/recipes"
mkdir -p "${CONTENT_DIR}/assets/images"
mkdir -p "${CONTENT_DIR}/assets/audio"

# Se houver sementes do repositório, copiar se vazio
if [ -d "assets/images" ]; then
    cp -rn assets/images/* "${CONTENT_DIR}/assets/images/" 2>/dev/null || true
fi
if [ -d "data/content" ]; then
    cp -rn data/content/* "${CONTENT_DIR}/" 2>/dev/null || true
fi
chown -R "${CURRENT_USER}:${CURRENT_USER}" "${USER_HOME}/.local/share/elo" || true

# 7. Configuração dos Serviços Systemd
echo -e "${BLUE}>>> [6/6] Instalando e habilitando serviços no Systemd...${NC}"
$SUDO_CMD cp deploy/systemd/elo-admin.service /etc/systemd/system/elo-admin@.service
$SUDO_CMD cp deploy/systemd/elo-kiosk.service /etc/systemd/system/elo-kiosk@.service
$SUDO_CMD systemctl daemon-reload

$SUDO_CMD systemctl enable "elo-admin@${CURRENT_USER}"
$SUDO_CMD systemctl enable "elo-kiosk@${CURRENT_USER}"

IP_ADDR=$(hostname -I | awk '{print $1}')

echo ""
echo -e "${GREEN}${BOLD}================================================================${NC}"
echo -e "${GREEN}${BOLD}   ✓ Instalação concluída com sucesso no Raspberry Pi 5!       ${NC}"
echo -e "${GREEN}${BOLD}================================================================${NC}"
echo ""
echo -e "Para gerenciar o sistema, use o comando: ${BLUE}${BOLD}elo-ctl${NC}"
echo ""
echo -e "Comandos rápidos:"
echo -e "  Iniciar agora:       ${GREEN}elo-ctl start${NC}"
echo -e "  Ver status:          ${BLUE}elo-ctl status${NC}"
echo -e "  Acessar Studio Web:  ${BOLD}http://${IP_ADDR}:8080${NC}"
echo -e "  Ver logs do Totem:   ${YELLOW}elo-ctl logs-kiosk${NC}"
echo ""
echo -e "${YELLOW}Nota: Para que as novas permissões de grupo de vídeo e GPU tenham efeito imediato,${NC}"
echo -e "${YELLOW}recomendamos reiniciar o Raspberry Pi com: ${BOLD}sudo reboot${NC}"
echo ""
