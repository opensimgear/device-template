"""Independent read-only home-board geometry, face isolation and physical-ground audit.

Writes only audit JSON. Run with KiCad Python and optional PCB/source/output paths.
No source PCB, schematic, footprint, project, zone fill, or saved board is edited.
"""
import pcbnew as p, os, sys, json, math, hashlib, re, xml.etree.ElementTree as ET
from pathlib import Path
root=Path(__file__).resolve().parent
default_board=root.parent/'starter.kicad_pcb' if root.name=='review' else root.parent/'home/starter.kicad_pcb'
default_canonical=root.parent.parent if root.name=='review' else root.parent/'production'
if not (default_canonical/'netlist.xml').is_file() and (default_canonical/'production/netlist.xml').is_file():default_canonical=default_canonical/'production'
path=Path(sys.argv[1]) if len(sys.argv)>1 else default_board
canonical=Path(sys.argv[2]) if len(sys.argv)>2 else default_canonical
output=Path(sys.argv[3]) if len(sys.argv)>3 else root/'home-construction-review.json'
hash0=hashlib.sha256(path.read_bytes()).hexdigest();mtime0=path.stat().st_mtime
b=p.LoadBoard(str(path)); home=path.parent
xml=ET.parse(canonical/'netlist.xml').getroot()
librefs={c.attrib['ref']:c.findtext('footprint') for c in xml.find('components')}
netcontract={(n.attrib['ref'],n.attrib['pin']):net.attrib['name'] for net in xml.find('nets') for n in net}
libfps={}
for ref,name in librefs.items():
 lib,item=name.split(':',1);libfps[ref]=p.FootprintLoad(str(canonical/(lib+'.pretty')),item)
def xy(v):return [p.ToMM(v.x),p.ToMM(v.y)]
def rec(f):
 return sorted([(d.GetNumber(),xy(d.GetFPRelativePosition()),xy(d.GetSize()),xy(d.GetDrillSize()),d.GetShape(),d.GetDrillShape(),d.GetAttribute(),round(d.GetRoundRectRadiusRatio(),6)) for d in f.Pads()],key=str)
mismatch=[f.GetReference() for f in b.GetFootprints() if f.GetReference() in libfps and rec(f)!=rec(libfps[f.GetReference()])]
missingrefs=sorted(set(librefs)-{f.GetReference()for f in b.GetFootprints()})
netmismatch=[{'ref':f.GetReference(),'pad':d.GetNumber(),'actual_net':d.GetNetname(),'expected_net':netcontract[(f.GetReference(),d.GetNumber())]}for f in b.GetFootprints()for d in f.Pads()if (f.GetReference(),d.GetNumber())in netcontract and d.GetNetname()!=netcontract[(f.GetReference(),d.GetNumber())]]
# Inspect libraries separately, not only board's embedded footprints.
homeids=json.loads((home/'source-footprints.json').read_text())
libmismatch=[]
for ref,homeid in homeids.items():
 lib,item=homeid.split(':',1);h=p.FootprintLoad(str(home/(lib+'.pretty')),item)
 if rec(h)!=rec(libfps[ref]):libmismatch.append(ref)
vs=[t for t in b.GetTracks() if isinstance(t,p.PCB_VIA)]
ts=[t for t in b.GetTracks() if not isinstance(t,p.PCB_VIA)]
bodymin=(1e10,None);bad_access=[]
for v in vs:
 x,y=xy(v.GetPosition());rr=p.ToMM(v.GetWidth(p.F_Cu))/2
 for f in b.GetFootprints():
  bb=f.GetBoundingBox(False,False);x0,y0=xy(bb.GetPosition());x1,y1=xy(bb.GetEnd())
  d=math.hypot(max(x0-x,0,x-x1),max(y0-y,0,y-y1))-rr
  if d<bodymin[0]:bodymin=(d,{'via':[x,y],'ref':f.GetReference()})
  if d<.199:bad_access.append({'via':[x,y],'ref':f.GetReference(),'land_to_bound_mm':d})
