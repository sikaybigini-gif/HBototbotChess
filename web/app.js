const app = document.getElementById("app");

const storageKeys = {
  lobby: "nobet-lobby",
  player: "nobet-player",
};

let state = null;
let lastEvent = -1;
let sceneTransitionAt = 0;
let scenePointer = { x: 0, y: 0 };
let sceneCamera = { x: 0, y: 0 };
let polling = false;
let sending = false;

const kindLabels = {
  lobby: "Giriş holü",
  corridor: "Uzun koridor",
  guest: "Misafir odası",
  archive: "Arşiv",
  workshop: "Bakım odası",
  infirmary: "Revir",
  elevator: "Servis asansörü",
};

const roleLabels = {
  scout: "KEŞİFÇİ",
  medic: "SAĞLIKÇI",
  engineer: "MÜHENDİS",
  guardian: "MUHAFIZ",
};

const roleDescriptions = {
  scout: "Yankısız kapılarda ilk tuzağı hasarsız fark eder.",
  medic: "Canlandırmaları güçlendirir, revire ekibe yardım eder.",
  engineer: "Maymuncuk kullanınca onu tüketmez.",
  guardian: "Tehditlerden daha az hasar alır.",
};

const itemLabels = {
  key: "Pirinç anahtar",
  lockpick: "Maymuncuk",
  bandage: "Bandaj",
  tonic: "Sakinleştirici",
  lighter: "Çakmak",
  adrenaline: "Adrenalin",
  fuse: "Sigorta",
  coin: "Jeton",
};

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function padDoor(number) {
  return String(number).padStart(2, "0");
}

function bar(value) {
  const safe = Math.max(0, Math.min(100, Number(value) || 0));
  return `<div class="meter"><i style="width:${safe}%"></i></div><b>${safe}</b>`;
}

function formBody(values) {
  const body = new URLSearchParams();
  Object.entries(values).forEach(([key, value]) => body.set(key, value ?? ""));
  return body;
}

async function post(path, values) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
    body: formBody(values),
  });
  const data = await response.json();
  if (!response.ok || data.ok === false) {
    throw new Error(data.error || "Sunucu isteği başarısız.");
  }
  return data;
}

async function getState() {
  if (!state) return;
  const lobby = encodeURIComponent(state.lobby);
  const player = encodeURIComponent(state.me);
  const response = await fetch(`/api/state?lobby=${lobby}&player=${player}`, { cache: "no-store" });
  const data = await response.json();
  if (!response.ok || data.ok === false) {
    throw new Error(data.error || "Lobi bağlantısı kesildi.");
  }
  return data;
}

function playCue(next) {
  if (!next.sound || next.eventSeq === lastEvent) return;
  lastEvent = next.eventSeq;
  const audio = new Audio(`/assets/sfx/${encodeURIComponent(next.sound)}`);
  audio.volume = next.threat !== "none" || next.phase === "chase" ? 0.65 : 0.42;
  audio.play().catch(() => {
    // Mobile browsers require the first audio to follow a tap. The controls
    // remain usable even when the browser blocks autoplay.
  });
  if (next.threat !== "none" || next.phase === "chase") {
    if (navigator.vibrate) navigator.vibrate([90, 60, 140]);
  }
}

function setState(next) {
  if (!next) return;
  if (state && next.door !== state.door) sceneTransitionAt = performance.now();
  playCue(next);
  const changed = !state || JSON.stringify(state) !== JSON.stringify(next);
  state = next;
  localStorage.setItem(storageKeys.lobby, next.lobby || "");
  localStorage.setItem(storageKeys.player, next.me || "");
  // Do not replace the DOM on identical polls; otherwise a player typing a
  // code on a phone would lose their input every 700ms.
  if (changed) render();
}

function roomArt(room, phase) {
  const dark = room.dark && !room.lit;
  if (dark) {
    return [
      "      . . . . . . . . . . . . . . . . . . .",
      "     /                                       \\",
      "    |               [  ?  ]                  |",
      "    |            .-----------.                |",
      "    |            |     ?     |                |",
      "    |            '-----------'                |",
      "    |       . . . . . . . . . . . .          |",
    ].join("\n");
  }
  const art = {
    lobby: [
      "      .------------------------------------.",
      "     /       o                 o            \\",
      "    |       /|\\               /|\\           |",
      "    |    .--------------------------------.    |",
      "    |    |          RESEPSİYON           |    |",
      "    |    '--------------------------------'    |",
      "    '------------------------------------------'",
    ],
    corridor: [
      "      .------------------------------------.",
      "     /       *       *       *       *        \\",
      "    |          .-----------------.            |",
      "    |          |                 |            |",
      "    |          |    İLERİ        |            |",
      "    |          |                 |            |",
      "    '----------'-----------------'------------'",
    ],
    guest: [
      "      .------------------------------------.",
      "     |      .--------.             _______    |",
      "     |      | pencere|            /       \\   |",
      "     |      '--------'           /  YATAK  \\  |",
      "     |                         '-----------'  |",
      "     |       .---------.                       |",
      "     '-------|  DOLAP  |----------------------'",
    ],
    archive: [
      "      .------------------------------------.",
      "     |  [====] [====] [====] [====]         |",
      "     |  [====] [====] [====] [====]         |",
      "     |  [====] [====] [====] [====]         |",
      "     |       o                 o             |",
      "     |             .---------.               |",
      "     '-------------|  MASA   |---------------'",
    ],
    workshop: [
      "      .------------------------------------.",
      "     |  ===\\        ______        /===      |",
      "     |      \\______/      \\______/          |",
      "     |        |       ⚙       |              |",
      "     |        '---------------'              |",
      "     |       _/|    ALET    |\\_             |",
      "     '------'------------------'-------------'",
    ],
    infirmary: [
      "      .------------------------------------.",
      "     |          +       +                   |",
      "     |          |       |       .---.       |",
      "     |      .---+-------+---.   | o |       |",
      "     |      |    MUAYENE   |   '---'       |",
      "     |      '---------------'               |",
      "     '---------------------------------------'",
    ],
    elevator: [
      "      .------------------------------------.",
      "     |              SERVİS                  |",
      "     |          .------------.              |",
      "     |          |  ASANSÖR   |              |",
      "     |          |      >     |              |",
      "     |          '------------'              |",
      "     '---------------------------------------'",
    ],
  };
  return (art[room.kind] || art.corridor).join("\n");
}

