// TdfFile and TdfRecord as the rest of the game sees them: a TDF text file is
// loaded into a tree of sections (TdfRecord), and the file keeps a root and a
// current section. Callers select a section and read its `name = value;`
// entries by key. The layout of a section and the code that builds the tree
// stay in tdf.cpp; callers only hold pointers to a
// TdfRecord, so only its methods are declared here.
#ifndef TDF_H
#define TDF_H

#include <string.h>

#pragma pack(push, 8)
#include "string_ref.h"

class TdfRecord;

struct TdfField {
    StringRef key;
    StringRef value;

    TdfField(const StringRef& a, const StringRef& b);
    // FUNCTION: 0x4c54a0 ??0TdfField@@QAE@ABU0@@Z
    TdfField(const TdfField& other)
        : key(other.key), value(other.value)
    {
    }
    ~TdfField();
    void* AssignPair(int* param_1);
};
#pragma pack(pop)

#pragma pack(push, 1)
struct TdfRecordList {
    char unknown_0[4];
    TdfRecord** first;
    TdfRecord** last;
    TdfRecord** end;
};

struct TdfFieldList {
    char unknown_0[4];
    TdfField* first;
    TdfField* last;
    TdfField* end;
};

struct TdfEntries {
    char less;
    TdfFieldList v;
};

class TdfRecord {
public:
    char* name;
    TdfRecordList children;
    TdfEntries entries;
    char* text;

    TdfRecord(char* name, char* text, char** nextblock, char* filename);
    ~TdfRecord();

    StringRef MakeTrimmedString(char* start, char* end);
    TdfRecord* FindChild(char* key);
    TdfRecord* GetChild(int index);
    void ComputeRecordChecksum(char* start, char* end);
    TdfRecord* FindSubRecord(const char* name);
    int GetRecordName();
    char* GetFieldName(int index);
    int FindFieldValue(char* name);
    int* GetFieldFixed(int* dst, char* key, int def);
    int GetFieldInt(const char* name, int def);
    double GetFieldDouble(const char* name, double def);
    int GetFieldString(char* dst, char* key, size_t size, char* def);
    void CopyRecordName(char* dest, size_t count);
    int GetSubRecordCount();
    TdfRecord* GetSubRecord(int index);
    int GetFieldCount();
};
#pragma pack(pop)

class TdfFile {
public:
    TdfRecord* root;                   // +0x0
    TdfRecord* current;                // +0x4
    int flags;                         // +0x8

    TdfFile();
    ~TdfFile();
    int GetString(char* dst, char* key, int size, char* def)
    {
        return current->GetFieldString(dst, key, size, def);
    }
    int GetInt(char* key, int def) { return current->GetFieldInt(key, def); }
    double GetDouble(char* key, double def) { return current->GetFieldDouble(key, def); }
    char* GetValue(char* key) { return (char*)current->FindFieldValue(key); }
    int LoadFile(char* path);
    void StripComments(char* p);
    void LoadBuffer(char* data, int size, int flag, char* path);
    void Unload();
    int SelectRecord(char* name);
    int SelectRecordAt(int index);
    void ResetCurrentRecord();
    int GetCurrentRecord();
    void SetCurrentRecord(int val);
};

#endif
