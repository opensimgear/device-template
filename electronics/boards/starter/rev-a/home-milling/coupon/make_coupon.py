from pathlib import Path
import json, os, sys
import pcbnew as p
out=Path(__file__).resolve().parent
lib=Path(os.environ.get('KICAD10_FOOTPRINT_DIR','/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'))
b=p.BOARD(); b.SetCopperLayerCount(2)
def v(x,y): return p.VECTOR2I(p.FromMM(x),p.FromMM(y))
def mm(q): return tuple(p.ToMM(q))
def net(name):
 n=p.NETINFO_ITEM(b,name); b.Add(n); return n
nets={}
def n(name):
 if name not in nets: nets[name]=net(name)
 return nets[name]
def text(s,x,y,size=.8,layer=p.F_Fab,angle=0):
 t=p.PCB_TEXT(b);t.SetText(s);t.SetPosition(v(x,y));t.SetTextSize(v(size,size));t.SetTextThickness(p.FromMM(.1));t.SetLayer(layer);t.SetTextAngle(p.EDA_ANGLE(angle,p.DEGREES_T));b.Add(t);return t
def track(a,z,w,name,layer=p.F_Cu):
 t=p.PCB_TRACK(b);t.SetStart(v(*a));t.SetEnd(v(*z));t.SetWidth(p.FromMM(w));t.SetLayer(layer);t.SetNet(n(name));b.Add(t);return t
def endpoint(ref,position,netname,sz,layer=p.F_Cu):
 f=p.FOOTPRINT(b);f.SetReference(ref);f.SetValue('probe end');f.SetPosition(v(0,0));f.Reference().SetVisible(False);f.Value().SetVisible(False);b.Add(f)
 pad=p.PAD(f);pad.SetNumber('1');pad.SetAttribute(p.PAD_ATTRIB_SMD);pad.SetShape(p.PAD_SHAPE_RECT);pad.SetSize(v(*sz));layers=p.LSET();layers.AddLayer(layer);layers.AddLayer(p.F_Mask if layer==p.F_Cu else p.B_Mask);pad.SetLayerSet(layers);pad.SetPosition(v(*position));pad.SetNet(n(netname));f.Add(pad)
def footprint(libname,name,ref,x,y,ang=0):
 source=out/'OpenSimGear.pretty' if libname=='OpenSimGear' else lib/(libname+'.pretty')
 f=p.FootprintLoad(str(source),name)
 if libname=='OpenSimGear':f.SetFPID(p.LIB_ID('OpenSimGear',name))
 f.SetReference(ref);f.SetPosition(v(x,y));f.SetOrientationDegrees(ang)
 f.Reference().SetVisible(False);f.Value().SetVisible(False)
 b.Add(f);return f
# Outline: 45 x 35 mm, chosen to keep actual USB footprint and alignment holes apart.
for a,z in [((100,100),(145,100)),((145,100),(145,135)),((145,135),(100,135)),((100,135),(100,100))]:
 s=p.PCB_SHAPE(b);s.SetShape(p.SHAPE_T_SEGMENT);s.SetStart(v(*a));s.SetEnd(v(*z));s.SetWidth(p.FromMM(.05));s.SetLayer(p.Edge_Cuts);b.Add(s)
# Stock installed KiCad footprints remain dimensionally unchanged.
us=footprint('Connector_USB','USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal','J1',112,108,180)
us.SetValue('USB4105-GF-A milling fit test')
# Assign one net per geometrically distinct contact; coincident duplicated power lands share identity.
# No USB circuit is implied; independent fanouts deliberately do not join D+ and D- duplicate pins.
seen={}
for pad in us.Pads():
 if pad.GetNumber():
  key=mm(pad.GetPosition())
  if pad.GetNumber()=='SH':
   pad.SetNetCode(0);continue
  if key not in seen:seen[key]='USB_CONTACT_'+pad.GetNumber()
  pad.SetNet(n(seen[key]))
for num in ['B6','A7','A6','B7']:
 pad=next(a for a in us.Pads() if a.GetNumber()==num)
 x,y=mm(pad.GetPosition())
 track((x,y),(x,116.25),.2,pad.GetNetname());endpoint('USB_END_'+num,(x,116.25),pad.GetNetname(),(.2,.5))
