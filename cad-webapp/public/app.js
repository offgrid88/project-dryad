import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { STLExporter } from 'three/addons/exporters/STLExporter.js';
import { DEFAULTS, build, volumeMm3, printPlate, printOrient } from '/src/cad.js';

THREE.Object3D.DEFAULT_UP.set(0, 0, 1); // Z-up, same as slicers

const PARTS = {
  housing: { label: 'Housing', color: '#8fa98f', explode: [0, 0, 0] },
  panel: { label: 'Front panel', color: '#a6b8a6', explode: [0, 70, 0] },
  collar: { label: 'Vase collar', color: '#c9a66b', explode: [0, -70, 0] },
  arm: { label: 'Sensor arm', color: '#7fa6c9', explode: [0, 0, 60] },
  cradle: { label: 'Bottle cradle', color: '#c98f8f', explode: [70, 0, 0] },
};
// Numeric inputs grouped for the side panel.
const GROUPS = {
  'Vase collar': ['clearance', 'ringT', 'ringH', 'padT', 'rimGap'],
  'Housing': ['W', 'D', 'H', 'wall', 'topT', 'panelT'],
  'Screen': ['screenW', 'screenH', 'screenT', 'windowW', 'windowH', 'windowDz'],
  'Boards': ['espW', 'espH', 'espT', 'chgW', 'chgH', 'chgT', 'dcW', 'dcH', 'dcT', 'drvL', 'drvW', 'drvH'],
  'Battery / pump / switch': ['batL', 'batD', 'pumpL', 'pumpW', 'pumpH', 'swW', 'swH'],
  'Bottle cradle': ['cupH', 'cupWall', 'bottleMl'],
  'Sensor arm': ['armW', 'armT', 'inset', 'slotW', 'slotT', 'tubeOD'],
};

const $ = (id) => document.getElementById(id);
const canvas = $('view');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.setPixelRatio(devicePixelRatio);
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(40, 1, 1, 3000);
camera.position.set(330, 330, 120);
const controls = new OrbitControls(camera, canvas);
controls.target.set(0, -30, -60);
scene.add(new THREE.HemisphereLight(0xffffff, 0x556655, 1.1));
const sun = new THREE.DirectionalLight(0xffffff, 1.8); sun.position.set(200, 300, 400); scene.add(sun);
const grid = new THREE.GridHelper(600, 30, 0x889988, 0xaabbaa); grid.rotation.x = Math.PI / 2; grid.position.z = -145;
grid.material.transparent = true; grid.material.opacity = 0.35; scene.add(grid);

let params = { ...DEFAULTS };
try { params = { ...params, ...JSON.parse(localStorage.getItem('dryad-cad') || '{}') }; } catch {}
let model = null;                 // result of build()
const groups = {};                // part name -> THREE.Group (solid + ghosts)
const ghostMeshes = [];

function resize() {
  const r = canvas.parentElement.getBoundingClientRect();
  renderer.setSize(r.width, r.height, false);
  camera.aspect = r.width / r.height; camera.updateProjectionMatrix();
}
addEventListener('resize', resize); resize();
renderer.setAnimationLoop(() => { controls.update(); renderer.render(scene, camera); });

function ghostMesh(g) {
  const geo = g.shape === 'box'
    ? new THREE.BoxGeometry(g.b[1] - g.b[0], g.b[3] - g.b[2], g.b[5] - g.b[4]).translate((g.b[0] + g.b[1]) / 2, (g.b[2] + g.b[3]) / 2, (g.b[4] + g.b[5]) / 2)
    : (() => {
        const c = new THREE.CylinderGeometry(g.r, g.r, g.a1 - g.a0, 48), m = (g.a0 + g.a1) / 2;
        if (g.axis === 'z') return c.rotateX(Math.PI / 2).translate(g.c1, g.c2, m);
        if (g.axis === 'x') return c.rotateZ(Math.PI / 2).translate(m, g.c1, g.c2);
        return c.translate(g.c1, m, g.c2);
      })();
  const isVase = g.label === 'Vase';
  const mesh = new THREE.Mesh(geo, new THREE.MeshStandardMaterial({
    color: g.color, transparent: true, opacity: isVase ? 0.18 : 0.75, depthWrite: false, side: isVase ? THREE.DoubleSide : THREE.FrontSide,
  }));
  mesh.userData.isVase = isVase;
  return mesh;
}

