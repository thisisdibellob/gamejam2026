// Copyright Epic Games, Inc. All Rights Reserved.

#include "mainCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameSystemSubsystem.h"
#include "Public/PatrolNPC2.h"
#include "Camera/PlayerCameraManager.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/PointLightComponent.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"

namespace
{
	UWidgetAnimation* FindWidgetAnimationByName(UUserWidget* Widget, const FName& AnimationName)
	{
		if (!Widget)
		{
			return nullptr;
		}

		const FString TargetName = AnimationName.ToString();
		const FString TargetInstName = TargetName + TEXT("_INST");

		for (TFieldIterator<FObjectPropertyBase> PropertyIt(Widget->GetClass()); PropertyIt; ++PropertyIt)
		{
			FObjectPropertyBase* ObjectProperty = *PropertyIt;
			if (!ObjectProperty || !ObjectProperty->PropertyClass || !ObjectProperty->PropertyClass->IsChildOf(UWidgetAnimation::StaticClass()))
			{
				continue;
			}

			const FString PropertyName = ObjectProperty->GetName();
			if (PropertyName != TargetName && PropertyName != TargetInstName)
			{
				continue;
			}

			return Cast<UWidgetAnimation>(ObjectProperty->GetObjectPropertyValue_InContainer(Widget));
		}

		return nullptr;
	}

	float GetWidgetAnimationDuration(const UWidgetAnimation* Animation)
	{
		if (!Animation)
		{
			return 0.0f;
		}

		return FMath::Max(0.0f, Animation->GetEndTime() - Animation->GetStartTime());
	}
}

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	VampireAuraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("VampireAuraLight"));
	if (VampireAuraLight)
	{
		VampireAuraLight->SetupAttachment(RootComponent);
		VampireAuraLight->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
		VampireAuraLight->SetIntensity(0.0f);
		VampireAuraLight->SetAttenuationRadius(260.0f);
		VampireAuraLight->SetSourceRadius(80.0f);
		VampireAuraLight->SetSoftSourceRadius(120.0f);
		VampireAuraLight->SetLightColor(FLinearColor(1.0f, 0.08f, 0.035f, 1.0f));
		VampireAuraLight->SetCastShadows(false);
		VampireAuraLight->SetVisibility(false);
	}
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetGuilt(Guilt);
	SetInvest(Invest);
	SetBlood(Blood);
	SetStamina(MaxStamina);

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// ���� ���� �� ù ���� �����ٸ�
	ScheduleNextTransformation();

	GetWorldTimerManager().SetTimer(SubtitleBlinkTimerHandle, this, &AMainCharacter::BlinkSubtitleText, FMath::Max(SubtitleBlinkInterval, 0.05f), true);
}

void AMainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSprint(DeltaSeconds);
	UpdateCharacterUIShake(DeltaSeconds);
	UpdateCharacterModeUITransition(DeltaSeconds);
	UpdateVampireAura(DeltaSeconds);

	AActor* NewNearbyNPC = GetClosestNPC();
	if (CurrentNearbyNPC != NewNearbyNPC)
	{
		CurrentNearbyNPC = NewNearbyNPC;
		OnNearbyNPCChangedBroadcast.Broadcast(CurrentNearbyNPC != nullptr, CurrentNearbyNPC);
	}

	// �����̾� ������ �� ���� ���̵� ���� ��� (���� �����Ͱ� �ƴ� ����)
	if (bIsVampire && PermanentState != EPermanentState::PureVampire)
	{
		float OldBlood = Blood;
		float BloodDecreasePerSecond = 1.0f;

		SetBlood(Blood - (BloodDecreasePerSecond * DeltaSeconds));

		if (OldBlood > 0.0f && Blood <= 0.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[System] Blood reached 0. Game over."));
			bIsDead = true;

			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (UGameSystemSubsystem* GameSystem = GameInstance->GetSubsystem<UGameSystemSubsystem>())
				{
					GameSystem->EndGame();
				}
			}
		}
	}
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AMainCharacter::TestIncreaseGuilt);
	PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &AMainCharacter::TestIncreaseBlood);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// �޸��� (Shift)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMainCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMainCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMainCharacter::StopSprint);
		}

		// ���� �˻� (Q)
		if (InspectAction)
		{
			EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Started, this, &AMainCharacter::PerformInspect);
		}

		// ���� (E)
		if (StunAction)
		{
			EnhancedInputComponent->BindAction(StunAction, ETriggerEvent::Started, this, &AMainCharacter::PerformStun);
		}

		// ���̱�/��� (R)
		if (KillAction)
		{
			EnhancedInputComponent->BindAction(KillAction, ETriggerEvent::Started, this, &AMainCharacter::PerformKill);
		}
	}
}

