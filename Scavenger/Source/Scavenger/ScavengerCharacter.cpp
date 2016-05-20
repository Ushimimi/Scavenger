// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Scavenger.h"
#include "ScavengerCharacter.h"

//////////////////////////////////////////////////////////////////////////
// AScavengerCharacter

AScavengerCharacter::AScavengerCharacter()
{
	// Set up a tick so we can do stuff here where it's less messy than in a separate component
	PrimaryActorTick.bCanEverTick = true;

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
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->AttachTo(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AScavengerCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Set up gameplay key bindings
	check(InputComponent);
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAxis("MoveForward", this, &AScavengerCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AScavengerCharacter::MoveRight);

	//Run Button
	InputComponent->BindAction("Run", IE_Pressed, this, &AScavengerCharacter::StartRunning);
	InputComponent->BindAction("Run", IE_Released, this, &AScavengerCharacter::StartWalking);

	//Cover Button
	//InputComponent->BindAction("TakeCover", IE_Pressed, this, &AScavengerCharacter::CoverButton);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AScavengerCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AScavengerCharacter::LookUpAtRate);

	// handle touch devices
	InputComponent->BindTouch(IE_Pressed, this, &AScavengerCharacter::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &AScavengerCharacter::TouchStopped);

	WalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	RunSpeed = WalkSpeed * RunMultiplier;

	MyCapsule = GetCapsuleComponent();

	if (MyCapsule)
	{
		MyCapsule->OnComponentHit.AddDynamic(this, &AScavengerCharacter::OnHit);
	}
}

void AScavengerCharacter::StartRunning()
{
	Running = true;
	float OldSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

	UE_LOG(LogTemp, Warning, TEXT("Running! Old speed: %f, New speed: %f"), OldSpeed, GetCharacterMovement()->MaxWalkSpeed);
}

void AScavengerCharacter::StartWalking()
{
	Running = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AScavengerCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	// jump, but only on the first touch
	if (FingerIndex == ETouchIndex::Touch1)
	{
		Jump();
	}
}

void AScavengerCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		StopJumping();
	}
}

void AScavengerCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AScavengerCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AScavengerCharacter::MoveForward(float Value)
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

void AScavengerCharacter::MoveRight(float Value)
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

void AScavengerCharacter::OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//UE_LOG(LogTemp, Warning, TEXT("Bonk"));
	if (OtherComp->ComponentHasTag("Cover"))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Derp"));

		//Get Surface Normal of hit surface
		FVector SurfaceNormal = Hit.Normal;

		//Flatten it on the Z axis to prevent weirdness
		SurfaceNormal.Z = 0.0f;
		
		//Compare it to movement vector, and align the surface vector to world space
		FVector MoveVector = GetMovementComponent()->GetLastInputVector();
		MoveVector.Z = 0;
		SurfaceNormal.Normalize();
		MoveVector.Normalize();
		//SurfaceNormal += GetActorLocation();

		DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + SurfaceNormal*40.0f, FColor(255, 0, 0), false, 0.0f, 0, 10.0f);

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + MoveVector*40.0f, FColor(0, 255, 0), false, 0.0f, 0, 10.0f);

		//Check if approach angle is less than the maximum angle to enter cover, to prevent drivebys
		float AngleFound = AngleBetween(MoveVector, SurfaceNormal * -1);
		if (AngleFound < MaxCoverAngle)
		{
			//UE_LOG(LogTemp, Warning, TEXT("EnteringCoverOhGodHelp, %f"), AngleFound);
		}

	}
}

void AScavengerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // Call parent class tick function  

	if (GetCharacterMovement()->IsWalking()) OnGround = true;
	else OnGround = false;

}

void AScavengerCharacter::Jump()
{
	if (OnGround)
	{
		bPressedJump = true;
		JumpKeyHoldTime = 0.0f;
	}
}

float AScavengerCharacter::AngleBetween(FVector a, FVector b)
{
	//UE_LOG(LogTemp, Warning, TEXT("Vectors: %s, %s"), *a.ToString(), *b.ToString());
	float ThisIsSparta;
	ThisIsSparta = FMath::RadiansToDegrees(acosf(FVector::DotProduct(a, b)));

	UE_LOG(LogTemp, Warning, TEXT("%s, %s - This is: %f"), *a.ToString(), *b.ToString(), ThisIsSparta);
	return ThisIsSparta;
}
