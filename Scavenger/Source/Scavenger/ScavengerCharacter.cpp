// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Scavenger.h"
#include "ScavengerCharacter.h"

#include "UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AScavengerCharacter

AScavengerCharacter::AScavengerCharacter()
{
	// Set up a tick so we can do stuff here where it's less messy than in a separate component
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

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

void AScavengerCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//UE_LOG(LogTemp, Warning, TEXT("Replicating"));

	//DOREPLIFETIME(AScavengerCharacter, MyMove);
	DOREPLIFETIME(AScavengerCharacter, InCoverCPP);
	DOREPLIFETIME(AScavengerCharacter, CrouchedCPP);
	DOREPLIFETIME(AScavengerCharacter, CoverFacingRightCPP);
	DOREPLIFETIME(AScavengerCharacter, CurrentCoverDirection);
	DOREPLIFETIME(AScavengerCharacter, Dashing);
	DOREPLIFETIME(AScavengerCharacter, Running);
	DOREPLIFETIME(AScavengerCharacter, DashDirection);
	DOREPLIFETIME(AScavengerCharacter, IsDashingCPP);
	DOREPLIFETIME(AScavengerCharacter, IsPoppedOutCPP);
	DOREPLIFETIME(AScavengerCharacter, IsAimingCPP);
	DOREPLIFETIME(AScavengerCharacter, IsDeadCPP);
	DOREPLIFETIME(AScavengerCharacter, AimPitchCPP);
	DOREPLIFETIME(AScavengerCharacter, AimYawCPP);
	
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
	InputComponent->BindAction("Aim", IE_Pressed, this, &AScavengerCharacter::LocalStartAiming);
	InputComponent->BindAction("Aim", IE_Released, this, &AScavengerCharacter::LocalStopAiming);

	InputComponent->BindAction("TakeCover", IE_Pressed, this, &AScavengerCharacter::Die);

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

	//WalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	//RunSpeed = WalkSpeed * RunMultiplier;

	MyCapsule = GetCapsuleComponent();
	MyMove = FindComponentByClass<UCharacterMovementComponent>();
	
	if (MyCapsule)
	{
		MyCapsule->OnComponentHit.AddDynamic(this, &AScavengerCharacter::OnHit);
	}

	if (GetCameraBoom())
	{
		StoredAimZoomDistance = 300.0;
		TargetAimOffsetAmount = GetCameraBoom()->SocketOffset.Y;
		TargetAimZoomDistance = StoredAimZoomDistance;
	}
	else
	{
		StoredAimZoomDistance = 300.0;
		TargetAimOffsetAmount = 0.0;
	}
}

void AScavengerCharacter::StartRunning_Implementation()
{
	RunKeyPressed = true;

	if (Dashing)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Already Dashing"));
		return;
	}

	if (IsAimingCPP) LocalStopAiming();

	if (InCoverCPP)
	{
		//UE_LOG(LogTemp, Warning, TEXT("In Cover, %d, %d"), DashCooldownTimer, DashCooldown);

		if (DashCooldownTimer >= DashCooldown)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Dash!"));
			ExitCover();
			StartDash();
		}
		return;
	}

	Running = true;
	float OldSpeed = GetCharacterMovement()->MaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

	ClientUpdateWalkSpeed(RunSpeed);

	//UE_LOG(LogTemp, Warning, TEXT("Running! Old speed: %f, New speed: %f"), OldSpeed, GetCharacterMovement()->MaxWalkSpeed);
}

void AScavengerCharacter::StartWalking_Implementation()
{
	RunKeyPressed = false;
	if (Dashing) return;
	Running = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	ClientUpdateWalkSpeed(WalkSpeed);

}