void AMainCharacter::TestIncreaseGuilt()
{
	AddGuilt(10.0f);
}

void AMainCharacter::TestIncreaseBlood()
{
	AddBlood(10.0f);
}

void AMainCharacter::SetGuilt(float NewGuilt)
{
	const float OldGuilt = Guilt;
	Guilt = FMath::Clamp(NewGuilt, 0.0f, 100.0f);

	// ���� ���� üũ �� �α� ���
	if (Guilt >= 100.0f && PermanentState != EPermanentState::PureVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Guilt reached 100. Permanently turned into 'Pure Vampire'!"));
		PermanentState = EPermanentState::PureVampire;
		SetIsVampire(true);
	}
	else if (Guilt <= 0.0f && PermanentState != EPermanentState::PureHuman)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Guilt reached 0. Permanently turned into 'Pure Human'!"));
		PermanentState = EPermanentState::PureHuman;
		SetIsVampire(false);
	}

	if (!FMath::IsNearlyEqual(OldGuilt, Guilt))
	{
		OnGuiltChanged(Guilt, OldGuilt);
		OnGuiltChangedBroadcast.Broadcast(Guilt, OldGuilt);
	}
}

void AMainCharacter::AddGuilt(float Amount)
{
	// óġ�� ��� ���� �α� ��� (���: �Ϲ���, ����: ������)
	if (Amount > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Killed an innocent. Guilt increased by %f."), Amount);
	}
	else if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Killed a criminal. Guilt decreased by %f."), FMath::Abs(Amount));
	}

	SetGuilt(Guilt + Amount);
}

void AMainCharacter::SetInvest(float NewInvest)
{
	const float OldInvest = Invest;
	Invest = FMath::Clamp(NewInvest, 0.0f, 100.0f);

	if (!FMath::IsNearlyEqual(OldInvest, Invest))
	{
		OnInvestChanged(Invest, OldInvest);
		OnInvestChangedBroadcast.Broadcast(Invest, OldInvest);
	}
}

void AMainCharacter::AddInvest(float Amount)
{
	SetInvest(Invest + Amount);
}

void AMainCharacter::SetBlood(float NewBlood)
{
	const float OldBlood = Blood;
	Blood = FMath::Clamp(NewBlood, 0.0f, MaxBlood);

	if (!FMath::IsNearlyEqual(OldBlood, Blood))
	{
		OnBloodChanged(Blood, OldBlood);
		OnBloodChangedBroadcast.Broadcast(Blood, OldBlood);
	}
}

void AMainCharacter::AddBlood(float Amount)
{
	// �������� ������ ����� ���� ���̵��� �ؼ��� ������ ����
	if (Amount < 0.0f && bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Bloodsucking successful. Blood deficiency decreased by %f."), FMath::Abs(Amount));
	}

	SetBlood(Blood + Amount);
}

void AMainCharacter::SetIsVampire(bool bNewIsVampire)
{
	if (bIsVampire == bNewIsVampire) return;

	PrepareCharacterModeUITransition(bNewIsVampire);

	bIsVampire = bNewIsVampire;

	if (bIsVampire)
	{
		PlayTransformCameraShake();
		ShowBloodTransformWidget();

		if (TransformSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TransformSound, GetActorLocation());
		}

		UE_LOG(LogTemp, Warning, TEXT("[System] Transformed into a Vampire! Will revert to Human in 30 seconds."));

		if (bIsSprinting) StopSprint();
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		// ���� �����Ͱ� �ƴ� ���� 30�� �� ���ư��� Ÿ�̸� �۵�
		if (PermanentState != EPermanentState::PureVampire)
		{
			const float VampireDuration = 30.0f;
			const float FadeOutDuration = GetBloodWidgetFadeOutDuration();
			const float FadeOutStartDelay = FMath::Max(VampireDuration - FadeOutDuration, 0.0f);
			GetWorldTimerManager().SetTimer(TransformTimerHandle, this, &AMainCharacter::StartRevertToHuman, FadeOutStartDelay, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Reverted to Human. Awaiting next transformation."));
		StartBloodWidgetFadeOut();

		if (RevertTransformSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RevertTransformSound, GetActorLocation());
		}


		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		ScheduleNextTransformation(); // �ΰ��� �Ǿ����Ƿ� ���� ���� �����ٸ�
	}

	OnVampireChanged(bIsVampire);
	OnVampireChangedBroadcast.Broadcast(bIsVampire);
	StartCharacterModeUITransition(bIsVampire);
}

