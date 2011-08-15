#pragma once

#include "Interface.h"

#include "Singleton.h"
#include "BorderPosition.h"

class BorderTrackerClass
{
public:
	BorderTrackerClass(){}

	const std::map<PositionType, std::set<BorderPosition>>& getBorderPositions() const { return mBorderPositions; }
	const std::set<BorderPosition>& getBorderPositions(PositionType type) { return mBorderPositions[type]; }

	void onBegin();
	void update();

	void toggleDebugInfo() { mShowDebugInfo = !mShowDebugInfo; }

private:
	void recalculateBorders();

	std::map<PositionType, std::set<BorderPosition>> mBorderPositions;

	std::set<Base> mMyBases;
	std::set<Base> mEnemyBases;

	std::set<Region> mMyRegions;
	std::set<Region> mMyForwardRegions;
	std::set<Region> mEnemyRegions;

	std::set<Chokepoint> mMyBorder;
	std::set<Chokepoint> mMyForwardBorder;
	std::set<Chokepoint> mEnemyBorder;

	bool mShowDebugInfo;

	std::set<Region> mLargeRegions;

	void drawDebugInfo(std::map<PositionType, std::set<BorderPosition>>::const_iterator it);
};

typedef Singleton<BorderTrackerClass> BorderTracker;