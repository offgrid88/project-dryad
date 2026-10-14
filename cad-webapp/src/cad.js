// Parametric CAD for the Dryad body. Units: mm. Z is up.
//
// All parts are built in ONE shared "housing-local" frame so the assembly needs
// no transforms:  x = left/right, y = 0 at the housing back face (+y = front,
// towards the user), z = 0 at the housing top (negative = down).
// The vase axis sits at y = -(padT + ringT + vaseD/2 + clearance).
//
// Only the COLLAR depends on the vase, only the CRADLE on the bottle.
// Housing, front panel and arm are the same for every vase.
//
// Default combo (electronics bay), sources in the DEFAULTS comment:
//   LOLIN ESP32-S2 Mini, TP4056 USB-C, mini L298, VTC6 18650, JSB diaphragm pump.
// The soil probe is clipped by the sensor arm; it does not sit in the bay.
import * as THREE from 'three';
import { Brush, Evaluator, ADDITION, SUBTRACTION, INTERSECTION } from 'three-bvh-csg';

export const DEFAULTS = {
  // vase collar (the only vase-dependent part)
  vaseD: 110, clearance: 0.5, ringT: 4, ringH: 30, padT: 6, rimGap: 4,
  // Tight pod around the S2 Mini combo. Wall and top stay thick enough that
  // a 10 mm corner radius still leaves ~1.2 mm of skin outside the bay.
  W: 76, D: 42, H: 64, wall: 4, topT: 4.6, panelT: 2.6,
  cornerR: 10, crownH: 9,
  // 1 = screen window in the lid, 0 = closed lid (switch + charge port only)
  screen: 1,
  screenW: 38, screenH: 28, screenT: 4, windowW: 30, windowH: 22, windowDz: -10,
  // LOLIN S2 Mini V1.0.0: 34.3×25.4 mm from the Wemos dimension drawing
  // (CoThings page has no mm). Thickness 5.0 mm is assumed: 1.6 mm PCB + USB-C.
  // Headers in the product photo are loose, so the bay is the bare board.
  espW: 34.3, espH: 25.4, espT: 5.0,
  // TP4056/TC4056 USB-C module. Page has no mm; assumed 28×17×4.2 with the
  // USB-C receptacle on a short edge (the edge that meets the left-wall port).
  chgW: 28, chgH: 17, chgT: 4.2,
  // Mini driver. Page MEASURED 24.7×21×5 mm. The photo shows two 100 µF/16 V
  // cans, so the pocket is 12 mm tall (assumed can height) on that footprint.
  drvL: 24.7, drvW: 21, drvH: 12,
  // VTC6. Page says format 18650 only. Assumed IEC/Murata cell 18.35×65.1 mm.
  batL: 65.1, batD: 18.35,
  // JSB1523078 / 030 diaphragm pump (photo). Page lists 5 V, 80 mA, 60 kPa,
  // −40 kPa, 0.6 L/min, 15 g, and no millimeters. Assumed body 40×22×20 mm;
  // the twin nozzles point at the right wall.
  pumpL: 40, pumpW: 22, pumpH: 20,
  swW: 13.5, swH: 19.8,
  // reservoir cradle (the only bottle-dependent part)
  bottleD: 44, cupH: 45, cupWall: 2.4, bottleMl: 100,
  // Sensor arm. The listing MEASURED the probe at 92.1×23.5×6.5 mm and calls
  // it capacitive; the photo is a thin YL-69 fork plus a YL-38 board. The clip
  // grips the 1.6 mm blade (6.5 mm is the head/comparator, above the sheath).
  // Spine was a 26×10 mm bar; it is now a 8.6×6.8 mm rounded bar, clip wall 1.2.
  armW: 8.6, armT: 6.8, holderWall: 1.2, inset: 38,
  slotW: 24.0, slotT: 2.15, tubeOD: 4,
  probeW: 23.5, probeL: 92.1, probeHead: 6.5,
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
const I = (a, b) => ev.evaluate(a, b, INTERSECTION);

// Largest corner radius that keeps `skin` mm of plastic outside the rectangular bay.
function clampCorner(p, skin = 1.2) {
  let r = Math.min(p.cornerR, p.W / 2 - 8, p.H / 2 - 8);
  const limit = (dx, dz) => {
    while (r > 2) {
      if ((r - dx) * (r - dx) + (r - dz) * (r - dz) <= (r - skin) * (r - skin)) return;
      r -= 0.25;
    }
  };
  limit(p.wall, p.topT);
  limit(p.wall, p.wall);
  return Math.round(r * 100) / 100;
}

// Arch in the front view: side edges stay near z=0, the middle rises by crownH.
// `drop` lowers the whole arch (inner ceiling, or the lid sitting under the lip).
function crownBrush(p, drop = 0) {
  const R = p.crownR;
  const zc = p.crownH - drop - R;
  return cyl('y', R, -4, p.D + p.panelT + 8, 0, zc, 80);
}

function shapeOf(raw) {
  const p = { ...raw };
  p.crownH = Math.min(14, Math.max(1, p.crownH));
  const d = p.W / 2;
  p.crownR = (d * d + p.crownH * p.crownH) / (2 * p.crownH);
  p.cornerAsked = p.cornerR;
  p.cornerR = clampCorner(p);
  p.screenOn = !(p.screen === 0 || p.screen === false);
  return p;
}

// Rounded rectangle extruded along `axis`. Cross-section axes are
// y: (x,z), x: (y,z), z: (x,y).
function roundRect(axis, a0, a1, u0, u1, v0, v1, r, seg = 32) {
  r = Math.min(Math.max(r, 0), (u1 - u0) / 2 - 0.05, (v1 - v0) / 2 - 0.05);
  const slab = (uu0, uu1, vv0, vv1) => {
    if (axis === 'y') return box(uu0, uu1, a0, a1, vv0, vv1);
    if (axis === 'x') return box(a0, a1, uu0, uu1, vv0, vv1);
    return box(uu0, uu1, vv0, vv1, a0, a1);
  };
  if (!(r > 0.4)) return slab(u0, u1, v0, v1);
  const parts = [slab(u0 + r, u1 - r, v0, v1), slab(u0, u1, v0 + r, v1 - r)];
  for (const [cu, cv] of [[u0 + r, v0 + r], [u1 - r, v0 + r], [u0 + r, v1 - r], [u1 - r, v1 - r]])
    parts.push(cyl(axis, r, a0, a1, cu, cv, seg));
  return U(parts[0], ...parts.slice(1));
}

function bossZs(p) {
  // Stay on the flat band, clear of the lid window.
  return [-p.cornerR - 4.5, -p.H + p.cornerR + 7];
}

// Component boxes [x0,x1,y0,y1,z0,z1] shared by the CAD and the preview ghosts.
// Gaps are 0.8 mm between modules, ~1 mm around the cell, USB-C hard against
// the left inner wall so the charge port meets the TP4056.
export function layout(p) {
  const iw = p.W / 2 - p.wall;
  const yb = p.wall + 1.2; // clear of the back-wall screw pilots
  const floor = -p.H + p.wall;
  const g = 0.8;
  const bat = [-p.batL / 2, p.batL / 2, yb, yb + p.batD, floor + 1.0, floor + 1.0 + p.batD];
  // Pump above the cell, nozzles at +X. The body is the long side of the bay.
  const pump = [iw - 1 - p.pumpL, iw - 1, yb, yb + p.pumpW, bat[5] + g, bat[5] + g + p.pumpH];
  // Driver lies on the pump, clear of the nozzle end so the outlet tube can rise.
  const drv = [pump[0] + 0.6, pump[0] + 0.6 + p.drvL, pump[2] + 0.5, pump[2] + 0.5 + p.drvW, pump[5] + 0.6, pump[5] + 0.6 + p.drvH];
  const yFront = Math.max(pump[3], bat[3]) + g;
  const usbHalf = 7.4;
  const usbLo = -p.H + p.cornerR + usbHalf + 1;
  const usbHi = -p.cornerR - usbHalf - 1;
  const usbZ = Math.min(usbHi, Math.max(usbLo, -p.topT - 24));
  // Charger length runs in from the left wall (USB-C on that short edge).
  const chg = [-iw, -iw + p.chgW, yFront, yFront + p.chgT, usbZ - p.chgH / 2, usbZ + p.chgH / 2];
  const esp = [chg[1] + 1.0, chg[1] + 1.0 + p.espW, yFront, yFront + p.espT, -p.topT - 1 - p.espH, -p.topT - 1];
  const zc = -p.topT - 2 - p.screenH / 2;
  const screen = [-p.screenW / 2, p.screenW / 2, p.D - p.screenT, p.D, zc - p.screenH / 2, zc + p.screenH / 2];
  const swHalf = p.swH / 2;
  const swLo = -p.H + p.cornerR + swHalf + 1.2;
  const swHi = -p.cornerR - swHalf - 1.2;
  const sw = {
    y: yb + 7,
    z: Math.min(swHi, Math.max(swLo, bat[5] + swHalf + 0.9)),
  };
  const usb = { y: (chg[2] + chg[3]) / 2, z: (chg[4] + chg[5]) / 2 };
  const nozzleY = (pump[2] + pump[3]) / 2;
  const nozzleZ = (pump[4] + pump[5]) / 2;
  // Twin nozzles on the +X face, 9 mm apart. Inlet leaves through the right wall;
  // outlet turns up through the crown into the sensor arm.
  const inlet = { y: nozzleY, z: nozzleZ - 4.5 };
  const outlet = { x: pump[1] - 8, y: nozzleY, z: nozzleZ + 4.5 };
  const ys = -(p.padT + p.ringT + p.inset);
  return { iw, yb, floor, zc, esp, chg, drv, pump, bat, screen, sw, usb, inlet, outlet, ys };
}

// How far two AABB boxes are apart. Negative means they overlap by that much.
function separation(a, b) {
  const gaps = [0, 1, 2].map((i) => {
    const a0 = a[i * 2], a1 = a[i * 2 + 1], b0 = b[i * 2], b1 = b[i * 2 + 1];
    if (a1 <= b0) return b0 - a1;
    if (b1 <= a0) return a0 - b1;
    return 0;
  });
  if (gaps.every((g) => g === 0)) {
    const overlaps = [0, 1, 2].map((i) => Math.min(a[i * 2 + 1], b[i * 2 + 1]) - Math.max(a[i * 2], b[i * 2]));
    return -Math.min(...overlaps);
  }
  return Math.hypot(...gaps);
}

const BOM = [
  ['esp', 'S2 Mini'],
  ['chg', 'TP4056'],
  ['drv', 'L298'],
  ['pump', 'pump'],
  ['bat', '18650'],
];

// Bay containment and pair gaps for the default combo. Used by validate() and the smoke test.
export function fitReport(p, L) {
  const iw = L.iw, floor = L.floor, ceil = -p.topT;
  const outside = [];
  for (const [key, label] of BOM) {
    const b = L[key];
    if (b[0] < -iw - 0.05 || b[1] > iw + 0.05 || b[2] < p.wall - 0.05 || b[3] > p.D - 5.5 || b[4] < floor - 0.05 || b[5] > ceil + 0.05)
      outside.push(label);
  }
  let minGap = Infinity, closest = '';
  for (let i = 0; i < BOM.length; i++) for (let j = i + 1; j < BOM.length; j++) {
    const gap = separation(L[BOM[i][0]], L[BOM[j][0]]);
    if (gap < minGap) { minGap = gap; closest = `${BOM[i][1]}–${BOM[j][1]}`; }
  }
  return {
    outside, minGap, closest,
    holderWall: p.holderWall,
    armT: p.armT,
    sheathT: p.slotT + 2 * p.holderWall,
  };
}

function buildHousing(p, L) {
  const { W, D, H, wall, topT } = p, iw = L.iw, r = p.cornerR;
  // Outer pod: rounded front silhouette, then a crown that rises in the middle.
  let h = roundRect('y', 0, D, -W / 2, W / 2, -H, p.crownH + 1, r, 36);
  h = I(h, crownBrush(p, -0.2));
  // Rectangular bay (components sit in the corners of this, not in the fillet)
  // plus a ceiling that follows the crown so the cap is a shell, not a solid blob.
  const bay = box(-iw, iw, wall, D + 1, -H + wall, -topT);
  let cap = box(-iw + 0.4, iw - 0.4, wall, D + 1, -topT - 1, p.crownH + 2);
  cap = I(cap, crownBrush(p, topT));
  h = S(h, bay, cap);

  const bossX = iw - 1.6, bossR = 2.5;
  const bossZ = bossZs(p);
  const bossY0 = D - 6.2;
  const add = [];
  for (const sx of [-1, 1]) for (const z of bossZ)
    add.push(cyl('y', bossR, bossY0, D - 0.3, sx * bossX, z, 20));
  // End stops for the cell. It slides in from the front; the stops keep the
  // spring gap without a loose cavity.
  const [bx0, bx1, by0, by1, bz0] = L.bat;
  const t = 1.0;
  add.push(
    box(bx0 - t - 0.35, bx0 - 0.35, by0, by1, bz0, bz0 + 6),
    box(bx1 + 0.35, bx1 + t + 0.35, by0, by1, bz0, bz0 + 6),
  );
  h = U(h, ...add);

  const cut = [];
  for (const sx of [-1, 1]) for (const z of bossZ)
    cut.push(cyl('y', 1.25, bossY0 + 0.8, D + 1, sx * bossX, z, 16));
  for (const sx of [-1, 1]) for (const z of [-11, -27])
    cut.push(cyl('y', 1.7, -1, wall + 0.6, sx * (W / 2 - 12), z, 16)); // collar screws, stop short of the cell
  cut.push(cyl('z', 2.6, -topT - 1, p.crownH + 1, L.outlet.x, L.outlet.y, 20)); // pump outlet -> arm
  for (const sx of [-1, 1]) cut.push(cyl('z', 1.7, -topT - 1, p.crownH + 1, sx * 9, 17, 16));
  // USB-C slot, with a shallow oval recess so the charge port reads as a finished detail.
  cut.push(roundRect('x', -W / 2 - 1, -W / 2 + 0.7, L.usb.y - 4.2, L.usb.y + 4.2, L.usb.z - 7.2, L.usb.z + 7.2, 3.2, 18));
  cut.push(roundRect('x', -W / 2 - 1, -iw + 1, L.usb.y - 2.2, L.usb.y + 2.2, L.usb.z - 5.2, L.usb.z + 5.2, 2.1, 16));
  cut.push(roundRect('x', -W / 2 - 1, -W / 2 + 0.7, L.sw.y - p.swW / 2 - 1.4, L.sw.y + p.swW / 2 + 1.4, L.sw.z - p.swH / 2 - 1.4, L.sw.z + p.swH / 2 + 1.4, 3.4, 18));
  cut.push(roundRect('x', -W / 2 - 1, -iw + 1, L.sw.y - p.swW / 2, L.sw.y + p.swW / 2, L.sw.z - p.swH / 2, L.sw.z + p.swH / 2, 3.2, 18));
  cut.push(cyl('x', 2.6, iw - 1, W / 2 + 1, L.inlet.y, L.inlet.z, 24)); // pump inlet -> bottle
  for (const y of [6, 30]) cut.push(cyl('x', 1.7, iw - 1, W / 2 + 1, y, -H + 20, 16));
  return S(h, ...cut);
}

function buildPanel(p, L) {
  const { W, D, H, panelT } = p;
  const inset = 3.5;
  const lip = 4.0;
  const pr = Math.min(8, Math.max(3, p.cornerR - 2));
  let pl = roundRect('y', D - 0.45, D + panelT, -W / 2 + inset, W / 2 - inset, -H + inset, p.crownH, pr, 32);
  pl = I(pl, crownBrush(p, lip));
  const cut = [];
  const bossX = L.iw - 1.6;
  for (const sx of [-1, 1]) for (const z of bossZs(p))
    cut.push(cyl('y', 1.6, D - 1, D + panelT + 1, sx * bossX, z, 16));
  if (p.screenOn) {
    const [sx0, sx1, , , sz0, sz1] = L.screen;
    const q = 2.4, add = [];
    for (const [px, pz] of [[sx0 - q, sz0 - q], [sx1, sz0 - q], [sx0 - q, sz1], [sx1, sz1]])
      add.push(box(px, px + q, D - p.screenT, D + 0.4, pz, pz + q));
    pl = U(pl, ...add);
    const wz0 = L.zc + p.windowDz - p.windowH / 2, wz1 = L.zc + p.windowDz + p.windowH / 2;
    // Seated bezel, then the window itself. Both have soft corners.
    pl = S(pl, roundRect('y', D + panelT - 0.65, D + panelT + 2, -p.windowW / 2 - 3.2, p.windowW / 2 + 3.2, wz0 - 3.2, wz1 + 3.2, 6, 24));
    cut.push(roundRect('y', D - 2, D + panelT + 2, -p.windowW / 2, p.windowW / 2, wz0, wz1, 5.5, 28));
  }
  // screen off: the lid stays closed. The on/off switch and the USB-C port are on the side wall.
  return cut.length ? S(pl, ...cut) : pl;
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
  const tipY = L.ys - p.slotT / 2 - 8;
  const yo = L.ys + 12;
  const zc = p.armT / 2;
  const bore = (p.tubeOD + 0.45) / 2;
  const wall = p.holderWall;
  const spineR = Math.min(2.2, p.armW / 2 - 0.35, p.armT / 2 - 0.35);
  // Slim rounded spine. A small pad takes the two lid screws, a rib carries the
  // outlet tube out to the pump chimney, and a thin sheath clips the probe.
  let a = roundRect('y', tipY, 26, -p.armW / 2, p.armW / 2, 0, p.armT, spineR, 20);
  const pad = roundRect('y', 7, 24, -13, 13, 0, p.armT, 2.6, 18);
  const rib = roundRect('x', -1.5, L.outlet.x + 2.5, L.outlet.y - p.armT / 2, L.outlet.y + p.armT / 2, 0, p.armT, spineR, 16);
  const clip = roundRect('z', 0, p.armT,
    -p.slotW / 2 - wall, p.slotW / 2 + wall,
    L.ys - p.slotT / 2 - wall, L.ys + p.slotT / 2 + wall,
    Math.min(1.15, wall), 16);
  a = U(a, pad, rib, clip);
  const cut = [
    roundRect('z', -1, p.armT + 1, -p.slotW / 2, p.slotW / 2, L.ys - p.slotT / 2, L.ys + p.slotT / 2, 0.55, 14),
    cyl('z', bore, -1, p.armT + 1, L.outlet.x, L.outlet.y, 16),
    cyl('x', bore, 0, L.outlet.x, L.outlet.y, zc, 16),
    cyl('y', bore, yo, L.outlet.y, 0, zc, 16),
    cyl('z', bore, -1, zc, 0, yo, 16),
  ];
  for (const sx of [-1, 1]) cut.push(cyl('z', 1.3, -1, p.armT - 1.4, sx * 9, 17, 16));
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
    cyl('x', 2.6, W / 2 - 1, W / 2 + 4, cy, L.inlet.z, 24),                    // tube hole through plate
    box(cx - 0.3 * bd, cx + 0.3 * bd, cy, cy + bd, zb + 8, zt + 1),            // front slit (odd bottle shapes)
  ];
  for (const y of [6, 30]) cut.push(cyl('x', 1.3, W / 2 - 1, W / 2 + 3, y, -H + 20, 16));
  return S(c, ...cut);
}

