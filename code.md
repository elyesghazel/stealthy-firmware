Yes — let’s do the next real step properly:

You’ll get:

- folder/file listing for LittleFS
- file download/export
- file upload/import
- simple browser UI in the portal for `/ir`

This gives you a scalable base for:

- uploading database `.ir` files
- organizing them in folders
- downloading/exporting files back to your PC
- later parsing/sending imported Flipper-style files

---

# 1) Extend `FileSystemDriver`

## `src/drivers/FileSystemDriver.h`

Replace with this version:

```cpp
#pragma once

#include <Arduino.h>
#include <vector>

struct FileEntry {
    String name;
    String path;
    bool isDirectory;
};

class FileSystemDriver {
public:
    bool begin();
    bool isMounted() const;

    bool exists(const char* path) const;
    String readTextFile(const char* path) const;
    bool writeTextFile(const char* path, const String& content);
    std::vector<String> listFiles(const char* path) const;
    std::vector<FileEntry> listEntries(const char* path) const;
    bool removeFile(const char* path);
    bool ensureDir(const char* path);

private:
    bool _mounted = false;
};
```

---

## `src/drivers/FileSystemDriver.cpp`

Replace with this version:

```cpp
#include "FileSystemDriver.h"
#include <LittleFS.h>

bool FileSystemDriver::begin() {
    _mounted = LittleFS.begin(true);
    return _mounted;
}

bool FileSystemDriver::isMounted() const {
    return _mounted;
}

bool FileSystemDriver::exists(const char* path) const {
    if (!_mounted) return false;
    return LittleFS.exists(path);
}

String FileSystemDriver::readTextFile(const char* path) const {
    if (!_mounted) return "";

    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        return "";
    }

    String content;
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}

bool FileSystemDriver::writeTextFile(const char* path, const String& content) {
    if (!_mounted) return false;

    File file = LittleFS.open(path, "w");
    if (!file) {
        return false;
    }

    file.print(content);
    file.close();
    return true;
}

std::vector<String> FileSystemDriver::listFiles(const char* path) const {
    std::vector<String> files;

    if (!_mounted) return files;

    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
        return files;
    }

    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            files.push_back(String(entry.name()));
        }
        entry.close();
        entry = dir.openNextFile();
    }

    dir.close();
    return files;
}

std::vector<FileEntry> FileSystemDriver::listEntries(const char* path) const {
    std::vector<FileEntry> entries;

    if (!_mounted) return entries;

    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
        return entries;
    }

    File entry = dir.openNextFile();
    while (entry) {
        FileEntry item;
        item.path = String(entry.name());

        int slash = item.path.lastIndexOf('/');
        item.name = (slash >= 0) ? item.path.substring(slash + 1) : item.path;
        item.isDirectory = entry.isDirectory();

        entries.push_back(item);

        entry.close();
        entry = dir.openNextFile();
    }

    dir.close();
    return entries;
}

bool FileSystemDriver::removeFile(const char* path) {
    if (!_mounted) return false;
    return LittleFS.remove(path);
}

bool FileSystemDriver::ensureDir(const char* path) {
    if (!_mounted) return false;

    if (LittleFS.exists(path)) {
        File f = LittleFS.open(path, "r");
        bool ok = f && f.isDirectory();
        f.close();
        return ok;
    }

    return LittleFS.mkdir(path);
}
```

---

# 2) Extend `StorageManager`

## `src/core/StorageManager.h`

Add these methods:

```cpp
std::vector<FileEntry> listFsEntries(const String& path) const;
bool ensureDirectory(const String& path);
```

So the public section includes:

```cpp
    std::vector<String> listPayloadFiles() const;
    String readPayloadFile(const String& filename) const;

    std::vector<FileEntry> listFsEntries(const String& path) const;
    bool ensureDirectory(const String& path);

    // IR storage
    bool saveIrCapture(const IrCapture& capture);
```

---

## `src/core/StorageManager.cpp`

Add these methods:

```cpp
std::vector<FileEntry> StorageManager::listFsEntries(const String& path) const {
    if (!_available) return {};
    return _fileSystemDriver->listEntries(path.c_str());
}

bool StorageManager::ensureDirectory(const String& path) {
    if (!_available) return false;
    return _fileSystemDriver->ensureDir(path.c_str());
}
```

Also in `begin()` you should create the folders you need:

