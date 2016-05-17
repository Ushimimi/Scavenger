// Fill out your copyright notice in the Description page of Project Settings.

#include "Scavenger.h"
#include "ScavengerController.h"

#define OUT

// Sets default values for this component's properties
UScavengerController::UScavengerController()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	Owner = GetOwner();
}


// Called when the game starts
void UScavengerController::BeginPlay()
{
	Super::BeginPlay();

	// ...
	FindInputComponent();
}


// Called every frame
void UScavengerController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UScavengerController::StartWalking()
{
	UE_LOG(LogTemp, Warning, TEXT("Begin walking."));
	if (CMComponent) CMComponent->MaxWalkSpeed = WalkingSpeed;
}

void UScavengerController::StartRunning()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop walking."));
	if (CMComponent) CMComponent->MaxWalkSpeed = RunningSpeed;
}

void UScavengerController::CoverButton()
{
	if (IsInCover) IsInCover = false;
	else IsInCover = true;
}

void UScavengerController::EnterCover()
{

}

void UScavengerController::LeaveCover()
{

}

bool UScavengerController::GetCoverState()
{
	return IsInCover;
}

bool UScavengerController::GetIsCrouching()
{
	return IsCrouching;
}

void UScavengerController::FindInputComponent()
{
	// Find attached InputComponent
	InputComponent = Owner->FindComponentByClass<UInputComponent>();
	if (InputComponent)
	{
		//Walk Button
		InputComponent->BindAction("Walk", IE_Pressed, this, &UScavengerController::StartWalking);
		InputComponent->BindAction("Walk", IE_Released, this, &UScavengerController::StartRunning);

		//Cover Button
		InputComponent->BindAction("TakeCover", IE_Pressed, this, &UScavengerController::CoverButton);
	}

	CMComponent = Owner->FindComponentByClass<UCharacterMovementComponent>();
	if (!CMComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s needs a UCharacterMovementComponent!"), *Owner->GetName());
		this->DestroyComponent();
	}
}

