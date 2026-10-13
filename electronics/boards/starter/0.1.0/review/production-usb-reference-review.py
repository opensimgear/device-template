"""Read-only native polygon audit of USB planes and the two authorized L2 bridges."""
import pcbnew as p, json, sys, os, math, hashlib
from pathlib import Path
root=Path(__file__).resolve().parent
default_board=root.parent/'starter.kicad_pcb' if root.name=='review' else root.parent/'production/starter.kicad_pcb'
path=Path(sys.argv[1]) if len(sys.argv)>1 else default_board
output=Path(sys.argv[2]) if len(sys.argv)>2 else root/'production-usb-reference-review.json'
hash0=hashlib.sha256(path.read_bytes()).hexdigest();b=p.LoadBoard(str(path))
issues=[];checks=[]
def ck(ok,label): (checks if ok else issues).append(label)
def xy(v):return [round(p.ToMM(v.x),7),round(p.ToMM(v.y),7)]
def poly(item,layer):
 s=p.SHAPE_POLY_SET();item.TransformShapeToPolygon(s,layer,0,1000,p.ERROR_OUTSIDE);return s
def ground(layer):
 s=p.SHAPE_POLY_SET()
 for z in b.Zones():
  if not z.GetIsRuleArea() and z.GetNetname()=='GND' and z.IsOnLayer(layer):s.BooleanAdd(z.GetFilledPolysList(layer))
 return s
def missing(s,plane):
 q=p.SHAPE_POLY_SET();q.BooleanSubtract(s,plane);return q
def span_gap(a,c):
 # Exact native copper shapes with a one-micrometre binary search on planar separation.
 lo,hi=0,100000000
 if a.Collide(c,0):return 0.0
 for _ in range(28):
  mid=(lo+hi)//2
  if a.Collide(c,mid):hi=mid
  else:lo=mid+1
 return p.ToMM(hi)
tracks=[t for t in b.GetTracks() if not isinstance(t,p.PCB_VIA)]
vias=[t for t in b.GetTracks() if isinstance(t,p.PCB_VIA)]
usbnet={'/USB_DM','/USB_DP'}
usbtracks=[t for t in tracks if t.GetNetname() in usbnet]
usbvias=[t for t in vias if t.GetNetname() in usbnet]
usbpads=[(f,d) for f in b.GetFootprints() for d in f.Pads() if d.GetNetname() in usbnet]
allusb=[t for t in tracks if t.GetNetname().startswith('/USB_')]+usbvias
allusb += [d for f in b.GetFootprints() for d in f.Pads() if d.GetNetname().startswith('/USB_') or f.GetReference()=='J1']
l2=[t for t in tracks if t.GetLayer()==p.In1_Cu]
allowed={'D14':[[128.0,126.2],[128.05,118.9]],'D12':[[125.4571429,129.0071429],[117.65,115.85]]}
ck(set(t.GetNetname()for t in l2)==set(allowed),'Only the two authorized D12/D14 signal nets occupy L2')
bridges=[]
for net,endpoints in allowed.items():
 ts=[t for t in l2 if t.GetNetname()==net]
 ends={tuple(xy(q)) for t in ts for q in [t.GetStart(),t.GetEnd()]}
 ck(all(any(math.dist(a,z)<.005 for z in ends)for a in endpoints),f'{net} connects the revised authorized transition sites')
 length=sum(math.dist(xy(t.GetStart()),xy(t.GetEnd()))for t in ts)
 bridgevias=[v for v in vias if v.GetNetname()==net and tuple(xy(v.GetPosition()))in ends]
 distances=[]
 for t in ts+bridgevias:
  shape=t.GetEffectiveShape(p.In1_Cu)
  for u in allusb:
   layer=u.GetLayer() if not isinstance(u,p.PCB_VIA) else p.F_Cu
   distances.append(span_gap(shape,u.GetEffectiveShape(layer)))
 minimum=min(distances)if distances else None
 ck(minimum is not None and minimum>=1.0,f'{net} stays at least1mm from every USB copper track/pad projection')
 bridges.append({'net':net,'tracks':[{'start_mm':xy(t.GetStart()),'end_mm':xy(t.GetEnd()),'width_mm':p.ToMM(t.GetWidth()),'uuid':t.m_Uuid.AsString()}for t in ts],'transition_vias':[{'position_mm':xy(v.GetPosition()),'l2_copper_diameter_mm':p.ToMM(v.GetWidth(p.In1_Cu)),'uuid':v.m_Uuid.AsString()}for v in bridgevias],'authorized_transition_sites_mm':endpoints,'total_trace_length_mm':length,'minimum_planar_copper_gap_including_transition_vias_to_all_usb_mm':minimum})
planes={p.In1_Cu:ground(p.In1_Cu),p.In2_Cu:ground(p.In2_Cu)}
ck(planes[p.In1_Cu].OutlineCount()==1,'Filled L2 GND is exactly one connected region')
# Front legs ending at duplicate-contact signal vias are branches, not the main front pair.
# Their unavoidable antipads are reported separately, never counted as main-path coverage.
via_points={tuple(xy(v.GetPosition())) for v in usbvias}
front=[t for t in usbtracks if t.GetLayer()==p.F_Cu]
duplicates=[t for t in front if tuple(xy(t.GetStart()))in via_points or tuple(xy(t.GetEnd()))in via_points]
main=[t for t in front if t not in duplicates]
uncovered_main=[]
for t in main:
 q=missing(poly(t,p.F_Cu),planes[p.In1_Cu])
 if not q.IsEmpty():uncovered_main.append({'net':t.GetNetname(),'uuid':t.m_Uuid.AsString(),'start_mm':xy(t.GetStart()),'end_mm':xy(t.GetEnd()),'uncovered_outline_count':q.OutlineCount()})
