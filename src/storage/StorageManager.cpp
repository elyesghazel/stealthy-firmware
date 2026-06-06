#include "StorageManager.h"

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

    // Split on "---\n" or "---" at line boundaries
    int start = 0;
    while (start < (int)text.length()) {
        int sep = text.indexOf("\n---", start);
        String block;
        if (sep < 0) {
            block = text.substring(start);
            start = text.length();
        } else {
            block = text.substring(start, sep);
            start = sep + 4; // skip "\n---"
            // skip optional \n after separator
            if (start < (int)text.length() && text[start] == '\n') start++;
        }

        block.trim();
        if (block.isEmpty()) continue;

        IrUploadSignal sig;
        sig.name = extractValue(block, "name");
        if (sig.name.isEmpty()) sig.name = "Signal";

        // Remove the name line before deserializing capture
        String captureBlock = block;
        int nameLine = captureBlock.indexOf("name=");
        if (nameLine >= 0) {
            int nameEnd = captureBlock.indexOf('\n', nameLine);
            if (nameEnd >= 0)
                captureBlock = captureBlock.substring(0, nameLine)
                             + captureBlock.substring(nameEnd + 1);
            else
                captureBlock = captureBlock.substring(0, nameLine);
        }

        if (deserializeIrCapture(captureBlock, sig.capture)) {
            signals.push_back(sig);
        }
    }
    return signals;
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
