#include "AIG_ShipBuildHelper.h"
#include "MapManager.h"

AIG_ShipBuildHelper::AIG_ShipBuildHelper(MapManager* Mapm)
	:m_MapManager(Mapm)
{

}

//Called every second unless a new turn
void AIG_ShipBuildHelper::UpdateSurroundingInfo()
{
	//Functions to update the values
	CalculateThreat();
	CalculateLocalResources();
	AssignThreatLevels();
	AssignResourceLevels();

}

//Called by the UI to determine what ship to build
ShipTypes AIG_ShipBuildHelper::CalculateShipToBuild()
{
	//The weights for the final weight value 
	float NoThreatWeight = 0.0f;
	float LowThreatWeight = 0.1f;
	float ModerateThreatWeight = 0.4f;
	float HighThreatWeight = 0.8f;
	float FewResourcesWeight = 0.2f;
	float SomeResourcesWeight = 0.1f;
	float AbundantResourcesWeight = 0.0f;

	//adding up all the weights for a final weightd t0tal
	float finalWeightedValue = (m_NoThreat * NoThreatWeight) + (m_LowThreat * LowThreatWeight) + (m_ModerateThreat * ModerateThreatWeight) + (m_HighThreat * HighThreatWeight) + (m_FewResources * FewResourcesWeight) + (m_SomeResources * SomeResourcesWeight) + (m_AbundantResources * AbundantResourcesWeight);


	ShipTypes typeToBuild;

	//simplified MAX function finding where the crossover point would have been and just placing an if to check instead
	if (finalWeightedValue < 0.4f)
	{
		typeToBuild = ShipTypes::Cargo; // AKA Harvester
	}
	else if (finalWeightedValue < 0.6f)
	{
		typeToBuild = ShipTypes::Scout;
	}
	else 
	{
		typeToBuild = ShipTypes::Combat;
	}

	return ShipTypes();
}



void AIG_ShipBuildHelper::CalculateThreat()
{
	//gets he total amoun o ships
	int totalships = m_MapManager->GetTotalShips();
	//reseting the playr strength
	float playerStrength = 0.0f;
	float enemyStrength = 0.0f;
	//gets the stations position
	v3 stationPos = m_MapManager->GetCurrentTeamStation()->GetPosition();
	//loops through all the ships
	for (int i = 0; i < totalships; i++)
	{
		//gets the Team of the current Player
		int shipTeamID = m_MapManager->GetShipFromIndex(i)->GetTeam();
		//Checks if the ship is on the current players team
		if (shipTeamID == m_TeamId)
		{
			//different ship types do different damage so i get there attack individually
			switch (m_MapManager->GetShipFromIndex(i)->GetType())
			{
			case ShipTypes::Scout:
				playerStrength += m_MapManager->GetShipFromIndex(i)->GetAttackDamage();
				break;
			case ShipTypes::Combat:
				playerStrength += m_MapManager->GetShipFromIndex(i)->GetAttackDamage();
				break;
			case ShipTypes::Cargo:
				playerStrength += m_MapManager->GetShipFromIndex(i)->GetAttackDamage();
				break;
			default:
				break;
			}
		}
		else 
		{
			//gets the enemy ships position
			v3 enemyPos = m_MapManager->GetShipFromIndex(i)->GetPosition();

			// finds the distance using tiles difference instead of pythagoras because ships can only move on a grid and cant go diagonal
			float xDif = stationPos.getX() - enemyPos.getX();
			float zDif = stationPos.getZ() - enemyPos.getZ();
			if (xDif < 0)
			{
				xDif *= -1;
			}
			if (zDif < 0)
			{
				zDif *= -1;
			}

			int distance = xDif + zDif;
			int tempDist = distance;
			//loops through trying to calculate how many turns it will take for the enemy ships to reach the station
			int turnsTilAttack = 0;
			if (m_MapManager->GetShipFromIndex(i)->GetMaxTurnRange() > 0)
			{
				while (tempDist < m_MapManager->GetShipFromIndex(i)->GetAttackRange())
				{
				turnsTilAttack++;
				tempDist -= m_MapManager->GetShipFromIndex(i)->GetMaxTurnRange();
				}

				//adding to the enemyStrength by multiplying the ships attack strength but how far away the ship is 
				enemyStrength += m_MapManager->GetShipFromIndex(i)->GetAttackDamage() * (1.0f / (float)turnsTilAttack);
			}

			
		}

	}

	//checks if both the players have ships
	if ((enemyStrength > 0) && (playerStrength > 0))
	{
		//reduces the players strength to account for the spread out ships
		playerStrength *= 0.4f;
		m_HostilityValue = ((enemyStrength + 0.01f) / (enemyStrength + playerStrength + 0.01f));
	}
	else if (enemyStrength > 0)
	{
		//The player has no ships
		m_HostilityValue = 1.0f;

	}else
	{
		//either both players have no ships or only the current player has a ship
		m_HostilityValue = 0.0f;
	}
}