export function build(params = {}) {
  const raw = { ...DEFAULTS, ...params };
  const p = shapeOf(raw);
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
  return { params: p, layout: L, parts, ghosts: ghosts(p, L), warnings: validate(p, L) };
}

// Non-printed stand-ins for the real hardware, so placement can be checked visually.
export function ghosts(p, L) {
  const Ri = p.vaseD / 2 + p.clearance, yc = -p.padT - Ri - p.ringT;
  const bR = p.bottleD / 2, bH = (p.bottleMl * 1000) / (Math.PI * bR * bR) * 1.15 + 15;
  const bx = p.W / 2 + 4 + bR + p.cupWall - 0.8;
  const B = (part, label, color, b) => ({ part, label, color, shape: 'box', b });
  const C = (part, label, color, axis, r, a0, a1, c1, c2) => ({ part, label, color, shape: 'cyl', axis, r, a0, a1, c1, c2 });
  const head = p.probeHead;
  return [
    B('housing', 'ESP32-S2 Mini', '#2e7d32', L.esp),
    B('housing', 'TP4056 USB-C', '#1565c0', L.chg),
    B('housing', 'L298 driver', '#6d4c41', L.drv),
    B('housing', 'Pump', '#6a1b9a', L.pump),
    C('housing', 'VTC6 18650', '#c62828', 'x', p.batD / 2, L.bat[0], L.bat[1], (L.bat[2] + L.bat[3]) / 2, (L.bat[4] + L.bat[5]) / 2),
    B('housing', 'Switch', '#212121', [-L.iw - 2, -L.iw + 12, L.sw.y - p.swW / 2, L.sw.y + p.swW / 2, L.sw.z - p.swH / 2, L.sw.z + p.swH / 2]),
    ...(p.screenOn ? [B('panel', 'Screen', '#00838f', L.screen)] : []),
    B('arm', 'Soil probe', '#f9a825', [-p.probeW / 2, p.probeW / 2, L.ys - 0.8, L.ys + 0.8, p.armT + 2 - p.probeL, p.armT + 2]),
    B('arm', 'Soil probe', '#ef6c00', [-p.probeW / 2, p.probeW / 2, L.ys - head / 2, L.ys + head / 2, p.armT + 2, p.armT + 10]),
    C('collar', 'Vase', '#8d6e63', 'z', p.vaseD / 2, -p.rimGap - 130, -p.rimGap, 0, yc),
    C('cradle', 'Bottle', '#4fc3f7', 'z', bR, -p.H + 3, -p.H + 3 + bH, bx, L.inlet.y),
  ];
}

