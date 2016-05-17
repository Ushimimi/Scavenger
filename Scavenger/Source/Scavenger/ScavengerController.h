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

private:
	// Stats for movement, speed, aiming, etc
	UPROPERTY(EditAnywhere)
	float WalkingSpeed = 300.0;

	UPROPERTY(EditAnywhere)
	float RunningSpeed = 600.0;
	
	// Pointers to game components
	AActor* Owner = nullptr;
	UInputComponent* InputComponent = nullptr;
	UCharacterMovementComponent* CMComponent = nullptr;
	
	// Methods for instance setup
	void FindInputComponent();

	// Methods to switch between run speed and walk speed
	virtual void StartWalking();
	virtual void StartRunning();
};
