//
//  ServerMgr.h
//  KLights
//
//  Created by Casey Fleser on 04/22/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#ifndef ServerMgr_h
#define ServerMgr_h

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

class ServerMgr {
public:
    ServerMgr();
    
    void setup();
    void loop();

protected:
    void handleFileList();
    void handleSysInfo();
    void handleBasicUpload();
    void handleRedirect();
    void handleNotFound();

    ESP8266WebServer        server;
    ESP8266HTTPUpdateServer httpUpdater;
    time_t                  bootTime;
};

#endif
