/* ── Toast ─────────────────────────────────────────────────────── */

function toast(msg, type = '') {
  const c = document.getElementById('toast-container');
  const t = document.createElement('div');
  t.className = 'toast' + (type ? ' ' + type : '');
  t.textContent = msg;
  c.appendChild(t);
  requestAnimationFrame(() => requestAnimationFrame(() => t.classList.add('show')));
  setTimeout(() => {
    t.classList.remove('show');
    setTimeout(() => t.remove(), 220);
  }, 3200);
}

/* ── API helpers ───────────────────────────────────────────────── */

async function api(url, opts = {}) {
  try {
    const r = await fetch(url, opts);
    return await r.json();
  } catch {
    return { ok: false, error: 'network' };
  }
}

async function apiPost(url, params) {
  const body = new URLSearchParams(params);
  return api(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body
  });
}

function setBtn(btn, text, disabled) {
  btn.textContent = text;
  btn.disabled    = disabled;
}

/* ── Tab switching ─────────────────────────────────────────────── */

const TAB_LOADERS = {
  system:      () => { loadSystem(); loadAutostart(); },
  profiles:    loadProfiles,
  badge:       loadBadge,
  'ir-lib':    loadIrLib,
  'ir-upload': loadIrUpload,
  flood:       loadFlood,
  recon:       loadRecon,
  totp:        loadTotp,
};

document.querySelectorAll('.nav-tab').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.nav-tab').forEach(t => {
      t.classList.remove('active');
      t.setAttribute('aria-selected', 'false');
    });
    document.querySelectorAll('.pane').forEach(p => p.classList.remove('active'));

    btn.classList.add('active');
    btn.setAttribute('aria-selected', 'true');
    const pane = document.getElementById('pane-' + btn.dataset.tab);
    if (pane) pane.classList.add('active');

    const loader = TAB_LOADERS[btn.dataset.tab];
    if (loader) loader();
  });
});

/* ── System ────────────────────────────────────────────────────── */

async function loadSystem() {
  const [bat, status] = await Promise.all([
    api('/api/battery'),
    api('/api/status'),
  ]);

  if (bat.ok) {
    const v = Number(bat.voltage).toFixed(2) + ' V';
    document.getElementById('sys-battery').textContent   = v;
    document.getElementById('battery-display').textContent = v;
  }

  if (status.ok) {
    document.getElementById('sys-status').textContent = status.portal || 'active';
    document.getElementById('sys-ssid').textContent   = status.ssid   || '--';
    document.getElementById('sys-ip').textContent     = status.ip     || '--';
  }
}

async function loadAutostart() {
  const data = await api('/api/settings');
  if (!data.ok) return;
  const toggle = document.getElementById('autostart-toggle');
  if (toggle) toggle.checked = !!data.portalAutostart;
  const sleepSel = document.getElementById('sleep-timeout-select');
  if (sleepSel) sleepSel.value = String(data.sleepTimeoutIndex ?? 1);
  const startToggle = document.getElementById('start-screen-toggle');
  if (startToggle) startToggle.checked = data.showStartScreen !== false;
}

document.getElementById('autostart-toggle').addEventListener('change', async function() {
  await apiPost('/api/settings', { portalAutostart: this.checked ? '1' : '0' });
});

document.getElementById('sleep-timeout-select').addEventListener('change', async function() {
  await apiPost('/api/settings', { sleepTimeoutIndex: this.value });
});

document.getElementById('start-screen-toggle').addEventListener('change', async function() {
  await apiPost('/api/settings', { showStartScreen: this.checked ? '1' : '0' });
});

setInterval(async () => {
  const bat = await api('/api/battery');
  if (bat.ok) {
    const v = Number(bat.voltage).toFixed(2) + ' V';
    document.getElementById('sys-battery').textContent   = v;
    document.getElementById('battery-display').textContent = v;
  }
}, 5000);

/* ── Profiles ──────────────────────────────────────────────────── */

let _profileData  = null;
let _editProfile  = 0;

async function loadProfiles() {
  const data = await api('/api/profiles');
  if (!data.ok) return;
  _profileData = data;
  _editProfile = data.active;
  renderProfileTabs();
  renderProfileForm();
}