fills=[z.GetFilledPolysList(p.F_Cu) for z in b.Zones() if not z.GetIsRuleArea() and z.IsOnLayer(p.F_Cu)]
backfills=[z.GetFilledPolysList(p.B_Cu) for z in b.Zones() if not z.GetIsRuleArea() and z.IsOnLayer(p.B_Cu)]
front=[t for t in ts if t.GetLayer()==p.F_Cu];back=[t for t in ts if t.GetLayer()==p.B_Cu]
zh=[];th=[];nlong=0;shells=[];shellback=[];moatmissing=[];moats=[]
for f in b.GetFootprints():
 for i,d in enumerate(f.Pads()):
  if d.GetAttribute()!=p.PAD_ATTRIB_PTH:continue
  is_shell=f.GetReference()=='J1' and d.GetNumber()=='SH';layer=p.B_Cu if is_shell else p.F_Cu
  s=d.GetEffectiveShape(layer);pos=xy(d.GetPosition())
  if is_shell:
   shells.append(pos)
   if any(z.Collide(s,199000) for z in backfills) or any(s.Collide(t.GetEffectiveShape(p.B_Cu),199000) for t in back):shellback.append(pos)
   prefix=f'Isolated back USB shell annulus instance {i}'
  else:
   nlong+=1
   if any(z.Collide(s,199000) for z in fills):zh.append([f.GetReference(),d.GetNumber(),pos])
   for t in front:
    if s.Collide(t.GetEffectiveShape(p.F_Cu),199000):th.append([f.GetReference(),d.GetNumber(),pos,t.GetNetname()])
   prefix=f'Bottom-only unplated physical pin {f.GetReference()}.{d.GetNumber()} instance {i}'
  # Verify a separate >=0.199 mm bounding moat at every physical pad instance.
  bb=d.GetBoundingBox();lo,hi=xy(bb.GetPosition()),xy(bb.GetEnd());corners=[(lo[0]-.199,lo[1]-.199),(hi[0]+.199,lo[1]-.199),(hi[0]+.199,hi[1]+.199),(lo[0]-.199,hi[1]+.199)]
  own=[z for z in b.Zones() if z.GetIsRuleArea() and z.GetZoneName()==prefix and z.IsOnLayer(layer)]
  good=any(z.GetDoNotAllowTracks() and z.GetDoNotAllowVias() and z.GetDoNotAllowZoneFills() and all(z.Outline().Contains(p.VECTOR2I(p.FromMM(x),p.FromMM(y))) for x,y in corners) for z in own)
  if not good:moatmissing.append({'ref':f.GetReference(),'pad':d.GetNumber(),'instance':i,'position_mm':pos,'expected_zone':prefix})
  else:moats.append(prefix)
# Only hand-wired vias bridge faces. Component holes use their soldered face only.
items=[];pads=[]
for z in b.Zones():
 if z.GetIsRuleArea() or not z.GetNetname():continue
 for l in (p.F_Cu,p.B_Cu):
  if not z.IsOnLayer(l):continue
  pl=z.GetFilledPolysList(l)
  for i in range(pl.OutlineCount()):
   s=p.SHAPE_POLY_SET();s.AddOutline(pl.COutline(i))
   for j in range(pl.HoleCount(i)):s.AddHole(pl.CHole(i,j),0)
   items.append({'label':'zone:'+b.GetLayerName(l)+':'+str(i),'net':z.GetNetname(),'layers':{l},'shape':s,'wire':False})
for t in b.GetTracks():
 if not t.GetNetname():continue
 v=isinstance(t,p.PCB_VIA);layers={p.F_Cu,p.B_Cu} if v else {t.GetLayer()}
 items.append({'label':('wire:'+str(xy(t.GetPosition()))) if v else 'track:'+str(xy(t.GetStart())),'net':t.GetNetname(),'layers':layers,'shape':t.GetEffectiveShape(p.F_Cu if v else t.GetLayer()),'wire':v})
