#include "CMCTestCharacterMovementComponent.h"
#include "GameFramework/Character.h"

void FNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character &clientMove, ENetworkMoveType moveType)
{
  Super::ClientFillNetworkMoveData(clientMove, moveType);

  auto savedMove = static_cast<const FCharacterSavedMove &>(clientMove);
}

bool FNetworkMoveData::Serialize(
    UCharacterMovementComponent &characterMovement,
    FArchive &archive,
    UPackageMap *packageMap,
    ENetworkMoveType moveType)
{
  Super::Serialize(characterMovement, archive, packageMap, moveType);

  return !archive.IsError();
}

bool FCharacterSavedMove::CanCombineWith(const FSavedMovePtr &newMove, ACharacter *inCharacter, float maxDelta) const
{
  auto newCharacterMove = static_cast<FCharacterSavedMove *>(newMove.Get());
  return Super::CanCombineWith(newMove, inCharacter, maxDelta);
}

void FCharacterSavedMove::Clear()
{
  Super::Clear();
}

void FCharacterSavedMove::SetMoveFor(
    ACharacter *character,
    float inDeltaTime,
    FVector const &newAccel,
    FNetworkPredictionData_Client_Character &clientData)
{
  Super::SetMoveFor(character, inDeltaTime, newAccel, clientData);

  auto characterMovement = Cast<UCMCTestCharacterMovementComponent>(character->GetCharacterMovement());
}

void FCharacterSavedMove::PrepMoveFor(ACharacter *character)
{
  Super::PrepMoveFor(character);

  auto characterMovement = Cast<UCMCTestCharacterMovementComponent>(character->GetCharacterMovement());
}

FNetworkMoveDataContainer::FNetworkMoveDataContainer()
{
  NewMoveData = &MoveData[0];
  PendingMoveData = &MoveData[1];
  OldMoveData = &MoveData[2];
}

FCharacterPredictionData::FCharacterPredictionData(const UCharacterMovementComponent &ClientMovement)
    : Super(ClientMovement)
{
  MaxSmoothNetUpdateDist = 92.f;
  NoSmoothNetUpdateDist = 140.f;
}

FSavedMovePtr FCharacterPredictionData::AllocateNewMove()
{
  return FSavedMovePtr(new FCharacterSavedMove());
}

UCMCTestCharacterMovementComponent::UCMCTestCharacterMovementComponent(const FObjectInitializer &objectInitializer)
    : Super(objectInitializer)
{
  SetIsReplicatedByDefault(true);
  SetNetworkMoveDataContainer(MoveDataContainer);
}

void UCMCTestCharacterMovementComponent::BeginPlay()
{
}

FNetworkPredictionData_Client *UCMCTestCharacterMovementComponent::GetPredictionData_Client() const
{
  if (ClientPredictionData == nullptr)
  {
    auto mutableThis = const_cast<UCMCTestCharacterMovementComponent *>(this);
    mutableThis->ClientPredictionData = new FCharacterPredictionData(*this);
  }

  return ClientPredictionData;
}