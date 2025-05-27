#!/bin/bash

# Abilita modalità di uscita immediata in caso di errore
set -e

# Configura il firewall
echo "Configurazione del firewall con UFW..."

sudo ufw enable
sudo ufw allow 17080/tcp
sudo ufw allow 17081/tcp
sudo ufw allow 17082/tcp
sudo ufw allow 3333/tcp
sudo ufw allow 4000/tcp
sudo ufw allow 22/tcp
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
# Definisci la cartella delle dipendenze
DEPENDENCIES_DIR="/opt/dipendenze_mevacoin"

# Crea la cartella per le dipendenze, se non esiste
sudo mkdir -p "${DEPENDENCIES_DIR}"

# Aggiorna il sistema
echo "Aggiornamento del sistema..."
sudo apt update && sudo apt upgrade -y

# Installa i pacchetti di base necessari per il compilatore
echo "Installazione dei pacchetti di base..."
sudo apt install -y build-essential cmake git ccache clang g++ python3 python3-pip libssl-dev libzmq3-dev libsodium-dev

# Installa le librerie richieste per Boost e altre dipendenze
echo "Installazione delle librerie richieste..."
sudo apt install -y libbz2-dev zlib1g-dev liblzma-dev

# Crea una funzione per compilare una libreria statica in una directory personalizzata
compile_static_lib() {
    LIBRARY_NAME=$1
    LIBRARY_URL=$2
    LIBRARY_DIR="${DEPENDENCIES_DIR}/${LIBRARY_NAME}"

    # Scarica e decomprimi la libreria
    if [ ! -d "${LIBRARY_DIR}" ]; then
        echo "Scaricamento e decompressione di ${LIBRARY_NAME}..."
        wget -q "${LIBRARY_URL}" -O "${LIBRARY_NAME}.tar.bz2"
        tar -xjf "${LIBRARY_NAME}.tar.bz2" -C "${DEPENDENCIES_DIR}"
    fi

    # Configura e compila la libreria in modalità statica
    echo "Compilazione e installazione di ${LIBRARY_NAME}..."
    cd "${LIBRARY_DIR}"
    ./configure --prefix="${DEPENDENCIES_DIR}" --enable-static --disable-shared
    make -j$(nproc)
    sudo make install
}

# Installa Boost 1.55 in modo statico
BOOST_VERSION="1_55_0"
BOOST_DIR="${DEPENDENCIES_DIR}/boost_${BOOST_VERSION}"
BOOST_ARCHIVE="boost_${BOOST_VERSION}.tar.bz2"
BOOST_URL="http://sourceforge.net/projects/boost/files/boost/1.55.0/${BOOST_ARCHIVE}"

if [ ! -d "${BOOST_DIR}" ]; then
    echo "Scaricamento di Boost ${BOOST_VERSION}..."
    wget -q "${BOOST_URL}" -O "${BOOST_ARCHIVE}"

    echo "Estrazione di Boost ${BOOST_VERSION}..."
    tar --bzip2 -xf "${BOOST_ARCHIVE}"
    cd "boost_${BOOST_VERSION}"

    echo "Compilazione e installazione di Boost ${BOOST_VERSION} in ${BOOST_DIR}..."
    ./bootstrap.sh --prefix="${DEPENDENCIES_DIR}" --with-libraries=all
    sudo ./b2 install
    cd ..
else
    echo "Boost ${BOOST_VERSION} è già installato in ${BOOST_DIR}"
fi

# Clona o aggiorna il repository Mevacoin
if [ -d "/opt/mevacoin" ]; then
    if [ -d "/opt/mevacoin/.git" ]; then
        echo "Aggiornamento del repository Mevacoin..."
        cd /opt/mevacoin
        git reset --hard
        git pull origin main
    else
        echo "La directory /opt/mevacoin non è un repository Git. Eliminazione e nuova clonazione..."
        rm -rf /opt/mevacoin
        git clone https://github.com/pasqualelembo78/Mevacoin_portable.git /opt/mevacoin
    fi
else
    echo "Clonazione del repository Mevacoin..."
    git clone https://github.com/pasqualelembo78/Mevacoin_portable.git /opt/mevacoin
fi

# Configura il progetto
echo "Creazione della directory di build e configurazione del progetto..."
cd /opt/mevacoin
mkdir -p build
cd build
cmake -DBOOST_ROOT=/opt/dipendenze_mevacoin -DBOOST_INCLUDEDIR=/opt/dipendenze_mevacoin/include -DBOOST_LIBRARYDIR=/opt/dipendenze_mevacoin/lib -DBoost_NO_SYSTEM_PATHS=ON ..

# Compila il progetto
echo "Avvio della compilazione del progetto Mevacoin..."
make -j$(nproc)

# Configura mevacoind come servizio systemd
echo "Configurazione del demone Mevacoin come servizio systemd..."

SERVICE_FILE="/etc/systemd/system/mevacoind.service"

sudo bash -c "cat > ${SERVICE_FILE}" <<EOF
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

# Ricarica systemd
echo "Ricaricamento di systemd..."
sudo systemctl daemon-reload

# Avvia e abilita il servizio
echo "Avvio del servizio Mevacoin..."
sudo systemctl start mevacoind

echo "Abilitazione del servizio Mevacoin per l'avvio automatico..."
sudo systemctl enable mevacoind

# Configura i permessi per cartelle e file
echo "Configurazione dei permessi per il progetto Mevacoin..."

PROJECT_DIR="/opt/mevacoin"
LOG_DIR="/opt/mevacoin/logs"
MEVACOIND_BIN="${PROJECT_DIR}/build/src/mevacoind"

sudo chown -R root:root "${PROJECT_DIR}"
sudo chmod -R 755 "${PROJECT_DIR}"

sudo mkdir -p "${LOG_DIR}"
sudo chown -R root:root "${LOG_DIR}"
sudo chmod -R 775 "${LOG_DIR}"

sudo chmod +x "${MEVACOIND_BIN}"




echo "Verifica del firewall..."
sudo ufw status

# Verifica permessi
echo "Verifica dei permessi sulle cartelle e sui file..."
ls -ld "${PROJECT_DIR}"
ls -l "${MEVACOIND_BIN}"

echo "Configurazione completata! Riavvia il server per testare l'avvio automatico del demone Mevacoin."
