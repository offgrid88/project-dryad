import AsyncStorage from "@react-native-async-storage/async-storage";

import { DEFAULT_HOST, deviceUrl, parseSnapshot, type Snapshot } from "./protocol";

const HOST_KEY = "dryad-host";
const SNAP_KEY = "dryad-snapshot";

export async function loadHost(): Promise<string> {
  return (await AsyncStorage.getItem(HOST_KEY)) || DEFAULT_HOST;
}

export async function saveHost(host: string): Promise<void> {
  await AsyncStorage.setItem(HOST_KEY, host.trim() || DEFAULT_HOST);
}

async function remember(snap: Snapshot): Promise<void> {
  await AsyncStorage.setItem(SNAP_KEY, JSON.stringify(snap));
}

export async function loadCachedSnapshot(): Promise<Snapshot | null> {
  const raw = await AsyncStorage.getItem(SNAP_KEY);
  return raw ? parseSnapshot(raw) : null;
}

export async function fetchStatus(host: string): Promise<Snapshot> {
  const response = await fetch(deviceUrl(host, "/status"), { signal: AbortSignal.timeout(4000) });
  if (!response.ok) {
    throw new Error(`status ${response.status}`);
  }
  const snap = parseSnapshot(await response.text());
  if (!snap) {
    throw new Error("unexpected status");
  }
  await remember(snap);
  return snap;
}

export async function postCommand(host: string, body: string): Promise<Snapshot> {
  const response = await fetch(deviceUrl(host, "/cmd"), {
    method: "POST",
    headers: { "Content-Type": "application/json", Accept: "application/json" },
    body,
    signal: AbortSignal.timeout(4000),
  });
  if (!response.ok) {
    throw new Error(`command ${response.status}`);
  }
  const snap = parseSnapshot(await response.text());
  if (!snap) {
    throw new Error("unexpected reply");
  }
  await remember(snap);
  return snap;
}