qp=footprint('Package_QFP','LQFP-48_7x7mm_P0.5mm','U1',133,109)
qp.SetValue('LQFP48 milling fit test')
for pad in qp.Pads():pad.SetNet(n('QFP_PAD_'+pad.GetNumber()))
for num in ['13','14','15','16','17','18']:
 pad=next(a for a in qp.Pads() if a.GetNumber()==num)
 x,y=mm(pad.GetPosition()); track((x,y),(x,117),.2,pad.GetNetname());endpoint('QFP_END_'+num,(x,117),pad.GetNetname(),(.2,.5))
for num in ['1','2','3','4']:
 pad=next(a for a in qp.Pads() if a.GetNumber()==num)
 x,y=mm(pad.GetPosition()); track((x,y),(125.5,y),.2,pad.GetNetname());endpoint('QFP_END_'+num,(125.5,y),pad.GetNetname(),(.5,.2))
# Exact TI DCK land pattern used by the device's LM66100: 0.65 mm pitch,
# 0.90 x 0.40 mm rounded lands, 0.25 mm adjacent land gap. No active circuit.
sc=footprint('OpenSimGear','TI_DCK0006A_SC70-6_LM66100','U2',140.5,117.5)
sc.SetValue('LM66100DCKR milling fit test')
sc70_pads=[]
for pad in sc.Pads():
 name='SC70_PAD_'+pad.GetNumber();pad.SetNet(n(name))
 x,y=mm(pad.GetPosition());probe_x=138 if x<140.5 else 143.4
 track((x,y),(probe_x,y),.2,name);endpoint('SC70_END_'+pad.GetNumber(),(probe_x,y),name,(.5,.2))
 sc70_pads.append({'pin':pad.GetNumber(),'pad_center_mm':[x,y],'probe_center_mm':[probe_x,y]})
# Test lanes: isolated two-net parallel traces, each terminated with width-matched SMT lands.
# The narrow lands do not enlarge or invalidate the nominal test gap.
lanes=[]
for col,gap in enumerate([.20,.25,.30]):
 x0=104+10.5*col;x1=x0+8
 f=p.FOOTPRINT(b);f.SetReference('G'+str(col+1));f.SetValue('gap %.2f'%gap);f.SetPosition(v(0,0));f.Reference().SetVisible(False);f.Value().SetVisible(False);b.Add(f)
 for row,w in enumerate([.20,.25,.30,.40]):
  y0=123+2.2*row
  for side,yy in [('A',y0),('B',y0+w+gap)]:
   name='G%03d_W%03d_%s'%(round(gap*1000),round(w*1000),side)
   for end,xx in [('L',x0),('R',x1)]:
    pad=p.PAD(f);pad.SetNumber('%s%d%s'%(side,row,end));pad.SetAttribute(p.PAD_ATTRIB_SMD);pad.SetShape(p.PAD_SHAPE_RECT);pad.SetSize(v(.8,w));pad.SetLayerSet(p.PAD.SMDMask());pad.SetPosition(v(xx,yy));pad.SetNet(n(name));f.Add(pad)
   track((x0,y0 if side=='A' else yy),(x1,y0 if side=='A' else yy),w,name)
  lanes.append({'gap_mm':gap,'track_width_mm':w,'x_start_mm':x0,'x_end_mm':x1,'y_first_mm':y0,'y_second_mm':y0+w+gap})
 text('gap %.2f'%gap,x0+4,120.5,.8)
for row,w in enumerate([.20,.25,.30,.40]): text('%.2f'%w,101.65,123+2.2*row+.2,.8)
# Wire vias are visibly represented by through-hole land pairs, NOT plated barrels in home fabrication.
wire= p.FOOTPRINT(b);wire.SetReference('WV');wire.SetValue('manual wire vias');wire.SetPosition(v(0,0));wire.Reference().SetVisible(False);wire.Value().SetVisible(False);b.Add(wire)
for row,y in enumerate([123,128]):
 name='MANUAL_WIRE_'+str(row+1)
 for col,x in enumerate([138,141]):
  pad=p.PAD(wire);pad.SetNumber(str(row*2+col+1));pad.SetAttribute(p.PAD_ATTRIB_PTH);pad.SetShape(p.PAD_SHAPE_CIRCLE);pad.SetSize(v(1.8,1.8));pad.SetDrillSize(v(.8,.8));pad.SetLayerSet(p.LSET.AllCuMask());pad.SetPosition(v(x,y));pad.SetNet(n(name));wire.Add(pad)
 track((138,y),(141,y),.4,name)
 # Back-side protrusions let a probe exercise each manually joined face.
 track((138,y),(136.5,y),.4,name,p.B_Cu);track((141,y),(142.5,y),.4,name,p.B_Cu)
 endpoint('BACK_END_L'+str(row),(136.5,y),name,(.6,.4),p.B_Cu);endpoint('BACK_END_R'+str(row),(142.5,y),name,(.6,.4),p.B_Cu)
 text('WV'+str(row+1),139.5,y-1.65,.8)
