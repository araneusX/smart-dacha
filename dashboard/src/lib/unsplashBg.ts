export type UnsplashBg = {
  /** data: URL (base64) preferred; http(s) only before encode completes */
  url: string;
  photographerName: string;
  photographerUrl: string;
};

type UnsplashPhoto = {
  urls?: { raw?: string; full?: string; regular?: string };
  user?: { name?: string; links?: { html?: string } };
  links?: { download_location?: string };
};

export type Season = "winter" | "spring" | "summer" | "autumn";

/** Northern-hemisphere season from local date. */
export function localSeason(now = new Date()): Season {
  const month = now.getMonth(); // 0–11
  if (month === 11 || month <= 2) {
    return "winter";
  }
  if (month <= 4) {
    return "spring";
  }
  if (month <= 9) {
    return "summer";
  }
  return "autumn";
}

type DayPart = "morning" | "day" | "evening" | "night";

/** Local hour windows shifted by season (≈ mid-latitude NH daylight). */
const DAY_PART_HOURS: Record<
  Season,
  { morning: number; day: number; evening: number; night: number }
> = {
  winter: { morning: 8, day: 11, evening: 15, night: 17 },
  spring: { morning: 6, day: 11, evening: 17, night: 20 },
  summer: { morning: 5, day: 10, evening: 18, night: 22 },
  autumn: { morning: 7, day: 11, evening: 16, night: 19 },
};

const DAY_PART_QUERY: Record<Season, Record<DayPart, string>> = {
  winter: {
    morning: "winter morning",
    day: "winter daytime",
    evening: "winter evening",
    night: "winter night",
  },
  spring: {
    morning: "spring morning",
    day: "spring daytime",
    evening: "spring evening",
    night: "spring night",
  },
  summer: {
    morning: "summer morning",
    day: "summer daytime",
    evening: "summer evening",
    night: "summer night",
  },
  autumn: {
    morning: "autumn morning",
    day: "autumn daytime",
    evening: "autumn evening",
    night: "autumn night",
  },
};

export function localDayPart(now = new Date()): DayPart {
  const season = localSeason(now);
  const hour = now.getHours();
  const bounds = DAY_PART_HOURS[season];

  if (hour >= bounds.night || hour < bounds.morning) {
    return "night";
  }
  if (hour >= bounds.evening) {
    return "evening";
  }
  if (hour >= bounds.day) {
    return "day";
  }
  return "morning";
}

/** Time-of-day + season phrases tuned for Unsplash (user timezone). */
export function localTimeOfDayQuery(now = new Date()): string {
  const season = localSeason(now);
  const part = localDayPart(now);
  return DAY_PART_QUERY[season][part];
}

/** Keep in sync with inline boot script in BaseLayout.astro */
export const SESSION_CACHE_KEY = "smart-dch-unsplash-bg";
export const UNLOCK_KEY = "smart-dch-unlocked";

type SessionCacheEntry = UnsplashBg & { query: string };

function isDataUrl(url: string): boolean {
  return url.startsWith("data:image/");
}

function readSessionCache(query: string): UnsplashBg | null {
  try {
    const raw = sessionStorage.getItem(SESSION_CACHE_KEY);
    if (!raw) {
      return null;
    }
    const entry = JSON.parse(raw) as SessionCacheEntry;
    if (!entry?.url || entry.query !== query || !isDataUrl(entry.url)) {
      return null;
    }
    return {
      url: entry.url,
      photographerName: entry.photographerName,
      photographerUrl: entry.photographerUrl,
    };
  } catch {
    return null;
  }
}

function writeSessionCache(query: string, bg: UnsplashBg): void {
  if (!isDataUrl(bg.url)) {
    return;
  }
  try {
    const entry: SessionCacheEntry = { ...bg, query };
    sessionStorage.setItem(SESSION_CACHE_KEY, JSON.stringify(entry));
  } catch (err) {
    console.warn("[unsplash] sessionStorage quota — base64 not cached", err);
  }
}

export function isSessionUnlocked(): boolean {
  try {
    return sessionStorage.getItem(UNLOCK_KEY) === "1";
  } catch {
    return false;
  }
}

export function markSessionUnlocked(): void {
  try {
    sessionStorage.setItem(UNLOCK_KEY, "1");
  } catch {
    // ignore
  }
}

function buildImageUrl(raw: string, width: number, quality: number): string {
  const url = new URL(raw);
  url.searchParams.set("auto", "format");
  url.searchParams.set("fit", "crop");
  url.searchParams.set("w", String(width));
  url.searchParams.set("q", String(quality));
  return url.toString();
}

