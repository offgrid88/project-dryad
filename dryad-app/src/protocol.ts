export type LinkType = "status" | "set" | "pump" | "cfg";

export type StatusNode = {
  t: "status";
  id: string;
  name: string;
  moisture: number;
  pump: number;
  duty: number;
  on: number;
  off: number;
  pwm: number;
  hop: number;
  seq: number;
  mqtt: number;
  root: number;
};

export type Snapshot = {
  self: StatusNode;
  peers: StatusNode[];
  mesh: string;
  mqtt: number;
};

export const DEFAULT_HOST = "192.168.4.1";
export const MESH_SSID = "DRYAD1";
export const MESH_PASSWORD = "dryad-mesh";

function num(value: unknown, fallback = 0): number {
  return typeof value === "number" && Number.isFinite(value) ? value : fallback;
}

function node(value: unknown): StatusNode | null {
  if (!value || typeof value !== "object") {
    return null;
  }
  const row = value as Record<string, unknown>;
  if (typeof row.moisture !== "number") {
    return null;
  }
  return {
    t: "status",
    id: typeof row.id === "string" ? row.id : "",
    name: typeof row.name === "string" && row.name ? row.name : "Dryad",
    moisture: num(row.moisture),
    pump: num(row.pump),
    duty: num(row.duty),
    on: num(row.on, 35),
    off: num(row.off, 45),
    pwm: num(row.pwm, 70),
    hop: num(row.hop),
    seq: num(row.seq),
    mqtt: row.mqtt === 1 ? 1 : 0,
    root: row.root === 1 ? 1 : 0,
  };
}

export function parseSnapshot(text: string): Snapshot | null {
  try {
    const value = JSON.parse(text) as Record<string, unknown>;
    const self = node(value.self);
    if (!self) {
      return null;
    }
    const peers = Array.isArray(value.peers)
      ? value.peers.map(node).filter((peer): peer is StatusNode => peer !== null)
      : [];
    return {
      self,
      peers,
      mesh: typeof value.mesh === "string" && value.mesh ? value.mesh : MESH_SSID,
      mqtt: value.mqtt === 1 ? 1 : 0,
    };
  } catch {
    return null;
  }
}

export function thresholdsOk(on: number, off: number, pwm: number): boolean {
  return on >= 1 && on <= 98 && off >= 2 && off <= 100 && on < off && pwm >= 0 && pwm <= 100;
}

export function setCommand(on: number, off: number, pwm: number): string {
  return JSON.stringify({ t: "set", on, off, pwm });
}

export function pumpCommand(run: -1 | 0 | 1): string {
  return JSON.stringify({ t: "pump", run });
}

export type CfgInput = {
  name: string;
  mqtt: 0 | 1;
  broker: string;
  ssid: string;
  pass: string;
};

export function cfgCommand(input: CfgInput): string {
  return JSON.stringify({
    t: "cfg",
    name: input.name,
    mqtt: input.mqtt,
    broker: input.broker,
    ssid: input.ssid,
    pass: input.pass,
  });
}

export function deviceUrl(host: string, path: string): string {
  const trimmed = host.trim().replace(/\/+$/, "");
  const base =
    trimmed.startsWith("http://") || trimmed.startsWith("https://") ? trimmed : `http://${trimmed}`;
  return `${base}${path}`;
}
