// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/UGCTemplateDescriptor.h"
#include "JsonObjectConverter.h"


const FString UUGCTemplateDescriptor::VersionFieldName = "Version";
const FString UUGCTemplateDescriptor::TypeFieldName = "Type";
const FString UUGCTemplateDescriptor::ParamArrayFieldName = "Parameters";
const FString UUGCTemplateDescriptor::ChildTemplatesFieldName = "ChildTemplates";
const FString UUGCTemplateDescriptor::ChildObjectsFieldName = "ChildObjects";


bool UUGCTemplateDescriptor::Load(TArray<uint8> Data, UUGCTemplateDescriptor* Descriptor)
{
	FString JsonString;
	FFileHelper::BufferToString(JsonString, Data.GetData(), Data.Num());

	TSharedPtr<FJsonObject> JsonObject;
	auto JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		return false;
	}

	double DescriptorVersion;
	bool bFoundVersion = JsonObject->TryGetNumberField(VersionFieldName, DescriptorVersion);
	if (!bFoundVersion)
	{
		DescriptorVersion = UUGCTemplateDescriptor::CurrentVersion;
	}
	Descriptor->Version = static_cast<int32>(DescriptorVersion);

	double TypeValue = 0.0f;
	bool bFoundType = JsonObject->TryGetNumberField(TypeFieldName, TypeValue);
	if (bFoundType)
	{
		Descriptor->Type = static_cast<EUGCTemplateType>(TypeValue);
	}
	
	const TArray<TSharedPtr<FJsonValue>>* ParamList;
	bool bFoundParams = JsonObject->TryGetArrayField(ParamArrayFieldName, ParamList);
	if (bFoundParams)
	{
		for (auto Param : *ParamList)
		{
			Descriptor->SubstitutionParameters.Add(Param->AsString());
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ChildTemplateList;
	bool bFoundChildTemplates = JsonObject->TryGetArrayField(ChildTemplatesFieldName, ChildTemplateList);
	if (bFoundChildTemplates)
	{
		for (auto Child : *ChildTemplateList)
		{
			FChildTemplateEntry ChildTemplate;
			if (FJsonObjectConverter::JsonObjectToUStruct<FChildTemplateEntry>(Child->AsObject().ToSharedRef(), &ChildTemplate))
			{
				Descriptor->ChildTemplates.Add(ChildTemplate);
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ChildObjectList;
	bool bFoundChildrObjects = JsonObject->TryGetArrayField(ChildObjectsFieldName, ChildObjectList);
	if (bFoundChildrObjects)
	{
		for (auto Child : *ChildObjectList)
		{
			FUGCChildObject ChildObject;
			if (FJsonObjectConverter::JsonObjectToUStruct<FUGCChildObject>(Child->AsObject().ToSharedRef(),
																		   &ChildObject))
			{
				Descriptor->ChildObjects.Add(ChildObject);
			}
		}
	}

	return true;
}

bool UUGCTemplateDescriptor::Save(const UUGCTemplateDescriptor* Descriptor, TArray<uint8>& Data) 
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	JsonObject->SetNumberField(VersionFieldName, Descriptor->Version);

	JsonObject->SetNumberField(TypeFieldName, (double)Descriptor->Type);

	TArray<TSharedPtr<FJsonValue>> ParamsArray;
	for (auto Param : Descriptor->SubstitutionParameters)
	{
		ParamsArray.Add(MakeShareable<FJsonValueString>(new FJsonValueString(Param)));
	}
	JsonObject->SetArrayField(ParamArrayFieldName, ParamsArray);

	TArray<TSharedPtr<FJsonValue>> ChildTemplates;
	for (auto Child : Descriptor->ChildTemplates)
	{
		auto ChildJsonObj = FJsonObjectConverter::UStructToJsonObject<FChildTemplateEntry>(Child);
		if (ChildJsonObj == nullptr)
		{
			continue;
		}
		ChildTemplates.Add(MakeShareable<FJsonValueObject>(new FJsonValueObject(ChildJsonObj)));
	}
	JsonObject->SetArrayField(ChildTemplatesFieldName, ChildTemplates);

	TArray<TSharedPtr<FJsonValue>> ChildObjects;
	for (auto Child : Descriptor->ChildObjects)
	{
		auto ChildJsonObj = FJsonObjectConverter::UStructToJsonObject<FUGCChildObject>(Child);
		if (ChildJsonObj == nullptr)
		{
			continue;
		}
		ChildObjects.Add(MakeShareable<FJsonValueObject>(new FJsonValueObject(ChildJsonObj)));
	}
	JsonObject->SetArrayField(ChildObjectsFieldName, ChildObjects);


	FString JsonText;
	{
		auto Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonText);
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			return false;
		}
	}

	auto JsonToBufferConverter = TStringConversion<FTCHARToUTF8_Convert>(*JsonText);
	Data.AddUninitialized(JsonToBufferConverter.Length());
	FMemory::Memcpy(Data.GetData(), JsonToBufferConverter.Get(), JsonToBufferConverter.Length());
	
	return true;
}


