// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
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

    CrouchScale *= NormalScale.Z;

    // Sets character's max walkspeed to default set in the class
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->AirControl = .7f;
    GetCharacterMovement()->FormerBaseVelocityDecayHalfLife = 1;
    GetCharacterMovement()->MaxStepHeight = 50;
    GetMesh()->bAutoActivate = false;
    CameraComp->FieldOfView = 140.f;
    GetCapsuleComponent()->SetCapsuleHalfHeight(50);
    GetCapsuleComponent()->SetCapsuleRadius(26);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    SpringArm->TargetArmLength = 0;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 200;
    bIsSpatiallyLoaded = false;
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Links oncomponenthit function
    GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AFPSCharacter::OnComponentHitCharacter);
    // Links onLanded function
    LandedDelegate.AddDynamic(this, &AFPSCharacter::OnJumpLand);
    // Set player scale to default scale
    SetActorScale3D(NormalScale);
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
        GradualCrouch(CrouchScale, DeltaTime);
        if (GetCharacterMovement()->IsMovingOnGround() && GetCharacterMovement()->IsJumpAllowed())
        {
            // Applies force to speed up player when sliding down slopes

            // Projected vector on slope
            FVector ProjectedVector =
                FVector::VectorPlaneProject(FVector::DownVector, GetCharacterMovement()->CurrentFloor.HitResult.Normal);
            // Adds downwards force based off player's allignment off slope
            GetCharacterMovement()->Velocity +=
                FMath::Abs(FVector::DotProduct(GetActorForwardVector(), ProjectedVector.GetSafeNormal2D())) *
                ProjectedVector * DeltaTime * 10000.f;
            // Applies gradual slide force to counter friction
            GradualSlide(DeltaTime);
        }
    }
    else
    {
        // Makes smoothly camera tilt when not sliding
        if (!bIsOnWall || !bIsWallrunning)
        {
            SmoothCameraTilt(0.f, SlideCameraTiltSpeed, DeltaTime);
        }
        // Gradually changes scale of player to normal scale
        GradualCrouch(NormalScale.Z, DeltaTime);
    }
    if (bIsWallrunning && bIsOnWall)
    {
        // Triggers every 1 / 5 of a second
        if (FrameCounter % 5 / DeltaTime == 0)
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
    //                                                  GetCharacterMovement()->Velocity.SizeSquared2D(),
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
// * Crouching and sliding functionality
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
    //                                  GetCharacterMovement()->Velocity.SizeSquared2D()));
    // Sets ground friction to sliding friction
    GetCharacterMovement()->GroundFriction = SlideFriction;
    GetCharacterMovement()->BrakingFrictionFactor = 0.1f;
    // Sets walkspeed to bIsCrouching walkspeed
    GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;

    // Checks if character is on ground
    if (GetCharacterMovement()->IsMovingOnGround())
    {
        StartSlide();
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
// Gradually changes scale of player to crouch or normal scale
void AFPSCharacter::GradualCrouch(const float &ZScale, const float &DeltaTime)
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
// TODO - Check if function requires bool
// Applies gradual slide force to player
// Returns true when still applying force and false when it has stopped
bool AFPSCharacter::GradualSlide(const float &DeltaTime)
{
    // Velocity vector to add to player
    AddVelocityMag = FMath::FInterpTo(AddVelocityMag, 0.f, DeltaTime, 20.f);

    // GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red,
    //                                  FString::Printf(TEXT("AddVelocityMag: %d"), AddVelocityMag));

    // Checks if adding velocity is needed
    if (!FMath::IsNearlyEqual(AddVelocityMag, 0))
    {
        GetCharacterMovement()->Velocity +=
            AddVelocityMag * GetCharacterMovement()->Velocity.GetSafeNormal2D() * DeltaTime * 60;
        return true;
    }
    else
    {
        return false;
    }
}
// Applies initial slide force and starts gradual slide
void AFPSCharacter::StartSlide()
{
    // Checks if player has enough speed to apply slide force
    if (GetCharacterMovement()->Velocity.SizeSquared2D() > MinSlideSpeed * MinSlideSpeed && !bAppliedSlideForce)
    {
        // Adds impulse force to character
        GetCharacterMovement()->Velocity += GetCharacterMovement()->Velocity.GetSafeNormal2D() * SlideForce;
        bAppliedSlideForce = true;
        AddVelocityMag = GradualSlideForce;
        // GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Cyan, TEXT("JumpSlide"));
    }
}
// Triggers when player hits an object
void AFPSCharacter::OnComponentHitCharacter(UPrimitiveComponent *HitComp, AActor *OtherActor,
                                            UPrimitiveComponent *OtherComp, FVector NormalImpulse,
                                            const FHitResult &Hit)
{
    // GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Cyan, TEXT("CompHit"));

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
void AFPSCharacter::SmoothCameraTilt(const float &Angle, const float &TiltSpeed, const float &DeltaTime)
{
    FRotator CameraTilt = CameraComp->GetRelativeRotation();
    if (!FMath::IsNearlyEqual(CameraTilt.Roll, Angle))
    {
        CameraTilt.Roll = FMath::FInterpTo(CameraTilt.Roll, Angle, DeltaTime, TiltSpeed);
        CameraComp->SetRelativeRotation(CameraTilt);
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
void AFPSCharacter::StartWallRun(const FVector &Normal)
{
    if (!GetCharacterMovement()->IsMovingOnGround())
    {
        WallNormalVector = Normal;
        WallRunTiltDirection = FMath::Sign(FVector::DotProduct(GetActorRightVector(), WallNormalVector));
        if (!bIsWallrunning)
        {
            GetCharacterMovement()->Velocity.Z = 150;
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
// Jumps off the wall when wall running
void AFPSCharacter::WallJump()
{
    // Check if character is on wall and wall running
    if (bIsWallrunning && bIsOnWall)
    {
        StopWallRun();
        // Records velocity before the jump
        FVector InitVelocity = GetCharacterMovement()->Velocity;
        // Launches the player upwards and off the wall
        GetCharacterMovement()->Launch((FVector::UpVector * 1.7 +
                                        FVector::VectorPlaneProject(WallNormalVector, FVector::UpVector) * 2 +
                                        GetCharacterMovement()->Velocity.GetSafeNormal()) *
                                       WallJumpForce);
        // Adds back velocity after jump to preserve momentum
        GetCharacterMovement()->Velocity += InitVelocity;
    }
}
// Triggers on landing from jump
void AFPSCharacter::OnJumpLand(const FHitResult &Hit)
{
    // TODO - Add functionality for refreshing double jump here
    if (bIsCrouching)
    {
        StartSlide();
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
// TODO #3 - Add double jumping
// TODO #4 - Add vaulting functionality
