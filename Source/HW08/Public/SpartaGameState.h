#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpartaGameState.generated.h"

// 웨이브별 설정을 담는 구조체
USTRUCT(BlueprintType)
struct FWaveConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration;    // 제한 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CoinCount;   // 코인 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PotionCount; // 포션 개수
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MineCount;   // 폭탄 개수
};

UCLASS()
class HW08_API ASpartaGameState : public AGameState
{
    GENERATED_BODY()

public:
    ASpartaGameState();

    virtual void BeginPlay() override;

    // --- 기존 변수 유지 ---
    UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "Score")
    int32 Score;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
    int32 SpawnedCoinCount;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
    int32 CollectedCoinCount;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
    float LevelDuration;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
    int32 CurrentLevelIndex;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
    int32 MaxLevels;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    TArray<FName> LevelMapNames;

    // --- 웨이브 관련 추가 ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
    int32 CurrentWaveIndex; // 현재 웨이브 (0, 1, 2)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
    TArray<FWaveConfiguration> WaveConfigs; // 웨이브 설정 데이터

    FTimerHandle LevelTimerHandle;
    FTimerHandle HUDUpdateTimerHandle;

    UFUNCTION(BlueprintPure, Category = "Score")
    int32 GetScore() const;
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(int32 Amount);

    // UI에서 호출할 레벨 전환 함수
    UFUNCTION(BlueprintCallable, Category = "Level")
    void GoToNextLevel();

    UFUNCTION(BlueprintCallable, Category = "Level")
    void OnGameOver();

    // --- 로직 함수 수정 및 추가 ---
    void StartLevel();
    void StartWave();        // 신규: 웨이브 시작 로직
    void HandleWaveClear();  // 신규: 웨이브 클리어 시 처리
    void OnLevelTimeUp();
    void OnCoinCollected();
    void EndLevel();
    void UpdateHUD();
};