void AScavengerCharacter::StartDash_Implementation()
{
	DashTimer = 0;
	Dashing = true;
	//FVector MoveVector = GetMovementComponent()->GetLastInputVector();
	//MoveVector.Normalize();
	
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), CurrentCoverDirection.Rotation(), GetWorld()->GetDeltaSeconds(),9999));

	FVector MoveVector = GetActorRightVector();

	if (!CoverFacingRightCPP) MoveVector *= -1;
	
	if (Role == ROLE_Authority) GetCharacterMovement()->MaxWalkSpeed = DashSpeed;
	ClientUpdateWalkSpeed(DashSpeed);
	DashDirection = MoveVector;
	IsDashingCPP = true;
}

void AScavengerCharacter::ExecuteDash()
{
	if (Role == ROLE_Authority)
	{
		DashTimer++;
		if (DashTimer >= DashDuration)
		{
			StopDash();
		}
	}

	GetMovementComponent()->AddInputVector(DashDirection * DashForce, true);
}

void AScavengerCharacter::StopDash_Implementation()
{
	Dashing = false;
	DashCooldownTimer = 0;
	DashTimer = 0;
	if (Role == ROLE_Authority) GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	ClientUpdateWalkSpeed(WalkSpeed);
	IsDashingCPP = false;
	if (!InCoverCPP)
	{
		if (RunKeyPressed) StartRunning();
		else StartWalking();
	}
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
	if (IsDeadCPP) return false;
	if (IsPoppedOutCPP) return false;
	if (InCoverCPP && MyMove)
	{
		// Check if we are trying to leave cover by pulling off
		float AngleFound = AngleBetween(MyMove->GetLastInputVector(), -CurrentCoverDirection);
		if (AngleFound <= MaxCoverAngle)
		{
			EnterCoverTimer++;
			if (EnterCoverTimer >= EnterCoverHoldTime)
			{
				EnterCoverTimer = 0;
				ExitCover();
			}
		}

		if (EdgeAdjustedLeft)
		{
			if (AngleBetween(-GetActorRightVector(), MyMove->GetLastInputVector()) < 90.0)
			{
				return false;
			}
		}
		else if (EdgeAdjustedRight)
		{
			if (AngleBetween(GetActorRightVector(), MyMove->GetLastInputVector()) < 90.0)
			{
				return false;
			}
		}
	}

	if (Dashing) return false;

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
				EnterCoverTimer = 0;
				EnterCover(GetMovementComponent()->GetLastInputVector(), CurrentCoverDirection);
			}
		}
	}
}

void AScavengerCharacter::UpdateCamera()
{
	if (GetCameraBoom())
	{
		float CurrentLength = GetCameraBoom()->TargetArmLength;
		float CurrentOffset = GetCameraBoom()->SocketOffset.Y;

		float OffsetMultiplier = 1.0;
		if (IsPoppedOutCPP) OffsetMultiplier = 2.0;

		if (CurrentLength > TargetAimZoomDistance)
		{
			CurrentLength -= CameraTrackSpeed;
		}
		if (CurrentLength < TargetAimZoomDistance)
		{
			CurrentLength += CameraTrackSpeed;
		}

		if (CurrentOffset > TargetAimOffsetAmount * OffsetMultiplier)
		{
			CurrentOffset -= CameraTrackSpeed;
		}

		if (CurrentOffset < TargetAimOffsetAmount * OffsetMultiplier)
		{
			CurrentOffset += CameraTrackSpeed;
		}

		GetCameraBoom()->TargetArmLength = CurrentLength;
		GetCameraBoom()->SocketOffset.Y = CurrentOffset;
	}
}