function renderProfileTabs() {
  const container = document.getElementById('profile-tabs');
  container.innerHTML = '';
  for (let i = 0; i < _profileData.profiles.length; i++) {
    const btn = document.createElement('button');
    btn.className = 'btn' + (i === _editProfile ? ' btn-primary' : '');
    btn.textContent = _profileData.profiles[i].name || ('Profile ' + (i + 1));
    if (i === _profileData.active) btn.textContent += ' ✓';
    btn.addEventListener('click', () => { _editProfile = i; renderProfileTabs(); renderProfileForm(); });
    container.appendChild(btn);
  }
}

function renderProfileForm() {
  const p = _profileData.profiles[_editProfile];
  document.getElementById('pf-name').value    = p.name    || '';
  document.getElementById('pf-tagline').value = p.tagline || '';
  document.getElementById('pf-qr').value      = p.qrData  || '';
  const btn = document.getElementById('pf-activate-btn');
  btn.textContent = _editProfile === _profileData.active
    ? 'Active on Device ✓' : 'Set as Active on Device';
  btn.disabled = _editProfile === _profileData.active;
}

document.getElementById('profile-form').addEventListener('submit', async function(e) {
  e.preventDefault();
  await apiPost('/api/profiles', {
    index:   String(_editProfile),
    name:    document.getElementById('pf-name').value,
    tagline: document.getElementById('pf-tagline').value,
    qrData:  document.getElementById('pf-qr').value,
  });
  await loadProfiles();
});

document.getElementById('pf-activate-btn').addEventListener('click', async function() {
  await apiPost('/api/profiles/active', { index: String(_editProfile) });
  await loadProfiles();
});

/* ── Badge ─────────────────────────────────────────────────────── */

async function loadBadge() {
  const data = await api('/api/settings');
  if (!data.ok) return;

  document.getElementById('badge-name').value    = data.badgeName    || '';
  document.getElementById('badge-tagline').value = data.badgeTagline || '';
  document.getElementById('badge-qr').value      = data.badgeQrData  || '';

  const sval = data.badgeStatus || 'Online';
  const sel  = document.getElementById('badge-status');
  const opt  = [...sel.options].find(o => o.value.toLowerCase() === sval.toLowerCase());
  sel.value  = opt ? opt.value : 'Online';
}

document.getElementById('badge-form').addEventListener('submit', async e => {
  e.preventDefault();
  const btn = document.getElementById('badge-save-btn');
  setBtn(btn, 'Saving…', true);

  const data = await apiPost('/api/settings', {
    badgeName:    document.getElementById('badge-name').value.trim(),
    badgeStatus:  document.getElementById('badge-status').value,
    badgeTagline: document.getElementById('badge-tagline').value.trim(),
    badgeQrData:  document.getElementById('badge-qr').value.trim(),
  });
  setBtn(btn, 'Save', false);

  if (data.ok) {
    // Re-read to confirm what was stored
    const confirmed = await api('/api/settings');
    if (confirmed.ok) {
      document.getElementById('badge-name').value    = confirmed.badgeName    || '';
      document.getElementById('badge-tagline').value = confirmed.badgeTagline || '';
      document.getElementById('badge-qr').value      = confirmed.badgeQrData  || '';
    }
    toast('Badge saved.', 'ok');
  } else {
    toast('Save failed.', 'err');
  }
});

/* ── IR Library ────────────────────────────────────────────────── */

async function loadIrLib() {
  const root = document.getElementById('ir-list');
  root.innerHTML = '<div class="list-empty">Loading…</div>';

  const data = await api('/api/ir/list');
  if (!data.ok || !data.items) {
    root.innerHTML = '<div class="list-empty">Failed to load.</div>';
    return;
  }
  if (!data.items.length) {
    root.innerHTML = '<div class="list-empty">No saved IR captures.</div>';
    return;
  }

  root.innerHTML = '';
  data.items.forEach(item => root.appendChild(buildIrRow(item)));
}

