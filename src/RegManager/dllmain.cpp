// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "reg.h"
#include <iostream>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



HKEY reg::hKey = HKEY_CURRENT_USER;
LONG reg::result = RegCreateKeyExA(reg::hKey, "Software\\Rominky Soft\\DXVK addition", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &reg::hKey, NULL);

bool reg::IsStringKeyExist(LONG& result, HKEY& hKey, const char* valueName) {
    DWORD dwType;
    DWORD dwDataSize = 0;

    result = RegQueryValueExA(hKey, valueName, NULL, &dwType, NULL, &dwDataSize);

    return dwType == REG_SZ;
}

void reg::EditKeyString(const char* keyParam, const char* valueParam) {
    if (result == ERROR_SUCCESS) {
        result = RegSetValueExA(hKey, keyParam, 0, REG_SZ, (BYTE*)valueParam, strlen(valueParam) + 1);
        printf("Создан ключ со значением\n");
        if (result != ERROR_SUCCESS) {
            printf("Ошибка при создании ключа или значения: %d\n", result);
        }
    }
    else {
        printf("Ошибка открытия ключа: %d\n", result);
    }
}

void reg::EditKeyDWORD(const char* keyParam, DWORD valueParam) {
    if (result == ERROR_SUCCESS) {
        result = RegSetValueExA(hKey, keyParam, 0, REG_DWORD, (BYTE*)&valueParam, sizeof(DWORD));
        printf("Создан ключ со значением\n");
        if (result != ERROR_SUCCESS) {
            printf("Ошибка при создании ключа или значения: %d\n", result);
        }
    }
    else {
        printf("Ошибка открытия ключа: %d\n", result);
    }
}

void reg::CreateKey(const char* keyParam, const char* valueParam) {
    if (result == ERROR_SUCCESS) {
        if (!IsStringKeyExist(result, hKey, keyParam)) {
            if (result != ERROR_SUCCESS) {
                result = RegSetValueExA(hKey, keyParam, 0, REG_SZ, (BYTE*)valueParam, strlen(valueParam) + 1);
                printf("Создание ключа со значением\n");
                if (result != ERROR_SUCCESS) {
                    printf("Ошибка при создании ключа или значения: %d\n", result);
                }
            }
        }
        else {
            printf("Ключ присутствует\n");
        }
    }
    else {
        printf("Ошибка открытия ключа: %d\n", result);
    }
}

char* reg::GetValueString(const char* keyParam) {
    if (result == ERROR_SUCCESS) {
        const int bufferSize = 1024; // Максимальный размер значения ключа
        char* buffer = new char[bufferSize];
        DWORD bufferSizeBytes = bufferSize; 
        DWORD dwType;
        result = RegQueryValueExA(hKey, keyParam, NULL, &dwType, (LPBYTE)buffer, &bufferSizeBytes);
        if (result == ERROR_SUCCESS && dwType == REG_SZ)
            return buffer;
        else
            return nullptr;
    }
    else {
        printf("Ошибка открытия ключа: %d\n", result);
        return nullptr;
    }
}

DWORD reg::GetValueDWORD(const char* keyParam) {
    DWORD value = 0;
    DWORD bufferSizeBytes = sizeof(DWORD);
    DWORD dwType;
    result = RegQueryValueExA(hKey, keyParam, NULL, &dwType, (LPBYTE)&value, &bufferSizeBytes);
    if (result == ERROR_SUCCESS && dwType == REG_DWORD)
        return value;
    else
        return 0;
}

reg::~reg() {
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }
}