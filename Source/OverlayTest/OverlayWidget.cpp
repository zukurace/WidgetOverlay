
#include "OverlayWidget.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

static const FName matrixRowNames[] = {
    TEXT("ViewProjectionMatrixRow0"),
    TEXT("ViewProjectionMatrixRow1"),
    TEXT("ViewProjectionMatrixRow2"),
    TEXT("ViewProjectionMatrixRow3"),
};

void UOverlayWidget::UpdateOverlay()
{
    bool draw = false;
    ON_SCOPE_EXIT->void
    {
        m_imageOverlay->SetVisibility(draw ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    };

    const APlayerController* player = GetOwningPlayer();
    if (!player || !m_imageOverlay)
        return;

    FSceneViewProjectionData ProjectionData;
    ULocalPlayer* const LP = player->GetLocalPlayer();

    if (!(LP && LP->ViewportClient && LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData)))
        return;

    auto dynamicMaterial = m_imageOverlay->GetDynamicMaterial();
    if (!dynamicMaterial)
        return;

    FMatrix projectionInverse = ProjectionData.ProjectionMatrix.Inverse();

    FMatrix viewMatrixInverse = ProjectionData.ViewRotationMatrix.Inverse();
    viewMatrixInverse.SetOrigin(ProjectionData.ViewOrigin);

    FMatrix modelMatrixInverse = FMatrix::Identity; // it does nothing for now, object is in the world origin

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
