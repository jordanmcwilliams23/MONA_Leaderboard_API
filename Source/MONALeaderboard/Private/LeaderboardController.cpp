// Fill out your copyright notice in the Description page of Project Settings.


#include "LeaderboardController.h"
#include "http.h"
#include "Json.h"

//Singleton
ULeaderboardController* ULeaderboardController::Instance = nullptr;

ULeaderboardController* ULeaderboardController::GetLeaderboardController()
{
	//Create LeaderboardController Singleton if it does not exist already
	if (Instance == nullptr)
	{
		Instance = NewObject<ULeaderboardController>();
	}
	return Instance;
}

void ULeaderboardController::GenerateOTP(const FString& Email)
{
	if (ApplicationID.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController ApplicationID has not been set"));
		return;
	}
	//Setup Request Body	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("email", Email);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);
	
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::OnResponseReceived);
	//Format API call
	Request->SetURL("https://api.monaverse.com/public/auth/otp/generate");
	//Set Header Info
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);
	Request->SetHeader("content-type", "application/json");
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void ULeaderboardController::VerifyOTP(const FString& Email, const FString& OTP)
{
	if (ApplicationID.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController ApplicationID has not been set"));
		return;
	}
	//Setup Request Body	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("email", Email);
	RequestObj->SetStringField("otp", OTP);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);
	
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::OnResponseReceived);
	//Format API call
	Request->SetURL("https://api.monaverse.com/public/auth/otp/verify");
	//Set Header Info
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);
	Request->SetHeader("content-type", "application/json");
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void ULeaderboardController::GetTopScores()
{
	if (ApplicationID.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController ApplicationID has not been set"));
		return;
	}
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("GET");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::OnResponseReceived);
	//Format API call
	Request->SetURL(FString::Printf(TEXT("https://api.monaverse.com/public/leaderboards/%s/top-scores"), *ApplicationID));

	Request->ProcessRequest();
}

void ULeaderboardController::PostScore(const FString& Username, const double Score)
{
	if (MonaApiSecret.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController MONA API Secret has not been set"));
		return;
	}
	
	//Setup Request Body	
    TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
    RequestObj->SetStringField("username", Username);
    RequestObj->SetNumberField("score", Score);
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RequestObj, Writer);
	
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::OnResponseReceived);
	//Format API call
	Request->SetURL(FString::Printf(TEXT("https://api.monaverse.com/public/leaderboards/%s/scores"), *ApplicationID));
	//Set Header Info
	Request->SetHeader("accept", "application/json");
	Request->SetHeader("X-Mona-Api-Secret", MonaApiSecret);
	Request->SetHeader("content-type", "application/json");
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void ULeaderboardController::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;

	//Read response content as JSON
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	UE_LOG(LogTemp, Display, TEXT("Response %s"), *Response->GetContentAsString());
}
