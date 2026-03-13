#pragma once

#include <Arduino.h>
#include <vector>
#include "../drivers/FileSystemDriver.h"

class StorageManager {
public:
    explicit StorageManager(FileSystemDriver* fileSystemDriver);

    bool begin();
    bool available() const;

    String getBadgeName() const;
    String getBadgeStatus() const;

    bool setBadgeName(const String& name);
    bool setBadgeStatus(const String& status);

    std::vector<String> listPayloadFiles() const;
    String readPayloadFile(const String& filename) const;

private:
    FileSystemDriver* _fileSystemDriver = nullptr;
    bool _available = false;

    static constexpr const char* NAME_PATH = "/config/name.txt";
    static constexpr const char* STATUS_PATH = "/config/status.txt";
    static constexpr const char* PAYLOADS_DIR = "/payloads";
};