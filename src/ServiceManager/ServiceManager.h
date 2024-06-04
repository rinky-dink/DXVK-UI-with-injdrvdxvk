#ifndef SERVICE_MANAGER_LIB_H
#define SERVICE_MANAGER_LIB_H

#include <string>

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

enum InstallDriverStatus {
    SuccessInstalled,
    SuccessUninstalled,
    NotLoaded,
    InsufficientPermissions,
    UnknownError,
    ServiceAlreadyRunning,
    ServiceNotFound,
    StartServiceFailed,
    ServiceMarkedForDelete,
    ServiceExists,
    ServiceControlStopFailed,
    ServiceDeleteFailed,
    InvalidDriverOrService
};

struct ServiceManager {
    std::string DriverName;

    std::string DriverLocation;
    ServiceManager(const std::string& DriverName);

    ~ServiceManager();

    InstallDriverStatus Install();

    InstallDriverStatus Uninstall();

private:
    InstallDriverStatus SetupDriverName();

    InstallDriverStatus ManageDriver(
        _In_ USHORT   Function
    );

    InstallDriverStatus SetupDriver(
        _In_ SC_HANDLE  SchSCManager
    );

    InstallDriverStatus StartDriver(
        _In_ SC_HANDLE    SchSCManager
    );


    InstallDriverStatus StopDriver(
        _In_ SC_HANDLE    SchSCManager
    );

    InstallDriverStatus RemoveDriver(
        _In_ SC_HANDLE    SchSCManager
    );
};


#endif // SERVICE_MANAGER_LIB_H
