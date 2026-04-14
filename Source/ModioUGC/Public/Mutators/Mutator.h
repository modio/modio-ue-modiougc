/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "MutatorUtils.h"
#include "MutatorEvents.generated.inl"
#include "Mutator.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class MODIOUGC_API UMutator : public UObject
{
	GENERATED_BODY()

public:
	
	/**
	 * Initialises the mutator
	 */
	void Initialise();

	/**
	 * Gets an asset override if one exists
	 * @param AssetTag The tag of the asset to retrieve
	 */
	template<typename T>
	T* GetAsset(FGameplayTag AssetTag) const;


	class UWorld* GetWorld() const override;

public:
	
	MUTATOR_EVENTS_START
	/**
	 * Gets called when an enemy wave has ended
	 * @param WaveIndex The number wave that was just completed
	 */
	DECLARE_MUTATOR_EVENT_BLUEPRINT(EnemyWaveEnded)

	/**
	 * Called when a player is initialised
	 * @param Controller The controller of the newly initialised player
	 */
	DECLARE_MUTATOR_EVENT_BLUEPRINT(PostPlayerInit)

	/**
	 * Called when a player pawn is spawned
	 * @param Pawn The pawn that was just spawned
	 */
	DECLARE_MUTATOR_EVENT_BLUEPRINT(PostPawnSpawned)

	/**
	 * Gets called when damage is about to be dealt
	 * @param Target The actor about to be dealt damage
	 * @param Source The actor about to deal damage
	 * @param Amount The amount of damage to be dealt. May have already been mutated
	 * @return The mutated value of the amount of damage to dealt
	 */
	DECLARE_MUTATOR_EVENT_BLUEPRINT_RETURN(ModifyDamage)

	/**
	 * Gets called when points are about to be scored
	 * @param Scorer The controller about to score points
	 * @param Amount The amount of points to score. May have already been mutated
	 * @return The mutated value of the number of points to score
	 */
	DECLARE_MUTATOR_EVENT_BLUEPRINT_RETURN(ScorePoints)
	MUTATOR_EVENTS_END

protected:

	/**
	 * Called during initialisation of a mutator
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "mod.io|Mutators", meta = (ForceAsFunction))
	void Initialised();
	virtual void Initialised_Implementation() {};

private:

	/**
	 * Set of assets this mutator will override
	 */
	UPROPERTY(EditDefaultsOnly, Category = "mod.io|Mutators|Assets")
	TMap<FGameplayTag, TSoftObjectPtr<UObject>> AssetOverrides;
	
};

template<typename T>
T* UMutator::GetAsset(FGameplayTag AssetTag) const
{
	if (!AssetOverrides.Contains(AssetTag))
	{
		return nullptr;
	}

	return static_cast<T*>(AssetOverrides[AssetTag].Get());
}
