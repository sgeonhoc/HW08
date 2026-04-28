#pragma once

#include "CoreMinimal.h"
#include "BaseItem.h"
#include "HealingItem.generated.h"

UCLASS()
class HW08_API AHealingItem : public ABaseItem
{
    GENERATED_BODY()

public:
        AHealingItem();

protected:
    // È¸º¹·®
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
    int32 HealAmount;

    virtual void ActivateItem(AActor* Activator) override;
};