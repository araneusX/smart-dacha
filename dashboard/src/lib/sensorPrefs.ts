export type SensorPref = {
  name: string;
  hidden: boolean;
};

export type SensorPrefs = Record<string, SensorPref>;

const PREFS_KEY = "smart-dch-sensor-prefs";
const SEEN_KEY = "smart-dch-seen-sensors";

export function loadSensorPrefs(): SensorPrefs {
  try {
    const raw = localStorage.getItem(PREFS_KEY);
    if (!raw) {
      return {};
    }
    return JSON.parse(raw) as SensorPrefs;
  } catch {
    return {};
  }
}

export function saveSensorPrefs(prefs: SensorPrefs): void {
  localStorage.setItem(PREFS_KEY, JSON.stringify(prefs));
}

export function loadSeenSensorIds(): string[] {
  try {
    const raw = localStorage.getItem(SEEN_KEY);
    if (!raw) {
      return [];
    }
    const ids = JSON.parse(raw) as string[];
    return Array.isArray(ids) ? ids : [];
  } catch {
    return [];
  }
}

export function rememberSeenSensorIds(ids: string[]): void {
  const prev = new Set(loadSeenSensorIds());
  for (const id of ids) {
    if (id) {
      prev.add(id);
    }
  }
  localStorage.setItem(SEEN_KEY, JSON.stringify([...prev]));
}

export function displayName(id: string, prefs: SensorPrefs): string {
  const name = prefs[id]?.name?.trim();
  return name || id;
}

export function isHidden(id: string, prefs: SensorPrefs): boolean {
  return Boolean(prefs[id]?.hidden);
}
