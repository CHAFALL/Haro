// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class HaroServerTarget : TargetRules
{
	public HaroServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;

		ExtraModuleNames.AddRange(new string[] { "LyraGame" });

		HaroGameTarget.ApplySharedLyraTargetSettings(this);

		bUseChecksInShipping = true;
	}
}
