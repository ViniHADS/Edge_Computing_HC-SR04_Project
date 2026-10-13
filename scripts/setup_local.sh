#!/usr/bin/env sh
# Prepara todas as dependencias no projeto, sem instalar servicos no sistema.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"

# Ambiente Python exclusivo para a API HTTP.
python3 -m venv .venv
.venv/bin/python -m pip install -r api/requirements.txt

# Paho C/C++ e ponte serial da implementacao existente.
sh ESP32/ESP32_mqtt_mock/bootstrap_paho.sh

# Broker local: compila somente o servidor, sem TLS para uso em loopback.
mkdir -p .local/sources .local/bin
curl -fL --retry 2 https://mosquitto.org/files/source/mosquitto-2.0.22.tar.gz \
  -o .local/sources/mosquitto.tar.gz
tar -xzf .local/sources/mosquitto.tar.gz -C .local/sources
make -C .local/sources/mosquitto-2.0.22/src -j4 \
  WITH_TLS=no WITH_CJSON=no WITH_DOCS=no WITH_WEBSOCKETS=no
cp .local/sources/mosquitto-2.0.22/src/mosquitto .local/bin/mosquitto
# Clientes locais usados pela suite MQTT de integracao.
make -C .local/sources/mosquitto-2.0.22/client -j4 static_pub static_sub \
  WITH_SHARED_LIBRARIES=no WITH_STATIC_LIBRARIES=yes \
  WITH_TLS=no WITH_CJSON=no WITH_DOCS=no WITH_WEBSOCKETS=no
printf '\nPronto. Execute: .venv/bin/python scripts/run_local.py --mock\n'
