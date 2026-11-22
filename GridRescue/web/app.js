'use strict';

// =====================================================================
// State
// =====================================================================
const NS = 'http://www.w3.org/2000/svg';
const SNAP = 20;              // grid spacing (map units)
const KM_PER_UNIT = 0.1;      // auto line length: 10 map units = 1 km

let S = null;                 // latest grid state from the C++ backend
let hl = {nodes: [], edges: [], steps: false};
let lastDelivered = null;

let mode = 'select';          // select | station | substation | connect | fail | remove
let selected = null;          // {kind:'node', id} | {kind:'line', from, to} | null
let connectFrom = null;       // first node picked in connect mode
let pointer = null;           // last pointer position on the map {x, y}
let press = null;             // active pointer press on the map
let dirty = false;            // has the user edited the map since the last preset/clear?
let lastPreset = null;      // preset loaded in this session (for the button highlight)

const $ = id => document.getElementById(id);
const svg = $('map');
const layers = {edges: $('edgesLayer'), nodes: $('nodesLayer'), overlay: $('overlayLayer')};

const MODES = {
  select:     {key: 'v'},
  station:    {key: 'p'},
  substation: {key: 's'},
  connect:    {key: 'c'},
  fail:       {key: 'f'},
  remove:     {key: 'd'},
};

