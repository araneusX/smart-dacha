export type LiveSensor = { id?: string; t?: number };
export type LivePayload = { sensors?: LiveSensor[] };

export type LiveCacheEntry = {
  sensors: LiveSensor[];
  updatedAt: number;
};

/** Keep in sync if referenced from inline scripts later */
export const LIVE_CACHE_KEY = "smart-dch-live-snapshot";

export function loadLiveSnapshot(): LiveCacheEntry | null {
  try {
    const raw = sessionStorage.getItem(LIVE_CACHE_KEY);
    if (!raw) {
      return null;
    }
    const entry = JSON.parse(raw) as LiveCacheEntry;
    if (!entry || !Array.isArray(entry.sensors) || typeof entry.updatedAt !== "number") {
      return null;
    }
    return entry;
  } catch {
    return null;
  }
}

export function saveLiveSnapshot(data: LivePayload, updatedAt = Date.now()): void {
  try {
    const entry: LiveCacheEntry = {
      sensors: Array.isArray(data?.sensors) ? data.sensors : [],
      updatedAt,
    };
    sessionStorage.setItem(LIVE_CACHE_KEY, JSON.stringify(entry));
  } catch {
    // ignore
  }
}

export function clearLiveSnapshot(): void {
  try {
    sessionStorage.removeItem(LIVE_CACHE_KEY);
  } catch {
    // ignore
  }
}
