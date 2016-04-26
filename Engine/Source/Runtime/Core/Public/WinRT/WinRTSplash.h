// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GenericPlatform/GenericPlatformSplash.h"

/**
 * WinRT splash implementation
 */
struct CORE_API FWinRTSplash : public FGenericPlatformSplash
{
	/**
	* Show the splash screen
	*/
	static void Show();
	/**
	* Hide the splash screen
	*/
	static void Hide();

	/**
	 * Sets the text displayed on the splash screen (for startup/loading progress)
	 *
	 * @param	InType		Type of text to change
	 * @param	InText		Text to display
	 */
	static void SetSplashText( const SplashTextType::Type InType, const TCHAR* InText );
};

typedef FWinRTSplash FPlatformSplash;
