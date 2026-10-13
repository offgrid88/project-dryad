import http from "node:http";

const state = {
  name: "Basil",
  on: 35,
  off: 45,
  pwm: 70,
  run: -1,
  mqtt: 0,
  broker: "",
  ssid: "",
  pass: "",
  moisture: 28,
};

function pumping() {
  return state.run === 1 || (state.run === -1 && state.moisture < state.on);
}

function snapshot() {
  const on = pumping();
  return {
    self: {
      t: "status",
      id: "aabbccddeeff",
      name: state.name,
      moisture: state.moisture,
      pump: on ? 1 : 0,
      duty: on ? state.pwm : 0,
      on: state.on,
      off: state.off,
      pwm: state.pwm,
      hop: 0,
      seq: 1,
      mqtt: state.mqtt,
      root: 1,
    },
    peers: [
      {
        t: "status",
        id: "112233445566",
        name: "Fern",
        moisture: 62,
        pump: 0,
        duty: 0,
        on: 35,
        off: 45,
        pwm: 70,
        hop: 1,
        seq: 4,
        mqtt: 0,
        root: 0,
      },
    ],
    mesh: "DRYAD1",
    mqtt: state.mqtt,
  };
}

function apply(body) {
  const msg = JSON.parse(body);
  if (msg.t === "set") {
    if (msg.on < msg.off && msg.pwm >= 0 && msg.pwm <= 100) {
      state.on = msg.on;
      state.off = msg.off;
      state.pwm = msg.pwm;
    }
  } else if (msg.t === "pump" && (msg.run === -1 || msg.run === 0 || msg.run === 1)) {
    state.run = msg.run;
  } else if (msg.t === "cfg") {
    if (msg.name) state.name = msg.name;
    state.mqtt = msg.mqtt === 1 && msg.broker ? 1 : 0;
    state.broker = msg.broker ?? "";
    state.ssid = msg.ssid ?? "";
    state.pass = msg.pass ?? "";
  }
}

const server = http.createServer((req, res) => {
  const headers = {
    "Content-Type": "application/json",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Headers": "Content-Type, Accept",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
  };
  if (req.method === "OPTIONS") {
    res.writeHead(204, headers);
    res.end();
    return;
  }
  if (req.url === "/status" && req.method === "GET") {
    res.writeHead(200, headers);
    res.end(JSON.stringify(snapshot()));
    return;
  }
  if (req.url === "/cmd" && req.method === "POST") {
    const chunks = [];
    req.on("data", (chunk) => chunks.push(chunk));
    req.on("end", () => {
      try {
        apply(Buffer.concat(chunks).toString("utf8"));
        res.writeHead(200, headers);
        res.end(JSON.stringify(snapshot()));
      } catch {
        res.writeHead(400, headers);
        res.end('{"error":"bad json"}');
      }
    });
    return;
  }
  res.writeHead(404, headers);
  res.end('{"error":"not found"}');
});

server.listen(8092, "127.0.0.1", () => {
  console.log("mock node http://127.0.0.1:8092/status");
});
