#include "StorageManager.h"
#include "ir/FlipperIrParser.h"
#include "ir/FlipperIrSerializer.h"

StorageManager::StorageManager(FileSystemDriver* fileSystemDriver)
    : _fileSystemDriver(fileSystemDriver) {
}

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
    _fileSystemDriver->ensureDir("/ir/uploads");
    _fileSystemDriver->ensureDir("/ir/db");

    // Create default config files so LittleFS never errors on a missing read
    auto ensureFile = [this](const char* path, const char* def) {
        if (!_fileSystemDriver->exists(path))
            _fileSystemDriver->writeTextFile(path, def);
    };
    ensureFile(NAME_PATH,    "Stealthy\n");
    ensureFile(STATUS_PATH,  "Online\n");
    ensureFile(TAGLINE_PATH, "\n");
    ensureFile(QR_DATA_PATH, "\n");
    ensureFile(BADGE_MODE_PATH,        "0\n");
    ensureFile(SPAM_SSIDS_PATH,        "");
    ensureFile(PORTAL_AUTOSTART_PATH,  "0");
    ensureFile(DEVICE_SETTINGS_PATH,
        "sleepTimeoutIndex=1\n"
        "refreshIndex=1\n"
        "badgeStatusIndex=0\n"
        "showStartScreen=1\n"
    );
    ensureFile(ACTIVE_PROFILE_PATH, "0\n");
    // Default profile names
    ensureFile("/config/profile0_name.txt",    "Stealthy\n");
    ensureFile("/config/profile0_tagline.txt", "\n");
    ensureFile("/config/profile0_qr.txt",      "\n");
    ensureFile("/config/profile1_name.txt",    "Profile 2\n");
    ensureFile("/config/profile1_tagline.txt", "\n");
    ensureFile("/config/profile1_qr.txt",      "\n");
    ensureFile("/config/profile2_name.txt",    "Profile 3\n");
    ensureFile("/config/profile2_tagline.txt", "\n");
    ensureFile("/config/profile2_qr.txt",      "\n");

    return true;
}

bool StorageManager::available() const {
    return _available;
}

String StorageManager::getBadgeName() const {
    if (!_available) return "Stealthy";

    String value = _fileSystemDriver->readTextFile(NAME_PATH);
    value.trim();

    if (value.isEmpty()) {
        return "Stealthy";
    }

    return value;
}

String StorageManager::getBadgeStatus() const {
    if (!_available) return "Ready";

    String value = _fileSystemDriver->readTextFile(STATUS_PATH);
    value.trim();

    if (value.isEmpty()) {
        return "Ready";
    }

    return value;
}

bool StorageManager::setBadgeName(const String& name) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(NAME_PATH, name + "\n");
}

bool StorageManager::setBadgeStatus(const String& status) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(STATUS_PATH, status + "\n");
}

String StorageManager::getBadgeTagline() const {
    if (!_available) return "";
    String v = _fileSystemDriver->readTextFile(TAGLINE_PATH);
    v.trim();
    return v;
}

String StorageManager::getBadgeQrData() const {
    if (!_available) return "";
    String v = _fileSystemDriver->readTextFile(QR_DATA_PATH);
    v.trim();
    Serial.printf("[Storage] getBadgeQrData() -> '%s'\n", v.c_str());
    return v;
}

int StorageManager::getBadgeMode() const {
    if (!_available) return 0;
    String v = _fileSystemDriver->readTextFile(BADGE_MODE_PATH);
    v.trim();
    return v.toInt();
}

bool StorageManager::setBadgeTagline(const String& tagline) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(TAGLINE_PATH, tagline + "\n");
}

bool StorageManager::setBadgeQrData(const String& url) {
    if (!_available) return false;
    bool ok = _fileSystemDriver->writeTextFile(QR_DATA_PATH, url + "\n");
    Serial.printf("[Storage] setBadgeQrData('%s') -> %s\n", url.c_str(), ok ? "ok" : "FAILED");
    return ok;
}

