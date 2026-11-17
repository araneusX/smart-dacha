import { defineConfig } from "astro/config";
import tailwind from "@astrojs/tailwind";

// Placeholder until the GitHub repo/Pages URL is fixed.
const site = process.env.SITE_URL || "https://example.github.io";
const rawBase = process.env.BASE_PATH || "/smart-dachs";
const base = rawBase.endsWith("/") ? rawBase : `${rawBase}/`;

export default defineConfig({
  site,
  base,
  output: "static",
  integrations: [tailwind()],
});
