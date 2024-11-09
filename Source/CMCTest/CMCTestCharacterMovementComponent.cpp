#include "CMCTestCharacterMovementComponent.h"
#include "GameFramework/Character.h"

void FNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character &clientMove, ENetworkMoveType moveType)
{
  Super::ClientFillNetworkMoveData(clientMove, moveType);

  auto savedMove = static_cast<const FCharacterSavedMove &>(clientMove);

  WantsToPull = savedMove.WantsToPull;
}

bool FNetworkMoveData::Serialize(
    UCharacterMovementComponent &characterMovement,
    FArchive &archive,
    UPackageMap *packageMap,
    ENetworkMoveType moveType)
{
  Super::Serialize(characterMovement, archive, packageMap, moveType);

  SerializeOptionalValue<bool>(archive.IsSaving(), archive, WantsToPull, false);

  return !archive.IsError();
}

bool FCharacterSavedMove::CanCombineWith(const FSavedMovePtr &newMove, ACharacter *inCharacter, float maxDelta) const
{
  auto newCharacterMove = static_cast<FCharacterSavedMove *>(newMove.Get());

  if (WantsToPull != newCharacterMove->WantsToPull)
  {
    return false;
  }

  return Super::CanCombineWith(newMove, inCharacter, maxDelta);
}

void FCharacterSavedMove::Clear()
{
  Super::Clear();
  WantsToPull = false;
}

void FCharacterSavedMove::SetMoveFor(
    ACharacter *character,
    float inDeltaTime,
    FVector const &newAccel,
    FNetworkPredictionData_Client_Character &clientData)
{
  Super::SetMoveFor(character, inDeltaTime, newAccel, clientData);

  auto characterMovement = Cast<UCMCTestCharacterMovementComponent>(character->GetCharacterMovement());
  WantsToPull = characterMovement->WantsToPull;
}

void FCharacterSavedMove::PrepMoveFor(ACharacter *character)
{
  Super::PrepMoveFor(character);

  auto characterMovement = Cast<UCMCTestCharacterMovementComponent>(character->GetCharacterMovement());
  characterMovement->WantsToPull = WantsToPull;
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
  Super::BeginPlay();
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
void UCMCTestCharacterMovementComponent::MoveAutonomous(float clientTimeStamp, float deltaTime, uint8 compressedFlags, const FVector &newAccel)
{
  Super::MoveAutonomous(clientTimeStamp, deltaTime, compressedFlags, newAccel);

  if (auto moveData = static_cast<FNetworkMoveData *>(GetCurrentNetworkMoveData()))
  {
    WantsToPull = moveData->WantsToPull;
  }
}

void UCMCTestCharacterMovementComponent::OnMovementUpdated(float deltaSeconds, const FVector &oldLocation, const FVector &oldVelocity)
{
  Super::OnMovementUpdated(deltaSeconds, oldLocation, oldVelocity);

  if (!IsPulling && WantsToPull)
  {
    auto rotation = CharacterOwner->GetViewRotation().Vector();
    auto traceStart = CharacterOwner->GetActorLocation() + rotation * 200.f;
    auto traceEnd = traceStart + rotation * 2000.f;
    FCollisionQueryParams queryParams;
    TArray<TEnumAsByte<EObjectTypeQuery>> traceObjectTypes;
    FHitResult hit;

    if (GetWorld()->LineTraceSingleByObjectType(hit, traceStart, traceEnd, traceObjectTypes, queryParams))
    {
      IsPulling = true;
      HitActor = hit.GetActor();
      OffsetOnActor = hit.Location - HitActor->GetActorLocation();
      PullSpeed = 0.f;
    }
  }
  else if (IsPulling && !WantsToPull)
  {
    IsPulling = false;
  }
}

void UCMCTestCharacterMovementComponent::CalcVelocity(float deltaTime, float friction, bool bFluid, float brakingDeceleration)
{
  Super::CalcVelocity(deltaTime, friction, bFluid, brakingDeceleration);

  if (IsPulling)
  {
    PullSpeed = FMath::Min(PullSpeed + PullAcceleration * deltaTime, MaxPullSpeed);

    auto ownerLocation = GetActorLocation();
    auto pullPoint = HitActor->GetActorLocation() + OffsetOnActor;
    auto distance = pullPoint - ownerLocation;
    auto direction = distance.GetSafeNormal();
    auto velocity = direction * PullSpeed;

    Launch(velocity);
  }
}