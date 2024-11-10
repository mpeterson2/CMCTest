#include "CMCTestCharacterMovementComponent.h"
#include "CMCTestCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

UCMCTestCharacterMovementComponent::UCMCTestCharacterMovementComponent(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
  SetIsReplicatedByDefault(true);
  SetNetworkMoveDataContainer(MoveDataContainer);
}

void UCMCTestCharacterMovementComponent::BeginPlay()
{
  Super::BeginPlay();
  CustomCharacter = Cast<ACMCTestCharacter>(PawnOwner);
}

void UCMCTestCharacterMovementComponent::StartLaunching()
{
  bWantsToLaunch = true;
}

void UCMCTestCharacterMovementComponent::StopLaunching()
{
  bWantsToLaunch = false;
}

void UCMCTestCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector &OldLocation, const FVector &OldVelocity)
{
  Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

  if (bWantsToLaunch)
  {
    auto velocity = GetCharacterOwner()->GetViewRotation().Vector() * 1000;
    Launch(velocity);
  }
}

void UCMCTestCharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector &NewAccel)
{
  FCustomNetworkMoveData *CurrentMoveData = static_cast<FCustomNetworkMoveData *>(GetCurrentNetworkMoveData());
  if (CurrentMoveData != nullptr)
  {
    bWantsToLaunch = CurrentMoveData->bWantsToLaunchMoveData;
  }
  Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

bool FCustomNetworkMoveData::Serialize(UCharacterMovementComponent &CharacterMovement, FArchive &Ar, UPackageMap *PackageMap, ENetworkMoveType MoveType)
{
  Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);

  SerializeOptionalValue<bool>(Ar.IsSaving(), Ar, bWantsToLaunchMoveData, false);

  return !Ar.IsError();
}

void FCustomNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character &ClientMove, ENetworkMoveType MoveType)
{
  Super::ClientFillNetworkMoveData(ClientMove, MoveType);

  const FCustomSavedMove &CurrentSavedMove = static_cast<const FCustomSavedMove &>(ClientMove);

  bWantsToLaunchMoveData = CurrentSavedMove.bWantsToLaunchSaved;
}

bool FCustomSavedMove::CanCombineWith(const FSavedMovePtr &NewMove, ACharacter *Character, float MaxDelta) const
{
  FCustomSavedMove *NewMovePtr = static_cast<FCustomSavedMove *>(NewMove.Get());

  if (bWantsToLaunchSaved != NewMovePtr->bWantsToLaunchSaved)
  {
    return false;
  }

  return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FCustomSavedMove::SetMoveFor(ACharacter *Character, float InDeltaTime, FVector const &NewAccel, FNetworkPredictionData_Client_Character &ClientData)
{
  Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

  UCMCTestCharacterMovementComponent *CharacterMovement = Cast<UCMCTestCharacterMovementComponent>(Character->GetCharacterMovement());
  if (CharacterMovement)
  {
    bWantsToLaunchSaved = CharacterMovement->bWantsToLaunch;
  }
}

void FCustomSavedMove::PrepMoveFor(ACharacter *Character)
{
  Super::PrepMoveFor(Character);

  UCMCTestCharacterMovementComponent *CharacterMovementComponent = Cast<UCMCTestCharacterMovementComponent>(Character->GetCharacterMovement());
  if (CharacterMovementComponent)
  {
    CharacterMovementComponent->bWantsToLaunch = bWantsToLaunchSaved;
  }
}

void FCustomSavedMove::Clear()
{
  Super::Clear();

  bWantsToLaunchSaved = false;
}

FNetworkPredictionData_Client *UCMCTestCharacterMovementComponent::GetPredictionData_Client() const
{
  check(PawnOwner != NULL);

  if (!ClientPredictionData)
  {
    UCMCTestCharacterMovementComponent *MutableThis = const_cast<UCMCTestCharacterMovementComponent *>(this);

    MutableThis->ClientPredictionData = new FCustomNetworkPredictionData_Client(*this);
  }

  return ClientPredictionData;
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

void UCMCTestCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
  Super::UpdateFromCompressedFlags(Flags);
}

FCustomCharacterNetworkMoveDataContainer::FCustomCharacterNetworkMoveDataContainer()
{
  NewMoveData = &CustomDefaultMoveData[0];
  PendingMoveData = &CustomDefaultMoveData[1];
  OldMoveData = &CustomDefaultMoveData[2];
}