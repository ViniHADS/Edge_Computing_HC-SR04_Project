#!/usr/bin/env sh
# Compila dependências locais sem instalar pacotes no sistema.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
deps_dir="$project_dir/.deps"
prefix="$deps_dir/install"
# Baixa versoes fixadas para manter o build reproduzivel.
mkdir -p "$deps_dir"
curl -fL --retry 2 https://github.com/eclipse-paho/paho.mqtt.c/archive/refs/tags/v1.3.15.tar.gz -o "$deps_dir/paho-c.tar.gz"
curl -fL --retry 2 https://github.com/eclipse-paho/paho.mqtt.cpp/archive/refs/tags/v1.5.3.tar.gz -o "$deps_dir/paho-cpp.tar.gz"
# Extrai as fontes dentro da pasta local ignorada pelo Git.
tar -xzf "$deps_dir/paho-c.tar.gz" -C "$deps_dir"
tar -xzf "$deps_dir/paho-cpp.tar.gz" -C "$deps_dir"
# Primeiro compila a biblioteca C, usada pelo adaptador C++.
cmake -S "$deps_dir/paho.mqtt.c-1.3.15" -B "$deps_dir/build-c-cp2" \
  -DCMAKE_INSTALL_PREFIX="$prefix" -DPAHO_WITH_SSL=OFF \
  -DPAHO_BUILD_SAMPLES=OFF -DPAHO_ENABLE_TESTING=OFF
cmake --build "$deps_dir/build-c-cp2" --parallel 4
cmake --install "$deps_dir/build-c-cp2"
# Em seguida compila C++ usando o mesmo prefixo local.
cmake -S "$deps_dir/paho.mqtt.cpp-1.5.3" -B "$deps_dir/build-cpp-cp2" \
  -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_PREFIX_PATH="$prefix" \
  -DPAHO_WITH_SSL=OFF -DPAHO_BUILD_SAMPLES=OFF -DPAHO_BUILD_TESTS=OFF
cmake --build "$deps_dir/build-cpp-cp2" --parallel 4
cmake --install "$deps_dir/build-cpp-cp2"
# Por fim gera a ponte da aplicacao vinculada as bibliotecas locais.
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$project_dir/build" --parallel 4
