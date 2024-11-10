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

/*Here we can create custom launch logic based on a pending launch value.
 * Remember, our launch value in this instance is UNSAFE.
 * Before performing the launch, we should sanity check the data.
 * This depends on your game. Do you have a cooldown timer? Do you have required tags? Is there a max or min launch value?
 * The server might not run this code despite the player performing the move locally. This will result in a pretty significant correction, but it's better than allowing cheaters to take over.
 * The default code (from the parent function) is placed here for you to tweak.
 *
 * Tip: Launching isn't a consistent way to apply a "boop" as ground friction and air friction are different by default.
 * A launch event will inherently have a larger impact while falling or flying and have almost no effect on a grounded entity.
 * To achieve the same launch effect in both movement modes, you will need to either momentarily change frictional values to align for a small while or a similar workaround.
 * Alternatively, you can edit how and when friction/deceleration/braking force is applied based on an ongoing launch event.
 */
bool UCMCTestCharacterMovementComponent::HandlePendingLaunch()
{
  // IF: LaunchIsValidBasedOnMyProjectRequirementsAndBespokeNeeds()
  // THEN: Run the code below

  if (!PendingLaunchVelocity.IsZero() && HasValidData())
  {
    Velocity = PendingLaunchVelocity;
    SetMovementMode(MOVE_Falling); // Notice that we enter falling after launch in the base version, which may not be what you want.
    PendingLaunchVelocity = FVector::ZeroVector;
    bForceNextFloorCheck = true;
    return true;
  }

  return false;
}

#pragma endregion

#pragma region Custom Movement

#pragma region Flying

// Whether or not we should allow the player to fly in a certain situation.
bool UCMCTestCharacterMovementComponent::CanFly() const
{
  return true;
}

// Code to execute upon entering flight
void UCMCTestCharacterMovementComponent::EnterFlying_Implementation()
{
  bIsFlying = true;
}

// Code to execute upon exiting flight
void UCMCTestCharacterMovementComponent::ExitFlying_Implementation()
{
  bIsFlying = false;
}

// We don't need to add our own phys_flying function because it already exists in the CMC! Thanks, Epic!
#pragma endregion

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

void UCMCTestCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
  if (!HasValidData())
  {
    return;
  }

  // We perform our custom logic first, then allow the Super (parent) function to handle the usual movement mode changes as it calls the character's OnMovementModeChanged.
  // You can change the order depending on your needs in this particular case. Simply read through the parent function to see what works best for you.

  /*
   * This is purely a personal preference thing, but I like to ensure there is always a State Entry and State Exit function for my movement modes.
   * You can even write these functions for the default modes if you aren't satisfied with their base implementation.
   * It is INCREDIBLY important to channel logic like this through a single "handler" function. This pattern can be used for many other systems that handle state changes.
   * Why? Because you want to clean up a state BEFORE entering a new one. You want a defined logic flow to prevent weird bugs.
   *
   * Here's a use case:
   * You have a Parkour movement mode that requires the player's capsule collision to be turned off during the move.
   * When you ENTER the move, you need to perform certain logic that turns off collision, prepares the movement component with new variables, etc.
   * However, when you want to end the move and return to Walking or Falling, you obviously always want collision to be turned back on.
   * What happens if it is interrupted, or another bit of logic dictates that the player should now enter Falling or Flying or something?
   * If that designer/programmer just sets the movement mode directly, you're going to be left with no collision. Or worse, in a networked environment you might have complete variable desync if done incorrectly.
   * Therefore, you need to ensure that everyone REQUESTS a movement mode change using a particular function.
   * In this case, we use SetMovementMode, which will then always call this function.
   * Using SetMovementMode is NOT inherently network-replicated if triggered by a client, so adding a RequestMovementModeChange() function can be useful to flip-flop our network prediction flags (like) and perform other checks.
   * Thus, no matter what dynamic bit of gameplay code triggers a movement change, it will be caught by this function and properly EXIT a mode before ENTERing the next.
   * This design pattern is handy for many systems, as mentioned, but movement is one such place where it can be essential.
   */

  // First, call exit code for the PREVIOUS movement mode.
  if (PreviousMovementMode == MOVE_Custom)
  {
  }
  else
  {
    switch (PreviousMovementMode)
    {
    case MOVE_Flying:
      ExitFlying();
    }
  }

  // Next, call entry code for the NEW movement mode.
  if (MovementMode == MOVE_Custom)
  {
  }
  else
  {
    switch (MovementMode)
    {
    case MOVE_Flying:
      EnterFlying();
    }
  }

  /*
   * Remember, you can always copy and modify the code from any function's Super (parent) if you need new or different functionality.
   * Just keep in mind that failing to call Super on many functions can break a lot of functionality, as you may need code to trigger that is further up the inheritance chain. (ESPECIALLY BeginPlay or other core engine functions).
   * Be sure to follow the inheritance chain to see when you should always call Super.
   * If you need to modify the direct parent's Super but still want the rest of the chain to trigger, you can always call Super::Super:: or call a particular parent function up the chain (like UPawnMovementComponent::OnMovementModeChanged, which does not exist in this case).
   */
  Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

