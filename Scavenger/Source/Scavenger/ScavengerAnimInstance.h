// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Animation/AnimInstance.h"
#include "ScavengerAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class SCAVENGER_API UScavengerAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = State)
	bool IsInCover = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = State)
	bool IsCrouched = false;
	*/
	
};