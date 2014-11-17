// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTATypes.generated.h"

// Const defines for Dialogs
const uint16 UTDIALOG_BUTTON_OK = 0x0001;			
const uint16 UTDIALOG_BUTTON_CANCEL = 0x0002;
const uint16 UTDIALOG_BUTTON_YES = 0x0004;
const uint16 UTDIALOG_BUTTON_NO = 0x0008;
const uint16 UTDIALOG_BUTTON_HELP = 0x0010;
const uint16 UTDIALOG_BUTTON_RECONNECT = 0x0020;
const uint16 UTDIALOG_BUTTON_EXIT = 0x0040;
const uint16 UTDIALOG_BUTTON_QUIT = 0x0080;
const uint16 UTDIALOG_BUTTON_VIEW = 0x0100;

UENUM()
namespace EGameStage
{
	enum Type
	{
		Initializing,
		PreGame, 
		GameInProgress,
		GameOver,
		MAX,
	};
}

UENUM()
namespace ETextHorzPos
{
	enum Type
	{
		Left,
		Center, 
		Right,
		MAX,
	};
}

UENUM()
namespace ETextVertPos
{
	enum Type
	{
		Top,
		Center,
		Bottom,
		MAX,
	};
}

namespace CarriedObjectState
{
	const FName Home = FName(TEXT("Home"));
	const FName Held = FName(TEXT("Held"));
	const FName Dropped = FName(TEXT("Dropped"));
}

namespace ChatDestinations
{
	// You can chat with your friends from anywhere
	const FName Friends = FName(TEXT("CHAT_Friends"));

	// These are lobby chat types
	const FName Global = FName(TEXT("CHAT_Global"));
	const FName Match = FName(TEXT("CHAT_Match"));

	// these are general game chating
	const FName Lobby = FName(TEXT("CHAT_Lobby"));
	const FName Local = FName(TEXT("CHAT_Local"));
	const FName Team = FName(TEXT("CHAT_Team"));

	const FName System = FName(TEXT("CHAT_System"));
}

// Our Dialog results delegate.  It passes in a reference to the dialog triggering it, as well as the button id 
DECLARE_DELEGATE_TwoParams(FDialogResultDelegate, TSharedPtr<SCompoundWidget>, uint16);