function roundedBox(ctx, x, y, width, height, radius) {
  const r = Math.min(radius, width / 2, height / 2);
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.lineTo(x + width - r, y);
  ctx.quadraticCurveTo(x + width, y, x + width, y + r);
  ctx.lineTo(x + width, y + height - r);
  ctx.quadraticCurveTo(x + width, y + height, x + width - r, y + height);
  ctx.lineTo(x + r, y + height);
  ctx.quadraticCurveTo(x, y + height, x, y + height - r);
  ctx.lineTo(x, y + r);
  ctx.quadraticCurveTo(x, y, x + r, y);
  ctx.closePath();
}

function drawScene(canvas, now) {
  if (!state || !state.room || !canvas) return;
  const room = state.room;
  const bounds = canvas.getBoundingClientRect();
  const width = Math.max(1, Math.floor(bounds.width));
  const height = Math.max(1, Math.floor(bounds.height));
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  const pixelWidth = Math.max(1, Math.floor(width * dpr));
  const pixelHeight = Math.max(1, Math.floor(height * dpr));
  if (canvas.width !== pixelWidth || canvas.height !== pixelHeight) {
    canvas.width = pixelWidth;
    canvas.height = pixelHeight;
  }
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, width, height);

  const dark = room.dark && !room.lit;
  const danger = state.threat !== "none";
  const chase = state.phase === "chase";
  const reducedMotion = window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const motion = reducedMotion ? 0 : 1;
  const flicker = dark ? 0.42 + Math.abs(Math.sin(now * 0.014)) * 0.25 : 1;
  const bob = (chase ? Math.sin(now * 0.018) * 3 : Math.sin(now * 0.0017) * 0.7) * motion;
  sceneCamera.x += (scenePointer.x - sceneCamera.x) * 0.08;
  sceneCamera.y += (scenePointer.y - sceneCamera.y) * 0.08;
  const shakeX = chase ? Math.sin(now * 0.061) * 3.4 * motion : 0;
  const shakeY = chase ? Math.cos(now * 0.073) * 2.2 * motion : 0;
  ctx.save();
  ctx.translate(sceneCamera.x * width * 0.018 + shakeX, sceneCamera.y * height * 0.014 + shakeY);

  const sky = ctx.createLinearGradient(0, 0, 0, height * .58);
  sky.addColorStop(0, dark ? "#05050c" : "#111b29");
  sky.addColorStop(1, dark ? "#11101c" : "#27364a");
  ctx.fillStyle = sky;
  ctx.fillRect(0, 0, width, height);

  // First-person hotel geometry: ceiling, converging walls and a deep floor.
  ctx.fillStyle = dark ? "#090914" : "#172332";
  ctx.beginPath();
  ctx.moveTo(0, height * .12);
  ctx.lineTo(width, height * .12);
  ctx.lineTo(width * .72, height * .52);
  ctx.lineTo(width * .28, height * .52);
  ctx.closePath();
  ctx.fill();

  ctx.fillStyle = dark ? "#11101b" : "#35404c";
  ctx.beginPath();
  ctx.moveTo(0, height * .12);
  ctx.lineTo(width * .28, height * .52);
  ctx.lineTo(width * .32, height);
  ctx.lineTo(0, height);
  ctx.closePath();
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(width, height * .12);
  ctx.lineTo(width * .72, height * .52);
  ctx.lineTo(width * .68, height);
  ctx.lineTo(width, height);
  ctx.closePath();
  ctx.fill();

  const floor = ctx.createLinearGradient(0, height * .5, 0, height);
  floor.addColorStop(0, dark ? "#171522" : "#443b42");
  floor.addColorStop(1, dark ? "#05060b" : "#14151e");
  ctx.fillStyle = floor;
  ctx.beginPath();
  ctx.moveTo(width * .28, height * .52);
  ctx.lineTo(width * .72, height * .52);
  ctx.lineTo(width, height);
  ctx.lineTo(0, height);
  ctx.closePath();
  ctx.fill();

  // Carpet seams and perspective wall trim.
  ctx.lineWidth = 1;
  ctx.strokeStyle = dark ? "rgba(117, 84, 128, .28)" : "rgba(177, 188, 201, .25)";
  for (let index = 1; index < 7; index += 1) {
    const t = index / 7;
    const y = height * (.53 + t * .47);
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }
  ctx.strokeStyle = dark ? "rgba(130, 88, 148, .24)" : "rgba(214, 197, 158, .22)";
  ctx.beginPath();
  ctx.moveTo(0, height * .48);
  ctx.lineTo(width * .28, height * .52);
  ctx.lineTo(width * .32, height);
  ctx.moveTo(width, height * .48);
  ctx.lineTo(width * .72, height * .52);
  ctx.lineTo(width * .68, height);
  ctx.stroke();

  // Ceiling lamps recede toward the door.
  const lamps = [
    [width * .16, height * .2, 19],
    [width * .84, height * .2, 19],
    [width * .32, height * .3, 13],
    [width * .68, height * .3, 13],
    [width * .5, height * .38, 8],
  ];
  lamps.forEach(([x, y, radius], index) => {
    const power = dark ? flicker * (index === lamps.length - 1 ? .4 : .18) : .9;
    ctx.fillStyle = `rgba(239, 211, 151, ${power})`;
    ctx.shadowColor = dark ? "rgba(163, 105, 196, .3)" : "rgba(246, 216, 154, .7)";
    ctx.shadowBlur = dark ? radius * 1.4 : radius * 2.2;
    ctx.beginPath();
    ctx.ellipse(x, y + bob * .2, radius, radius * .27, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
  });

  // A small piece of room dressing changes with the procedural room type.
  ctx.strokeStyle = dark ? "rgba(103, 69, 121, .35)" : "rgba(206, 181, 150, .32)";
  ctx.lineWidth = 3;
  if (room.kind === "archive") {
    for (let index = 0; index < 4; index += 1) {
      const x = width * (.06 + index * .07);
      ctx.strokeRect(x, height * (.3 + index * .015), width * .045, height * .2);
      ctx.strokeRect(width - x - width * .045, height * (.3 + index * .015), width * .045, height * .2);
    }
  } else if (room.kind === "guest") {
    ctx.strokeRect(width * .03, height * .57, width * .16, height * .16);
    ctx.strokeRect(width * .81, height * .55, width * .15, height * .18);
  } else if (room.kind === "workshop") {
    ctx.beginPath();
    ctx.moveTo(width * .04, height * .22);
    ctx.bezierCurveTo(width * .15, height * .37, width * .1, height * .43, width * .22, height * .53);
    ctx.moveTo(width * .96, height * .22);
    ctx.bezierCurveTo(width * .85, height * .37, width * .9, height * .43, width * .78, height * .53);
    ctx.stroke();
  } else if (room.kind === "infirmary") {
    ctx.strokeStyle = dark ? "rgba(170, 74, 102, .38)" : "rgba(225, 103, 112, .62)";
    ctx.lineWidth = 5;
    ctx.beginPath();
    ctx.moveTo(width * .09, height * .4);
    ctx.lineTo(width * .18, height * .4);
    ctx.moveTo(width * .135, height * .35);
    ctx.lineTo(width * .135, height * .45);
    ctx.stroke();
  }

  // The numbered door is the visual anchor of the run.
  const doorWidth = Math.min(width * .42, 330);
  const doorHeight = Math.min(height * .48, 285);
  const doorX = (width - doorWidth) / 2;
  const doorY = height * .23 + bob;
  const doorGradient = ctx.createLinearGradient(doorX, doorY, doorX + doorWidth, doorY + doorHeight);
  doorGradient.addColorStop(0, dark ? "#171320" : room.kind === "elevator" ? "#245650" : "#243444");
  doorGradient.addColorStop(.5, dark ? "#0b0b13" : "#121c29");
  doorGradient.addColorStop(1, dark ? "#1e1428" : "#2b3947");
  ctx.fillStyle = doorGradient;
  ctx.strokeStyle = danger || chase ? "rgba(255, 88, 105, .9)" : dark ? "rgba(145, 94, 164, .7)" : "rgba(116, 210, 216, .65)";
  ctx.lineWidth = danger || chase ? 3 : 2;
  roundedBox(ctx, doorX, doorY, doorWidth, doorHeight, 5);
  ctx.fill();
  ctx.stroke();
  ctx.fillStyle = "rgba(255,255,255,.035)";
  ctx.fillRect(doorX + doorWidth * .08, doorY + doorHeight * .09, doorWidth * .35, doorHeight * .75);
  ctx.fillRect(doorX + doorWidth * .57, doorY + doorHeight * .09, doorWidth * .35, doorHeight * .75);
  ctx.strokeStyle = "rgba(206, 222, 231, .15)";
  ctx.lineWidth = 1;
  ctx.strokeRect(doorX + doorWidth * .09, doorY + doorHeight * .1, doorWidth * .34, doorHeight * .72);
  ctx.strokeRect(doorX + doorWidth * .57, doorY + doorHeight * .1, doorWidth * .34, doorHeight * .72);

  const plateText = room.number === state.targetDoors ? "EXIT" : `DOOR ${String(room.number).padStart(2, "0")}`;
  ctx.fillStyle = room.number === state.targetDoors ? "#72e2a1" : danger || chase ? "#ff6976" : "#f2d58e";
  ctx.font = `800 ${Math.max(11, Math.min(18, width * .028))}px ui-monospace, monospace`;
  ctx.textAlign = "center";
  ctx.shadowColor = ctx.fillStyle;
  ctx.shadowBlur = 12;
  ctx.fillText(plateText, width / 2, doorY - 15);
  ctx.shadowBlur = 0;

  const knobX = doorX + doorWidth * .52;
  const knobY = doorY + doorHeight * .57;
  ctx.fillStyle = room.locked ? "#e9ad59" : "#8fd9d3";
  ctx.beginPath();
  ctx.arc(knobX, knobY, Math.max(4, width * .009), 0, Math.PI * 2);
  ctx.fill();
  ctx.strokeStyle = "rgba(0,0,0,.55)";
  ctx.stroke();

  if (room.puzzle !== "none" && !room.puzzleSolved) {
    ctx.fillStyle = "#f6c96b";
    ctx.fillRect(doorX + doorWidth * .39, doorY + doorHeight * .35, doorWidth * .22, doorHeight * .15);
    ctx.fillStyle = "#3b2d16";
    ctx.font = "700 11px ui-monospace, monospace";
    ctx.fillText(room.puzzle === "fuse" ? "FUSE" : "CODE", width / 2, doorY + doorHeight * .445);
  }

  if (room.teamDoor) {
    ctx.fillStyle = "rgba(100, 217, 220, .85)";
    ctx.font = "700 10px ui-monospace, monospace";
    ctx.fillText("TEAM", width / 2, doorY + doorHeight + 18);
  }

  // Threat silhouette and red vignette are deliberately abstract/original.
  if (danger || chase) {
    const shadowX = chase ? width * (.5 + Math.sin(now * .012) * .08) : width * .5;
    const shadowY = height * .54 + bob;
    const shadowScale = chase ? 1.15 : .8;
    ctx.fillStyle = chase ? "rgba(7, 6, 12, .86)" : "rgba(9, 5, 14, .7)";
    ctx.beginPath();
    ctx.ellipse(shadowX, shadowY - height * .14, width * .065 * shadowScale, height * .11 * shadowScale, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.moveTo(shadowX - width * .11 * shadowScale, shadowY + height * .2);
    ctx.quadraticCurveTo(shadowX, shadowY - height * .03, shadowX + width * .11 * shadowScale, shadowY + height * .2);
    ctx.lineTo(shadowX + width * .07, height);
    ctx.lineTo(shadowX - width * .07, height);
    ctx.closePath();
    ctx.fill();
    ctx.fillStyle = "#ff6670";
    ctx.shadowColor = "#ff334d";
    ctx.shadowBlur = 18;
    ctx.beginPath();
    ctx.ellipse(shadowX - width * .022, shadowY - height * .15, Math.max(2, width * .007), Math.max(2, height * .009), 0, 0, Math.PI * 2);
    ctx.ellipse(shadowX + width * .022, shadowY - height * .15, Math.max(2, width * .007), Math.max(2, height * .009), 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
  }

  const vignette = ctx.createRadialGradient(width / 2, height * .48, height * .1, width / 2, height * .5, Math.max(width, height) * .72);
  vignette.addColorStop(0, "rgba(0,0,0,0)");
  vignette.addColorStop(1, danger || chase ? "rgba(88, 4, 21, .63)" : "rgba(0,0,0,.62)");
  ctx.fillStyle = vignette;
  ctx.fillRect(0, 0, width, height);
  if (dark) {
    ctx.fillStyle = `rgba(4, 3, 10, ${.25 + (1 - flicker) * .42})`;
    ctx.fillRect(0, 0, width, height);
  }
  if (sceneTransitionAt) {
    const progress = (now - sceneTransitionAt) / 620;
    if (progress < 1) {
      ctx.fillStyle = `rgba(0, 0, 0, ${Math.max(0, 1 - progress)})`;
      ctx.fillRect(0, 0, width, height);
    } else {
      sceneTransitionAt = 0;
    }
  }
  ctx.restore();
}

let sceneLoopStarted = false;
function startSceneLoop() {
  if (sceneLoopStarted) return;
  sceneLoopStarted = true;
  const loop = (now) => {
    const canvas = document.getElementById("scene-canvas");
    if (canvas && state) drawScene(canvas, now);
    window.requestAnimationFrame(loop);
  };
  window.requestAnimationFrame(loop);
}

function mapMarkup() {
  const start = Math.max(1, state.door - 5);
  const end = Math.min(state.targetDoors, state.door + 5);
  let output = "";
  for (let number = start; number <= end; number += 1) {
    const playerHere = number === state.door;
    const visited = number < state.door;
    output += `<span class="map-node ${playerHere ? "current" : visited ? "visited" : "unknown"}">${visited || playerHere ? padDoor(number) : "??"}</span>`;
    if (number !== end) output += `<span class="map-link">—</span>`;
  }
  return output;
}

function playersMarkup() {
  return state.players.map((player) => {
    const items = Object.entries(player.items || {})
      .filter(([, amount]) => amount > 0)
      .map(([item, amount]) => `${escapeHtml(itemLabels[item] || item)} x${amount}`)
      .join(", ");
    const status = !player.alive ? "KAYIP" : player.hidden ? "DOLAPTA" : player.ready ? "HAZIR" : "BEKLİYOR";
    const revive = state && state.phase === "explore" && !player.alive && player.id !== state.me
      ? `<button class="revive-button" data-action="revive" data-value="${escapeHtml(player.id)}">CANLANDIR</button>`
      : "";
    return `<article class="player-card ${player.alive ? "" : "dead"}">
      <div class="player-line"><strong>${escapeHtml(player.name)}</strong>${player.host ? '<span class="host-mark">HOST</span>' : ""}</div>
      <small class="role-tag" title="${escapeHtml(roleDescriptions[player.role] || "")}">${escapeHtml(player.roleText || roleLabels[player.role] || player.role || "KEŞİFÇİ")}</small>
      <small>${status}</small>
      <div class="mini-stat"><span>CAN</span>${bar(player.health)}</div>
      <div class="mini-stat"><span>AKIL</span>${bar(player.sanity)}</div>
      <small class="inventory-line">${escapeHtml(items || "Eşyasız")}</small>
      ${revive}
    </article>`;
  }).join("");
}

function renderJoin(error = "") {
  app.innerHTML = `<section class="join-layout">
    <div class="join-hero">
      <div class="eyebrow">NÖBET // CO-OP KORKU MACERASI</div>
      <h1>UZUN<br><span>KORİDOR</span></h1>
      <p class="hero-copy">Bir bilgisayar sunucu olsun. Arkadaşların telefondan aynı asansöre binsin. Kapılar açıldığında ekip birlikte karar versin.</p>
      <div class="hero-door" aria-hidden="true"><i></i><b>EXIT?</b></div>
      <p class="legal-note">Özgün bir C++ oyunu — Roblox veya başka bir oyunun kopyası değildir.</p>
    </div>
    <section class="panel join-panel">
      <div class="panel-kicker">NÖBETE KATIL</div>
      <h2>Asansör kartını al</h2>
      <p>Yeni bir lobi kur veya arkadaşının verdiği dört karakterli kodu gir.</p>
      <form id="join-form" class="stack-form">
        <label>İsmin<input name="name" maxlength="20" placeholder="Gezgin" required autocomplete="nickname"></label>
        <label>Lobi kodu <span>(boş bırakırsan yeni lobi)</span><input name="code" maxlength="4" placeholder="AB7K" autocapitalize="characters" spellcheck="false"></label>
        <label>Kapı sayısı <span>(yalnızca yeni lobide)</span><select name="doors"><option value="20">20 — kısa vardiya</option><option value="30">30 — standart</option><option value="40">40 — uzun gece</option></select></label>
        <button class="primary wide" type="submit">ASANSÖRE BİN <span>→</span></button>
      </form>
      ${error ? `<div class="error-box">${escapeHtml(error)}</div>` : ""}
      <div class="network-hint"><span class="pulse-dot"></span> Aynı Wi-Fi’daki arkadaşların sunucunun adresini telefon tarayıcısına yazabilir.</div>
    </section>
  </section>`;
}

function renderLobby() {
  const me = state.players.find((player) => player.id === state.me);
  const readyCount = state.players.filter((player) => player.ready).length;
  const isHost = state.host === state.me;
  app.innerHTML = `<section class="lobby-layout">
    <header class="topbar">
      <div class="brand"><span class="brand-mark">N</span> NÖBET <em>CO-OP</em></div>
      <button class="icon-button" data-action="leave_lobby" title="Lobiden ayrıl">ÇIK</button>
    </header>
    <div class="lobby-code panel">
      <div><div class="panel-kicker">ASANSÖR KODU</div><strong>${escapeHtml(state.lobby)}</strong><p>Arkadaşların bu kodu girsin.</p></div>
      <button class="copy-button" data-copy="${escapeHtml(state.lobby)}">KODU KOPYALA</button>
    </div>
    <div class="lobby-main">
      <section class="panel elevator-panel">
        <div class="elevator-lights"><i></i><i></i><i></i></div>
        <div class="elevator-visual"><div class="elevator-door left"></div><div class="elevator-door right"></div><span>MAX 8</span></div>
        <p class="panel-kicker">GRUP ASANSÖRÜ</p>
        <h2>${readyCount}/${state.players.length} kişi hazır</h2>
        <p>${isHost ? "Herkes hazır olduğunda asansörü sen başlatabilirsin." : "Host asansörü çalıştırana kadar burada bekle."}</p>
        <div class="lobby-actions">
          <button class="primary" data-action="ready">${me && me.ready ? "HAZIRLIĞI İPTAL ET" : "HAZIRIM"}</button>
          ${isHost ? `<button class="secondary" data-action="start" ${readyCount === state.players.length ? "" : "disabled"}>ASANSÖRÜ BAŞLAT</button>` : ""}
        </div>
      </section>
      <section class="panel roster-panel">
        <div class="panel-heading"><div><div class="panel-kicker">KABİNDEKİLER</div><h3>Ekibin</h3></div><span class="count-pill">${state.players.length}/8</span></div>
        <div class="player-grid">${playersMarkup()}</div>
        <label class="role-picker">Kendi rolün<select id="role-select"><option value="scout" ${me && me.role === "scout" ? "selected" : ""}>Keşifçi</option><option value="medic" ${me && me.role === "medic" ? "selected" : ""}>Sağlıkçı</option><option value="engineer" ${me && me.role === "engineer" ? "selected" : ""}>Mühendis</option><option value="guardian" ${me && me.role === "guardian" ? "selected" : ""}>Muhafız</option></select><span id="role-description">${escapeHtml(roleDescriptions[(me && me.role) || "scout"])}</span></label>
      </section>
    </div>
    <section class="log-panel panel"><div class="panel-kicker">KABİN KAYDI</div>${state.log.slice(-5).map((line) => `<p>${escapeHtml(line)}</p>`).join("")}<form class="chat-form" data-form="chat"><input name="value" maxlength="180" placeholder="Ekibe mesaj yaz..." autocomplete="off"><button class="secondary" type="submit">GÖNDER</button></form></section>
  </section>`;
}

function actionButton(action, label, value = "", extra = "") {
  return `<button class="action-button ${extra}" data-action="${action}" data-value="${escapeHtml(value)}">${label}</button>`;
}

function renderChase() {
  const next = state.nextMove;
  const labels = { left: "SOL", right: "SAĞ", jump: "ATLA", duck: "EĞİL" };
  return `<div class="chase-panel">
    <div class="danger-icon">!</div>
    <div><span class="panel-kicker">KAÇIŞ ${state.chaseStep + 1}/${state.chaseTotal}</span><h2>ARKANA BAKMA</h2><p>Ekip birlikte hareket ediyor. Sıradaki hamle:</p></div>
    <strong class="move-call">${labels[next] || "?"}</strong>
    <div class="chase-actions">${actionButton("move", "SOL", "left", next === "left" ? "recommended" : "")}${actionButton("move", "SAĞ", "right", next === "right" ? "recommended" : "")}${actionButton("move", "ATLA", "jump", next === "jump" ? "recommended" : "")}${actionButton("move", "EĞİL", "duck", next === "duck" ? "recommended" : "")}</div>
  </div>`;
}

function normalActions(room, me) {
  if (!me || !me.alive) return `<div class="empty-actions">Bu oyuncu artık hareket edemiyor. Ekibin devam edebilir.</div>`;
  const items = me.items || {};
  const openLabel = room.number === state.targetDoors ? "ASANSÖRÜ AÇ" : room.teamDoor ? "EKİPÇE AÇ" : "KAPIYI AÇ";
  let html = `<div class="action-row primary-actions">${actionButton("search", "ARA", "", room.lootAvailable ? "attention" : "")}${actionButton("listen", "DİNLE")}${actionButton("open", openLabel, "", "main-action")}</div>`;
  html += `<div class="action-row">${actionButton(me.hidden ? "leave_hide" : "hide", me.hidden ? "DOLAPTAN ÇIK" : "SAKLAN", "", me.hidden ? "safe" : "")}${actionButton("back", "GERİ")}${actionButton("rest", "DİNLEN")}</div>`;
  html += `<div class="action-row item-actions">`;
  if (items.lighter) html += actionButton("use", "ÇAKMAK", "lighter");
  if (items.bandage) html += actionButton("use", "BANDAJ", "bandage");
  if (items.tonic) html += actionButton("use", "TONİK", "tonic");
  if (items.lockpick) html += actionButton("use", "MAYMUNCUK", "lockpick");
  if (items.fuse) html += actionButton("use", "SİGORTA", "fuse");
  html += `</div>`;
  if (room.puzzle === "number" && !room.puzzleSolved) {
    html += `<form class="inline-form" data-form="solve"><input name="value" inputmode="numeric" maxlength="3" placeholder="Kod" aria-label="Üç haneli kod" required><button class="secondary" type="submit">KODU GİR</button></form>`;
  }
  if (room.hasShop) {
    html += `<div class="shop-row"><span>SERVİS ARABASI</span><select id="buy-item"><option value="bandage">Bandaj · 3</option><option value="tonic">Tonik · 4</option><option value="lockpick">Maymuncuk · 5</option><option value="fuse">Sigorta · 2</option></select>${actionButton("buy", "SATIN AL")}</div>`;
  }
  return html;
}

function renderGame() {
  const room = state.room;
  const me = state.players.find((player) => player.id === state.me);
  const won = state.phase === "won";
  const dead = state.phase === "dead";
  const phaseBanner = state.phase === "chase" ? renderChase() : "";
  const threat = state.threat !== "none" ? `<div class="threat-banner"><span>!</span><div><strong>${escapeHtml(state.threatText)}</strong><small>${state.threatTurns} hamle içinde herkes saklanmalı.</small></div></div>` : "";
  const puzzle = room.puzzle !== "none" && !room.puzzleSolved ? `<div class="objective-banner"><span>⚙</span><div><strong>${room.puzzle === "fuse" ? "Sigorta paneli" : "Sayı kilidi"}</strong><small>${room.puzzle === "fuse" ? "Bir sigorta kullan." : room.searched ? `İpucu: ${escapeHtml(room.clue)}` : "Odayı ara; kodu bul."}</small></div></div>` : "";
  const aliveCount = state.players.filter((player) => player.alive).length;
  const teamDoor = room.teamDoor && aliveCount > 1 ? `<div class="objective-banner team-objective"><span>2</span><div><strong>Ekip kapısı</strong><small>${room.openVotes}/2 oyuncu kapıyı tuttu. İkinci oyuncu da KAPIYI AÇ düğmesine bassın.</small></div></div>` : "";
  const end = won || dead ? `<div class="end-overlay ${won ? "won" : "dead"}"><div class="end-symbol">${won ? "✓" : "×"}</div><div class="panel-kicker">${won ? "VARDİYA TAMAMLANDI" : "SİNYAL KESİLDİ"}</div><h2>${won ? "Asansörden çıktınız." : "Koridor ekibi yuttu."}</h2><p>${won ? "Bu binanın kapısı bir daha aynı yerde olmayacak." : "Bir sonraki ekip daha dikkatli olmalı."}</p><button class="primary" data-action="reset">YENİ LOBİ</button></div>` : "";
  app.innerHTML = `<section class="game-layout">
    <header class="topbar">
      <div class="brand"><span class="brand-mark">N</span> NÖBET <em>CO-OP</em></div>
      <div class="top-status"><span class="door-chip">KAPI ${padDoor(state.door)} / ${state.targetDoors}</span><button class="icon-button" data-action="leave_lobby">ÇIK</button></div>
    </header>
    <div class="route-line"><span>ROTA</span><div>${mapMarkup()}</div><span class="route-end">EXIT</span></div>
    <main class="game-grid">
      <section class="scene-panel panel ${room.dark && !room.lit ? "unlit" : ""} ${state.threat !== "none" ? "under-threat" : ""}">
        <div class="scene-heading"><div><div class="panel-kicker">ODA ${padDoor(room.number)}</div><h1>${escapeHtml(room.name)}</h1></div><span class="room-kind">${escapeHtml(kindLabels[room.kind] || room.kind)}</span></div>
        <div class="scene-viewport ${state.threat !== "none" || state.phase === "chase" ? "alert" : ""} ${room.dark && !room.lit ? "low-light" : ""}" aria-label="Birinci şahıs oda görünümü"><canvas id="scene-canvas" width="960" height="540"></canvas><div class="scene-crosshair" aria-hidden="true"></div><div class="scene-scanlines" aria-hidden="true"></div><div class="scene-hud"><span>${room.dark && !room.lit ? "LIGHT LOW" : "SIGNAL OK"}</span><span>${state.phase === "chase" ? "RUN" : "LOOK"}</span></div></div>
        <div class="room-copy"><p>${escapeHtml(room.description)}</p>${room.dark && !room.lit ? '<small class="muted">Oda karanlık. Çakmak kullanmadan arama yapamazsın.</small>' : ""}${room.locked ? '<small class="warning-text">İlerideki kapı kilitli; ekip anahtar veya maymuncuk aramalı.</small>' : ""}${room.falseDoorSeen ? '<small class="warning-text">Bu kapının yankısı yok. Dikkatli olun.</small>' : ""}</div>
        ${phaseBanner}${threat}${puzzle}${teamDoor}
        <div class="controls-panel">${state.phase === "chase" ? "" : normalActions(room, me)}</div>
      </section>
      <aside class="side-column">
        <section class="panel self-panel"><div class="panel-kicker">SEN</div><div class="self-name">${escapeHtml(me ? me.name : "Gezgin")}</div><div class="large-stat"><span>CAN</span>${bar(me ? me.health : 0)}</div><div class="large-stat"><span>AKIL</span>${bar(me ? me.sanity : 0)}</div><div class="self-items">${Object.entries((me && me.items) || {}).filter(([, amount]) => amount > 0).map(([item, amount]) => `<span>${escapeHtml(itemLabels[item] || item)} <b>${amount}</b></span>`).join("")}</div></section>
        <section class="panel team-panel"><div class="panel-heading"><div><div class="panel-kicker">EKİP</div><h3>Asansörde</h3></div><span class="count-pill">${state.players.length}</span></div><div class="team-list">${playersMarkup()}</div></section>
      </aside>
    </main>
    <section class="panel event-panel"><div class="panel-heading"><div><div class="panel-kicker">SON SİNYALLER</div><h3>Koridor kaydı</h3></div><span class="live-pill"><i></i> CANLI</span></div><div class="event-log">${state.log.slice(-8).map((line) => `<p>${escapeHtml(line)}</p>`).join("")}</div><form class="chat-form" data-form="chat"><input name="value" maxlength="180" placeholder="Ekibe mesaj yaz..." autocomplete="off"><button class="secondary" type="submit">GÖNDER</button></form></section>
    ${end}
  </section>`;
}

function render() {
  if (!state) {
    renderJoin();
  } else if (state.phase === "lobby") {
    renderLobby();
  } else {
    renderGame();
  }
  startSceneLoop();
}

async function sendAction(action, value = "") {
  if (!state || sending) return;
  sending = true;
  try {
    const next = await post("/api/action", { lobby: state.lobby, player: state.me, action, value });
    if (next.left) {
      state = null;
      localStorage.removeItem(storageKeys.lobby);
      localStorage.removeItem(storageKeys.player);
      render();
    } else {
      setState(next);
    }
  } catch (error) {
    if (error.message.includes("bulunamadı") || error.message.includes("oturum")) {
      state = null;
      localStorage.removeItem(storageKeys.lobby);
      localStorage.removeItem(storageKeys.player);
      renderJoin(error.message);
    } else {
      showToast(error.message);
    }
  } finally {
    sending = false;
  }
}

function resetScenePointer() {
  scenePointer.x = 0;
  scenePointer.y = 0;
}

document.addEventListener("pointermove", (event) => {
  const canvas = document.getElementById("scene-canvas");
  if (!canvas || event.target !== canvas || event.pointerType === "touch") {
    resetScenePointer();
    return;
  }
  const bounds = canvas.getBoundingClientRect();
  scenePointer.x = Math.max(-1, Math.min(1, ((event.clientX - bounds.left) / bounds.width - 0.5) * 2));
  scenePointer.y = Math.max(-1, Math.min(1, ((event.clientY - bounds.top) / bounds.height - 0.5) * 2));
}, { passive: true });
window.addEventListener("blur", resetScenePointer);

function showToast(message) {
  let toast = document.querySelector(".toast");
  if (!toast) {
    toast = document.createElement("div");
    toast.className = "toast";
    document.body.appendChild(toast);
  }
  toast.textContent = message;
  toast.classList.add("visible");
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => toast.classList.remove("visible"), 2600);
}

async function poll() {
  if (polling) return;
  polling = true;
  try {
    if (state) {
      const next = await getState();
      if (next) setState(next);
    }
  } catch (error) {
    if (state) showToast(error.message);
  } finally {
    polling = false;
    window.setTimeout(poll, 700);
  }
}

app.addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = event.target;
  if (form.id === "join-form") {
    const values = Object.fromEntries(new FormData(form).entries());
    try {
      const joined = await post("/api/join", values);
      lastEvent = joined.eventSeq;
      setState(joined);
    } catch (error) {
      renderJoin(error.message);
    }
    return;
  }
  if (form.dataset.form === "chat") {
    const input = form.elements.value;
    const message = input.value.trim();
    if (message) sendAction("chat", message);
    input.value = "";
    return;
  }
  if (form.dataset.form === "solve") {
    const input = form.elements.value;
    sendAction("solve", input.value.trim());
    input.value = "";
  }
});

