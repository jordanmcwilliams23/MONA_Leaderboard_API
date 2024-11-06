
#include "LeaderboardController.h"
#include "http.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"

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

void ULeaderboardController::ServerSetMonaAPISecret_Implementation(const FString& InMonaAPISecret)
{
	MonaApiSecret = InMonaAPISecret;
}

void ULeaderboardController::GenerateOTP(const FString& Email)
{
	if (!ValidAppID()) return;
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
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::GenerateOTPResponseReceived);
	//Format API call
	Request->SetURL("https://api.monaverse.com/public/auth/otp/generate");
	//Set Header Info
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);
	Request->AppendToHeader("content-type", "application/json");
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void ULeaderboardController::VerifyOTP(const FString& Email, const FString& OTP)
{
	//return if invalid App ID or empty OTP
	if (!ValidAppID()) return;
	if (OTP.IsEmpty()) return;
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
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::VerifyOTPResponseReceived);
	//Format API call
	Request->SetURL("https://api.monaverse.com/public/auth/otp/verify");
	//Set Header Info
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);
	Request->AppendToHeader("content-type", "application/json");
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void ULeaderboardController::RefreshAccessToken_Implementation()
{
	if (!ValidAppID() || !ValidAuthorization()) return;
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("refresh", RefreshToken);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("GET");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::RefreshAccessTokenResponseReceived);
	FString url = TEXT("https://api.monaverse.com/public/auth/token/refresh");
	Request->SetURL(url);
	Request->SetContentAsString(RequestBody);
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);

	Request->ProcessRequest();
}

void ULeaderboardController::GetTopScores()
{
	if (!ValidAppID()) return;
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("GET");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::TopScoresResponseReceived);
	//Format API call
	FString url = FString::Printf(TEXT("https://api.monaverse.com/public/leaderboards/%s/top-scores"), *ApplicationID);
	//Set limit of scores to get
	url.Append("?limit=");
	url.AppendInt(NumTopScoresToGet);
	Request->SetURL(url);
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);

	Request->ProcessRequest();
}

void ULeaderboardController::ServerPostScore_Implementation(const FString& Username, const double Score)
{
	if (MonaApiSecret.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController MONA API Secret has not been set"));
		return;
	}
	if (!ValidAppID()) return;
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
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::ServerPostScoreResponseReceived);
	//Format API call
	Request->SetURL(FString::Printf(TEXT("https://api.monaverse.com/public/leaderboards/%s/scores"), *ApplicationID));
	//Set Header Info
	Request->SetHeader("accept", "application/json");
	Request->AppendToHeader("X-Mona-Api-Secret", MonaApiSecret);
	Request->AppendToHeader("X-Mona-Application-Id", ApplicationID);
	Request->AppendToHeader("content-type", "application/json");
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void ULeaderboardController::ClientPostScore_Implementation(const FString& Username, const double Score)
{
	if (!ValidAppID() || !ValidAuthorization()) return;
	//Setup Request Body	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	FDateTime Timestamp = FDateTime::UtcNow();
	RequestObj->SetNumberField("score", Score);
	RequestObj->SetStringField("timestamp", Timestamp.ToString());
	RequestObj->SetStringField("signature", Username);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);
	
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::ClientPostScoreResponseReceived);
	//Format API call
	Request->SetURL(FString::Printf(TEXT("https://api.monaverse.com/public/leaderboards/%s/scores"), *ApplicationID));
	//Set Header Info
	Request->SetHeader("accept", "application/json");
	Request->AppendToHeader("X-Mona-Application-Id", ApplicationID);
	Request->AppendToHeader("content-type", "application/json");
	Request->AppendToHeader("Authorization", "Bearer " + AccessToken);
	//Set Request Body
	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

bool ULeaderboardController::ValidAppID() const
{
	if (ApplicationID.IsEmpty())
	{
		//Debug messages
		if (bShowDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController ApplicationID has not been set"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
				   22,
				   3.f,
				   FColor::Red,
				   TEXT("Error: LeaderboardController ApplicationID has not been set"));
			}
		}
		return false;
	}
	return true;
}

bool ULeaderboardController::ValidResponse(const FHttpResponsePtr& Response, const bool bRefreshAccessToken)
{
	//200 is valid response code here
	if (Response->GetResponseCode() != 200)
	{
		if (bRefreshAccessToken)
		{
			RefreshAccessToken();
		}
		//Debug messages
		if (GEngine && bShowDebug)
			GEngine->AddOnScreenDebugMessage(22, 3.f, FColor::Red, FString::Printf(TEXT("Error: Invalid Response Code: %d"), Response->GetResponseCode()));
		return false;
	}
	return true;
}

bool ULeaderboardController::ValidAuthorization() const
{
	if (AccessToken.IsEmpty() || RefreshToken.IsEmpty())
	{
		if (GEngine && bShowDebug)
			GEngine->AddOnScreenDebugMessage(22, 3.f, FColor::Red, TEXT("Error: AccessToken / RefreshToken has not been validated"));
		return false;
	}
	return true;
}

void ULeaderboardController::FinishDestroy()
{
	UObject::FinishDestroy();
	//Delete singleton
	Instance = nullptr;
}

void ULeaderboardController::GetUser(const FString& BearerToken)
{
	if (!ValidAppID()) return;
	if (AccessToken.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: Leaderboard Controller Access Token has not been set"));
		return;
	}
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("GET");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::GetUserResponseReceived);
	//Format API call
	FString url = "https://api.monaverse.com/public/user/";
	//Set limit of scores to get
	Request->SetURL(url);
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);
	Request->AppendToHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AccessToken));

	Request->ProcessRequest();
}

void ULeaderboardController::TopScoresResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                       bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response)) return;
	//Read response content as JSON
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	//Convert JSON object into custom struct to hold info
	FScores AllScores;
	if (FJsonObjectConverter::JsonObjectToUStruct<FScores>(ResponseObj.ToSharedRef(), &AllScores))
	{
		//Broadcast struct with info. No cyclical dependencies / hard references here :)
		//This delegate can be bound to from any other C++ class or blueprint
		OnTopScoresReceived.Broadcast(AllScores);
	} else
	{
		UE_LOG(LogTemp, Display, TEXT("Object Conversion Failed"));
	}
}

void ULeaderboardController::ServerPostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response)) return;
}

void ULeaderboardController::ClientPostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response, true)) return;
}

void ULeaderboardController::GenerateOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response)) return;
}

void ULeaderboardController::VerifyOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response, false)) return;
	//Read response content as JSON
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	//Get Access Token
	FString OutAccessToken;
	if (ResponseObj->TryGetStringField("access", OutAccessToken))
	{
		AccessToken = OutAccessToken;
	}
	//Get Refresh Token
	FString OutRefreshToken;
	if (ResponseObj->TryGetStringField("refresh", OutRefreshToken))
	{
		RefreshToken = OutRefreshToken;
	}
	if (!AccessToken.IsEmpty() && !RefreshToken.IsEmpty())
	{
		RefreshAccessToken();
		OnOtpVerified.Broadcast();
	}
}

void ULeaderboardController::GetUserResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response)) return;
}

void ULeaderboardController::RefreshAccessTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
}