bool StorageManager::setBadgeMode(int mode) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(BADGE_MODE_PATH, String(mode) + "\n");
}

std::vector<String> StorageManager::listPayloadFiles() const {
    if (!_available) return {};
    return _fileSystemDriver->listFiles(PAYLOADS_DIR);
}

String StorageManager::readPayloadFile(const String& filename) const {
    if (!_available) return "";

    String path = String(PAYLOADS_DIR) + "/" + filename;
    return _fileSystemDriver->readTextFile(path.c_str());
}

std::vector<FileEntry> StorageManager::listFsEntries(const String& path) const {
    if (!_available) return {};
    return _fileSystemDriver->listEntries(path.c_str());
}

bool StorageManager::ensureDirectory(const String& path) {
    if (!_available) return false;
    return _fileSystemDriver->ensureDir(path.c_str());
}


// ---------- IR storage ----------

String StorageManager::makeFileId(int id) const {
    char buf[8];
    snprintf(buf, sizeof(buf), "%04d", id);
    return String(buf);
}

String StorageManager::makeIrCodePath(const String& fileId) const {
    return String(IR_DIR) + "/" + fileId + ".ir";
}

String StorageManager::makeIrNamePath(const String& fileId) const {
    return String(IR_DIR) + "/" + fileId + ".txt";
}

int StorageManager::nextIrCaptureId() const {
    if (!_available) return 1;

    std::vector<String> files = _fileSystemDriver->listFiles(IR_DIR);
    int maxId = 0;

    for (const String& file : files) {
        int slash = file.lastIndexOf('/');
        String base = (slash >= 0) ? file.substring(slash + 1) : file;

        if (!base.endsWith(".ir")) {
            continue;
        }

        String idPart = base.substring(0, base.length() - 3);
        int id = idPart.toInt();
        if (id > maxId) {
            maxId = id;
        }
    }

    return maxId + 1;
}

String StorageManager::escapeName(const String& value) {
    String result = value;
    result.replace("\n", " ");
    result.replace("\r", " ");
    result.trim();
    return result;
}

String StorageManager::unescapeName(const String& value) {
    String result = value;
    result.trim();
    return result;
}

String StorageManager::protocolToStorageString(decode_type_t protocol) {
    return IrDriver::protocolToString(protocol);
}

decode_type_t StorageManager::protocolFromStorageString(const String& value) {
    String v = value;
    v.trim();

    if (v == "NEC") return NEC;
    if (v == "SONY") return SONY;
    if (v == "RC5") return RC5;
    if (v == "RC6") return RC6;
    if (v == "JVC") return JVC;
    if (v == "SAMSUNG") return SAMSUNG;
    if (v == "LG") return LG;
    return UNKNOWN;
}

String StorageManager::serializeIrCapture(const IrCapture& capture) {
    String text;

    text += "protocol=" + protocolToStorageString(capture.protocol) + "\n";
    text += "value=" + String(static_cast<unsigned long long>(capture.value)) + "\n";
    text += "bits=" + String(capture.bits) + "\n";
    text += "rawlen=" + String(capture.rawData.size()) + "\n";
    text += "raw=";

    for (size_t i = 0; i < capture.rawData.size(); i++) {
        text += String(capture.rawData[i]);
        if (i + 1 < capture.rawData.size()) {
            text += ",";
        }
    }
    text += "\n";

    return text;
}

String StorageManager::extractValue(const String& text, const String& key) {
    String prefix = key + "=";
    int start = text.indexOf(prefix);
    if (start < 0) {
        return "";
    }

    start += prefix.length();
    int end = text.indexOf('\n', start);
    if (end < 0) {
        end = text.length();
    }

    return text.substring(start, end);
}

