#pragma once

#include<iostream>
#include<windows.h>

// color: foreground + background * 16
struct Log {
    struct Colors {
        static const int BLACK = 0;
        static const int BLUE = 9;
        static const int GREEN = 10;
        static const int RED = 12;
        static const int WHITE = 15;
        static const int DEFAULT = WHITE;
        static const int INVERTED = DEFAULT * 16;
    };

    static void write(std::string msg, int colors = Colors::DEFAULT) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colors);
        std::cout << msg;
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Colors::DEFAULT);
    }

    static void writeLine(std::string msg="", int colors = Colors::DEFAULT) {
        write(msg + '\n', colors);
    }
};
