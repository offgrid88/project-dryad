// Parametric CAD for the Dryad body. Units: mm. Z is up.
//
// All parts are built in ONE shared "housing-local" frame so the assembly needs
// no transforms:  x = left/right, y = 0 at the housing back face (+y = front,
// towards the user), z = 0 at the housing top (negative = down).
// The vase axis sits at y = -(padT + ringT + vaseD/2 + clearance).
//
// Only the COLLAR depends on the vase, only the CRADLE on the bottle.
// Housing, front panel and arm are the same for every vase.
import * as THREE from 'three';
import { Brush, Evaluator, ADDITION, SUBTRACTION } from 'three-bvh-csg';

export const DEFAULTS = {
  // vase collar (the only vase-dependent part)
  vaseD: 110, clearance: 0.5, ringT: 4, ringH: 30, padT: 6, rimGap: 4,
  // housing
  W: 88, D: 52, H: 115, wall: 2.4, topT: 6, panelT: 3,
  // components (pocket sizes, edit to match your modules)
  screenW: 50, screenH: 70, screenT: 6, windowW: 43, windowH: 57, windowDz: 3,
  espW: 28, espH: 51, espT: 13,
  chgW: 26, chgH: 17, chgT: 8,
  dcW: 22, dcH: 17, dcT: 8,
  batL: 67, batD: 19,
  pumpL: 38, pumpW: 22, pumpH: 26,
  swW: 13.5, swH: 19.8,
  // reservoir cradle (the only bottle-dependent part)
  bottleD: 44, cupH: 45, cupWall: 2.4, bottleMl: 100,
  // sensor arm
  armW: 26, armT: 10, inset: 38, slotW: 24.5, slotT: 3, tubeOD: 4,
};

const SEG = 48;
const MAT = new THREE.MeshStandardMaterial();
const ev = new Evaluator();
ev.useGroups = false;
ev.attributes = ['position', 'normal'];