function buildIrRow(item) {
  const row = document.createElement('div');
  row.className = 'ir-row';
  row.dataset.id = item.id;

  const info   = document.createElement('div');
  info.className = 'ir-info';
  const nameEl = document.createElement('div');
  nameEl.className = 'ir-name';
  nameEl.textContent = item.name;
  const sub = document.createElement('div');
  sub.className = 'ir-sub';
  sub.textContent = 'ID ' + item.id + ' · ' + item.fileId;
  info.append(nameEl, sub);

  const actions = document.createElement('div');
  actions.className = 'ir-actions';

  const sendBtn   = makeBtn('Send',   () => irSend(item.id, sendBtn, row));
  const renameBtn = makeBtn('Rename', () => startRename(row, item));
  const exportBtn = makeBtn('Export', () => {
    window.location.href = '/api/ir/export?id=' + item.id;
  });
  const delBtn    = makeBtn('Delete', () => irDelete(item.id, item.name, row), true);

  actions.append(sendBtn, renameBtn, exportBtn, delBtn);
  row.append(info, actions);
  return row;
}

function makeBtn(label, onClick, isDanger = false) {
  const b = document.createElement('button');
  b.className = 'btn btn-sm' + (isDanger ? ' btn-danger' : '');
  b.textContent = label;
  b.addEventListener('click', onClick);
  return b;
}

async function irSend(id, btn, row) {
  setBtn(btn, '…', true);
  const data = await apiPost('/api/ir/send', { id });
  setBtn(btn, 'Send', false);
  if (data.ok) toast('Sent.');
  else         rowFeedback(row, 'Send failed.', true);
}

async function irDelete(id, name, row) {
  const ok = await showConfirm(row, `Delete "${name}"?`);
  if (!ok) return;

  const btn = row.querySelector('.btn-danger');
  if (btn) setBtn(btn, '…', true);

  const data = await apiPost('/api/ir/delete', { id });
  if (data.ok) {
    row.remove();
    const list = document.getElementById('ir-list');
    if (!list.children.length)
      list.innerHTML = '<div class="list-empty">No saved IR captures.</div>';
  } else {
    if (btn) setBtn(btn, 'Delete', false);
    rowFeedback(row, 'Delete failed.', true);
  }
}

function startRename(row, item) {
  const nameEl  = row.querySelector('.ir-name');
  const actions = row.querySelector('.ir-actions');

  const input = document.createElement('input');
  input.type      = 'text';
  input.value     = item.name;
  input.className = 'input rename-input';
  input.maxLength = 32;
  nameEl.replaceWith(input);
  input.focus(); input.select();

  actions.innerHTML = '';
  const saveBtn   = makeBtn('Save', async () => {
    const newName = input.value.trim();
    if (!newName) return;
    setBtn(saveBtn, '…', true);
    const data = await apiPost('/api/ir/rename', { id: item.id, name: newName });
    if (data.ok) {
      item.name = newName;
      const restored = document.createElement('div');
      restored.className = 'ir-name';
      restored.textContent = newName;
      input.replaceWith(restored);
      actions.innerHTML = '';
      const s = makeBtn('Send',   () => irSend(item.id, s, row));
      const r = makeBtn('Rename', () => startRename(row, item));
      const d = makeBtn('Delete', () => irDelete(item.id, item.name, row), true);
      actions.append(s, r, d);
    } else {
      setBtn(saveBtn, 'Error', false);
      setTimeout(() => setBtn(saveBtn, 'Save', false), 1500);
    }
  });

  const cancelBtn = makeBtn('Cancel', () => {
    const restored = document.createElement('div');
    restored.className = 'ir-name';
    restored.textContent = item.name;
    input.replaceWith(restored);
    actions.innerHTML = '';
    const s = makeBtn('Send',   () => irSend(item.id, s, row));
    const r = makeBtn('Rename', () => startRename(row, item));
    const d = makeBtn('Delete', () => irDelete(item.id, item.name, row), true);
    actions.append(s, r, d);
  });

  actions.append(saveBtn, cancelBtn);
}

function rowFeedback(row, msg, isError = false) {
  let fb = row.querySelector('.row-fb');
  if (!fb) {
    fb = document.createElement('div');
    fb.className = 'row-fb';
    fb.style.width = '100%';
    row.appendChild(fb);
  }
  fb.textContent = msg;
  fb.classList.toggle('err', isError);
  if (msg) setTimeout(() => { if (fb.textContent === msg) fb.textContent = ''; }, 4000);
}

