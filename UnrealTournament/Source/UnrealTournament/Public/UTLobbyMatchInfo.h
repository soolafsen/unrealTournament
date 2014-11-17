// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTLobbyGameState.h"
#include "UTLobbyPlayerState.h"
#include "OnlineSubsystemTypes.h"
#include "UTGameMode.h"
#include "UTLobbyMatchInfo.generated.h"

namespace ELobbyMatchState
{
	extern const FName Dead;						// this match is dead and watching to be cleaned up.
	extern const FName Initializing;				// this match info is being initialized with game needed game data
	extern const FName Setup;						// We are entering this map, actors are not yet ticking
	extern const FName WaitingForPlayers;			// The game is entering overtime
	extern const FName Launching;					// The game is in the process of launching the instanced server
	extern const FName Aborting;					// The game is aborting the setup of a server...
	extern const FName InProgress;					// The game is in progress, the instanced server has ack'd the lobby server and everything is good to go.
	extern const FName Returning;					// the instance server has said the game is over and players should be returning to this server
}

class FAllowedMapData
{
public:
	FString MapName;

	FAllowedMapData(FString inMapName)
		: MapName(inMapName)
	{
	};

	static TSharedRef<FAllowedMapData> Make(FString inMapName)
	{
		return MakeShareable( new FAllowedMapData( inMapName) );
	}
};

class AUTGameMode;

class FAllowedGameModeData
{
public:
	FString ClassName;
	FString DisplayName;
	TWeakObjectPtr<AUTGameMode> DefaultObject;

	FAllowedGameModeData(FString inClassName, FString inDisplayName, TWeakObjectPtr<AUTGameMode> inDefaultObject)
		: ClassName(inClassName)
		, DisplayName(inDisplayName)
		, DefaultObject(inDefaultObject)
	{
	};

	static TSharedRef<FAllowedGameModeData> Make(FString inClassName, FString inDisplayName, TWeakObjectPtr<AUTGameMode> inDefaultObject)
	{
		return MakeShareable(new FAllowedGameModeData(inClassName, inDisplayName, inDefaultObject));
	}
};

DECLARE_DELEGATE(FOnMatchInfoGameModeChanged);
DECLARE_DELEGATE(FOnMatchInfoMapChanged);
DECLARE_DELEGATE(FOnMatchInfoOptionsChanged);

UCLASS(notplaceable)
class UNREALTOURNAMENT_API AUTLobbyMatchInfo : public AInfo
{
	GENERATED_UCLASS_BODY()
public:
	// We use  the FUniqueNetID of the owner to be the Anchor point for this object.  This way we can reassociated the MatchInfo with the player when they reenter a server from travel.
	UPROPERTY(Replicated)
	FUniqueNetIdRepl OwnerID;

	// The current state of this match.  
	UPROPERTY(Replicated)
	FName CurrentState;

	// The PlayerState of the player that currently owns this match info
	UPROPERTY(Replicated)
	AUTLobbyPlayerState* OwnersPlayerState;

	// if true, the owner will have to accept people joining this lobby
	UPROPERTY(Replicated)
	uint32 bPrivateMatch:1;

	// if true (defaults to true) then this match can be joined as a spectator.
	UPROPERTY(Replicated)
	uint32 bSpectatable:1;

	// The name of this lobby.
	UPROPERTY(Replicated)
	FString MatchDescription;

	// The options for this match
	UPROPERTY(Replicated, replicatedUsing = OnRep_MatchGameMode)
	FString MatchGameMode;

	// The options for this match
	UPROPERTY(Replicated, replicatedUsing = OnRep_MatchOptions)
	FString MatchOptions;

 	// The options for this match
	UPROPERTY(Replicated, replicatedUsing = OnRep_MatchMap)
	FString MatchMap;

	// Number of players in this Match Lobby
	UPROPERTY(Replicated)
	int32 MaxPlayers;

	// A list of players in this lobby
	UPROPERTY(Replicated)
	TArray<AUTLobbyPlayerState*> Players;

	// Holds a list of all Game modes available to both the server and the host.  This list
	// is only replicated to the host.  Clients receive just MatchGameMode string
	TArray<TSharedPtr<FAllowedGameModeData>> HostAvailbleGameModes;

	// Holds a list of map available to this match.  This list is only replicated to the
	// host.  Clients receive just MatchMatch string.
	TArray<TSharedPtr<FAllowedMapData>> HostAvailableMaps;

	/**
	 *	Start sending the allowed list of maps to the client/host
	 **/
	virtual void StartServerToClientDataPush();
	
	// Cache some data
	virtual void PreInitializeComponents() override;

	virtual void AddPlayer(AUTLobbyPlayerState* PlayerToAdd, bool bIsOwner = false);
	virtual bool RemovePlayer(AUTLobbyPlayerState* PlayerToRemove);
	virtual FText GetActionText();

	// The GameState needs to tell this MatchInfo what settings should be made available
	virtual void SetSettings(AUTLobbyGameState* GameState);

	virtual void SetMatchDescription(const FString NewDescription);
	virtual void SetMatchGameMode(const FString NewGameMode);
	virtual void SetMatchOptions(const FString NewMatchOptions);
	virtual void SetMatchMap(const FString NewMatchMap);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerMatchDescriptionChanged(const FString& NewMatchGameMode);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerMatchGameModeChanged(const FString& NewMatchGameMode);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerMatchMapChanged(const FString& NewMatchMap);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerMatchOptionsChanged(const FString& NewMatchOptions);

	FOnMatchInfoGameModeChanged OnMatchGameModeChanged;
	FOnMatchInfoMapChanged OnMatchMapChanged;
	FOnMatchInfoOptionsChanged OnMatchOptionsChanged;

	AUTGameMode* GetGameModeDefaultObject(FString ClassName);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerManageUser(int32 CommandID, AUTLobbyPlayerState* Target);

	TArray<FUniqueNetIdRepl> BannedIDs;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerStartMatch();
	
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerAbortMatch();

protected:
	void SetLobbyMatchState(FName NewMatchState);

	// Only available on the server, this holds a cached reference to the GameState.
	AUTLobbyGameState* LobbyGameState;

	// Called when Match Options change.  This should funnel the new options string to the UI and update everyone.
	UFUNCTION()
	virtual void OnRep_MatchOptions();
	
	UFUNCTION()
	virtual void OnRep_MatchGameMode();

	UFUNCTION()
	virtual void OnRep_MatchMap();

	// This holds the bulk match data that has to be sent to the host.  Servers can contain a large number of possible
	// game modes and maps available for hosting.  So we have a system to bulk send them.
	TArray<FString> HostMatchData;

	// Send the next set of maps
	virtual void SendNextBulkBlock();

	/**
	 *	Receive a map in a given block of maps being sent to the client.
	 **/
	UFUNCTION(client, reliable)
	virtual void ClientReceiveMatchData(uint8 BulkSendCount, uint16 BulkSendID, const FString& MatchData);

	/**
	 *	Event function called from the server when it has finished sending all data.
	 **/
	UFUNCTION(client, reliable)
	virtual void ClientReceivedAllData();

	UFUNCTION(server, reliable, WithValidation)
	virtual void ServerACKBulkCompletion(uint16 BuildSendID);

	// The current bulk id that is being sent to the client
	uint16 CurrentBulkID;
	uint8 CurrentBlockCount;
	uint8 ExpectedBlockCount;

	// The current index in to the GameState's AllowedMaps array.
	int32 DataIndex;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetDefaults(const FString& NewMatchGameMode,const FString& NewMatchOptions, const FString& NewMatchMap);

};