void AScavengerCharacter::UpdateAiming()
{
	if (InputComponent)
	{
		float DerpAim = GetControlRotation().Pitch;

		if (DerpAim > 180.0) DerpAim -= 360.0;

		AimPitchCPP = DerpAim + 10.0;

		if (IsPoppedOutCPP)
		{
			//float AngleOffset = AngleBetween(CurrentCoverDirection, GetControlRotation().Vector());

			float AngleOffset = GetControlRotation().Yaw - GetActorRotation().Yaw;

			if (AngleOffset > 180.0) AngleOffset -= 360.0;

			//UE_LOG(LogTemp, Warning, TEXT("Current Offset: %f"), AngleOffset);
			AimYawCPP = AngleOffset;
		}
		else AimYawCPP = 0.0;

		//Find viable target under crosshair, if this is our actor

		//Set up Query Parameters
		FCollisionQueryParams TraceParameters(FName(TEXT("AimTrace")), false, this);

		// Send out our target probe ray from the crosshair's world-space coordinates
		FHitResult AimHit;

		MyPC = Cast<APlayerController>(Controller);

		if (MyPC && GetFollowCamera())
		{
			// Got a PlayerController
			//AHUD* MyHud = MyPC->GetHUD();
			
			FVector CrosshairLocation = CrosshairLocationCPP;
			FVector CameraDirection = CrosshairRayCPP;

			GetWorld()->LineTraceSingleByObjectType(
				AimHit,
				CrosshairLocation,
				CrosshairLocation + (CrosshairRayCPP * AimDistance),
				FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
				TraceParameters
			);

			FVector DirectionToTarget;

			if (AimHit.GetActor())
			{
				DirectionToTarget = AimHit.ImpactPoint - GetCharacterMovement()->GetActorLocation();
			}
			else DirectionToTarget = CameraDirection;

			GetWorld()->LineTraceSingleByObjectType(
				AimHit,
				CrosshairLocation,
				CrosshairLocation + (CrosshairRayCPP * AimDistance),
				FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
				TraceParameters
			);

			if (AimHit.GetActor())
			{
				DirectionToTarget = AimHit.ImpactPoint - GetCharacterMovement()->GetActorLocation();
			}
			
			//DrawDebugLine(GetWorld(), GetCharacterMovement()->GetActorLocation(), GetCharacterMovement()->GetActorLocation() + DirectionToTarget, FColor(0, 255, 0), false, 0.0f, 0, 10.0f);

			/*if (IsAimingCPP)
			{
				AimPitchCPP = DirectionToTarget.X;
				//AimYawCPP = DirectionToTarget.Y;
			}*/
		}

		ServerSetAimPitch(AimPitchCPP);
		ServerSetAimYaw(AimYawCPP);
	}

	if (Role < ROLE_Authority)
	{
		//ServerSetAimPitch(AimPitchCPP);
	}

	
}

void AScavengerCharacter::Die_Implementation()
{
	IsDeadCPP = true;
}

void AScavengerCharacter::BeginPlay()
{
	Super::BeginPlay();

	//UE_LOG(LogTemp, Warning, TEXT("Derp"));

	UWorld* const World = GetWorld();

	if (World) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = this;
		EquippedWeapon = World->SpawnActor<AGun>(WeaponBPClass, GetActorLocation(), GetActorRotation(), SpawnParams);
	}

	FName fnWeaponSocket = TEXT("RHand_Socket");

	EquippedWeapon->AttachRootComponentTo(GetMesh(),fnWeaponSocket, EAttachLocation::SnapToTarget, true);

	MyPC = Cast<APlayerController>(Controller);

	if (MyPC)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Initalizing ScavengerCharacter...Found PlayerController!"))
			//MyPC->GetHUD()-> // ... CALL_WHETEVER_YOU_WANT_HERE
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Initalizing ScavengerCharacter...Did not find PlayerController."))
	}
}

void AScavengerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // Call parent class tick function  
	//Set blueprint aiming value every frame. May change this later in case I need to time it differently
	//IsAimingCPP = Aiming;

	UpdateCamera();
	UpdateAiming();

	if (GetCharacterMovement()->IsWalking()) OnGround = true;
	else OnGround = false;
	if (InCoverCPP)
	{
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), CurrentCoverDirection.Rotation(), GetWorld()->GetDeltaSeconds(), 640));

		StickToCover();
	}

	if (Dashing)
	{
		ExecuteDash();
	}
	else if (DashCooldownTimer < DashCooldown) DashCooldownTimer++;

	FVector MoveVector = GetMovementComponent()->GetLastInputVector();
	MoveVector.Normalize();
}

