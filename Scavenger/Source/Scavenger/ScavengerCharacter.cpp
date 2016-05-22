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

	//Aim Button
	InputComponent->BindAction("Aim", IE_Pressed, this, &AScavengerCharacter::StartAiming);
	InputComponent->BindAction("Aim", IE_Released, this, &AScavengerCharacter::StopAiming);

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
	MyMove = FindComponentByClass<UCharacterMovementComponent>();
	
	if (MyCapsule)
	{
		MyCapsule->OnComponentHit.AddDynamic(this, &AScavengerCharacter::OnHit);
	}

	if (GetCameraBoom())
	{
		StoredAimZoomDistance = GetCameraBoom()->TargetArmLength;
	}
}

void AScavengerCharacter::StartRunning()
{
	if (Aiming) StopAiming();
	if (InCoverCPP) ExitCover();

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
		if (CheckIsMovementAllowed(Direction, Value)) AddMovementInput(Direction, Value);
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
		if (CheckIsMovementAllowed(Direction, Value)) AddMovementInput(Direction, Value);
	}
}

bool AScavengerCharacter::CheckIsMovementAllowed(FVector Direction, float Value)
{
	if (InCoverCPP)
	{
		// Check if we are trying to leave cover by pulling off
		float AngleFound = AngleBetween(MyMove->GetLastInputVector(), -CurrentCoverDirection);
		if (AngleFound <= MaxCoverAngle)
		{
			EnterCoverTimer++;
			if (EnterCoverTimer >= EnterCoverHoldTime)
			{
				ExitCover();
			}
		}

		//Set up Query Parameters
		FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());

		// Ray-Cast our Grabber Ray
		FHitResult LeftHit;
		FHitResult RightHit;

		FVector LocationPlusMovementLeft = GetActorLocation() + (Direction) + GetActorRightVector() * -40.0;
		FVector LocationPlusMovementRight = GetActorLocation() + (Direction) + GetActorRightVector() * 40.0;

		GetWorld()->LineTraceSingleByObjectType(
			LeftHit,
			LocationPlusMovementLeft,
			LocationPlusMovementLeft + CurrentCoverDirection * CoverSenseDistance,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
			TraceParameters
		);

		GetWorld()->LineTraceSingleByObjectType(
			RightHit,
			LocationPlusMovementRight,
			LocationPlusMovementRight + CurrentCoverDirection * CoverSenseDistance,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
			TraceParameters
		);

		if (!LeftHit.GetComponent() || !RightHit.GetComponent()) return false;
	}

	return true;
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

		//DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + SurfaceNormal*40.0f, FColor(255, 0, 0), false, 0.0f, 0, 10.0f);

		//Check if approach angle is less than the maximum angle to enter cover, to prevent drivebys
		float AngleFound = AngleBetween(MoveVector, SurfaceNormal * -1);
		if (AngleFound < MaxCoverAngle && InCoverCPP == false)
		{
			EnterCoverTimer++;
			if (EnterCoverTimer >= EnterCoverHoldTime)
			{
				//UE_LOG(LogTemp, Warning, TEXT("EnteringCoverOhGodHelp, %f"), AngleFound);
				CurrentCoverDirection = SurfaceNormal * -1;
				EnterCover();
			}
		}

	}
}

void AScavengerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // Call parent class tick function  

	if (GetCharacterMovement()->IsWalking()) OnGround = true;
	else OnGround = false;

	if (InCoverCPP)
	{
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), CurrentCoverDirection.Rotation(), GetWorld()->GetDeltaSeconds(), 640));
		
		StickToCover();
	}

	FVector MoveVector = GetMovementComponent()->GetLastInputVector();
	MoveVector.Normalize();
}

