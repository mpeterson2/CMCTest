#include "CMCTestCharacterMovementComponent.h"
#include "GameFramework/Character.h"

void FCustomNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character &clientMove, ENetworkMoveType moveType)
{
  Super::ClientFillNetworkMoveData(clientMove, moveType);

  auto savedMove = static_cast<const FCustomSavedMove &>(clientMove);

  bWantsToLaunchMoveData = savedMove.bWantsToLaunchSaved;
}

bool FCustomNetworkMoveData::Serialize(
    UCharacterMovementComponent &characterMovement,
    FArchive &archive,
    UPackageMap *packageMap,
    ENetworkMoveType moveType)
{
  Super::Serialize(characterMovement, archive, packageMap, moveType);

  SerializeOptionalValue<bool>(archive.IsSaving(), archive, bWantsToLaunchMoveData, false);

  return !archive.IsError();
}

bool FCustomSavedMove::CanCombineWith(const FSavedMovePtr &newMove, ACharacter *character, float maxDelta) const
{
  auto newCharacterMove = static_cast<FCustomSavedMove *>(newMove.Get());

  if (bWantsToLaunchSaved != newCharacterMove->bWantsToLaunchSaved)
  {
    return false;
  }

  return Super::CanCombineWith(newMove, character, maxDelta);
}

void FCustomSavedMove::Clear()
{
  Super::Clear();

  bWantsToLaunchSaved = false;
}

void FCustomSavedMove::SetMoveFor(
    ACharacter *character,
    float inDeltaTime,
    FVector const &newAccel,
    FNetworkPredictionData_Client_Character &clientData)
{
  Super::SetMoveFor(character, inDeltaTime, newAccel, clientData);

  auto characterMovement = Cast<UCMCTestCharacterMovementComponent>(character->GetCharacterMovement());
  if (characterMovement)
  {
    bWantsToLaunchSaved = characterMovement->bWantsToLaunch;
  }
}

void FCustomSavedMove::PrepMoveFor(ACharacter *character)
{
  Super::PrepMoveFor(character);

  auto characterMovement = Cast<UCMCTestCharacterMovementComponent>(character->GetCharacterMovement());
  characterMovement->bWantsToLaunch = bWantsToLaunchSaved;
}

FCustomCharacterNetworkMoveDataContainer::FCustomCharacterNetworkMoveDataContainer()
{
  NewMoveData = &MoveData[0];
  PendingMoveData = &MoveData[1];
  OldMoveData = &MoveData[2];
}

FCustomNetworkPredictionData_Client::FCustomNetworkPredictionData_Client(const UCharacterMovementComponent &ClientMovement) : Super(ClientMovement)
{
  MaxSmoothNetUpdateDist = 92.f;
  NoSmoothNetUpdateDist = 140.f;
}

FSavedMovePtr FCustomNetworkPredictionData_Client::AllocateNewMove()
{
  return FSavedMovePtr(new FCustomSavedMove());
}

UCMCTestCharacterMovementComponent::UCMCTestCharacterMovementComponent(const FObjectInitializer &objectInitializer) : Super(objectInitializer)
{
  SetIsReplicatedByDefault(true);
  SetNetworkMoveDataContainer(MoveDataContainer);
}

void UCMCTestCharacterMovementComponent::BeginPlay()
{
  Super::BeginPlay();
}

FNetworkPredictionData_Client *UCMCTestCharacterMovementComponent::GetPredictionData_Client() const
{
  if (ClientPredictionData == nullptr)
  {
    UCMCTestCharacterMovementComponent *mutableThis = const_cast<UCMCTestCharacterMovementComponent *>(this);
    mutableThis->ClientPredictionData = new FCustomNetworkPredictionData_Client(*this);
  }

  return ClientPredictionData;
}

void UCMCTestCharacterMovementComponent::MoveAutonomous(float clientTimeStamp, float deltaTime, uint8 compressedFlags, const FVector &newAccel)
{
  if (auto moveData = static_cast<FCustomNetworkMoveData *>(GetCurrentNetworkMoveData()))
  {
    bWantsToLaunch = moveData->bWantsToLaunchMoveData;
  }

  Super::MoveAutonomous(clientTimeStamp, deltaTime, compressedFlags, newAccel);
}

void UCMCTestCharacterMovementComponent::OnMovementUpdated(float deltaSeconds, const FVector &oldLocation, const FVector &oldVelocity)
{
  Super::OnMovementUpdated(deltaSeconds, oldLocation, oldVelocity);

  if (bWantsToLaunch)
  {
    auto velocity = GetCharacterOwner()->GetViewRotation().Vector() * 1000;
    Launch(velocity);
  }
}

void UCMCTestCharacterMovementComponent::StartLaunching()
{
  bWantsToLaunch = true;
}

void UCMCTestCharacterMovementComponent::StopLaunching()
{
  bWantsToLaunch = false;
}
