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
#include "InstancedStruct.h"
#include "MutatorUtils.h"

class AController;

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
	class UUGCMutator* Instance = nullptr;

	friend class UUGCMutatorSubsystem;
};

/**
* Intermediate struct used to bypass not being able to have TArray of mutators as the registry key.
*/
USTRUCT()
struct FMutatorPriorityBucket
{
	GENERATED_BODY()

public:
	
	int32 Add(class UUGCMutator* Mutator)
	{
		return Contents.Add(Mutator);
	}

	int32 Remove(class UUGCMutator* Mutator)
	{
		return Contents.Remove(Mutator);
	}

//private:
	
	UPROPERTY()
	TArray<class UUGCMutator*> Contents;

	friend class UUGCMutatorSubsystem;
};

/**
 * 
 */
UCLASS(BlueprintType)
class MODIOUGC_API UUGCMutatorSubsystem : public UGameInstanceSubsystem
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
	bool RegisterMutator(TSubclassOf<class UUGCMutator> Type, int32 Priority, FMutatorHandle& OutHandle);

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
	bool GetMutatorClassList(TArray<TSubclassOf<UUGCMutator>>& OutMutators, bool bUseCache = true);

public:
	
	
	
	MUTATOR_EVENTS_START
	DEFINE_MUTATOR(PostPlayerInit, class AController*, Controller)
	DEFINE_MUTATOR(PostPawnSpawned, class APawn*, Pawn)
	MUTATOR_EVENTS_END

public:

	/**
	 * Checks to see if a mutator of a given type is registereed or not
	 * @param Type the type of mutator to check for
	 */
	UFUNCTION(BlueprintPure, Category = "mod.io|Mutators")
	bool IsRegistered(TSubclassOf<UUGCMutator> Type);

protected:

	UPROPERTY()
	TMap<int32, FMutatorPriorityBucket> MutatorRegistry;

private:

	UPROPERTY()
	TArray<TSubclassOf<UUGCMutator>> MutatorCache;

	bool bHasMutatorListCache = false;
};