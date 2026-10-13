"""CP2: UART real via PTY + MQTT real; prova ACK correlacionado e falhas.
Nao usa a placa nem modifica cadastros FIWARE. Execute com .venv/bin/python.
"""
import json
import os
from pathlib import Path
import pty
import queue
import re
import select
import socket
import subprocess
import tempfile
import time
import tty

ROOT = Path(__file__).resolve().parents[1]


def main():
    import paho.mqtt.client as mqtt
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', 0)); port = probe.getsockname()[1]
    master, slave = pty.openpty(); tty.setraw(slave)
    received = queue.Queue()
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id='cp2-observer')
    client.on_message = lambda c, u, msg: received.put((msg.topic, msg.payload.decode()))
    attrs, command, result = '/TEF/dist001/attrs', '/TEF/dist001/cmd', '/ul/TEF/dist001/cmdexe'
    processes = []
    with tempfile.TemporaryDirectory(prefix='cp2-uart-') as directory:
        config = Path(directory) / 'mosquitto.conf'
        config.write_text(f'listener {port} 127.0.0.1\nallow_anonymous true\npersistence false\n')
        with (Path(directory)/'log').open('w+') as log:
            def start(args):
                proc = subprocess.Popen(args, cwd=ROOT, stdout=log, stderr=log)
                processes.append(proc); return proc
            def message(topic, predicate, timeout=8):
                until = time.monotonic() + timeout
                while time.monotonic() < until:
                    try: t, payload = received.get(timeout=min(.2, max(.01, until-time.monotonic())))
                    except queue.Empty: continue
                    if t == topic and predicate(payload): return payload
                raise AssertionError(f'Mensagem ausente em {topic}')
            def no_success(timeout=.3):
                until=time.monotonic()+timeout
                while time.monotonic()<until:
                    try: t,payload=received.get(timeout=.05)
                    except queue.Empty: continue
                    assert not (t==result and payload.endswith('|OK')), payload
            pending = bytearray()
            def serial_line(predicate, timeout=6):
                until=time.monotonic()+timeout
                while time.monotonic()<until:
                    while b'\n' in pending:
                        frame, _, rest = pending.partition(b'\n'); pending[:] = rest
                        text=frame.decode().strip()
                        if predicate(text): return text
                    if select.select([master], [], [], .1)[0]: pending.extend(os.read(master,4096))
                raise AssertionError('Frame serial ausente')
            sample={'distancia_cm':25,'referencia_cm':54,'brilho':255,'status':'ok',
                    'uptime_ms':1000,'potenciometro':501,'habilitado':True,'comunicacao':'online'}
            def send_sample():
                os.write(master,(json.dumps(sample,separators=(',',':'))+'\n').encode())
            try:
                broker=start([str(ROOT/'.local/bin/mosquitto'),'-c',str(config)])
                deadline=time.monotonic()+5
                while True:
                    try: client.connect('127.0.0.1',port); break
                    except OSError:
                        assert time.monotonic()<deadline; time.sleep(.05)
                client.loop_start(); client.subscribe('/TEF/#'); client.subscribe('/ul/#'); time.sleep(.2)
                bridge=start([str(ROOT/'ESP32/ESP32_mqtt_mock/build/ESP32Mock'),'--serial',os.ttyname(slave),
                              '--fiware','--broker','127.0.0.1','--port',str(port)])
                time.sleep(.3); send_sample()
                message(attrs,lambda p:'p|501|r|54|e|ok|c|online' in p)
                serial_line(lambda p:p=='hb|online')
                print('PASS: serial fisica suporta FIWARE, ADC real e heartbeat',flush=True)
                for name,enabled in [('off',False),('on',True)]:
                    send_sample(); time.sleep(.2)
                    client.publish(command,'dist001@'+name+'|').wait_for_publish()
                    frame=serial_line(lambda p:re.fullmatch(name+r'\|[0-9]+',p) is not None)
                    ident=int(frame.split('|')[1]); no_success()
                    os.write(master,(json.dumps({'comando':name,'id':ident+1,'resultado':'OK'},separators=(',',':'))+'\n').encode())
                    no_success()
                    os.write(master,(json.dumps({'comando':name,'id':ident,'resultado':'OK'},separators=(',',':'))+'\n').encode())
                    sample.update(habilitado=enabled,brilho=255 if enabled else 0,uptime_ms=sample['uptime_ms']+100)
                    send_sample()
                    message(result,lambda p:p=='dist001@'+name+'|OK')
                    message(attrs,lambda p:'s|'+('ON' if enabled else 'OFF')+'|l|'+str(sample['brilho']) in p)
                print('PASS: on/off so confirmado apos ACK do Arduino; ID incorreto ignorado',flush=True)
                send_sample(); time.sleep(.2); client.publish(command,'dist001@off|').wait_for_publish()
                serial_line(lambda p:p.startswith('off|'))
                message(result,lambda p:p=='dist001@off|ERROR',timeout=7)
                print('PASS: ausencia de ACK gera ERROR, sem confirmacao falsa',flush=True)
                # Firmware sem transmitir: a UART precisa ficar quieta para nao
                # manter o bootloader ocupado durante recuperacao por watchdog.
                time.sleep(.4)
                pending.clear()
                while select.select([master],[],[],0)[0]: os.read(master,4096)
                until=time.monotonic()+1.8
                while time.monotonic()<until:
                    assert not select.select([master],[],[],.1)[0], 'Heartbeat enviado sem telemetria recente'
                send_sample()
                serial_line(lambda p:p=='hb|online')
                print('PASS: UART silenciosa sem telemetria; heartbeat retoma quando firmware volta',flush=True)
                send_sample(); time.sleep(.3)
                broker.terminate(); broker.wait(timeout=5)
                serial_line(lambda p:p=='hb|offline',timeout=6)
                print('PASS: perda MQTT informada ao Arduino por heartbeat',flush=True)
                start([str(ROOT/'.local/bin/mosquitto'),'-c',str(config)])
                time.sleep(1); client.reconnect()
                send_sample() # A placa continua transmitindo mesmo com MQTT indisponivel.
                serial_line(lambda p:p=='hb|online',timeout=8)
                client.subscribe('/TEF/#'); time.sleep(.2); send_sample()
                message(attrs,lambda p:'p|501' in p)
                os.close(master); master=None
                bridge.wait(timeout=5)
                assert bridge.returncode != 0
                message(attrs,lambda p:p=='c|usb_desconectada')
                print('PASS: reconexao MQTT e desconexao USB detectadas',flush=True)
            finally:
                client.disconnect(); client.loop_stop()
                for proc in reversed(processes):
                    if proc.poll() is None:
                        proc.terminate()
                        try: proc.wait(timeout=6)
                        except subprocess.TimeoutExpired: proc.kill();proc.wait()
                if master is not None: os.close(master)
                os.close(slave)
                log.seek(0)
                if any(p.returncode not in (0,-15,1) for p in processes): print(log.read())

if __name__=='__main__': main()
