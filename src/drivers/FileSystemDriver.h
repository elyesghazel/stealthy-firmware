#pragma once

#include <Arduino.h>
#include <vector>

class FileSystemDriver {
public:
    bool begin();
    bool isMounted() const;

    bool exists(const char* path) const;
    String readTextFile(const char* path) const;
    bool writeTextFile(const char* path, const String& content);
    std::vector<String> listFiles(const char* path) const;
    bool removeFile(const char* path);

private:
    bool _mounted = false;
};