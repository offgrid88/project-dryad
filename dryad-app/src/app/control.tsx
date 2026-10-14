import { useEffect, useState } from "react";
import { Pressable, StyleSheet, Text, TextInput, View } from "react-native";

import { fetchStatus, loadHost, postCommand } from "../device";
import { pumpCommand, setCommand, thresholdsOk } from "../protocol";
import { colors } from "../theme";

type Run = -1 | 0 | 1;

export default function ControlScreen() {
  const [on, setOn] = useState("35");
  const [off, setOff] = useState("45");
  const [pwm, setPwm] = useState("70");
  const [run, setRun] = useState<Run>(-1);
  const [message, setMessage] = useState("Automatic watering uses the thresholds below.");

  useEffect(() => {
    loadHost()
      .then(fetchStatus)
      .then((snap) => {
        setOn(String(snap.self.on));
        setOff(String(snap.self.off));
        setPwm(String(snap.self.pwm));
      })
      .catch(() => {
        setMessage("The node is out of reach. Thresholds will send when it answers.");
      });
  }, []);

  async function sendPump(next: Run) {
    setRun(next);
    try {
      const host = await loadHost();
      await postCommand(host, pumpCommand(next));
      setMessage(next === -1 ? "Automatic watering." : next === 1 ? "Pump forced on." : "Pump forced off.");
    } catch {
      setMessage("Could not reach the node. The pump command was not applied.");
    }
  }

  async function sendThresholds() {
    const onN = Number(on);
    const offN = Number(off);
    const pwmN = Number(pwm);
    if (!thresholdsOk(onN, offN, pwmN)) {
      setMessage("Start must be below stop, and PWM must be 0 to 100.");
      return;
    }
    try {
      const host = await loadHost();
      await postCommand(host, setCommand(onN, offN, pwmN));
      setMessage("Thresholds saved on the node.");
    } catch {
      setMessage("Could not reach the node. Thresholds were not saved.");
    }
  }

  return (
    <View style={styles.page}>
      <Text style={styles.lead}>{message}</Text>
      <View style={styles.modes}>
        <Mode label="Auto" active={run === -1} onPress={() => sendPump(-1)} />
        <Mode label="On" active={run === 1} onPress={() => sendPump(1)} />
        <Mode label="Off" active={run === 0} onPress={() => sendPump(0)} />
      </View>
      <Text style={styles.hint}>A forced run still stops after 20 seconds, then waits 60 seconds.</Text>
      <Field label="Water below %" value={on} onChangeText={setOn} />
      <Field label="Stop at %" value={off} onChangeText={setOff} />
      <Field label="PWM %" value={pwm} onChangeText={setPwm} />
      <Pressable style={styles.button} onPress={sendThresholds}>
        <Text style={styles.buttonText}>Save thresholds</Text>
      </Pressable>
    </View>
  );
}

function Mode({ label, active, onPress }: { label: string; active: boolean; onPress: () => void }) {
  return (
    <Pressable style={[styles.mode, active && styles.modeOn]} onPress={onPress}>
      <Text style={[styles.modeText, active && styles.modeTextOn]}>{label}</Text>
    </Pressable>
  );
}

function Field({
  label,
  value,
  onChangeText,
}: {
  label: string;
  value: string;
  onChangeText: (value: string) => void;
}) {
  return (
    <View style={styles.field}>
      <Text style={styles.label}>{label}</Text>
      <TextInput
        value={value}
        onChangeText={onChangeText}
        keyboardType="number-pad"
        style={styles.input}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  page: { flex: 1, backgroundColor: colors.cream, padding: 20 },
  lead: { color: colors.ink, fontSize: 16, lineHeight: 22, marginBottom: 16 },
  modes: { flexDirection: "row", gap: 8, marginBottom: 10 },
  mode: {
    flex: 1,
    borderRadius: 14,
    paddingVertical: 14,
    alignItems: "center",
    backgroundColor: colors.card,
    borderWidth: 1,
    borderColor: colors.line,
  },
  modeOn: { backgroundColor: colors.ink, borderColor: colors.ink },
  modeText: { color: colors.ink, fontWeight: "700" },
  modeTextOn: { color: colors.cream },
  hint: { color: colors.moss, marginBottom: 18 },
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
    fontSize: 18,
  },
  button: {
    marginTop: 8,
    backgroundColor: colors.leaf,
    borderRadius: 14,
    paddingVertical: 14,
    alignItems: "center",
  },
  buttonText: { color: colors.cream, fontWeight: "700", fontSize: 16 },
});
