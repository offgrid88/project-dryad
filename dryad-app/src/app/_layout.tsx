import { Tabs } from "expo-router";
import { StatusBar } from "expo-status-bar";

import { colors } from "../theme";

export default function RootLayout() {
  return (
    <>
      <StatusBar style="light" />
      <Tabs
        screenOptions={{
          headerStyle: { backgroundColor: colors.ink },
          headerTintColor: colors.cream,
          headerTitleStyle: { fontWeight: "600" },
          tabBarStyle: { backgroundColor: colors.ink, borderTopColor: colors.ink },
          tabBarActiveTintColor: "#d7e7c8",
          tabBarInactiveTintColor: "#8aa592",
        }}
      >
        <Tabs.Screen name="index" options={{ title: "Monitor" }} />
        <Tabs.Screen name="control" options={{ title: "Control" }} />
        <Tabs.Screen name="configure" options={{ title: "Configure" }} />
      </Tabs>
    </>
  );
}
