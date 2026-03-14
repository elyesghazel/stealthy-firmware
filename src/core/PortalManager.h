#pragma once

#include <WebServer.h>
#include <DNSServer.h>

class StorageManager;
class IrManager;
class PowerManager;


class PortalManager {
public:
    PortalManager(StorageManager* storageManager, IrManager* irManager, PowerManager* powerManager);
    bool begin();
    void stop();
    void update();

    bool isRunning() const;
    const char* ssid() const;
    const char* ipAddress() const;

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
    void handleApiFileDownload();


    String contentTypeForPath(const String& path) const;
    bool serveFile(const char* path);

    StorageManager* _storageManager = nullptr;
    IrManager* _irManager = nullptr;
    PowerManager* _powerManager = nullptr;

    DNSServer _dnsServer;
    WebServer _server;

    bool _running = false;

    static constexpr const char* AP_SSID = "STEALTHY-SETUP";
    static constexpr const char* AP_IP = "192.168.4.1";
};
