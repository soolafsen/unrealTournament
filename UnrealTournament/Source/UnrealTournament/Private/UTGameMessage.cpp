// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTGameMessage.h"
#include "GameFramework/LocalMessage.h"


UUTGameMessage::UUTGameMessage(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	MessageArea = FName(TEXT("GameMessages"));

	GameBeginsMessage = NSLOCTEXT("UTGameMessage","GameBeginsMessage","BEGIN...");
	OvertimeMessage = NSLOCTEXT("UTGameMessage","OvertimeMessage","!!!! OVERTIME !!!!");
	SuddenDeathMessage = NSLOCTEXT("UTGameMessage", "SuddenDeathMessage", "!!!! SUDDEN DEATH !!!!");
	CantBeSpectator = NSLOCTEXT("UTGameMessage", "CantBeSpectator", "You can not become a spectator!");
	CantBePlayer = NSLOCTEXT("UTGameMessage","CantBePlayer","Sorry, you can not become a player!");
	SwitchLevelMessage = NSLOCTEXT("UTGameMessage","SwitchLevelMessage","Loading....");
	NoNameChange = NSLOCTEXT("UTGameMessage","NoNameChange","You can not change your name.");
	BecameSpectator = NSLOCTEXT("UTGameMessage","BecameSpectator","You are now a spectator.");
	DidntMakeTheCut= NSLOCTEXT("UTGameMessage","DidntMakeTheCut","!! You didn't make the cut !!");
}

FText UUTGameMessage::GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false,class APlayerState* RelatedPlayerState_1 = NULL,class APlayerState* RelatedPlayerState_2 = NULL,class UObject* OptionalObject = NULL) const
{
	switch (Switch)
	{
		case 0:
			return GetDefault<UUTGameMessage>(GetClass())->GameBeginsMessage;
			break;
		case 1:
			return GetDefault<UUTGameMessage>(GetClass())->OvertimeMessage;
			break;
		case 2:
			return GetDefault<UUTGameMessage>(GetClass())->CantBeSpectator;
			break;
		case 3:
			return GetDefault<UUTGameMessage>(GetClass())->CantBePlayer;
			break;
		case 4:
			return GetDefault<UUTGameMessage>(GetClass())->SwitchLevelMessage;
			break;
		case 5:
			return GetDefault<UUTGameMessage>(GetClass())->NoNameChange;
			break;
		case 6:
			return GetDefault<UUTGameMessage>(GetClass())->BecameSpectator;
			break;
		case 7:
			return GetDefault<UUTGameMessage>(GetClass())->SuddenDeathMessage;
			break;
		case 8:
			return GetDefault<UUTGameMessage>(GetClass())->DidntMakeTheCut;
			break;
		default:
			return FText::GetEmpty();
	}
}

FName UUTGameMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	switch (Switch)
	{
		case 1: return TEXT("SuddenDeath"); break;
		case 7: return TEXT("SuddenDeath"); break;
	}
	return NAME_None;
}