/* --- �޸��� ���� --- */
void AMainCharacter::ShowBloodTransformWidget()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(BloodRemoveTimerHandle);

	if (ActiveBloodWidget)
	{
		ActiveBloodWidget->RemoveFromParent();
		ActiveBloodWidget = nullptr;
	}

	if (!BloodWidgetClass)
	{
		BloodWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP/WBP_Blood.WBP_Blood_C"));
	}

	if (!BloodWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[MainCharacter] WBP_Blood widget class was not found."));
		return;
	}

	ActiveBloodWidget = CreateWidget<UUserWidget>(PC, BloodWidgetClass);
	if (!ActiveBloodWidget)
	{
		return;
	}

	ActiveBloodWidget->AddToViewport(BloodWidgetZOrder);

	UWidgetAnimation* FadeInAnimation = FindWidgetAnimationByName(ActiveBloodWidget, FName(TEXT("FadeIn")));

	if (FadeInAnimation)
	{
		ActiveBloodWidget->PlayAnimation(FadeInAnimation);
	}
}

float AMainCharacter::GetBloodWidgetFadeOutDuration() const
{
	if (!ActiveBloodWidget)
	{
		return 0.0f;
	}

	if (UWidgetAnimation* FadeOutAnimation = FindWidgetAnimationByName(ActiveBloodWidget, FName(TEXT("FadeOut"))))
	{
		return GetWidgetAnimationDuration(FadeOutAnimation);
	}

	return 0.0f;
}

float AMainCharacter::StartBloodWidgetFadeOut()
{
	if (!ActiveBloodWidget)
	{
		return 0.0f;
	}

	if (GetWorldTimerManager().IsTimerActive(BloodRemoveTimerHandle))
	{
		return GetWorldTimerManager().GetTimerRemaining(BloodRemoveTimerHandle);
	}

	float FadeOutDuration = 0.0f;
	if (UWidgetAnimation* FadeOutAnimation = FindWidgetAnimationByName(ActiveBloodWidget, FName(TEXT("FadeOut"))))
	{
		FadeOutDuration = GetWidgetAnimationDuration(FadeOutAnimation);
		ActiveBloodWidget->PlayAnimation(FadeOutAnimation);
	}

	const float RemoveDelay = FMath::Max(FadeOutDuration, 0.1f);
	GetWorldTimerManager().SetTimer(BloodRemoveTimerHandle, this, &AMainCharacter::RemoveBloodWidget, RemoveDelay, false);
	return RemoveDelay;
}

void AMainCharacter::StartRevertToHuman()
{
	const float FadeOutDuration = StartBloodWidgetFadeOut();

	if (FadeOutDuration <= 0.0f)
	{
		FinishRevertToHuman();
		return;
	}

	GetWorldTimerManager().SetTimer(TransformTimerHandle, this, &AMainCharacter::FinishRevertToHuman, FadeOutDuration, false);
}

void AMainCharacter::FinishRevertToHuman()
{
	SetIsVampire(false);
}

void AMainCharacter::RemoveBloodWidget()
{
	GetWorldTimerManager().ClearTimer(BloodRemoveTimerHandle);

	if (ActiveBloodWidget)
	{
		ActiveBloodWidget->RemoveFromParent();
		ActiveBloodWidget = nullptr;
	}
}

void AMainCharacter::BlinkSubtitleText()
{
	UTextBlock* SubtitleTextBlock = FindSubtitleTextBlock();
	if (!SubtitleTextBlock)
	{
		return;
	}

	bSubtitleBlinkVisible = !bSubtitleBlinkVisible;
	SubtitleTextBlock->SetRenderOpacity(bSubtitleBlinkVisible ? 1.0f : SubtitleBlinkDimOpacity);
}

