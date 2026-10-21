#ifndef DEBUG_COMMAND_LINE_H
#define DEBUG_COMMAND_LINE_H

class CommandLineSwitch {
public:
    char on;
    CommandLineSwitch(char* name, int a, char def, char* onSwitch,
                   char* offSwitch, char* onSwitch2, char* offSwitch2);
    ~CommandLineSwitch() {}
};

#endif
