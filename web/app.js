const app = document.getElementById("app");

const storageKeys = {
  lobby: "nobet-lobby",
  player: "nobet-player",
};

let state = null;
let lastEvent = -1;
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
        <pre class="room-art" aria-label="Oda görseli">${escapeHtml(roomArt(room, state.phase))}</pre>
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
