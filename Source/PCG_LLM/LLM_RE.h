// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interfaces/IHttpRequest.h"

#include "LLM_RE.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLLM, Log, All);

// 플레이어 상태 데이터
USTRUCT(BlueprintType)
struct FLLMPlayerState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 KillCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AverageKillTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDying = false;
};

// 멥 생성 파라미터
USTRUCT(BlueprintType)
struct FLLMMapParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Analysis = "";

	UPROPERTY(BlueprintReadOnly)
	float DifficultyMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	float EnemySpawnRate = 0.5f;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EnemyAggressionTag;

	UPROPERTY(BlueprintReadOnly)
	float MapComplexity = 0.5f;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag ObstacleTypeTag;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AtmosphereTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLLMResponseReceived, const FLLMMapParams&, MapParams);

UCLASS()
class PCG_LLM_API ALLM_RE : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALLM_RE();

	// LLM 응답 시 호출하는 델리게이트
	// 다른 함수들을 실행시키는 리모컨같은 역할..
	UPROPERTY(BlueprintAssignable, Category = "LLM|Events")
	FOnLLMResponseReceived OnMapParamsReceived;

	// API 주소를 에디터에서 변경 가능하게 노출
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Config")
	FString ApiUrl = TEXT("http://localhost:3000/api/openai");

	// 서버에 현재 플레이어의 상태 분석 요청
	UFUNCTION(BlueprintCallable, Category = "LLM|Requests")
	void RequestMapAnalysis(const FLLMPlayerState& PlayerState);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 내부 로직 함수들 
	void HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful);
	FString SerializePlayerState(const FLLMPlayerState& PlayerState);
	FLLMMapParams ParseMapParams(const FString& JsonString);
};
