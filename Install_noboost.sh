#!/bin/bash

# Abilita uscita immediata in caso di errore
set -e

echo "Configurazione del firewall UFW..."

sudo ufw enable
sudo ufw allow 22/tcp         # SSH
sudo ufw allow 21/tcp         # FTP
sudo ufw allow 4000/tcp       # NoMachine
sudo ufw allow 18080/tcp      # Mevacoin P2P
sudo ufw allow 18081/tcp      # Mevacoin RPC
sudo ufw allow 18082/tcp      # Mevacoin RPC secondario
sudo ufw allow 'Apache'       # HTTP (porta 80)
sudo ufw allow 'Apache Full'  # HTTP + HTTPS (porte 80 e 443)

echo "Firewall configurato."

echo "Aggiornamento del sistema..."
sudo apt update && sudo apt upgrade -y

echo "Installazione pacchetti di base e Apache..."
sudo apt install -y build-essential cmake git ccache clang g++ python3 python3-pip \
libssl-dev libzmq3-dev libsodium-dev libbz2-dev zlib1g-dev liblzma-dev \
libboost-all-dev apache2

echo "Preparazione Mevacoin..."
if [ -d "/opt/mevacoin" ]; then
    if [ -d "/opt/mevacoin/.git" ]; then
        echo "Aggiorno repository Mevacoin..."
        cd /opt/mevacoin
        git reset --hard
        git pull origin main
    else
        echo "Rimuovo cartella non-Git e riclono..."
        sudo rm -rf /opt/mevacoin
        git clone https://github.com/pasqualelembo78/Mevacoin_portable.git /opt/mevacoin
    fi
else
    echo "Clono Mevacoin..."
    git clone https://github.com/pasqualelembo78/Mevacoin_portable.git /opt/mevacoin
fi

echo "Compilazione Mevacoin..."
cd /opt/mevacoin
mkdir -p build && cd build
cmake ..
make -j$(nproc)

echo "Configurazione del servizio mevacoind..."
sudo tee /etc/systemd/system/mevacoind.service > /dev/null <<EOF
[Unit]
Description=Mevacoin Daemon
After=network.target

[Service]
ExecStart=/opt/mevacoin/build/src/mevacoind
WorkingDirectory=/opt/mevacoin/build/src
Restart=always
RestartSec=5
User=root
LimitNOFILE=4096

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl start mevacoind
sudo systemctl enable mevacoind

echo "Imposto permessi corretti..."
sudo chown -R root:root /opt/mevacoin
sudo chmod -R 755 /opt/mevacoin
sudo mkdir -p /opt/mevacoin/logs
sudo chmod -R 775 /opt/mevacoin/logs
sudo chmod +x /opt/mevacoin/build/src/mevacoind

echo "Verifica UFW:"
sudo ufw status verbose

echo "Installazione e configurazione completata con successo."
