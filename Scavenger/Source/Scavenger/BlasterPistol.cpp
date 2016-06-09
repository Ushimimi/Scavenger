// Fill out your copyright notice in the Description page of Project Settings.

#include "Scavenger.h"
#include "BlasterPistol.h"


// Sets default values
ABlasterPistol::ABlasterPistol()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABlasterPistol::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABlasterPistol::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

