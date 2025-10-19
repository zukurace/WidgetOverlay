
#include "OverlayWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

static const FName matrixRowNames[] = {
    TEXT("ViewProjectionMatrixRow0"),
    TEXT("ViewProjectionMatrixRow1"),
    TEXT("ViewProjectionMatrixRow2"),
    TEXT("ViewProjectionMatrixRow3"),
};

void UOverlayWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // In our case the object created before this widget.
    // On widget construction it iterates all objects with tag "Target".
    // Warning: object can be created after our widget, so we should subscribe to actor creation delegate
    TArray<AActor*> taggedActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Target"), taggedActors);
    m_targetActor = taggedActors.Num() ? taggedActors[0] : nullptr;
}

void UOverlayWidget::UpdateOverlay()
{
    bool draw = false;
    ON_SCOPE_EXIT->void
    { // the code inside executes when scope exit '}' and on 'return' statement
        m_imageOverlay->SetVisibility(draw ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    };

    if (!m_targetActor.IsValid() || !m_imageOverlay || !m_canvasRoot)
        return;

    const APlayerController* player = GetOwningPlayer();
    if (!player)
        return;

    FSceneViewProjectionData ProjectionData;
    ULocalPlayer* const LP = player->GetLocalPlayer();

    if (!(LP && LP->ViewportClient && LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData)))
        return;

    auto dynamicMaterial = m_imageOverlay->GetDynamicMaterial();
    if (!dynamicMaterial)
        return;

    auto canvasSlot = Cast<UCanvasPanelSlot>(m_imageOverlay->Slot);
    if (!canvasSlot)
        return;

    // TODO: calculate position and size
    canvasSlot->SetPosition(FVector2D(200, 200));
    canvasSlot->SetSize(FVector2D(200, 200));

    FMatrix projectionInverse = ProjectionData.ProjectionMatrix.Inverse();

    FMatrix viewMatrixInverse = ProjectionData.ViewRotationMatrix.Inverse();
    viewMatrixInverse.SetOrigin(ProjectionData.ViewOrigin);

    FTransform targetTransform = m_targetActor->GetTransform();
    targetTransform.SetScale3D(FVector(1, 1, .3)); // check non-uniform scale

    // correct non-uniform scale
    FMatrix modelMatrixInverse = targetTransform.ToMatrixWithScale().Inverse();
    // faster, but wrong with non-uniform scale
    // FMatrix modelMatrixInverse = targetTransform.Inverse().ToMatrixWithScale();

    FMatrix pvm = projectionInverse * viewMatrixInverse * modelMatrixInverse;

    for (int i = 0; i < 4; ++i) {
        dynamicMaterial->SetVectorParameterValue(matrixRowNames[i],
            FLinearColor(pvm.M[i][0], pvm.M[i][1], pvm.M[i][2], pvm.M[i][3]));
    }

    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(size_t(this), .1f, FColor::Red, pvm.ToString(), true);
    }

    draw = true;
}
