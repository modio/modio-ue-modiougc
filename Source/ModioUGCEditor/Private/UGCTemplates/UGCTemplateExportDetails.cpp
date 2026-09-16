// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/UGCTemplateExportDetails.h"
#include "Misc/Paths.h"

UUGCTemplateExportDetails::UUGCTemplateExportDetails()
{
	Directory.Path = FPaths::ProjectDir() / "ModTemplates";
}
