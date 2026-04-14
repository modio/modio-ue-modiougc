/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */


#include "Mutators/MutatorSubsystem.h"
#include "Mutators/Mutator.h"
#include "ModioUGC.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "Misc/EngineVersionComparison.h"
#include "Templates/SubclassOf.h"
#if WITH_EDITOR
#include "Kismet2/KismetEditorUtilities.h"
#endif
#include "UObject/UObjectIterator.h"

void UMutatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//TODO: Set up system delegates for gamemode and similar actors
}

bool UMutatorSubsystem::RegisterMutator(TSubclassOf<class UMutator> Type, int32 Priority, FMutatorHandle& OutHandle)
{
	if (!Type->IsValidLowLevelFast())
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Trying to register a mutator using type [none]"));
		return false;
	}

	UMutator* NewMutator = NewObject<UMutator>(this, Type);
	if (NewMutator == nullptr)
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Unable to initialise mutator [%s]"), *Type->GetName());
		return false;
	}

	NewMutator->Initialise();

	OutHandle.Priority = Priority;
	OutHandle.Instance = NewMutator;

	MutatorRegistry.FindOrAdd(Priority).Add(NewMutator);
	MutatorRegistry.KeySort([](int32 A, int32 B)
		{
			return A > B;
		});

	return true;
}

void UMutatorSubsystem::DeregisterMutator(FMutatorHandle& Handle)
{
	if (!Handle.IsValid())
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Trying to degresister a mutator from an invalid handle"));
		return;
	}

	if (!MutatorRegistry.Contains(Handle.Priority))
	{
		return;
	}

	MutatorRegistry[Handle.Priority].Remove(Handle.Instance);
	Handle.Invalidate();

	MutatorRegistry.KeySort([](int32 A, int32 B) { return A > B; });
}

UObject* UMutatorSubsystem::GetAsset(FGameplayTag AssetTag, UObject* DefaultValue) const
{
	//TODO: might be possible to pre-compute this when adding/removing mutators
	for (auto it = MutatorRegistry.CreateConstIterator(); it; ++it)
	{
		for (auto Mutator : it->Value.Contents)
		{
			UObject* Replacement = Mutator->GetAsset<UObject>(AssetTag);
			if (Replacement != nullptr)
			{
				return Replacement;
			}
		}
	}

	return DefaultValue;
}


bool UMutatorSubsystem::GetMutatorClassList(TArray<TSubclassOf<UMutator>>& OutMutators, bool bUseCache /*= true*/)
{
	//TODO: Replace this with a more appropriate method; scan for Primary Assets etc
	if (!bUseCache || !bHasMutatorListCache)
	{
		//Generate cache
		//Gather native classes
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* Class = *ClassIt;

			// Only interested in native C++ classes
			if (!Class->IsNative())
			{
				continue;
			}

			// Ignore deprecated
			if (Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Abstract))
			{
				continue;
			}

#if WITH_EDITOR
			// Ignore skeleton classes (semi-compiled versions that only exist in-editor)
			if (FKismetEditorUtilities::IsClassABlueprintSkeleton(Class))
			{
				continue;
			}
#endif

			// Check this class is a subclass of Base
			if (!Class->IsChildOf(UMutator::StaticClass()))
			{
				continue;
			}

			// Add this class
			MutatorCache.Add(Class);
		}

		FARFilter ARFilter;
		TArray<FAssetData> AssetList;
#if UE_VERSION_NEWER_THAN(5, 1, 0)
		ARFilter.ClassPaths.Add(UMutator::StaticClass()->GetClassPathName());
#else
		ARFilter.ClassNames.Add(UMutator::StaticClass()->GetFName());
#endif
		ARFilter.bIncludeOnlyOnDiskAssets = true;
		
		FAssetRegistryModule& AssetRegistryModule =	FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		// Expand list of classes to include derived classes
		TArray<FTopLevelAssetPath> BlueprintParentClassPathRoots = MoveTemp(ARFilter.ClassPaths);
		TSet<FTopLevelAssetPath> BlueprintParentClassPaths;
		if (ARFilter.bRecursiveClasses)
		{
			AssetRegistry.GetDerivedClassNames(BlueprintParentClassPathRoots, TSet<FTopLevelAssetPath>(),
											   BlueprintParentClassPaths);
		}
		else
		{
			BlueprintParentClassPaths.Append(BlueprintParentClassPathRoots);
		}

		// Search for all blueprints and then check BlueprintParentClassPaths in the results
		ARFilter.ClassPaths.Reset(1);
		ARFilter.ClassPaths.Add(FTopLevelAssetPath(FName(TEXT("/Script/Engine")), FName(TEXT("BlueprintCore"))));
		ARFilter.bRecursiveClasses = true;

		auto FilterLambda = [&AssetList, &BlueprintParentClassPaths](const FAssetData& AssetData) {
			// Verify blueprint class
			if (BlueprintParentClassPaths.IsEmpty() ||
				UAssetRegistryHelpers::IsAssetDataBlueprintOfClassSet(AssetData, BlueprintParentClassPaths))
			{
				AssetList.Add(AssetData);
			}
			return true;
		};
		AssetRegistry.EnumerateAssets(ARFilter, FilterLambda);

		for (auto const& Asset : AssetList) 
		{
			auto GeneratedClassPathPtr = Asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));
			if (GeneratedClassPathPtr.IsSet())
			{
				const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPathPtr.GetValue());
				const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

				// Store using the path to the generated class
				FString ClassNameStr = Asset.GetObjectPathString() + TEXT("_C");
				auto ClassObj = LoadObject<UClass>(nullptr, *ClassNameStr);
				if (!ClassObj->HasAnyClassFlags(EClassFlags::CLASS_Abstract))
				{
					MutatorCache.Add(ClassObj);
				}
			}
		}

		bHasMutatorListCache = true;
	}

	

	OutMutators = MutatorCache;
	return true;
}

MUTATOR_EVENTS_START
IMPLEMENT_MUTATOR_EVENT(EnemyWaveEnded)
IMPLEMENT_MUTATOR_EVENT(PostPlayerInit)
IMPLEMENT_MUTATOR_EVENT(PostPawnSpawned)
IMPLEMENT_MUTATOR_EVENT_RETURN(ModifyDamage)
IMPLEMENT_MUTATOR_EVENT_RETURN(ScorePoints)
MUTATOR_EVENTS_END

bool UMutatorSubsystem::IsRegistered(TSubclassOf<UMutator> Type) 
{
	for (auto B : MutatorRegistry)
	{
		for (auto M : B.Value.Contents)
		{
			if (M->IsA(Type))
			{
				return true;
			}
		}
	}

	return false;
}