UTextBlock* AMainCharacter::FindSubtitleTextBlock()
{
	if (IsValid(CachedSubtitleTextBlock))
	{
		return CachedSubtitleTextBlock;
	}

	CachedSubtitleTextBlock = nullptr;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return nullptr;
	}

	if (!MainWidgetClass)
	{
		MainWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP/WBP_main.WBP_main_C"));
	}

	if (!MainWidgetClass)
	{
		return nullptr;
	}

	TArray<UUserWidget*> MainWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, MainWidgets, MainWidgetClass, false);

	for (UUserWidget* MainWidget : MainWidgets)
	{
		if (!MainWidget || !MainWidget->WidgetTree)
		{
			continue;
		}

		CachedSubtitleTextBlock = Cast<UTextBlock>(MainWidget->WidgetTree->FindWidget(FName(TEXT("TextBlock_Subtitle"))));
		if (CachedSubtitleTextBlock)
		{
			CachedSubtitleTextBlock->SetRenderOpacity(1.0f);
			bSubtitleBlinkVisible = true;
			return CachedSubtitleTextBlock;
		}
	}

	return nullptr;
}

void AMainCharacter::UpdateCharacterUIShake(float DeltaSeconds)
{
	UUserWidget* CharacterWidget = FindCharacterWidget();
	if (!CharacterWidget)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const bool bIsMovingOnGround = GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround() && Speed > 5.0f;
	const float TargetIntensity = (bEnableCharacterUIShake && bIsMovingOnGround)
		? FMath::Clamp(Speed / FMath::Max(WalkSpeed, 1.0f), 0.0f, 1.0f)
		: 0.0f;

	CharacterUIShakeIntensity = FMath::FInterpTo(
		CharacterUIShakeIntensity,
		TargetIntensity,
		DeltaSeconds,
		FMath::Max(CharacterUIShakeSmoothSpeed, 0.1f));

	if (CharacterUIShakeIntensity <= 0.01f)
	{
		CharacterUIShakeIntensity = 0.0f;
		CharacterWidget->SetRenderTranslation(CharacterUIBaseRenderTranslation);
		return;
	}

	const float SprintBlendRange = FMath::Max(SprintSpeed - WalkSpeed, 1.0f);
	const float SprintAlpha = FMath::Clamp((Speed - WalkSpeed) / SprintBlendRange, 0.0f, 1.0f);
	const float Amplitude = FMath::Lerp(CharacterUIWalkShakeAmplitude, CharacterUISprintShakeAmplitude, SprintAlpha) * CharacterUIShakeIntensity;
	const float Frequency = FMath::Lerp(CharacterUIWalkShakeFrequency, CharacterUISprintShakeFrequency, SprintAlpha);

	CharacterUIShakePhase = FMath::Fmod(CharacterUIShakePhase + DeltaSeconds * Frequency * UE_TWO_PI, UE_TWO_PI);

	const float HorizontalSway = FMath::Sin(CharacterUIShakePhase * 0.5f) * Amplitude * 0.35f;
	const float VerticalBob = FMath::Sin(CharacterUIShakePhase) * Amplitude;
	const FVector2D MotionOffset(HorizontalSway, VerticalBob);

	CharacterWidget->SetRenderTranslation(CharacterUIBaseRenderTranslation + MotionOffset);
}

UUserWidget* AMainCharacter::FindCharacterWidget()
{
	if (IsValid(CachedCharacterWidget))
	{
		return CachedCharacterWidget;
	}

	CachedCharacterWidget = nullptr;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return nullptr;
	}

	if (!CharacterWidgetClass)
	{
		CharacterWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP/WBP_character.WBP_character_C"));
	}

	if (!CharacterWidgetClass)
	{
		return nullptr;
	}

	TArray<UUserWidget*> CharacterWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, CharacterWidgets, CharacterWidgetClass, false);

	for (UUserWidget* CharacterWidget : CharacterWidgets)
	{
		if (!CharacterWidget)
		{
			continue;
		}

		CachedCharacterWidget = CharacterWidget;
		CharacterUIBaseRenderTranslation = CharacterWidget->GetRenderTransform().Translation;
		return CachedCharacterWidget;
	}

	return nullptr;
}

void AMainCharacter::PrepareCharacterModeUITransition(bool bNewIsVampire)
{
	if (!bEnableCharacterModeTransition || !CacheCharacterModeImages())
	{
		return;
	}

	bCharacterModeTransitionToVampire = bNewIsVampire;
	ApplyCharacterModeUITransitionStyle(0.0f);
}

