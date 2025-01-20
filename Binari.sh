#!/bin/dash

# 1. Installazione di zip, unzip, ufw e altri pacchetti necessari
echo "Installazione di pacchetti necessari..."
apt-get update -y
apt-get install -y zip unzip wget ufw

# 2. Configurazione del firewall
echo "Configurazione del firewall (UFW)..."

sudo ufw enable
sudo ufw allow 18080/tcp
sudo ufw allow 18081/tcp
sudo ufw allow 4000/tcp
sudo ufw allow 22/tcp
# 3. Creazione delle directory di destinazione
mkdir -p /opt/dipendenze_mevacoin
mkdir -p /opt/mevacoin/build/src

# 4. Definizione dei link di download
DIPENDENZE_URL="https://github.com/pasqualelembo78/Mevacoin_portable/releases/download/Mevacoin/dipendenze_mevacoin.zip"
BINARI_URL="https://github.com/pasqualelembo78/Mevacoin_portable/releases/download/Mevacoin/mevacoin.zip"

# 5. Download dei file ZIP
echo "Scarico dipendenze_mevacoin.zip..."
wget -q $DIPENDENZE_URL -O /tmp/dipendenze_mevacoin.zip

echo "Scarico mevacoin.zip..."
wget -q $BINARI_URL -O /tmp/mevacoin.zip

# 6. Estrazione dei file ZIP
echo "Estrai dipendenze_mevacoin.zip..."
unzip -o /tmp/dipendenze_mevacoin.zip -d /opt/

echo "Estrai mevacoin.zip..."
unzip -o /tmp/mevacoin.zip -d /opt/mevacoin/build/src/

# 7. Rimuovi i file temporanei
rm /tmp/dipendenze_mevacoin.zip
rm /tmp/mevacoin.zip

# 8. Creazione del servizio systemd per mevacoind
echo "Configurazione del servizio systemd per mevacoind..."
cat <<EOF > /etc/systemd/system/mevacoind.service
[Unit]
Description=Mevacoin Daemon
After=network.target

[Service]
ExecStart=/opt/mevacoin/build/src/mevacoind
WorkingDirectory=/opt/mevacoin/build/src
Restart=always
User=root
Group=root

[Install]
WantedBy=multi-user.target
EOF

# 9. Ricarica systemd e abilita il servizio
systemctl daemon-reload
systemctl enable mevacoind.service
systemctl start mevacoind.service

echo "Installazione completata! Il daemon mevacoind è in esecuzione."