// Geometry is baked into world (housing-local) coordinates; brushes keep an
// identity transform so CSG results stay in the same frame.
const mk = (g) => { const b = new Brush(g, MAT); b.updateMatrixWorld(true); return b; };
const box = (x0, x1, y0, y1, z0, z1) =>
  mk(new THREE.BoxGeometry(x1 - x0, y1 - y0, z1 - z0).translate((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2));
// Cylinder along `axis` from a0..a1; c1,c2 = centre on the other two axes in x,y,z order.
function cyl(axis, r, a0, a1, c1, c2, seg = SEG) {
  const g = new THREE.CylinderGeometry(r, r, a1 - a0, seg);
  const m = (a0 + a1) / 2;
  if (axis === 'z') return mk(g.rotateX(Math.PI / 2).translate(c1, c2, m));
  if (axis === 'x') return mk(g.rotateZ(Math.PI / 2).translate(m, c1, c2));
  return mk(g.translate(c1, m, c2));
}
const U = (a, ...r) => r.reduce((acc, b) => ev.evaluate(acc, b, ADDITION), a);
const S = (a, ...r) => r.reduce((acc, b) => ev.evaluate(acc, b, SUBTRACTION), a);

// Component boxes [x0,x1,y0,y1,z0,z1] shared by the CAD and the preview ghosts.
export function layout(p) {
  const iw = p.W / 2 - p.wall;
  const yb = p.wall + 3; // 3 mm standoff from the back wall (screw heads live there)
  const floor = -p.H + p.wall;
  const zc = -p.topT - 3 - p.screenH / 2;
  const shelfTop = zc - p.screenH / 2 - 3;
  const esp = [6, 6 + p.espW, yb, yb + p.espT, -9 - p.espH, -9];
  const chg = [-iw, -iw + p.chgW, yb, yb + p.chgT, -13 - p.chgH, -13];
  const dcTop = chg[4] - 5;
  const dc = [-iw, -iw + p.dcW, yb, yb + p.dcT, dcTop - p.dcH, dcTop];
  const pump = [-p.pumpL / 2, p.pumpL / 2, p.wall + 1, p.wall + 1 + p.pumpW, floor, floor + p.pumpH];
  const by0 = pump[3] + 3;
  const bat = [-p.batL / 2, p.batL / 2, by0, by0 + p.batD + 2, floor, floor + p.batD + 2];
  const screen = [-p.screenW / 2, p.screenW / 2, p.D - p.screenT, p.D, zc - p.screenH / 2, zc + p.screenH / 2];
  const sw = { y: p.D / 2, z: dc[4] - 18 };
  const usb = { y: yb + 3.2, z: (chg[4] + chg[5]) / 2 };
  const inlet = { y: (pump[2] + pump[3]) / 2, z: -p.H + p.cupH - 5 };
  const ys = -(p.padT + p.ringT + p.inset);
  return { iw, yb, floor, zc, shelfTop, esp, chg, dc, pump, bat, screen, sw, usb, inlet, ys };
}

// Corner posts + pegs that locate a rectangular board. axis 'y': board mounted on the
// back wall (x/z footprint). axis 'z': board resting on a floor (x/y footprint).
function cradle(c, axis, wallY, p = 2) {
  const [x0, x1, y0, y1, z0, z1] = c;
  const out = [];
  if (axis === 'y') {
    for (const [px, pz] of [[x0 - p, z0 - p], [x1, z0 - p], [x0 - p, z1], [x1, z1]])
      out.push(box(px, px + p, wallY - 0.5, y1, pz, pz + p));
    for (const [qx, qz] of [[x0 + 4, z0 + 4], [x1 - 4, z0 + 4], [x0 + 4, z1 - 4], [x1 - 4, z1 - 4]])
      out.push(cyl('y', 2, wallY - 0.5, y0, qx, qz, 16));
  } else {
    const h = (z1 - z0) * 0.6;
    for (const [px, py] of [[x0 - p, y0 - p], [x1, y0 - p], [x0 - p, y1], [x1, y1]])
      out.push(box(px, px + p, py, py + p, z0 - 0.5, z0 + h));
  }
  return out;
}

function buildHousing(p, L) {
  const { W, D, H, wall, topT } = p, iw = L.iw;
  const postX = iw - 3, postZ = [-topT - 3, -H + wall + 3];
  let h = box(-W / 2, W / 2, 0, D, -H, 0);
  h = S(h, box(-iw, iw, wall, D + 1, -H + wall, -topT));
  const add = [];
  // front-panel screw posts (full depth, 4 corners)
  for (const sx of [-1, 1]) for (const z of postZ) add.push(cyl('y', 3, wall - 0.5, D, sx * postX, z, 24));
  // shelf between screen zone and pump/battery zone
  add.push(S(box(-iw, iw, wall - 0.5, D, L.shelfTop - 2, L.shelfTop),
    box(-8, 8, 6, 18, L.shelfTop - 3, L.shelfTop + 1),    // pump-outlet tube pass
    box(-36, -16, 6, 16, L.shelfTop - 3, L.shelfTop + 1))); // wire pass
  add.push(...cradle(L.esp, 'y', wall), ...cradle(L.chg, 'y', wall), ...cradle(L.dc, 'y', wall));
  add.push(...cradle(L.pump, 'z', wall), ...cradle(L.bat, 'z', wall, 1.6));
  h = U(h, ...add);

  const cut = [];
  for (const sx of [-1, 1]) for (const z of postZ) cut.push(cyl('y', 1.3, D - 12, D + 1, sx * postX, z, 16)); // panel pilots
  for (const sx of [-1, 1]) for (const z of [-11, -27]) cut.push(cyl('y', 1.7, -1, wall + 1, sx * (W / 2 - 12), z, 16)); // collar screws
  cut.push(cyl('z', 3.2, -topT - 1, 1, 0, 12, 24)); // pump outlet tube -> arm
  for (const sx of [-1, 1]) cut.push(cyl('z', 1.7, -topT - 1, 1, sx * 9, 17, 16)); // arm screws
  cut.push(box(-W / 2 - 1, -iw + 1, L.usb.y - 2.3, L.usb.y + 2.3, L.usb.z - 5.5, L.usb.z + 5.5)); // USB-C
  cut.push(box(-W / 2 - 1, -iw + 1, L.sw.y - p.swW / 2, L.sw.y + p.swW / 2, L.sw.z - p.swH / 2, L.sw.z + p.swH / 2)); // switch
  cut.push(cyl('x', 3.2, iw - 1, W / 2 + 1, L.inlet.y, L.inlet.z, 24)); // pump inlet tube -> bottle
  for (const y of [6, 30]) cut.push(cyl('x', 1.7, iw - 1, W / 2 + 1, y, -H + 20, 16)); // cradle screws
  return S(h, ...cut);
}

function buildPanel(p, L) {
  const { W, D, H, panelT } = p, iw = L.iw;
  let pl = box(-W / 2, W / 2, D, D + panelT, -H, 0);
  const [sx0, sx1, , , sz0, sz1] = L.screen;
  const q = 3, add = [];
  for (const [px, pz] of [[sx0 - q, sz0 - q], [sx1, sz0 - q], [sx0 - q, sz1], [sx1, sz1]])
    add.push(box(px, px + q, D - p.screenT, D + 0.5, pz, pz + q));
  pl = U(pl, ...add);
  const cut = [box(-p.windowW / 2, p.windowW / 2, D - 1, D + panelT + 1,
    L.zc + p.windowDz - p.windowH / 2, L.zc + p.windowDz + p.windowH / 2)];
  for (const sx of [-1, 1]) for (const z of [-p.topT - 3, -H + p.wall + 3])
    cut.push(cyl('y', 1.7, D - 1, D + panelT + 1, sx * (iw - 3), z, 16));
  return S(pl, ...cut);
}

function buildCollar(p) {
  const Ri = p.vaseD / 2 + p.clearance, Ro = Ri + p.ringT;
  const yc = -p.padT - Ro, zt = -p.rimGap, zb = zt - p.ringH, zm = (zt + zb) / 2;
  const ring = cyl('z', Ro, zb, zt, 0, yc, 96);
  const block = box(-p.W / 2, p.W / 2, yc + Ro * 0.5, 0, zb, zt);
  const ears = box(-10, 10, yc - Ro - 9, yc - Ro + 3, zb, zt);
  let c = U(ring, block, ears);
  const cut = [
    cyl('z', Ri, zb - 1, zt + 1, 0, yc, 128),
    box(-1.5, 1.5, yc - Ro - 10, yc, zb - 1, zt + 1),                 // clamp slit
    cyl('x', 2.3, -11, 11, yc - Ro - 4.5, zm, 24),                    // M4 clamp bolt
    cyl('x', 4.4, 6, 11, yc - Ro - 4.5, zm, 6),                       // nut trap
    cyl('x', 4.2, -11, -6, yc - Ro - 4.5, zm, 24),                    // bolt head recess
  ];
  for (const sx of [-1, 1]) for (const z of [-11, -27])
    cut.push(cyl('y', 1.3, -p.padT + 1.2, 0.5, sx * (p.W / 2 - 12), z, 16)); // M3 pilot holes
  return S(c, ...cut);
}

function buildArm(p, L) {
  const tipY = L.ys - p.slotT / 2 - 14, yo = L.ys + 10, zc = p.armT / 2, r = (p.tubeOD + 0.6) / 2;
  let a = box(-p.armW / 2, p.armW / 2, tipY, 28, 0, p.armT);
  const cut = [
    box(-p.slotW / 2, p.slotW / 2, L.ys - p.slotT / 2, L.ys + p.slotT / 2, -1, p.armT + 1), // sensor slot
    cyl('z', r, -1, zc, 0, 12, 24),                  // tube in (from housing)
    cyl('y', r, yo, 12, 0, zc, 24),                  // tube run
    cyl('z', r, -1, zc, 0, yo, 24),                  // outlet onto the soil
  ];
  for (const sx of [-1, 1]) cut.push(cyl('z', 1.3, -1, p.armT - 2, sx * 9, 17, 16));
  return S(a, ...cut);
}

function buildCradle(p, L) {
  const { W, H, cupH, cupWall: cw, bottleD: bd } = p;
  const Rc = bd / 2 + cw, cy = L.inlet.y, cx = W / 2 + 4 + Rc - 0.8, zb = -H, zt = -H + cupH;
  const plate = box(W / 2, W / 2 + 4, 0, 34, zb, zt);
  const cup = cyl('z', Rc, zb, zt, cx, cy, 96);
  let c = U(plate, cup);
  const cut = [
    cyl('z', bd / 2, zb + 3, zt + 1, cx, cy, 96),                              // bottle bore
    box(W / 2 + 1.5, cx, cy - 4, cy + 4, zt - 10, zt + 1),                     // tube notch
    cyl('x', 3.2, W / 2 - 1, W / 2 + 4, cy, L.inlet.z, 24),                    // tube hole through plate
    box(cx - 0.3 * bd, cx + 0.3 * bd, cy, cy + bd, zb + 8, zt + 1),            // front slit (odd bottle shapes)
  ];
  for (const y of [6, 30]) cut.push(cyl('x', 1.3, W / 2 - 1, W / 2 + 3, y, -H + 20, 16));
  return S(c, ...cut);
}

export function build(params = {}) {
  const p = { ...DEFAULTS, ...params };
  const L = layout(p);
  const geo = {
    housing: buildHousing(p, L),
    panel: buildPanel(p, L),
    collar: buildCollar(p),
    arm: buildArm(p, L),
    cradle: buildCradle(p, L),
  };
  const parts = {};
  for (const [k, b] of Object.entries(geo)) parts[k] = b.geometry;
  return { params: p, layout: L, parts, ghosts: ghosts(p, L), warnings: validate(p) };
}

// Non-printed stand-ins for the real hardware, so placement can be checked visually.
export function ghosts(p, L) {
  const Ri = p.vaseD / 2 + p.clearance, yc = -p.padT - Ri - p.ringT;
  const bR = p.bottleD / 2, bH = (p.bottleMl * 1000) / (Math.PI * bR * bR) * 1.15 + 15;
  const bx = p.W / 2 + 4 + bR + p.cupWall - 0.8;
  const B = (part, label, color, b) => ({ part, label, color, shape: 'box', b });
  const C = (part, label, color, axis, r, a0, a1, c1, c2) => ({ part, label, color, shape: 'cyl', axis, r, a0, a1, c1, c2 });
  return [
    B('housing', 'ESP32', '#2e7d32', L.esp),
    B('housing', 'USB-C charger', '#1565c0', L.chg),
    B('housing', 'DC-DC 3.7→5V', '#ef6c00', L.dc),
    B('housing', 'Pump', '#6a1b9a', L.pump),
    C('housing', 'Battery', '#c62828', 'x', p.batD / 2, L.bat[0], L.bat[1], (L.bat[2] + L.bat[3]) / 2, L.bat[4] + (p.batD + 2) / 2),
    B('housing', 'Switch', '#212121', [-L.iw - 3, -L.iw + 15, L.sw.y - p.swW / 2, L.sw.y + p.swW / 2, L.sw.z - p.swH / 2, L.sw.z + p.swH / 2]),
    B('panel', 'Screen', '#00838f', L.screen),
    B('arm', 'Moisture sensor', '#f9a825', [-11.5, 11.5, L.ys - 0.8, L.ys + 0.8, p.armT + 4 - 98, p.armT + 4]),
    C('collar', 'Vase', '#8d6e63', 'z', p.vaseD / 2, -p.rimGap - 130, -p.rimGap, 0, yc),
    C('cradle', 'Bottle', '#4fc3f7', 'z', bR, -p.H + 3, -p.H + 3 + bH, bx, L.inlet.y),
  ];
}

export function validate(p) {
  const w = [];
  if (p.vaseD < 60) w.push('Vase Ø < 60 mm: the sensor arm cannot reach inside the pot.');
  if (p.vaseD < 2 * p.inset) w.push('Sensor reach is larger than the vase diameter; reduce "sensor inset".');
  if (p.vaseD > 220) w.push('Vase Ø > 220 mm: collar will be large and needs more than one housing screw row.');
  if (p.bottleD < 25 || p.bottleD > 70) w.push('Bottle Ø outside 25–70 mm: check the cradle fits the 100 ml bottle.');
  return w;
}

// ---- helpers used by both the browser app and the node smoke test ----

export function volumeMm3(geometry) {
  const pos = geometry.attributes.position, idx = geometry.index;
  const n = idx ? idx.count : pos.count, a = new THREE.Vector3(), b = new THREE.Vector3(), c = new THREE.Vector3();
  let v = 0;
  for (let i = 0; i < n; i += 3) {
    const [i0, i1, i2] = idx ? [idx.getX(i), idx.getX(i + 1), idx.getX(i + 2)] : [i, i + 1, i + 2];
    a.fromBufferAttribute(pos, i0); b.fromBufferAttribute(pos, i1); c.fromBufferAttribute(pos, i2);
    v += a.dot(b.cross(c)) / 6;
  }
  return Math.abs(v);
}

// Rotation that puts each part in a sensible print orientation (flat face on the bed).
const PRINT_ROT = {
  housing: (g) => g.rotateX(Math.PI / 2),    // open front up
  panel: (g) => g.rotateX(-Math.PI / 2),     // outer face on the bed
  collar: (g) => g, arm: (g) => g, cradle: (g) => g,
};

// Returns { name: geometry } laid out on a bed, each part on z=0, parts spaced `gap` apart.
export function printPlate(parts, bedW = 256, gap = 8) {
  const out = {};
  let x = 0, y = 0, rowH = 0;
  for (const name of ['collar', 'housing', 'panel', 'arm', 'cradle']) {
    const g = PRINT_ROT[name](parts[name].clone());
    g.computeBoundingBox();
    const bb = g.boundingBox, w = bb.max.x - bb.min.x, d = bb.max.y - bb.min.y;
    if (x > 0 && x + w > bedW) { x = 0; y += rowH + gap; rowH = 0; }
    g.translate(x - bb.min.x, y - bb.min.y, -bb.min.z);
    out[name] = g;
    x += w + gap; rowH = Math.max(rowH, d);
  }
  return out;
}

export function printOrient(name, geometry) {
  const g = PRINT_ROT[name](geometry.clone());
  g.computeBoundingBox();
  const bb = g.boundingBox;
  return g.translate(-bb.min.x, -bb.min.y, -bb.min.z);
}
