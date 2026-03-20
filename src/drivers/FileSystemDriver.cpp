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