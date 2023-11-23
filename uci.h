// uci.h

#ifndef UCI_H
#define UCI_H

#include "position.h"  // Include your Position class

class UCI {
public:
    void run();
    void position(Position& pos);
    void go();
    void perftTest(Position& pos, int depth);

};

#endif // UCI_H
