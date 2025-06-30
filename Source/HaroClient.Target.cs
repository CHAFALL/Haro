// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HaroClientTarget : TargetRules
{
	public HaroClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;

		ExtraModuleNames.AddRange(new string[] { "LyraGame" });

		HaroGameTarget.ApplySharedLyraTargetSettings(this);
	}
}
