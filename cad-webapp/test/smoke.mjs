// Builds the default model plus a small and a large vase and sanity-checks the meshes.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import * as THREE from 'three';
import { STLExporter } from 'three/addons/exporters/STLExporter.js';
import { build, volumeMm3, printPlate, fitReport } from '../src/cad.js';

const out = fs.mkdtempSync(path.join(process.env.TMPDIR || os.tmpdir(), 'dryad-cad-'));
let failed = false;
for (const vaseD of [70, 110, 200]) {
  const t0 = Date.now();
  const { parts } = build({ vaseD });
  console.log(`vaseD=${vaseD}  built in ${Date.now() - t0} ms`);
  for (const [name, g] of Object.entries(parts)) {
    g.computeBoundingBox();
    const s = new THREE.Vector3(); g.boundingBox.getSize(s);
    const vol = volumeMm3(g) / 1000;
    const tris = g.attributes.position.count / 3;
    console.log(`  ${name.padEnd(8)} ${tris} tris  ${s.x.toFixed(1)}x${s.y.toFixed(1)}x${s.z.toFixed(1)} mm  ${vol.toFixed(1)} cm3`);
    if (!(vol > 1) || tris < 12) { console.log('  !! suspicious mesh'); failed = true; }
  }
  if (vaseD === 110) {
    const open = build({ vaseD, screen: 1 });
    const closed = build({ vaseD, screen: 0 });
    const { layout: L, params, warnings } = open;
    const espL = L.esp[1] - L.esp[0], espZ = L.esp[5] - L.esp[4];
    if (espL < 34 || espL > 36 || espZ < 25 || espZ > 27) {
      console.log(`  !! ESP bay ${espL.toFixed(1)}x${espZ.toFixed(1)} is not the S2 Mini (34.3x25.4)`); failed = true;
    }
    if (L.drv[1] - L.drv[0] > 30) { console.log('  !! driver bay is still the large heatsink module'); failed = true; }
    const h = parts.housing; h.computeBoundingBox();
    const hs = new THREE.Vector3(); h.boundingBox.getSize(hs);
    console.log(`  enclosure ${hs.x.toFixed(1)}x${hs.y.toFixed(1)}x${hs.z.toFixed(1)} (limit 78x46x78)`);
    if (!(hs.x < 78 && hs.y < 46 && hs.z < 78)) { console.log('  !! enclosure exceeds 78x46x78'); failed = true; }
    if (!(hs.x > 70 && hs.y > 36 && hs.z > 68)) { console.log('  !! enclosure bbox is not the packed pod'); failed = true; }
    if (!(hs.z > params.H + 2)) { console.log('  !! crown missing'); failed = true; }
    const fit = fitReport(params, L);
    console.log(`  bay gap ${fit.minGap.toFixed(2)} mm (${fit.closest})  holder wall ${fit.holderWall} mm  sheath ${fit.sheathT.toFixed(2)} mm`);
    if (fit.outside.length) { console.log('  !! BOM outside the bay: ' + fit.outside.join(', ')); failed = true; }
    if (!(fit.minGap >= 0.55)) { console.log('  !! BOM clearance under 0.55 mm'); failed = true; }
    if (Math.abs(L.chg[0] + params.W / 2 - params.wall) > 0.3) { console.log('  !! TP4056 USB-C is not on the left wall'); failed = true; }
    const arm = parts.arm; arm.computeBoundingBox();
    const asz = new THREE.Vector3(); arm.boundingBox.getSize(asz);
    const armCm3 = volumeMm3(arm) / 1000;
    console.log(`  holder bbox ${asz.x.toFixed(1)}x${asz.y.toFixed(1)}x${asz.z.toFixed(1)} mm  ${armCm3.toFixed(1)} cm3`);
    if (!(asz.z < 7.5 && asz.z > 6)) { console.log('  !! holder spine is not the slim bar'); failed = true; }
    if (!(fit.holderWall <= 1.35 && fit.holderWall >= 0.8)) { console.log('  !! holder wall regressed'); failed = true; }
    if (!(fit.sheathT <= 5 && fit.sheathT >= 3.2)) { console.log('  !! probe sheath is chunky or missing'); failed = true; }
    if (!(armCm3 < 14)) { console.log('  !! sensor arm is still a chunky block'); failed = true; }
    if (warnings.length) { console.log('  !! ' + warnings.join(' | ')); failed = true; }
    if (closed.warnings.length) { console.log('  !! closed ' + closed.warnings.join(' | ')); failed = true; }
    const labels = (m) => m.ghosts.map((g) => g.label);
    if (!labels(open).includes('Screen') || labels(closed).includes('Screen')) {
      console.log('  !! screen ghost did not follow the switch'); failed = true;
    }
    const vol = (g) => volumeMm3(g) / 1000;
    const openLid = vol(open.parts.panel), closedLid = vol(closed.parts.panel);
    console.log(`  lid open ${openLid.toFixed(1)} cm3  closed ${closedLid.toFixed(1)} cm3`);
    if (!(closedLid > openLid + 1.5)) { console.log('  !! closed lid is not solid'); failed = true; }
    for (const [name, geom] of [['lid', open.parts.panel], ['closed lid', closed.parts.panel]]) {
      const worst = outsideRounded(geom, params);
      if (worst) { console.log(`  !! ${name} sticks past the rounded shell by ${worst.over.toFixed(2)} mm at x=${worst.x.toFixed(1)} z=${worst.z.toFixed(1)}`); failed = true; }
    }
    const plate = printPlate(parts), grp = new THREE.Group();
    for (const g of Object.values(plate)) grp.add(new THREE.Mesh(g));
    const stl = new STLExporter().parse(grp, { binary: true });
    fs.writeFileSync(path.join(out, 'plate.stl'), Buffer.from(stl.buffer));
    console.log('  wrote', path.join(out, 'plate.stl'));
  }
}

// Lid silhouette stays inside the housing's rounded front profile (XZ). The lid may stand proud in Y.
function outsideRounded(geom, p) {
  const pos = geom.attributes.position;
  const r = p.cornerR, hx = p.W / 2, allow = r + 0.45;
  const corners = [[hx - r, -r], [-(hx - r), -r], [hx - r, -p.H + r], [-(hx - r), -p.H + r]];
  const boxes = [[hx - r, 1e9, -r, 1e9], [-1e9, -(hx - r), -r, 1e9], [hx - r, 1e9, -1e9, -p.H + r], [-1e9, -(hx - r), -1e9, -p.H + r]];
  let worst = null;
  for (let i = 0; i < pos.count; i++) {
    const x = pos.getX(i), z = pos.getZ(i);
    let over = 0;
    if (z > p.crownH + 0.35) over = z - p.crownH;
    if (z < -p.H - 0.25) over = Math.max(over, -p.H - z);
    if (Math.abs(x) > hx + 0.25) over = Math.max(over, Math.abs(x) - hx);
    for (let k = 0; k < 4; k++) {
      const [x0, x1, z0, z1] = boxes[k];
      if (x > x0 && x < x1 && z > z0 && z < z1) {
        const dx = x - corners[k][0], dz = z - corners[k][1];
        const dist = Math.hypot(dx, dz);
        if (dist > allow) over = Math.max(over, dist - r);
      }
    }
    if (over > 0 && (!worst || over > worst.over)) worst = { over, x, z };
  }
  return worst;
}
process.exit(failed ? 1 : 0);
