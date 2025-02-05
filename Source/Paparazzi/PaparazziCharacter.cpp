// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PaparazziCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

const float APaparazziCharacter::MAXDASHSTAMINA = 5.f;

//////////////////////////////////////////////////////////////////////////
// APaparazziCharacter

APaparazziCharacter::APaparazziCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)


	// Set Dash Paramters
	Dashing = false;
	DashTime = MAXDASHSTAMINA;
	DashSpeedMultiplier = 1.5;

}

void APaparazziCharacter::BeginPlay() {
	Super::BeginPlay();

	// Set Walk Speed
	UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	WalkSpeed = MovementComponent->MaxWalkSpeed;
	OrignalAcceleration = MovementComponent->MaxAcceleration;
}

// Called every frame
void APaparazziCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// Subtract Dash Stamina
	if (Dashing && DashTime > 0.f) {
		UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
		MovementComponent->MaxWalkSpeed = WalkSpeed * DashSpeedMultiplier;
		MovementComponent->MaxAcceleration = OrignalAcceleration * DashSpeedMultiplier;
		DashTime -= DeltaTime;
		if (DashTime < 0.f) {
			DashTime = 0.f;
		}
	} else {
		UCharacterMovementComponent* MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
		MovementComponent->MaxWalkSpeed = WalkSpeed;
		MovementComponent->MaxAcceleration = OrignalAcceleration;
		if (!Dashing && DashTime < MAXDASHSTAMINA) {
			DashTime += DeltaTime;
			if (DashTime > MAXDASHSTAMINA) {
				DashTime = MAXDASHSTAMINA;
			}
		}
	}

}

float APaparazziCharacter::GetDashStaminaRate() {
	return DashTime / MAXDASHSTAMINA;
}

//////////////////////////////////////////////////////////////////////////
// Input

void APaparazziCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &APaparazziCharacter::StartDashing);
	PlayerInputComponent->BindAction("Dash", IE_Released, this, &APaparazziCharacter::StopDashing);

	PlayerInputComponent->BindAxis("MoveForward", this, &APaparazziCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APaparazziCharacter::MoveRight);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &APaparazziCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &APaparazziCharacter::TouchStopped);
}

void APaparazziCharacter::StartDashing() {
	Dashing = true;
}

void APaparazziCharacter::StopDashing() {
	Dashing = false;
}

void APaparazziCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void APaparazziCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		StartDashing();
}

void APaparazziCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopDashing();
}

void APaparazziCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void APaparazziCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
