"""Inclui atributos CP2 no cadastro existente; sem exclusao ou reprovisionamento.
Sem --apply apenas mostra o corpo planejado. Usa smart / e localhost:4041.
"""
import argparse
import json
import urllib.request

HEADERS = {'fiware-service':'smart','fiware-servicepath':'/'}
EXTRA = [
    {'object_id':'r','name':'reference','type':'float'},
    {'object_id':'e','name':'sensor_status','type':'Text'},
    {'object_id':'c','name':'communication','type':'Text'},
]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply',action='store_true')
    args=parser.parse_args()
    url='http://localhost:4041/iot/devices/dist001'
    with urllib.request.urlopen(urllib.request.Request(url,headers=HEADERS),timeout=5) as response:
        device=json.load(response)
    attributes=device['attributes'][:]
    for field in EXTRA:
        existing=next((a for a in attributes if a['name']==field['name'] or a.get('object_id')==field['object_id']),None)
        if existing and existing != field:
            raise RuntimeError('Atributo existente conflitante: '+repr(existing))
        if not existing: attributes.append(field)
    body={'attributes':attributes}
    print(json.dumps(body,indent=2,ensure_ascii=False))
    if not args.apply:
        print('Somente leitura. Para aplicar: .venv/bin/python scripts/configure_fiware.py --apply')
        return
    if attributes==device['attributes']:
        print('Cadastro ja atualizado; nenhuma escrita necessaria.');return
    headers=dict(HEADERS, **{'Content-Type':'application/json'})
    request=urllib.request.Request(url,data=json.dumps(body).encode(),headers=headers,method='PUT')
    with urllib.request.urlopen(request,timeout=5) as response:
        print('Atualizacao HTTP',response.status)
    with urllib.request.urlopen(urllib.request.Request(url,headers=HEADERS),timeout=5) as response:
        updated=json.load(response)
    assert updated['attributes']==attributes
    assert updated['commands']==device['commands'], 'Comandos foram alterados'
    print('PASS: atributos CP2 incluidos; comandos existentes preservados.')

if __name__=='__main__': main()
