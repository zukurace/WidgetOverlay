
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

    FTransform targetTransform = m_targetActor->GetTransform();
    targetTransform.SetScale3D(FVector(1, 1, .3));

    const FMatrix model = targetTransform.ToMatrixWithScale();
    const FMatrix view = FTranslationMatrix(-ProjectionData.ViewOrigin) * ProjectionData.ViewRotationMatrix;
    const FMatrix proj = ProjectionData.ProjectionMatrix;

    const FMatrix MVP = model * view * proj; // to place widget on screen

    constexpr float ext = 100.f;
    static FVector localBounds[8] = {
        FVector(-ext, -ext, -ext), FVector(-ext, -ext, ext), FVector(-ext, ext, -ext), FVector(-ext, ext, ext),
        FVector(ext, -ext, -ext), FVector(ext, -ext, ext), FVector(ext, ext, -ext), FVector(ext, ext, ext)
    };

    FVector2D minB(1, 1), maxB(-1, -1);

    for (int i = 0; i < 8; ++i) {
        const FVector4& projectedVertex = MVP.TransformPosition(localBounds[i]);
        if (projectedVertex.W <= 0)
            continue;
        FVector2D screenNormalizedPos(((FVector)projectedVertex) / projectedVertex.W);
        minB = FVector2D::Min(minB, screenNormalizedPos);
        maxB = FVector2D::Max(maxB, screenNormalizedPos);

        DrawDebugPoint(GetWorld(), targetTransform.TransformPosition(localBounds[i]), 30, FColor::Red, 0, 0.f, 1);
    }

    minB = minB.ClampAxes(-1, 1); // clamp in range (-1, 1)
    maxB = maxB.ClampAxes(-1, 1);

    FVector2D normalizedSize = maxB - minB;
    if (normalizedSize.X <= 0 || normalizedSize.Y <= 0) {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(0, 0, FColor::Red, TEXT("Invisible"), true, FVector2D(2));
        return;
    }

    normalizedSize *= 0.5;
    minB = minB * FVector2D(0.5, -0.5) + 0.5; // normalize to canvas (0 .. 1)
    maxB = maxB * FVector2D(0.5, -0.5) + 0.5;
    minB = FVector2D(minB.X, maxB.Y); // as we inversed by Y we should swap Y component
    maxB = FVector2D(maxB.X, minB.Y);

    FVector2D canvasSize(m_canvasRoot->GetCachedGeometry().GetLocalSize()); // full screen canvas size
    canvasSlot->SetPosition(canvasSize * minB);
    canvasSlot->SetSize(canvasSize * normalizedSize);

    // Prepare data for shader
    const FMatrix PVM = MVP.Inverse(); // to convert screen space UV to model-space rays

    for (int i = 0; i < 4; ++i) {
        dynamicMaterial->SetVectorParameterValue(matrixRowNames[i],
            FLinearColor(PVM.M[i][0], PVM.M[i][1], PVM.M[i][2], PVM.M[i][3]));
    }

    draw = true;
}