void AMainCharacter::StartCharacterModeUITransition(bool bNewIsVampire)
{
	if (!bEnableCharacterModeTransition || !CacheCharacterModeImages())
	{
		return;
	}

	bCharacterModeTransitionActive = true;
	bCharacterModeTransitionToVampire = bNewIsVampire;
	CharacterModeTransitionTime = 0.0f;
	ApplyCharacterModeUITransitionStyle(0.0f);
}

void AMainCharacter::UpdateCharacterModeUITransition(float DeltaSeconds)
{
	if (!bCharacterModeTransitionActive)
	{
		return;
	}

	const float Duration = FMath::Max(CharacterModeTransitionDuration, 0.05f);
	CharacterModeTransitionTime += DeltaSeconds;

	const float Alpha = FMath::Clamp(CharacterModeTransitionTime / Duration, 0.0f, 1.0f);
	ApplyCharacterModeUITransitionStyle(Alpha);

	if (Alpha >= 1.0f)
	{
		bCharacterModeTransitionActive = false;
		ResetCharacterModeImages();
	}
}

bool AMainCharacter::CacheCharacterModeImages()
{
	const bool bHasCachedImages =
		CharacterModeImages.Num() == 3 &&
		IsValid(CharacterModeImages[0]) &&
		IsValid(CharacterModeImages[1]) &&
		IsValid(CharacterModeImages[2]) &&
		CharacterModeImageBaseScales.Num() == CharacterModeImages.Num() &&
		CharacterModeImageBaseOpacities.Num() == CharacterModeImages.Num() &&
		CharacterModeImageBaseColors.Num() == CharacterModeImages.Num();

	if (bHasCachedImages)
	{
		return true;
	}

	CharacterModeImages.Reset();
	CharacterModeImageBaseScales.Reset();
	CharacterModeImageBaseOpacities.Reset();
	CharacterModeImageBaseColors.Reset();

	UUserWidget* CharacterWidget = FindCharacterWidget();
	if (!CharacterWidget || !CharacterWidget->WidgetTree)
	{
		return false;
	}

	static const FName ImageNames[] =
	{
		FName(TEXT("Image_Person")),
		FName(TEXT("Image_Tie")),
		FName(TEXT("Image_Vampire"))
	};

	for (const FName& ImageName : ImageNames)
	{
		UImage* Image = Cast<UImage>(CharacterWidget->WidgetTree->FindWidget(ImageName));
		if (!Image)
		{
			continue;
		}

		CharacterModeImages.Add(Image);
		CharacterModeImageBaseScales.Add(Image->GetRenderTransform().Scale);
		CharacterModeImageBaseOpacities.Add(Image->GetRenderOpacity());
		CharacterModeImageBaseColors.Add(Image->GetColorAndOpacity());
	}

	return CharacterModeImages.Num() > 0 &&
		CharacterModeImageBaseScales.Num() == CharacterModeImages.Num() &&
		CharacterModeImageBaseOpacities.Num() == CharacterModeImages.Num() &&
		CharacterModeImageBaseColors.Num() == CharacterModeImages.Num();
}

void AMainCharacter::ApplyCharacterModeUITransitionStyle(float Alpha)
{
	if (CharacterModeImages.Num() == 0)
	{
		return;
	}

	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float EasedAlpha = 1.0f - FMath::Pow(1.0f - ClampedAlpha, 3.0f);
	const float PulseAmount = 1.0f - EasedAlpha;
	const float PopAmount = FMath::Sin(EasedAlpha * UE_PI) * 0.018f;
	const FLinearColor PulseColor = bCharacterModeTransitionToVampire
		? CharacterModeVampirePulseColor
		: CharacterModeHumanPulseColor;

	for (int32 Index = 0; Index < CharacterModeImages.Num(); ++Index)
	{
		UImage* Image = CharacterModeImages[Index];
		if (!IsValid(Image) ||
			!CharacterModeImageBaseScales.IsValidIndex(Index) ||
			!CharacterModeImageBaseOpacities.IsValidIndex(Index) ||
			!CharacterModeImageBaseColors.IsValidIndex(Index))
		{
			continue;
		}

		const float BaseOpacity = CharacterModeImageBaseOpacities[Index];
		const float NewOpacity = FMath::Lerp(BaseOpacity * CharacterModeTransitionStartOpacity, BaseOpacity, EasedAlpha);
		Image->SetRenderOpacity(NewOpacity);

		const FVector2D BaseScale = CharacterModeImageBaseScales[Index];
		const float ScaleMultiplier = 1.0f - (CharacterModeTransitionScaleAmount * PulseAmount) + PopAmount;
		Image->SetRenderScale(BaseScale * ScaleMultiplier);

		const FLinearColor BaseColor = CharacterModeImageBaseColors[Index];
		const float ColorBlend = PulseAmount * 0.38f;
		const FLinearColor NewColor(
			FMath::Lerp(BaseColor.R, PulseColor.R, ColorBlend),
			FMath::Lerp(BaseColor.G, PulseColor.G, ColorBlend),
			FMath::Lerp(BaseColor.B, PulseColor.B, ColorBlend),
			BaseColor.A);
		Image->SetColorAndOpacity(NewColor);
	}
}

