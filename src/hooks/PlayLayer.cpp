#include "PlayLayer.hpp"
#include "Geode/binding/StartPosObject.hpp"
#include "UILayer.hpp"
#include "../ModManager.hpp"

using namespace geode::prelude;

void HookPlayLayer::addObject(GameObject* obj) {
    if (obj->m_objectID == 31) {
        if(!static_cast<StartPosObject*>(obj)->m_startSettings->m_disableStartPos || !ModManager::sharedState()->m_ignoreDisabled)
            m_fields->m_startPosObjects.push_back(obj);
    }
    PlayLayer::addObject(obj);
}

void HookPlayLayer::updateStartPos(int idx) {
    auto fields = m_fields.self();

    if (fields->m_startPosObjects.size() == 0)
        return;

    if(idx < 0) idx = fields->m_startPosObjects.size();
    if(idx > fields->m_startPosObjects.size()) idx = 0;

    
    if(idx == 0) {
        m_isTestMode = false;
        updateTestModeLabel();
    } else {
        m_isTestMode = true;
        updateTestModeLabel();
    }

    m_currentCheckpoint = nullptr;
    fields->m_startPosIdx = idx;

    GameObject* object = nullptr;

    if(true) {
        object = idx > 0 ? fields->m_startPosObjects[idx - 1] : nullptr;
    } else {
        auto rand = std::rand() % fields->m_startPosObjects.size() + 1;
        object = idx > 0 ? fields->m_startPosObjects[rand] : nullptr;
    }
    setStartPosObject(static_cast<StartPosObject*>(object));

    if(m_isPracticeMode)
        resetLevelFromStart();

    resetLevel();
    startMusic();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

void HookPlayLayer::createObjectsFromSetupFinished() {
    PlayLayer::createObjectsFromSetupFinished();
    auto fields = m_fields.self();

    std::sort(fields->m_startPosObjects.begin(), fields->m_startPosObjects.end(), [](auto a, auto b) { return a->getPositionX() < b->getPositionX(); });

    if(this->m_startPosObject) {
        auto currentIdx = find(fields->m_startPosObjects.begin(), fields->m_startPosObjects.end(), this->m_startPosObject) - fields->m_startPosObjects.begin();
        fields->m_startPosIdx = currentIdx + 1;
    }

    // Work out each start position's save key once, from its position at load,
    // so triggers moving it mid-level can't change which key it matches.
    fields->m_startPosKeys.clear();
    for (auto& obj : fields->m_startPosObjects)
        fields->m_startPosKeys.push_back(startPosKey(obj));

    loadFavourites();
    fields->m_favouritesMode = ModManager::sharedState()->m_favouritesByDefault;

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

// ---------- Favourites ----------

// Moves one step in `direction` (+1 / -1), skipping start positions that are
// not allowed in the current mode. Index 0 (level start) is always allowed.
void HookPlayLayer::switchStartPos(int direction) {
    auto fields = m_fields.self();
    int count = static_cast<int>(fields->m_startPosObjects.size());
    if (count == 0)
        return;

    int idx = fields->m_startPosIdx;
    for (int step = 0; step <= count; step++) {
        idx += direction;
        if (idx < 0) idx = count;
        if (idx > count) idx = 0;
        if (isAllowed(idx))
            break;
    }
    updateStartPos(idx);
}

void HookPlayLayer::toggleFavouritesMode() {
    auto fields = m_fields.self();
    fields->m_favouritesMode = !fields->m_favouritesMode;

    std::string text = fields->m_favouritesMode ? "Favourites: ON" : "Favourites: OFF";
    if (fields->m_favouritesMode && favouriteCount() == 0)
        text += " (no favourites marked yet)";
    Notification::create(text, NotificationIcon::Info)->show();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

void HookPlayLayer::toggleFavourite() {
    auto fields = m_fields.self();
    int idx = fields->m_startPosIdx;

    if (idx <= 0 || idx > static_cast<int>(fields->m_startPosKeys.size())) {
        Notification::create("Select a start position to favourite it", NotificationIcon::Warning)->show();
        return;
    }

    auto const& key = fields->m_startPosKeys[idx - 1];
    bool nowFavourite;
    if (fields->m_favourites.contains(key)) {
        fields->m_favourites.erase(key);
        nowFavourite = false;
    } else {
        fields->m_favourites.insert(key);
        nowFavourite = true;
    }
    saveFavourites();

    Notification::create(
        fmt::format("Start position {} {} favourites", idx, nowFavourite ? "added to" : "removed from"),
        nowFavourite ? NotificationIcon::Success : NotificationIcon::Info
    )->show();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

bool HookPlayLayer::isFavourite(int idx) {
    auto fields = m_fields.self();
    if (idx <= 0 || idx > static_cast<int>(fields->m_startPosKeys.size()))
        return false;
    return fields->m_favourites.contains(fields->m_startPosKeys[idx - 1]);
}

int HookPlayLayer::favouriteCount() {
    auto fields = m_fields.self();
    int count = 0;
    for (int i = 1; i <= static_cast<int>(fields->m_startPosKeys.size()); i++) {
        if (isFavourite(i))
            count++;
    }
    return count;
}

// 1-based position of start position `idx` among the favourites, or 0 if it isn't one
int HookPlayLayer::favouriteRank(int idx) {
    if (!isFavourite(idx))
        return 0;
    int rank = 0;
    for (int i = 1; i <= idx; i++) {
        if (isFavourite(i))
            rank++;
    }
    return rank;
}

// Favourites mode only filters once at least one favourite exists; otherwise
// it would leave nothing but the level start to switch to.
bool HookPlayLayer::isFilteringFavourites() {
    return m_fields->m_favouritesMode && favouriteCount() > 0;
}

bool HookPlayLayer::isAllowed(int idx) {
    if (idx == 0)
        return true;
    if (!isFilteringFavourites())
        return true;
    return isFavourite(idx);
}

std::string HookPlayLayer::levelSaveKey() {
    if (!m_level)
        return "favourites-unknown";
    if (m_level->m_levelType == GJLevelType::Editor)
        return fmt::format("favourites-editor-{}", std::string(m_level->m_levelName));
    return fmt::format("favourites-{}", m_level->m_levelID.value());
}

std::string HookPlayLayer::startPosKey(GameObject* obj) {
    auto pos = obj->getPosition();
    return fmt::format("{},{}", static_cast<int>(std::round(pos.x)), static_cast<int>(std::round(pos.y)));
}

void HookPlayLayer::loadFavourites() {
    auto fields = m_fields.self();
    auto saved = Mod::get()->getSavedValue<std::vector<std::string>>(levelSaveKey(), {});
    fields->m_favourites = std::unordered_set<std::string>(saved.begin(), saved.end());
}

void HookPlayLayer::saveFavourites() {
    auto fields = m_fields.self();
    std::vector<std::string> out(fields->m_favourites.begin(), fields->m_favourites.end());
    std::sort(out.begin(), out.end());
    Mod::get()->setSavedValue(levelSaveKey(), out);

    // Geode normally only writes saved values to disk when GD saves on a clean
    // exit, so a crash or force-close loses them. Write straight away instead.
    if (auto res = Mod::get()->saveData(); !res) {
        log::error("Failed to save favourites: {}", res.unwrapErr());
    }
}
