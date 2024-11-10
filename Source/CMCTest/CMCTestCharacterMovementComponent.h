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

  // UNSAFE variables
  float MaxCustomSpeedMoveData = 800.f;
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

  // Contains our saved custom movement flags, like CFLAG_WantsToFly.
  uint8 SavedMovementFlagCustom = 0;

  // Variables
  float SavedMaxCustomSpeed = 800.f;
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

/* Our optimised movement flag container.
 * This uses bitshifting to pack a whole lot of extra flags into one tiny container, which can lower your bitrate usage.
 * Sending larger data types via packed moves can negatively impact network performance.
 * However, every project is different. You might prefer the readability and ease of use of using normal data types like bools (as demonstrated).
 * But this may not cut it in a networked environment that requires minimal network usage (like saving on server bandwidth costs in AA to AAA games).
 */
UENUM(BlueprintType, Meta = (Bitflags))
enum class EMovementFlag : uint8
{
  NONE = 0 UMETA(Hidden), // Requires a 0 entry as default when initialised, but we hide it from BP.

  CFLAG_WantsToFly = 1 << 0,

  CFLAG_OtherFlag1 = 1 << 1,
  CFLAG_OtherFlag2 = 1 << 2,
  CFLAG_OtherFlag3 = 1 << 3,
};

/**
 * Forward-declare our imported classes that are referenced here.
 * We do this to reduce header file overhead. Too many includes in a header file that is then included in subsequent header files can bog down compile times.
 * Also, most classes that include this header may not need all of the references contained in the .cpp file.
 * Thus, we declare external classes here in the header and include required headers in the .cpp file.
 * There are certainly exceptions to this rule, such as reducing the need to include common classes by hosting them within a certain header file.
 */
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

#pragma region Flying

  /*
   * This is different from our other replicated simulated proxy variables (this is not replicated).
   * Because we have an enter/exit mode when the movement mode changes, our bIsFlying bool is automatically updated on simulated proxies when they apply the new movement mode.
   * Of course, we might not want some logic to run on sim proxies, but you can simply perform checks for that.
   * Just keep track of where code will and will not (or should not) be run on simulated proxies.
   * Working around movement mode changes works well for setting extra variables.
   * You could make this variable protected/private and expose an accessor method to BP, but this works just fine. Even something like MovementMode is publicly accessible in the parent classes.
   * Pick a pattern or stick to coding conventions adopted by your team/company/etc.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flying")
  bool bIsFlying;

  // This function will be called in OnMovementUpdated to change movement modes if able.
  UFUNCTION(BlueprintCallable)
  virtual bool CanFly() const;

  /*
   * This is a great way to expose easy helper functions to BP.
   * Instead of having to call IsFlagActive directly for common checks, you can also add in helpers like this.
   * But IsFlagActive is already quite easy to use in BP because of how it was declared, so these helpers are still completely optional.
   * It is included here as an example.
   */
  UFUNCTION(BlueprintCallable)
  virtual bool DoesCharacterWantToFly() const { return IsFlagActive((uint8)EMovementFlag::CFLAG_WantsToFly); }

  /*We make our enter/exit functions private/protected so people don't accidentally call them when trying to start/end flying.
   * We should ALWAYS use SetMovementMode instead based on our design pattern.
   * Always consider how your function names and their visibility can accidentally mislead your team members.
   * private/protected exists for this reason. It is also best to have accessible documentation for team members to reference and understand your design pattern.
   */
protected:
  /*
   * Here we have our ENTER/EXIT functions for the flying movement mode.
   */
  UFUNCTION(BlueprintNativeEvent, Category = "Flying")
  void EnterFlying();
  virtual void EnterFlying_Implementation();

  UFUNCTION(BlueprintNativeEvent, Category = "Flying")
  void ExitFlying();
  virtual void ExitFlying_Implementation();

#pragma endregion

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

protected:
  /** Called after MovementMode has changed. Base implementation performs special handling for starting certain modes, then notifies the CharacterOwner.
   *	We update it to become our central movement mode switching function. All enter/exit functions should be stored here or in SetMovementMode.
   */
  virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

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