uncovered_pads=[]
for f,d in usbpads:
 q=missing(poly(d,d.GetLayer()),planes[p.In1_Cu])
 if not q.IsEmpty():uncovered_pads.append({'ref':f.GetReference(),'pad':d.GetNumber(),'position_mm':xy(d.GetPosition()),'uncovered_outline_count':q.OutlineCount()})
ck(not uncovered_main,'All main front USB track full widths and endcaps have filled L2 GND beneath')
ck(not uncovered_pads,'All differential USB component/connector launch pad full widths have filled L2 GND beneath')
# Signal vias necessarily have L3 antipads. Use the actual saved fill holes, clipped
# independently at each via's copper radius + clearance + native maximum polygon
# error. A merged hole is never excluded as a whole. KiCad's fill has a ~0.5 um
# offset beyond TransformShapeToPolygon's outside approximation, so subtracting
# that approximation alone would leave artificial rounding slivers. The bounded
# actual-hole exclusion preserves all real cuts outside each launch's error bound.
holes=p.SHAPE_POLY_SET();excluded=[]
plane=planes[p.In2_Cu]
actual_voids=p.SHAPE_POLY_SET()
for i in range(plane.OutlineCount()):
 for j in range(plane.HoleCount(i)):
  h=p.SHAPE_POLY_SET();h.AddOutline(plane.CHole(i,j));actual_voids.BooleanAdd(h)
zoneclear=max(z.GetLocalClearance()for z in b.Zones()if not z.GetIsRuleArea()and z.GetNetname()=='GND'and z.IsOnLayer(p.In2_Cu))
for v in usbvias:
 clearance=max(zoneclear,v.GetOwnClearance(p.In2_Cu))
 nominal_radius=v.GetWidth(p.In2_Cu)/2+clearance
 bound_radius=nominal_radius+b.GetDesignSettings().m_MaxError
 # A dense inscribed disk stays within the explicitly recorded radial bound.
 disk=p.SHAPE_POLY_SET();ring=p.SHAPE_LINE_CHAIN();center=v.GetPosition()
 for k in range(512):
  a=2*math.pi*k/512
  ring.Append(p.VECTOR2I(round(center.x+bound_radius*math.cos(a)),round(center.y+bound_radius*math.sin(a))))
 ring.SetClosed(True);disk.AddOutline(ring)
 antipad=p.SHAPE_POLY_SET();antipad.BooleanIntersection(actual_voids,disk)
 holes.BooleanAdd(antipad)
 excluded.append({'via_mm':xy(v.GetPosition()),'net':v.GetNetname(),'via_l3_copper_diameter_mm':p.ToMM(v.GetWidth(p.In2_Cu)),'clearance_mm':p.ToMM(clearance),'native_zone_polygon_max_error_mm':p.ToMM(b.GetDesignSettings().m_MaxError),'nominal_antipad_radius_mm':p.ToMM(nominal_radius),'actual_hole_clip_radius_mm':p.ToMM(bound_radius),'actual_clipped_antipad_area_mm2':antipad.Area()/1e12})
back_coverage=[]
for t in usbtracks:
 if t.GetLayer()!=p.B_Cu:continue
 full=poly(t,p.B_Cu);expected=p.SHAPE_POLY_SET();expected.BooleanSubtract(full,holes)
 q=missing(expected,plane)
 # Require a nonempty reference-covered part; a completely excluded bridge does not pass.
 ok=not expected.IsEmpty()and q.IsEmpty()
 ck(ok,f'{t.GetNetname()} back duplicate bridge has full-width filled L3 GND outside exact signal-via antipads')
 uncovered_points=[xy(q.COutline(i).CPoint(j))for i in range(q.OutlineCount())for j in range(q.COutline(i).PointCount())]
 bbox=[min(x[0]for x in uncovered_points),min(x[1]for x in uncovered_points),max(x[0]for x in uncovered_points),max(x[1]for x in uncovered_points)]if uncovered_points else None
 back_coverage.append({'net':t.GetNetname(),'start_mm':xy(t.GetStart()),'end_mm':xy(t.GetEnd()),'length_mm':math.dist(xy(t.GetStart()),xy(t.GetEnd())),'width_mm':p.ToMM(t.GetWidth()),'passes':ok,'reference_projection_empty_after_antipads':expected.IsEmpty(),'full_track_projection_area_mm2':full.Area()/1e12,'required_reference_area_after_launch_antipads_mm2':expected.Area()/1e12,'uncovered_area_mm2':q.Area()/1e12,'uncovered_outline_count':q.OutlineCount(),'uncovered_projection_bounds_mm':bbox})
ck(hash0==hashlib.sha256(path.read_bytes()).hexdigest(),'Source unchanged throughout independent audit')
result={'scope':'Native full-copper polygon projection check, with only the revised D12/D14 L2 exceptions. Does not certify impedance or hardware.','board_path':str(path),'board_sha256':hash0,'check_count':len(checks)+len(issues),'checks':checks,'issues':issues,'l2_signal_bridges':bridges,'l2_ground_regions':planes[p.In1_Cu].OutlineCount(),'l3_ground_regions':planes[p.In2_Cu].OutlineCount(),'main_front_usb_track_count':len(main),'main_usb_fullwidth_uncovered':uncovered_main,'usb_pad_fullwidth_uncovered':uncovered_pads,'duplicate_front_branch_tracks':len(duplicates),'back_bridge_coverage':back_coverage,'unavoidable_l3_signal_via_antipad_exclusions':excluded}
output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));sys.stdout.flush();os._exit(0)
