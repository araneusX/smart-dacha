const enc = new TextEncoder();
const dec = new TextDecoder();

function decodeBase64(b64: string): Uint8Array {
  const bin = atob(b64);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) {
    out[i] = bin.charCodeAt(i);
  }
  return out;
}

async function deriveAesKey(password: string, salt: Uint8Array): Promise<CryptoKey> {
  const base = await crypto.subtle.importKey("raw", enc.encode(password), "PBKDF2", false, [
    "deriveKey",
  ]);
  return crypto.subtle.deriveKey(
    { name: "PBKDF2", salt, iterations: 100000, hash: "SHA-256" },
    base,
    { name: "AES-GCM", length: 256 },
    false,
    ["decrypt"],
  );
}

export async function decryptUrl(token: string, password: string): Promise<string> {
  const data = JSON.parse(atob(token)) as { s: string; i: string; c: string };
  const salt = decodeBase64(data.s);
  const iv = decodeBase64(data.i);
  const cipher = decodeBase64(data.c);
  const key = await deriveAesKey(password, salt);
  const plain = await crypto.subtle.decrypt({ name: "AES-GCM", iv }, key, cipher);
  return dec.decode(plain);
}
