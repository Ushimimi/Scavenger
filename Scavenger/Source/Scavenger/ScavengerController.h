// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "ScavengerController.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SCAVENGER_API UScavengerController : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UScavengerController();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UFUNCTION(BlueprintPure, Category="Character")
	bool GetCoverState();

	UFUNCTION(BlueprintPure, Category = "Character")
	bool GetIsCrouching();
	
private:
	// Stats for movement, speed, aiming, etc
	UPROPERTY(EditAnywhere)
	float WalkingSpeed = 300.0; // Static! Do not change during gameplay

	UPROPERTY(EditAnywhere)
	float RunningSpeed = 600.0; // Static! Do not change during gameplay
	
	UPROPERTY(EditAnywhere)
	bool CanTakeCover = true; // Static! Do not change during gameplay

	UPROPERTY(EditAnywhere)
	float CoverInteractRange = 100.0; // Range of Take Cover ability

	UPROPERTY(EditAnywhere)
	float InteractRange = 100.0; // Range of Interact ability

	// Pointers to game components
	AActor* Owner = nullptr;
	UInputComponent* InputComponent = nullptr;
	UCharacterMovementComponent* CMComponent = nullptr;

	// State tracking
	bool IsInCover = false;
	bool IsCrouching = false;
	
	// Methods for instance setup
	void FindInputComponent();

	// Methods to switch between run speed and walk speed
	virtual void StartWalking();
	virtual void StartRunning();

	// Methods to enter and leave cover, and base input handler
	virtual void CoverButton();
	virtual void EnterCover();
	virtual void LeaveCover();
};
