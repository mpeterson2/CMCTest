#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "OscillatingActor.generated.h"

UCLASS()
class AOscillatingActor : public AActor
{
  GENERATED_BODY()

public:
  AOscillatingActor();
  virtual void BeginPlay() override;
  virtual void Tick(float deltaSeconds) override;

protected:
  UPROPERTY(EditAnywhere)
  UCurveFloat *CurveX;
  UPROPERTY(EditAnywhere)
  UCurveFloat *CurveY;
  UPROPERTY(EditAnywhere)
  UCurveFloat *CurveZ;

  UPROPERTY(EditAnywhere)
  FVector OffsetMovement;
  UPROPERTY(EditAnywhere)
  FVector Velocity;

  FVector OriginalLocation;

  float GetOffsetValue(UCurveFloat *curve, float time, float speed, float distance);
};
