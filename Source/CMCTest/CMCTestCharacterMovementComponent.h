#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CMCTestCharacterMovementComponent.generated.h"

class FCustomSavedMove : public FSavedMove_Character
{
  typedef FSavedMove_Character Super;

public:
  bool WantsToPull;

  virtual bool CanCombineWith(const FSavedMovePtr &newMove, ACharacter *character, float maxDelta) const override;
  virtual void Clear() override;
  virtual void SetMoveFor(
      ACharacter *character,
      float inDeltaTime,
      FVector const &newAccel,
      FNetworkPredictionData_Client_Character &clientData) override;
  virtual void PrepMoveFor(class ACharacter *character) override;
};

class FCustomNetworkMoveData : public FCharacterNetworkMoveData
{
public:
  typedef FCharacterNetworkMoveData Super;

  bool WantsToPull;

  virtual void ClientFillNetworkMoveData(const FSavedMove_Character &clientMove, ENetworkMoveType moveType) override;
  virtual bool Serialize(
      UCharacterMovementComponent &characterMovement,
      FArchive &archive,
      UPackageMap *packageMap,
      ENetworkMoveType moveType) override;
};

class FCustomCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
public:
  FCustomCharacterNetworkMoveDataContainer();
  FCustomNetworkMoveData MoveData[3];
};

class FCustomNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
public:
  FCustomNetworkPredictionData_Client(const UCharacterMovementComponent &clientMovement);
  typedef FNetworkPredictionData_Client_Character Super;
  virtual FSavedMovePtr AllocateNewMove() override;
};

UCLASS()
class UCMCTestCharacterMovementComponent : public UCharacterMovementComponent
{
  GENERATED_BODY()
protected:
  FCustomCharacterNetworkMoveDataContainer MoveDataContainer;

public:
  UCMCTestCharacterMovementComponent(const FObjectInitializer &objectInitializer);
  virtual void BeginPlay() override;
  virtual FNetworkPredictionData_Client *GetPredictionData_Client() const override;
  virtual void MoveAutonomous(float clientTimeStamp, float deltaTime, uint8 compressedFlags, const FVector &newAccel)
      override;
  virtual void OnMovementUpdated(float deltaSeconds, const FVector &oldLocation, const FVector &oldVelocity) override;

  bool WantsToPull;

  bool IsPulling;
  AActor *HitActor;
  FVector OffsetOnActor;
  float PullSpeed;
  float MaxPullSpeed = 2000;
  float PullAcceleration = 4000;
};