// =====================================================================
// Small helpers
// =====================================================================
const esc = s => String(s).replace(/[&<>"']/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c]));
const edgeKey = (a, b) => Math.min(a, b) + '-' + Math.max(a, b);
const allNodes = () => S ? [...S.stations.map(s => ({...s, src: true})), ...S.substations] : [];
const nodeById = id => allNodes().find(n => n.id === id);
const nameOf = id => (nodeById(id) || {}).name || `Node ${id}`;
const lineOf = (a, b) => S.lines.find(l => (l.from === a && l.to === b) || (l.from === b && l.to === a));
const isPowered = n => n && (n.src ? !n.failed : n.connected);

function nodeRadius() {
  const n = S ? S.stations.length + S.substations.length : 0;
  if (n > 30) return 20;
  if (n > 14) return 22;
  if (n > 8) return 26;
  return 30;
}

function el(tag, attrs = {}, parent) {
  const e = document.createElementNS(NS, tag);
  for (const k in attrs) e.setAttribute(k, attrs[k]);
  if (parent) parent.appendChild(e);
  return e;
}

function svgPoint(evt) {
  const pt = svg.createSVGPoint();
  pt.x = evt.clientX; pt.y = evt.clientY;
  const p = pt.matrixTransform(svg.getScreenCTM().inverse());
  return {x: p.x, y: p.y};
}

function snapPoint(p) {
  const r = nodeRadius() + 6;
  const W = S.map.w, H = S.map.h;
  const snap = v => Math.round(v / SNAP) * SNAP;
  return {
    x: Math.min(W - r, Math.max(r, snap(p.x))),
    y: Math.min(H - r, Math.max(r, snap(p.y))),
  };
}

// Returns the node that a node placed at p would overlap, if any.
function overlapAt(p, ignoreId = -1) {
  const minGap = nodeRadius() * 2 + 10;
  return allNodes().find(n => n.id !== ignoreId && Math.hypot(n.x - p.x, n.y - p.y) < minGap);
}

function log(msg, cls = '') {
  if (!msg) return;
  const li = document.createElement('li');
  const t = new Date().toLocaleTimeString([], {hour: '2-digit', minute: '2-digit', second: '2-digit'});
  li.innerHTML = `<time>${t}</time><span class="${cls}"></span>`;
  li.querySelector('span').textContent = msg;
  $('log').prepend(li);
}

// =====================================================================
// Backend
// =====================================================================
async function api(path, method = 'GET') {
  const res = await fetch('/api/' + path, {method});
  if (!res.ok) throw new Error('HTTP ' + res.status);
  const data = await res.json();
  S = data.state;
  return data.result;
}

function resetAllocation() {
  lastDelivered = null;
  $('alloc').innerHTML = '<li class="muted">Run “Redistribute power” to allocate.</li>';
}

function clearHl() { hl = {nodes: [], edges: [], steps: false}; }

// Runs a grid-changing request. Topology changes invalidate highlights and
// the last allocation, because both were computed for the old grid.
async function mutate(path, {quiet = false, keepAnalysis = false, cls = 'good'} = {}) {
  const r = await api(path, 'POST');
  if (r.ok) {
    dirty = true;
    if (!keepAnalysis) { clearHl(); resetAllocation(); }
    if (!quiet) log(r.message, cls);
  } else {
    log(r.message, 'bad');
  }
  render();
  return r;
}

// =====================================================================
// Rendering
// =====================================================================
function render() {
  if (!S) return;
  validateSelection();
  renderStatus();
  renderMap();
  renderInspector();
  renderFailures();
  renderSelects();
  renderLoads();
  renderConsumers();
  renderPresets();
  renderHint();
}

function validateSelection() {
  if (!selected) return;
  if (selected.kind === 'node' && !nodeById(selected.id)) selected = null;
  else if (selected.kind === 'line' && !lineOf(selected.from, selected.to)) selected = null;
  if (connectFrom !== null && !nodeById(connectFrom)) connectFrom = null;
}

function renderStatus() {
  const failed = allNodes().filter(n => n.failed).length;
  const cut = S.substations.filter(s => !s.failed && !s.connected).length;
  const online = S.substations.filter(s => s.connected).length;
  const working = S.stations.filter(s => !s.failed).length;
  $('status').innerHTML =
    `<span class="pill ${working ? 'ok' : 'bad'}">${working}/${S.stations.length} power stations</span>` +
    `<span class="pill ${failed ? 'bad' : 'ok'}">${failed} failed</span>` +
    `<span class="pill ${cut ? 'warn' : 'ok'}">${cut} cut off</span>` +
    `<span class="pill">${online}/${S.substations.length} substations online</span>`;
}

function renderMap() {
  layers.edges.innerHTML = '';
  layers.nodes.innerHTML = '';
  svg.dataset.mode = mode;

  const r = nodeRadius();

  for (const l of S.lines) {
    const a = nodeById(l.from), b = nodeById(l.to);
    if (!a || !b) continue;
    const k = edgeKey(l.from, l.to);
    let cls = 'edge';
    if (!l.active) cls += ' off';
    else if (isPowered(a) && isPowered(b)) cls += ' flow';
    if (hl.edges.includes(k)) cls += ' hl';
    const isSel = selected && selected.kind === 'line' && edgeKey(selected.from, selected.to) === k;
    if (isSel) cls += ' sel';

    const g = el('g', {'data-line': k}, layers.edges);
    el('line', {x1: a.x, y1: a.y, x2: b.x, y2: b.y, class: 'ehit'}, g);
    el('line', {x1: a.x, y1: a.y, x2: b.x, y2: b.y, class: cls}, g);
    const lab = el('text', {x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 - 8, class: 'elabel'}, g);
    lab.textContent = l.distance + ' km';
  }

  for (const n of allNodes()) {
    const R = n.src ? r + 3 : r;
    let cls = 'node ' + (n.src ? (n.failed ? 'src bad' : 'src') : n.failed ? 'bad' : n.connected ? 'ok' : 'warn');
    if (hl.nodes.includes(n.id)) cls += ' hl';
    if (selected && selected.kind === 'node' && selected.id === n.id) cls += ' sel';
    if (connectFrom === n.id) cls += ' cfrom';
    if (press && press.drag && press.drag.id === n.id && press.moved) cls += ' dragging';

    const g = el('g', {class: cls, transform: `translate(${n.x},${n.y})`, 'data-node': n.id,
      tabindex: 0, role: 'button', 'aria-label': `${n.src ? 'Power station' : 'Substation'} ${n.name}`}, layers.nodes);

    if (n.src) el('rect', {x: -R, y: -R, width: 2 * R, height: 2 * R, rx: 9, class: 'shape'}, g);
    else el('circle', {r: R, class: 'shape'}, g);

    const small = R < 26;
    el('text', {y: small ? -1 : -2, style: `font-size:${small ? 11 : 13}px`}, g).textContent = n.name;
    const sub = el('text', {y: small ? 10 : 13, class: 'sub', style: `font-size:${small ? 8 : 9.5}px`}, g);
    sub.textContent = n.failed ? 'FAILED' : n.src ? `${n.capacity} kW` : n.connected ? `${n.load}/${n.capacity}` : 'CUT OFF';
    el('text', {y: R + 14, class: 'nid'}, g).textContent = 'node ' + n.id;

    const step = hl.nodes.indexOf(n.id);
    if (hl.steps && step >= 0) el('text', {y: -(R + 9), class: 'step'}, g).textContent = '#' + (step + 1);
  }

  renderOverlay();
}

// Ghost previews, the rubber-band line in connect mode and the empty state.
function renderOverlay() {
  const o = layers.overlay;
  o.innerHTML = '';
  if (!S) return;

  if (!allNodes().length) {
    const t = el('text', {x: S.map.w / 2, y: S.map.h / 2 - 10, class: 'empty-msg'}, o);
    t.textContent = 'The map is empty.';
    const t2 = el('text', {x: S.map.w / 2, y: S.map.h / 2 + 16, class: 'empty-sub'}, o);
    t2.textContent = 'Pick “Power station” (P) or “Substation” (S) and click anywhere to place one.';
  }

  if (!pointer || press) return;

  if (mode === 'station' || mode === 'substation') {
    const p = snapPoint(pointer);
    const R = nodeRadius() + (mode === 'station' ? 3 : 0);
    const bad = !!overlapAt(p) || allNodes().length >= S.map.maxNodes;
    const g = el('g', {class: 'ghost' + (bad ? ' bad' : ''), transform: `translate(${p.x},${p.y})`}, o);
    if (mode === 'station') el('rect', {x: -R, y: -R, width: 2 * R, height: 2 * R, rx: 9}, g);
    else el('circle', {r: R}, g);
  }

  if (mode === 'connect' && connectFrom !== null) {
    const a = nodeById(connectFrom);
    if (a) el('line', {x1: a.x, y1: a.y, x2: pointer.x, y2: pointer.y, class: 'rubber'}, o);
  }
}

function renderHint() {
  const hints = {
    select: 'Click a node or line to see its details. Drag a node to move it. Delete key removes the selection.',
    station: 'Click an empty spot to place a power station. Esc returns to Select.',
    substation: 'Click an empty spot to place a substation. Esc returns to Select.',
    connect: connectFrom === null
      ? 'Click the first node of the new line.'
      : `Now click the node to connect to ${nameOf(connectFrom)}. Esc cancels.`,
    fail: 'Click a node to fail it. Click the most recent failure to restore it (stack order).',
    remove: 'Click a node or line to delete it. Deleting a node also deletes its lines.',
  };
  $('hint').textContent = hints[mode];
  document.querySelectorAll('.tool').forEach(b => b.setAttribute('aria-pressed', b.dataset.mode === mode));
}

function renderInspector() {
  const box = $('inspector');

  if (!selected) {
    box.innerHTML = `<h2>Inspector</h2>
      <p class="muted small">Nothing selected. Use the Select tool (V) and click a node or line to rename it,
      change its capacity or length, fail it, or delete it.</p>
      <p class="small">${S.stations.length} power station${S.stations.length === 1 ? '' : 's'},
      ${S.substations.length} substation${S.substations.length === 1 ? '' : 's'},
      ${S.lines.length} line${S.lines.length === 1 ? '' : 's'}.</p>`;
    return;
  }

  if (selected.kind === 'line') {
    const l = lineOf(selected.from, selected.to);
    const a = nodeById(l.from), b = nodeById(l.to);
    const status = !l.active ? 'Out of service (an end has failed)'
      : isPowered(a) && isPowered(b) ? 'Carrying power' : 'Not energised (no station upstream)';
    box.innerHTML = `<h2>Inspector</h2>
      <div class="insp-title"><b>${esc(a.name)} ↔ ${esc(b.name)}</b><span class="tag">Line</span></div>
      <p class="small ${l.active ? '' : 'bad-text'}">${status}</p>
      <form id="lineForm" class="insp-form">
        <label>Distance (km) <input id="iDist" type="number" min="1" max="1000" value="${l.distance}"></label>
        <button class="btn" type="submit">Save</button>
      </form>
      <div class="btn-row">
        <button class="btn" id="iEnds">Use ends as From / To</button>
        <button class="btn danger" id="iDelLine">Delete line</button>
      </div>`;
    $('lineForm').onsubmit = e => { e.preventDefault(); guard(() => mutate(`updateline?from=${l.from}&to=${l.to}&dist=${+$('iDist').value}`))(); };
    $('iDelLine').onclick = guard(() => mutate(`removeline?from=${l.from}&to=${l.to}`));
    $('iEnds').onclick = () => { $('from').value = l.from; $('to').value = l.to; log(`From = ${a.name}, To = ${b.name}.`, 'info'); };
    return;
  }

  const n = nodeById(selected.id);
  const top = S.history[S.history.length - 1];
  const status = n.failed ? 'Failed' : n.src ? 'Generating' : n.connected ? `Online, load ${n.load}/${n.capacity} kW` : 'Cut off from every power station';
  const neighbours = S.lines.filter(l => l.from === n.id || l.to === n.id).map(l => {
    const other = l.from === n.id ? l.to : l.from;
    return `<li class="${l.active ? '' : 'muted'}">${esc(nameOf(other))} <span>${l.distance} km${l.active ? '' : ', out of service'}</span></li>`;
  }).join('');

  let failBtn;
  if (!n.failed) failBtn = `<button class="btn danger" id="iFail">Fail ${esc(n.name)}</button>`;
  else if (top === n.id) failBtn = `<button class="btn" id="iFail">Restore ${esc(n.name)}</button>`;
  else failBtn = `<button class="btn" disabled title="Undo the failures above it in the stack first">Restore (not top of stack)</button>`;

  box.innerHTML = `<h2>Inspector</h2>
    <div class="insp-title"><b>${esc(n.name)}</b><span class="tag ${n.src ? 'src' : ''}">${n.src ? 'Power station' : 'Substation'}</span><span class="muted small">node ${n.id}</span></div>
    <p class="small ${n.failed ? 'bad-text' : !n.src && !n.connected ? 'warn-text' : ''}">${status}</p>
    <form id="nodeForm" class="insp-form">
      <label>Name <input id="iName" value="${esc(n.name)}" maxlength="20"></label>
      <label>${n.src ? 'Output' : 'Capacity'} (kW) <input id="iCap" type="number" min="1" value="${n.capacity}"></label>
      <button class="btn" type="submit">Save</button>
    </form>
    <span class="lbl">Lines</span>
    <ul class="neigh">${neighbours || '<li class="muted">Not connected. Use Connect (C) to add a line.</li>'}</ul>
    <div class="btn-row">${failBtn}<button class="btn danger" id="iDel">Delete</button></div>
    <div class="btn-row">
      <button class="btn" id="iFrom">Use as From</button>
      <button class="btn" id="iTo">Use as To</button>
      <button class="btn" id="iConn">Connect…</button>
    </div>`;

  $('nodeForm').onsubmit = e => {
    e.preventDefault();
    const name = $('iName').value.trim();
    const cap = parseInt($('iCap').value, 10);
    const qs = [];
    if (name && name !== n.name) qs.push('name=' + encodeURIComponent(name));
    if (!isNaN(cap) && cap !== n.capacity) qs.push('cap=' + cap);
    if (!qs.length) return log('Nothing to change.', 'info');
    guard(() => mutate(`updatenode?id=${n.id}&` + qs.join('&'), {keepAnalysis: !qs.some(s => s.startsWith('cap'))}))();
  };
  const fb = $('iFail');
  if (fb) fb.onclick = guard(() => failOrRestore(n));
  $('iDel').onclick = guard(() => removeNode(n.id));
  $('iFrom').onclick = () => { $('from').value = n.id; log(`From = ${n.name}.`, 'info'); };
  $('iTo').onclick = () => { $('to').value = n.id; log(`To = ${n.name}.`, 'info'); };
  $('iConn').onclick = () => { setMode('connect'); connectFrom = n.id; render(); };
}

function renderFailures() {
  const box = $('failBtns');
  box.innerHTML = '';
  for (const n of allNodes()) {
    const b = document.createElement('button');
    b.className = 'btn danger' + (n.failed ? ' on' : '');
    b.textContent = (n.failed ? '✕ ' : '') + n.name;
    b.disabled = n.failed;
    b.title = n.failed ? 'Already failed' : 'Fail ' + n.name;
    b.onclick = guard(() => failNode(n.id));
    box.appendChild(b);
  }
  if (!box.children.length) box.innerHTML = '<span class="muted small">No nodes on the map yet.</span>';
  $('undoBtn').disabled = S.history.length === 0;
  $('restoreBtn').disabled = S.history.length === 0;

  $('stack').innerHTML = S.history.length
    ? S.history.map(id => `<div class="item">${esc(nameOf(id))} failed</div>`).join('')
    : '<span class="empty">No failures. Grid healthy.</span>';
}

function renderSelects() {
  const nodes = allNodes();
  const opts = nodes.map(n => `<option value="${n.id}">${esc(n.name)} (${n.id})</option>`).join('');
  const firstSrc = S.stations.length ? S.stations[0].id : (nodes[0] || {}).id;
  for (const id of ['from', 'to', 'lineFrom', 'lineTo']) {
    const sel = $(id);
    const old = sel.value;
    sel.innerHTML = opts;
    if (old !== '' && nodes.some(n => String(n.id) === old)) sel.value = old;
    else if (nodes.length) {
      sel.value = (id === 'from' || id === 'lineFrom') ? firstSrc : nodes[nodes.length - 1].id;
    }
  }
  // Avoid "From = To" defaults once there is more than one node.
  for (const [a, b] of [['from', 'to'], ['lineFrom', 'lineTo']]) {
    if (nodes.length > 1 && $(a).value === $(b).value) {
      $(b).value = nodes.find(n => String(n.id) !== $(a).value).id;
    }
  }
}

function renderLoads() {
  $('loads').innerHTML = S.substations.length ? S.substations.map(s => {
    const pct = Math.round(100 * s.load / s.capacity);
    return `<div class="lrow"><div class="meta"><span>${esc(s.name)} ${s.failed ? '(failed)' : !s.connected ? '(cut off)' : ''}</span><span>${s.load}/${s.capacity} kW</span></div>
      <div class="bar"><i style="width:${pct}%" class="${pct > 85 ? 'low' : pct > 60 ? 'mid' : ''}"></i></div></div>`;
  }).join('') : '<p class="muted small">No substations yet.</p>';
}

function renderConsumers() {
  $('consumers').innerHTML = S.consumers.length ? S.consumers.map(c => {
    const d = lastDelivered ? (lastDelivered[c.id] ?? 0) : null;
    const pct = d === null ? 0 : Math.round(100 * d / c.required);
    const cls = pct >= 100 ? '' : pct > 0 ? 'mid' : 'low';
    return `<div class="cons">
      <button class="x" data-cid="${c.id}" aria-label="Remove ${esc(c.name)}" title="Remove consumer">×</button>
      <b>${esc(c.name)}</b>
      <div class="meta"><span>Priority ${c.priority}</span><span>${c.required} kW needed</span></div>
      <div class="bar"><i class="${cls}" style="width:${pct}%"></i></div>
      <div class="meta"><span>${d === null ? 'not allocated yet' : d + ' kW delivered'}</span><span>${d === null ? '' : pct + '%'}</span></div></div>`;
  }).join('') : '<p class="muted small">No consumers. Add one above so there is demand to supply.</p>';
  $('consumers').querySelectorAll('.x').forEach(b => {
    b.onclick = guard(() => mutate('removeconsumer?id=' + b.dataset.cid));
  });
}

function renderPresets() {
  document.querySelectorAll('.preset-btn').forEach(btn => {
    btn.classList.toggle('active', !dirty && +btn.dataset.n === lastPreset);
  });
}

// =====================================================================
// Editing actions
// =====================================================================
function setMode(m) {
  mode = m;
  connectFrom = null;
  renderHint();
  renderMap();
}

function select(target) {
  selected = target;
  renderMap();
  renderInspector();
}

async function placeNode(type, pt) {
  if (allNodes().length >= S.map.maxNodes) return log(`The map is full (${S.map.maxNodes} nodes). Delete a node first.`, 'bad');
  const p = snapPoint(pt);
  const hit = overlapAt(p);
  if (hit) return log(`Too close to ${hit.name}. Pick an empty spot.`, 'bad');
  const r = await mutate(`addnode?type=${type}&x=${p.x}&y=${p.y}`);
  if (r.ok) { selected = {kind: 'node', id: r.id}; render(); }
}

async function connect(a, b) {
  const A = nodeById(a), B = nodeById(b);
  let dist = parseInt($('toolDist').value, 10);
  if (isNaN(dist)) dist = Math.max(1, Math.round(Math.hypot(A.x - B.x, A.y - B.y) * KM_PER_UNIT));
  const r = await mutate(`addline?from=${a}&to=${b}&dist=${dist}`);
  if (r.ok) selected = {kind: 'line', from: a, to: b};
  render();
}

async function removeNode(id) {
  await mutate('removenode?id=' + id);
}

async function failNode(id) {
  const r = await mutate('fail?id=' + id, {cls: 'bad'});
  if (r.ok) {
    const cut = S.substations.filter(s => !s.failed && !s.connected).map(s => s.name);
    if (cut.length) log('Cut off from every power station: ' + cut.join(', '), 'bad');
  }
}

async function failOrRestore(n) {
  if (!n.failed) return failNode(n.id);
  if (S.history[S.history.length - 1] === n.id) return undo();
  log(`${n.name} can only be restored after later failures are undone (stack order).`, 'info');
}

async function undo() { await mutate('undo'); }

async function restoreAll() { await mutate('restore'); }

async function finishDrag(d) {
  const n = nodeById(d.id);
  const hit = overlapAt(n, n.id);
  if (hit) {
    n.x = d.orig.x; n.y = d.orig.y;   // put it back locally; the backend never moved it
    // keep S consistent with the backend
    const list = n.src ? S.stations : S.substations;
    Object.assign(list.find(x => x.id === n.id), d.orig);
    renderMap();
    return log(`Can't drop ${n.name} on top of ${hit.name}.`, 'bad');
  }
  await mutate(`move?id=${n.id}&x=${n.x}&y=${n.y}`, {quiet: true, keepAnalysis: true});
}

// Applies a click (pointer press without drag) on the map.
function mapClick(target, pt) {
  const isNode = target && target.kind === 'node';
  switch (mode) {
    case 'select':
      select(target);
      break;
    case 'station':
    case 'substation':
      if (target) select(target);
      else guard(() => placeNode(mode, pt))();
      break;
    case 'connect':
      if (!isNode) { if (connectFrom !== null) { connectFrom = null; render(); } return; }
      if (connectFrom === null) { connectFrom = target.id; render(); }
      else if (connectFrom === target.id) { connectFrom = null; render(); }
      else { const a = connectFrom; connectFrom = null; guard(() => connect(a, target.id))(); }
      break;
    case 'fail':
      if (isNode) guard(() => failOrRestore(nodeById(target.id)))();
      break;
    case 'remove':
      if (isNode) guard(() => removeNode(target.id))();
      else if (target) guard(() => mutate(`removeline?from=${target.from}&to=${target.to}`))();
      break;
  }
}

function targetOf(domEl) {
  const node = domEl.closest && domEl.closest('[data-node]');
  if (node) return {kind: 'node', id: +node.dataset.node};
  const line = domEl.closest && domEl.closest('[data-line]');
  if (line) {
    const [a, b] = line.dataset.line.split('-').map(Number);
    return {kind: 'line', from: a, to: b};
  }
  return null;
}

// =====================================================================
// Map pointer handling (click to place / connect, drag to move)
// =====================================================================
svg.addEventListener('pointerdown', e => {
  if (!S || e.button !== 0) return;
  const pt = svgPoint(e);
  const target = targetOf(e.target);
  press = {pt, target, moved: false, drag: null};
  if (mode === 'select' && target && target.kind === 'node') {
    const n = nodeById(target.id);
    press.drag = {id: n.id, dx: n.x - pt.x, dy: n.y - pt.y, orig: {x: n.x, y: n.y}};
  }
  svg.setPointerCapture(e.pointerId);
  e.preventDefault();
});

svg.addEventListener('pointermove', e => {
  if (!S) return;
  const pt = svgPoint(e);
  pointer = pt;
  if (press && press.drag) {
    if (!press.moved && Math.hypot(pt.x - press.pt.x, pt.y - press.pt.y) > 4) press.moved = true;
    if (press.moved) {
      const n = nodeById(press.drag.id);
      const p = snapPoint({x: pt.x + press.drag.dx, y: pt.y + press.drag.dy});
      const list = n.src ? S.stations : S.substations;
      Object.assign(list.find(x => x.id === n.id), p);
      renderMap();
    }
    return;
  }
  renderOverlay();
});

svg.addEventListener('pointerup', () => {
  if (!press) return;
  const p = press;
  press = null;
  if (p.drag && p.moved) { guard(() => finishDrag(p.drag))(); return; }
  mapClick(p.target, p.pt);
});

svg.addEventListener('pointercancel', () => {
  if (press && press.drag && press.moved) {
    const d = press.drag, n = nodeById(d.id);
    Object.assign((n.src ? S.stations : S.substations).find(x => x.id === n.id), d.orig);
  }
  press = null;
  renderMap();
});

svg.addEventListener('pointerleave', () => { if (!press) { pointer = null; renderOverlay(); } });

// Keyboard access to nodes: Enter / Space acts like a click.
svg.addEventListener('keydown', e => {
  if (e.key !== 'Enter' && e.key !== ' ') return;
  const t = targetOf(e.target);
  if (!t) return;
  e.preventDefault();
  const n = nodeById(t.id);
  mapClick(t, {x: n.x, y: n.y});
});

document.addEventListener('keydown', e => {
  if (e.target.matches('input, select, textarea') || e.ctrlKey || e.metaKey || e.altKey) return;
  const k = e.key.toLowerCase();
  if (k === 'escape') {
    if (connectFrom !== null) { connectFrom = null; render(); }
    else if (mode !== 'select') setMode('select');
    else if (selected) select(null);
    return;
  }
  if ((k === 'delete' || k === 'backspace') && selected) {
    e.preventDefault();
    const s = selected;
    if (s.kind === 'node') guard(() => removeNode(s.id))();
    else guard(() => mutate(`removeline?from=${s.from}&to=${s.to}`))();
    return;
  }
  if (e.target.closest && e.target.closest('[data-node]') && k === ' ') return;
  for (const m in MODES) if (MODES[m].key === k) { setMode(m); return; }
});

document.querySelectorAll('.tool').forEach(b => b.addEventListener('click', () => setMode(b.dataset.mode)));

// =====================================================================
// Analysis (BFS / DFS / Dijkstra / search / redistribution)
// =====================================================================
function pathEdges(path) {
  const e = [];
  for (let i = 0; i + 1 < path.length; i++) e.push(edgeKey(path[i], path[i + 1]));
  return e;
}

function needNodes() {
  if (allNodes().length >= 1) return true;
  log('Add some nodes to the map first.', 'bad');
  return false;
}

async function bfs() {
  if (!needNodes()) return;
  const r = await api('affected');
  hl = {nodes: r.order, edges: [], steps: true};
  if (!r.sources.length) log('BFS: there is no working power station, so nothing is powered.', 'bad');
  else log(`BFS from ${r.sources.map(nameOf).join(', ')}: reached ${r.order.map(nameOf).join(' → ')}`, 'info');
  log(r.affected.length ? 'Affected areas: ' + r.affected.map(nameOf).join(', ') : 'No affected areas. Every substation is connected.', r.affected.length ? 'bad' : 'good');
  render();
}

async function dfs() {
  if (!needNodes()) return;
  const a = +$('from').value, b = +$('to').value;
  const r = await api(`connection?from=${a}&to=${b}`);
  if (!r.ok) return log(r.message, 'bad');
  hl = {nodes: r.order, edges: [], steps: true};
  log(`DFS ${nameOf(a)} ⇒ ${nameOf(b)}: ${r.connected ? 'connection exists' : 'NO connection'} (visited ${r.order.map(nameOf).join(', ') || 'nothing'})`, r.connected ? 'good' : 'bad');
  render();
}

async function path() {
  if (!needNodes()) return;
  const a = +$('from').value, b = +$('to').value;
  const r = await api(`path?from=${a}&to=${b}`);
  if (!r.ok) return log(r.message, 'bad');
  if (!r.found) { clearHl(); render(); return log(`No path available from ${nameOf(a)} to ${nameOf(b)}.`, 'bad'); }
  hl = {nodes: r.path, edges: pathEdges(r.path), steps: true};
  log(`Shortest path: ${r.path.map(nameOf).join(' → ')} (${r.distance} km)`, 'good');
  render();
}

async function redistribute() {
  const r = await api('redistribute', 'POST');
  lastDelivered = {};
  r.consumers.forEach(c => lastDelivered[c.id] = c.delivered);
  $('alloc').innerHTML = r.allocations.length
    ? r.allocations.map(a => `<li><b>${esc(a.consumer)}</b> ← ${a.power} kW from ${esc(a.substation)}</li>`).join('')
    : '<li class="muted">No power could be allocated.</li>';

  if (r.idle.length) log(`${r.idle.map(nameOf).join(', ')} ${r.idle.length === 1 ? 'is' : 'are'} not connected to any substation, so ${r.idle.length === 1 ? 'its' : 'their'} output can't be used.`, 'bad');
  if (!r.islands.length) log('No working power station reaches any substation, so nothing can be supplied.', 'bad');
  else if (r.islands.length > 1) {
    r.islands.forEach(isl => log(`Island fed by ${isl.stations.map(nameOf).join(' + ')}: ${isl.supplied}/${isl.budget} kW used across ${isl.substations.length} substation(s).`, 'info'));
  }
  const short = r.consumers.filter(c => c.delivered < c.required);
  log(`Redistributed ${r.supplied}/${r.budget} kW by priority.` + (short.length ? ' Short: ' + short.map(c => `${c.name} (${c.delivered}/${c.required})`).join(', ') : ' All consumers supplied.'), short.length ? 'bad' : 'good');
  render();
}

async function search() {
  const name = $('q').value.trim();
  if (!name) return;
  const r = await api('search?name=' + encodeURIComponent(name));
  if (r.found) {
    hl = {nodes: [r.id], edges: [], steps: false};
    selected = {kind: 'node', id: r.id};
    log(`Found ${r.name} (node ID ${r.id}).`, 'good');
  } else { clearHl(); log(`Component "${name}" not found.`, 'bad'); }
  render();
}

// =====================================================================
// Presets, blank canvas, consumers, line form
// =====================================================================
function confirmReplace() {
  return !dirty || confirm('Replace the current map? Your edits will be lost.');
}

async function loadPreset(val) {
  const n = parseInt(val, 10);
  const errDiv = $('configError');
  errDiv.hidden = true;
  if (isNaN(n) || n < 2 || n > 20) {
    errDiv.textContent = 'Enter a number of substations between 2 and 20.';
    errDiv.hidden = false;
    return;
  }
  if (!confirmReplace()) return;
  const r = await api(`reset?n=${n}`, 'POST');
  if (!r.ok) return log(r.message, 'bad');
  dirty = false; lastPreset = n; selected = null; connectFrom = null;
  clearHl(); resetAllocation();
  $('substationCount').value = n;
  log(r.message, 'good');
  render();
}

async function blankCanvas() {
  if (!confirmReplace()) return;
  const r = await api('clear', 'POST');
  dirty = false; lastPreset = null; selected = null; connectFrom = null;
  clearHl(); resetAllocation();
  log(r.message, 'info');
  setMode('station');
  render();
}

async function addLineFromForm() {
  const from = +$('lineFrom').value, to = +$('lineTo').value;
  const dist = parseInt($('lineDist').value, 10);
  if (from === to) return log('Cannot connect a node to itself.', 'bad');
  if (isNaN(dist) || dist <= 0) return log('Transmission line distance must be a positive number.', 'bad');
  await mutate(`addline?from=${from}&to=${to}&dist=${dist}`);
}

async function addConsumer(e) {
  e.preventDefault();
  const name = $('cName').value.trim();
  const req = parseInt($('cReq').value, 10);
  const pri = $('cPri').value;
  const r = await mutate(`addconsumer?name=${encodeURIComponent(name)}&req=${req}&pri=${pri}`);
  if (r.ok) $('cName').value = '';
}

// =====================================================================
// Wiring
// =====================================================================
const guard = fn => () => fn().catch(e => log('Error: ' + e.message + ' — is the GridRescue server running?', 'bad'));

document.querySelectorAll('.preset-btn').forEach(btn => {
  btn.addEventListener('click', guard(() => loadPreset(btn.dataset.n)));
});
$('applyCountBtn').onclick = guard(() => loadPreset($('substationCount').value));
$('substationCount').addEventListener('keydown', e => { if (e.key === 'Enter') guard(() => loadPreset($('substationCount').value))(); });
$('blankBtn').onclick = guard(blankCanvas);

$('undoBtn').onclick = guard(undo);
$('restoreBtn').onclick = guard(restoreAll);
$('bfsBtn').onclick = guard(bfs);
$('dfsBtn').onclick = guard(dfs);
$('pathBtn').onclick = guard(path);
$('redistBtn').onclick = guard(redistribute);
$('searchBtn').onclick = guard(search);
$('q').addEventListener('keydown', e => { if (e.key === 'Enter') guard(search)(); });
$('clearLog').onclick = () => $('log').innerHTML = '';
$('addLineBtn').onclick = guard(addLineFromForm);
$('consumerForm').addEventListener('submit', e => guard(() => addConsumer(e))());

// Initial load
guard(async () => {
  await api('state');
  resetAllocation();
  log('Connected to GridRescue backend. Pick a tool above the map to start editing.', 'good');
  render();
})();
