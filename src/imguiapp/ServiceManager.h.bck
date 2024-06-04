#pragma once
#include "pch.h"

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME             "injdrv"

struct ServiceManager {
    std::string DriverName = DRIVER_NAME;

    ServiceManager(std::string path);

    ~ServiceManager();

    bool Install();

    bool Uninstall();

private:
    
    std::string DriverLocation;

    bool SetupDriverName();


    bool ManageDriver(
            _In_ USHORT   Function
        );


    bool SetupDriver(
            _In_ SC_HANDLE  SchSCManager
        );


    bool StartDriver(
            _In_ SC_HANDLE    SchSCManager
        );



    bool StopDriver(
            _In_ SC_HANDLE    SchSCManager
        );


    bool RemoveDriver(
            _In_ SC_HANDLE    SchSCManager
        );
};
