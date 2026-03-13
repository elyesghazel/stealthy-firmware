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
    return _available;
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