void AMainCharacter::ResetCharacterModeImages()
{
	for (int32 Index = 0; Index < CharacterModeImages.Num(); ++Index)
	{
		UImage* Image = CharacterModeImages[Index];
		if (!IsValid(Image) ||
			!CharacterModeImageBaseScales.IsValidIndex(Index) ||
			!CharacterModeImageBaseOpacities.IsValidIndex(Index) ||
			!CharacterModeImageBaseColors.IsValidIndex(Index))
		{
			continue;
		}

		Image->SetRenderOpacity(CharacterModeImageBaseOpacities[Index]);
		Image->SetRenderScale(CharacterModeImageBaseScales[Index]);
		Image->SetColorAndOpacity(CharacterModeImageBaseColors[Index]);
	}
}

void AMainCharacter::UpdateVampireAura(float DeltaSeconds)
{
	if (!VampireAuraLight)
	{
		return;
	}

	const float TargetIntensity = (bEnableVampireAura && bIsVampire) ? VampireAuraIntensity : 0.0f;
	CurrentVampireAuraIntensity = FMath::FInterpTo(
		CurrentVampireAuraIntensity,
		TargetIntensity,
		DeltaSeconds,
		FMath::Max(VampireAuraFadeSpeed, 0.1f));

	if (TargetIntensity <= 0.0f && CurrentVampireAuraIntensity <= 0.5f)
	{
		CurrentVampireAuraIntensity = 0.0f;
		VampireAuraLight->SetIntensity(0.0f);
		VampireAuraLight->SetVisibility(false);
		return;
	}

	VampireAuraPulseTime += DeltaSeconds;

	const float Pulse = bIsVampire
		? 1.0f + (FMath::Sin(VampireAuraPulseTime * 2.1f) * 0.06f)
		: 1.0f;

	VampireAuraLight->SetVisibility(true);
	VampireAuraLight->SetLightColor(VampireAuraColor);
	VampireAuraLight->SetAttenuationRadius(FMath::Max(VampireAuraRadius, 0.0f));
	VampireAuraLight->SetIntensity(CurrentVampireAuraIntensity * Pulse);
}

void AMainCharacter::StartSprint()
{
	if (bIsVampire) return;
	if (Stamina < MinStaminaToSprint) return;

	if (!bIsSprinting)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		OnSprintChanged(true);
	}
}

void AMainCharacter::StopSprint()
{
	if (bIsVampire) return;

	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		OnSprintChanged(false);
	}
}

void AMainCharacter::UpdateSprint(float DeltaSeconds)
{
	if (bIsVampire)
	{
		SetStamina(Stamina + StaminaRegenPerSecond * DeltaSeconds);
		return;
	}

	if (bIsSprinting)
	{
		SetStamina(Stamina - SprintStaminaDrainPerSecond * DeltaSeconds);
		if (Stamina <= 0.0f) StopSprint();
	}
	else
	{
		SetStamina(Stamina + StaminaRegenPerSecond * DeltaSeconds);
	}
}

void AMainCharacter::SetStamina(float NewStamina)
{
	Stamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
}

/* --- ��ų ���� --- */
void AMainCharacter::PerformInspect()
{
	if (bIsVampire) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastInspectTime < InspectCooldown) return;

	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		LastInspectTime = CurrentTime;
		if (APatrolNPC2* PatrolNPC = Cast<APatrolNPC2>(TargetNPC))
		{
			PatrolNPC->ShowKnotByCrimeState();
		}

		OnInspectNPC(TargetNPC);
	}
}