// Confirm dialog: replaces action buttons with Yes/No, preserves listeners via live node refs
function showConfirm(row, msg) {
  return new Promise(resolve => {
    const actions = row.querySelector('.ir-actions') || row.querySelector('.file-actions');
    if (!actions) { resolve(true); return; }

    const origNodes = Array.from(actions.children);
    const restore = () => {
      actions.innerHTML = '';
      origNodes.forEach(n => actions.appendChild(n));
    };

    const label = document.createElement('span');
    label.className = 'ir-sub';
    label.textContent = msg;

    const yes = makeBtn('Yes', () => { restore(); resolve(true); }, true);
    const no  = makeBtn('No',  () => { restore(); resolve(false); });

    actions.innerHTML = '';
    actions.append(label, yes, no);
  });
}

/* ── IR Upload ─────────────────────────────────────────────────── */

const UPLOAD_PATH = '/ir/uploads';

function loadIrUpload() { loadUploadList(); }

const dropZone    = document.getElementById('drop-zone');
const uploadInput = document.getElementById('upload-input');

dropZone.addEventListener('click', () => uploadInput.click());
dropZone.addEventListener('keydown', e => {
  if (e.key === 'Enter' || e.key === ' ') uploadInput.click();
});
dropZone.addEventListener('dragover', e => {
  e.preventDefault(); dropZone.classList.add('dragover');
});
dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
dropZone.addEventListener('drop', e => {
  e.preventDefault();
  dropZone.classList.remove('dragover');
  if (e.dataTransfer.files.length) uploadFiles(e.dataTransfer.files);
});
uploadInput.addEventListener('change', () => {
  if (uploadInput.files.length) uploadFiles(uploadInput.files);
  uploadInput.value = '';
});

function uploadFiles(files) {
  Array.from(files).forEach(f => uploadSingle(f));
}

function uploadSingle(file) {
  const wrap  = document.getElementById('upload-progress-wrap');
  const bar   = document.getElementById('upload-progress-bar');
  const label = document.getElementById('upload-progress-label');

  wrap.classList.remove('hidden');
  bar.style.setProperty('--pct', '0%');
  label.textContent = '0%';

  const fd = new FormData();
  fd.append('file', file);

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/file/upload?path=' + encodeURIComponent(UPLOAD_PATH));

  xhr.upload.addEventListener('progress', e => {
    if (!e.lengthComputable) return;
    const pct = Math.round(e.loaded / e.total * 100) + '%';
    bar.style.setProperty('--pct', pct);
    label.textContent = pct;
  });

  xhr.addEventListener('load', () => {
    wrap.classList.add('hidden');
    try {
      const data = JSON.parse(xhr.responseText);
      if (data.ok) { toast(file.name + ' uploaded.', 'ok'); loadUploadList(); }
      else         { toast('Upload failed.', 'err'); }
    } catch { toast('Upload failed.', 'err'); }
  });

  xhr.addEventListener('error', () => {
    wrap.classList.add('hidden');
    toast('Upload error.', 'err');
  });

  xhr.send(fd);
}

async function loadUploadList() {
  const root = document.getElementById('upload-list');
  root.innerHTML = '<div class="list-empty">Loading…</div>';

  const data = await api('/api/fs/list?path=' + encodeURIComponent(UPLOAD_PATH));
  if (!data.ok) { root.innerHTML = '<div class="list-empty">Failed to load.</div>'; return; }

  const files = (data.entries || []).filter(e => !e.isDirectory);
  if (!files.length) { root.innerHTML = '<div class="list-empty">No uploaded files.</div>'; return; }

  root.innerHTML = '';
  files.forEach(entry => root.appendChild(buildFileRow(entry)));
}

async function irImport(path, btn, row) {
  setBtn(btn, '…', true);
  const data = await apiPost('/api/ir/import', { path });
  setBtn(btn, 'Import', false);
  if (data.ok) {
    toast('Imported ' + data.count + ' signal(s).', 'ok');
    const irPane = document.getElementById('pane-ir-lib');
    if (irPane && irPane.classList.contains('active')) loadIrLib();
  } else {
    rowFeedback(row, 'Import failed.', true);
  }
}

