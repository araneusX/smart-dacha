const enc = new TextEncoder();
const dec = new TextDecoder();

function decodeBase64(b64) {
  const bin = atob(b64);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) {
    out[i] = bin.charCodeAt(i);
  }
  return out;
}

function encodeBase64(bytes) {
  let s = "";
  for (let i = 0; i < bytes.length; i++) {
    s += String.fromCharCode(bytes[i]);
  }
  return btoa(s);
}

async function deriveAesKey(password, salt) {
  const base = await crypto.subtle.importKey("raw", enc.encode(password), "PBKDF2", false, [
    "deriveKey"
  ]);
  return crypto.subtle.deriveKey(
    { name: "PBKDF2", salt, iterations: 100000, hash: "SHA-256" },
    base,
    { name: "AES-GCM", length: 256 },
    false,
    ["encrypt", "decrypt"]
  );
}

async function encryptUrl(url, password) {
  const salt = crypto.getRandomValues(new Uint8Array(16));
  const iv = crypto.getRandomValues(new Uint8Array(12));
  const key = await deriveAesKey(password, salt);
  const cipher = new Uint8Array(
    await crypto.subtle.encrypt({ name: "AES-GCM", iv }, key, enc.encode(url))
  );
  return btoa(
    JSON.stringify({
      v: 1,
      s: encodeBase64(salt),
      i: encodeBase64(iv),
      c: encodeBase64(cipher)
    })
  );
}

async function decryptUrl(token, password) {
  const data = JSON.parse(atob(token));
  const salt = decodeBase64(data.s);
  const iv = decodeBase64(data.i);
  const cipher = decodeBase64(data.c);
  const key = await deriveAesKey(password, salt);
  const plain = await crypto.subtle.decrypt({ name: "AES-GCM", iv }, key, cipher);
  return dec.decode(plain);
}
