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
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "MutatorUtils.h"
#include "MutatorEvents.generated.inl"
#include "MutatorSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnRecieveEventDelegate, const FInstancedStruct&, Context);



/**
 * Handle to a registered mutator
 */
USTRUCT(BlueprintType)
struct MODIOUGC_API FMutatorHandle
{
	GENERATED_BODY()

public:

	/**
	 * Checks if the handle is valid
	 * @return Whether the handle is valid
	 */
	bool IsValid() const 
	{
		return Instance != nullptr;
	}

	/**
	 * Invalidates the handle making it no longer usable
	 */
	void Invalidate()
	{
		Instance = nullptr;
	}

	bool operator==(const FMutatorHandle& Other) const
	{
		return Instance == Other.Instance && Priority == Other.Priority;
	}

	bool operator!=(const FMutatorHandle& Other) const
	{
		return Instance != Other.Instance && Priority != Other.Priority;
	}

private:
	
	int32 Priority = -1;
	class UMutator* Instance = nullptr;

	friend class UMutatorSubsystem;
};

/**
* Intermediate struct used to bypass not being able to have TArray of mutators as the registry key.
*/
USTRUCT()
struct FMutatorPriorityBucket
{
	GENERATED_BODY()

public:
	
	int32 Add(class UMutator* Mutator)
	{
		return Contents.Add(Mutator);
	}

	int32 Remove(class UMutator* Mutator)
	{
		return Contents.Remove(Mutator);
	}

private:
	
	UPROPERTY()
	TArray<class UMutator*> Contents;

	friend class UMutatorSubsystem;
};

/**
 * 
 */
UCLASS(BlueprintType)
class MODIOUGC_API UMutatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Creates an instance of a mutator and adds it to the list of active mutators
	 * @param Type The class of mutator you want to register
	 * @param Priority Defines the order in which mutators are applied. Higher priority = more influence
	 * @param OutHandle A handle to the create mutator
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mutators")
	bool RegisterMutator(TSubclassOf<class UMutator> Type, int32 Priority, FMutatorHandle& OutHandle);

	/**
	 * Creates an instance of a mutator and adds it to the list of active mutators
	 * @param Type The class of mutator you want to register
	 * @param Priority Defines the order in which mutators are applied. Higher priority = more influence
	 * @param OutHandle A handle to the create mutator
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mutators")
	void DeregisterMutator(UPARAM(ref) FMutatorHandle& Handle);

	/**
	 * Gets an asset override if one exists
	 * @param AssetTag The tag of the asset to retrieve
	 * @param DefaultValue The asset to use if no mutator is overriding it
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mutators|Asset")
	UObject* GetAsset(FGameplayTag AssetTag, UObject* DefaultValue = nullptr) const;

	/**
	 * Get a list of all mutator classes
	 * @param OutMutators the mutator class list
	 * @param bUseCache whether or not we should return the cached result or rerun the query
	 * @return Was the query successful
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mutators")
	bool GetMutatorClassList(TArray<TSubclassOf<UMutator>>& OutMutators, bool bUseCache = true);

public:
	
	
	
	MUTATOR_EVENTS_START
	/**
	 * Trigger EnemyWaveEnded for registered mutators
	 * @param WaveNumber Which number wave just ended
	 */
	DEFINE_MUTATOR(EnemyWaveEnded, int32, WaveNumber)
	DEFINE_MUTATOR(PostPlayerInit, class AController*, Controller)
	DEFINE_MUTATOR(PostPawnSpawned, class APawn*, Pawn)

	/**
	 * Trigger ModifyDamage for registered mutators
	 * @param Target The Actor about to be damaged
	 * @param Source The Actor responsible for dealing the damage
	 * @param Amount The amount of damage about to be dealt
	 * @return The amount of points after being modified by mutators
	 */
	DEFINE_MUTATOR_RETURN(ModifyDamage, class AActor*, Target, class AActor*, Source, float, Amount)

	/**
	 * Trigger ScorePoints for registered mutators
	 * @param Scorer The controller that is scoring points
	 * @param Amount The amount of points about to be scored
	 * @return The amount of points after being modified by mutators
	 */
	DEFINE_MUTATOR_RETURN(ScorePoints, class AController*, Scorer, int32, Amount)
	MUTATOR_EVENTS_END

public:

	/**
	 * Checks to see if a mutator of a given type is registereed or not
	 * @param Type the type of mutator to check for
	 */
	UFUNCTION(BlueprintPure, Category = "mod.io|Mutators")
	bool IsRegistered(TSubclassOf<UMutator> Type);

protected:

	UPROPERTY()
	TMap<int32, FMutatorPriorityBucket> MutatorRegistry;

private:

	UPROPERTY()
	TArray<TSubclassOf<UMutator>> MutatorCache;

	bool bHasMutatorListCache = false;
};