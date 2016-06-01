// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTShowdownGame.h"

#include "UTTeamShowdownGame.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTTeamShowdownGame : public AUTShowdownGame
{
	GENERATED_BODY()
public:
	AUTTeamShowdownGame(const FObjectInitializer& OI);

	virtual bool ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast) override
	{
		return AUTTeamDMGameMode::ChangeTeam(Player, NewTeam, bBroadcast);
	}
	virtual bool ShouldBalanceTeams(bool bInitialTeam) const override
	{
		return AUTTeamDMGameMode::ShouldBalanceTeams(bInitialTeam);
	}

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void RestartPlayer(AController* aPlayer) override;
	virtual void ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType) override;
	virtual AInfo* GetTiebreakWinner(FName* WinReason = NULL) const override;
	virtual void ScoreExpiredRoundTime() override;
	virtual void PlayEndOfMatchMessage() override;
	virtual bool CheckRelevance_Implementation(AActor* Other) override;
	virtual void DiscardInventory(APawn* Other, AController* Killer) override;

	virtual bool CanSpectate_Implementation(APlayerController* Viewer, APlayerState* ViewTarget) override
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(ViewTarget);
		return Super::CanSpectate_Implementation(Viewer, ViewTarget) && (PS == NULL || PS->GetUTCharacter() != NULL);
	}

	// TODO: move this up a level if we're going to have it in multiple gametypes
	TAssetSubclassOf<class AUTInventory> ActivatedPowerupPlaceholderObject;
	UPROPERTY()
	TSubclassOf<class AUTInventory> ActivatedPowerupPlaceholderClass;
	virtual TSubclassOf<class AUTInventory> GetActivatedPowerupPlaceholderClass() { return ActivatedPowerupPlaceholderClass; };

	virtual void GetGameURLOptions(const TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, TArray<FString>& OptionsList, int32& DesiredPlayerCount) override;
	virtual void CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps) override;
#if !UE_SERVER
	virtual void CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers) override;
#endif
};