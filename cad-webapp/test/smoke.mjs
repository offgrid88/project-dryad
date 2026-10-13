// Builds the default model plus a small and a large vase and sanity-checks the meshes.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import * as THREE from 'three';
import { STLExporter } from 'three/addons/exporters/STLExporter.js';
import { build, volumeMm3, printPlate } from '../src/cad.js';

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
    const { layout: L, params, warnings } = build({ vaseD });
    const espL = L.esp[1] - L.esp[0], espW = L.esp[5] - L.esp[4];
    if (espL < 70 || espW < 28) { console.log('  !! ESP pocket smaller than S3 DevKit 70x28'); failed = true; }
    const h = parts.housing; h.computeBoundingBox();
    const hs = new THREE.Vector3(); h.boundingBox.getSize(hs);
    console.log(`  enclosure ${hs.x.toFixed(1)}x${hs.y.toFixed(1)}x${hs.z.toFixed(1)} (was 88x52x115)`);
    if (!(hs.x < 88 && hs.y < 52 && hs.z < 115)) { console.log('  !! enclosure did not shrink'); failed = true; }
    if (warnings.length) { console.log('  !! ' + warnings.join(' | ')); failed = true; }
    const plate = printPlate(parts), grp = new THREE.Group();
    for (const g of Object.values(plate)) grp.add(new THREE.Mesh(g));
    const stl = new STLExporter().parse(grp, { binary: true });
    fs.writeFileSync(path.join(out, 'plate.stl'), Buffer.from(stl.buffer));
    console.log('  wrote', path.join(out, 'plate.stl'));
  }
}
process.exit(failed ? 1 : 0);
