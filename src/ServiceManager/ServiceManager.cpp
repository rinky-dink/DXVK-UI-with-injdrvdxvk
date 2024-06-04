#include "pch.h"
#include "ServiceManager.h"

ServiceManager::ServiceManager(const std::string& DriverName) :DriverName(DriverName){
    DriverLocation = std::filesystem::absolute("./" + DriverName + ".sys").string();
}

ServiceManager::~ServiceManager() {
    Uninstall();
}

InstallDriverStatus ServiceManager::Install() {
    InstallDriverStatus status = SetupDriverName();

    if (status != InstallDriverStatus::SuccessInstalled) {
        return status;
    }

    status = ManageDriver(DRIVER_FUNC_INSTALL);
    if (status != InstallDriverStatus::SuccessInstalled && status != InstallDriverStatus::ServiceAlreadyRunning && status != InstallDriverStatus::ServiceExists)  {
        ManageDriver(DRIVER_FUNC_REMOVE);
        return status;
    }

    return status;
}

InstallDriverStatus ServiceManager::Uninstall() {
    InstallDriverStatus status = SetupDriverName();
    if (status != InstallDriverStatus::SuccessInstalled) {
        return status;
    }

    return ManageDriver(DRIVER_FUNC_REMOVE);
}

InstallDriverStatus ServiceManager::SetupDriverName() {
    HANDLE fileHandle;
    if ((fileHandle = CreateFile(DriverLocation.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        return InstallDriverStatus::NotLoaded;
    }

    if (fileHandle) {
        CloseHandle(fileHandle);
    }

    return InstallDriverStatus::SuccessInstalled;
}

InstallDriverStatus ServiceManager::ManageDriver(_In_ USHORT Function) {
    if (DriverName.empty() || DriverLocation.empty()) {
        return InstallDriverStatus::InvalidDriverOrService;
    }

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            return InstallDriverStatus::InsufficientPermissions;
        }
        else {
            return InstallDriverStatus::UnknownError;
        }
    }

    InstallDriverStatus status = InstallDriverStatus::SuccessInstalled;
    switch (Function) {
    case DRIVER_FUNC_INSTALL:
        status = SetupDriver(schSCManager);
        if (status == (InstallDriverStatus::SuccessInstalled)) {
            status = StartDriver(schSCManager);
        }
        break;

    case DRIVER_FUNC_REMOVE:
        StopDriver(schSCManager);
        RemoveDriver(schSCManager);
        status = InstallDriverStatus::SuccessUninstalled;
        break;

    default:
        status = InstallDriverStatus::UnknownError;
        break;
    }

    if (schSCManager) {
        CloseServiceHandle(schSCManager);
    }

    return status;
}

InstallDriverStatus ServiceManager::SetupDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;
    DWORD err;

    schService = CreateService(SchSCManager, DriverName.c_str(), DriverName.c_str(), SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, DriverLocation.c_str(), NULL, NULL, NULL, NULL, NULL);

    if (schService == NULL) {
        err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            return InstallDriverStatus::ServiceExists;
        }
        else if (err == ERROR_SERVICE_MARKED_FOR_DELETE) {
            return InstallDriverStatus::ServiceMarkedForDelete;
        }
        else {
            return InstallDriverStatus::UnknownError;
        }
    }

    if (schService) {
        CloseServiceHandle(schService);
    }

    return InstallDriverStatus::SuccessInstalled;
}

InstallDriverStatus ServiceManager::StartDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;
    DWORD err;

    schService = OpenService(SchSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        return InstallDriverStatus::ServiceNotFound;
    }

    if (!StartService(schService, 0, NULL)) {
        err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            return InstallDriverStatus::ServiceAlreadyRunning;
        }
        else {
            return InstallDriverStatus::StartServiceFailed;
        }
    }

    if (schService) {
        CloseServiceHandle(schService);
    }

    return InstallDriverStatus::SuccessInstalled;
}

InstallDriverStatus ServiceManager::StopDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;
    SERVICE_STATUS serviceStatus;

    schService = OpenService(SchSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        return InstallDriverStatus::ServiceNotFound;
    }

    if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) {
        if (schService) {
            CloseServiceHandle(schService);
        }
        return InstallDriverStatus::SuccessUninstalled;
    }
    else {
        if (schService) {
            CloseServiceHandle(schService);
        }
        return InstallDriverStatus::ServiceControlStopFailed;
    }
}

InstallDriverStatus ServiceManager::RemoveDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;

    schService = OpenService(SchSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        return InstallDriverStatus::ServiceNotFound;
    }

    if (DeleteService(schService)) {
        if (schService) {
            CloseServiceHandle(schService);
        }
        return InstallDriverStatus::SuccessUninstalled;
    }
    else {
        if (schService) {
            CloseServiceHandle(schService);
        }
        return InstallDriverStatus::ServiceDeleteFailed;
    }
}
