#pragma once

#include <WebServer.h>
#include <DNSServer.h>
#include "DeviceSettings.h"

class StorageManager;
class IrManager;
class PowerManager;
class WifiKarma;

class PortalManager {
public:
    PortalManager(StorageManager* storageManager, IrManager* irManager,
                  PowerManager* powerManager, WifiKarma* karmaManager,
                  DeviceSettings* deviceSettings);
    bool begin();
    void stop();
    void update();

    bool        isRunning()  const;
    const char* ssid()       const;
    const char* ipAddress()  const;

private:
    void setupRoutes();

    void handleRoot();
    void handleCss();
    void handleJs();

    void handleApiBattery();
    void handleApiStatus();
    void handleApiSettingsGet();
    void handleApiSettingsPost();

    void handleApiIrList();
    void handleApiIrSend();
    void handleApiIrRename();
    void handleApiIrDelete();
    void handleApiIrImport();
    void handleApiIrUploadSignals();
    void handleApiIrSendUpload();
    void handleApiIrExport();

    void handleApiWifiSsidsGet();
    void handleApiWifiSsidsPost();
    void handleApiWifiStatus();

    void handleApiProfilesGet();
    void handleApiProfilesPost();
    void handleApiProfilesSetActive();

    void handleApiFsList();
    void handleApiFileDownload();
    void handleApiFileUpload();
    void handleApiFileDelete();

    void handleApiKarmaProbes();
    void handleApiKarmaStartSniff();
    void handleApiKarmaStopSniff();
    void handleApiKarmaStart();
    void handleApiKarmaStop();
    void handleApiKarmaStatus();
    void handleApiKarmaClear();

    String contentTypeForPath(const String& path) const;
    bool   serveFile(const char* path);

    StorageManager* _storageManager  = nullptr;
    IrManager*      _irManager       = nullptr;
    PowerManager*   _powerManager    = nullptr;
    WifiKarma*      _karmaManager    = nullptr;
    DeviceSettings* _deviceSettings  = nullptr;

    DNSServer  _dnsServer;
    WebServer  _server;

    bool _running        = false;
    bool _uploadHadError = false;

    static constexpr const char* AP_SSID = "STEALTHY-SETUP";
    static constexpr const char* AP_IP   = "192.168.4.1";
};
