#include "SpartaGameState.h"
#include "SpartaGameInstance.h"
#include "SpartaPlayerController.h"
#include "SpawnVolume.h"
#include "CoinItem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"

ASpartaGameState::ASpartaGameState()
{
	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	LevelDuration = 30.0f;
	CurrentLevelIndex = 0;
	MaxLevels = 3;
	CurrentWaveIndex = 0; // 추가

	// 웨이브 설정 (시간, 코인, 포션, 지뢰 순서)
	WaveConfigs.Add({ 60.0f, 10, 3, 10 }); // 1웨이브
	WaveConfigs.Add({ 45.0f, 10, 2, 15 }); // 2웨이브
	WaveConfigs.Add({ 30.0f, 10, 1, 20 }); // 3웨이브
}

void ASpartaGameState::BeginPlay()
{
	Super::BeginPlay();

	StartLevel();

	GetWorldTimerManager().SetTimer(
		HUDUpdateTimerHandle,
		this,
		&ASpartaGameState::UpdateHUD,
		0.1f,
		true
	);
}

int32 ASpartaGameState::GetScore() const
{
	return Score;
}

void ASpartaGameState::AddScore(int32 Amount)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToScore(Amount);
		}
	}
}

void ASpartaGameState::StartLevel()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->ShowGameHUD();
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			CurrentLevelIndex = SpartaGameInstance->CurrentLevelIndex;
		}
	}

	CurrentWaveIndex = 0; // 레벨 시작 시 1웨이브(0번 인덱스)부터 시작
	StartWave();          // 실제 스폰과 타이머는 여기서 처리
}

void ASpartaGameState::StartWave()
{
	// 바로 실행하는 대신, 0.1초 뒤에 실제 스폰 로직(실행될 내부 함수)을 호출하도록 합니다.
	// 기존 로직을 별도의 람다나 함수로 감싸서 타이머로 실행합니다.

	GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			if (!WaveConfigs.IsValidIndex(CurrentWaveIndex)) return;

			FWaveConfiguration& Config = WaveConfigs[CurrentWaveIndex];
			SpawnedCoinCount = Config.CoinCount;
			CollectedCoinCount = 0;
			LevelDuration = Config.Duration;

			TArray<AActor*> FoundVolumes;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

			// 1. 화면에 즉시 알림 출력 (GEngine)
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,                 // 메시지 ID (새 메시지로 계속 띄움)
					5.0f,               // 표시 시간 (5초)
					FColor::Yellow,     // 글자 색상
					FString::Printf(TEXT("Wave %d !"), CurrentWaveIndex + 1)
				);
			}

			// 2. 출력 로그에 기록 (UE_LOG)
			UE_LOG(LogTemp, Warning, TEXT("Wave %d Started!"), CurrentWaveIndex + 1);

			if (FoundVolumes.Num() > 0)
			{
				ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
				if (SpawnVolume)
				{
					SpawnVolume->SpawnWaveItems(Config.CoinCount, Config.PotionCount, Config.MineCount);
				}
			}

			// 타이머는 여기서 시작 (아이템이 확실히 나온 뒤에 시간이 흐르도록)
			GetWorldTimerManager().SetTimer(LevelTimerHandle, this, &ASpartaGameState::OnLevelTimeUp, LevelDuration, false);
		});
}


// 이렇게 수정하세요
void ASpartaGameState::OnLevelTimeUp()
{
	OnGameOver(); // 제한 시간이 다 되면 게임 오버 UI를 띄웁니다.
}

// 1. 수집 시 웨이브 클리어 조건 체크로 변경
void ASpartaGameState::OnCoinCollected()
{
	CollectedCoinCount++;

	if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
	{
		HandleWaveClear(); // 다음 레벨로 바로 가는 대신, 웨이브 클리어 함수 호출!
	}
}

void ASpartaGameState::HandleWaveClear()
{
	GetWorldTimerManager().ClearTimer(LevelTimerHandle);

	if (CurrentWaveIndex < 2) // 아직 1, 2웨이브라면
	{
		CurrentWaveIndex++;
		StartWave();
	}
	else // 3웨이브까지 다 깼을 때!
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (ASpartaPlayerController* SpartaPC = Cast<ASpartaPlayerController>(PC))
			{
				// 마지막 레벨(3레벨)의 마지막 웨이브라면 '게임 클리어'
				if (CurrentLevelIndex >= MaxLevels - 1)
				{
					SpartaPC->ShowGameClearMenu();
				}
				else // 그 외 레벨 클리어라면 '다음 레벨 버튼 UI'
				{
					SpartaPC->ShowLevelClearMenu();
				}
			}
		}
	}
}

void ASpartaGameState::GoToNextLevel()
{
	// 이동 전 타이머 혹시 모르니 정리
	GetWorldTimerManager().ClearTimer(LevelTimerHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			// 현재 레벨까지의 점수를 GameInstance에 최종 합산
			AddScore(Score);

			// 다음 레벨 인덱스로 증가
			CurrentLevelIndex++;
			SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;

			// 만약 모든 레벨(3레벨)을 다 깼다면
			if (CurrentLevelIndex >= MaxLevels)
			{
				// 게임 클리어 UI를 띄워야 하지만, 우선은 OnGameOver로 처리
				OnGameOver();
				return;
			}

			// 다음 맵 이름이 배열에 있다면 해당 맵 열기
			if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
			{
				UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
			}
			else
			{
				// 맵 이름이 없으면 게임 종료 처리
				OnGameOver();
			}
		}
	}
}


void ASpartaGameState::OnGameOver()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->SetPause(true);
			SpartaPlayerController->ShowMainMenu(true);
		}
	}
}

void ASpartaGameState::UpdateHUD()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController);
		{
			if (UUserWidget* HUDWidget = SpartaPlayerController->GetHUDWidget())
			{
				if (UTextBlock* TimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
				{
					float RemainingTime = GetWorldTimerManager().GetTimerRemaining(LevelTimerHandle);
					TimeText->SetText(FText::FromString(FString::Printf(TEXT("Time: %.1f"), RemainingTime)));
				}

				if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
				{
					if (UGameInstance* GameInstance = GetGameInstance())
					{
						USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
						if (SpartaGameInstance)
						{
							ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), SpartaGameInstance->TotalScore)));
						}
					}
				}

				if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
				{
					LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), CurrentLevelIndex + 1)));
				}
				// SpartaGameState.cpp 내 UpdateHUD 함수 중
				if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
				{
					// 기존 Level 텍스트 아래나 옆에 Wave 정보 추가
					LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d | Wave: %d/3"), CurrentLevelIndex + 1, CurrentWaveIndex + 1)));
				}
			}
		}
	}
}