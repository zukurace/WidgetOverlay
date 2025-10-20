
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

    //// TODO: calculate position and size from MVP matrix
    // canvasSlot->SetPosition(CALCULATE POSITION);
    // canvasSlot->SetSize(CALCULATE SIZE);
    // if(OUT OF SCREEN BOUNDS) { draw = false; return; }

    const FMatrix PVM = MVP.Inverse(); // to convert screen space UV to model-space rays

    for (int i = 0; i < 4; ++i) {
        dynamicMaterial->SetVectorParameterValue(matrixRowNames[i],
            FLinearColor(PVM.M[i][0], PVM.M[i][1], PVM.M[i][2], PVM.M[i][3]));
    }

    draw = true;
}
