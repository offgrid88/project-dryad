import assert from "node:assert/strict";

import {
  cfgCommand,
  deviceUrl,
  parseSnapshot,
  pumpCommand,
  setCommand,
  thresholdsOk,
} from "./protocol.ts";

const sample = JSON.stringify({
  self: {
    t: "status",
    id: "aabbccddeeff",
    name: "Basil",
    moisture: 42,
    pump: 0,
    duty: 0,
    on: 35,
    off: 45,
    pwm: 70,
    hop: 0,
    seq: 3,
    mqtt: 0,
    root: 1,
  },
  peers: [
    {
      t: "status",
      id: "112233445566",
      name: "Fern",
      moisture: 18,
      pump: 1,
      duty: 70,
      on: 0,
      off: 0,
      pwm: 0,
      hop: 1,
      seq: 7,
      mqtt: 0,
      root: 0,
    },
  ],
  mesh: "DRYAD1",
  mqtt: 0,
});

const snap = parseSnapshot(sample);
assert.ok(snap);
assert.equal(snap.self.name, "Basil");
assert.equal(snap.self.moisture, 42);
assert.equal(snap.mqtt, 0);
assert.equal(snap.self.mqtt, 0);
assert.equal(snap.peers.length, 1);
assert.equal(snap.peers[0].moisture, 18);
assert.equal(parseSnapshot("not json"), null);
assert.equal(parseSnapshot("{}"), null);

assert.equal(thresholdsOk(30, 55, 60), true);
assert.equal(thresholdsOk(80, 40, 70), false);
assert.equal(setCommand(30, 55, 60), '{"t":"set","on":30,"off":55,"pwm":60}');
assert.equal(pumpCommand(-1), '{"t":"pump","run":-1}');
assert.equal(
  cfgCommand({ name: "Herb", mqtt: 0, broker: "", ssid: "", pass: "" }),
  '{"t":"cfg","name":"Herb","mqtt":0,"broker":"","ssid":"","pass":""}',
);
assert.equal(deviceUrl("192.168.4.1", "/status"), "http://192.168.4.1/status");
assert.equal(deviceUrl("http://127.0.0.1:8092/", "/cmd"), "http://127.0.0.1:8092/cmd");

console.log("protocol ok");
