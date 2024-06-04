#include "pch.h"
#include "TaskSchedulerLib.h"

bool DoesScheduledTaskExist(const std::wstring& taskName) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library. Error code: " << hr << std::endl;
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create an instance of ITaskService. Error code: " << hr << std::endl;
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::wcerr << L"ITaskService::Connect failed. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get Root Folder pointer. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->GetTask(_bstr_t(taskName.c_str()), &pRegisteredTask);
    if (SUCCEEDED(hr)) {
        pRegisteredTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return true;
    }

    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
    return false;
}

bool CreateScheduledTask(const std::wstring& taskName, const std::wstring& executablePath, const std::wstring& arguments) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library. Error code: " << hr << std::endl;
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create an instance of ITaskService. Error code: " << hr << std::endl;
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::wcerr << L"ITaskService::Connect failed. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get Root Folder pointer. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create a task definition. Error code: " << hr << std::endl;
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get Task collection pointer. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot create the action. Error code: " << hr << std::endl;
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IExecAction* pExecAction = NULL;
    hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    if (FAILED(hr)) {
        std::wcerr << L"QueryInterface call failed for IExecAction. Error code: " << hr << std::endl;
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pExecAction->put_Path(_bstr_t(executablePath.c_str()));
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put path to exec action. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pExecAction->put_Arguments(_bstr_t(arguments.c_str()));
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put arguments to exec action. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegistrationInfo* pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get identification pointer. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    BSTR author = SysAllocString(L"Rominky Soft");
    hr = pRegInfo->put_Author(author);
    SysFreeString(author);
    pRegInfo->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put identification info. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get trigger collection. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Создание триггера для запуска при входе пользователя
    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    pTriggerCollection->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot create logon trigger. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    ILogonTrigger* pLogonTrigger = NULL;
    hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
    pTrigger->Release();
    if (FAILED(hr)) {
        std::wcerr << L"QueryInterface call failed for ILogonTrigger. Error code: " << hr << std::endl;
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Установка идентификатора триггера (опционально)
    hr = pLogonTrigger->put_Id(_bstr_t(L"Trigger1"));
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put trigger ID. Error code: " << hr << std::endl;
        pLogonTrigger->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    pLogonTrigger->Release();

    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get principal pointer. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put principal info. Error code: " << hr << std::endl;
        pPrincipal->Release();
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    pPrincipal->Release();
    if (FAILED(hr)) {
        std::wcerr << L"Cannot put run level. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pFolder);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get root folder pointer. Error code: " << hr << std::endl;
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    IRegisteredTask* pRegisteredTask = NULL;
    hr = pFolder->RegisterTaskDefinition(_bstr_t(taskName.c_str()), pTask, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask);
    if (FAILED(hr)) {
        std::wcerr << L"Error saving the Task. Error code: " << hr << std::endl;
        pFolder->Release();
        pExecAction->Release();
        pAction->Release();
        pActionCollection->Release();
        pTask->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    std::wcout << L"Task successfully registered." << std::endl;

    pRegisteredTask->Release();
    pFolder->Release();
    pExecAction->Release();
    pAction->Release();
    pActionCollection->Release();
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    return true;
}

bool DeleteScheduledTask(const std::wstring& taskName) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM library. Error code: " << hr << std::endl;
        return false;
    }

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create an instance of ITaskService. Error code: " << hr << std::endl;
        CoUninitialize();
        return false;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::wcerr << L"ITaskService::Connect failed. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::wcerr << L"Cannot get Root Folder pointer. Error code: " << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return false;
    }

    hr = pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to delete the task. Error code: " << hr << std::endl;
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    pRootFolder->Release();
    pService->Release();
    CoUninitialize();
    std::wcout << L"Task successfully deleted." << std::endl;

    return true;
}
