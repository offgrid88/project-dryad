import { useCallback, useEffect, useState } from "react";
import { Pressable, ScrollView, StyleSheet, Text, View } from "react-native";

import { fetchStatus, loadCachedSnapshot, loadHost } from "../device";
import { MESH_PASSWORD, MESH_SSID, type Snapshot } from "../protocol";
import { colors } from "../theme";

export default function MonitorScreen() {
  const [snap, setSnap] = useState<Snapshot | null>(null);
  const [offline, setOffline] = useState(false);
  const [note, setNote] = useState("Looking for the node…");

  const refresh = useCallback(async () => {
    const host = await loadHost();
    try {
      const next = await fetchStatus(host);
      setSnap(next);
      setOffline(false);
      setNote("");
    } catch {
      const cached = await loadCachedSnapshot();
      setSnap(cached);
      setOffline(true);
      setNote(
        cached
          ? "Offline. Showing the last reading stored on this phone."
          : `No reading yet. Join Wi-Fi ${MESH_SSID}, password ${MESH_PASSWORD}.`,
      );
    }
  }, []);

  useEffect(() => {
    refresh();
    const timer = setInterval(refresh, 3000);
    return () => clearInterval(timer);
  }, [refresh]);

  const self = snap?.self;

  return (
    <ScrollView style={styles.page} contentContainerStyle={styles.content}>
      {offline || !self ? (
        <View style={styles.banner}>
          <Text style={styles.bannerText}>{note}</Text>
        </View>
      ) : null}

      <View style={styles.hero}>
        <Text style={styles.kicker}>{self?.name ?? "Dryad"}</Text>
        <Text style={styles.moisture}>{self ? self.moisture : "—"}</Text>
        <Text style={styles.unit}>percent soil moisture</Text>
        <View style={[styles.pill, self?.pump ? styles.pillOn : styles.pillOff]}>
          <Text style={styles.pillText}>
            {self ? (self.pump ? `Pump running · ${self.duty}%` : "Pump idle") : "Pump unknown"}
          </Text>
        </View>
      </View>

      {self ? (
        <View style={styles.card}>
          <Row label="Mesh" value={snap?.mesh ?? MESH_SSID} />
          <Row label="Node" value={self.id || "this node"} />
          <Row label="Role" value={self.root ? "Mesh root" : "Mesh node"} />
          <Row label="Water below" value={`${self.on}%`} />
          <Row label="Stop at" value={`${self.off}%`} />
          <Row label="PWM" value={`${self.pwm}%`} />
          <Row label="MQTT" value={snap?.mqtt ? "Armed" : "Off"} />
        </View>
      ) : null}

      <Text style={styles.section}>Nearby nodes</Text>
      {snap && snap.peers.length > 0 ? (
        snap.peers.map((peer) => (
          <View key={peer.id || peer.name} style={styles.card}>
            <Text style={styles.peerName}>{peer.name}</Text>
            <Text style={styles.peerMeta}>
              {peer.moisture}% moisture · {peer.pump ? "pump on" : "pump idle"} · {peer.hop} hop
              {peer.hop === 1 ? "" : "s"}
            </Text>
          </View>
        ))
      ) : (
        <Text style={styles.empty}>Other plants on the mesh show up here.</Text>
      )}

      <Pressable style={styles.button} onPress={refresh}>
        <Text style={styles.buttonText}>Refresh</Text>
      </Pressable>
    </ScrollView>
  );
}

function Row({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.row}>
      <Text style={styles.rowLabel}>{label}</Text>
      <Text style={styles.rowValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  page: { flex: 1, backgroundColor: colors.cream },
  content: { padding: 20, paddingBottom: 40 },
  banner: {
    backgroundColor: "#f3e2d4",
    borderRadius: 14,
    padding: 14,
    marginBottom: 16,
  },
  bannerText: { color: colors.warn, fontSize: 15, lineHeight: 21 },
  hero: {
    backgroundColor: colors.ink,
    borderRadius: 24,
    padding: 22,
    marginBottom: 16,
  },
  kicker: { color: "#d7e7c8", fontSize: 16, fontWeight: "600" },
  moisture: { color: colors.cream, fontSize: 72, fontWeight: "700", marginTop: 4 },
  unit: { color: "#b7c9b4", fontSize: 15 },
  pill: { alignSelf: "flex-start", marginTop: 16, borderRadius: 999, paddingHorizontal: 12, paddingVertical: 6 },
  pillOn: { backgroundColor: colors.leaf },
  pillOff: { backgroundColor: "#3d5346" },
  pillText: { color: colors.cream, fontWeight: "600" },
  card: {
    backgroundColor: colors.card,
    borderRadius: 16,
    padding: 14,
    marginBottom: 10,
    borderWidth: 1,
    borderColor: colors.line,
  },
  row: { flexDirection: "row", justifyContent: "space-between", paddingVertical: 6 },
  rowLabel: { color: colors.moss, fontSize: 15 },
  rowValue: { color: colors.ink, fontSize: 15, fontWeight: "600" },
  section: { color: colors.ink, fontSize: 18, fontWeight: "700", marginTop: 8, marginBottom: 8 },
  peerName: { color: colors.ink, fontSize: 16, fontWeight: "700" },
  peerMeta: { color: colors.moss, marginTop: 4 },
  empty: { color: colors.off, marginBottom: 12 },
  button: {
    marginTop: 8,
    backgroundColor: colors.leaf,
    borderRadius: 14,
    paddingVertical: 14,
    alignItems: "center",
  },
  buttonText: { color: colors.cream, fontWeight: "700", fontSize: 16 },
});
