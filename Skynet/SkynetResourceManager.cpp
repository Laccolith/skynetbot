#include "SkynetResourceManager.h"

#include "SkynetTaskRequirement.h"
#include "Types.h"
#include "PlayerTracker.h"
#include "UnitTracker.h"

SkynetResourceManager::SkynetResourceManager( Core & core )
	: ResourceManagerInterface( core )
{
	core.registerUpdateProcess( 3.5f, [this]() { update(); } );

	// Assume all workers start mining on the first frame
	m_mineral_rate = 4 * 0.0501;
}

void SkynetResourceManager::update()
{
	m_task_reserved_minerals.clear();
	m_task_reserved_gas.clear();
	m_task_reserved_supply.clear();

	m_task_reserved_supply.insert( m_task_reserved_supply.end(), m_task_output_supply.begin(), m_task_output_supply.end() );

	auto player = getPlayerTracker().getLocalPlayer();

	for( auto unit : getUnitTracker().getSupplyUnits( player ) )
	{
		if( unit->exists() && !unit->isCompleted() )
		{
			reserveTaskResource( unit->getTimeTillCompleted(), -unit->getType().supplyProvided(), m_task_reserved_supply );
		}
	}
}

void SkynetResourceManager::reserveTaskResource( int time, int amount, std::vector<ResourceTiming> & m_reserved_timings )
{
	auto it = std::upper_bound( m_reserved_timings.begin(), m_reserved_timings.end(), time, []( int time, auto & item ) { return item.time >= time; } );
	if( it != m_reserved_timings.end() && it->time == time )
		it->amount += amount;
	else
		m_reserved_timings.insert( it, ResourceTiming{ time, amount } );
}

void SkynetResourceManager::reserveTaskMinerals( int time, int amount )
{
	if( time <= 0 )
		m_reserved_minerals += amount;
	else
		reserveTaskResource( time, amount, m_task_reserved_minerals );
}

void SkynetResourceManager::reserveTaskGas( int time, int amount )
{
	if( time <= 0 )
		m_reserved_gas += amount;
	else
		reserveTaskResource( time, amount, m_task_reserved_gas );
}

void SkynetResourceManager::reserveTaskSupply( int time, int amount )
{
	if( time <= 0 )
		m_reserved_supply += amount;
	else
		reserveTaskResource( time, amount, m_task_reserved_supply );
}

void SkynetResourceManager::freeTaskMinerals( int amount )
{
	m_reserved_minerals -= amount;
}

void SkynetResourceManager::freeTaskGas( int amount )
{
	m_reserved_gas -= amount;
}

void SkynetResourceManager::freeTaskSupply( int amount )
{
	m_reserved_supply -= amount;
}

int SkynetResourceManager::earliestAvailability( int required_amount, double free_amount, double resource_rate, const std::vector<ResourceTiming> & m_reserved_timings ) const
{
	bool currently_available = false;
	if( free_amount >= required_amount )
	{
		free_amount -= required_amount;
		currently_available = true;
	}

	int earliest_time = 0;
	int last_time = 0;
	for( auto & time_point : m_reserved_timings )
	{
		double previous_free_amount = free_amount;

		free_amount -= time_point.amount;

		int time_passed = time_point.time - last_time;
		free_amount += time_passed * resource_rate;

		if( !currently_available )
		{
			if( free_amount >= required_amount )
			{
				free_amount -= required_amount;
				currently_available = true;

				if( resource_rate > 0 )
				{
					previous_free_amount -= required_amount;
					earliest_time = std::min( last_time + int( std::ceil( -previous_free_amount / resource_rate ) ), time_point.time );
				}
				else
				{
					earliest_time = time_point.time;
				}
			}
		}
		else
		{
			if( free_amount < 0 )
			{
				currently_available = false;
				free_amount += required_amount;
			}
		}

		last_time = time_point.time;
	}

	if( !currently_available )
	{
		if( resource_rate <= 0 )
			return max_time;

		free_amount -= required_amount;
		earliest_time = last_time + int(std::ceil( -free_amount / resource_rate ) );
	}

	return earliest_time;
}

int SkynetResourceManager::earliestMineralAvailability( int amount ) const
{
	return earliestAvailability( amount, BWAPI::Broodwar->self()->minerals() - m_reserved_minerals, m_mineral_rate, m_task_reserved_minerals );
}

int SkynetResourceManager::earliestGasAvailability( int amount ) const
{
	return earliestAvailability( amount, BWAPI::Broodwar->self()->gas() - m_reserved_gas, m_gas_rate, m_task_reserved_gas );
}

int SkynetResourceManager::earliestSupplyAvailability( int amount ) const
{
	return earliestAvailability( amount, BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed() - m_reserved_supply, 0.0, m_task_reserved_supply );
}

int SkynetResourceManager::availabilityAtTime( int time, double free_amount, double resource_rate, const std::vector<ResourceTiming> & m_reserved_timings ) const
{
	int last_time = 0;
	for( auto & time_point : m_reserved_timings )
	{
		if( time_point.time > time )
			break;

		double previous_free_amount = free_amount;

		free_amount -= time_point.amount;

		int time_passed = time_point.time - last_time;
		free_amount += time_passed * resource_rate;

		last_time = time_point.time;
	}

	if( time > last_time )
	{
		int time_passed = time - last_time;
		free_amount += time_passed * resource_rate;
	}

	return (int)free_amount;
}

int SkynetResourceManager::availableMineralsAtTime( int time ) const
{
	return availabilityAtTime( time, BWAPI::Broodwar->self()->minerals() - m_reserved_minerals, m_mineral_rate, m_task_reserved_minerals );
}

int SkynetResourceManager::availableGasAtTime( int time ) const
{
	return availabilityAtTime( time, BWAPI::Broodwar->self()->gas() - m_reserved_gas, m_gas_rate, m_task_reserved_gas );
}

int SkynetResourceManager::availableSupplyAtTime( int time ) const
{
	return availabilityAtTime( time, BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed() - m_reserved_supply, 0.0, m_task_reserved_supply );
}

int SkynetResourceManager::totalSupplyAtTime( int time ) const
{
	int supply_ammount = BWAPI::Broodwar->self()->supplyTotal();

	for( auto & time_point : m_task_reserved_supply )
	{
		if( time_point.time > time )
			break;

		if( time_point.amount < 0 )
			supply_ammount -= time_point.amount;
	}

	return supply_ammount;
}

void SkynetResourceManager::addTaskSupplyOutput( int time, int amount, bool temporary )
{
	reserveTaskResource( time, -amount, m_task_reserved_supply );

	if( !temporary )
		reserveTaskResource( time, -amount, m_task_output_supply );
}

void SkynetResourceManager::removeTaskSupplyOutput( int time, int amount )
{
	auto it = std::find_if( m_task_output_supply.begin(), m_task_output_supply.end(), [time]( ResourceTiming timing )
	{
		return time == timing.time;
	} );

	it->amount += amount;

	if( it->amount == 0 )
	{
		m_task_output_supply.erase( it );
	}
}