```cpp
bool StorageManager::begin() {
    if (_fileSystemDriver == nullptr) {
        _available = false;
        return false;
    }

    _available = _fileSystemDriver->begin();
    if (!_available) {
        return false;
    }

    _fileSystemDriver->ensureDir("/config");
    _fileSystemDriver->ensureDir("/payloads");
    _fileSystemDriver->ensureDir("/ir");
    _fileSystemDriver->ensureDir("/ir/saved");
    _fileSystemDriver->ensureDir("/ir/db");

    return true;
}
```

### Important

Also change your IR paths to save into `/ir/saved` instead of `/ir`.

In `StorageManager.h`, change:

```cpp
static constexpr const char* IR_DIR = "/ir";
```

to:

```cpp
static constexpr const char* IR_DIR = "/ir/saved";
```

That keeps learned captures separate from imported database files.

---

# 3) Extend `PortalManager`

## `src/core/PortalManager.h`

Add these handlers:

```cpp
void handleApiFsList();
void handleApiFileDownload();
void handleApiFileUpload();
```

So the private section includes:

```cpp
    void handleApiIrList();
    void handleApiIrSend();
    void handleApiIrRename();
    void handleApiIrDelete();

    void handleApiFsList();
    void handleApiFileDownload();
    void handleApiFileUpload();
```

---

## `src/core/PortalManager.cpp`

### Add routes in `setupRoutes()`

Add:

```cpp
_server.on("/api/fs/list", HTTP_GET, [this]() { handleApiFsList(); });
_server.on("/api/file/download", HTTP_GET, [this]() { handleApiFileDownload(); });
_server.on("/api/file/upload", HTTP_POST,
    [this]() { _server.send(200, "application/json", "{\"ok\":true}"); },
    [this]() { handleApiFileUpload(); }
);
```

So that part becomes:

```cpp
void PortalManager::setupRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/style.css", HTTP_GET, [this]() { handleCss(); });
    _server.on("/app.js", HTTP_GET, [this]() { handleJs(); });

    _server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    _server.on("/api/battery", HTTP_GET, [this]() { handleApiBattery(); });
    _server.on("/api/settings", HTTP_GET, [this]() { handleApiSettingsGet(); });
    _server.on("/api/settings", HTTP_POST, [this]() { handleApiSettingsPost(); });

    _server.on("/api/ir/list", HTTP_GET, [this]() { handleApiIrList(); });
    _server.on("/api/ir/send", HTTP_POST, [this]() { handleApiIrSend(); });
    _server.on("/api/ir/rename", HTTP_POST, [this]() { handleApiIrRename(); });
    _server.on("/api/ir/delete", HTTP_POST, [this]() { handleApiIrDelete(); });

    _server.on("/api/fs/list", HTTP_GET, [this]() { handleApiFsList(); });
    _server.on("/api/file/download", HTTP_GET, [this]() { handleApiFileDownload(); });
    _server.on("/api/file/upload", HTTP_POST,
        [this]() { _server.send(200, "application/json", "{\"ok\":true}"); },
        [this]() { handleApiFileUpload(); }
    );

    _server.onNotFound([this]() {
        if (!serveFile("/web/index.html")) {
            _server.send(404, "text/plain", "Not found");
        }
    });
}
```

---

### Add helper for basic path validation

Add this near the top of `PortalManager.cpp` if you want a tiny helper:

```cpp
static bool isSafeFsPath(const String& path) {
    return !path.isEmpty() &&
           path.startsWith("/") &&
           path.indexOf("..") < 0;
}
```

---

### Add `handleApiFsList()`

```cpp
void PortalManager::handleApiFsList() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String path = _server.arg("path");
    if (path.isEmpty()) {
        path = "/ir";
    }

    if (!isSafeFsPath(path)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_path\"}");
        return;
    }

    std::vector<FileEntry> entries = _storageManager->listFsEntries(path);

    // sort dirs first, then by name
    for (size_t i = 0; i < entries.size(); i++) {
        for (size_t j = i + 1; j < entries.size(); j++) {
            bool swapNeeded = false;

            if (entries[j].isDirectory && !entries[i].isDirectory) {
                swapNeeded = true;
            } else if (entries[j].isDirectory == entries[i].isDirectory &&
                       entries[j].name < entries[i].name) {
                swapNeeded = true;
            }

            if (swapNeeded) {
                FileEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    String json = "{";
    json += "\"ok\":true,";
    json += "\"path\":\"" + path + "\",";
    json += "\"entries\":[";

    for (size_t i = 0; i < entries.size(); i++) {
        String name = entries[i].name;
        String fullPath = entries[i].path;

        name.replace("\"", "\\\"");
        fullPath.replace("\"", "\\\"");

        json += "{";
        json += "\"name\":\"" + name + "\",";
        json += "\"path\":\"" + fullPath + "\",";
        json += "\"isDirectory\":";
        json += entries[i].isDirectory ? "true" : "false";
        json += "}";

        if (i + 1 < entries.size()) {
            json += ",";
        }
    }

    json += "]";
    json += "}";

    _server.send(200, "application/json", json);
}
```