void AScavengerCharacter::StickToCover()
{
	FVector MoveVector = GetMovementComponent()->GetLastInputVector();

	if (MoveVector != FVector::ZeroVector)
	{
		//Check input for left or right movement to flip animation direction
		if (AngleBetween(MoveVector, GetActorRightVector()) < AngleBetween(MoveVector, -GetActorRightVector()))
		{
			CoverFacingRightCPP = true;
			ServerSetCoverState(true, IsPoppedOutCPP);
		}
		else
		{
			CoverFacingRightCPP = false;
			ServerSetCoverState(false, IsPoppedOutCPP);
		}
	}

	if (Role == ROLE_Authority)
	{
		//DrawDebugLine(GetWorld(), GetActorLocation() + (GetActorUpVector() * 20.0), GetActorLocation() + (GetActorUpVector() * 20.0) + CurrentCoverDirection*40.0f, FColor(255, 0, 0), false, 0.0f, 0, 10.0f);

		//Set up Query Parameters
		FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());

		// Ray-Cast our Grabber Ray
		FHitResult LeftHit;
		FHitResult RightHit;

		//FVector LocationPlusMovementLeft = GetActorLocation() + (GetMovementComponent()->GetLastInputVector()) + GetActorRightVector() * -41.0;
		//FVector LocationPlusMovementRight = GetActorLocation() + (GetMovementComponent()->GetLastInputVector()) + GetActorRightVector() * 41.0;

		FVector LocationPlusMovementLeft = GetActorLocation() + GetActorRightVector() * -CoverHalfWidth;
		FVector LocationPlusMovementRight = GetActorLocation() + GetActorRightVector() * CoverHalfWidth;

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

		//DrawDebugLine(GetWorld(), LocationPlusMovementLeft, LocationPlusMovementLeft + CurrentCoverDirection * CoverSenseDistance, FColor(0, 255, 0), false, 0.0f, 0, 3.0f);
		//DrawDebugLine(GetWorld(), LocationPlusMovementRight, LocationPlusMovementRight + CurrentCoverDirection * CoverSenseDistance, FColor(0, 255, 0), false, 0.0f, 0, 3.0f);

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

		if (OnEdgeLeft || OnEdgeRight)
		{
			SetActorLocation(LastFramePosition, false);
			if (Role < ROLE_Authority) ServerAdjustActorLocation(LastFramePosition);
		}

		if (OnEdgeLeft == true && OnEdgeRight == true)
		{
			if (Role == ROLE_Authority) ExitCover();
		}

		// Ray cast a second time, to see if we are close enough to the edges to pop out

		LocationPlusMovementLeft = GetActorLocation() + GetActorRightVector() * CoverHalfWidth * -1.25;
		LocationPlusMovementRight = GetActorLocation() + GetActorRightVector() * CoverHalfWidth * 1.25;

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

		if (!LeftHit.GetComponent()) EdgeAdjustedLeft = true;
		else EdgeAdjustedLeft = false;

		if (!RightHit.GetComponent()) EdgeAdjustedRight = true;
		else EdgeAdjustedRight = false;

		ClientUpdateEdges(EdgeAdjustedLeft, EdgeAdjustedRight);

		//Update LastFramePosition
		if (!OnEdgeLeft && !OnEdgeRight)
			LastFramePosition = GetActorLocation();

		if (IsCoverStandable()) CrouchedCPP = false;
		else CrouchedCPP = true;
	}
}

void AScavengerCharacter::Jump()
{
	//if (Aiming) StopAiming();
	if (Dashing) return;

	if (OnGround)
	{
		bPressedJump = true;
		JumpKeyHoldTime = 0.0f;
	}
	if (InCoverCPP) ExitCover();
}

void AScavengerCharacter::StartAiming_Implementation()
{
	//UE_LOG(LogTemp, Warning, TEXT("StartAiming"));
	
	if (Running) return;
	if (Dashing) return;

	if (!InCoverCPP)
	{
		if (MyMove)
		{
			MyMove->bOrientRotationToMovement = false;
		}
		bUseControllerRotationYaw = true;
	}
	IsAimingCPP = true;
}