function rebuild() {
  $('busy').style.display = 'block';
  // let the "Generating…" badge paint before the (synchronous) CSG work
  setTimeout(() => {
    const t0 = performance.now();
    model = build(params);
    for (const g of Object.values(groups)) { scene.remove(g); g.traverse((o) => { if (o.isMesh) { o.geometry.dispose(); o.material.dispose(); } }); }
    ghostMeshes.length = 0;
    for (const [name, def] of Object.entries(PARTS)) {
      const grp = new THREE.Group();
      grp.add(new THREE.Mesh(model.parts[name], new THREE.MeshStandardMaterial({ color: def.color, roughness: 0.6, flatShading: false })));
      for (const g of model.ghosts.filter((x) => x.part === name)) { const m = ghostMesh(g); ghostMeshes.push(m); grp.add(m); }
      groups[name] = grp; scene.add(grp);
    }
    applyView();
    $('warn').textContent = model.warnings.join(' ');
    showStats(performance.now() - t0);
    $('busy').style.display = 'none';
  }, 20);
}

function applyView() {
  const e = Number($('explode').value);
  for (const [name, def] of Object.entries(PARTS)) groups[name]?.position.set(def.explode[0] * e, def.explode[1] * e, def.explode[2] * e);
  for (const m of ghostMeshes) m.visible = m.userData.isVase ? $('showVase').checked : $('showGhost').checked;
  const legend = [...new Map(model.ghosts.map((g) => [g.label, g.color]))];
  $('legend').innerHTML = legend.map(([l, c]) => `<div><i style="background:${c}"></i>${l}</div>`).join('');
}

function showStats(ms) {
  let totalCm3 = 0;
  const rows = Object.entries(PARTS).map(([name, def]) => {
    const g = model.parts[name]; g.computeBoundingBox();
    const s = g.boundingBox.getSize(new THREE.Vector3()), cm3 = volumeMm3(g) / 1000; totalCm3 += cm3;
    return `<tr><td>${def.label}</td><td>${s.x.toFixed(0)}×${s.y.toFixed(0)}×${s.z.toFixed(0)}</td><td>${cm3.toFixed(0)} cm³</td></tr>`;
  });
  // PETG ≈ 1.27 g/cm³; real prints are ~25–40 % solid, so show the solid upper bound.
  rows.push(`<tr><td><b>Total</b></td><td></td><td>${totalCm3.toFixed(0)} cm³ (≤${(totalCm3 * 1.27).toFixed(0)} g solid)</td></tr>`);
  rows.push(`<tr><td colspan="3">Generated in ${ms.toFixed(0)} ms</td></tr>`);
  $('stats').innerHTML = rows.join('');
}

// ---- exports ----
function download(name, group) {
  group.updateMatrixWorld(true);
  const data = new STLExporter().parse(group, { binary: true });
  const a = document.createElement('a');
  a.href = URL.createObjectURL(new Blob([data], { type: 'model/stl' }));
  a.download = name; a.click(); setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}
const meshes = (geos) => { const g = new THREE.Group(); for (const geo of geos) g.add(new THREE.Mesh(geo)); return g; };
const tag = () => `v${params.vaseD}_b${params.bottleD}`;

$('dlAll').onclick = () => download(`dryad_assembly_${tag()}.stl`, meshes(Object.values(model.parts)));
$('dlPlate').onclick = () => download(`dryad_print_plate_${tag()}.stl`, meshes(Object.values(printPlate(model.parts))));
for (const [name, def] of Object.entries(PARTS)) {
  const b = document.createElement('button'); b.textContent = def.label + ' STL';
  b.onclick = () => download(`dryad_${name}_${tag()}.stl`, meshes([printOrient(name, model.parts[name])]));
  $('partBtns').appendChild(b);
}

// ---- inputs ----
let timer;
function bind(input, key) {
  input.value = params[key];
  input.oninput = () => {
    const v = parseFloat(input.value);
    if (!Number.isFinite(v) || v <= 0) return;
    params[key] = v;
    try { localStorage.setItem('dryad-cad', JSON.stringify(params)); } catch {}
    clearTimeout(timer); timer = setTimeout(rebuild, 400);
  };
}
bind($('vaseD'), 'vaseD'); bind($('bottleD'), 'bottleD');
for (const [title, keys] of Object.entries(GROUPS)) {
  const d = document.createElement('details');
  d.innerHTML = `<summary>${title}</summary>` + keys.map((k) => `<label>${k}<input type="number" step="any" data-k="${k}"></label>`).join('');
  d.querySelectorAll('input').forEach((i) => bind(i, i.dataset.k));
  $('groups').appendChild(d);
}
for (const id of ['explode', 'showGhost', 'showVase']) $(id).oninput = () => model && applyView();

rebuild();
