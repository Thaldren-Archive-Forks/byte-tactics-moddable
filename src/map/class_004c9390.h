#ifndef CLASS_004C9390_H
#define CLASS_004C9390_H

class Class_004c9390 {
public:
    char* data;
    Class_004c9390(Class_004c9390&);
    ~Class_004c9390(void) { ReleaseRef(); }
    Class_004c9390(void);
    Class_004c9390* Assign(Class_004c9390*);
    void ReleaseRef(void);
    Class_004c9390& operator=(Class_004c9390&);
};

#endif
