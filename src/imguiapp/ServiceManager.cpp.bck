#include "pch.h"
#include "ServiceManager.h"


ServiceManager::ServiceManager(std::string path){

    DriverLocation = path + DriverName + ".sys";
}

ServiceManager::~ServiceManager() {
    Uninstall();
}

bool ServiceManager::Install() {
    bool status = true;
    if (!SetupDriverName()) {
        return !status;
    }

    if (!ManageDriver(DRIVER_FUNC_INSTALL)) {
        ManageDriver(DRIVER_FUNC_REMOVE);
        return !status;
    }

    DbgPrint("Driver %s.sys %sinstalled!\n", DriverName.c_str(), status ? "" : "not");
    return status;
}

bool ServiceManager::Uninstall() {
    bool status = true;
    if (!SetupDriverName()) {
        return !status;
    }

    status = ManageDriver(DRIVER_FUNC_REMOVE);
    DbgPrint("Driver %s.sys %suninstalled!\n", DriverName.c_str(), status ? "" : "not");
    return status;
}

bool ServiceManager::SetupDriverName() {

    HANDLE fileHandle;
    DbgPrint("DriverLocation.c_str() %s\n", DriverLocation.c_str());
    if ((fileHandle = CreateFile(DriverLocation.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        DbgPrint("%s.sys is not loaded.\n", DriverName.c_str());
        return false;
    }

    if (fileHandle) {
        CloseHandle(fileHandle);
    }

    return true;
}

bool ServiceManager::ManageDriver(_In_ USHORT Function) {
    SC_HANDLE schSCManager;
    bool status = true;

    if (!DriverName.c_str() || !DriverLocation.c_str()) {
        DbgPrint("Invalid Driver or Service provided to ManageDriver() \n");
        return false;
    }

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) {
        DbgPrint("Open SC Manager failed! Error = %d \n", GetLastError());
        return false;
    }

    switch (Function) {
    case DRIVER_FUNC_INSTALL:
        if (SetupDriver(schSCManager)) {
            status = StartDriver(schSCManager);
        }
        else {
            status = false;
        }
        break;

    case DRIVER_FUNC_REMOVE:
        StopDriver(schSCManager);
        RemoveDriver(schSCManager);
        status = true;
        break;

    default:
        DbgPrint("Unknown ManageDriver() function. \n");
        status = false;
        break;
    }

    if (schSCManager) {
        CloseServiceHandle(schSCManager);
    }

    return status;
}

bool ServiceManager::SetupDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;
    DWORD err;

    schService = CreateService(SchSCManager, DriverName.c_str(), DriverName.c_str(), SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, DriverLocation.c_str(), NULL, NULL, NULL, NULL, NULL);

    if (schService == NULL) {
        err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            return true;
        }
        else if (err == ERROR_SERVICE_MARKED_FOR_DELETE) {
            DbgPrint("Previous instance of the service is not fully deleted. Try again...\n");
            return false;
        }
        else {
            DbgPrint("CreateService failed!  Error = %d \n", err);
            return false;
        }
    }

    if (schService) {
        CloseServiceHandle(schService);
    }

    return true;
}

bool ServiceManager::StartDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;
    DWORD err;

    schService = OpenService(SchSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        DbgPrint("OpenService failed!  Error = %d \n", GetLastError());
        return false;
    }

    if (!StartService(schService, 0, NULL)) {
        err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            return true;
        }
        else {
            DbgPrint("StartService failure! Error = %d \n", err);
            return false;
        }
    }

    if (schService) {
        CloseServiceHandle(schService);
    }

    return true;
}

bool ServiceManager::StopDriver(_In_ SC_HANDLE SchSCManager) {
    bool status = true;
    SC_HANDLE schService;
    SERVICE_STATUS serviceStatus;

    schService = OpenService(SchSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        DbgPrint("OpenService failed!  Error = %d \n", GetLastError());
        return false;
    }

    if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) {
        status = true;
    }
    else {
        DbgPrint("ControlService failed!  Error = %d \n", GetLastError());
        status = false;
    }

    if (schService) {
        CloseServiceHandle(schService);
    }

    return status;
}

bool ServiceManager::RemoveDriver(_In_ SC_HANDLE SchSCManager) {
    SC_HANDLE schService;
    bool status;

    schService = OpenService(SchSCManager, DriverName.c_str(), SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        DbgPrint("OpenService failed!  Error = %d \n", GetLastError());
        return false;
    }

    if (DeleteService(schService)) {
        status = true;
    }
    else {
        DbgPrint("DeleteService failed!  Error = %d \n", GetLastError());
        status = false;
    }

    if (schService) {
        CloseServiceHandle(schService);
    }

    return status;
}