for f in b.GetFootprints():
 for d in f.Pads():
  if not d.GetNetname():continue
  if d.GetAttribute()==p.PAD_ATTRIB_PTH:l=p.F_Cu if f.GetReference()=='J1' and d.GetNumber()=='SH' else p.B_Cu
  # PAD.GetLayer() is F.Cu even for flipped SMD pads in this KiCad binding;
  # the physical copper face is given by the actual layer set.
  elif d.GetAttribute()==p.PAD_ATTRIB_SMD:l=p.B_Cu if d.IsOnLayer(p.B_Cu) else p.F_Cu
  else:continue
  pads.append((len(items),f.GetReference(),d.GetNumber(),l))
  items.append({'label':f.GetReference()+'.'+d.GetNumber(),'net':d.GetNetname(),'layers':{l},'shape':d.GetEffectiveShape(l),'wire':False})
parents=list(range(len(items)))
def find(i):
 while parents[i]!=i:parents[i]=parents[parents[i]];i=parents[i]
 return i
def join(i,j):parents[find(i)]=find(j)
for i,x in enumerate(items):
 for j in range(i):
  y=items[j]
  if x['net']==y['net'] and x['layers']&y['layers'] and x['shape'].Collide(y['shape'],0):join(i,j)
# Duplicate pad numbers are one physical shared component contact (Omron paired leads).
for i,r,n,l in pads:
 for j,r2,n2,l2 in pads:
  if i<j and r==r2 and n==n2:join(i,j)
padgroups={}
for i,r,n,l in pads:padgroups.setdefault(items[i]['net'],{}).setdefault(find(i),[]).append(r+'.'+n)
disconnected={net:list(groups.values()) for net,groups in padgroups.items() if len(groups)>1}
groups=set(padgroups.get('GND',{}));sh=[i for i,r,n,_ in pads if r=='J1' and n=='SH'];wires=[i for i,x in enumerate(items) if x['wire']]
edge=[]
for d in b.GetDrawings():
 if d.GetLayer()==p.Edge_Cuts:edge.extend([xy(d.GetStart()),xy(d.GetEnd())])
