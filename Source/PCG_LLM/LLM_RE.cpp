// Fill out your copyright notice in the Description page of Project Settings.


#include "LLM_RE.h"
#include "HttpModule.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Interfaces/IHttpResponse.h"

DEFINE_LOG_CATEGORY(LogLLM);

// Sets default values
ALLM_RE::ALLM_RE()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void ALLM_RE::RequestMapAnalysis(const FLLMPlayerState& PlayerState)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		UE_LOG(LogLLM, Error, TEXT("HTTP 모듈을 사용할 수 없습니다."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

	// 1. URL 및 헤더 설정
	Request->SetURL(ApiUrl);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");

	// 2. 데이터 직렬화 (Struct -> JSON String)
	FString PlayerDataJson = SerializePlayerState(PlayerState);
    
	// 서버 규격에 맞춰 { "data": "..." } 형태로 포장
	// (서버가 req.body.data로 받기 때문)
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetStringField("data", PlayerDataJson);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	// 3. 응답 처리 (Weak Pointer를 사용한 안전한 바인딩)
	// 액터가 파괴된 후 응답이 와도 크래시가 나지 않도록 방어 코드 작성
	TWeakObjectPtr<ALLM_RE> WeakThis = this;

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnected)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->HandleResponse(Res, bConnected);
			}
		});

	UE_LOG(LogLLM, Log, TEXT("LLM 요청 전송: %s"), *ApiUrl);
	Request->ProcessRequest();
}

// Called when the game starts or when spawned
void ALLM_RE::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALLM_RE::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALLM_RE::HandleResponse(FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogLLM, Error, TEXT("서버 통신 실패"));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogLLM, Error, TEXT("서버 에러 코드: %d"), Response->GetResponseCode());
		return;
	}

	// JSON 파싱 시작
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		// 서버 응답 { "status": "ok", "answer": "{ ...GPT JSON... }" }
		// 여기서 "answer" 필드를 꺼냅니다.
		FString GPTAnswerString = JsonObject->GetStringField("answer");

		// GPT가 준 JSON 문자열을 구조체로 변환
		FLLMMapParams ResultParams = ParseMapParams(GPTAnswerString);

		// 결과 브로드캐스트 (블루프린트로 전송)
		UE_LOG(LogLLM, Log, TEXT("분석 완료. 분위기: %s"), *ResultParams.AtmosphereTag.ToString());
		OnMapParamsReceived.Broadcast(ResultParams);
	}
}

FString ALLM_RE::SerializePlayerState(const FLLMPlayerState& PlayerState)
{
	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
    
	JsonObj->SetNumberField("health", PlayerState.CurrentHealth);
	JsonObj->SetNumberField("killCount", PlayerState.KillCount);
	JsonObj->SetNumberField("averageKillTime", PlayerState.AverageKillTime);
	JsonObj->SetBoolField("isDying", PlayerState.bIsDying);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

	return OutputString;
}

FLLMMapParams ALLM_RE::ParseMapParams(const FString& JsonString)
{
	FLLMMapParams Params;
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObj))
	{
		Params.Analysis = JsonObj->GetStringField("analysis");
		Params.DifficultyMultiplier = JsonObj->GetNumberField("difficultyMultiplier");
		Params.EnemySpawnRate = JsonObj->GetNumberField("enemySpawnRate");
		Params.MapComplexity = JsonObj->GetNumberField("mapComplexity");

		// [중요] 문자열을 GameplayTag로 변환하는 과정
		// JSON의 "Dark_Foggy" 문자열을 받아서 태그로 변환
        
		FString AggroStr = JsonObj->GetStringField("enemyAggression");
		Params.EnemyAggressionTag = FGameplayTag::RequestGameplayTag(FName(*AggroStr));

		FString ObstacleStr = JsonObj->GetStringField("obstacleType");
		Params.ObstacleTypeTag = FGameplayTag::RequestGameplayTag(FName(*ObstacleStr));

		FString AtmosStr = JsonObj->GetStringField("atmosphere");
		Params.AtmosphereTag = FGameplayTag::RequestGameplayTag(FName(*AtmosStr));
	}
	else
	{
		UE_LOG(LogLLM, Warning, TEXT("GPT 응답 파싱 실패 (JSON 형식 오류 가능성)"));
	}

	return Params;
}