std::vector<uint16_t> StorageManager::parseRawList(const String& rawText) {
    std::vector<uint16_t> values;
    int start = 0;

    while (start < rawText.length()) {
        int comma = rawText.indexOf(',', start);
        String part;

        if (comma < 0) {
            part = rawText.substring(start);
            start = rawText.length();
        } else {
            part = rawText.substring(start, comma);
            start = comma + 1;
        }

        part.trim();
        if (!part.isEmpty()) {
            values.push_back(static_cast<uint16_t>(part.toInt()));
        }
    }

    return values;
}

bool StorageManager::deserializeIrCapture(const String& text, IrCapture& outCapture) {
    String protocolStr = extractValue(text, "protocol");
    String valueStr = extractValue(text, "value");
    String bitsStr = extractValue(text, "bits");
    String rawStr = extractValue(text, "raw");

    if (valueStr.isEmpty() || bitsStr.isEmpty()) {
        return false;
    }

    outCapture.valid = true;
    outCapture.protocol = protocolFromStorageString(protocolStr);
    outCapture.value = static_cast<uint64_t>(strtoull(valueStr.c_str(), nullptr, 10));
    outCapture.bits = static_cast<uint16_t>(bitsStr.toInt());
    outCapture.rawData = parseRawList(rawStr);

    return true;
}

bool StorageManager::saveIrCapture(const IrCapture& capture) {
    if (!_available || !capture.valid) {
        return false;
    }

    int id = nextIrCaptureId();
    String fileId = makeFileId(id);

    String codePath = makeIrCodePath(fileId);
    String namePath = makeIrNamePath(fileId);

    String defaultName = "IR " + fileId;
    String payload = serializeIrCapture(capture);

    bool okCode = _fileSystemDriver->writeTextFile(codePath.c_str(), payload);
    bool okName = _fileSystemDriver->writeTextFile(namePath.c_str(), defaultName + "\n");

    return okCode && okName;
}

std::vector<IrSavedItem> StorageManager::listIrCaptures() const {
    std::vector<IrSavedItem> items;
    if (!_available) return items;

    std::vector<String> files = _fileSystemDriver->listFiles(IR_DIR);

    for (const String& file : files) {
        int slash = file.lastIndexOf('/');
        String base = (slash >= 0) ? file.substring(slash + 1) : file;

        if (!base.endsWith(".ir")) {
            continue;
        }

        String fileId = base.substring(0, base.length() - 3);
        int id = fileId.toInt();

        IrSavedItem item;
        item.id = id;
        item.fileId = fileId;

        String nameText = _fileSystemDriver->readTextFile(makeIrNamePath(fileId).c_str());
        nameText.trim();

        if (nameText.isEmpty()) {
            item.name = "IR " + fileId;
        } else {
            item.name = unescapeName(nameText);
        }

        items.push_back(item);
    }

    for (size_t i = 0; i < items.size(); i++) {
        for (size_t j = i + 1; j < items.size(); j++) {
            if (items[j].id < items[i].id) {
                IrSavedItem tmp = items[i];
                items[i] = items[j];
                items[j] = tmp;
            }
        }
    }

    return items;
}

bool StorageManager::loadIrCaptureById(int id, IrCapture& outCapture) const {
    if (!_available) {
        return false;
    }

    String fileId = makeFileId(id);
    String text = _fileSystemDriver->readTextFile(makeIrCodePath(fileId).c_str());
    if (text.isEmpty()) {
        return false;
    }

    return deserializeIrCapture(text, outCapture);
}

bool StorageManager::renameIrCaptureById(int id, const String& newName) {
    if (!_available) {
        return false;
    }

    String fileId = makeFileId(id);
    String safeName = escapeName(newName);
    if (safeName.isEmpty()) {
        safeName = "IR " + fileId;
    }

    return _fileSystemDriver->writeTextFile(
        makeIrNamePath(fileId).c_str(),
        safeName + "\n"
    );
}

std::vector<IrUploadItem> StorageManager::listIrUploads() const {
    std::vector<IrUploadItem> items;
    if (!_available) return items;

    std::vector<String> files = _fileSystemDriver->listFiles(IR_UPLOADS_DIR);
    for (const String& file : files) {
        int slash = file.lastIndexOf('/');
        String base = (slash >= 0) ? file.substring(slash + 1) : file;
        if (!base.endsWith(".ir")) continue;

        IrUploadItem item;
        item.filename = base;
        item.displayName = base.substring(0, base.length() - 3);
        items.push_back(item);
    }
    return items;
}

