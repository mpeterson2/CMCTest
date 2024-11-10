#include "CMCTestCharacterMovementComponent.h"
#include "CMCTestCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

UCMCTestCharacterMovementComponent::UCMCTestCharacterMovementComponent(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
  SetIsReplicatedByDefault(true);

  // Tells the system to use the new packed data system
  SetNetworkMoveDataContainer(MoveDataContainer);
}

void UCMCTestCharacterMovementComponent::BeginPlay()
{
  Super::BeginPlay();
  CustomCharacter = Cast<ACMCTestCharacter>(PawnOwner);
}

#pragma region Replicated Launch

// Launching
void UCMCTestCharacterMovementComponent::LaunchCharacterReplicated(FVector NewLaunchVelocity, bool bXYOverride, bool bZOverride)
{
  // Only launch if our custom character exists, otherwise exit function.
  if (!CustomCharacter)
  {
    return;
  }

  FVector FinalVel = NewLaunchVelocity;

  if (!bXYOverride)
  {
    FinalVel.X += Velocity.X;
    FinalVel.Y += Velocity.Y;
  }
  if (!bZOverride)
  {
    FinalVel.Z += LastUpdateVelocity.Z;
  }

  LaunchVelocityCustom = FinalVel;

  // This isn't where the launch occurs, it is a blueprint implementable event for additional BP logic. See the declaration for more info.
  // The launch will occur next frame in this setup when PendingLaunchVelocity is handled by HandlePendingLaunch() during the PerformMovement() update.
  // If you need special logic, you can always override HandlePendingLaunch(), which we have already done.
  CustomCharacter->OnLaunched(NewLaunchVelocity, bXYOverride, bZOverride);
}

void UCMCTestCharacterMovementComponent::StartLaunching()
{
  bWantsToLaunch = true;
}

void UCMCTestCharacterMovementComponent::StopLaunching()
{
  bWantsToLaunch = false;
}

bool UCMCTestCharacterMovementComponent::HandlePendingLaunch()
{
  if (!PendingLaunchVelocity.IsZero() && HasValidData())
  {
    Velocity = PendingLaunchVelocity;
    SetMovementMode(MOVE_Falling);
    PendingLaunchVelocity = FVector::ZeroVector;
    bForceNextFloorCheck = true;
    return true;
  }

  return false;
}

#pragma endregion

#pragma region Custom Movement

// Movement Flag Manipulation//
void UCMCTestCharacterMovementComponent::ActivateMovementFlag(uint8 FlagToActivate)
{
  MovementFlagCustom |= FlagToActivate;
}

void UCMCTestCharacterMovementComponent::ClearMovementFlag(uint8 FlagToClear)
{
  MovementFlagCustom &= ~FlagToClear;
}

#pragma endregion

#pragma region Movement Mode Switching

// Called on tick, can be used for setting values and movement modes for next tick.
void UCMCTestCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector &OldLocation, const FVector &OldVelocity)
{
  Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

  if (bWantsToLaunch)
  {
    auto velocity = GetCharacterOwner()->GetViewRotation().Vector() * 1000;
    LaunchCharacterReplicated(velocity, true, true);
  }

  /*
   * This is where the launch value will be set for the next tick. Both the client and server run this code, which is why it is important that our LaunchVelocityCustom is tracked in our net code.
   * However, it is an UNSAFE variable. We must sanity check this logic when the time comes to execute it, based on our game's mechanics.
   * We can do these checks in HandlePendingLaunch.
   */

  if ((MovementMode != MOVE_None) && IsActive() && HasValidData())
  {
    PendingLaunchVelocity = LaunchVelocityCustom;
    LaunchVelocityCustom = FVector(0.f, 0.f, 0.f);
  }
}

#pragma endregion

/////BEGIN Networking/////
#pragma region Networked Movement
/*
* General workflow adapted from SNM1 and SMN2 by Reddy-dev.
* The explanation below comes from SMN1.
* Copyright (c) 2021 Reddy-dev
*@Documentation Extending Saved Move Data
To add new data, first extend FSavedMove_Character to include whatever information your Character Movement Component needs. Next, extend FCharacterNetworkMoveData and add the custom data you want to send across the network; in most cases, this mirrors the data added to FSavedMove_Character. You will also need to extend FCharacterNetworkMoveDataContainer so that it can serialize your FCharacterNetworkMoveData for network transmission, and deserialize it upon receipt. When this setup is finished, configure the system as follows:
Modify your Character Movement Component to use the FCharacterNetworkMoveDataContainer subclass you created with the SetNetworkMoveDataContainer function. The simplest way to accomplish this is to add an instance of your FCharacterNetworkMoveDataContainer to your Character Movement Component child class, and call SetNetworkMoveDataContainer from the constructor.
Since your FCharacterNetworkMoveDataContainer needs its own instances of FCharacterNetworkMoveData, point it (typically in the constructor) to instances of your FCharacterNetworkMoveData subclass. See the base constructor for more details and an example.
In your extended version of FCharacterNetworkMoveData, override the ClientFillNetworkMoveData function to copy or compute data from the saved move. Override the Serialize function to read and write your data using an FArchive; this is the bit stream that RPCs require.
To extend the server response to clients, which can acknowledges a good move or send correction data, extend FCharacterMoveResponseData, FCharacterMoveResponseDataContainer, and override your Character Movement Component's version of the SetMoveResponseDataContainer.
*/
// Receives moves from Serialize
void UCMCTestCharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector &NewAccel)
{
  FCustomNetworkMoveData *CurrentMoveData = static_cast<FCustomNetworkMoveData *>(GetCurrentNetworkMoveData());
  if (CurrentMoveData != nullptr)
  {

    bWantsToLaunch = CurrentMoveData->bWantsToLaunchMoveData;

    LaunchVelocityCustom = CurrentMoveData->LaunchVelocityCustomMoveData;

    MovementFlagCustom = CurrentMoveData->MovementFlagCustomMoveData;
  }
  Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

// Sends the Movement Data
bool FCustomNetworkMoveData::Serialize(UCharacterMovementComponent &CharacterMovement, FArchive &Ar, UPackageMap *PackageMap, ENetworkMoveType MoveType)
{
  Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);

  SerializeOptionalValue<bool>(Ar.IsSaving(), Ar, bWantsToLaunchMoveData, false);
  SerializeOptionalValue<FVector>(Ar.IsSaving(), Ar, LaunchVelocityCustomMoveData, FVector(0.f, 0.f, 0.f));

  SerializeOptionalValue<uint8>(Ar.IsSaving(), Ar, MovementFlagCustomMoveData, 0);

  return !Ar.IsError();
}

void FCustomNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character &ClientMove, ENetworkMoveType MoveType)
{
  Super::ClientFillNetworkMoveData(ClientMove, MoveType);

  const FCustomSavedMove &CurrentSavedMove = static_cast<const FCustomSavedMove &>(ClientMove);

  bWantsToLaunchMoveData = CurrentSavedMove.bWantsToLaunchSaved;
  LaunchVelocityCustomMoveData = CurrentSavedMove.SavedLaunchVelocityCustom;
}

// Combines Flags together as an optimization option by the engine to send less data over the network
bool FCustomSavedMove::CanCombineWith(const FSavedMovePtr &NewMove, ACharacter *Character, float MaxDelta) const
{
  FCustomSavedMove *NewMovePtr = static_cast<FCustomSavedMove *>(NewMove.Get());

  if (bWantsToLaunchSaved != NewMovePtr->bWantsToLaunchSaved)
  {
    return false;
  }

  if (SavedLaunchVelocityCustom != NewMovePtr->SavedLaunchVelocityCustom)
  {
    return false;
  }

  return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

// Saves Move before Using
void FCustomSavedMove::SetMoveFor(ACharacter *Character, float InDeltaTime, FVector const &NewAccel, FNetworkPredictionData_Client_Character &ClientData)
{
  Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

  // This is where you set the saved move in case a packet is dropped containing this to minimize corrections
  UCMCTestCharacterMovementComponent *CharacterMovement = Cast<UCMCTestCharacterMovementComponent>(Character->GetCharacterMovement());
  if (CharacterMovement)
  {
    bWantsToLaunchSaved = CharacterMovement->bWantsToLaunch;

    SavedLaunchVelocityCustom = CharacterMovement->LaunchVelocityCustom;
  }
}

// This is called usually when a packet is dropped and resets the compressed flag to its saved state
void FCustomSavedMove::PrepMoveFor(ACharacter *Character)
{
  Super::PrepMoveFor(Character);

  UCMCTestCharacterMovementComponent *CharacterMovementComponent = Cast<UCMCTestCharacterMovementComponent>(Character->GetCharacterMovement());
  if (CharacterMovementComponent)
  {
    CharacterMovementComponent->bWantsToLaunch = bWantsToLaunchSaved;

    CharacterMovementComponent->LaunchVelocityCustom = SavedLaunchVelocityCustom;
  }
}

// Just used to reset the data in a saved move.
void FCustomSavedMove::Clear()
{
  Super::Clear();

  bWantsToLaunchSaved = false;
  SavedLaunchVelocityCustom = FVector(0.f, 0.f, 0.f);
}

// Acquires prediction data from clients (boilerplate code)
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

// An older function used more in versions of Unreal prior to the introduction of Packed Movement Data (FCharacterNetworkMoveData).
// Can still be used for unpacking additional compressed flags within CustomSavedMove.
uint8 FCustomSavedMove::GetCompressedFlags() const
{

  return Super::GetCompressedFlags();
}

// Default constructor for FCustomNetworkPredictionData_Client. It's usually not necessary to populate this function.
FCustomNetworkPredictionData_Client::FCustomNetworkPredictionData_Client(const UCharacterMovementComponent &ClientMovement) : Super(ClientMovement)
{
}

// Generates a new saved move that will be populated and used by the system.
FSavedMovePtr FCustomNetworkPredictionData_Client::AllocateNewMove()
{
  return FSavedMovePtr(new FCustomSavedMove());
}

// The Flags parameter contains the compressed input flags that are stored in the parent saved move.
// UpdateFromCompressed flags simply copies the flags from the saved move into the movement component.
// It basically just resets the movement component to the state when the move was made so it can simulate from there.
// We use ClientFillNetworkMoveData instead of this function, due to the limitation of the original flags system.
void UCMCTestCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
  Super::UpdateFromCompressedFlags(Flags);
}

/*
 * Another boilerplate function that simply allows you to specify your custom move data class.
 */
FCustomCharacterNetworkMoveDataContainer::FCustomCharacterNetworkMoveDataContainer()
{
  NewMoveData = &CustomDefaultMoveData[0];
  PendingMoveData = &CustomDefaultMoveData[1];
  OldMoveData = &CustomDefaultMoveData[2];
}

#pragma endregion
/////END Networking/////