#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CMCTestCharacterMovementComponent.generated.h"

/////BEGIN Network Prediction Setup/////
#pragma region Network Prediction Setup

// Network Move DATA
// This new Move Data system enables much more flexibility.
// Before, using the old compressed flags approach, we were limited to around 8 flags.
// This is usually not enough for modern games.
// But more than this, we can send almost any data type using Move Data within the corresponding Serialize function.
// As mentioned above, just be careful of UNSAFE variables. The client might be sending false info to cheat.
class FCustomNetworkMoveData : public FCharacterNetworkMoveData
{

public:
  typedef FCharacterNetworkMoveData Super;

  virtual void ClientFillNetworkMoveData(const FSavedMove_Character &ClientMove, ENetworkMoveType MoveType) override;

  virtual bool Serialize(UCharacterMovementComponent &CharacterMovement, FArchive &Ar, UPackageMap *PackageMap, ENetworkMoveType MoveType) override;

  // SAFE variables
  bool bWantsToLaunchMoveData = false;
  FVector LaunchVelocityCustomMoveData = FVector(0.f, 0.f, 0.f);

  // This bypasses the limitations of the typical compressed flags used in past versions of UE4.
  // You would still use bitflags like this in games in order to improve network performance.
  // Imagine hundreds of clients sending this info every tick. Even a small saving can add up significantly, especially when considering server costs.
  // It is up to you to decide if you prefer sending bools like above or using the lightweight bitflag approach like below.
  // If you're making P2P games, casual online, or co-op vs AI, etc, then you might not care too much about maximising efficiency. The bool approach might be more readable.
  uint8 MovementFlagCustomMoveData = 0;
};

class FCustomCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{

public:
  FCustomCharacterNetworkMoveDataContainer();

  FCustomNetworkMoveData CustomDefaultMoveData[3];
};

// Class FCustomSavedMove
class FCustomSavedMove : public FSavedMove_Character
{
public:
  typedef FSavedMove_Character Super;

  // All Saved Variables are placed here.
  // Boolean Flags

  bool bWantsToLaunchSaved = false;

  FVector SavedLaunchVelocityCustom = FVector(0.f, 0.f, 0.f);

  /** Returns a byte containing encoded special movement information (jumping, crouching, etc.)	 */
  virtual uint8 GetCompressedFlags() const override;

  /** Returns true if this move can be combined with NewMove for replication without changing any behaviour.
   * Just determines if any variables were modified between moves for optimisation purposes. If nothing changed, combine moves to save time.
   */
  virtual bool CanCombineWith(const FSavedMovePtr &NewMove, ACharacter *Character, float MaxDelta) const override;

  /** Called to set up this saved move (when initially created) to make a predictive correction. */
  virtual void SetMoveFor(ACharacter *Character, float InDeltaTime, FVector const &NewAccel, class FNetworkPredictionData_Client_Character &ClientData) override;
  /** Called before ClientUpdatePosition uses this SavedMove to make a predictive correction	 */
  virtual void PrepMoveFor(class ACharacter *Character) override;

  /** Clear saved move properties, so it can be re-used. */
  virtual void Clear() override;
};

// Class Prediction Data
class FCustomNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
public:
  FCustomNetworkPredictionData_Client(const UCharacterMovementComponent &ClientMovement);

  typedef FNetworkPredictionData_Client_Character Super;

  /// Allocates a new copy of our custom saved move
  virtual FSavedMovePtr AllocateNewMove() override;
};

#pragma endregion
/////END Network Prediction Setup/////

/////BEGIN CMC Setup/////
#pragma region CMC Setup

class ACMCTestCharacter;

/**
 *
 */
UCLASS()
class UCMCTestCharacterMovementComponent : public UCharacterMovementComponent
{
  GENERATED_BODY()

public:
  UCMCTestCharacterMovementComponent(const FObjectInitializer &ObjectInitializer);

  // Reference to our network prediction buddy, the custom saved move class created above.
  friend class FCustomSavedMove;

  bool bWantsToLaunch;

  /////BEGIN Custom Movement/////
#pragma region Custom Movement

  // Notice how this variable is declared. This allows BP to allow you to work with bit flags.
  UPROPERTY(EditAnywhere, Category = TestBitflag, meta = (Bitmask, BitmaskEnum = "EMovementFlag"))
  uint8 MovementFlagCustom = 0;

  // We also want a way to test if a flag is active in C++ or BP.
  UFUNCTION(BlueprintCallable)
  bool IsFlagActive(UPARAM(meta = (Bitmask, BitmaskEnum = EMovementFlag)) uint8 TestFlag) const { return (MovementFlagCustom & TestFlag); }

  UFUNCTION(BlueprintCallable)
  virtual void ActivateMovementFlag(UPARAM(meta = (Bitmask, BitmaskEnum = EMovementFlag)) uint8 FlagToActivate);

  UFUNCTION(BlueprintCallable)
  virtual void ClearMovementFlag(UPARAM(meta = (Bitmask, BitmaskEnum = EMovementFlag)) uint8 FlagToClear);

#pragma region Replicated Launch

public:
  UFUNCTION(BlueprintCallable)
  void StartLaunching();
  UFUNCTION(BlueprintCallable)
  void StopLaunching();

  /*
   * Allows us to apply a replicated launch, effectively adding a "boop" mechanic or a predicted jump pad, etc.
   * This can be considered an UNSAFE variable, though. The player could use this to cheat, so we need to make other checks before executing the launch.
   */
  UFUNCTION(BlueprintCallable, Category = "Launching")
  void LaunchCharacterReplicated(FVector NewLaunchVelocity, bool bXYOverride, bool bZOverride);
  // Network predicted variable.
  FVector LaunchVelocityCustom;

  // Override the parent handle launch to check our incoming launch requests.
  virtual bool HandlePendingLaunch() override;

#pragma endregion

#pragma endregion
  /////END Custom Movement/////

protected:
  /** Character movement component belongs to */
  UPROPERTY(Transient, DuplicateTransient)
  TObjectPtr<ACMCTestCharacter> CustomCharacter;

public:
  // BEGIN UActorComponent Interface
  virtual void BeginPlay() override;
  // END UActorComponent Interface

  // BEGIN UMovementComponent Interface

  /** Consider this to be the final chance to change logic before the next tick. It can be useful to defer certain actions to the next tick. */
  virtual void OnMovementUpdated(float DeltaSeconds, const FVector &OldLocation, const FVector &OldVelocity) override;

public:
/////BEGIN Networked Movement/////
#pragma region Networked Movement Setup
  // New Move Data Container
  FCustomCharacterNetworkMoveDataContainer MoveDataContainer;

  virtual void UpdateFromCompressedFlags(uint8 Flags) override;
  virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector &NewAccel) override;

  virtual class FNetworkPredictionData_Client *GetPredictionData_Client() const override;

#pragma endregion
  /////END Networked Movement/////
};

#pragma endregion
/////END CMC Setup/////