fps={f.GetReference():f for f in b.GetFootprints()}
nose={r:round(xy(fps[r].GetPosition())[0]+(3.675 if r=='J1' else 14.25),6) for r in ['J1','J2','J3']}
boardtext=path.read_text()
stacktext=boardtext.split('(stackup',1)[1].split('(copper_finish',1)[0]
stackrows=[{'name':n,'type':typ,'thickness_mm':float(thick) if thick else None} for n,typ,thick in re.findall(r'\(layer \"([^\"]+)\"\s+\(type \"([^\"]+)\"\)(?:\s+\(thickness ([^)]+)\))?',stacktext)]
physical=[x for x in stackrows if x['type'] in ['copper','core','prepreg','dielectric']]
stackokay=len(physical)==3 and [x['type'] for x in physical]==['copper','core','copper'] and [x['name'] for x in physical]==['F.Cu','dielectric 1','B.Cu'] and all(abs(a['thickness_mm']-z)<1e-6 for a,z in zip(physical,[.035,1.53,.035])) and not any(x['type'].endswith('Solder Mask') and (x['thickness_mm'] or 0)>0 for x in stackrows)
smd_back=[f.GetReference() for f in b.GetFootprints() if any(d.GetAttribute()==p.PAD_ATTRIB_SMD and d.IsOnLayer(p.B_Cu) for d in f.Pads())]
result={
 'scope':'Independent read-only nominal construction review using physical copper faces, manual wire joins and the canonical netlist; no source modifications or hardware qualification.',
 'board_path':str(path.resolve()),'canonical_directory':str(canonical.resolve()),'netlist_sha256':hashlib.sha256((canonical/'netlist.xml').read_bytes()).hexdigest(),'source_footprints_sha256':hashlib.sha256((home/'source-footprints.json').read_bytes()).hexdigest(),
 'board_sha256':hash0,'board_unchanged_during_audit':hash0==hashlib.sha256(path.read_bytes()).hexdigest(),'mtime':mtime0,
 'copper_layer_count':b.GetCopperLayerCount(),'board_thickness_mm':p.ToMM(b.GetDesignSettings().GetBoardThickness()),'stackup_layers':stackrows,'bare_two_layer_nominal_stackup_matches':stackokay,'back_face_smd_refs':smd_back,
 'tracks':len(ts),'vias':len(vs),'via_sizes_mm':sorted(set((p.ToMM(v.GetWidth(p.F_Cu)),p.ToMM(v.GetDrill())) for v in vs)),
 'minimum_track_mm':min(p.ToMM(t.GetWidth()) for t in ts),'minimum_via_land_to_component_bound_mm':round(bodymin[0],6),'nearest':bodymin[1],'via_access_below_0_199mm':bad_access,
 'missing_canonical_component_refs':missingrefs,'embedded_pad_net_contract_mismatches':netmismatch,'production_embedded_pad_geometry_mismatches':mismatch,'production_library_pad_geometry_mismatches':libmismatch,'long_lead_pad_count':nlong,'short_usb_shell_pads':len(shells),
 'physical_pad_moats_passed':len(moats),'physical_pad_moats_missing_or_too_small':moatmissing,
 'front_filled_copper_clearance_below_0_199mm':zh,'front_track_clearance_below_0_199mm':th,'usb_shell_back_copper_clearance_below_0_199mm':shellback,
 'physical_connectivity_graph_nodes':len(items),'physical_connectivity_net_count':len(padgroups),'physical_connectivity_disconnected_nets':disconnected,'physical_ground_pad_groups':len(groups),
 'usb_shell_ground_reaches_manual_wire':all(any(find(i)==find(j) for j in wires) for i in sh),
 'edge_bounds_mm':[min(x[0] for x in edge),min(x[1] for x in edge),max(x[0] for x in edge),max(x[1] for x in edge)],
 'connector_noses_x_mm':nose}
checks=[];issues=[]
def ck(ok,label): (checks if ok else issues).append(label)
ck(result['board_unchanged_during_audit'],'Source unchanged throughout independent audit')
ck(b.GetCopperLayerCount()==2,'Exactly two copper faces')
ck(abs(result['board_thickness_mm']-1.6)<.000001,'Nominal1.60mm laminate')
ck(stackokay,'Bare35um/1.53mm/35um two-face stackup')
ck(not missingrefs,'All111 canonical component references present')
ck(not netmismatch,'Every embedded contact matches canonical netlist contract')
ck(not mismatch,'Every embedded manufacturer land/drill/pin geometry preserved')
ck(not libmismatch,'Every HomeMilling library manufacturer land/drill/pin geometry preserved')
ck(not bad_access,'Every manual wire land accessible outside both-face component body bounds')
ck(result['minimum_track_mm']>=.199,'At least.20mm nominal track width (1um audit tolerance)')
ck(result['via_sizes_mm']==[(1.8,.8)],'All manual wire joins1.80mm lands/.80mm drills')
ck(not moatmissing,'Individual physical long-lead front moats and USB-shell back moats meet.20mm nominal target')
ck(not zh and not th,'No front track/fill reaches any bottom-only long-lead annulus')
ck(len(shells)==4 and not shellback,'Exactly four front-only USB shell contacts; isolated back annuli')
ck(len(groups)==1,'All physical ground contacts form one connected group using only documented solder faces and manual wire joins')
ck(result['usb_shell_ground_reaches_manual_wire'],'All four USB shell contacts reach a manual ground wire join')
ck(not disconnected,'All nets physically connected without imaginary plated component-hole barrels')
ck(all(abs(x-185)<.000001 for x in nose.values()),'USB and both4P4C mouths align with nominal east edge185mm')
result['check_count']=len(checks)+len(issues);result['checks']=checks;result['issues']=issues
output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));sys.stdout.flush();os._exit(0)
