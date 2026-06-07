#include "PortalManager.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <esp_wifi.h>

#include "storage/StorageManager.h"
#include "ir/IrManager.h"
#include "power/PowerManager.h"
#include "ir/IrDriver.h"
#include "ir/FlipperIrSerializer.h"
#include "wifi/WifiKarma.h"

PortalManager::PortalManager(StorageManager* storageManager, IrManager* irManager,
                             PowerManager* powerManager, WifiKarma* karmaManager,
                             DeviceSettings* deviceSettings)
    : _storageManager(storageManager),
      _irManager(irManager),
      _powerManager(powerManager),
      _karmaManager(karmaManager),
      _deviceSettings(deviceSettings),
      _server(80) {
}

static bool isSafeFsPath(const String& path) {
    return !path.isEmpty() &&
           path.startsWith("/") &&
           path.indexOf("..") < 0;
}

static bool tryParsePositiveIntArg(WebServer& server, const char* key, int& outValue) {
    if (!server.hasArg(key)) {
        return false;
    }

    String raw = server.arg(key);
    raw.trim();
    if (raw.isEmpty()) {
        return false;
    }

    for (size_t i = 0; i < raw.length(); i++) {
        if (!isDigit(raw[i])) {
            return false;
        }
    }

    long parsed = raw.toInt();
    if (parsed <= 0) {
        return false;
    }

    outValue = static_cast<int>(parsed);
    return true;
}


bool PortalManager::begin() {
    if (_running) {
        Serial.println("[Portal] already running");
        return true;
    }

    Serial.println("[Portal] starting...");
    Serial.flush();
    // AP_STA: portal AP coexists with STA-based tools (deauth, spammer, karma)
    WiFi.mode(WIFI_AP_STA);

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

    if (_powerManager && WiFi.softAPgetStationNum() > 0) {
        _powerManager->notifyUserActivity();
    }
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

    _server.on("/api/ir/list",   HTTP_GET,  [this]() { handleApiIrList();   });
    _server.on("/api/ir/send",   HTTP_POST, [this]() { handleApiIrSend();   });
    _server.on("/api/ir/rename", HTTP_POST, [this]() { handleApiIrRename(); });
    _server.on("/api/ir/delete", HTTP_POST, [this]() { handleApiIrDelete(); });
    _server.on("/api/ir/import", HTTP_POST, [this]() { handleApiIrImport(); });
    _server.on("/api/ir/upload-signals", HTTP_GET,  [this]() { handleApiIrUploadSignals(); });
    _server.on("/api/ir/send-upload",    HTTP_POST, [this]() { handleApiIrSendUpload();    });
    _server.on("/api/ir/export",         HTTP_GET,  [this]() { handleApiIrExport();        });

    _server.on("/api/wifi/ssids",  HTTP_GET,  [this]() { handleApiWifiSsidsGet(); });
    _server.on("/api/wifi/ssids",  HTTP_POST, [this]() { handleApiWifiSsidsPost(); });
    _server.on("/api/wifi/status", HTTP_GET,  [this]() { handleApiWifiStatus(); });

    _server.on("/api/profiles",        HTTP_GET,  [this]() { handleApiProfilesGet(); });
    _server.on("/api/profiles",        HTTP_POST, [this]() { handleApiProfilesPost(); });
    _server.on("/api/profiles/active", HTTP_POST, [this]() { handleApiProfilesSetActive(); });

    _server.on("/api/fs/list",      HTTP_GET,  [this]() { handleApiFsList(); });
    _server.on("/api/file/download",HTTP_GET,  [this]() { handleApiFileDownload(); });
    _server.on("/api/file/delete",  HTTP_POST, [this]() { handleApiFileDelete(); });
    _server.on("/api/file/upload", HTTP_POST,
        [this]() {
            if (_uploadHadError) {
                _server.send(400, "application/json", "{\"ok\":false}");
            } else {
                _server.send(200, "application/json", "{\"ok\":true}");
            }
        },
        [this]() { handleApiFileUpload(); }
    );

    _server.on("/api/karma/probes",      HTTP_GET,  [this]() { handleApiKarmaProbes();     });
    _server.on("/api/karma/start-sniff", HTTP_POST, [this]() { handleApiKarmaStartSniff();  });
    _server.on("/api/karma/stop-sniff",  HTTP_POST, [this]() { handleApiKarmaStopSniff();   });
    _server.on("/api/karma/start",       HTTP_POST, [this]() { handleApiKarmaStart();       });
    _server.on("/api/karma/stop",        HTTP_POST, [this]() { handleApiKarmaStop();        });
    _server.on("/api/karma/status",      HTTP_GET,  [this]() { handleApiKarmaStatus();      });
    _server.on("/api/karma/clear",       HTTP_POST, [this]() { handleApiKarmaClear();       });

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

    auto esc = [](String s) -> String {
        s.replace("\\", "\\\\"); s.replace("\"", "\\\""); return s;
    };

    String name    = esc(_storageManager->getBadgeName());
    String status  = esc(_storageManager->getBadgeStatus());
    String tagline = esc(_storageManager->getBadgeTagline());
    String qrData  = esc(_storageManager->getBadgeQrData());

    bool autostart = _storageManager->getPortalAutostart();
    int  sleepIdx  = _deviceSettings ? _deviceSettings->sleepTimeoutIndex    : 1;
    bool startScr  = _deviceSettings ? _deviceSettings->showStartScreen      : true;

    String json = "{";
    json += "\"ok\":true,";
    json += "\"badgeName\":\""    + name    + "\",";
    json += "\"badgeStatus\":\""  + status  + "\",";
    json += "\"badgeTagline\":\"" + tagline + "\",";
    json += "\"badgeQrData\":\""  + qrData  + "\",";
    json += "\"portalAutostart\":"  + String(autostart ? "true" : "false") + ",";
    json += "\"sleepTimeoutIndex\":" + String(sleepIdx) + ",";
    json += "\"showStartScreen\":"   + String(startScr ? "true" : "false");
    json += "}";

    _server.send(200, "application/json", json);
}