---

### Add `handleApiFileDownload()`

```cpp
void PortalManager::handleApiFileDownload() {
    String path = _server.arg("path");

    if (!isSafeFsPath(path)) {
        _server.send(400, "text/plain", "Invalid path");
        return;
    }

    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        _server.send(404, "text/plain", "Not found");
        return;
    }

    String filename = path.substring(path.lastIndexOf('/') + 1);
    _server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    _server.streamFile(file, "application/octet-stream");
    file.close();
}
```

---

### Add `handleApiFileUpload()`

```cpp
void PortalManager::handleApiFileUpload() {
    HTTPUpload& upload = _server.upload();

    static File uploadFile;
    static String uploadPath;

    if (upload.status == UPLOAD_FILE_START) {
        String dir = _server.arg("path");
        if (dir.isEmpty()) {
            dir = "/ir/db";
        }

        if (!isSafeFsPath(dir)) {
            return;
        }

        if (_storageManager) {
            _storageManager->ensureDirectory(dir);
        }

        String filename = upload.filename;
        filename.replace("\\", "_");
        filename.replace("/", "_");

        uploadPath = dir + "/" + filename;

        Serial.print("[Portal] upload start: ");
        Serial.println(uploadPath);

        uploadFile = LittleFS.open(uploadPath, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
        }

        Serial.print("[Portal] upload done: ");
        Serial.println(uploadPath);
    }
}
```

---

# 4) Update portal frontend

## `data/web/index.html`

Add a new card below the IR captures card:

```html
<section class="card">
  <h2>IR File Browser</h2>
  <div id="fsPath" class="result">/ir</div>

  <form id="uploadForm">
    <input id="uploadFile" type="file" name="file" />
    <button type="submit">Upload Here</button>
  </form>

  <div id="fsList" class="list">Loading...</div>
</section>
```

---

## `data/web/style.css`

Add:

```css
.file-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  padding: 12px;
  border: 1px solid var(--border);
  border-radius: 12px;
  background: var(--panel-2);
}

.file-meta {
  min-width: 0;
}

.file-name {
  font-weight: 600;
  word-break: break-word;
}

.file-path {
  color: var(--muted);
  font-size: 0.9rem;
  margin-top: 4px;
}

.file-actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

#uploadForm {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
  margin-bottom: 14px;
}
```

---

## `data/web/app.js`

Replace or extend with this full set for file browser support.

Add near the top:

```javascript
let currentFsPath = "/ir";
```

Add these functions:

```javascript
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
```

Then near the bottom add:

```javascript
document
  .getElementById("uploadForm")
  .addEventListener("submit", uploadCurrentFile);
```

And call it once:

```javascript
loadFsList();
```

So your bottom init section becomes:

```javascript
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
```

---

# 5) Create initial folders in `data/`

Make sure your uploaded filesystem has these directories.

Create:

```text
data/ir/saved/.keep
data/ir/db/.keep
```

If `.keep` causes issues, use `readme.txt` files instead, for example:

```text
data/ir/saved/readme.txt
data/ir/db/readme.txt
```

with tiny content like:

```text
saved ir captures
```

and

```text
imported ir files
```

---

# 6) What this gives you

Now in the portal you can:

- browse `/ir`
- open `/ir/db`
- upload imported `.ir` files there
- browse folders
- download/export files back out

That already solves your “I don’t always have the remote” problem in a practical way:

- get projector/beamer/TV `.ir` files from a database
- upload them to the badge
- keep them in folders

---

# 7) Important note

Right now this **does not yet parse and send uploaded Flipper multi-command files**.

It only lets you:

- import them
- store them
- export them
- organize them

That is still a very good next step.

The next stage after this is:

- parse a selected `.ir` file
- extract command names
- send chosen command

That’s a bigger feature, but now you’ll have the file-management foundation for it.

---

# 8) Upload steps

After adding these files/code:

```bash
pio run -t uploadfs
pio run -t upload
```

---

If you want, the next step after this should be:
**parse Flipper-style `.ir` files and show the commands inside one uploaded file**.
