#include "UILayer.hpp"
#include "PlayLayer.hpp"
#include "../ModManager.hpp"

using namespace geode::prelude;

bool HookUILayer::init(GJBaseGameLayer* baseGame) {
    if(!UILayer::init(baseGame))
        return false;

    if(auto playLayer = typeinfo_cast<PlayLayer*>(baseGame)) {
        auto director = CCDirector::sharedDirector();
        auto winSize = director->getWinSize();
        auto fields = m_fields.self();
        auto mm = ModManager::sharedState();

        fields->m_switcherMenu = CCMenu::create();
        fields->m_switcherMenu->setPosition(ccp(winSize.width / 2, director->getScreenBottom() + 17.f));
        fields->m_switcherMenu->setID("menu"_spr);

        fields->m_switcherLabel = CCLabelBMFont::create("0/0", "bigFont.fnt");
        fields->m_switcherLabel->setID("idx-label"_spr);
        fields->m_switcherLabel->setScale(0.6);

        bool gamepad = PlatformToolbox::isControllerConnected() && false;

        auto nextSpr = CCSprite::createWithSpriteFrameName(gamepad ? "controllerBtn_DPad_Right_001.png" : "GJ_arrow_02_001.png");
        auto prevSpr = CCSprite::createWithSpriteFrameName(gamepad ? "controllerBtn_DPad_Left_001.png" : "GJ_arrow_02_001.png");
        nextSpr->setScale(0.6);
        prevSpr->setScale(0.6);
        nextSpr->setFlipX(true);

        fields->m_nextSwitcherBtn = CCMenuItemExt::createSpriteExtra(nextSpr, [this](CCObject* sender){
            auto playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
            playLayer->switchStartPos(1);
        });
        fields->m_nextSwitcherBtn->setID("next-btn"_spr);

        fields->m_prevSwitcherBtn = CCMenuItemExt::createSpriteExtra(prevSpr, [this](CCObject* sender){
            auto playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
            playLayer->switchStartPos(-1);
        });
        fields->m_prevSwitcherBtn->setID("prev-btn"_spr);

        fields->m_switcherMenu->addChild(fields->m_prevSwitcherBtn);
        fields->m_switcherMenu->addChild(fields->m_switcherLabel);
        fields->m_switcherMenu->addChild(fields->m_nextSwitcherBtn);
        fields->m_switcherMenu->setLayout(AxisLayout::create()->setAutoScale(false)->setGap(10));

        if(mm->m_hideBtns) {
            fields->m_prevSwitcherBtn->setVisible(false);
            fields->m_nextSwitcherBtn->setVisible(false);
        }

        addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "leftSwitch"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                auto playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
                playLayer->switchStartPos(-1);
            }
        });

        addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "rightSwitch"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                auto playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
                playLayer->switchStartPos(1);
            }
        });

        addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "toggleExtraPractice"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                auto mm = ModManager::sharedState();
                mm->m_extraPractice = !mm->m_extraPractice;
                Notification::create(
                    mm->m_extraPractice ? "Extra Practice: ON" : "Extra Practice: OFF",
                    NotificationIcon::Info
                )->show();
                this->updateUI();
            }
        });

        addEventListener(KeybindSettingPressedEventV3(GEODE_MOD_ID, "toggleFavourite"), [this](const Keybind& keybind, bool down, bool repeat, double timestamp) {
            if (down && !repeat) {
                auto playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
                playLayer->toggleFavourite();
            }
        });

        // Shown above the switcher while Extra Practice is on
        fields->m_modeLabel = CCLabelBMFont::create("Extra Practice", "goldFont.fnt");
        fields->m_modeLabel->setID("extra-practice-label"_spr);
        fields->m_modeLabel->setScale(0.45f);
        fields->m_modeLabel->setPosition(ccp(winSize.width / 2, director->getScreenBottom() + 36.f));
        fields->m_modeLabel->setVisible(false);
        this->addChild(fields->m_modeLabel);

        this->addChild(fields->m_switcherMenu);
    }

    return true;
}

static CCAction* makeFadeAction(double restOpacity) {
    auto action = CCSequence::create(CCEaseInOut::create(CCFadeTo::create(0.3f, 255), 2), CCDelayTime::create(0.5), CCEaseInOut::create(CCFadeTo::create(0.5f, restOpacity), 2), nullptr);
    action->setTag(676767677);
    return action;
}

void HookUILayer::updateUI() {
    auto playLayer = static_cast<HookPlayLayer*>(PlayLayer::get());
    auto fields = m_fields.self();
    auto mm = ModManager::sharedState();

    if(playLayer->m_fields->m_startPosObjects.empty()) {
        fields->m_switcherMenu->setVisible(false);
        fields->m_modeLabel->setVisible(false);
        return;
    } else {
        fields->m_switcherMenu->setVisible(true);
        fields->m_modeLabel->setVisible(mm->m_extraPractice);
    }

    int idx = playLayer->m_fields->m_startPosIdx;
    fields->m_switcherLabel->setString(fmt::format("{}/{}", idx, playLayer->m_fields->m_startPosObjects.size()).c_str());
    fields->m_switcherLabel->limitLabelWidth(40, 0.6f, 0);

    // Marked ("accessible at all times") start positions show in green
    fields->m_switcherLabel->setColor(playLayer->isFavourite(idx) ? ccc3(0, 255, 0) : ccc3(255, 255, 255));

    fields->m_switcherMenu->stopActionByTag(676767677);
    fields->m_modeLabel->stopActionByTag(676767677);
    if(fields->m_firstUpdate && mm->m_dontFadeOnStart) {
        fields->m_switcherMenu->setOpacity(mm->m_opacity);
        fields->m_modeLabel->setOpacity(mm->m_opacity);
        fields->m_firstUpdate = false;
    } else {
        fields->m_switcherMenu->runAction(makeFadeAction(mm->m_opacity));
        fields->m_modeLabel->runAction(makeFadeAction(mm->m_opacity));
    }
}
