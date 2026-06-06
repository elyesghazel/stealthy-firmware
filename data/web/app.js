/* ── Helpers ───────────────────────────────────────────────────── */

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

function setFeedback(el, msg, isError = false) {
  el.textContent = msg;
  el.classList.toggle('err', isError);
  if (msg) setTimeout(() => { if (el.textContent === msg) el.textContent = ''; }, 4000);
}

function setBtn(btn, text, disabled) {
  btn.textContent = text;
  btn.disabled    = disabled;
}

/* ── Tab switching ─────────────────────────────────────────────── */

const TAB_LOADERS = {
  system:    loadSystem,
  badge:     loadBadge,
  'ir-lib':  loadIrLib,
  'ir-upload': loadIrUpload,
  wifi:      loadWifi
};

document.querySelectorAll('.tab').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(t => {
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

/* ── System tab ────────────────────────────────────────────────── */

async function loadSystem() {
  const [bat, status] = await Promise.all([
    api('/api/battery'),
    api('/api/status')
  ]);

  if (bat.ok) {
    const v = Number(bat.voltage).toFixed(2) + ' V';
    document.getElementById('sys-battery').textContent  = v;
    document.getElementById('battery-top').textContent  = v;
  }

  if (status.ok) {
    document.getElementById('sys-status').textContent = status.portal || 'active';
    document.getElementById('sys-ssid').textContent   = status.ssid   || '--';
    document.getElementById('sys-ip').textContent     = status.ip     || '--';
  }
}

// Auto-refresh battery every 5 s
setInterval(async () => {
  const bat = await api('/api/battery');
  if (bat.ok) {
    const v = Number(bat.voltage).toFixed(2) + ' V';
    document.getElementById('sys-battery').textContent = v;
    document.getElementById('battery-top').textContent = v;
  }
}, 5000);

/* ── Badge tab ─────────────────────────────────────────────────── */

async function loadBadge() {
  const data = await api('/api/settings');
  if (!data.ok) return;

  const nameInput    = document.getElementById('badge-name');
  const statusSel    = document.getElementById('badge-status');
  const taglineInput = document.getElementById('badge-tagline');
  const qrInput      = document.getElementById('badge-qr');
  const modeSel      = document.getElementById('badge-mode');
  const topLabel     = document.getElementById('badge-name-top');

  nameInput.value    = data.badgeName    || '';
  taglineInput.value = data.badgeTagline || '';
  qrInput.value      = data.badgeQrData  || '';

  if (topLabel) topLabel.textContent = data.badgeName || 'stealthy';

  const sval = data.badgeStatus || 'Online';
  const sopt = [...statusSel.options].find(o => o.value.toLowerCase() === sval.toLowerCase());
  statusSel.value = sopt ? sopt.value : 'Online';

  const mval = String(data.badgeMode ?? 0);
  const mopt = [...modeSel.options].find(o => o.value === mval);
  modeSel.value = mopt ? mopt.value : '0';
}

document.getElementById('badge-form').addEventListener('submit', async e => {
  e.preventDefault();
  const btn      = document.getElementById('badge-save-btn');
  const feedback = document.getElementById('badge-feedback');
  const name     = document.getElementById('badge-name').value.trim();
  const status   = document.getElementById('badge-status').value;
  const tagline  = document.getElementById('badge-tagline').value.trim();
  const qrData   = document.getElementById('badge-qr').value.trim();
  const mode     = document.getElementById('badge-mode').value;

  setBtn(btn, 'Saving…', true);
  const data = await apiPost('/api/settings', {
    badgeName: name, badgeStatus: status,
    badgeTagline: tagline, badgeQrData: qrData, badgeMode: mode
  });
  setBtn(btn, 'Save', false);

  if (data.ok) {
    // Reload from device to confirm what was actually stored
    const topLabel = document.getElementById('badge-name-top');
    const confirmed = await api('/api/settings');
    if (confirmed.ok) {
      if (topLabel) topLabel.textContent = confirmed.badgeName || 'stealthy';
      document.getElementById('badge-name').value    = confirmed.badgeName    || '';
      document.getElementById('badge-tagline').value = confirmed.badgeTagline || '';
      document.getElementById('badge-qr').value      = confirmed.badgeQrData  || '';
      const mval = String(confirmed.badgeMode ?? 0);
      const modeSel = document.getElementById('badge-mode');
      const mopt = [...modeSel.options].find(o => o.value === mval);
      modeSel.value = mopt ? mopt.value : '0';
      setFeedback(feedback, 'Saved.');
    } else {
      setFeedback(feedback, 'Saved (verify failed).', false);
    }
  } else {
    setFeedback(feedback, 'Save failed.', true);
  }
});

/* ── IR Library tab ────────────────────────────────────────────── */

async function loadIrLib() {
  const root = document.getElementById('ir-list');
  root.innerHTML = '<div class="list-placeholder">Loading…</div>';

  const data = await api('/api/ir/list');

  if (!data.ok || !data.items) {
    root.innerHTML = '<div class="list-placeholder">Failed to load.</div>';
    return;
  }

  if (data.items.length === 0) {
    root.innerHTML = '<div class="list-placeholder">No saved IR captures.</div>';
    return;
  }

  root.innerHTML = '';
  data.items.forEach(item => root.appendChild(buildIrRow(item)));
}

function buildIrRow(item) {
  const row = document.createElement('div');
  row.className = 'ir-row';
  row.dataset.id = item.id;

  const info = document.createElement('div');
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
  const delBtn    = makeBtn('Delete', () => irDelete(item.id, item.name, row), true);

  actions.append(sendBtn, renameBtn, delBtn);
  row.append(info, actions);
  return row;
}

function makeBtn(label, onClick, isDanger = false) {
  const b = document.createElement('button');
  b.className = 'btn-sm' + (isDanger ? ' btn-danger' : '');
  b.textContent = label;
  b.addEventListener('click', onClick);
  return b;
}

async function irSend(id, btn, row) {
  setBtn(btn, '…', true);
  const data = await apiPost('/api/ir/send', { id });
  setBtn(btn, 'Send', false);
  showRowFeedback(row, data.ok ? 'Sent.' : 'Send failed.', !data.ok);
}

async function irDelete(id, name, row) {
  const confirmed = await showConfirm(row, `Delete "${name}"?`);
  if (!confirmed) return;

  const btn = row.querySelector('.btn-danger');
  if (btn) setBtn(btn, '…', true);

  const data = await apiPost('/api/ir/delete', { id });
  if (data.ok) {
    row.remove();
    const list = document.getElementById('ir-list');
    if (!list.children.length) {
      list.innerHTML = '<div class="list-placeholder">No saved IR captures.</div>';
    }
  } else {
    if (btn) setBtn(btn, 'Delete', false);
    showRowFeedback(row, 'Delete failed.', true);
  }
}

function startRename(row, item) {
  const info    = row.querySelector('.ir-info');
  const nameEl  = row.querySelector('.ir-name');
  const actions = row.querySelector('.ir-actions');

  // Swap name → input
  const input = document.createElement('input');
  input.type      = 'text';
  input.value     = item.name;
  input.className = 'rename-input';
  input.maxLength = 32;
  nameEl.replaceWith(input);
  input.focus();
  input.select();

  // Swap actions → save / cancel
  const origActions = actions.cloneNode(true); // keep for cancel restore
  actions.innerHTML = '';

  const saveBtn   = makeBtn('Save',   async () => {
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
      const newSend   = makeBtn('Send',   () => irSend(item.id, newSend, row));
      const newRename = makeBtn('Rename', () => startRename(row, item));
      const newDel    = makeBtn('Delete', () => irDelete(item.id, item.name, row), true);
      actions.append(newSend, newRename, newDel);
    } else {
      setBtn(saveBtn, 'Error', false);
      setTimeout(() => setBtn(saveBtn, 'Save', false), 1500);
    }
  });

  const cancelBtn = makeBtn('Cancel', () => {
    // Restore original name div
    const restored = document.createElement('div');
    restored.className = 'ir-name';
    restored.textContent = item.name;
    input.replaceWith(restored);

    // Restore original actions
    actions.innerHTML = '';
    const s = makeBtn('Send',   () => irSend(item.id, s, row));
    const r = makeBtn('Rename', () => startRename(row, item));
    const d = makeBtn('Delete', () => irDelete(item.id, item.name, row), true);
    actions.append(s, r, d);
  });

  actions.append(saveBtn, cancelBtn);
}

function showRowFeedback(row, msg, isError = false) {
  let fb = row.querySelector('.row-feedback');
  if (!fb) {
    fb = document.createElement('div');
    fb.className = 'inline-feedback row-feedback';
    fb.style.width = '100%';
    row.appendChild(fb);
  }
  setFeedback(fb, msg, isError);
}

// Inline confirm: replaces action buttons temporarily.
// Saves original DOM *nodes* (not innerHTML) so event listeners are preserved on restore.
function showConfirm(row, msg) {
  return new Promise(resolve => {
    const actions = row.querySelector('.ir-actions') || row.querySelector('.file-actions');
    if (!actions) { resolve(true); return; }

    // Keep live DOM references — event listeners survive the swap
    const origNodes = Array.from(actions.children);

    const label = document.createElement('span');
    label.className = 'ir-sub';
    label.textContent = msg;

    const restore = () => {
      actions.innerHTML = '';
      origNodes.forEach(n => actions.appendChild(n));
    };

    const yes = makeBtn('Yes', () => { restore(); resolve(true); }, true);
    const no  = makeBtn('No',  () => { restore(); resolve(false); });

    actions.innerHTML = '';
    actions.append(label, yes, no);
  });
}

/* ── IR Upload tab ─────────────────────────────────────────────── */

const UPLOAD_PATH = '/ir/uploads';

function loadIrUpload() {
  loadUploadList();
}

const dropZone    = document.getElementById('drop-zone');
const uploadInput = document.getElementById('upload-input');

dropZone.addEventListener('click', () => uploadInput.click());
dropZone.addEventListener('keydown', e => { if (e.key === 'Enter' || e.key === ' ') uploadInput.click(); });

dropZone.addEventListener('dragover', e => { e.preventDefault(); dropZone.classList.add('dragover'); });
dropZone.addEventListener('dragleave', ()  => dropZone.classList.remove('dragover'));
dropZone.addEventListener('drop', e => {
  e.preventDefault();
  dropZone.classList.remove('dragover');
  const files = e.dataTransfer.files;
  if (files.length) uploadFiles(files);
});

uploadInput.addEventListener('change', () => {
  if (uploadInput.files.length) uploadFiles(uploadInput.files);
  uploadInput.value = '';
});

function uploadFiles(files) {
  Array.from(files).forEach(file => uploadSingleFile(file));
}

function uploadSingleFile(file) {
  const wrap    = document.getElementById('upload-progress-wrap');
  const bar     = document.getElementById('upload-progress-bar');
  const label   = document.getElementById('upload-progress-label');
  const feedback = document.getElementById('upload-feedback');

  wrap.classList.remove('hidden');
  bar.style.setProperty('--pct', '0%');
  label.textContent = '0%';

  const fd = new FormData();
  fd.append('file', file);

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/file/upload?path=' + encodeURIComponent(UPLOAD_PATH));

  xhr.upload.addEventListener('progress', e => {
    if (!e.lengthComputable) return;
    const pct = Math.round(e.loaded / e.total * 100);
    bar.style.setProperty('--pct', pct + '%');
    label.textContent = pct + '%';
  });

  xhr.addEventListener('load', () => {
    wrap.classList.add('hidden');
    try {
      const data = JSON.parse(xhr.responseText);
      if (data.ok) {
        setFeedback(feedback, file.name + ' uploaded.');
        loadUploadList();
      } else {
        setFeedback(feedback, 'Upload failed: server error.', true);
      }
    } catch {
      setFeedback(feedback, 'Upload failed: bad response.', true);
    }
  });

  xhr.addEventListener('error', () => {
    wrap.classList.add('hidden');
    setFeedback(feedback, 'Upload failed: network error.', true);
  });

  xhr.send(fd);
}

async function loadUploadList() {
  const root = document.getElementById('upload-list');
  root.innerHTML = '<div class="list-placeholder">Loading…</div>';

  const data = await api('/api/fs/list?path=' + encodeURIComponent(UPLOAD_PATH));

  if (!data.ok) {
    root.innerHTML = '<div class="list-placeholder">Failed to load.</div>';
    return;
  }

  const files = (data.entries || []).filter(e => !e.isDirectory);
  if (!files.length) {
    root.innerHTML = '<div class="list-placeholder">No uploaded files.</div>';
    return;
  }

  root.innerHTML = '';
  files.forEach(entry => root.appendChild(buildFileRow(entry)));
}

async function irImport(path, btn, row) {
  setBtn(btn, '…', true);
  const data = await apiPost('/api/ir/import', { path });
  setBtn(btn, 'Import', false);
  if (data.ok) {
    showRowFeedback(row, 'Imported ' + data.count + ' signal(s).', false);
    // Refresh IR library if currently active
    const irPane = document.getElementById('pane-ir-lib');
    if (irPane && irPane.classList.contains('active')) loadIrLib();
  } else {
    showRowFeedback(row, 'Import failed.', true);
  }
}

function buildFileRow(entry) {
  const row = document.createElement('div');
  row.className = 'file-row';

  const name = document.createElement('div');
  name.className = 'file-name';
  name.textContent = entry.name;

  const acts = document.createElement('div');
  acts.className = 'file-actions';

  // Import button for Flipper .ir files
  if (entry.name.toLowerCase().endsWith('.ir')) {
    const importBtn = makeBtn('Import to Library', () => irImport(entry.path, importBtn, row));
    acts.append(importBtn);
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
        list.innerHTML = '<div class="list-placeholder">No uploaded files.</div>';
    } else {
      setBtn(delBtn, 'Delete', false);
    }
  }, true);

  acts.append(dlBtn, delBtn);
  row.append(name, acts);
  return row;
}

