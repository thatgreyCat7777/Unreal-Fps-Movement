// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/Platform.h"
#include "InputTriggers.h"
#include "Math/Color.h"
#include "Math/MathFwd.h"
#include "Math/UnrealMathUtility.h"
#include "Templates/Casts.h"
#include "Delegates/Delegate.h"

// Sets default values
AFPSCharacter::AFPSCharacter()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need
    // it.
    PrimaryActorTick.bCanEverTick = true;

    // Get default capsule collider
    UCapsuleComponent *Collider = GetCapsuleComponent();

    // Setup Static mesh
    PlayerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
    PlayerMesh->SetupAttachment(Collider);

    // Setup spring arm
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(Collider);

    // Setup camera
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComp->SetupAttachment(SpringArm);
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
    Super::BeginPlay();
    // Sets character's max walkspeed to default set in the class
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    // Links oncomponenthit function
    GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AFPSCharacter::OnComponentHitCharacter);
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bIsCrouching)
    {
        // Makes smoothly camera tilt when sliding
        if (!bIsWallrunning || !bIsOnWall)
        {
            SmoothCameraTilt(-3.f, SlideCameraTiltSpeed, DeltaTime);
        }
        // Gradually changes scale of player to crouch scale
        GradualCrouch(CrouchScale.Z, DeltaTime);
        if (GetCharacterMovement()->IsMovingOnGround() && GetCharacterMovement()->IsJumpAllowed())
        {
            // Make sliding down slope faster
            GetCharacterMovement()->Velocity +=
                FVector::VectorPlaneProject(FVector::DownVector,
                                            GetCharacterMovement()->CurrentFloor.HitResult.Normal) *
                DeltaTime * 10000.f;
        }
    }
    else
    {
        // Makes smoothly camera tilt when sliding
        if (!bIsOnWall || !bIsWallrunning)
        {
            SmoothCameraTilt(0.f, SlideCameraTiltSpeed, DeltaTime);
        }
        // Gradually changes scale of player to normal scale
        GradualCrouch(NormalScale.Z, DeltaTime);
    }
    if (bIsWallrunning && bIsOnWall)
    {
        if (FrameCounter % 20 == 0)
        {
            bIsOnWall = false;
        }
        WallRun(DeltaTime);
        SmoothCameraTilt(WallRunTiltDirection * WallRunCameraTiltAngle, WallRunTransitionSpeed, DeltaTime);
    }
    FrameCounter++;
}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (UEnhancedInputComponent *EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Binds walk function to walk action
        EnhancedInput->BindAction(WalkAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Walk);
        // Binds look function to look action
        EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);
        // Binds jump function to built in jump function
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AFPSCharacter::WallJump);
        // Binds bIsCrouching to startcrouch and stopcrouch function
        EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &AFPSCharacter::StartCrouch);
        EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AFPSCharacter::StopCrouch);
        // Screen Text for debugging
        GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Green, TEXT("Input Actions Binded"));
    }
}
// Function for walking functionality
void AFPSCharacter::Walk(const FInputActionInstance &Instance)
{
    // Gets value of input
    FVector2D Input = Instance.GetValue().Get<FVector2D>();
    // Adds input corresponding to character's forward and right vector
    AddMovementInput(GetActorForwardVector(), Input.Y);
    AddMovementInput(GetActorRightVector(), Input.X);
    // GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Green,
    //                                  FString::Printf(TEXT("Velocity = %d, Floor normal = %d"),
    //                                                  GetCharacterMovement()->Velocity.Size2D(),
    //                                                  GetCharacterMovement()->CurrentFloor.HitResult.Normal.Z));
}
// Function for player camera rotation
void AFPSCharacter::Look(const FInputActionInstance &Instance)
{
    FVector2D Input = Instance.GetValue().Get<FVector2D>();
    AddControllerPitchInput(Input.Y);
    AddControllerYawInput(Input.X);
    // GEngine->AddOnScreenDebugMessage(0, 3.0f, FColor::Blue, TEXT("Look"));
}
// Starts crouching
void AFPSCharacter::StartCrouch(const FInputActionInstance &Instance)
{
    // SetActorScale3D(CrouchScale);
    // FVector NewLocation = GetActorLocation();
    // NewLocation.Z -= NormalScale.Z - CrouchScale.Z;
    // SetActorLocation(NewLocation);

    bIsCrouching = true;

    // Adds message containing character velocity
    // GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Green,
    //                                  FString::Printf(TEXT("Velocity = %d"),
    //                                  GetCharacterMovement()->Velocity.Size2D()));
    // Sets ground friction to sliding friction
    GetCharacterMovement()->GroundFriction = SlideFriction;
    GetCharacterMovement()->BrakingFrictionFactor = 0.1f;
    // Sets walkspeed to bIsCrouching walkspeed
    GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;

    // Checks if current velocity is fast enough for sliding
    if (GetCharacterMovement()->Velocity.Size() > 100.f)
    {
        // Checks if character is on ground
        if (GetCharacterMovement()->IsMovingOnGround())
        {
            // Adds impulse force to character
            GetCharacterMovement()->Velocity += GetCharacterMovement()->Velocity.GetSafeNormal2D() * SlideForce;
            // Log message for debugging
            // GEngine->AddOnScreenDebugMessage(0, 5, FColor::Green, TEXT("Force added"));
            bAppliedSlideForce = true;
        }
    }
}
// Stops Crouching
void AFPSCharacter::StopCrouch(const FInputActionInstance &Instance)
{
    // SetActorScale3D(NormalScale);
    // FVector NewLocation = GetActorLocation();
    // NewLocation.Z += NormalScale.Z - CrouchScale.Z;
    // SetActorLocation(NewLocation);

    bIsCrouching = false;

    // Reset to default walkspeed and friction
    GetCharacterMovement()->GroundFriction = 8.0f;
    GetCharacterMovement()->BrakingFrictionFactor = 2.0f;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    if (GetCharacterMovement()->IsMovingOnGround())
    {
        bAppliedSlideForce = false;
    }
}
void AFPSCharacter::OnComponentHitCharacter(UPrimitiveComponent *HitComp, AActor *OtherActor,
                                            UPrimitiveComponent *OtherComp, FVector NormalImpulse,
                                            const FHitResult &Hit)
{
    // GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Cyan, TEXT("CompHit"));
    if (GetCharacterMovement()->IsMovingOnGround())
    {
        if (bIsCrouching)
        {
            if (GetCharacterMovement()->Velocity.Size() > 100.f && !bAppliedSlideForce)
            {
                // Adds impulse force to character
                GetCharacterMovement()->Velocity += GetCharacterMovement()->Velocity.GetSafeNormal2D() * SlideForce;
                bAppliedSlideForce = true;
                // GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Cyan, TEXT("JumpSlide"));
            }
        }
        // Sets bAppliedSlideForce to false when player hits ground and is not crouching
        else
        {
            bAppliedSlideForce = false;
            // GEngine->AddOnScreenDebugMessage(0, 5, FColor::Blue, TEXT("Slide reset"));
        }
        if (bIsWallrunning)
        {
            bIsWallrunning = false;
        }
    }
    // Debug message
    GEngine->AddOnScreenDebugMessage(0, 5, FColor::Emerald,
                                     FString::Printf(TEXT("Normal: %s, RightVector: %s"), *Hit.Normal.ToString(),
                                                     *GetActorRightVector().ToString()));
    // Checks if there is a wall
    if (IsWall(Hit.Normal))
    {
        if (!bIsWallrunning)
        {
            // GEngine->AddOnScreenDebugMessage(0, 5, FColor::Blue, TEXT("IsWall = True!"));
            StartWallRun(Hit.Normal);
        }
        bIsOnWall = true;
    }
}
// Makes smoothly camera tilt when sliding
void AFPSCharacter::SmoothCameraTilt(float Angle, const float &TiltSpeed, const float &DeltaTime)
{
    FRotator CameraTilt = CameraComp->GetRelativeRotation();
    if (!FMath::IsNearlyEqual(CameraTilt.Roll, Angle))
    {
        CameraTilt.Roll = FMath::FInterpTo(CameraTilt.Roll, Angle, DeltaTime, TiltSpeed);
        CameraComp->SetRelativeRotation(CameraTilt);
    }
}
// Gradually changes scale of player to crouch or normal scale
void AFPSCharacter::GradualCrouch(float ZScale, const float &DeltaTime)
{
    FVector NewScale = GetActorScale3D();
    if (!FMath::IsNearlyEqual(NewScale.Z, ZScale))
    {
        NewScale.Z = FMath::FInterpTo(NewScale.Z, ZScale, DeltaTime, CrouchTransitionSpeed);
        SetActorScale3D(NewScale);
    }
    FVector NewLocation = GetActorLocation();
    float TargetLocationZ = NewLocation.Z + (NormalScale.Z - ZScale) * (bIsCrouching ? -1 : 1);
    if (!FMath::IsNearlyEqual(NewLocation.Z, TargetLocationZ))
    {
        NewLocation.Z = FMath::FInterpTo(NewLocation.Z, TargetLocationZ, DeltaTime, CrouchTransitionSpeed);
        SetActorLocation(NewLocation);
    }
}
// Checks if the object the player collides with is a wall
bool AFPSCharacter::IsWall(const FVector &Normal)
{
    return Normal.Z >= -0.01 && Normal.Z <= 0.5;
    // return FMath::IsNearlyEqual(FMath::Abs(Normal.Z), 0);
    // return FMath::IsNearlyEqual(FMath::Abs(Normal.X), 1) || FMath::IsNearlyEqual(FMath::Abs(Normal.Y), 1);
}
// Starts the wall run
void AFPSCharacter::StartWallRun(const FVector Normal)
{
    if (!GetCharacterMovement()->IsMovingOnGround())
    {
        WallNormalVector = Normal;
        WallRunTiltDirection = FMath::Sign(FVector::DotProduct(GetActorRightVector(), WallNormalVector));
        if (!bIsWallrunning)
        {
            GetCharacterMovement()->Velocity.Z = 0;
            GetCharacterMovement()->Velocity.Z += 100.f;
        }
        bIsWallrunning = true;
    }
}
// Called every frame when wall running
void AFPSCharacter::WallRun(const float &DeltaTime)
{
    // Force to keep player on wall when wall running
    GetCharacterMovement()->Velocity += -WallNormalVector * DeltaTime * WallRunSpeed;
    // Counter gravity to make player fall slower
    GetCharacterMovement()->Velocity += DeltaTime * GetCharacterMovement()->Mass * WallRunCounterGravity *
                                        -GetCharacterMovement()->GetGravityDirection() * .4f;
}
// Stops the wall running
void AFPSCharacter::StopWallRun()
{
    GetCharacterMovement()->Velocity += WallNormalVector * WallRunSpeed;
    bIsWallrunning = false;
    bIsOnWall = false;
}
// Jumos off the wall when wall running
void AFPSCharacter::WallJump()
{
    if (bIsWallrunning)
    {
        StopWallRun();
        GetCharacterMovement()->Launch(
            (FVector::UpVector * 1.7 + WallNormalVector * 2 + GetCharacterMovement()->Velocity.GetSafeNormal()) *
            WallJumpForce);
    }
}
