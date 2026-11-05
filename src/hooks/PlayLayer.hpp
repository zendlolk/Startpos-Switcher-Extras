#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <unordered_set>

struct HookPlayLayer : geode::Modify<HookPlayLayer, PlayLayer> {
	struct Fields {
		std::vector<geode::Ref<GameObject>> m_startPosObjects = {};
		int m_startPosIdx = 0;
        bool m_canSwitch = true;

		// Save key ("x,y") for each entry in m_startPosObjects, same order
		std::vector<std::string> m_startPosKeys = {};
		// Keys of the start positions favourited in this level
		std::unordered_set<std::string> m_favourites = {};
		// When on, only favourites can be switched to and the counter shows favourites only
		bool m_favouritesMode = false;
	};

	void addObject(GameObject* obj);
    void createObjectsFromSetupFinished();

	void updateStartPos(int index);

	// Favourites
	void switchStartPos(int direction);
	void toggleFavouritesMode();
	void toggleFavourite();
	bool isFavourite(int idx);
	int favouriteCount();
	int favouriteRank(int idx);
	bool isFilteringFavourites();
	bool isAllowed(int idx);

	std::string levelSaveKey();
	std::string startPosKey(GameObject* obj);
	void loadFavourites();
	void saveFavourites();
};
