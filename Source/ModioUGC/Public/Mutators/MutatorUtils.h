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

#define MUTATOR_EVENTS_START
#define MUTATOR_EVENTS_END


/**
 * Helper macro which expands the parameter over each mutator
 */
#define MUTATOR_ITERATOR(Body) { for (auto it = MutatorRegistry.CreateConstIterator(); it; ++it) { for (auto Mutator : it->Value.Contents) { Body ; } } }

/**
 * Helper macro which expands the parameter over each mutator accumulating the results
 */
#define MUTATOR_ITERATOR_RETURN(Body, Type, Default)                    \
	{                                                                   \
		Type Result = Default;											\
		for (auto it = MutatorRegistry.CreateConstIterator(); it; ++it) \
		{                                                               \
			for (auto Mutator : it->Value.Contents)                     \
			{                                                           \
				Result = Body;                                        \
			}                                                           \
		}                                                               \
		return Result;													\
	}

/**
 * Defines a mutator event in UMutatorSubsystem
 * @param Name Name of the event
 * @param ... A list of type + name pairs used to generate parameters for the event
 */
#define DEFINE_MUTATOR(Name, ...)

/**
 * Defines a mutator event in UMutatorSubsystem that returns a value
 * @param Name Name of the event
 * @param ... A list of type + name pairs used to generate parameters for the event
 */
#define DEFINE_MUTATOR_RETURN(Name, ...)

/**
 * Implements a mutator event previously defined in UMutatorSubsystem
 * @param Name Name of the event
 */
#define IMPLEMENT_MUTATOR_EVENT(Name) \
MODIOUGC_API void UMutatorSubsystem::Name(F##Name##_Params Params) MUTATOR_ITERATOR(Mutator->Name(Params)) 

/**
 * Implements a mutator event previously defined in UMutatorSubsystem
 * @param Name Name of the event
 */
#define IMPLEMENT_MUTATOR_EVENT_RETURN(Name) \
MODIOUGC_API F##Name##_Params UMutatorSubsystem::Name(F##Name##_Params Params) MUTATOR_ITERATOR_RETURN(Mutator->Name(Result), F##Name##_Params, Params) 

/**
 * Declares a native only mutator event in UMutator
 * @param Name Name of the event
 */
#define DECLARE_MUTATOR_EVENT_NATIVE(Name) \
virtual void Name(F##Name##_Params Params) {}

/**
 * Declares a native only mutator event in UMutator
 * @param Name Name of the event
 */
#define DECLARE_MUTATOR_EVENT_NATIVE_RETURN(Name) \
virtual F##Name##_Params Name(F##Name##_Params Params) { return Params; }

/**
 * Declares a native blueprint mutator event in UMutator
 * @param Name Name of the event
 */
#define DECLARE_MUTATOR_EVENT_BLUEPRINT(Name) \
void Name(F##Name##_Params Params); \
virtual void Name##_Implementation(F##Name##_Params Params) {}

/**
 * Declares a native blueprint mutator event in UMutator
 * @param Name Name of the event
 */
#define DECLARE_MUTATOR_EVENT_BLUEPRINT_RETURN(Name) \
F##Name##_Params Name(F##Name##_Params Params); \
virtual F##Name##_Params Name##_Implementation(F##Name##_Params Params) { return Params; }
