const FILTER_ID = "glass-distortion";

function ensureGlassFilter(): void {
  if (document.getElementById(FILTER_ID)) {
    return;
  }

  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("aria-hidden", "true");
  svg.style.cssText = "position:absolute;width:0;height:0;overflow:hidden";
  svg.innerHTML = `
    <filter id="${FILTER_ID}" x="0%" y="0%" width="100%" height="100%" filterUnits="objectBoundingBox">
      <feTurbulence type="fractalNoise" baseFrequency="0.001 0.005" numOctaves="1" seed="17" result="turbulence"></feTurbulence>
      <feComponentTransfer in="turbulence" result="mapped">
        <feFuncR type="gamma" amplitude="1" exponent="10" offset="0.5"></feFuncR>
        <feFuncG type="gamma" amplitude="0" exponent="1" offset="0"></feFuncG>
        <feFuncB type="gamma" amplitude="0" exponent="1" offset="0.5"></feFuncB>
      </feComponentTransfer>
      <feGaussianBlur in="turbulence" stdDeviation="3" result="softMap"></feGaussianBlur>
      <feSpecularLighting in="softMap" surfaceScale="5" specularConstant="1" specularExponent="100" lighting-color="white" result="specLight">
        <fePointLight x="-200" y="-200" z="300"></fePointLight>
      </feSpecularLighting>
      <feComposite in="specLight" operator="arithmetic" k1="0" k2="1" k3="1" k4="0" result="litImage"></feComposite>
      <feDisplacementMap in="SourceGraphic" in2="softMap" scale="200" xChannelSelector="R" yChannelSelector="G"></feDisplacementMap>
    </filter>
  `;
  document.body.prepend(svg);
}

function layer(className: string): HTMLDivElement {
  const el = document.createElement("div");
  el.className = className;
  el.setAttribute("aria-hidden", "true");
  return el;
}

/**
 * Light-DOM liquid glass shell (suraj-xd/liquid-glass layer stack).
 * Usage: <liquid-glass class="p-6">…</liquid-glass>
 */
export class LiquidGlass extends HTMLElement {
  connectedCallback(): void {
    if (this.dataset.glassReady === "1") {
      return;
    }
    this.dataset.glassReady = "1";

    ensureGlassFilter();
    this.classList.add("glass-panel");

    const content = document.createElement("div");
    content.className = "glass-content relative z-30";
    while (this.firstChild) {
      content.appendChild(this.firstChild);
    }

    this.append(
      layer("glass-fx-blur absolute inset-0 z-0 overflow-hidden rounded-3xl"),
      layer("glass-fx-tint absolute inset-0 z-10"),
      layer("glass-fx-shine absolute inset-0 z-20 overflow-hidden rounded-3xl"),
      content,
    );
  }

  /** Slot for runtime text updates (e.g. empty state). */
  get contentEl(): HTMLElement | null {
    return this.querySelector(":scope > .glass-content");
  }
}

declare global {
  interface HTMLElementTagNameMap {
    "liquid-glass": LiquidGlass;
  }
}

if (!customElements.get("liquid-glass")) {
  customElements.define("liquid-glass", LiquidGlass);
}
