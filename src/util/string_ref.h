#ifndef STRING_REF_H
#define STRING_REF_H

#include <stdlib.h>
#include <string.h>

class StringRef {
public:
    union {
        char* p;
        char* data;
        char* ptr;
    };

    StringRef();
    StringRef(const StringRef& other);
    StringRef(const char* text);
    StringRef(const char* text, int len);
    ~StringRef() { ReleaseRef(); }
    StringRef& operator=(const StringRef& other)
    {
        Assign(&other);
        return *this;
    }
    void ReleaseRef();
    StringRef* Assign(const StringRef* other);
    StringRef* Append(const StringRef& other);
    StringRef* MakeLower();
    StringRef* MakeUpper();
    StringRef* AssignText(const char* text);
    char* GetUnique()
    {
        int len = (int)strlen(data);
        if (*(int*)(data - 4) != 1) {
            int* block = (int*)malloc(len + 5);
            *block = 1;
            char* copy = (char*)(block + 1);
            strcpy(copy, data);
            (*(int*)(data - 4))--;
            if (*(int*)(data - 4) == 0) {
                free(data - 4);
            }
            data = copy;
            return copy;
        }
        return data;
    }
    int IsEmpty() const;
    StringRef SubString(int start, int end) const;
    void* FUN_00432c00(unsigned char param_1);
    void* FUN_00432c20(unsigned char param_1);
};

#endif
