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