void PortalManager::handleApiSettingsPost() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String name    = _server.arg("badgeName");
    String status  = _server.arg("badgeStatus");
    String tagline = _server.arg("badgeTagline");
    String qrData  = _server.arg("badgeQrData");

    if (name.length() > 12) name = name.substring(0, 12);

    Serial.printf("[Portal] settings POST: name='%s' status='%s' tagline='%s' qr='%s'\n",
        name.c_str(), status.c_str(), tagline.c_str(), qrData.c_str());

    bool ok = _storageManager->setBadgeName(name)
           && _storageManager->setBadgeStatus(status);

    _storageManager->setBadgeTagline(tagline);
    _storageManager->setBadgeQrData(qrData);

    if (_server.hasArg("portalAutostart")) {
        String val = _server.arg("portalAutostart");
        _storageManager->setPortalAutostart(val == "1" || val == "true");
    }

    if (_deviceSettings && _server.hasArg("sleepTimeoutIndex")) {
        int idx = constrain(_server.arg("sleepTimeoutIndex").toInt(), 0, 3);
        _deviceSettings->sleepTimeoutIndex = idx;
        if (_powerManager) {
            static const unsigned long timeouts[] = {10000, 30000, 60000, 180000};
            _powerManager->setSleepTimeout(timeouts[idx]);
        }
        _storageManager->saveDeviceSettings(*_deviceSettings);
    }

    if (_deviceSettings && _server.hasArg("showStartScreen")) {
        String val = _server.arg("showStartScreen");
        _deviceSettings->showStartScreen = (val == "1" || val == "true");
        _storageManager->saveDeviceSettings(*_deviceSettings);
    }

    _server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
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

    int id = 0;
    if (!tryParsePositiveIntArg(_server, "id", id)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_id\"}");
        return;
    }

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

    int id = 0;
    if (!tryParsePositiveIntArg(_server, "id", id)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_id\"}");
        return;
    }

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

    int id = 0;
    if (!tryParsePositiveIntArg(_server, "id", id)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_id\"}");
        return;
    }

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
        _uploadHadError = false;

        String dir = _server.arg("path");
        if (dir.isEmpty()) {
            dir = "/ir/db";
        }

        if (!isSafeFsPath(dir)) {
            _uploadHadError = true;
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
        if (!uploadFile) {
            _uploadHadError = true;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        } else {
            _uploadHadError = true;
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
        } else {
            _uploadHadError = true;
        }

        Serial.print("[Portal] upload done: ");
        Serial.println(uploadPath);
    }
}


