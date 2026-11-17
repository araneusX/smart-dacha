/** Keep in sync with inline boot script on index.astro */
export const API_URL_KEY = "smart-dch-api-url";

export function loadSessionApiUrl(): string | null {
  try {
    const value = sessionStorage.getItem(API_URL_KEY)?.trim() || "";
    return /^https:\/\//i.test(value) ? value : null;
  } catch {
    return null;
  }
}

export function saveSessionApiUrl(apiUrl: string): void {
  try {
    sessionStorage.setItem(API_URL_KEY, apiUrl.trim());
  } catch {
    // ignore
  }
}

export function clearSessionApiUrl(): void {
  try {
    sessionStorage.removeItem(API_URL_KEY);
    // legacy key from password-based session
    sessionStorage.removeItem("smart-dch-dashboard-password");
    sessionStorage.removeItem("smart-dch-live-snapshot");
  } catch {
    // ignore
  }
}
