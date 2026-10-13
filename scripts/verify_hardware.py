"""Teste opt-in do Mega real; requer ponte --serial --fiware e API ativas.
Envia off/on. --watchdog envia 2000 e verifica reinicializacao real pelo uptime.
Nao edita codigo, colecao ou cadastro. Ao final deixa on e atraso 100 ms.
"""
import argparse
import json
import time
import urllib.request

HEADERS={'fiware-service':'smart','fiware-servicepath':'/'}
ENTITY='urn:ngsi-ld:dist001:001'

def request(url, body=None, headers=None):
    headers=dict(headers or {})
    data=json.dumps(body).encode() if body is not None else None
    if data is not None: headers['Content-Type']='application/json'
    with urllib.request.urlopen(urllib.request.Request(url,data=data,headers=headers),timeout=5) as response:
        raw=response.read();return response.status,json.loads(raw) if raw else None

def entity():
    return request(f'http://localhost:1026/v2/entities/{ENTITY}?type=dist001',headers=HEADERS)[1]

def wait(check, timeout=15):
    until=time.monotonic()+timeout
    while time.monotonic()<until:
        value=check()
        if value: return value
        time.sleep(.2)
    raise AssertionError('Resultado fisico nao confirmado no prazo')

def power(name):
    previous=entity().get(name+'_status',{}).get('metadata',{}).get('TimeInstant',{}).get('value')
    body={'actionType':'update','entities':[{'id':ENTITY,'type':'dist001',name:{'type':'command','value':''}}]}
    code,_=request('http://localhost:4041/v2/op/update',body,HEADERS)
    assert 200 <= code < 300
    def confirmed():
        current=entity();attr=current.get(name+'_status',{})
        if (attr.get('value')=='OK' and
            attr.get('metadata',{}).get('TimeInstant',{}).get('value')!=previous and
            current['state']['value']==('ON' if name=='on' else 'OFF')):
            if name=='off': assert str(current['led']['value'])=='0'
            return current
    wait(confirmed)
    print('PASS: Arduino fisico confirmou '+name+'; Orion atualizou estado',flush=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--watchdog',action='store_true')
    args=parser.parse_args()
    assert request('http://localhost:8001/health')[0]==200
    code, sample=request('http://localhost:8001/telemetria')
    assert code==200 and sample['origem']=='arduino' and not sample['desatualizada']
    data=sample['dados']
    assert data['referencia_cm']==10+data['potenciometro']*90//1023
    current=entity()
    for field in ('potentiometer','reference','sensor_status','communication'):
        assert field in current
    print('PASS: ADC real, referencia, falha do sensor e comunicacao fisica disponiveis',flush=True)
    power('off');power('on')
    if args.watchdog:
        before=request('http://localhost:8001/telemetria')[1]['dados']['uptime_ms']
        assert before>5000, 'Aguarde alguns segundos apos iniciar o firmware'
        assert request('http://localhost:8001/comandos/atraso',{'atraso_ms':2000})[0]==202
        def restarted():
            snapshot=request('http://localhost:8001/telemetria')[1]
            if not snapshot['desatualizada'] and snapshot['dados']['uptime_ms']<before:
                return snapshot
        after=wait(restarted,20)
        assert request('http://localhost:8001/comandos/atraso',{'atraso_ms':100})[0]==202
        time.sleep(2)
        again=request('http://localhost:8001/telemetria')[1]
        assert again['dados']['uptime_ms']>after['dados']['uptime_ms'] and not again['desatualizada']
        print(f"PASS: watchdog reiniciou Mega; uptime {before} -> {after['dados']['uptime_ms']} ms",flush=True)

if __name__=='__main__': main()