function buildFileRow(entry) {
  const row  = document.createElement('div');
  row.className = 'file-row';

  const name = document.createElement('div');
  name.className = 'file-name';
  name.textContent = entry.name;

  const acts = document.createElement('div');
  acts.className = 'file-actions';

  if (entry.name.toLowerCase().endsWith('.ir')) {
    const importBtn  = makeBtn('Import', () => irImport(entry.path, importBtn, row));
    const browseBtn  = makeBtn('Browse', () => toggleSignalBrowser(row, entry.name, browseBtn));
    acts.append(importBtn, browseBtn);
  }

  const dlBtn = makeBtn('Download', () => {
    window.location.href = '/api/file/download?path=' + encodeURIComponent(entry.path);
  });

  const delBtn = makeBtn('Delete', async () => {
    const yes = await showConfirm(row, 'Delete "' + entry.name + '"?');
    if (!yes) return;
    setBtn(delBtn, '…', true);
    const data = await apiPost('/api/file/delete', { path: entry.path });
    if (data.ok) {
      row.remove();
      const list = document.getElementById('upload-list');
      if (!list.children.length)
        list.innerHTML = '<div class="list-empty">No uploaded files.</div>';
    } else {
      setBtn(delBtn, 'Delete', false);
      toast('Delete failed.', 'err');
    }
  }, true);

  acts.append(dlBtn, delBtn);
  row.append(name, acts);
  return row;
}

async function toggleSignalBrowser(row, filename, btn) {
  let panel = row.nextElementSibling;
  if (panel && panel.classList.contains('signal-panel')) {
    panel.remove();
    setBtn(btn, 'Browse', false);
    return;
  }
  setBtn(btn, '…', true);
  const data = await api('/api/ir/upload-signals?filename=' + encodeURIComponent(filename));
  setBtn(btn, 'Browse', false);
  if (!data.ok || !data.signals) {
    rowFeedback(row, 'Failed to load signals.', true);
    return;
  }
  if (!data.signals.length) {
    rowFeedback(row, 'No signals found in file.', false);
    return;
  }
  panel = document.createElement('div');
  panel.className = 'signal-panel';
  data.signals.forEach(sig => {
    const srow = document.createElement('div');
    srow.className = 'signal-row';
    const sname = document.createElement('span');
    sname.className = 'signal-name';
    sname.textContent = sig.name;
    const sendBtn = makeBtn('Send', async () => {
      setBtn(sendBtn, '…', true);
      const r = await apiPost('/api/ir/send-upload', { filename, index: sig.index });
      setBtn(sendBtn, 'Send', false);
      if (r.ok) toast('Sent: ' + sig.name);
      else      toast('Send failed.', 'err');
    });
    srow.append(sname, sendBtn);
    panel.appendChild(srow);
  });
  row.after(panel);
}

/* ── Flood (Beacon Spam) ───────────────────────────────────────── */

async function loadFlood() {
  const data = await api('/api/wifi/ssids');
  if (!data.ok || !data.ssids) return;
  document.getElementById('ssid-textarea').value = data.ssids.join('\n');
  document.getElementById('ssid-count-badge').textContent = data.ssids.length + ' SSIDs';
}

document.getElementById('ssid-save-btn').addEventListener('click', async () => {
  const btn   = document.getElementById('ssid-save-btn');
  const badge = document.getElementById('ssid-count-badge');
  const raw   = document.getElementById('ssid-textarea').value;
  const ssids = raw.split('\n').map(s => s.trim()).filter(Boolean);

  if (ssids.length > 20) { toast('Max 20 SSIDs.', 'err'); return; }

  setBtn(btn, 'Saving…', true);
  const data = await apiPost('/api/wifi/ssids', { ssids: ssids.join('\n') });
  setBtn(btn, 'Save SSIDs', false);

  if (data.ok) {
    badge.textContent = ssids.length + ' SSIDs';
    toast('SSID list saved.', 'ok');
  } else {
    toast('Save failed.', 'err');
  }
});

/* ── Recon: Probe Sniffer + Karma ──────────────────────────────── */

let _sniffInterval = null;
let _sniffing      = false;

function loadRecon() {
  loadKarmaStatus();
  if (_sniffing) loadProbes();
}

