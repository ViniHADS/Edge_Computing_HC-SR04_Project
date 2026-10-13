#!/usr/bin/env sh
# Compila/grava o sketch completo. Windows: executar dentro do WSL2.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"
arduino_tool=${ARDUINO_CLI:-}
if [ -z "$arduino_tool" ]; then
  arduino_tool=$(command -v arduino-cli || true)
fi
if [ -z "$arduino_tool" ] && [ -x '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli' ]; then
  arduino_tool='/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
fi
if [ -z "$arduino_tool" ]; then
  printf '%s\n' 'Instale Arduino CLI oficial ou defina ARDUINO_CLI com o caminho do executavel.' >&2
  exit 1
fi
case ${1:-compile} in
  compile) "$arduino_tool" compile --fqbn arduino:avr:mega Arduino ;;
  upload)
    if [ -z "${2:-}" ]; then printf '%s\n' 'Use: sh scripts/arduino.sh upload /dev/cu.usbserial-120' >&2; exit 1; fi
    "$arduino_tool" upload --port "$2" --fqbn arduino:avr:mega Arduino ;;
  *) printf '%s\n' 'Use compile ou upload PORTA.' >&2; exit 1 ;;
esac