void AScavengerCharacter::StopAiming_Implementation()
{
	//UE_LOG(LogTemp, Warning, TEXT("StopAiming"));
	if (!InCoverCPP)
	{
		if (MyMove)
		{
			MyMove->bOrientRotationToMovement = true;
		}
		bUseControllerRotationYaw = false;
	}

	//UE_LOG(LogTemp, Warning, TEXT("Stop Aiming (Server)!"));
	IsAimingCPP = false;
	IsPoppedOutCPP = false;
}

void AScavengerCharacter::ExitCover_Implementation()
{
	//UE_LOG(LogTemp, Warning, TEXT("ExitCover Called"));
	CrouchedCPP = false;
	IsPoppedOutCPP = false;
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
	else
	{
		//GetMovementComponent()->SetPlaneConstraintNormal(CurrentCover);
		ClientOrientRotationToMovement(true);
		GetMovementComponent()->bConstrainToPlane = false;
	}
}

bool AScavengerCharacter::IsCoverStandable()
{
	//Set up Query Parameters
	FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());

	// Ray-Cast our Grabber Ray
	FHitResult HeadHit;

	FVector HeadTest = GetActorLocation() + (GetActorUpVector() * 20.0);

	GetWorld()->LineTraceSingleByObjectType(
		HeadHit,
		HeadTest,
		HeadTest + CurrentCoverDirection * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	if (HeadHit.GetComponent()) return true;
	return false;
}