async function fetchAsDataUrl(imageUrl: string): Promise<string> {
  const res = await fetch(imageUrl, { mode: "cors", credentials: "omit" });
  if (!res.ok) {
    throw new Error(`image HTTP ${res.status}`);
  }
  const blob = await res.blob();
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(String(reader.result));
    reader.onerror = () => reject(reader.error ?? new Error("FileReader failed"));
    reader.readAsDataURL(blob);
  });
}

/** Fetch Unsplash photo and return background with base64 data URL. */
export async function fetchUnsplashBackground(): Promise<UnsplashBg | null> {
  const key = import.meta.env.PUBLIC_UNSPLASH_ACCESS_KEY?.trim();
  if (!key) {
    return null;
  }

  const base = import.meta.env.PUBLIC_UNSPLASH_QUERY?.trim() || "cottage countryside house";
  const query = `${base} ${localTimeOfDayQuery()}`;

  const cached = readSessionCache(query);
  if (cached) {
    return cached;
  }

  const endpoint = new URL("https://api.unsplash.com/photos/random");
  endpoint.searchParams.set("query", query);
  endpoint.searchParams.set("orientation", "landscape");
  endpoint.searchParams.set("content_filter", "high");

  try {
    const res = await fetch(endpoint, {
      headers: { Authorization: `Client-ID ${key}`, "Accept-Version": "v1" },
    });
    if (!res.ok) {
      console.warn(`[unsplash] HTTP ${res.status}`);
      return null;
    }

    const photo = (await res.json()) as UnsplashPhoto;
    const raw = photo.urls?.raw || photo.urls?.full || photo.urls?.regular;
    if (!raw) {
      return null;
    }

    if (photo.links?.download_location) {
      await fetch(photo.links.download_location, {
        headers: { Authorization: `Client-ID ${key}`, "Accept-Version": "v1" },
      }).catch(() => undefined);
    }

    // Sized for sessionStorage (~5MB limit); still sharp on typical screens.
    const remoteUrl = buildImageUrl(raw, 1600, 72);
    const dataUrl = await fetchAsDataUrl(remoteUrl);

    const bg: UnsplashBg = {
      url: dataUrl,
      photographerName: photo.user?.name || "Unsplash",
      photographerUrl: photo.user?.links?.html || "https://unsplash.com",
    };
    writeSessionCache(query, bg);
    return bg;
  } catch (err) {
    console.warn("[unsplash] fetch failed", err);
    return null;
  }
}

type PreloadStore = { promise?: Promise<UnsplashBg | null> };

function preloadStore(): PreloadStore {
  const g = globalThis as typeof globalThis & { __smartDchUnsplash?: PreloadStore };
  if (!g.__smartDchUnsplash) {
    g.__smartDchUnsplash = {};
  }
  return g.__smartDchUnsplash;
}

/** Fetch + encode base64 into sessionStorage; does not paint the page. */
export function preloadUnsplashBackground(): Promise<UnsplashBg | null> {
  const store = preloadStore();
  if (!store.promise) {
    store.promise = (async () => {
      const bg = await fetchUnsplashBackground();
      if (bg) {
        return bg;
      }
      store.promise = undefined;
      return null;
    })();
  }
  return store.promise;
}

export function applyUnsplashBackground(root: ParentNode, bg: UnsplashBg): void {
  const layer = root.querySelector<HTMLElement>("[data-unsplash-bg]");
  if (layer) {
    layer.style.backgroundImage = `url(${JSON.stringify(bg.url)})`;
    layer.style.opacity = "1";
    layer.dataset.ready = "1";
    layer.classList.remove("opacity-0");
    layer.classList.add("opacity-100");
  }

  const def = root.querySelector<HTMLElement>("[data-default-bg]");
  if (def) {
    def.style.visibility = "hidden";
  }

  const credit = root.querySelector<HTMLElement>("[data-unsplash-credit]");
  if (credit) {
    credit.hidden = false;
  }

  const photographer = root.querySelector<HTMLAnchorElement>("[data-unsplash-photographer]");
  if (photographer) {
    photographer.textContent = bg.photographerName;
    photographer.href = `${bg.photographerUrl}?utm_source=smart-dch&utm_medium=referral`;
  }
}

/** Show background after login (uses preloaded/cached result when ready). */
export async function revealUnsplashBackground(root: ParentNode = document): Promise<void> {
  const bg = await preloadUnsplashBackground();
  if (bg) {
    applyUnsplashBackground(root, bg);
  } else {
    console.warn("[unsplash] no background to reveal");
  }
}
