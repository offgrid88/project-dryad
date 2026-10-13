import { useEffect, useState } from "react";
import { Pressable, ScrollView, StyleSheet, Switch, Text, TextInput, View } from "react-native";

import { fetchStatus, loadHost, postCommand, saveHost } from "../device";
import { MESH_PASSWORD, MESH_SSID, cfgCommand } from "../protocol";
import { colors } from "../theme";

export default function ConfigureScreen() {
  const [host, setHost] = useState("192.168.4.1");
  const [name, setName] = useState("Dryad");
  const [ssid, setSsid] = useState("");
  const [pass, setPass] = useState("");
  const [mqtt, setMqtt] = useState(false);
  const [broker, setBroker] = useState("");
  const [message, setMessage] = useState(
    `Leave MQTT off until a broker exists. The node stays on Wi-Fi ${MESH_SSID}.`,
  );

  useEffect(() => {
    loadHost().then(async (saved) => {
      setHost(saved);
      try {
        const snap = await fetchStatus(saved);
        setName(snap.self.name);
        setMqtt(snap.mqtt === 1);
      } catch {
        setMessage("Offline. Name and Wi-Fi changes send when the node answers.");
      }
    });
  }, []);

  async function save() {
    const trimmedName = name.trim();
    if (!trimmedName) {
      setMessage("Give the plant a name.");
      return;
    }
    if (mqtt && !broker.trim()) {
      setMessage("Add a broker URI, or leave MQTT off.");
      return;
    }
    const nextHost = host.trim() || "192.168.4.1";
    await saveHost(nextHost);
    try {
      await postCommand(
        nextHost,
        cfgCommand({
          name: trimmedName,
          mqtt: mqtt ? 1 : 0,
          broker: broker.trim(),
          ssid: ssid.trim(),
          pass,
        }),
      );
      setMessage(
        mqtt
          ? "Saved. MQTT will connect only because a broker URI is set."
          : "Saved. MQTT stays off. An empty home network keeps the mesh offline.",
      );
    } catch {
      setMessage("The address is stored on this phone. The node did not accept the change.");
    }
  }

  return (
    <ScrollView style={styles.page} contentContainerStyle={styles.content}>
      <Text style={styles.lead}>{message}</Text>
      <Field label="Plant name" value={name} onChangeText={setName} />
      <Field label="Node address" value={host} onChangeText={setHost} autoCapitalize="none" />
      <Text style={styles.hint}>
        Phone Wi-Fi is {MESH_SSID}, password {MESH_PASSWORD}. The node address is usually 192.168.4.1.
      </Text>
      <Field
        label="Home Wi-Fi (optional)"
        value={ssid}
        onChangeText={setSsid}
        autoCapitalize="none"
        placeholder="Leave empty for offline mesh"
      />
      <Field label="Home Wi-Fi password" value={pass} onChangeText={setPass} secureTextEntry />
      <View style={styles.switchRow}>
        <View style={styles.switchCopy}>
          <Text style={styles.switchTitle}>MQTT</Text>
          <Text style={styles.hint}>Off until you have a server. The client is already in the firmware.</Text>
        </View>
        <Switch
          value={mqtt}
          onValueChange={setMqtt}
          trackColor={{ false: "#d9d0c3", true: colors.leaf }}
          thumbColor={colors.cream}
        />
      </View>
      <Field
        label="Broker URI"
        value={broker}
        onChangeText={setBroker}
        autoCapitalize="none"
        placeholder="mqtt://10.0.0.8"
        editable={mqtt}
      />
      <Pressable style={styles.button} onPress={save}>
        <Text style={styles.buttonText}>Save configuration</Text>
      </Pressable>
    </ScrollView>
  );
}

function Field({
  label,
  value,
  onChangeText,
  placeholder,
  secureTextEntry,
  autoCapitalize,
  editable = true,
}: {
  label: string;
  value: string;
  onChangeText: (value: string) => void;
  placeholder?: string;
  secureTextEntry?: boolean;
  autoCapitalize?: "none" | "sentences";
  editable?: boolean;
}) {
  return (
    <View style={styles.field}>
      <Text style={styles.label}>{label}</Text>
      <TextInput
        value={value}
        onChangeText={onChangeText}
        placeholder={placeholder}
        secureTextEntry={secureTextEntry}
        autoCapitalize={autoCapitalize}
        editable={editable}
        style={[styles.input, !editable && styles.inputDisabled]}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  page: { flex: 1, backgroundColor: colors.cream },
  content: { padding: 20, paddingBottom: 40 },
  lead: { color: colors.ink, fontSize: 16, lineHeight: 22, marginBottom: 16 },
  field: { marginBottom: 12 },
  label: { color: colors.moss, marginBottom: 6 },
  input: {
    backgroundColor: colors.card,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: colors.line,
    paddingHorizontal: 12,
    paddingVertical: 12,
    color: colors.ink,
    fontSize: 16,
  },
  inputDisabled: { opacity: 0.55 },
  hint: { color: colors.moss, marginBottom: 12, lineHeight: 20 },
  switchRow: { flexDirection: "row", alignItems: "center", gap: 12, marginBottom: 12 },
  switchCopy: { flex: 1 },
  switchTitle: { color: colors.ink, fontWeight: "700", fontSize: 16, marginBottom: 4 },
  button: {
    marginTop: 8,
    backgroundColor: colors.leaf,
    borderRadius: 14,
    paddingVertical: 14,
    alignItems: "center",
  },
  buttonText: { color: colors.cream, fontWeight: "700", fontSize: 16 },
});
