#pragma once

class ModManager {
public:
    static ModManager* sharedState();

    ModManager();

    bool m_dontFadeOnStart = false;
    bool m_hideBtns = false;
    bool m_ignoreDisabled = false;
    double m_opacity = 0;
    // Not saved: Extra Practice starts off every time the game launches
    bool m_extraPractice = false;
};
