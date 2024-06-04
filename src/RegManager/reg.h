#ifndef REG_H
#define REG_H

#include <Windows.h>

class __declspec(dllexport) reg {
private:
    static HKEY hKey;
    static LONG result;

    static bool IsStringKeyExist(LONG& result, HKEY& hKey, const char* valueName);

public:
    static void EditKeyString(const char* keyParam, const char* valueParam);
    static void EditKeyDWORD(const char* keyParam, DWORD valueParam);
    static void CreateKey(const char* keyParam, const char* valueParam);
    static char* GetValueString(const char* keyParam);
    static DWORD GetValueDWORD(const char* keyParam);

    ~reg();
};

#endif // REG_H