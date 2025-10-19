
#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "OverlayWidget.generated.h"

class UImage;

UCLASS()
class OVERLAYTEST_API UOverlayWidget : public UUserWidget {
    GENERATED_BODY()

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* m_imageOverlay;

    TWeakObjectPtr<AActor> m_targetActor;

protected:
    void NativeConstruct() override;

public:
    UFUNCTION(BlueprintCallable)
    void UpdateOverlay();
};
