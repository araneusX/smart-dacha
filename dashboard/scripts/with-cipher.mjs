import { spawn } from "node:child_process";
import { webcrypto } from "node:crypto";
import { existsSync, readFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const enc = new TextEncoder();

function loadEnvFile(path) {
  if (!existsSync(path)) {
    return;
  }
  for (const line of readFileSync(path, "utf8").split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith("#")) {
      continue;
    }
    const eq = trimmed.indexOf("=");
    if (eq < 0) {
      continue;
    }
    const key = trimmed.slice(0, eq).trim();
    let value = trimmed.slice(eq + 1).trim();
    if (
      (value.startsWith('"') && value.endsWith('"')) ||
      (value.startsWith("'") && value.endsWith("'"))
    ) {
      value = value.slice(1, -1);
    }
    if (process.env[key] === undefined) {
      process.env[key] = value;
    }
  }
}

function encodeBase64(bytes) {
  return Buffer.from(bytes).toString("base64");
}

async function deriveAesKey(password, salt) {
  const base = await webcrypto.subtle.importKey("raw", enc.encode(password), "PBKDF2", false, [
    "deriveKey",
  ]);
  return webcrypto.subtle.deriveKey(
    { name: "PBKDF2", salt, iterations: 100000, hash: "SHA-256" },
    base,
    { name: "AES-GCM", length: 256 },
    false,
    ["encrypt"],
  );
}

async function encryptUrl(url, password) {
  const salt = webcrypto.getRandomValues(new Uint8Array(16));
  const iv = webcrypto.getRandomValues(new Uint8Array(12));
  const key = await deriveAesKey(password, salt);
  const cipher = new Uint8Array(
    await webcrypto.subtle.encrypt({ name: "AES-GCM", iv }, key, enc.encode(url)),
  );
  return Buffer.from(
    JSON.stringify({
      v: 1,
      s: encodeBase64(salt),
      i: encodeBase64(iv),
      c: encodeBase64(cipher),
    }),
    "utf8",
  ).toString("base64");
}

loadEnvFile(resolve(root, ".env"));

const url = (process.env.DASHBOARD_API_URL || "").trim();
const password = process.env.DASHBOARD_PASSWORD || "";
if (!url || !password) {
  console.error(
    "Set DASHBOARD_API_URL and DASHBOARD_PASSWORD in dashboard/.env or the process env.",
  );
  process.exit(1);
}
if (!/^https:\/\//i.test(url)) {
  console.error("DASHBOARD_API_URL must start with https://");
  process.exit(1);
}

process.env.PUBLIC_DASHBOARD_CIPHER = await encryptUrl(url, password);

const command = process.argv[2];
if (command !== "dev" && command !== "build") {
  console.error("Usage: node scripts/with-cipher.mjs <dev|build>");
  process.exit(1);
}

const child = spawn("npx", ["astro", command], {
  cwd: root,
  env: process.env,
  stdio: "inherit",
  shell: true,
});

child.on("exit", (code, signal) => {
  if (signal) {
    process.kill(process.pid, signal);
    return;
  }
  process.exit(code ?? 1);
});
