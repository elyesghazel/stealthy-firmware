#include "PortalManager.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <esp_wifi.h>

#include "StorageManager.h"
#include "IrManager.h"
#include "PowerManager.h"
#include "../drivers/IrDriver.h"

PortalManager::PortalManager(StorageManager* storageManager, IrManager* irManager, PowerManager* powerManager)
    : _storageManager(storageManager),
      _irManager(irManager),
      _powerManager(powerManager),
      _server(80) {
}

static bool isSafeFsPath(const String& path) {
    return !path.isEmpty() &&
           path.startsWith("/") &&
           path.indexOf("..") < 0;
}


bool PortalManager::begin() {
    if (_running) {
        Serial.println("[Portal] already running");
        return true;
    }

    Serial.println("[Portal] starting...");
    Serial.flush();
    WiFi.mode(WIFI_AP);

    Serial.flush();
    bool apOk = WiFi.softAP(AP_SSID);
    Serial.flush();

    Serial.flush();
    IPAddress ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, subnet);

    Serial.flush();
    esp_wifi_set_max_tx_power(78); // 19.5 dBm, max for ESP32

    // Temporarily comment these out for isolation tests:
    Serial.flush();
    _dnsServer.start(53, "*", ip);

    Serial.flush();
    setupRoutes();

    Serial.flush();
    _server.begin();

    _running = true;

    Serial.println("[Portal] started");
    Serial.print("[Portal] SSID: ");
    Serial.println(AP_SSID);
    Serial.print("[Portal] IP: ");
    Serial.println(AP_IP);
    Serial.flush();

    return true;
}


void PortalManager::stop() {
    if (!_running) {
        return;
    }

    _server.stop();
    _dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    _running = false;

    Serial.println("[Portal] stopped");
}


void PortalManager::update() {
    if (!_running) {
        return;
    }

    _dnsServer.processNextRequest();
    _server.handleClient();
}

bool PortalManager::isRunning() const {
    return _running;
}

const char* PortalManager::ssid() const {
    return AP_SSID;
}

const char* PortalManager::ipAddress() const {
    return AP_IP;
}

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
void PortalManager::handleRoot() {
    serveFile("/web/index.html");
}

void PortalManager::handleCss() {
    serveFile("/web/style.css");
}

void PortalManager::handleJs() {
    serveFile("/web/app.js");
}

void PortalManager::handleApiBattery() {
    if (_powerManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    float voltage = _powerManager->readBatteryVoltage();

    String json = "{";
    json += "\"ok\":true,";
    json += "\"voltage\":";
    json += String(voltage, 2);
    json += "}";

    _server.send(200, "application/json", json);
}


void PortalManager::handleApiStatus() {
    String json = "{";
    json += "\"ok\":true,";
    json += "\"portal\":\"running\",";
    json += "\"ssid\":\"" + String(AP_SSID) + "\",";
    json += "\"ip\":\"" + String(AP_IP) + "\"";
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiSettingsGet() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String name = _storageManager->getBadgeName();
    String status = _storageManager->getBadgeStatus();

    name.replace("\"", "\\\"");
    status.replace("\"", "\\\"");

    String json = "{";
    json += "\"ok\":true,";
    json += "\"badgeName\":\"" + name + "\",";
    json += "\"badgeStatus\":\"" + status + "\"";
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiSettingsPost() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String name = _server.arg("badgeName");
    String status = _server.arg("badgeStatus");

    bool ok1 = _storageManager->setBadgeName(name);
    bool ok2 = _storageManager->setBadgeStatus(status);

    String json = "{";
    json += "\"ok\":";
    json += (ok1 && ok2) ? "true" : "false";
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiIrList() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    std::vector<IrSavedItem> items = _storageManager->listIrCaptures();

    String json = "{";
    json += "\"ok\":true,";
    json += "\"items\":[";

    for (size_t i = 0; i < items.size(); i++) {
        String name = items[i].name;
        name.replace("\"", "\\\"");

        json += "{";
        json += "\"id\":" + String(items[i].id) + ",";
        json += "\"fileId\":\"" + items[i].fileId + "\",";
        json += "\"name\":\"" + name + "\"";
        json += "}";

        if (i + 1 < items.size()) {
            json += ",";
        }
    }

    json += "]";
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiIrSend() {
    if (_storageManager == nullptr || _irManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    int id = _server.arg("id").toInt();
    IrCapture capture;

    if (!_storageManager->loadIrCaptureById(id, capture)) {
        _server.send(404, "application/json", "{\"ok\":false,\"error\":\"not_found\"}");
        return;
    }

    _irManager->setLastCapture(capture);
    bool ok = _irManager->replayLast();

    String json = "{";
    json += "\"ok\":";
    json += ok ? "true" : "false";
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiIrRename() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    int id = _server.arg("id").toInt();
    String name = _server.arg("name");

    bool ok = _storageManager->renameIrCaptureById(id, name);

    String json = "{";
    json += "\"ok\":";
    json += ok ? "true" : "false";
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiIrDelete() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    int id = _server.arg("id").toInt();
    bool ok = _storageManager->deleteIrCaptureById(id);

    String json = "{";
    json += "\"ok\":";
    json += ok ? "true" : "false";
    json += "}";

    _server.send(200, "application/json", json);
}




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


String PortalManager::contentTypeForPath(const String& path) const {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".js")) return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    return "text/plain";
}

bool PortalManager::serveFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
        return false;
    }

    _server.streamFile(file, contentTypeForPath(path));
    file.close();
    return true;
}
