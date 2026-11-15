#include "settings.h"

Settings *g_settings = new Settings;

Settings::Settings()
{
    _useDiceColours[0] = true;  // red
    _useDiceColours[1] = true;  // blue
    _useDiceColours[2] = true;  // green
    _useDiceColours[3] = false;  // black
    _adminMode = true;
    _maxOperands = 0;  //5;
    _maxEvaluations = 0;  //5000000;
    _maxTime = 10;  //10;
    _useThreads = true;
}
