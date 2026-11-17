/// <reference types="astro/client" />

interface ImportMetaEnv {
  readonly PUBLIC_DASHBOARD_CIPHER?: string;
  /** Exposed to the browser — Unsplash Access Key for client-side fetch. */
  readonly PUBLIC_UNSPLASH_ACCESS_KEY?: string;
  readonly PUBLIC_UNSPLASH_QUERY?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}

interface Window {
  DASHBOARD_CIPHER?: string;
}
