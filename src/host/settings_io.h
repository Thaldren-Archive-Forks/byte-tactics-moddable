#ifndef HOST_SETTINGS_IO_H
#define HOST_SETTINGS_IO_H

#include "../../interfaces/host.h"

static inline int SettingGetInt(const void* section, const void* name, void* value)
{
    return host_setting_get(HOST_SCOPE_GAME, (const char*)section, (const char*)name, HOST_SETTING_INT, value, 0) == 1;
}

static inline int SettingGetString(const void* section, const void* name, void* buf, void* size)
{
    return host_setting_get(HOST_SCOPE_GAME, (const char*)section, (const char*)name, HOST_SETTING_STRING, buf,
                            (unsigned int*)size) == 1;
}

static inline int SettingGetBlob(const void* section, const void* name, void* buf, unsigned int* size)
{
    return host_setting_get(HOST_SCOPE_GAME, (const char*)section, (const char*)name, HOST_SETTING_BLOB, buf, size) == 1;
}

static inline void SettingSetInt(const void* section, const void* name, int value)
{
    host_setting_set(HOST_SCOPE_GAME, (const char*)section, (const char*)name, HOST_SETTING_INT, &value, 4);
}

static inline void SettingSetString(const void* section, const void* name, const void* value)
{
    host_setting_set(HOST_SCOPE_GAME, (const char*)section, (const char*)name, HOST_SETTING_STRING, value, 0);
}

static inline void SettingSetBlob(const void* section, const void* name, const void* data, int size)
{
    host_setting_set(HOST_SCOPE_GAME, (const char*)section, (const char*)name, HOST_SETTING_BLOB, data, (unsigned int)size);
}

#endif