export function validate(p, L = layout(p)) {
  const w = [];
  if (p.cornerAsked != null && p.cornerR + 0.2 < p.cornerAsked)
    w.push(`Corner radius reduced to ${p.cornerR} mm so the bay wall stays intact.`);
  if (p.screenOn && (p.windowW > p.W - 16 || p.windowH > p.H - 20))
    w.push('Screen window is larger than the lid.');
  if (p.vaseD < 60) w.push('Vase Ø < 60 mm: the sensor arm cannot reach inside the pot.');
  if (p.vaseD < 2 * p.inset) w.push('Sensor reach is larger than the vase diameter; reduce "sensor inset".');
  if (p.vaseD > 220) w.push('Vase Ø > 220 mm: collar will be large and needs more than one housing screw row.');
  if (p.bottleD < 25 || p.bottleD > 70) w.push('Bottle Ø outside 25–70 mm: check the cradle fits the 100 ml bottle.');
  if (p.espW < 34 || p.espH < 25) w.push('ESP bay is smaller than a LOLIN S2 Mini (34.3×25.4 mm).');
  if (p.W < p.batL + 2 * p.wall + 2) w.push('Housing is narrower than the 18650 plus contact clearance.');
  if (p.armT < p.tubeOD + 2 * p.holderWall) w.push('Sensor arm is thinner than the tube plus its walls.');
  if (p.slotW < p.probeW) w.push('Probe clip is narrower than the soil probe.');
  const fit = fitReport(p, L);
  if (fit.outside.length) w.push(`Outside the bay: ${fit.outside.join(', ')}.`);
  if (fit.minGap < 0.55) w.push(`Parts closer than 0.55 mm (${fit.closest}, ${fit.minGap.toFixed(2)} mm).`);
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