// Called on tick, can be used for setting values and movement modes for next tick.
void UCMCTestCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector &OldLocation, const FVector &OldVelocity)
{
  Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

  /*
   * The code below could be done in UpdateCharacterStateBeforeMovement, but there's no perceptible difference for the player.
   * I included it here to demonstrate that this function can serve a similar purpose as UpdateCharacterStateBeforeMovement and UpdateCharacterStateAfterMovement.
   * However, it is always called AFTER UpdateCharacterStateAfterMovementand and is typically used to set up logic for the next tick in custom CMCs.
   */
  if (CharacterOwner->GetLocalRole() > ROLE_SimulatedProxy)
  {
    if (IsFlagActive((uint8)EMovementFlag::CFLAG_WantsToFly)) // We typecast to uint8 due to how we declared this function. It accepts uint8, not EMovementFlag in C++.
    {
      if (CanFly())
      {
        SetMovementMode(MOVE_Flying);
      }
    }
    else if ((!IsFlagActive((uint8)EMovementFlag::CFLAG_WantsToFly) || !CanFly()) && MovementMode == MOVE_Flying)
    {
      SetMovementMode(MOVE_Falling);
    }
  }

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

    // If you still wanted to use bools AND bitflags, you could unpack movement flags like this.
    // It is similar to UpdateFromCompressedFlags in this sense. Check out that function in the parent to see how it's done.
    // EXAMPLE:
    // bWantsToFly = (CurrentMoveData->MovementFlagCustomMoveData & (uint8)EMovementFlag::CFLAG_WantsToFly) != 0;

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
  SerializeOptionalValue<float>(Ar.IsSaving(), Ar, MaxCustomSpeedMoveData, 800.f);
  SerializeOptionalValue<FVector>(Ar.IsSaving(), Ar, LaunchVelocityCustomMoveData, FVector(0.f, 0.f, 0.f));

  SerializeOptionalValue<uint8>(Ar.IsSaving(), Ar, MovementFlagCustomMoveData, 0);

  return !Ar.IsError();
}

void FCustomNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character &ClientMove, ENetworkMoveType MoveType)
{
  Super::ClientFillNetworkMoveData(ClientMove, MoveType);

  const FCustomSavedMove &CurrentSavedMove = static_cast<const FCustomSavedMove &>(ClientMove);

  bWantsToLaunchMoveData = CurrentSavedMove.bWantsToLaunchSaved;
  MaxCustomSpeedMoveData = CurrentSavedMove.SavedMaxCustomSpeed;
  LaunchVelocityCustomMoveData = CurrentSavedMove.SavedLaunchVelocityCustom;

  MovementFlagCustomMoveData = CurrentSavedMove.SavedMovementFlagCustom;
}

// Combines Flags together as an optimization option by the engine to send less data over the network
bool FCustomSavedMove::CanCombineWith(const FSavedMovePtr &NewMove, ACharacter *Character, float MaxDelta) const
{
  FCustomSavedMove *NewMovePtr = static_cast<FCustomSavedMove *>(NewMove.Get());

  if (bWantsToLaunchSaved != NewMovePtr->bWantsToLaunchSaved)
  {
    return false;
  }

  if (SavedMaxCustomSpeed != NewMovePtr->SavedMaxCustomSpeed)
  {
    return false;
  }

  if (SavedLaunchVelocityCustom != NewMovePtr->SavedLaunchVelocityCustom)
  {
    return false;
  }

  if (SavedMovementFlagCustom != NewMovePtr->SavedMovementFlagCustom)
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

    SavedMovementFlagCustom = CharacterMovement->MovementFlagCustom;
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

    CharacterMovementComponent->MovementFlagCustom = SavedMovementFlagCustom;
  }
}

// Just used to reset the data in a saved move.
void FCustomSavedMove::Clear()
{
  Super::Clear();

  bWantsToLaunchSaved = false;

  SavedMaxCustomSpeed = 800.f;
  SavedLaunchVelocityCustom = FVector(0.f, 0.f, 0.f);

  SavedMovementFlagCustom = 0;
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