std::vector<IrUploadSignal> StorageManager::loadIrUploadFile(const String& filename) const {
    std::vector<IrUploadSignal> signals;
    if (!_available) return signals;

    String path = String(IR_UPLOADS_DIR) + "/" + filename;
    String text = _fileSystemDriver->readTextFile(path.c_str());
    if (text.isEmpty()) return signals;

    // Parse as Flipper Zero .ir format directly — no Import step needed
    auto parsed = FlipperIrParser::parse(text);
    for (auto& sig : parsed) {
        if (!sig.capture.valid) continue;
        IrUploadSignal out;
        out.name    = sig.name;
        out.capture = sig.capture;
        signals.push_back(out);
    }
    return signals;
}


std::vector<String> StorageManager::loadSpamSSIDs() const {
    std::vector<String> ssids;
    if (!_available) return ssids;

    String raw = _fileSystemDriver->readTextFile(SPAM_SSIDS_PATH);
    if (raw.isEmpty()) return ssids;

    int start = 0;
    while (start < (int)raw.length()) {
        int nl = raw.indexOf('\n', start);
        String line = (nl < 0)
            ? raw.substring(start)
            : raw.substring(start, nl);
        line.trim();
        if (!line.isEmpty() && ssids.size() < 20) {
            ssids.push_back(line);
        }
        if (nl < 0) break;
        start = nl + 1;
    }
    return ssids;
}

bool StorageManager::saveSpamSSIDs(const std::vector<String>& ssids) {
    if (!_available) return false;
    String content;
    int count = (int)ssids.size();
    if (count > 20) count = 20;
    for (int i = 0; i < count; i++) {
        String line = ssids[i];
        line.trim();
        if (!line.isEmpty()) {
            content += line + "\n";
        }
    }
    return _fileSystemDriver->writeTextFile(SPAM_SSIDS_PATH, content);
}

int StorageManager::importFlipperFile(const String& path) {
    if (!_available) return 0;

    String content = _fileSystemDriver->readTextFile(path.c_str());
    if (content.isEmpty()) return 0;

    auto signals = FlipperIrParser::parse(content);
    int imported = 0;

    for (auto& sig : signals) {
        if (!sig.capture.valid) continue;

        int    id     = nextIrCaptureId();
        String fileId = makeFileId(id);

        String payload  = serializeIrCapture(sig.capture);
        String safeName = escapeName(sig.name);
        if (safeName.isEmpty()) safeName = "Signal " + fileId;

        bool ok = _fileSystemDriver->writeTextFile(makeIrCodePath(fileId).c_str(), payload) &&
                  _fileSystemDriver->writeTextFile(makeIrNamePath(fileId).c_str(), safeName + "\n");
        if (ok) imported++;
    }

    Serial.printf("[Storage] Flipper import from %s: %d signal(s)\n", path.c_str(), imported);
    return imported;
}

bool StorageManager::loadDeviceSettings(DeviceSettings& out) const {
    if (!_available) return false;
    String raw = _fileSystemDriver->readTextFile(DEVICE_SETTINGS_PATH);
    if (raw.isEmpty()) return false;

    int pos = 0;
    while (pos < (int)raw.length()) {
        int nl = raw.indexOf('\n', pos);
        String line = (nl < 0) ? raw.substring(pos) : raw.substring(pos, nl);
        pos = (nl < 0) ? raw.length() : nl + 1;
        line.trim();
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        String key = line.substring(0, eq);
        String val = line.substring(eq + 1);
        key.trim(); val.trim();
        if (key == "sleepTimeoutIndex")    out.sleepTimeoutIndex    = constrain(val.toInt(), 0, 3);
        else if (key == "refreshIndex")    out.refreshIntervalIndex = constrain(val.toInt(), 0, 2);
        else if (key == "badgeStatusIndex")out.badgeStatusIndex     = constrain(val.toInt(), 0, 3);
        else if (key == "showStartScreen") out.showStartScreen      = (val == "1");
    }
    return true;
}

