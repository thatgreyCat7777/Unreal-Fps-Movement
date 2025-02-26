// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
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
    UPROPERTY(EditAnywhere, Category = "Components")
    UCapsuleComponent *PlayerOverlapCollider;

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
    UPROPERTY(EditAnywhere, Category = "Movement")
    float WallRunCounterGravity = 1;
    UPROPERTY(EditAnywhere, Category = "Movement")
    float WallRunSpeed = 1000;
    UPROPERTY(EditAnywhere, Category = "Movement")
    float WallJumpForce = 300.f;
    // TODO - Implement air strafing function for this
    UPROPERTY(EditAnywhere, Category = "Movement")
    float AirStrafeAcceleration = 500.f;
    // TODO - Implement modified walk function for this
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MaxWalkSpeed = 600.f;
    // Rate at which player's velocity is interpolated to wish velocity
    UPROPERTY(EditAnywhere, Category = "Movement")
    float GroundAcceleration = 10.f;

    // Transition Speeds

    // Camera tilt transition speed when sliding
    UPROPERTY(EditAnywhere, Category = "Transitions")
    float SlideCameraTiltSpeed = 7.f;
    // Transition speed of crouching
    UPROPERTY(EditAnywhere, Category = "Transitions")
    float CrouchTransitionSpeed = 25.f;
    // Wall running camera tilt speed
    UPROPERTY(EditAnywhere, Category = "Transitions")
    float WallRunTransitionSpeed = 10.f;
    // Angle camera tilts at when wall running
    UPROPERTY(EditAnywhere, Category = "Transitions")
    float WallRunCameraTiltAngle = 10.f;

    // States to keep track of

    // True whenever player is crouching
    bool bIsCrouching = false;
    // True whenever initial slide impulse is applied to the player
    bool bAppliedSlideForce = false;
    // True when player is wallrunning
    bool bIsWallrunning = false;
    // Normal vector for wall normal
    FVector WallNormalVector;
    // Dot product between wall normal and player right vector
    float WallRunTiltDirection = 0;
    // True when player is running on wall
    bool bIsOnWall = false;
    // TODO - Check if this bool variable is needed in the implementation
    // True when player slide force is being applied
    bool bIsSliding = false;
    // Frame counter to help run code every few frames
    // Note: Overflow of this number is intentional
    uint8 FrameCounter = 0;

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
    void OnComponentHitCharacter(UPrimitiveComponent *HitComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                 FVector NormalImpulse, const FHitResult &Hit);
    UFUNCTION()
    void SmoothCameraTilt(const float &Angle, const float &TiltSpeed, const float &DeltaTime);
    UFUNCTION()
    void GradualCrouch(const float &Scale, const float &DeltaTime);
    // TODO - Implement Slide force function
    // UFUNCTION()
    // void ApplySlideForce();
    // TODO - Implement IsWall function
    UFUNCTION()
    bool IsWall(const FVector &Normal);
    UFUNCTION()
    void StartWallRun(const FVector &Normal);
    UFUNCTION()
    void WallRun(const float &DeltaTime);
    UFUNCTION()
    void StopWallRun();
    UFUNCTION()
    void WallJump();
    UFUNCTION()
    void GradualSlideForce(const float &DeltaTime);
    
};