void AMainCharacter::PerformStun()
{
	if (!bIsVampire) return;

	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	if (StunMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Playing StunMontage!")); // 이 로그가 찍히는지 확인!
		PlayAnimMontage(StunMontage);
	}
	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		if (APatrolNPC2* PatrolNPC = Cast<APatrolNPC2>(TargetNPC))
		{
			PatrolNPC->SetStunned(true);
		}

		OnStunNPC(TargetNPC);
	}
}

void AMainCharacter::PerformKill()
{
	if (!bIsVampire) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastKillTime < KillCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cooling down..."));
		return;
	}

	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	if (KillMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Playing KillMontage!")); // 이 로그가 찍히는지 확인!
		PlayAnimMontage(KillMontage);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KillMontage is NULL!")); // 이게 찍히면 블루프린트 할당이 안 된 것
	}
	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		LastKillTime = CurrentTime;

		// 몽타주가 에디터에서 제대로 할당되었는지 확인 후 재생합니다.
		// 할당되지 않았는데 재생하려고 하면 게임이 튕길 수 있어서 꼭 검사해야 해요!
		
		UE_LOG(LogTemp, Warning, TEXT("[MainCharacter] NPC killed broadcast fired."));
		OnNPCKilledBroadcast.Broadcast(TargetNPC);
		OnNPCKilledSimpleBroadcast.Broadcast();
		OnKillNPC(TargetNPC);
	}
}

AActor* AMainCharacter::GetClosestNPC()
{
	FVector StartLoc = GetActorLocation();
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->SweepMultiByObjectType(HitResults, StartLoc, StartLoc, FQuat::Identity, FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Sphere, Params);

	AActor* ClosestNPC = nullptr;
	float MinDistSqr = InteractRadius * InteractRadius;

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor != this)
			{
				float DistSqr = (HitActor->GetActorLocation() - StartLoc).SizeSquared();
				if (DistSqr < MinDistSqr)
				{
					MinDistSqr = DistSqr;
					ClosestNPC = HitActor;
				}
			}
		}
	}
	return ClosestNPC;
}

/* --- ���� �ý��� --- */
void AMainCharacter::ScheduleNextTransformation()
{
	if (PermanentState == EPermanentState::PureHuman || PermanentState == EPermanentState::PureVampire) return;

	GetWorldTimerManager().ClearTimer(TransformTimerHandle);

	float BaseWaitTime = FMath::RandRange(30.0f, 50.0f);
	float GuiltFactor = 1.0f - ((Guilt / 100.0f) * 0.5f);
	float FinalWaitTime = BaseWaitTime * GuiltFactor;

	UE_LOG(LogTemp, Warning, TEXT("[System] Will transform into a Vampire in %f seconds."), FinalWaitTime);
	GetWorldTimerManager().SetTimer(TransformTimerHandle, this, &AMainCharacter::TransformToVampire, FinalWaitTime, false);
}

void AMainCharacter::TransformToVampire()
{
	if (PermanentState == EPermanentState::PureHuman) return;
	if (TransformMontage)
	{
		PlayAnimMontage(TransformMontage);
	}
	SetIsVampire(true);
}

void AMainCharacter::OnDiscoveredByNPC(AActor* NPC)
{

	UE_LOG(LogTemp, Warning, TEXT("[OnDiscoveredByNPC] bIsDead=%d bIsVampire=%d"),
		bIsDead,
		bIsVampire
	);

	if (bIsDead) return;

	if (!bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Discovered by NPC, but player is human. No game over."));
		return;
	}

	if (bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Discovered by NPC! Game Over."));
		bIsDead = true;

		GetCharacterMovement()->DisableMovement();

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}

		OnDeath();
	}
}

void AMainCharacter::PlayTransformCameraShake()
{
	if (!TransformCameraShakeClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	PC->PlayerCameraManager->StartCameraShake(TransformCameraShakeClass);
}

/* --- Percent ��ȯ ���� --- */
float AMainCharacter::GetGuiltPercent() const { return Guilt / 100.0f; }
float AMainCharacter::GetInvestPercent() const { return Invest / 100.0f; }
float AMainCharacter::GetBloodPercent() const { return MaxBlood > 0.0f ? Blood / MaxBlood : 0.0f; }
float AMainCharacter::GetStaminaPercent() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }
