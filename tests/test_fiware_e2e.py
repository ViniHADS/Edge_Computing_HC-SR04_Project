"""Teste opt-in no FIWARE local ja provisionado; envia off/on ao mock dist001."""
import json
from pathlib import Path
import subprocess
import tempfile
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
HEADERS = {'fiware-service': 'smart', 'fiware-servicepath': '/'}
ENTITY = 'urn:ngsi-ld:dist001:001'


def http(url, body=None, method=None):
    data = json.dumps(body).encode() if body is not None else None
    headers = dict(HEADERS)
    if data is not None:
        headers['Content-Type'] = 'application/json'
    with urllib.request.urlopen(urllib.request.Request(url, data=data, headers=headers, method=method), timeout=5) as response:
        payload = response.read()
        return response.status, json.loads(payload) if payload else None


def entity():
    return http(f'http://127.0.0.1:1026/v2/entities/{ENTITY}?type=dist001')[1]


def wait_for(predicate, timeout=20):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        data = entity()
        if predicate(data):
            return data
        time.sleep(.2)
    raise AssertionError('FIWARE nao apresentou estado esperado: ' + json.dumps(data))


def main():
    # Nao cria/remove cadastros: usa exatamente o dispositivo do usuario.
    http('http://127.0.0.1:4041/iot/devices/dist001')
    baseline = entity().get('TimeInstant', {}).get('value')
    with tempfile.TemporaryFile(mode='w+') as log:
        simulator = subprocess.Popen([str(ROOT / 'ESP32/ESP32_mqtt_mock/build/ESP32Mock'), '--mock', '--fiware',
                                      '--broker', '127.0.0.1', '--port', '1883', '--client-id', 'dist001-fiware-test'],
                                     stdout=log, stderr=log, cwd=ROOT)
        try:
            wait_for(lambda data: data.get('TimeInstant', {}).get('value') != baseline
                     and data.get('state', {}).get('value') == 'ON'
                     and isinstance(data.get('distance', {}).get('value'), (int, float))
                     and data.get('potentiometer', {}).get('value') == 1023)
            print('PASS: MQTT UltraLight atualiza state, led, distance e potentiometer no Orion', flush=True)
            for command, state in [('off', 'OFF'), ('on', 'ON')]:
                body = {'actionType': 'update', 'entities': [
                    {'id': ENTITY, 'type': 'dist001', command: {'type': 'command', 'value': ''}}]}
                previous_ack = entity().get(command + '_status', {}).get('metadata', {}).get('TimeInstant', {}).get('value')
                code, _ = http('http://127.0.0.1:4041/v2/op/update', body)
                assert 200 <= code < 300
                result = wait_for(lambda data: data.get('state', {}).get('value') == state
                                  and data.get(command + '_status', {}).get('value') == 'OK'
                                  and data.get(command + '_status', {}).get('metadata', {}).get('TimeInstant', {}).get('value') != previous_ack)
                if command == 'off':
                    assert str(result['led']['value']) == '0'
                print(f'PASS: POST IoT Agent {command} executado no mock; {command}_status=OK', flush=True)
        except Exception:
            log.flush(); log.seek(0); print(log.read())
            raise
        finally:
            simulator.terminate()
            try:
                simulator.wait(timeout=5)
            except subprocess.TimeoutExpired:
                simulator.kill(); simulator.wait()


if __name__ == '__main__':
    main()
