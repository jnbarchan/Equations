#ifndef SETTINGS_H
#define SETTINGS_H

class Settings
{
public:
    Settings();

    bool useDiceColour(int index) const { return _useDiceColours[index]; }
    void setUseDiceColour(int index, bool value) { _useDiceColours[index] = value; }
    bool adminMode() const { return _adminMode; }
    void setAdminMode(bool newAdminMode) { _adminMode = newAdminMode; }
    int maxOperands() const { return _maxOperands; }
    void setMaxOperands(int newMaxOperands) { _maxOperands = newMaxOperands; }
    int maxEvaluations() const { return _maxEvaluations; }
    void setMaxEvaluations(int newMaxEvaluations) { _maxEvaluations = newMaxEvaluations; }
    int maxTime() const { return _maxTime; }
    void setMaxTime(int newMaxTime) { _maxTime = newMaxTime; }
    int useThreads() const { return _useThreads; }
    void setUseThreads(int newUseThreads) { _useThreads = newUseThreads; }

private:
    bool _useDiceColours[4];
    bool _adminMode;
    int _maxOperands;
    int _maxEvaluations;
    int _maxTime;
    bool _useThreads;
};

extern Settings *g_settings;

#endif // SETTINGS_H