for ref,x,y in [('A1',102.5,102.5),('A2',142.5,132.5)]:
 hole=footprint('MountingHole','MountingHole_2mm',''+ref,x,y)
 hole.SetValue('2 mm NPTH alignment')
text('USB4105 / 0.5 mm pitch',112,101.4,.8)
text('LQFP48: 0.5 mm pitch',132.5,101.5,.8)
text('QFP .30 pad / .20 gap',129.7,119,.8)
text('0.20 mm fanout tracks',111,118.6,.8)
text('SC70 .65 pitch',140.5,115.3,.65)
text('0.25 mm pad gap',140.5,119.8,.65)
text('WIRE .8 / 1.8 mm',139.5,131,.8)
text('45 x 35 mm - MILLING TEST ONLY',122,133.4,.8)

# Standalone project without schematic; default net class is the minimum gap under test.
p.SaveBoard(str(out/'milling-coupon.kicad_pcb'),b)
proj=json.loads((out/'milling-coupon.kicad_pro').read_text())
proj['meta']={'filename':'milling-coupon.kicad_pro','version':1};proj['boards']=[];proj['sheets']=[];proj['text_variables']={}
proj['net_settings']['classes']=proj['net_settings']['classes'][:1]
cl=proj['net_settings']['classes'][0];cl.update(clearance=.20,track_width=.20,via_diameter=1.8,via_drill=.8,diff_pair_width=.2,diff_pair_gap=.2)
proj['net_settings']['netclass_assignments']={};proj['net_settings']['netclass_patterns']=[]
rules=proj['board']['design_settings']['rules'];rules.update(min_clearance=.20,min_track_width=.20,min_hole_clearance=.15,min_hole_to_hole=.25,min_via_diameter=1.8,min_through_hole_diameter=.3,min_copper_edge_clearance=.5)
proj['board']['design_settings']['drc_exclusions']=[]
proj['pcbnew']={'last_paths':{'netlist':'','specctra_dsn':'','specctra_ses':'','step':''}}
(out/'milling-coupon.kicad_pro').write_text(json.dumps(proj,indent=2)+'\n')
(out/'fp-lib-table').write_text('(fp_lib_table (version 7) (lib (name "OpenSimGear") (type "KiCad") (uri "${KIPRJMOD}/OpenSimGear.pretty") (options "") (descr "Exact TI DCK manufacturer land pattern for milling qualification")))\n')
(out/'geometry.json').write_text(json.dumps({'board_size_mm':[45,35],'lanes':lanes,'fine_pitch_mm':.5,'fine_pad_width_mm':.3,'fine_pad_gap_mm':.2,'fanout_width_mm':.2,'sc70_test':{'mpn':'LM66100DCKR','footprint':'OpenSimGear:TI_DCK0006A_SC70-6_LM66100','source':'https://www.ti.com/lit/ds/symlink/lm66100.pdf','origin_mm':[140.5,117.5],'pin_pitch_mm':.65,'pad_size_mm':[.9,.4],'adjacent_pad_gap_mm':.25,'pad_roundrect_ratio':.125,'fanout_width_mm':.2,'probe_land_size_mm':[.5,.2],'pads':sc70_pads},'wire_hole_mm':.8,'wire_land_mm':1.8,'wire_positions_mm':[[138,123],[141,123],[138,128],[141,128]],'alignment_hole_mm':2,'alignment_positions_mm':[[102.5,102.5],[142.5,132.5]],'netlist':'none; independent milling test structures, not a functional USB device'},indent=2)+'\n')
print('Created 45 x 35 mm two-layer milling coupon with actual USB/LQFP/TI SC70 footprints, 12 clearance/width lane pairs, 4 manual wire-via holes and 2 NPTH alignment holes.')
sys.stdout.flush();os._exit(0)
