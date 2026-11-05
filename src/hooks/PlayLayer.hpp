#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <unordered_set>

struct HookPlayLayer : geode::Modify<HookPlayLayer, PlayLayer> {
	struct Fields {
		std::vector<geode::Ref<GameObject>> m_startPosObjects = {};
		int m_startPosIdx = 0;
        bool m_canSwitch = true;
		// Keys ("x,y") of start positions marked green for this level
		std::unordered_set<std::string> m_favourites = {};
	};

	void addObject(GameObject* obj);
    void createObjectsFromSetupFinished();

	void updateStartPos(int index);

	// Extra Practice
	void switchStartPos(int direction);
	void toggleFavourite();
	bool isFavourite(int idx);
	bool isAllowed(int idx);
	bool hasAnyFavourite();

	std::string levelSaveKey();
	std::string startPosKey(GameObject* obj);
	void loadFavourites();
	void saveFavourites();
};
