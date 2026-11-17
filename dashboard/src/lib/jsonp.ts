let jsonpSeq = 0;

export function fetchJsonp(url: string, timeoutMs = 15000): Promise<unknown> {
  return new Promise((resolve, reject) => {
    const cb = `sdcb${++jsonpSeq}`;
    const script = document.createElement("script");
    const timer = window.setTimeout(() => {
      cleanup();
      reject(new Error("timeout"));
    }, timeoutMs);

    function cleanup() {
      window.clearTimeout(timer);
      delete (window as unknown as Record<string, unknown>)[cb];
      if (script.parentNode) {
        script.parentNode.removeChild(script);
      }
    }

    (window as unknown as Record<string, unknown>)[cb] = (data: unknown) => {
      cleanup();
      resolve(data);
    };

    script.onerror = () => {
      cleanup();
      reject(new Error("network"));
    };

    const join = url.includes("?") ? "&" : "?";
    script.src = `${url}${join}callback=${cb}`;
    document.body.appendChild(script);
  });
}