/* ── WiFi tab ──────────────────────────────────────────────────── */

async function loadWifi() {
  const data = await api('/api/wifi/ssids');
  const ta   = document.getElementById('ssid-textarea');
  const badge = document.getElementById('ssid-count-badge');

  if (data.ok && data.ssids) {
    ta.value = data.ssids.join('\n');
    badge.textContent = data.ssids.length + ' SSIDs';
  }
}

document.getElementById('ssid-save-btn').addEventListener('click', async () => {
  const btn      = document.getElementById('ssid-save-btn');
  const feedback = document.getElementById('wifi-feedback');
  const badge    = document.getElementById('ssid-count-badge');
  const raw      = document.getElementById('ssid-textarea').value;

  const ssids = raw.split('\n').map(s => s.trim()).filter(Boolean);
  if (ssids.length > 20) {
    setFeedback(feedback, 'Max 20 SSIDs allowed.', true);
    return;
  }

  setBtn(btn, 'Saving…', true);
  const data = await apiPost('/api/wifi/ssids', { ssids: ssids.join('\n') });
  setBtn(btn, 'Save SSIDs', false);

  if (data.ok) {
    badge.textContent = ssids.length + ' SSIDs';
    setFeedback(feedback, 'SSID list saved.');
  } else {
    setFeedback(feedback, 'Save failed.', true);
  }
});

/* ── Init ──────────────────────────────────────────────────────── */

loadSystem();
