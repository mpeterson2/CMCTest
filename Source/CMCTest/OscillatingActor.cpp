#include "OscillatingActor.h"
#include "Kismet/GameplayStatics.h"

AOscillatingActor::AOscillatingActor()
{
  bReplicates = true;
  SetReplicateMovement(true);
  PrimaryActorTick.bCanEverTick = true;
}

void AOscillatingActor::BeginPlay()
{
  Super::BeginPlay();

  if (!HasAuthority())
  {
    PrimaryActorTick.SetTickFunctionEnable(false);
    return;
  }

  PrimaryActorTick.bCanEverTick = true;
  OriginalLocation = GetActorLocation();
}

void AOscillatingActor::Tick(float deltaSeconds)
{
  Super::Tick(deltaSeconds);

  auto time = UGameplayStatics::GetRealTimeSeconds(GetWorld());

  auto offsetX = GetOffsetValue(CurveX, time, Velocity.X, OffsetMovement.X);
  auto offsetY = GetOffsetValue(CurveY, time, Velocity.Y, OffsetMovement.Y);
  auto offsetZ = GetOffsetValue(CurveZ, time, Velocity.Z, OffsetMovement.Z);

  auto newOffset = FVector(offsetX, offsetY, offsetZ);
  SetActorLocation(OriginalLocation + newOffset);
}

float AOscillatingActor::GetOffsetValue(UCurveFloat *curve, float time, float speed, float distance)
{
  return curve ? curve->GetFloatValue(FMath::Sin(time * speed)) * distance : 0;
}