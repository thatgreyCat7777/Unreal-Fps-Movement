// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/Platform.h"
#include "InputTriggers.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Color.h"
#include "Math/MathFwd.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/CoreMiscDefines.h"
#include "Templates/Casts.h"
#include "Delegates/Delegate.h"
#include <cmath>

// Sets default values
AFPSCharacter::AFPSCharacter()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need
    // it.
    PrimaryActorTick.bCanEverTick = true;

    // Setup Static mesh
    PlayerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
    PlayerMesh->SetupAttachment(GetCapsuleComponent());
    PlayerMesh->SetCollisionProfileName(TEXT("Pawn"));

    // Setup spring arm
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(GetCapsuleComponent());

    // Setup camera
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComp->SetupAttachment(SpringArm);

    // Sets character's max walkspeed to default set in the class
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->AirControl = .7f;
    GetCharacterMovement()->FormerBaseVelocityDecayHalfLife = 1;
    GetCharacterMovement()->MaxStepHeight = 50;
    GetCharacterMovement()->JumpZVelocity = 620;
    GetMesh()->bAutoActivate = false;
    CameraComp->FieldOfView = 120.f;
    GetCapsuleComponent()->SetCapsuleHalfHeight(50);
    GetCapsuleComponent()->SetCapsuleRadius(26);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    SpringArm->TargetArmLength = 0;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 200;
    bIsSpatiallyLoaded = false;
    AirJumpCount = AirJumpMax;
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Links oncomponenthit function
    GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AFPSCharacter::OnComponentHitCharacter);
    // Links onLanded function
    LandedDelegate.AddDynamic(this, &AFPSCharacter::OnJumpLand);
    // Links line trace function
    WallLineTraceDelegate.AddDynamic(this, &AFPSCharacter::OnLineWallTraceHit);
    // Set crouch scale to scaled value of normal scale
    CrouchScale *= NormalScale.Z;
    // Set player scale to default scale
    SetActorScale3D(NormalScale);
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    FHitResult LineTrace;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (GetWorld()->LineTraceSingleByChannel(LineTrace, GetActorLocation(),
                                             GetActorLocation() + GetActorRightVector() * 80,
                                             WallDetectionChannel.GetValue(), QueryParams))
    {
        WallLineTraceDelegate.Broadcast(LineTrace);
    }
    else if (GetWorld()->LineTraceSingleByChannel(LineTrace, GetActorLocation(),
                                                  GetActorLocation() + GetActorRightVector() * -80,
                                                  WallDetectionChannel.GetValue(), QueryParams))
    {
        WallLineTraceDelegate.Broadcast(LineTrace);
    }
    else if (bIsWallrunning)
    {
        // If no wall found anymore, stop wall run
        StopWallRun();
    }
    if (bIsCrouching)
    {
        // Makes smoothly camera tilt when sliding
        if (!bIsWallrunning)
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
        if (!bIsWallrunning)
        {
            SmoothCameraTilt(0.f, SlideCameraTiltSpeed, DeltaTime);
        }
        // Gradually changes scale of player to normal scale
        GradualCrouch(NormalScale.Z, DeltaTime);
    }
    if (bIsWallrunning)
    {
        WallRun(DeltaTime);
        SmoothCameraTilt(WallRunTiltDirection * WallRunCameraTiltAngle, WallRunTransitionSpeed, DeltaTime);
    }
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
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AFPSCharacter::WallJump);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AFPSCharacter::AirJump);
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
    WalkingInput = Instance.GetValue().Get<FVector>();
    // Adds input corresponding to character's forward and right vector
    AddMovementInput(GetActorForwardVector(), WalkingInput.Y);
    AddMovementInput(GetActorRightVector(), WalkingInput.X);
    // GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Green,
    //                                  FString::Printf(TEXT("Velocity = %d, Floor normal = %d"),
    //                                                  GetCharacterMovement()->Velocity.SizeSquared2D(),
    //                                                  GetCharacterMovement()->CurrentFloor.HitResult.Normal.Z));
}
// TODO #7 - Implement air strafing
void AFPSCharacter::AirAccelerate(const FVector &WishVelocity) { return; }
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
void AFPSCharacter::StartWallRun(const FHitResult &Hit)
{
    if (!GetCharacterMovement()->IsMovingOnGround())
    {
        WallNormalVector = Hit.Normal;
        CurrentWall = Hit.GetComponent();
        WallRunTiltDirection = FMath::Sign(FVector::DotProduct(GetActorRightVector(), WallNormalVector));
        if (!bIsWallrunning)
        {
            GetCharacterMovement()->Velocity.Z = 250;
        }
        bIsWallrunning = true;
        // Reset double jump
        AirJumpCount = AirJumpMax;
        // Set gravity to normal
        GetCharacterMovement()->GravityScale = 1;
        WallPerpendicularNormalVector = VectorRotate(WallNormalVector, PI / 2.0, 0, 0);
        WallPerpendicularNormalVector *=
            FMath::Sign(FVector::DotProduct(GetCharacterMovement()->Velocity, WallPerpendicularNormalVector));

        GEngine->AddOnScreenDebugMessage(
            INDEX_NONE, 5.0f, FColor::Blue,
            FString::Printf(TEXT("Perpenticulat wall vector = %s"), *WallPerpendicularNormalVector.ToString()));
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
    GetCharacterMovement()->Velocity += WallPerpendicularNormalVector * DeltaTime * WallRunSpeed * .1;
}
// Stops the wall running
void AFPSCharacter::StopWallRun()
{
    GetCharacterMovement()->Velocity += WallNormalVector * WallRunSpeed * GetWorld()->GetDeltaSeconds();
    bIsWallrunning = false;
    // Reset current wall pointer
    CurrentWall = nullptr;
    // If falling when wall run stops, set to falling gravity
    if (GetCharacterMovement()->IsFalling())
    {
        GetCharacterMovement()->GravityScale = 1.5;
    }
}
// Jumps off the wall when wall running
void AFPSCharacter::WallJump()
{
    // TODO #8 - Make wall jump intensity consistent regardless of player's orientation to wall
    // Check if character is on wall and wall running
    if (bIsWallrunning)
    {
        StopWallRun();
        GEngine->AddOnScreenDebugMessage(
            INDEX_NONE, 2, FColor::Red,
            FString::Printf(TEXT("Wall Normal: %s"),
                            *FVector::VectorPlaneProject(WallNormalVector, FVector::UpVector).ToString()));
        // TODO #6 - Make wall jump preserve xy velocity
        // Launches the player upwards and off the wall
        LaunchCharacter(
            (FVector::UpVector * 1.7 + FVector::VectorPlaneProject(WallNormalVector, FVector::UpVector) * 2) *
                WallJumpForce,
            false, true);
    }
}
// Triggers on landing from jump
void AFPSCharacter::OnJumpLand(const FHitResult &Hit)
{
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
        StopWallRun();
    }
    // Reset double jump
    AirJumpCount = AirJumpMax;
    // Reset gravity scale to normal
    GetCharacterMovement()->GravityScale = 1;
}
// TODO #3 - Add double jumping
void AFPSCharacter::AirJump()
{
    // Increases gravity when jumping
    GetCharacterMovement()->GravityScale = 1.5;
    if (GetCharacterMovement()->IsFalling() && !bIsWallrunning && AirJumpCount > 0)
    {
        // Spawns particle effect
        FVector Location = GetActorLocation();
        Location.Z -= 55;
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, Location);
        // Adds jump force
        LaunchCharacter(GetActorUpVector() * GetCharacterMovement()->JumpZVelocity, false, true);
        AirJumpCount--;
    }
}
// TODO #9 - Prevent player from bouncing consistently when beside a wall
void AFPSCharacter::OnLineWallTraceHit(const FHitResult &Hit)
{
    if (Hit.GetComponent() && CurrentWall && Hit.GetComponent() == CurrentWall)
        return;
    if (IsWall(Hit.Normal))
    {
        if (GetCharacterMovement()->IsFalling() && !bIsWallrunning)
        {
            GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2, FColor::Red,
                                             FString::Printf(TEXT("IsWall! Hit.Normal = %s"), *Hit.Normal.ToString()));
            StartWallRun(Hit);
        }
    }
}
// Returns rotated vector by pitch, yaw and roll angles respectively where angles are in radians
FVector AFPSCharacter::VectorRotate(const FVector &vec, const double &theta, const double &phi, const double &rho)
{
    // Precomputed values of sin and cos where 0,1,2th index represents sin and cos of theta, phi and rho respectively
    double s[3] = {sin(theta), sin(phi), sin(rho)};
    double c[3] = {cos(theta), cos(phi), cos(rho)};

    return vec.X * FVector(c[0] * c[1], s[0] * c[1], s[1]) +
           vec.Y * FVector(s[0] * c[2] + c[0] * s[1] * s[2], s[0] * s[1] * s[2] - c[0] * c[2], -c[1] * s[2]) +
           vec.Z * FVector(-s[0] * s[2] - s[0] * s[1] * c[2], c[0] * s[2] - s[0] * s[1] * c[2], -c[1] * c[2]);
}
// TODO #4 - Add vaulting functionality
