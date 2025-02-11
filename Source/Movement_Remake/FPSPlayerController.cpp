// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Math/Color.h"

void AFPSPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
            GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
    {
        Subsystem->AddMappingContext(InputMapping, 1);
        GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Green, TEXT("Subsystem found"));
        return;
    }
    GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, TEXT("Subsystem not found"));
}