// Sniff toggle
document.getElementById('sniff-toggle-btn').addEventListener('click', async () => {
  const btn      = document.getElementById('sniff-toggle-btn');
  const statusEl = document.getElementById('probe-status');

  if (_sniffing) {
    setBtn(btn, '…', true);
    await api('/api/karma/stop-sniff', { method: 'POST' });
    clearInterval(_sniffInterval);
    _sniffInterval = null;
    _sniffing      = false;
    setBtn(btn, 'Start', false);
    btn.className  = 'btn btn-primary';
    statusEl.textContent = 'Not sniffing · sniffs on current AP channel only';
    statusEl.className   = 'status-line';
  } else {
    setBtn(btn, '…', true);
    const data = await api('/api/karma/start-sniff', { method: 'POST' });
    setBtn(btn, 'Stop', false);
    if (!data.ok) { toast('Failed to start sniff.', 'err'); return; }
    btn.className  = 'btn btn-danger';
    _sniffing      = true;
    statusEl.textContent = 'Sniffing…';
    statusEl.className   = 'status-line active';
    loadProbes();
    _sniffInterval = setInterval(loadProbes, 2000);
  }
});

// Clear probes
document.getElementById('clear-probes-btn').addEventListener('click', async () => {
  await api('/api/karma/clear', { method: 'POST' });
  document.getElementById('probe-tbody').innerHTML =
    '<tr><td colspan="4" class="table-empty">No probes captured yet</td></tr>';
  if (_sniffing) {
    document.getElementById('probe-status').textContent = 'Sniffing…';
    document.getElementById('probe-status').className   = 'status-line active';
  }
  toast('Probes cleared.');
});

async function loadProbes() {
  const data = await api('/api/karma/probes');
  if (!data.ok) return;

  const probes = data.probes || [];
  const tbody  = document.getElementById('probe-tbody');
  const status = document.getElementById('probe-status');

  if (!probes.length) {
    tbody.innerHTML = '<tr><td colspan="4" class="table-empty">No probes yet — listening…</td></tr>';
    return;
  }

  const n = probes.length;
  status.textContent = `Sniffing — ${n} unique SSID${n !== 1 ? 's' : ''} captured`;

  tbody.innerHTML = '';
  probes.forEach(p => {
    const tr = document.createElement('tr');

    const ssidTd = document.createElement('td');
    ssidTd.textContent = p.ssid;
    ssidTd.style.fontWeight = '500';

    const countTd = document.createElement('td');
    countTd.className = 'text-right';
    countTd.textContent = p.count;

    const rssiTd = document.createElement('td');
    rssiTd.className = 'text-right text-muted';
    rssiTd.textContent = p.rssi + ' dBm';

    const actionTd = document.createElement('td');
    actionTd.className = 'text-right';
    const karmaBtn = document.createElement('button');
    karmaBtn.className   = 'btn btn-sm';
    karmaBtn.textContent = 'Karma';
    karmaBtn.addEventListener('click', () => startKarma(p.ssid));
    actionTd.appendChild(karmaBtn);

    tr.append(ssidTd, countTd, rssiTd, actionTd);
    tbody.appendChild(tr);
  });
}

// Karma controls
async function startKarma(ssid) {
  const stopBtn  = document.getElementById('karma-stop-btn');
  const statusEl = document.getElementById('karma-status');

  const data = await apiPost('/api/karma/start', { ssid });
  if (!data.ok) { toast('Karma failed to start.', 'err'); return; }

  // Stop probe sniff (karma and sniff share promiscuous mode)
  if (_sniffing) {
    clearInterval(_sniffInterval);
    _sniffInterval = null;
    _sniffing      = false;
    document.getElementById('sniff-toggle-btn').textContent = 'Start';
    document.getElementById('sniff-toggle-btn').className   = 'btn btn-primary';
    document.getElementById('probe-status').className = 'status-line';
    document.getElementById('probe-status').textContent = 'Stopped (karma active)';
  }

  stopBtn.classList.remove('hidden');
  statusEl.textContent = 'Karma active — cloning "' + ssid + '" as open AP · 192.168.4.1';
  statusEl.className   = 'status-line warn';
  toast('Karma started: ' + ssid, 'ok');
}

document.getElementById('karma-stop-btn').addEventListener('click', async () => {
  const stopBtn  = document.getElementById('karma-stop-btn');
  const statusEl = document.getElementById('karma-status');

  setBtn(stopBtn, '…', true);
  await api('/api/karma/stop', { method: 'POST' });
  setBtn(stopBtn, 'Stop Karma', false);
  stopBtn.classList.add('hidden');

  statusEl.textContent = 'Karma stopped — AP restored to STEALTHY-SETUP';
  statusEl.className   = 'status-line';
  toast('Karma stopped.');
});

