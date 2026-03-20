let currentFsPath = "/ir";

async function fetchJson(url, options = {}) {
  const res = await fetch(url, options);
  return await res.json();
}

async function loadStatus() {
  const data = await fetchJson("/api/status");
  const el = document.getElementById("status");
  el.textContent = `Portal: ${data.portal} | SSID: ${data.ssid} | IP: ${data.ip}`;
}

async function loadSettings() {
  const data = await fetchJson("/api/settings");
  document.getElementById("badgeName").value = data.badgeName || "";
  document.getElementById("badgeStatus").value = data.badgeStatus || "";
}

async function saveSettings(event) {
  event.preventDefault();

  const badgeName = document.getElementById("badgeName").value;
  const badgeStatus = document.getElementById("badgeStatus").value;

  const body = new URLSearchParams();
  body.set("badgeName", badgeName);
  body.set("badgeStatus", badgeStatus);

  const data = await fetchJson("/api/settings", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body,
  });

  document.getElementById("settingsResult").textContent = data.ok
    ? "Settings saved."
    : "Save failed.";
}

async function sendIr(id) {
  const body = new URLSearchParams();
  body.set("id", String(id));

  const data = await fetchJson("/api/ir/send", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body,
  });

  alert(data.ok ? "IR sent." : "Send failed.");
}

async function loadIrList() {
  const data = await fetchJson("/api/ir/list");
  const root = document.getElementById("irList");
  root.innerHTML = "";

  if (!data.items || data.items.length === 0) {
    root.textContent = "No saved IR captures.";
    return;
  }

  for (const item of data.items) {
    const row = document.createElement("div");
    row.className = "ir-item";

    const meta = document.createElement("div");
    meta.className = "ir-meta";

    const name = document.createElement("div");
    name.className = "ir-name";
    name.textContent = item.name;

    const id = document.createElement("div");
    id.className = "ir-id";
    id.textContent = `ID ${item.id} · ${item.fileId}`;

    meta.appendChild(name);
    meta.appendChild(id);

    const actions = document.createElement("div");
    actions.className = "ir-actions";

    const sendBtn = document.createElement("button");
    sendBtn.textContent = "Send";
    sendBtn.onclick = () => sendIr(item.id);

    const renameBtn = document.createElement("button");
    renameBtn.textContent = "Rename";
    renameBtn.onclick = () => renameIr(item.id, item.name);

    const deleteBtn = document.createElement("button");
    deleteBtn.textContent = "Delete";
    deleteBtn.onclick = () => deleteIr(item.id, item.name);

    actions.appendChild(sendBtn);
    actions.appendChild(renameBtn);
    actions.appendChild(deleteBtn);

    row.appendChild(meta);
    row.appendChild(actions);
    root.appendChild(row);
  }
}

async function renameIr(id, currentName) {
  const newName = prompt("Rename capture:", currentName);
  if (newName === null) return;

  const body = new URLSearchParams();
  body.set("id", String(id));
  body.set("name", newName);

  const data = await fetchJson("/api/ir/rename", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body,
  });

  if (!data.ok) {
    alert("Rename failed.");
    return;
  }

  await loadIrList();
}

async function deleteIr(id, name) {
  const confirmed = confirm(`Delete "${name}"?`);
  if (!confirmed) return;

  const body = new URLSearchParams();
  body.set("id", String(id));

  const data = await fetchJson("/api/ir/delete", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body,
  });

  if (!data.ok) {
    alert("Delete failed.");
    return;
  }

  await loadIrList();
}

async function loadBattery() {
  const data = await fetchJson("/api/battery");
  const el = document.getElementById("battery");

  if (!data.ok) {
    el.textContent = "Battery: unavailable";
    return;
  }

  el.textContent = `Battery: ${Number(data.voltage).toFixed(2)} V`;
}

function parentPath(path) {
  if (!path || path === "/") return "/";
  const idx = path.lastIndexOf("/");
  if (idx <= 0) return "/";
  return path.substring(0, idx);
}

async function downloadFile(path) {
  window.location.href = `/api/file/download?path=${encodeURIComponent(path)}`;
}

async function openFolder(path) {
  currentFsPath = path;
  await loadFsList();
}

async function uploadCurrentFile(event) {
  event.preventDefault();

  const input = document.getElementById("uploadFile");
  if (!input.files || input.files.length === 0) {
    alert("Choose a file first.");
    return;
  }

  const formData = new FormData();
  formData.append("file", input.files[0]);

  const res = await fetch(
    `/api/file/upload?path=${encodeURIComponent(currentFsPath)}`,
    {
      method: "POST",
      body: formData,
    },
  );

  if (!res.ok) {
    alert("Upload failed.");
    return;
  }

  input.value = "";
  await loadFsList();
}

async function loadFsList() {
  const data = await fetchJson(
    `/api/fs/list?path=${encodeURIComponent(currentFsPath)}`,
  );
  const root = document.getElementById("fsList");
  const pathEl = document.getElementById("fsPath");

  pathEl.textContent = currentFsPath;
  root.innerHTML = "";

  if (currentFsPath !== "/ir") {
    const upRow = document.createElement("div");
    upRow.className = "file-row";

    const meta = document.createElement("div");
    meta.className = "file-meta";

    const name = document.createElement("div");
    name.className = "file-name";
    name.textContent = "..";

    const fp = document.createElement("div");
    fp.className = "file-path";
    fp.textContent = parentPath(currentFsPath);

    meta.appendChild(name);
    meta.appendChild(fp);

    const actions = document.createElement("div");
    actions.className = "file-actions";

    const openBtn = document.createElement("button");
    openBtn.textContent = "Up";
    openBtn.onclick = () => openFolder(parentPath(currentFsPath));

    actions.appendChild(openBtn);
    upRow.appendChild(meta);
    upRow.appendChild(actions);
    root.appendChild(upRow);
  }

  if (!data.entries || data.entries.length === 0) {
    const empty = document.createElement("div");
    empty.textContent = "Folder is empty.";
    root.appendChild(empty);
    return;
  }

  for (const entry of data.entries) {
    const row = document.createElement("div");
    row.className = "file-row";

    const meta = document.createElement("div");
    meta.className = "file-meta";

    const name = document.createElement("div");
    name.className = "file-name";
    name.textContent = entry.isDirectory ? `[DIR] ${entry.name}` : entry.name;

    const fp = document.createElement("div");
    fp.className = "file-path";
    fp.textContent = entry.path;

    meta.appendChild(name);
    meta.appendChild(fp);

    const actions = document.createElement("div");
    actions.className = "file-actions";

    if (entry.isDirectory) {
      const openBtn = document.createElement("button");
      openBtn.textContent = "Open";
      openBtn.onclick = () => openFolder(entry.path);
      actions.appendChild(openBtn);
    } else {
      const dlBtn = document.createElement("button");
      dlBtn.textContent = "Download";
      dlBtn.onclick = () => downloadFile(entry.path);
      actions.appendChild(dlBtn);
    }

    row.appendChild(meta);
    row.appendChild(actions);
    root.appendChild(row);
  }
}
document
  .getElementById("settingsForm")
  .addEventListener("submit", saveSettings);
document
  .getElementById("uploadForm")
  .addEventListener("submit", uploadCurrentFile);

loadStatus();
loadSettings();
loadBattery();
loadIrList();
loadFsList();

setInterval(loadBattery, 3000);
