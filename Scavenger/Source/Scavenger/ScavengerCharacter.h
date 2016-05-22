// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

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

private:
	bool Running = false;
	bool OnGround = false;
	bool OnEdgeLeft = false;
	bool OnEdgeRight = false;

	FVector CurrentCoverDirection;

	float RunSpeed = 0.0;
	float WalkSpeed = 0.0;

	UPROPERTY(EditAnywhere)
	float RunMultiplier = 2.0;

	UPROPERTY(EditAnywhere)
	int MaxCoverAngle = 30; // The angle at which movement into a cover wall will result in entering cover

	UCapsuleComponent* MyCapsule = nullptr;
	UCharacterMovementComponent* MyMove = nullptr;

	void StickToCover();

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