bool StorageManager::saveDeviceSettings(const DeviceSettings& s) {
    if (!_available) return false;
    String content;
    content += "sleepTimeoutIndex="    + String(s.sleepTimeoutIndex)    + "\n";
    content += "refreshIndex="         + String(s.refreshIntervalIndex) + "\n";
    content += "badgeStatusIndex="     + String(s.badgeStatusIndex)     + "\n";
    content += "showStartScreen="      + String(s.showStartScreen ? 1 : 0) + "\n";
    return _fileSystemDriver->writeTextFile(DEVICE_SETTINGS_PATH, content);
}

int StorageManager::getActiveProfile() const {
    if (!_available) return 0;
    String v = _fileSystemDriver->readTextFile(ACTIVE_PROFILE_PATH);
    v.trim();
    int idx = v.toInt();
    return constrain(idx, 0, PROFILE_COUNT - 1);
}

bool StorageManager::setActiveProfile(int index) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(ACTIVE_PROFILE_PATH,
        String(constrain(index, 0, PROFILE_COUNT - 1)) + "\n");
}

static String profilePath(int index, const char* field) {
    return String("/config/profile") + String(index) + "_" + field + ".txt";
}

String StorageManager::getProfileName(int index) const {
    if (!_available) return "Profile " + String(index + 1);
    String v = _fileSystemDriver->readTextFile(profilePath(index, "name").c_str());
    v.trim();
    return v.isEmpty() ? "Profile " + String(index + 1) : v;
}

String StorageManager::getProfileTagline(int index) const {
    if (!_available) return "";
    String v = _fileSystemDriver->readTextFile(profilePath(index, "tagline").c_str());
    v.trim();
    return v;
}

String StorageManager::getProfileQrData(int index) const {
    if (!_available) return "";
    String v = _fileSystemDriver->readTextFile(profilePath(index, "qr").c_str());
    v.trim();
    return v;
}

bool StorageManager::setProfileName(int index, const String& name) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(profilePath(index, "name").c_str(), name + "\n");
}

bool StorageManager::setProfileTagline(int index, const String& tagline) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(profilePath(index, "tagline").c_str(), tagline + "\n");
}

bool StorageManager::setProfileQrData(int index, const String& qrData) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(profilePath(index, "qr").c_str(), qrData + "\n");
}

bool StorageManager::getPortalAutostart() const {
    if (!_available) return false;
    String v = _fileSystemDriver->readTextFile(PORTAL_AUTOSTART_PATH);
    v.trim();
    return v == "1";
}

bool StorageManager::setPortalAutostart(bool enabled) {
    if (!_available) return false;
    return _fileSystemDriver->writeTextFile(PORTAL_AUTOSTART_PATH, enabled ? "1" : "0");
}

String StorageManager::exportIrCaptureAsFlipperFormat(int id) const {
    if (!_available) return "";
    String fileId = makeFileId(id);
    IrCapture capture;
    if (!loadIrCaptureById(id, capture)) return "";
    String name = _fileSystemDriver->readTextFile(makeIrNamePath(fileId).c_str());
    name.trim();
    if (name.isEmpty()) name = "Signal";

    std::vector<String>    names    = { name };
    std::vector<IrCapture> captures = { capture };
    return FlipperIrSerializer::serializeFile(names, captures);
}

bool StorageManager::deleteIrCaptureById(int id) {
    if (!_available) {
        return false;
    }

    String fileId = makeFileId(id);

    bool ok1 = _fileSystemDriver->removeFile(makeIrCodePath(fileId).c_str());
    bool ok2 = _fileSystemDriver->removeFile(makeIrNamePath(fileId).c_str());

    return ok1 || ok2;
}
