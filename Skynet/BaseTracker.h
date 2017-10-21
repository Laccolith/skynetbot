#pragma once

#include "CoreModule.h"

#include "Base.h"

class BaseTrackerInterface : public CoreModule
{
public:
	BaseTrackerInterface( Core & core ) : CoreModule( core, "BaseTracker" ) {}

	virtual const std::vector<Base> &getAllBases() const = 0;

	virtual Base getBase( TilePosition possition ) const = 0;
};