async function loadKarmaStatus() {
  const data     = await api('/api/karma/status');
  const stopBtn  = document.getElementById('karma-stop-btn');
  const statusEl = document.getElementById('karma-status');

  if (!data.ok) return;

  if (data.karma) {
    stopBtn.classList.remove('hidden');
    statusEl.textContent = 'Karma active — cloning "' + data.karmaSsid + '" as open AP';
    statusEl.className   = 'status-line warn';
  } else {
    stopBtn.classList.add('hidden');
    statusEl.textContent = 'No karma active — click Karma on a probe entry above';
    statusEl.className   = 'status-line';
  }

  if (data.sniffing) {
    _sniffing = true;
    document.getElementById('sniff-toggle-btn').textContent = 'Stop';
    document.getElementById('sniff-toggle-btn').className   = 'btn btn-danger';
    document.getElementById('probe-status').className   = 'status-line active';
    document.getElementById('probe-status').textContent = 'Sniffing — ' + data.probeCount + ' unique SSIDs';
    if (!_sniffInterval) _sniffInterval = setInterval(loadProbes, 2000);
  }
}

/* ── TOTP ──────────────────────────────────────────────────────── */

// ── Base32 decode (RFC 4648) ──────────────────────────────────────
function base32Decode(str) {
  const alpha = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567';
  str = str.toUpperCase().replace(/[\s=]/g, '');
  let bits = 0, nbits = 0;
  const out = [];
  for (const c of str) {
    const v = alpha.indexOf(c);
    if (v < 0) continue;
    bits = (bits << 5) | v;
    nbits += 5;
    if (nbits >= 8) { out.push((bits >> (nbits - 8)) & 0xFF); nbits -= 8; }
  }
  return new Uint8Array(out);
}

// ── Compute TOTP code (RFC 6238, HMAC-SHA-1) ─────────────────────
async function computeTotp(secret, unixTime) {
  const key = base32Decode(secret);
  if (!key.length) return null;

  const counter = Math.floor(unixTime / 30);
  const buf = new ArrayBuffer(8);
  const view = new DataView(buf);
  // Big-endian 64-bit counter
  view.setUint32(0, Math.floor(counter / 0x100000000) >>> 0, false);
  view.setUint32(4, counter >>> 0, false);

  try {
    const cryptoKey = await crypto.subtle.importKey(
      'raw', key.buffer, { name: 'HMAC', hash: 'SHA-1' }, false, ['sign']
    );
    const sig    = await crypto.subtle.sign('HMAC', cryptoKey, buf);
    const hash   = new Uint8Array(sig);
    const offset = hash[19] & 0xF;
    const code   = ((hash[offset] & 0x7F) << 24 | hash[offset+1] << 16 |
                     hash[offset+2] << 8  | hash[offset+3]) % 1000000;
    return String(code).padStart(6, '0');
  } catch { return null; }
}

let _totpData      = null;
let _totpInterval  = null;
let _totpSecretMap = {}; // name → secret (only held in browser memory)

async function loadTotp() {
  // Sync device clock
  const ts = Math.floor(Date.now() / 1000);
  const syncEl = document.getElementById('totp-sync-status');
  const r = await apiPost('/api/totp/sync-time', { timestamp: String(ts) });
  if (syncEl) syncEl.textContent = r.ok ? 'Time synced ✓' : 'Sync failed';

  // Fetch account list
  const data = await api('/api/totp');
  if (!data.ok) return;
  _totpData = data;

  renderTotpList();

  // Live update every second
  if (_totpInterval) clearInterval(_totpInterval);
  _totpInterval = setInterval(updateTotpCodes, 1000);
  updateTotpCodes();
}