app.addEventListener("change", (event) => {
  if (event.target.id === "role-select") {
    const description = document.getElementById("role-description");
    if (description) description.textContent = roleDescriptions[event.target.value] || "";
    sendAction("role", event.target.value);
  }
});

app.addEventListener("click", async (event) => {
  const copy = event.target.closest("[data-copy]");
  if (copy) {
    try {
      await navigator.clipboard.writeText(copy.dataset.copy);
      showToast("Lobi kodu kopyalandı.");
    } catch (_) {
      showToast(`Kod: ${copy.dataset.copy}`);
    }
    return;
  }
  const button = event.target.closest("[data-action]");
  if (!button || button.disabled) return;
  const action = button.dataset.action;
  if (action === "reset") {
    state = null;
    localStorage.removeItem(storageKeys.lobby);
    localStorage.removeItem(storageKeys.player);
    render();
    return;
  }
  if (action === "buy") {
    const select = document.getElementById("buy-item");
    sendAction("buy", select ? select.value : "bandage");
    return;
  }
  sendAction(action, button.dataset.value || "");
});

async function resume() {
  const lobby = localStorage.getItem(storageKeys.lobby);
  const player = localStorage.getItem(storageKeys.player);
  if (!lobby || !player) {
    render();
    poll();
    return;
  }
  try {
    const response = await fetch(`/api/state?lobby=${encodeURIComponent(lobby)}&player=${encodeURIComponent(player)}`, { cache: "no-store" });
    const saved = await response.json();
    if (saved.ok) {
      lastEvent = saved.eventSeq;
      setState(saved);
    } else {
      render();
    }
  } catch (_) {
    render();
  }
  poll();
}

resume();
