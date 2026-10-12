// Fuzz14 - the diode picker (current against voltage, the same models as the plugin) and the
// audio clips (only one plays at a time).

"use strict";

// Source/circuit/Diodes.h: saturation current (A), emission coefficient
const DIODES = [
  { name: "1N4148", kind: "silicon", is: 2.52e-9, n: 1.752 },
  { name: "1N914", kind: "silicon", is: 2.67e-9, n: 1.84 },
  { name: "1N4001", kind: "rectifier", is: 14.11e-9, n: 1.984 },
  { name: "1N34A", kind: "germanium", is: 2.0e-7, n: 1.3 },
  { name: "OA90", kind: "germanium", is: 4.0e-7, n: 1.6 },
  { name: "1N270", kind: "germanium", is: 1.2e-6, n: 1.6 },
  { name: "BAT41", kind: "Schottky", is: 1.0e-9, n: 1.25 },
  { name: "BAT85", kind: "Schottky", is: 2.0e-7, n: 1.04 },
  { name: "1N5817", kind: "Schottky", is: 1.0e-6, n: 1.1 },
  { name: "Infrared LED", kind: "LED", is: 1.0e-14, n: 1.7 },
  { name: "Red LED", kind: "LED", is: 1.0e-18, n: 1.9 },
  { name: "Yellow LED", kind: "LED", is: 1.0e-19, n: 1.9 },
  { name: "Green LED", kind: "LED", is: 1.0e-21, n: 1.9 },
  { name: "Blue LED", kind: "LED", is: 1.0e-28, n: 1.8 },
  { name: "White LED", kind: "LED", is: 1.0e-29, n: 1.8 },
  { name: "None", kind: "pulled out", is: 0, n: 1 },
];

const VT = 0.0258648;            // thermal voltage at 27 C, as in the solver
const V_MAX = 3.0, I_MAX = 2e-3; // the plot: 0..3 V, 0..2 mA

const amps = (d, v) => d.is * (Math.exp(Math.min(v / (d.n * VT), 80)) - 1);
const turnOn = (d) => (d.is > 0 ? d.n * VT * Math.log(1e-3 / d.is + 1) : null);   // volts at 1 mA

function note(d) {
  const vf = turnOn(d);
  if (vf === null) return "An empty hole: that spot does nothing.";
  if (vf > 1.0) return "Dark next to a silicon diode or in D3; in D1 and D2 together it carries the bias and lights up.";
  if (vf < 0.4) return "Turns on early: squashes harder, and leaks a little.";
  return d.name === "1N4148" ? "The original." : "Close to the original.";
}

function drawCurve(canvas, chosen) {
  const ctx = canvas.getContext("2d");
  const w = canvas.width, h = canvas.height;
  const pad = { l: 64, r: 28, t: 16, b: 40 };
  const x = (v) => pad.l + (v / V_MAX) * (w - pad.l - pad.r);
  const y = (i) => h - pad.b - (i / I_MAX) * (h - pad.t - pad.b);

  ctx.clearRect(0, 0, w, h);
  ctx.font = "13px 'IBM Plex Mono', Consolas, monospace";

  // grid and axes
  ctx.strokeStyle = "#e4e4df";
  ctx.lineWidth = 1;
  ctx.fillStyle = "#666";
  for (let v = 0; v <= V_MAX + 1e-9; v += 0.5) {
    ctx.beginPath(); ctx.moveTo(x(v), pad.t); ctx.lineTo(x(v), h - pad.b); ctx.stroke();
    ctx.textAlign = "center";
    ctx.fillText(`${v.toFixed(1)} V`, x(v), h - pad.b + 20);
  }
  for (let i = 0; i <= I_MAX + 1e-12; i += 0.5e-3) {
    ctx.beginPath(); ctx.moveTo(pad.l, y(i)); ctx.lineTo(w - pad.r, y(i)); ctx.stroke();
    ctx.textAlign = "right";
    ctx.fillText(`${(i * 1000).toFixed(1)} mA`, pad.l - 6, y(i) + 4);
  }

  const plot = (d, colour, width) => {
    ctx.strokeStyle = colour;
    ctx.lineWidth = width;
    ctx.beginPath();
    for (let px = 0; px <= w - pad.l - pad.r; px++) {
      const v = (px / (w - pad.l - pad.r)) * V_MAX;
      const i = Math.min(amps(d, v), I_MAX * 1.05);
      if (px === 0) ctx.moveTo(x(v), y(i)); else ctx.lineTo(x(v), y(i));
      if (i >= I_MAX * 1.05) break;
    }
    ctx.stroke();
  };

  plot(DIODES[0], "#1d1d1f", 2);
  if (chosen !== DIODES[0]) plot(chosen, "#c0392b", 3);

  // the window the circuit actually works in: under about 1 V across a diode
  ctx.fillStyle = "rgba(192, 57, 43, 0.07)";
  ctx.fillRect(x(0), pad.t, x(1.0) - x(0), h - pad.t - pad.b);
  ctx.fillStyle = "#c0392b";
  ctx.textAlign = "left";
  ctx.fillText("where Fuzz14 lives", x(0.05), pad.t + 16);
  ctx.fillStyle = "#1d1d1f";
  ctx.fillText("black: 1N4148", x(1.6), pad.t + 16);
}

function setUpDiodes() {
  const pick = document.getElementById("diode-pick");
  const canvas = document.getElementById("diode-curve");
  if (!pick || !canvas) return;

  let kind = "";
  let group = null;
  DIODES.forEach((d, index) => {
    if (d.kind !== kind) {
      kind = d.kind;
      group = document.createElement("optgroup");
      group.label = kind;
      pick.appendChild(group);
    }
    const option = document.createElement("option");
    option.value = String(index);
    option.textContent = d.name;
    group.appendChild(option);
  });
  pick.value = "3";                       // start on a germanium one: the difference shows

  const show = () => {
    const d = DIODES[Number(pick.value)];
    const vf = turnOn(d);
    document.getElementById("diode-kind").textContent = d.kind;
    document.getElementById("diode-vf").textContent = vf === null ? "never" : `${vf.toFixed(2)} V (at 1 mA)`;
    document.getElementById("diode-note").textContent = note(d);
    drawCurve(canvas, d);
  };
  pick.addEventListener("change", show);
  show();
}

function setUpClips() {
  const players = document.querySelectorAll(".clips audio");
  players.forEach((audio) => {
    audio.addEventListener("play", () => {
      players.forEach((other) => { if (other !== audio) other.pause(); });
      audio.closest("figure").classList.add("playing");
    });
    const stop = () => audio.closest("figure").classList.remove("playing");
    audio.addEventListener("pause", stop);
    audio.addEventListener("ended", stop);
  });
}

setUpDiodes();
setUpClips();
