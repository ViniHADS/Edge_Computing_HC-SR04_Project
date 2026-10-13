"""Prepara FIWARE novo em localhost sem excluir cadastros existentes.
Sem --apply apenas verifica. Requer IoT Agent em 4041 e Orion interno orion:1026.
"""
import argparse
import json
import urllib.error
import urllib.request
from configure_fiware import EXTRA

HEADERS={'fiware-service':'smart','fiware-servicepath':'/'}
BASE='http://localhost:4041'

def request(path, body=None):
    headers=dict(HEADERS);data=None
    if body is not None:
        data=json.dumps(body).encode();headers['Content-Type']='application/json'
    try:
        with urllib.request.urlopen(urllib.request.Request(BASE+path,data=data,headers=headers),timeout=5) as response:
            raw=response.read();return response.status,json.loads(raw) if raw else None
    except urllib.error.HTTPError as error:
        if error.code==404: return 404,None
        raise

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--apply',action='store_true');args=p.parse_args()
    _,groups=request('/iot/services')
    exists=any(group.get('apikey')=='TEF' for group in groups.get('services',[]))
    if not exists:
        print('Grupo TEF ausente.')
        if args.apply:
            request('/iot/services',{'services':[{'apikey':'TEF','cbroker':'http://orion:1026','entity_type':'Thing','resource':''}]})
    code,device=request('/iot/devices/dist001')
    if code==404:
        print('Dispositivo dist001 ausente.')
        body={'device_id':'dist001','entity_name':'urn:ngsi-ld:dist001:001','entity_type':'dist001',
              'protocol':'PDI-IoTA-UltraLight','transport':'MQTT',
              'commands':[{'name':name,'type':'command'} for name in ('on','off')],
              'attributes':[{'object_id':'s','name':'state','type':'Text'},
                            {'object_id':'l','name':'led','type':'Text'},
                            {'object_id':'d','name':'distance','type':'float'},
                            {'object_id':'p','name':'potentiometer','type':'float'}]+EXTRA}
        if args.apply: request('/iot/devices',{'devices':[body]})
    else:
        if device['entity_name']!='urn:ngsi-ld:dist001:001' or device['transport']!='MQTT':
            raise RuntimeError('Cadastro existente diferente; revisar antes de aplicar.')
        print('Dispositivo existente preservado. Para os atributos adicionais use configure_fiware.py --apply.')
    print('Preparacao concluida.' if args.apply else 'Somente leitura; use --apply para criar apenas os cadastros ausentes.')

if __name__=='__main__': main()
