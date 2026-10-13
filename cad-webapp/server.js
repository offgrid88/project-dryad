// Minimal static file server (no dependencies). Usage: npm start  [PORT=8080]
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.dirname(fileURLToPath(import.meta.url));
const port = Number(process.env.PORT) || 8080;
const types = { '.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript', '.css': 'text/css', '.json': 'application/json', '.svg': 'image/svg+xml' };
// Only these are served; keeps the rest of the repo out of reach.
const allowed = ['/public/', '/src/', '/node_modules/three/', '/node_modules/three-mesh-bvh/', '/node_modules/three-bvh-csg/'];

http.createServer((req, res) => {
  let url = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  if (url === '/') url = '/public/index.html';
  else if (!allowed.some((a) => url.startsWith(a))) url = '/public' + url;
  const file = path.join(root, url);
  if (!file.startsWith(root + path.sep) || !allowed.some((a) => url.startsWith(a))) { res.writeHead(404).end('not found'); return; }
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404).end('not found'); return; }
    res.writeHead(200, { 'Content-Type': types[path.extname(file)] || 'application/octet-stream' }).end(data);
  });
}).listen(port, '127.0.0.1', () => console.log(`Dryad CAD → http://localhost:${port}`));
