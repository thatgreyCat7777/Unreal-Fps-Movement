// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/EnumAsByte.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "Math/MathFwd.h"
#include "FPSCharacter.generated.h"

/////
/////////////
//////////////////
//////////////////////

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWallLineTrace, const FHitResult &, Hit);

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

    UPROPERTY(EditAnywhere, Category = "Effects")
    UParticleSystem *ExplosionParticle;

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

    // Multiple of z scale when crouching
    UPROPERTY(EditAnywhere, Category = "Crouching")
    float CrouchScale = .5f;
    // Scale in normal state
    UPROPERTY(EditAnywhere, Category = "Crouching")
    FVector NormalScale = {1.5, 1.5, 1.5};

    // Movement Physics

    // Normal Walkspeed
    UPROPERTY(EditAnywhere, Category = "Basic Movement")
    float WalkSpeed = 1000.f;
    // Crouched Walkspeed
    UPROPERTY(EditAnywhere, Category = "Basic Movement")
    float CrouchSpeed = 300.f;
    // Slide force impulse applied when character slides
    UPROPERTY(EditAnywhere, Category = "Slide Movement")
    float SlideForce = 1000.f;
    // Slide force applied over time
    UPROPERTY(EditAnywhere, Category = "Slide Movement")
    float GradualSlideForce = 200.f;
    // Speed at which gradual slide force interps to 0
    UPROPERTY(EditAnywhere, Category = "Slide Movement")
    float GradualSlideForceTime = 20.f;
    // Ground friction when sliding
    UPROPERTY(EditAnywhere, Category = "Slide Movement")
    float SlideFriction = .2f;
    UPROPERTY(EditAnywhere, Category = "Wallrun Movement")
    float WallRunCounterGravity = 1;
    UPROPERTY(EditAnywhere, Category = "Wallrun Movement")
    float WallRunSpeed = 1000;
    UPROPERTY(EditAnywhere, Category = "Wallrun Movement")
    float WallJumpForce = 420.f;
    UPROPERTY(EditAnywhere, Category = "Wallrun Movement")
    float WallRunAirControl = .7;
    // Collision channel in line trace for wall detection
    UPROPERTY(EditAnywhere, Category = "Wallrun Movement")
    TEnumAsByte<ECollisionChannel> WallDetectionChannel =
        TEnumAsByte<ECollisionChannel>(ECollisionChannel::ECC_Visibility);
    // Max number of jumps that player can perform in air
    UPROPERTY(EditAnywhere, Category = "Double Jump Movement")
    int AirJumpMax = 1;
    UPROPERTY(EditAnywhere, Category = "Air Strafing")
    float AirStrafeMagnitude = 1;
    // Keeps track number of air jumps player can perform
    int AirJumpCount = 1;

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
    // Perpendicular wall normal
    FVector WallPerpendicularNormalVector;
    // Dot product between wall normal and player right vector
    float WallRunTiltDirection = 0;
    // Keeps track of current wall
    UPrimitiveComponent *CurrentWall = nullptr;
    // Keeps track of velocity to add when applying gradual slide force
    float AddVelocityMag = SlideForce;
    // Minimum slide speed required to trigger slide force
    float MinSlideSpeed = WalkSpeed * .5;
    // Keeps track of current player wasd input
    FVector WalkingInput = {0, 0, 0};
    // Keeps track of air control reset time after wall jump
    FTimerHandle AirControlResetTimer;
    // Timer to keep track whether player is still wall running
    FTimerHandle WallRunTimer;

protected:
    // Wall detection script delegate
    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FWallLineTrace WallLineTraceDelegate;

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
    void StartSlide();
    UFUNCTION()
    bool GradualSlide(const float &DeltaTime);
    UFUNCTION()
    void OnComponentHitCharacter(UPrimitiveComponent *HitComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                 FVector NormalImpulse, const FHitResult &Hit);
    UFUNCTION()
    void SmoothCameraTilt(const float &Angle, const float &TiltSpeed, const float &DeltaTime);
    UFUNCTION()
    void GradualCrouch(const float &Scale, const float &DeltaTime);
    UFUNCTION()
    bool IsWall(const FVector &Normal);
    UFUNCTION()
    void StartWallRun(const FHitResult &Hit);
    UFUNCTION()
    void WallRun(const float &DeltaTime);
    UFUNCTION()
    void StopWallRun();
    UFUNCTION()
    void WallJump();
    UFUNCTION()
    void OnJumpLand(const FHitResult &Hit);
    UFUNCTION()
    void AirJump();
    UFUNCTION()
    void AirAccelerate(FVector WishVelocity);
    UFUNCTION()
    void OnLineWallTraceHit(const FHitResult &Hit);
    UFUNCTION()
    FVector VectorRotate(const FVector &vec, const double &theta, const double &phi, const double &rho);
};