void PortalManager::handleApiWifiSsidsGet() {
    if (!_storageManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    auto ssids = _storageManager->loadSpamSSIDs();
    String json = "{\"ok\":true,\"ssids\":[";
    for (size_t i = 0; i < ssids.size(); i++) {
        String s = ssids[i];
        s.replace("\\", "\\\\");
        s.replace("\"", "\\\"");
        json += "\"" + s + "\"";
        if (i + 1 < ssids.size()) json += ",";
    }
    json += "]}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiWifiSsidsPost() {
    if (!_storageManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    String raw = _server.arg("ssids");
    std::vector<String> ssids;
    int start = 0;
    while (start < (int)raw.length()) {
        int nl = raw.indexOf('\n', start);
        String line = (nl < 0) ? raw.substring(start) : raw.substring(start, nl);
        line.trim();
        if (!line.isEmpty() && ssids.size() < 20) ssids.push_back(line);
        if (nl < 0) break;
        start = nl + 1;
    }
    bool ok = _storageManager->saveSpamSSIDs(ssids);
    _server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void PortalManager::handleApiWifiStatus() {
    int count = 0;
    if (_storageManager) {
        count = (int)_storageManager->loadSpamSSIDs().size();
    }
    String json = "{\"ok\":true,\"ssidCount\":" + String(count) + "}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiProfilesGet() {
    if (!_storageManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    auto esc = [](String s) -> String {
        s.replace("\\", "\\\\"); s.replace("\"", "\\\""); return s;
    };
    int active = _storageManager->getActiveProfile();
    String json = "{\"ok\":true,\"active\":" + String(active) + ",\"profiles\":[";
    for (int i = 0; i < StorageManager::PROFILE_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"index\":" + String(i)
             + ",\"name\":\""    + esc(_storageManager->getProfileName(i))    + "\""
             + ",\"tagline\":\"" + esc(_storageManager->getProfileTagline(i)) + "\""
             + ",\"qrData\":\""  + esc(_storageManager->getProfileQrData(i))  + "\""
             + "}";
    }
    json += "]}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiProfilesPost() {
    if (!_storageManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    int idx = constrain(_server.arg("index").toInt(), 0, StorageManager::PROFILE_COUNT - 1);
    String name    = _server.arg("name");
    String tagline = _server.arg("tagline");
    String qrData  = _server.arg("qrData");
    if (name.length() > 12) name = name.substring(0, 12);
    _storageManager->setProfileName(idx, name);
    _storageManager->setProfileTagline(idx, tagline);
    _storageManager->setProfileQrData(idx, qrData);
    _server.send(200, "application/json", "{\"ok\":true}");
}

void PortalManager::handleApiProfilesSetActive() {
    if (!_storageManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    int idx = constrain(_server.arg("index").toInt(), 0, StorageManager::PROFILE_COUNT - 1);
    _storageManager->setActiveProfile(idx);
    _server.send(200, "application/json", "{\"ok\":true}");
}

void PortalManager::handleApiIrImport() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String path = _server.arg("path");
    if (!isSafeFsPath(path)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_path\"}");
        return;
    }

    int count = _storageManager->importFlipperFile(path);

    String json = "{";
    json += "\"ok\":true,";
    json += "\"count\":" + String(count);
    json += "}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiIrUploadSignals() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String filename = _server.arg("filename");
    if (filename.isEmpty()) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_filename\"}");
        return;
    }

    auto signals = _storageManager->loadIrUploadFile(filename);

    String json = "{\"ok\":true,\"signals\":[";
    for (size_t i = 0; i < signals.size(); i++) {
        String name = signals[i].name;
        name.replace("\\", "\\\\");
        name.replace("\"", "\\\"");
        json += "{\"index\":";
        json += String(i);
        json += ",\"name\":\"";
        json += name;
        json += "\"}";
        if (i + 1 < signals.size()) json += ',';
    }
    json += "]}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiIrSendUpload() {
    if (_storageManager == nullptr || _irManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    String filename = _server.arg("filename");
    int    index    = _server.arg("index").toInt();

    if (filename.isEmpty()) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_filename\"}");
        return;
    }

    auto signals = _storageManager->loadIrUploadFile(filename);
    if (index < 0 || index >= (int)signals.size()) {
        _server.send(404, "application/json", "{\"ok\":false,\"error\":\"bad_index\"}");
        return;
    }

    _irManager->setLastCapture(signals[index].capture);
    bool ok = _irManager->replayLast();
    _server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void PortalManager::handleApiIrExport() {
    if (_storageManager == nullptr) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    int id = 0;
    if (!tryParsePositiveIntArg(_server, "id", id)) {
        _server.send(400, "text/plain", "Missing id");
        return;
    }

    String content = _storageManager->exportIrCaptureAsFlipperFormat(id);
    if (content.isEmpty()) {
        _server.send(404, "text/plain", "Not found or not exportable");
        return;
    }

    String disposition = "attachment; filename=\"ir_" + String(id) + ".ir\"";
    _server.sendHeader("Content-Disposition", disposition);
    _server.send(200, "text/plain", content);
}

void PortalManager::handleApiFileDelete() {
    String path = _server.arg("path");
    if (!isSafeFsPath(path)) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_path\"}");
        return;
    }
    bool ok = LittleFS.remove(path);
    _server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

// ─── Karma / Probe handlers ───────────────────────────────────────────────────

void PortalManager::handleApiKarmaProbes() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }

    auto probes = _karmaManager->getProbes();

    String json = "{\"ok\":true,\"probes\":[";
    for (size_t i = 0; i < probes.size(); i++) {
        String ssid = probes[i].ssid;
        ssid.replace("\\", "\\\\");
        ssid.replace("\"", "\\\"");

        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            probes[i].mac[0], probes[i].mac[1], probes[i].mac[2],
            probes[i].mac[3], probes[i].mac[4], probes[i].mac[5]);

        json += "{";
        json += "\"ssid\":\"" + ssid + "\",";
        json += "\"count\":" + String(probes[i].count) + ",";
        json += "\"rssi\":"  + String(probes[i].rssi)  + ",";
        json += "\"mac\":\""  + String(macStr) + "\"";
        json += "}";
        if (i + 1 < probes.size()) json += ",";
    }
    json += "]}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiKarmaStartSniff() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    _karmaManager->startSniff();
    _server.send(200, "application/json", "{\"ok\":true}");
}

