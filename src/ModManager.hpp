#pragma once

class ModManager {
public:
    static ModManager* sharedState();

    ModManager();

    bool m_dontFadeOnStart = false;
    bool m_hideBtns = false;
    bool m_ignoreDisabled = false;
    double m_opacity = 0;
    bool m_favouritesByDefault = false;
    bool m_showFavouritesLabel = true;
};