function renderTotpList() {
  const el = document.getElementById('totp-accounts-list');
  if (!_totpData || !_totpData.accounts.length) {
    el.innerHTML = '<p class="text-muted" style="margin-bottom:12px">No accounts yet. Add one below.</p>';
    return;
  }

  el.innerHTML = '';
  _totpData.accounts.forEach((acct, i) => {
    const card = document.createElement('div');
    card.className = 'totp-card';
    card.dataset.index = i;

    const nameEl = document.createElement('div');
    nameEl.className = 'totp-name';
    nameEl.textContent = acct.name;

    // Secret input (inline, only needed for live preview in browser)
    const secWrap = document.createElement('div');
    secWrap.className = 'totp-secret-row';
    const secInput = document.createElement('input');
    secInput.type = 'text';
    secInput.className = 'input totp-secret-input';
    secInput.placeholder = 'Paste secret for live preview';
    secInput.autocomplete = 'off';
    secInput.value = _totpSecretMap[acct.name] || '';
    secInput.addEventListener('input', () => {
      _totpSecretMap[acct.name] = secInput.value.trim();
      updateTotpCodes();
    });
    const secLabel = document.createElement('span');
    secLabel.className = 'label-hint';
    secLabel.textContent = 'Secret (browser only — not sent to device)';
    secWrap.append(secLabel, secInput);

    // Code display
    const codeEl = document.createElement('div');
    codeEl.className = 'totp-code';
    codeEl.dataset.name = acct.name;
    codeEl.textContent = '--- ---';

    // Progress bar
    const barWrap = document.createElement('div');
    barWrap.className = 'totp-bar-wrap';
    const bar = document.createElement('div');
    bar.className = 'totp-bar';
    bar.dataset.name = acct.name;
    barWrap.appendChild(bar);

    const timerEl = document.createElement('span');
    timerEl.className = 'totp-timer';
    timerEl.dataset.name = acct.name;
    timerEl.textContent = '30s';

    const barRow = document.createElement('div');
    barRow.className = 'totp-bar-row';
    barRow.append(barWrap, timerEl);

    // Delete button
    const delBtn = document.createElement('button');
    delBtn.className = 'btn';
    delBtn.textContent = 'Remove';
    delBtn.addEventListener('click', async () => {
      setBtn(delBtn, '…', true);
      const res = await apiPost('/api/totp/delete', { index: String(i) });
      if (res.ok) {
        delete _totpSecretMap[acct.name];
        await loadTotp();
      } else {
        setBtn(delBtn, 'Remove', false);
        toast('Delete failed.', 'err');
      }
    });

    card.append(nameEl, secWrap, codeEl, barRow, delBtn);
    el.appendChild(card);
  });
}

async function updateTotpCodes() {
  if (!_totpData) return;
  const now = Math.floor(Date.now() / 1000);
  const secsLeft = 29 - (now % 30);
  const pct = ((secsLeft + 1) / 30) * 100;

  for (const acct of _totpData.accounts) {
    const secret = _totpSecretMap[acct.name] || '';
    const codeEl  = document.querySelector(`.totp-code[data-name="${CSS.escape(acct.name)}"]`);
    const barEl   = document.querySelector(`.totp-bar[data-name="${CSS.escape(acct.name)}"]`);
    const timerEl = document.querySelector(`.totp-timer[data-name="${CSS.escape(acct.name)}"]`);

    if (timerEl) timerEl.textContent = (secsLeft + 1) + 's';
    if (barEl)   barEl.style.width = pct + '%';

    if (codeEl) {
      if (secret) {
        const code = await computeTotp(secret, now);
        codeEl.textContent = code ? (code.slice(0,3) + ' ' + code.slice(3)) : '--- ---';
      } else {
        codeEl.textContent = '--- ---';
      }
    }
  }
}

document.getElementById('totp-add-btn').addEventListener('click', async () => {
  const btn    = document.getElementById('totp-add-btn');
  const name   = document.getElementById('totp-name').value.trim();
  const secret = document.getElementById('totp-secret').value.trim().replace(/\s/g, '');

  if (!name || !secret) { toast('Fill in name and secret.', 'err'); return; }

  setBtn(btn, 'Adding…', true);
  const res = await apiPost('/api/totp/add', { name, secret });
  setBtn(btn, 'Add Account', false);

  if (res.ok) {
    _totpSecretMap[name] = secret;
    document.getElementById('totp-name').value   = '';
    document.getElementById('totp-secret').value = '';
    toast('Account added.', 'ok');
    await loadTotp();
  } else {
    toast(res.error || 'Failed — check secret format.', 'err');
  }
});

/* ── Init ──────────────────────────────────────────────────────── */

// Sync device clock silently on every portal load — keeps TOTP working
// after reboots without requiring the user to open the TOTP tab first.
(async () => {
  await apiPost('/api/totp/sync-time', { timestamp: String(Math.floor(Date.now() / 1000)) });
})();

loadSystem();
loadAutostart();