void PortalManager::handleApiKarmaStopSniff() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    _karmaManager->stopSniff();
    _server.send(200, "application/json", "{\"ok\":true}");
}

void PortalManager::handleApiKarmaStart() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    String ssid = _server.arg("ssid");
    ssid.trim();
    if (ssid.isEmpty()) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_ssid\"}");
        return;
    }
    bool ok = _karmaManager->startKarma(ssid);
    _server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void PortalManager::handleApiKarmaStop() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    _karmaManager->stopKarma();
    _server.send(200, "application/json", "{\"ok\":true}");
}

void PortalManager::handleApiKarmaStatus() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    bool    sniffing = _karmaManager->isSniffing();
    bool    karma    = _karmaManager->isKarmaRunning();
    String  ssid     = _karmaManager->getKarmaSsid();
    int     probes   = _karmaManager->getProbeCount();

    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");

    String json = "{\"ok\":true,";
    json += "\"sniffing\":"  + String(sniffing ? "true" : "false") + ",";
    json += "\"karma\":"     + String(karma    ? "true" : "false") + ",";
    json += "\"karmaSsid\":\"" + ssid + "\",";
    json += "\"probeCount\":" + String(probes);
    json += "}";
    _server.send(200, "application/json", json);
}

void PortalManager::handleApiKarmaClear() {
    if (!_karmaManager) {
        _server.send(500, "application/json", "{\"ok\":false}");
        return;
    }
    _karmaManager->clearProbes();
    _server.send(200, "application/json", "{\"ok\":true}");
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