void AScavengerCharacter::EnterCover_Implementation(FVector LastMoveVector, FVector CurrentCover)
{
	CurrentCoverDirection = CurrentCover;
	if (!OnGround) return;
	LastFramePosition = GetActorLocation();
	if (IsCoverStandable()) CrouchedCPP = false;
	else CrouchedCPP = true;
	
	//UE_LOG(LogTemp, Warning, TEXT("Made it past the initial checks"));

	StartWalking();

	EnterCoverTimer = 0;
	//Set up Query Parameters
	FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());

	// Ray-Cast our Grabber Ray
	FHitResult LeftHit;
	FHitResult RightHit;

	FVector LocationPlusMovementLeft = GetActorLocation() + (LastMoveVector) + GetActorRightVector() * -CoverHalfWidth;
	FVector LocationPlusMovementRight = GetActorLocation() + (LastMoveVector) + GetActorRightVector() * CoverHalfWidth;

	GetWorld()->LineTraceSingleByObjectType(
		LeftHit,
		LocationPlusMovementLeft,
		LocationPlusMovementLeft + CurrentCover * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	GetWorld()->LineTraceSingleByObjectType(
		RightHit,
		LocationPlusMovementRight,
		LocationPlusMovementRight + CurrentCover * CoverSenseDistance,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),
		TraceParameters
	);

	//UE_LOG(LogTemp, Warning, TEXT("Cast the rays..."));

	if (LeftHit.GetComponent() && RightHit.GetComponent())
	{
		//SetActorRotation(GetActorRotation().)
		InCoverCPP = true;
		//UE_LOG(LogTemp, Warning, TEXT("Hit cover!"));
		if (MyMove)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Constraining to cover"));
			MyMove->bOrientRotationToMovement = false;
			MyMove->SetPlaneConstraintNormal(CurrentCover);
			MyMove->bConstrainToPlane = true;
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Constraining to cover all funky-like cos no MyMove"));
			ClientOrientRotationToMovement(false);
			GetMovementComponent()->SetPlaneConstraintNormal(CurrentCover);
			GetMovementComponent()->bConstrainToPlane = true;
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

void AScavengerCharacter::LocalStartAiming()
{
	if (Running) return;
	if (Dashing) return;

	if (InCoverCPP)
	{
		if (EdgeAdjustedLeft)
		{
			TargetAimOffsetAmount = -AimOffsetAmount;

			CoverFacingRightCPP = false;
			IsPoppedOutCPP = true;

			ServerSetCoverState(CoverFacingRightCPP, IsPoppedOutCPP);
			
		}
		else if (EdgeAdjustedRight)
		{
			TargetAimOffsetAmount = AimOffsetAmount;
			
			CoverFacingRightCPP = true;
			IsPoppedOutCPP = true;

			ServerSetCoverState(CoverFacingRightCPP, IsPoppedOutCPP);
		}
		else return; //Can't aim, no edges
					 //ExitCover();
	}

	if (!InCoverCPP) TargetAimOffsetAmount = AimOffsetAmount;
	TargetAimZoomDistance = AimZoomDistance;

	if (!InCoverCPP)
	{
		if (MyMove)
		{
			MyMove->bOrientRotationToMovement = false;
		}
		bUseControllerRotationYaw = true;
	}	

	if (IsPoppedOutCPP) ServerAdjustActorLocation(GetActorLocation());

	StartAiming();
}

void AScavengerCharacter::LocalStopAiming()
{
	IsPoppedOutCPP = false;

	//UE_LOG(LogTemp, Warning, TEXT("Stop Aiming (Local)!"));
	ServerSetCoverState(CoverFacingRightCPP, IsPoppedOutCPP);

	TargetAimZoomDistance = StoredAimZoomDistance;

	if (!InCoverCPP)
	{
		if (MyMove)
		{
			MyMove->bOrientRotationToMovement = true;
		}
		bUseControllerRotationYaw = false;
	}

	TargetAimOffsetAmount = 0.0f;

	StopAiming();

}

// Client to Server variable setters
void AScavengerCharacter::ServerSetAimPitch_Implementation(float NewPitch)
{
	AimPitchCPP = NewPitch;
}

void AScavengerCharacter::ServerSetAimYaw_Implementation(float NewYaw)
{
	AimYawCPP = NewYaw;
}

void AScavengerCharacter::ServerSetCoverState_Implementation(bool FacingRight, bool PoppedOut)
{
	CoverFacingRightCPP = FacingRight;
	IsPoppedOutCPP = PoppedOut;
}

void AScavengerCharacter::ServerAdjustActorLocation_Implementation(FVector NewPos)
{
	SetActorLocation(NewPos, false);
}

// Server to Client variable setters
void AScavengerCharacter::ClientUpdateWalkSpeed_Implementation(float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void AScavengerCharacter::ClientOrientRotationToMovement_Implementation(bool Incoming)
{
	if (MyMove) MyMove->bOrientRotationToMovement = Incoming;
}

void AScavengerCharacter::ClientUpdateEdges_Implementation(bool LeftEdge, bool RightEdge)
{
	EdgeAdjustedLeft = LeftEdge;
	EdgeAdjustedRight = RightEdge;
}

// Network Validation
bool AScavengerCharacter::ServerAdjustActorLocation_Validate(FVector NewPos)
{
	return true;
}

bool AScavengerCharacter::ServerSetCoverState_Validate(bool FacingRight, bool PoppedOut)
{
	return true;
}

bool AScavengerCharacter::ServerSetAimPitch_Validate(float NewPitch)
{
	return true;
}

bool AScavengerCharacter::ServerSetAimYaw_Validate(float NewYaw)
{
	return true;
}

bool AScavengerCharacter::StartRunning_Validate()
{
	return true;
}

bool AScavengerCharacter::StartWalking_Validate()
{
	return true;
}

bool AScavengerCharacter::EnterCover_Validate(FVector LastMoveVector, FVector CurrentCover)
{
	return true;
}

bool AScavengerCharacter::ExitCover_Validate()
{
	return true;
}

bool AScavengerCharacter::StartAiming_Validate()
{
	return true;
}

bool AScavengerCharacter::StopAiming_Validate()
{
	return true;
}

bool AScavengerCharacter::StartDash_Validate()
{
	return true;
}

bool AScavengerCharacter::StopDash_Validate()
{
	return true;
}

bool AScavengerCharacter::Die_Validate()
{
	return true;
}