void AIG_ShipBuildHelper::AssignThreatLevels()
{
	//uses the previous function to calculate the threat level at differing parts of the aggressiveness spectum
	float tempHostval = 0.0f;
	if (m_HostilityValue <= 0.1f)
	{
		m_NoThreat = 1.0f;
		m_LowThreat = 0.0f;
		m_ModerateThreat = 0.0f;
		m_HighThreat = 0.0f;
	}
	else if (m_HostilityValue <= 0.4f)
	{
		tempHostval = m_HostilityValue - 0.1f;

		m_NoThreat = 1.0f - tempHostval;
		m_LowThreat = tempHostval;
		m_ModerateThreat = 0.0f;
		m_HighThreat = 0.0f;
	}
	else if (m_HostilityValue <= 0.7f)
	{
		tempHostval = m_HostilityValue - 0.4f;

		m_NoThreat = 0.0f;
		m_LowThreat = 1.0f - tempHostval;
		m_ModerateThreat = tempHostval;
		m_HighThreat = 0.0f;
	}
	else if (m_HostilityValue <= 1.0f)
	{
		tempHostval = m_HostilityValue - 0.4f;

		m_NoThreat = 0.0f;
		m_LowThreat = 0.0f;
		m_ModerateThreat = 1.0f - tempHostval;
		m_HighThreat = tempHostval;
	}

}

void AIG_ShipBuildHelper::CalculateLocalResources()
{
	
	//clears the current vector with resource coordinates
	m_ResourceMemory.clear();

	//go through all the deposits
	for (int i = 0; i < m_MapManager->GetDepositAmount(); i++)
	{
		//gets the resource positions and adds it to the memory
		v3 location = m_MapManager->GetDepositLocation(i);
		float xPos = location.getX();
		float zPos = location.getZ();
		m_ResourceMemory.push_back( Vector2(xPos, zPos));
	}

	//check the planets as well
	for (int i = 0; i < 4; i++)
	{
		//gets the resource position and adds it to the memory
		v3 location = m_MapManager->GetMediumPlanets(i);
		float xPos = location.getX();
		float zPos = location.getZ();
		//medium planets take up a 2x2 space
		m_ResourceMemory.push_back(Vector2(xPos + 0.5f, zPos + 0.5f));
		m_ResourceMemory.push_back(Vector2(xPos + 0.5f, zPos - 0.5f));
		m_ResourceMemory.push_back(Vector2(xPos - 0.5f, zPos + 0.5f));
		m_ResourceMemory.push_back(Vector2(xPos - 0.5f, zPos - 0.5f));
	}
	
	for (int i = 0; i < 2; i++)
	{
		//gets the resource position and adds it to the memory
		v3 location = m_MapManager->GetLargePlanets(i);
		float xPos = location.getX();
		float zPos = location.getZ();
		//large planets take up a 3x3 space
		for (int x = -1; x < 2; x++)
		{
			for (int z = -1; z < 2; z++)
			{
				m_ResourceMemory.push_back(Vector2(xPos + x, zPos + z));
			}
		}
	}

	//gets the total ships and station position again
	int totalships = m_MapManager->GetTotalShips();
	v3 stationPos = m_MapManager->GetCurrentTeamStation()->GetPosition();

	//resets the resource abundance value
	m_ResourceAbundanceValue = 0.0f;

	//loops through all the resources to check if they are currently occupied by a Cargo ship aka harvestor
	for (Vector2 s : m_ResourceMemory)
	{
		bool occupied = false;
		for (int i = 0; i < totalships; i++)
		{
			//check the ship type
			if (m_MapManager->GetShipFromIndex(i)->GetType() == ShipTypes::Cargo)
			{
				//egts the postions an then checks if they match
				v3 pos = m_MapManager->GetShipFromIndex(i)->GetPosition();
				Vector2 newPos = Vector2(pos.getX(), pos.getZ());
				if ((s.getX() == pos.getX()) && (s.getY() == pos.getZ()))
				{
					// if they do match then they exits out of the for loop
					i = totalships; 
					occupied = true;
				}

			}


		}

		//if the tile is free it add it to potential tiles to harvstor from
		if (occupied != true)
		{
			//gets how far away the resource is to the current players station
			float xDif = stationPos.getX() - s.getX();
			float zDif = stationPos.getZ() - s.getY();
			if (xDif < 0)
			{
				xDif *= -1;
			}
			if (zDif < 0)
			{
				zDif *= -1;
			}

			float distance = xDif + zDif;

			//sets the value as the invrse of the distance to the players station
			m_ResourceAbundanceValue += (1 / distance);
 		}
	}
	

}

void AIG_ShipBuildHelper::AssignResourceLevels()
{
	//Normalised value of m_AbundantResources
	float normVal = (m_AbundantResources / 20.0f);

	if (normVal < 5)
	{
		//uses the sigmoid function to calculate the new value
		float newVal = (1.0 / 1.0 + pow(2.71828, (-1.0 * ((normVal * 5.0f) - 5.0f))));
		
		//assigns the value to the resources
		m_FewResources = 1 - newVal;
		m_SomeResources = newVal;
		m_AbundantResources = 0.0f;

	}
	else
	{
		//uses the sigmoid function to calculate the new value
		//use -0.5f to reduce the normalised value between 0 and 0.5
		float newVal = (1.0 / 1.0 + pow(2.71828, (-1.0 * (((normVal - 0.5f) * 5.0f) - 5.0f))));
		//assigns the value to the resources
		m_FewResources = 0.0f;
		m_SomeResources = 1 - newVal;
		m_AbundantResources = newVal;
	}

	
}
