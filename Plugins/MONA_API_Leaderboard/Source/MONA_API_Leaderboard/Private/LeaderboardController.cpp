
#include "LeaderboardController.h"
#include "http.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#include "openssl/ssl.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"
THIRD_PARTY_INCLUDES_END
#undef UI
#include "Misc/Base64.h"
#include "Async/Async.h"

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

void ULeaderboardController::ServerSetSDKSecret_Implementation(const FString& InSDKSecret)
{
	SDKSecret = InSDKSecret;
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
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
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
	std::lock_guard Lock(Mutex);
	if (!ValidAppID() || !ValidAuthorization()) return;
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	RequestObj->SetStringField("refresh", RefreshToken);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);
	TSharedPtr<FHttpResponsePtr> TokenResponse = MakeShared<FHttpResponsePtr>();
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindLambda([&](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
		{
			FString ResponseString = Response->GetContentAsString();
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseString);

			if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
			{
				AccessToken = JsonObject->GetStringField("access");
			}
		}
	});
	FString url = TEXT("https://api.monaverse.com/public/auth/token/refresh");
	Request->SetURL(url);
	Request->SetContentAsString(RequestBody);
	Request->SetHeader("X-Mona-Application-Id", ApplicationID);

	Request->ProcessRequest();
}

/* void ULeaderboardController::GetTopScores()
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
} */



void ULeaderboardController::GetTopScores(
    bool featured, 
    FString topic, 
    ELeaderboardPeriod period, 
    ELeaderboardSortingOrder order, 
    FString startTime, 
    FString endTime, 
    bool includeAllUsersScores)
{
    if (!ValidAppID()) return;
    
    // Setup Request
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetVerb("GET");
    
    // Bind Response Received Callback
    Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::TopScoresResponseReceived);
    
    // Format API call
    FString url = FString::Printf(TEXT("https://api.monaverse.com/public/leaderboards/%s/top-scores"), *ApplicationID);
    
    TArray<FString> QueryParams;
    
    // Append optional parameters
    if (featured)
    {
        QueryParams.Add(FString("featured=true"));
    }
    if (!topic.IsEmpty())
    {
        QueryParams.Add(FString::Printf(TEXT("topic=%s"), *topic));
    }
    switch (period)
    {
        case ELeaderboardPeriod::daily:
            QueryParams.Add(FString("period=daily"));
            break;
        case ELeaderboardPeriod::weekly:
            QueryParams.Add(FString("period=weekly"));
            break;
        case ELeaderboardPeriod::monthly:
            QueryParams.Add(FString("period=monthly"));
            break;
        case ELeaderboardPeriod::all_time:
            QueryParams.Add(FString("period=all_time"));
            break;
    }
    switch (order)
    {
        case ELeaderboardSortingOrder::highest:
            QueryParams.Add(FString("order=highest"));
            break;
        case ELeaderboardSortingOrder::lowest:
            QueryParams.Add(FString("order=lowest"));
            break;
    }
    if (!startTime.IsEmpty())
    {
        QueryParams.Add(FString::Printf(TEXT("starttime=%s"), *startTime));
    }
    if (!endTime.IsEmpty())
    {
        QueryParams.Add(FString::Printf(TEXT("endtime=%s"), *endTime));
    }
    if (includeAllUsersScores)
    {
        QueryParams.Add(FString("include_all_users_scores=true"));
    }
    // Append limit of scores to get
    QueryParams.Add(FString::Printf(TEXT("limit=%d"), NumTopScoresToGet));
    
    // Append query parameters to URL
    if (!QueryParams.IsEmpty())
    {
        url.Append("?");
        url.Append(FString::Join(QueryParams, TEXT("&")));
    }
    
    Request->SetURL(url);
    Request->SetHeader("X-Mona-Application-Id", ApplicationID);

    Request->ProcessRequest();
}

void ULeaderboardController::ClientPostScore_Implementation(const float Score, const FString& Topic, const FString& InSDKSecret)
{
	if (!ValidAppID() || !ValidAuthorization()) return;
	//Setup Request Body	
	TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
	int64 Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
	if (!InSDKSecret.IsEmpty() && SDKSecret.IsEmpty())
	{
		ServerSetSDKSecret(InSDKSecret);
	}
	if (SDKSecret.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Error: LeaderboardController SDKSecret has not been set"));
		return;
	}
	const FString FormattedScore = FString::SanitizeFloat(Score, 3);
	const FString Message = FormattedScore + ":" + FString::Printf(TEXT("%lld"), Timestamp) + ":" + Topic;
	const FString Signature = GenerateHmac(Message, SDKSecret);
	RequestObj->SetNumberField("score", Score);
	RequestObj->SetStringField("timestamp", FString::Printf(TEXT("%lld"), Timestamp));
	RequestObj->SetStringField("signature", Signature);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestObj, Writer);
	
	//Setup Request
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("POST");
	//Bind Response Received Callback
	Request->OnProcessRequestComplete().BindUObject(this, &ULeaderboardController::ClientPostScoreResponseReceived);
	//Format API call
	Request->SetURL(TEXT("https://api.monaverse.com/public/leaderboards/sdk/score"));
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

bool ULeaderboardController::ValidResponse(const FHttpResponsePtr& Response)
{
	if (Response->GetResponseCode() == 401)
	{
		RefreshAccessToken();
	}
	//200 is valid response code here
	if (Response->GetResponseCode() != 200)
	{
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

FString ULeaderboardController::GenerateHmac(const FString& Message, const FString& Key)
{
	// Convert FString to std::string for OpenSSL compatibility
	std::string MessageStr = TCHAR_TO_UTF8(*Message);
	std::string KeyStr = TCHAR_TO_UTF8(*Key);

	// Compute HMAC SHA-256
	unsigned char* Hash = HMAC(EVP_sha256(),
							   reinterpret_cast<const unsigned char*>(KeyStr.c_str()),
							   KeyStr.length(),
							   reinterpret_cast<const unsigned char*>(MessageStr.c_str()),
							   MessageStr.length(),
							   nullptr,
							   nullptr);

	// Base64 encode the hash
	TArray<uint8> HashArray(Hash, SHA256_DIGEST_LENGTH);
	FString Base64Hash = FBase64::Encode(HashArray);

	return Base64Hash;
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

void ULeaderboardController::ClientPostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response.IsValid())
	{
		if (Response->GetResponseCode() == 200)
		{
			OnScorePosted.Broadcast();
		}
		if (Response->GetResponseCode() == 401)
		{
			FString ResponseString = Response->GetContentAsString();
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
			{
				std::lock_guard Lock(Mutex);
				UE_LOG(LogTemp, Warning, TEXT("401 Unauthorized - Refreshing Access Token"));
				RefreshAccessToken();
			});
		}
	}
}

void ULeaderboardController::GenerateOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		OnOtpSent.Broadcast();
	}
}

void ULeaderboardController::VerifyOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	if (!ValidResponse(Response)) return;
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
