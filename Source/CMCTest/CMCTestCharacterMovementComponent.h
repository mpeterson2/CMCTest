#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CMCTestCharacterMovementComponent.generated.h"

class ACMCTestCharacter;

class FCustomNetworkMoveData : public FCharacterNetworkMoveData
{
public:
  typedef FCharacterNetworkMoveData Super;

  virtual void ClientFillNetworkMoveData(const FSavedMove_Character &ClientMove, ENetworkMoveType MoveType) override;

  virtual bool Serialize(UCharacterMovementComponent &CharacterMovement, FArchive &Ar, UPackageMap *PackageMap, ENetworkMoveType MoveType) override;

  bool bWantsToLaunchMoveData = false;
  FVector LaunchVelocityCustomMoveData = FVector(0.f, 0.f, 0.f);
};

class FCustomCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{

public:
  FCustomCharacterNetworkMoveDataContainer();

  FCustomNetworkMoveData CustomDefaultMoveData[3];
};

class FCustomSavedMove : public FSavedMove_Character
{
public:
  typedef FSavedMove_Character Super;

  bool bWantsToLaunchSaved = false;
  FVector SavedLaunchVelocityCustom = FVector(0.f, 0.f, 0.f);

  virtual bool CanCombineWith(const FSavedMovePtr &NewMove, ACharacter *Character, float MaxDelta) const override;
  virtual void SetMoveFor(ACharacter *Character, float InDeltaTime, FVector const &NewAccel, class FNetworkPredictionData_Client_Character &ClientData) override;
  virtual void PrepMoveFor(class ACharacter *Character) override;
  virtual void Clear() override;
};

class FCustomNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
public:
  FCustomNetworkPredictionData_Client(const UCharacterMovementComponent &ClientMovement);
  typedef FNetworkPredictionData_Client_Character Super;
  virtual FSavedMovePtr AllocateNewMove() override;
};

UCLASS()
class UCMCTestCharacterMovementComponent : public UCharacterMovementComponent
{
  GENERATED_BODY()

public:
  UCMCTestCharacterMovementComponent(const FObjectInitializer &ObjectInitializer);

  friend class FCustomSavedMove;

  bool bWantsToLaunch;

public:
  UFUNCTION(BlueprintCallable)
  void StartLaunching();
  UFUNCTION(BlueprintCallable)
  void StopLaunching();

  void LaunchCharacterReplicated(FVector NewLaunchVelocity, bool bXYOverride, bool bZOverride);
  FVector LaunchVelocityCustom;

  virtual bool HandlePendingLaunch() override;

protected:
  UPROPERTY(Transient, DuplicateTransient)
  TObjectPtr<ACMCTestCharacter> CustomCharacter;

public:
  virtual void BeginPlay() override;
  virtual void OnMovementUpdated(float DeltaSeconds, const FVector &OldLocation, const FVector &OldVelocity) override;

public:
  FCustomCharacterNetworkMoveDataContainer MoveDataContainer;

  virtual void UpdateFromCompressedFlags(uint8 Flags) override;
  virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector &NewAccel) override;

  virtual class FNetworkPredictionData_Client *GetPredictionData_Client() const override;
};
