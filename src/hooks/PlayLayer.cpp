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

    loadFavourites();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

// ---------- Extra Practice ----------

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

void HookPlayLayer::toggleFavourite() {
    auto fields = m_fields.self();
    int idx = fields->m_startPosIdx;

    if (idx <= 0 || idx > static_cast<int>(fields->m_startPosObjects.size())) {
        Notification::create("Select a start position to mark it", NotificationIcon::Warning)->show();
        return;
    }

    auto key = startPosKey(fields->m_startPosObjects[idx - 1]);
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
        fmt::format("Start position {} {}", idx, nowFavourite ? "marked" : "unmarked"),
        nowFavourite ? NotificationIcon::Success : NotificationIcon::Info
    )->show();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

bool HookPlayLayer::isFavourite(int idx) {
    auto fields = m_fields.self();
    if (idx <= 0 || idx > static_cast<int>(fields->m_startPosObjects.size()))
        return false;
    return fields->m_favourites.contains(startPosKey(fields->m_startPosObjects[idx - 1]));
}

bool HookPlayLayer::hasAnyFavourite() {
    auto fields = m_fields.self();
    for (int i = 1; i <= static_cast<int>(fields->m_startPosObjects.size()); i++) {
        if (isFavourite(i))
            return true;
    }
    return false;
}

bool HookPlayLayer::isAllowed(int idx) {
    if (idx == 0)
        return true;
    if (ModManager::sharedState()->m_extraPractice)
        return true;
    // With nothing marked, Extra Practice off would leave only the level start,
    // so fall back to normal behaviour until at least one is marked.
    if (!hasAnyFavourite())
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
    Mod::get()->setSavedValue(levelSaveKey(), out);
}
