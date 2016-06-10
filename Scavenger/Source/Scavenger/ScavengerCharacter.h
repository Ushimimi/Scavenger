// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

#include <gun.h>

#include "ScavengerCharacter.generated.h"

UCLASS(Config = game, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class AScavengerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	AScavengerCharacter();

	// Tick method declaration
	virtual void Tick(float DeltaTime);
	virtual void BeginPlay();

	// Jump override to fix buggy UE code
	virtual void Jump() override;

	//virtual void Crouch();
	virtual void EnterCover();

	//UFUNCTION(BlueprintCallable, Category = "Pawn|Character")
	//bool IsInCover(); // Getter for cover state

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UFUNCTION()
	virtual void OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	float AngleBetween(FVector a, FVector b);

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category=Custom)
	bool InCoverCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	bool CrouchedCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	bool CoverFacingRightCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	bool IsDashingCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	bool IsPoppedOutCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	bool IsAimingCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	bool IsDeadCPP = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	float AimPitchCPP = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Custom)
	float AimYawCPP = 0.0;

	// BP Editor Objects
	
	// Weapon to spawn with
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BP Classes")
	UClass* WeaponBPClass;

	// Object Pointers

	AGun* EquippedWeapon;

private:
	bool Running = false;
	bool RunKeyPressed = false;
	bool Dashing = false;
	bool Aiming = false;
	bool OnGround = false;
	bool OnEdgeLeft = false;
	bool OnEdgeRight = false;
	bool EdgeAdjustedLeft = false;
	bool EdgeAdjustedRight = false;

	FVector CurrentCoverDirection;
	FVector LastFramePosition;
	FVector DashDirection;

	float RunSpeed = 0.0;
	float WalkSpeed = 0.0;

	int EnterCoverTimer = 0;
	int DashTimer = 0;
	int DashCooldownTimer = 0;

	// Number of frames to dash, when dash is executed
	UPROPERTY(EditAnywhere)
	int DashDuration = 30;

	// Number of frames before dash can be re-executed
	UPROPERTY(EditAnywhere)
	int DashCooldown = 60;

	// Amount of force to add when dashing
	UPROPERTY(EditAnywhere)
	float DashForce = 2.0;

	// Time required in frames before cover is entered or exited
	UPROPERTY(EditAnywhere)
	int EnterCoverHoldTime = 10;

	// Range of the ray traces that check for cover
	UPROPERTY(EditAnywhere)
	float CoverSenseDistance = 60.0;

	// Half of the width of the cover-sensing rays (distance from player location to scan right or left against the wall
	UPROPERTY(EditAnywhere)
	int CoverHalfWidth = 30;

	// Multiplier for running speed
	UPROPERTY(EditAnywhere)
	float RunMultiplier = 2.0;

	// Max dashing speed
	UPROPERTY(EditAnywhere)
	float DashSpeed = 600.0;

	//Camera Boom distance when aiming
	UPROPERTY(EditAnywhere)
	float AimZoomDistance = 150.0;

	//Amount of offset to apply (reversed for left side) when aiming
	UPROPERTY(EditAnywhere)
	float AimOffsetAmount = 50.0;

	float TargetAimOffsetAmount = 0.0;
	float CameraTrackSpeed = 8.0;

	float StoredAimZoomDistance = 300.0;
	float TargetAimZoomDistance = 0.0;

	UPROPERTY(EditAnywhere)
	int MaxCoverAngle = 30; // The angle at which movement into a cover wall will result in entering cover

	UCapsuleComponent* MyCapsule = nullptr;
	UCharacterMovementComponent* MyMove = nullptr;

	void StickToCover();
	bool CheckIsMovementAllowed(FVector Direction, float Value);
	bool IsCoverStandable();

	void UpdateCamera();
	void UpdateAiming();

	void Die();


protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	void StartRunning();
	void StartWalking();

	void ExitCover();
	void StartAiming();
	void StopAiming();
	void StartDash();
	void StopDash();
	void ExecuteDash();

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

