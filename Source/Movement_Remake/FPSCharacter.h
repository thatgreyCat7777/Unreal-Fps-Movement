// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "Math/MathFwd.h"
#include "FPSCharacter.generated.h"
UCLASS()
class MOVEMENT_REMAKE_API AFPSCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    AFPSCharacter();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

private:
    // Base character components
    UPROPERTY(EditAnywhere, Category = "Components")
    UStaticMeshComponent *PlayerMesh;
    UPROPERTY(EditAnywhere, Category = "Components")
    UCameraComponent *CameraComp;
    UPROPERTY(EditAnywhere, Category = "Components")
    USpringArmComponent *SpringArm;

    // Input actions
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *WalkAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *LookAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *JumpAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction *CrouchAction;

    // Values

    // Scale when crouching
    UPROPERTY(EditAnywhere, Category = "Values")
    FVector CrouchScale = {1, 1, .5f};
    // Scale in normal state
    UPROPERTY(EditAnywhere, Category = "Values")
    FVector NormalScale = {1, 1, 1};

    // Movement Physics

    // Normal Walkspeed
    UPROPERTY(EditAnywhere, Category = "Movement")
    float WalkSpeed = 600.f;
    // Crouched Walkspeed
    UPROPERTY(EditAnywhere, Category = "Movement")
    float CrouchSpeed = 300.f;
    // Slide force impulse applied when character slides
    UPROPERTY(EditAnywhere, Category = "Movement")
    float SlideForce = 1000.f;
    // Ground friction when sliding
    UPROPERTY(EditAnywhere, Category = "Movement")
    float SlideFriction = .2f;

    // Transition Speeds

    // Camera tilt transition speed when sliding
    UPROPERTY(EditAnywhere, Category = "Transitions")
    float SlideCameraTiltSpeed = 7.f;
    // Transition speed of crouching
    UPROPERTY(EditAnywhere, Category = "Transitions")
    float CrouchTransitionSpeed = 25.f;

    // Minimum pitch (x) and yaw (y) wall angles for wall running
    FVector2D MinWallAngle;

    // States to keep track of

    // True whenever player is crouching
    bool bIsCrouching = false;
    // True whenever initial slide impulse is applied to the player
    bool bAppliedSlideForce = false;

private:
    // Function for fps camera rotations
    UFUNCTION()
    void Walk(const FInputActionInstance &Instance);
    // Function for basic fps movement
    UFUNCTION()
    void Look(const FInputActionInstance &Instance);
    UFUNCTION()
    void StartCrouch(const FInputActionInstance &Instance);
    UFUNCTION()
    void StopCrouch(const FInputActionInstance &Instance);
    UFUNCTION()
    void OnComponentHit(UPrimitiveComponent *HitComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                        FVector NormalImpulse, const FHitResult &Hit);
    UFUNCTION()
    void SlideCameraTilt(float Angle, const float &DeltaTime);
    UFUNCTION()
    void GradualCrouch(float ZScale, const float &DeltaTime);
    // TODO - Implement Slide force function
    // UFUNCTION()
    // void ApplySlideForce();
    // TODO - Implement IsWall function
    UFUNCTION()
    bool IsWall(const FVector &Normal);
};
