"""Verify unplated soldered-face connectivity with KiCad pcbnew and kicad-cli.

The original PCB is unchanged. Writes a non-fabrication model under .audit/ and
review reports/CSV. Requires the KiCad Python environment, not plain Python.
"""
from pathlib import Path
import pcbnew as p,csv,os,math,json,subprocess,shutil,re,hashlib
out=Path(__file__).resolve().parent;b=p.LoadBoard(str(out/'starter.kicad_pcb'))
def mm(q):return p.ToMM(q.x),p.ToMM(q.y)
rows=[]
for t in b.GetTracks():
 if isinstance(t,p.PCB_VIA):
  x,y=mm(t.GetPosition());rows.append((x,y,t.GetNetname(),p.ToMM(t.GetDrillValue()),p.ToMM(t.GetFrontWidth()),'solder wire to both faces before bodies block access'))
rows.sort()
with(out/'review/wire-joins.csv').open('w')as f:
 w=csv.writer(f);w.writerow(['x_mm','y_mm','net','hole_mm','copper_land_mm','assembly']);w.writerows(rows)
counts={'wire_joins':len(rows),'component_long_lead_bottom_only_pads':0,'short_usb_shell_front_only_pads':0,'smd_front_pads':0,'smd_back_pads':0,'wire_drill_mm':sorted({q[3]for q in rows}),'wire_land_mm':sorted({q[4]for q in rows}),'min_track_width_mm':min(p.ToMM(t.GetWidth())for t in b.GetTracks()if not isinstance(t,p.PCB_VIA))}
counts['segments']=sum(1 for t in b.GetTracks()if not isinstance(t,p.PCB_VIA))
counts['back_smd_refs']=sorted(fp.GetReference()for fp in b.GetFootprints()if fp.GetLayer()==p.B_Cu)
lengths={}
for t in b.GetTracks():
 if isinstance(t,p.PCB_VIA):continue
 width=str(round(p.ToMM(t.GetWidth()),4));lengths[width]=lengths.get(width,0)+p.ToMM(t.GetLength())
counts['track_width_lengths_mm']={key:round(value,3)for key,value in sorted(lengths.items())}
edge_points=[mm(q)for t in b.GetDrawings()if isinstance(t,p.PCB_SHAPE)and t.GetLayer()==p.Edge_Cuts for q in [t.GetStart(),t.GetEnd()]]
blank=[round(max(q[k]for q in edge_points)-min(q[k]for q in edge_points),3)for k in [0,1]]
counts['blank_dimensions_mm']=blank
counts['registration_rail_width_mm']=5.0
counts['body_dimensions_mm']=[round(blank[0]-5.0,3),blank[1]]
for fp in b.GetFootprints():
 for q in fp.Pads():
  if q.GetAttribute()==p.PAD_ATTRIB_PTH:
   front=fp.GetReference()=='J1' and q.GetNumber()=='SH'
   counts['short_usb_shell_front_only_pads'if front else'component_long_lead_bottom_only_pads']+=1
   q.SetAttribute(p.PAD_ATTRIB_SMD);q.SetDrillSize(p.VECTOR2I(0,0));layers=p.LSET();layers.AddLayer(p.F_Cu if front else p.B_Cu);layers.AddLayer(p.F_Mask if front else p.B_Mask);q.SetLayerSet(layers)
  elif q.GetAttribute()==p.PAD_ATTRIB_SMD:
   counts['smd_back_pads'if q.GetLayerSet().Contains(p.B_Cu)else'smd_front_pads']+=1
audit=out/'.audit';audit.mkdir(exist_ok=True)
p.SaveBoard(str(audit/'diagnostic.kicad_pcb'),b)
shutil.copy2(out/'starter.kicad_pro',audit/'diagnostic.kicad_pro')
(audit/'fp-lib-table').write_text((out/'fp-lib-table').read_text().replace('${KIPRJMOD}/','${KIPRJMOD}/../'))
cli=shutil.which('kicad-cli')
if not cli:
 candidate=Path('/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
 if candidate.is_file():cli=str(candidate)
if not cli:raise RuntimeError('kicad-cli is required on PATH (or the standard macOS KiCad location)')
run=subprocess.run([cli,'pcb','drc','--all-track-errors','--refill-zones','--save-board','--output',str(out/'review/unplated-connectivity.rpt'),str(audit/'diagnostic.kicad_pcb')],capture_output=True,text=True)
print(run.stdout,flush=True)
report=(out/'review/unplated-connectivity.rpt').read_text()
if '** Found 0 unconnected pads **'not in report:raise RuntimeError('Unplated connectivity diagnostic has missing connections')
kinds=re.findall(r'^\[([^]]+)\]:',report,re.M)
unexpected=sorted(set(kinds)-{'padstack','lib_footprint_mismatch'})
if unexpected:raise RuntimeError('Unexpected physical-model DRC findings: '+', '.join(unexpected))
counts['unplated_model_unconnected_items']=0
counts['expected_diagnostic_warnings']={kind:kinds.count(kind)for kind in sorted(set(kinds))}
counts['source_sha256']=hashlib.sha256((out/'starter.kicad_pcb').read_bytes()).hexdigest()
counts['diagnostic_note']='Expected library/padstack warnings arise solely from replacing PTH component pads with single-face electrical contact models. This model is not a fabrication PCB.'
(out/'review/construction.json').write_text(json.dumps(counts,indent=2)+'\n')
print(counts,flush=True);os._exit(0)