void AScavengerCharacter::StickToCover()
{
	//Set up Query Parameters
	FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());

	// Ray-Cast our Grabber Ray
	FHitResult LeftHit;
	FHitResult RightHit;

	FVector LocationPlusMovementLeft = GetActorLocation() + (GetMovementComponent()->GetLastInputVector()) + GetActorRightVector() * -41.0;
	FVector LocationPlusMovementRight = GetActorLocation() + (GetMovementComponent()->GetLastInputVector()) + GetActorRightVector() * 41.0;

	GetWorld()->LineTraceSingleByObjectType(
		LeftHit,
		LocationPlusMovementLeft,
		LocationPlusMovementLeft + CurrentCoverDirection * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	GetWorld()->LineTraceSingleByObjectType(
		RightHit,
		LocationPlusMovementRight,
		LocationPlusMovementRight + CurrentCoverDirection * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	DrawDebugLine(GetWorld(), LocationPlusMovementLeft, LocationPlusMovementLeft + CurrentCoverDirection * CoverSenseDistance, FColor(0, 255, 0), false, 0.0f, 0, 3.0f);
	DrawDebugLine(GetWorld(), LocationPlusMovementRight, LocationPlusMovementRight + CurrentCoverDirection * CoverSenseDistance, FColor(0, 255, 0), false, 0.0f, 0, 3.0f);

	if (LeftHit.GetComponent())
	{
		if (LeftHit.GetComponent()->ComponentHasTag("Cover"))
		{
			OnEdgeLeft = false;
		}
		else
		{
			// No cover found! Must be at the end. 
			OnEdgeLeft = true;
		}
	}
	else
	{
		// No cover found! Must be at the end. 
		OnEdgeLeft = true;
	}

	if (RightHit.GetComponent())
	{
		if (RightHit.GetComponent()->ComponentHasTag("Cover"))
		{
			OnEdgeRight = false;
		}
		else
		{
			// No cover found! Must be at the end. 
			OnEdgeRight = true;
		}
	}
	else
	{
		// No cover found! Must be at the end. 
		OnEdgeRight = true;
	}

	if (OnEdgeLeft == false && OnEdgeRight == false)
	{
		EdgeAdjustedLeft = false;
		EdgeAdjustedRight = false;
	}

	if (OnEdgeLeft == true && OnEdgeRight == true)
	{
		ExitCover();
	}

	if (OnEdgeLeft == true)
	{
		AddMovementInput(GetActorRightVector(), 1.0f);
		EdgeAdjustedLeft = true;
	}

	if (OnEdgeRight == true)
	{
		AddMovementInput(GetActorRightVector(), -1.0f);
		EdgeAdjustedRight = true;
	}
}

void AScavengerCharacter::Jump()
{
	if (Aiming) StopAiming();
	if (OnGround)
	{
		bPressedJump = true;
		JumpKeyHoldTime = 0.0f;
	}
	if (InCoverCPP) ExitCover();
}

void AScavengerCharacter::StartAiming()
{
	if (Running) return;

	if (InCoverCPP)
	{
		if (GetCameraBoom())
		{
			if (EdgeAdjustedLeft)
			{
				GetCameraBoom()->SocketOffset.Y = -AimOffsetAmount;
			}
			else if (EdgeAdjustedRight)
			{
				GetCameraBoom()->SocketOffset.Y = AimOffsetAmount;
			}
			else return; //Can't aim, no edges
		}
		//ExitCover();
	}

	if (GetCameraBoom())
	{
		if (!InCoverCPP) GetCameraBoom()->SocketOffset.Y = AimOffsetAmount;
		GetCameraBoom()->TargetArmLength = AimZoomDistance;
		MyMove->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;		
	}

	Aiming = true;
}

void AScavengerCharacter::StopAiming()
{

	if (InCoverCPP)
	{
		// If on a corner
			// Go back under cover
	}

	if (GetCameraBoom())
	{
		GetCameraBoom()->TargetArmLength = StoredAimZoomDistance;
		MyMove->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;
		GetCameraBoom()->SocketOffset.Y = 0.0f;
	}

	Aiming = false;
}

void AScavengerCharacter::ExitCover()
{
	CrouchedCPP = false;
	EnterCoverTimer = 0;

	InCoverCPP = false;
	OnEdgeLeft = false;
	OnEdgeRight = false;
	EdgeAdjustedLeft = false;
	EdgeAdjustedRight = false;

	if (MyMove)
	{
		MyMove->bOrientRotationToMovement = true;
		MyMove->bConstrainToPlane = false;
	}
}

void AScavengerCharacter::EnterCover()
{
	CrouchedCPP = true;
	EnterCoverTimer = 0;
	//Set up Query Parameters
	FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());

	// Ray-Cast our Grabber Ray
	FHitResult LeftHit;
	FHitResult RightHit;

	FVector LocationPlusMovementLeft = GetActorLocation() + (GetMovementComponent()->GetLastInputVector()) + GetActorRightVector() * -40.0;
	FVector LocationPlusMovementRight = GetActorLocation() + (GetMovementComponent()->GetLastInputVector()) + GetActorRightVector() * 40.0;

	GetWorld()->LineTraceSingleByObjectType(
		LeftHit,
		LocationPlusMovementLeft,
		LocationPlusMovementLeft + CurrentCoverDirection * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	GetWorld()->LineTraceSingleByObjectType(
		RightHit,
		LocationPlusMovementRight,
		LocationPlusMovementRight + CurrentCoverDirection * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	if (LeftHit.GetComponent() && RightHit.GetComponent())
	{
		//SetActorRotation(GetActorRotation().)
		InCoverCPP = true;

		if (MyMove)
		{
			MyMove->bOrientRotationToMovement = false;
			MyMove->SetPlaneConstraintNormal(CurrentCoverDirection);
			MyMove->bConstrainToPlane = true;
		}
	}
	else return;
}

/*bool AScavengerCharacter::IsInCover()
{
	return InCover;
}*/

float AScavengerCharacter::AngleBetween(FVector a, FVector b)
{
	//UE_LOG(LogTemp, Warning, TEXT("Vectors: %s, %s"), *a.ToString(), *b.ToString());
	float ThisIsSparta;
	ThisIsSparta = FMath::RadiansToDegrees(acosf(FVector::DotProduct(a, b)));

	//UE_LOG(LogTemp, Warning, TEXT("%s, %s - This is: %f"), *a.ToString(), *b.ToString(), ThisIsSparta);
	return ThisIsSparta;
}
