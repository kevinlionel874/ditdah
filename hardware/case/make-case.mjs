// Generates the DitDah case as two printable STL files: the base and the lid.
// No dependencies: node hardware/case/make-case.mjs  ->  hardware/case/ditdah-case-base.stl, ditdah-case-lid.stl
//
// Coordinates (mm): X to the user's right, Y toward the back, Z up. The user sits at the front (Y = 0).
// Positions in docs/case.md are measured from the left edge (x) and from the BACK edge (y); this
// script converts them. Each solid is built from closed boxes and "square tile with a round hole"
// pieces that touch but don't overlap; slicers merge touching pieces into one part.
// Change the numbers in P after measuring your parts, then run the script again.
// `node make-case.mjs --wide` makes a 180 mm wide box that also fits a full-size (830-hole) breadboard;
// the lid features and jacks move right by half the extra width so the layout stays centered.
import { writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const WIDE = process.argv.includes("--wide");
const P = {
  width: WIDE ? 180 : 130, // X, outside
  depth: 110,          // Y, outside
  height: 50,          // Z, outside, base only
  wall: 2,
  floor: 2,
  lidThickness: 2.5,   // the 30mm buttons' clips catch 3 mm below the cap, so the lid must be thinner than 3 mm
  lipHeight: 2,        // lip under the lid that presses into the base
  lipThickness: 1.5,
  lipGap: 0.3,         // clearance between lip and the inside of the walls, per side

  buttonHole: 24,                                 // 30mm buttons: the body below the cap measured 23 mm; +1 mm so it slides in
  buttons: [[25, 85], [65, 85], [105, 85]],       // [x from left, y from back]: Dit, Dah, Mode
  oledWindow: { w: 31, h: 17, x: 40, y: 25 },     // center, from left / from back
  buzzerSlots: { x: 100, y: 25, count: 3, length: 10, width: 1.6, pitch: 3.2 },

  jackHole: 6.5,                                  // 3.5mm jack barrel
  jacksX: [45, 65, 85],
  jackZ: 18,                                      // center height above the bottom
  usb: { w: 14, h: 12, fromBack: 31.5, z: 17 },   // left wall opening
  tube: { d: 5, fromBack: 40, z: 25 },            // right wall hole

  tileMargin: 1.5,     // plastic around each round hole inside its square tile
  segments: 96,        // smoothness of round holes (multiple of 8)
};

// ---------- geometry helpers ----------
const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

// A shell is a list of triangles, each with the direction its face should point (outward).
function newShell() { return []; }
function addTri(shell, a, b, c, outward) { shell.push({ p: [a, b, c], d: outward }); }
function addQuad(shell, a, b, c, d, outward) { addTri(shell, a, b, c, outward); addTri(shell, a, c, d, outward); }

function boxShell(x0, x1, y0, y1, z0, z1) {
  const s = newShell();
  const v = (x, y, z) => [x, y, z];
  addQuad(s, v(x0, y0, z0), v(x1, y0, z0), v(x1, y1, z0), v(x0, y1, z0), [0, 0, -1]);
  addQuad(s, v(x0, y0, z1), v(x1, y0, z1), v(x1, y1, z1), v(x0, y1, z1), [0, 0, 1]);
  addQuad(s, v(x0, y0, z0), v(x1, y0, z0), v(x1, y0, z1), v(x0, y0, z1), [0, -1, 0]);
  addQuad(s, v(x0, y1, z0), v(x1, y1, z0), v(x1, y1, z1), v(x0, y1, z1), [0, 1, 0]);
  addQuad(s, v(x0, y0, z0), v(x0, y1, z0), v(x0, y1, z1), v(x0, y0, z1), [-1, 0, 0]);
  addQuad(s, v(x1, y0, z0), v(x1, y1, z0), v(x1, y1, z1), v(x1, y0, z1), [1, 0, 0]);
  return s;
}

// Square tile (side `side`, centered at cu, cv) with a round hole of radius r, thickness t, in local (u, v, w).
function holeTileShell(cu, cv, r, side, t, segments) {
  const s = newShell();
  const half = side / 2;
  const inner = [];
  const outer = [];
  for (let i = 0; i < segments; i++) {
    const a = Math.PI / 4 + (2 * Math.PI * i) / segments; // corners land exactly on i = 0, n/4, n/2, 3n/4
    const c = Math.cos(a);
    const sn = Math.sin(a);
    const k = half / Math.max(Math.abs(c), Math.abs(sn));
    inner.push([cu + r * c, cv + r * sn]);
    outer.push([cu + k * c, cv + k * sn]);
  }
  for (let i = 0; i < segments; i++) {
    const j = (i + 1) % segments;
    const [i0, i1, o0, o1] = [inner[i], inner[j], outer[i], outer[j]];
    addQuad(s, [...i0, t], [...i1, t], [...o1, t], [...o0, t], [0, 0, 1]);
    addQuad(s, [...i0, 0], [...i1, 0], [...o1, 0], [...o0, 0], [0, 0, -1]);
    const mi = [(i0[0] + i1[0]) / 2 - cu, (i0[1] + i1[1]) / 2 - cv, 0];
    addQuad(s, [...i0, 0], [...i1, 0], [...i1, t], [...i0, t], [-mi[0], -mi[1], 0]); // hole wall faces the hole
    const mo = [(o0[0] + o1[0]) / 2 - cu, (o0[1] + o1[1]) / 2 - cv, 0];
    addQuad(s, [...o0, 0], [...o1, 0], [...o1, t], [...o0, t], mo);
  }
  return s;
}

// A plate [0,W] x [0,H] x [0,T] in local (u, v, w) with round and rectangular holes.
// holes: { kind: "circle", u, v, d } or { kind: "rect", u0, u1, v0, v1 }
function plateShells(W, H, T, holes) {
  const shells = [];
  const blocked = [];
  for (const h of holes) {
    if (h.kind === "circle") {
      const side = h.d + 2 * P.tileMargin;
      blocked.push([h.u - side / 2, h.u + side / 2, h.v - side / 2, h.v + side / 2]);
      shells.push(holeTileShell(h.u, h.v, h.d / 2, side, T, P.segments));
    } else {
      blocked.push([h.u0, h.u1, h.v0, h.v1]);
    }
  }
  for (const [u0, u1, v0, v1] of blocked) {
    if (u0 < 0 || u1 > W || v0 < 0 || v1 > H) throw new Error(`hole [${u0},${u1}]x[${v0},${v1}] is outside a ${W}x${H} plate`);
  }
  for (let i = 0; i < blocked.length; i++) {
    for (let j = i + 1; j < blocked.length; j++) {
      const a = blocked[i], b = blocked[j];
      if (a[0] < b[1] && b[0] < a[1] && a[2] < b[3] && b[2] < a[3]) throw new Error("two holes are too close together");
    }
  }
  // Fill everything else with boxes: split into vertical strips at every hole edge.
  const us = [...new Set([0, W, ...blocked.flatMap((b) => [b[0], b[1]])])].sort((a, b) => a - b);
  for (let k = 0; k + 1 < us.length; k++) {
    const ua = us[k], ub = us[k + 1];
    if (ub - ua < 1e-9) continue;
    const mid = (ua + ub) / 2;
    const cuts = blocked.filter((b) => b[0] < mid && mid < b[1]).map((b) => [b[2], b[3]]).sort((a, b) => a[0] - b[0]);
    let v = 0;
    for (const [c0, c1] of cuts) {
      if (c0 - v > 1e-9) shells.push(boxShell(ua, ub, v, c0, 0, T));
      v = Math.max(v, c1);
    }
    if (H - v > 1e-9) shells.push(boxShell(ua, ub, v, H, 0, T));
  }
  return shells;
}

// Map local (u, v, w) shells into the model with an affine map, keeping faces pointing outward.
function mapShells(shells, map, mapDir) {
  return shells.map((s) => s.map((t) => ({ p: t.p.map(map), d: mapDir(t.d) })));
}

// ---------- the parts ----------
const W = P.width, D = P.depth, H = P.height;
const yFromBack = (y) => D - y;
const SHIFT = (W - 130) / 2; // layout positions are written for the 130 mm box
P.buttons = P.buttons.map(([x, y]) => [x + SHIFT, y]);
P.oledWindow = { ...P.oledWindow, x: P.oledWindow.x + SHIFT };
P.buzzerSlots = { ...P.buzzerSlots, x: P.buzzerSlots.x + SHIFT };
P.jacksX = P.jacksX.map((x) => x + SHIFT);

function base() {
  const shells = [];
  const wallH = H - P.floor;
  shells.push(boxShell(0, W, 0, D, 0, P.floor));
  // front wall (Y = 0..wall), no holes
  shells.push(boxShell(0, W, 0, P.wall, P.floor, H));
  // back wall: u = X, v = Z - floor, w = Y - (D - wall)
  const jackHoles = P.jacksX.map((x) => ({ kind: "circle", u: x, v: P.jackZ - P.floor, d: P.jackHole }));
  shells.push(...mapShells(plateShells(W, wallH, P.wall, jackHoles),
    ([u, v, w]) => [u, D - P.wall + w, P.floor + v], ([a, b, c]) => [a, c, b]));
  // left wall: u = Y - wall, v = Z - floor, w = X
  const sideLen = D - 2 * P.wall;
  const usbY = yFromBack(P.usb.fromBack) - P.wall;
  const usbHole = { kind: "rect", u0: usbY - P.usb.w / 2, u1: usbY + P.usb.w / 2, v0: P.usb.z - P.floor - P.usb.h / 2, v1: P.usb.z - P.floor + P.usb.h / 2 };
  shells.push(...mapShells(plateShells(sideLen, wallH, P.wall, [usbHole]),
    ([u, v, w]) => [w, P.wall + u, P.floor + v], ([a, b, c]) => [c, a, b]));
  // right wall: u = Y - wall, v = Z - floor, w = X - (W - wall)
  const tubeHole = { kind: "circle", u: yFromBack(P.tube.fromBack) - P.wall, v: P.tube.z - P.floor, d: P.tube.d };
  shells.push(...mapShells(plateShells(sideLen, wallH, P.wall, [tubeHole]),
    ([u, v, w]) => [W - P.wall + w, P.wall + u, P.floor + v], ([a, b, c]) => [c, a, b]));
  return shells;
}

function lid() {
  const T = P.lidThickness;
  const holes = [];
  for (const [x, y] of P.buttons) holes.push({ kind: "circle", u: x, v: yFromBack(y), d: P.buttonHole });
  const o = P.oledWindow;
  holes.push({ kind: "rect", u0: o.x - o.w / 2, u1: o.x + o.w / 2, v0: yFromBack(o.y) - o.h / 2, v1: yFromBack(o.y) + o.h / 2 });
  const bz = P.buzzerSlots;
  for (let i = 0; i < bz.count; i++) {
    const cy = yFromBack(bz.y) + (i - (bz.count - 1) / 2) * bz.pitch;
    holes.push({ kind: "rect", u0: bz.x - bz.length / 2, u1: bz.x + bz.length / 2, v0: cy - bz.width / 2, v1: cy + bz.width / 2 });
  }
  const shells = plateShells(W, D, T, holes); // installed: Z = 0..T (top face at Z = T)
  // lip under the lid (installed Z = -lipHeight..0), a rectangular frame
  const a = P.wall + P.lipGap, b = a + P.lipThickness, z0 = -P.lipHeight;
  shells.push(boxShell(a, W - a, a, b, z0, 0));
  shells.push(boxShell(a, W - a, D - b, D - a, z0, 0));
  shells.push(boxShell(a, b, b, D - b, z0, 0));
  shells.push(boxShell(W - b, W - a, b, D - b, z0, 0));
  // print it top face down: rotate 180 degrees about X so the top face sits on the bed (Z = 0)
  return mapShells(shells, ([x, y, z]) => [x, D - y, T - z], ([dx, dy, dz]) => [dx, -dy, -dz]);
}

// ---------- checks and output ----------
function orient(shells) {
  return shells.map((s) => s.map(({ p: [a, b, c], d }) => (dot(cross(sub(b, a), sub(c, a)), d) < 0 ? [a, c, b] : [a, b, c])));
}

function check(name, shells) {
  let volume = 0;
  let badEdges = 0;
  const key = (v) => v.map((x) => x.toFixed(5)).join(",");
  for (const s of shells) {
    const edges = new Map();
    for (const [a, b, c] of s) {
      volume += dot(a, cross(b, c)) / 6;
      for (const [p, q] of [[a, b], [b, c], [c, a]]) {
        const k = key(p) + ">" + key(q);
        edges.set(k, (edges.get(k) || 0) + 1);
      }
    }
    for (const [k, n] of edges) {
      const [p, q] = k.split(">");
      if (n !== 1 || edges.get(q + ">" + p) !== 1) badEdges++;
    }
  }
  const tris = shells.reduce((n, s) => n + s.length, 0);
  const all = shells.flat(2);
  const lo = [0, 1, 2].map((i) => Math.min(...all.map((v) => v[i])));
  const hi = [0, 1, 2].map((i) => Math.max(...all.map((v) => v[i])));
  const size = hi.map((h, i) => (h - lo[i]).toFixed(1)).join(" x ");
  console.log(`${name}: ${shells.length} pieces, ${tris} triangles, ${(volume / 1000).toFixed(1)} cm3, size ${size} mm, open edges ${badEdges}`);
  if (badEdges) throw new Error(`${name} has ${badEdges} edges that are not closed`);
  return volume;
}

function writeStl(path, shells) {
  const tris = shells.flat();
  const buf = Buffer.alloc(84 + 50 * tris.length);
  buf.write("DitDah case, generated by make-case.mjs", 0, "ascii");
  buf.writeUInt32LE(tris.length, 80);
  let o = 84;
  for (const [a, b, c] of tris) {
    const n = cross(sub(b, a), sub(c, a));
    const len = Math.hypot(...n) || 1;
    for (const x of [n[0] / len, n[1] / len, n[2] / len, ...a, ...b, ...c]) { buf.writeFloatLE(x, o); o += 4; }
    buf.writeUInt16LE(0, o); o += 2;
  }
  writeFileSync(path, buf);
}

const here = dirname(fileURLToPath(import.meta.url));
const baseShells = orient(base());
const lidShells = orient(lid());
check("base", baseShells);
check("lid", lidShells);
const suffix = WIDE ? "-wide" : "";
writeStl(join(here, `ditdah-case-base${suffix}.stl`), baseShells);
writeStl(join(here, `ditdah-case-lid${suffix}.stl`), lidShells);
console.log(`Wrote ditdah-case-base${suffix}.stl and ditdah-case-lid${suffix}.stl`);
