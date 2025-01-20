#!/bin/dash

# Installa zip e unzip se non sono già installati
echo "Installazione di zip e unzip..."
apt-get update -y
apt-get install -y zip unzip

# Crea le directory di destinazione se non esistono
mkdir -p /opt/dipendenze_mevacoin
mkdir -p /opt/mevacoin/build/src

# Definisci i link di download
DIPENDENZE_URL="https://github.com/pasqualelembo78/Mevacoin_portable/releases/download/Mevacoin/dipendenze_mevacoin.zip"
BINARI_URL="https://github.com/pasqualelembo78/Mevacoin_portable/releases/download/Mevacoin/mevacoin.zip"

# Scarica i file ZIP
echo "Scarico dipendenze_mevacoin.zip..."
wget -q $DIPENDENZE_URL -O /tmp/dipendenze_mevacoin.zip

echo "Scarico mevacoin.zip..."
wget -q $BINARI_URL -O /tmp/mevacoin.zip

# Estrai i file ZIP
echo "Estrai dipendenze_mevacoin.zip..."
unzip -o /tmp/dipendenze_mevacoin.zip -d /opt/dipendenze_mevacoin/

echo "Estrai mevacoin.zip..."
unzip -o /tmp/mevacoin.zip -d /opt/mevacoin/build/src/

# Rimuovi i file temporanei
rm /tmp/dipendenze_mevacoin.zip
rm /tmp/mevacoin.zip

echo "Estrazione completata!"
