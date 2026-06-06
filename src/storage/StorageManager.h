#pragma once

#include <Arduino.h>
#include <vector>
#include "storage/FileSystemDriver.h"
#include "ir/IrDriver.h"

struct IrSavedItem {
    int id = 0;
    String fileId;
    String name;
};

struct IrUploadItem {
    String filename;   // e.g. "samsung_tv.ir"
    String displayName; // same without extension
};

struct IrUploadSignal {
    String name;
    IrCapture capture;
};


class StorageManager {
public:
    explicit StorageManager(FileSystemDriver* fileSystemDriver);

    bool begin();
    bool available() const;

    String getBadgeName() const;
    String getBadgeStatus() const;
    String getBadgeTagline() const;
    String getBadgeQrData() const;
    int    getBadgeMode() const;

    bool setBadgeName(const String& name);
    bool setBadgeStatus(const String& status);
    bool setBadgeTagline(const String& tagline);
    bool setBadgeQrData(const String& url);
    bool setBadgeMode(int mode);

    std::vector<String> listPayloadFiles() const;
    String readPayloadFile(const String& filename) const;

    std::vector<FileEntry> listFsEntries(const String& path) const;
    bool ensureDirectory(const String& path);

    std::vector<IrUploadItem>   listIrUploads() const;
    std::vector<IrUploadSignal> loadIrUploadFile(const String& filename) const;

    // IR storage
    bool saveIrCapture(const IrCapture& capture);
    std::vector<IrSavedItem> listIrCaptures() const;
    bool loadIrCaptureById(int id, IrCapture& outCapture) const;
    bool renameIrCaptureById(int id, const String& newName);
    bool deleteIrCaptureById(int id);

    std::vector<String> loadSpamSSIDs() const;
    bool saveSpamSSIDs(const std::vector<String>& ssids);

    // Import all signals from a Flipper Zero .ir file; returns count imported
    int importFlipperFile(const String& path);

private:
    FileSystemDriver* _fileSystemDriver = nullptr;
    bool _available = false;

    static constexpr const char* NAME_PATH       = "/config/name.txt";
    static constexpr const char* STATUS_PATH     = "/config/status.txt";
    static constexpr const char* TAGLINE_PATH    = "/config/tagline.txt";
    static constexpr const char* QR_DATA_PATH    = "/config/qr_data.txt";
    static constexpr const char* BADGE_MODE_PATH = "/config/badge_mode.txt";
    static constexpr const char* SPAM_SSIDS_PATH = "/config/spam_ssids.txt";
    static constexpr const char* PAYLOADS_DIR = "/payloads";
    static constexpr const char* IR_UPLOADS_DIR = "/ir/uploads";
    static constexpr const char* IR_DIR = "/ir/saved";


    String makeIrCodePath(const String& fileId) const;
    String makeIrNamePath(const String& fileId) const;

    int nextIrCaptureId() const;
    String makeFileId(int id) const;

    static String escapeName(const String& value);
    static String unescapeName(const String& value);

    static String protocolToStorageString(decode_type_t protocol);
    static decode_type_t protocolFromStorageString(const String& value);

    static String serializeIrCapture(const IrCapture& capture);
    static bool deserializeIrCapture(const String& text, IrCapture& outCapture);

    static String extractValue(const String& text, const String& key);
    static std::vector<uint16_t> parseRawList(const String& rawText);
};