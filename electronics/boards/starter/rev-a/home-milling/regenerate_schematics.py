#!/usr/bin/env python3
"""Derive the home schematic cache from the canonical adjacent rev-a schematic.
Only Footprint properties change. Uses Python's standard library; no KiCad API required.
"""
from pathlib import Path
import argparse,json,re,shutil

def parse(text):
    tokens=re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+',text)
    def read(i):
        if tokens[i]!='(':return tokens[i],i+1
        row=[];i+=1
        while tokens[i]!=')':
            item,i=read(i);row.append(item)
        return row,i+1
    result,end=read(0)
    if end!=len(tokens):raise ValueError('Unexpected trailing schematic tokens')
    return result

def raw(token):
    return json.loads(token) if token.startswith('"') else token

def render(node,depth=0):
    if not isinstance(node,list):return node
    flat='('+ ' '.join(render(q,depth+1)for q in node)+')'
    if '\n'not in flat and len(flat)<=120:return flat
    return '('+render(node[0],depth+1)+''.join('\n'+'  '*(depth+1)+render(q,depth+1)for q in node[1:])+')'

def derive(source,destination,mapping):
    tree=parse(source.read_text());count=0
    for symbol in tree:
        if not isinstance(symbol,list)or not symbol or symbol[0]!='symbol':continue
        properties={raw(p[1]):p for p in symbol if isinstance(p,list)and p and p[0]=='property'}
        reference=raw(properties['Reference'][2]) if 'Reference'in properties else None
        if reference in mapping and 'Footprint'in properties:
            properties['Footprint'][2]=json.dumps(mapping[reference]);count+=1
        elif 'Footprint'in properties and raw(properties['Footprint'][2]):
            raise ValueError(f'No home footprint mapping for canonical {reference}')
    destination.write_text(render(tree)+'\n');return count

if __name__=='__main__':
    home=Path(__file__).resolve().parent
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--canonical',type=Path,default=home.parent,help='Canonical rev-a directory (default: parent directory)')
    args=parser.parse_args();canonical=args.canonical.resolve()
    mapping=json.loads((home/'source-footprints.json').read_text())
    for name in ('starter.kicad_sch','inputs.kicad_sch'):
        print(f'{name}: {derive(canonical/name,home/name,mapping)} Footprint properties mapped')
    # Use the canonical adjacent symbol libraries; do not duplicate shared caches.
    table=(canonical/'sym-lib-table').read_text()
    (home/'sym-lib-table').write_text(table.replace('${KIPRJMOD}/','${KIPRJMOD}/../'))
