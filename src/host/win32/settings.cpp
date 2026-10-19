#include <windows.h>
#include <string.h>

#include "../../../interfaces/host.h"

int __stdcall ReadRegistryValue(const char* app, const char* key, void* buf, unsigned int* size);
int __stdcall AccessRegistryValue(char* subKey, char* valueName, LPBYTE data, LPDWORD size,
                                  DWORD type, DWORD read);
int __stdcall ReadRegistryData(void* subKey, void* valueName, void* data, void* size);
int __stdcall ReadRegistryDword(void* subKey, void* valueName, void* value);
void __stdcall WriteRegistryBinary(void* subKey, void* valueName, void* data, int unused);
void __stdcall WriteRegistryString(void* subKey, void* valueName, char* value);
void __stdcall WriteRegistryDword(char* key, char* name, int value);

// FUNCTION: 0x4b6860
int __stdcall ReadRegistryValue(const char* app, const char* key, void* buf, unsigned int* size)
{
    return AccessRegistryValue((char*)app, (char*)key, (LPBYTE)buf, (LPDWORD)size, 0, 1);
}

// FUNCTION: 0x4b6880
int __stdcall AccessRegistryValue(char* subKey, char* valueName, LPBYTE data, LPDWORD size,
                           DWORD type, DWORD read)
{
    int result;
    HKEY key1 = 0;
    HKEY key2 = 0;
    HKEY key3 = 0;
    REGSAM samDesired = read ? KEY_READ : KEY_WRITE;
    int doRead = read;
    DWORD disp;
    LONG err;
    result = 0;
    err = RegCreateKeyExA(HKEY_CURRENT_USER, "Software", 0, 0, 0, samDesired, 0,
                          &key1, &disp);
    if (err != 0)
        goto close;
    err = RegCreateKeyExA(key1, "Cavedog Entertainment", 0, 0, 0, samDesired, 0,
                          &key2, &disp);
    if (err != 0)
        goto close;
    err = RegCreateKeyExA(key2, subKey, 0, 0, 0, samDesired, 0, &key3, &disp);
    if (err != 0)
        goto close;
    if (doRead) {
        err = RegQueryValueExA(key3, valueName, 0, 0, data, size);
        if (err != 0 && err != ERROR_MORE_DATA)
            goto close;
    } else {
        err = RegSetValueExA(key3, valueName, 0, type, data, *size);
        if (err != 0)
            goto close;
    }
    result = 1;
close:
    if (key3 != 0) {
        RegCloseKey(key3);
    }
    if (key2 != 0) {
        RegCloseKey(key2);
    }
    if (key1 != 0) {
        RegCloseKey(key1);
    }
    return result;
}

// FUNCTION: 0x4b69b0
int __stdcall ReadRegistryData(void* subKey, void* valueName, void* data, void* size)
{
    return AccessRegistryValue((char*)subKey, (char*)valueName, (LPBYTE)data, (LPDWORD)size, 0, 1);
}

// FUNCTION: 0x4b69d0
int __stdcall ReadRegistryDword(void* subKey, void* valueName, void* value)
{
    int size = 4;
    return AccessRegistryValue((char*)subKey, (char*)valueName, (LPBYTE)value, (LPDWORD)&size, 0, 1);
}

// FUNCTION: 0x4b6a00
void __stdcall WriteRegistryBinary(void* subKey, void* valueName, void* data, int unused)
{
    AccessRegistryValue((char*)subKey, (char*)valueName, (LPBYTE)data, (LPDWORD)&unused, 3, 0);
}

// FUNCTION: 0x4b6a20
void __stdcall WriteRegistryString(void* subKey, void* valueName, char* value)
{
    unsigned int size = strlen(value) + 1;
    AccessRegistryValue((char*)subKey, (char*)valueName, (LPBYTE)value, (LPDWORD)&size, 1, 0);
}

// FUNCTION: 0x4b6a50
void __stdcall WriteRegistryDword(char* key, char* name, int value)
{
    unsigned int size = 4;
    AccessRegistryValue(key, name, (LPBYTE)&value, (LPDWORD)&size, 4, 0);
}

static DWORD SystemValueType(int type)
{
    if (type == HOST_SETTING_INT) {
        return REG_DWORD;
    }
    if (type == HOST_SETTING_STRING) {
        return REG_SZ;
    }
    return REG_BINARY;
}

extern "C" int HOST_CALL host_setting_get(int scope, const char* section, const char* name, int type, void* buf, unsigned int* size)
{
    if (scope == HOST_SCOPE_SYSTEM || scope == HOST_SCOPE_SYSTEM_READ) {
        HKEY key = 0;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, section, 0, scope == HOST_SCOPE_SYSTEM ? 0xf003f : KEY_READ, &key) != 0) {
            return HOST_SETTING_NO_ACCESS;
        }
        DWORD valueType;
        LONG err = RegQueryValueExA(key, name, NULL, &valueType, (LPBYTE)buf, (LPDWORD)size);
        RegCloseKey(key);
        return err == 0 ? 1 : 0;
    }
    if (type == HOST_SETTING_INT) {
        return ReadRegistryDword((void*)section, (void*)name, buf);
    }
    if (type == HOST_SETTING_STRING) {
        return ReadRegistryData((void*)section, (void*)name, buf, size);
    }
    return ReadRegistryValue(section, name, buf, size);
}

extern "C" int HOST_CALL host_setting_set(int scope, const char* section, const char* name, int type, const void* buf, unsigned int size)
{
    if (scope == HOST_SCOPE_SYSTEM) {
        HKEY key = 0;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, section, 0, 0xf003f, &key) != 0) {
            return HOST_SETTING_NO_ACCESS;
        }
        if (type == HOST_SETTING_STRING) {
            size = (unsigned int)strlen((const char*)buf) + 1;
        } else if (type == HOST_SETTING_INT) {
            size = 4;
        }
        LONG err = RegSetValueExA(key, name, 0, SystemValueType(type), (const BYTE*)buf, size);
        RegFlushKey(key);
        RegCloseKey(key);
        return err == 0 ? 1 : 0;
    }
    if (type == HOST_SETTING_INT) {
        WriteRegistryDword((char*)section, (char*)name, *(const int*)buf);
    } else if (type == HOST_SETTING_STRING) {
        WriteRegistryString((void*)section, (void*)name, (char*)buf);
    } else {
        WriteRegistryBinary((void*)section, (void*)name, (void*)buf, (int)size);
    }
    return 1;
}
