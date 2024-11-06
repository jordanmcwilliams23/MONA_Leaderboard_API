// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <mutex>
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "LeaderboardController.generated.h"

USTRUCT(BlueprintType)
struct FUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	FString Username;

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	FString Name;
};

USTRUCT(BlueprintType)
struct FUserInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	int ID;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	FUser User;

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	int Score;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	FString Topic;
	
	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	FString Created_At;

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	int Rank;
};

USTRUCT(BlueprintType)
struct FScores
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	TArray<FUserInfo> Items;

	UPROPERTY(BlueprintReadOnly, Category = "Score Info")
	int Count;
};

//Delegates for broadcasting top scores, OTP Verified, etc.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTopScoresReceived, const FScores&, TopScores);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOTPVerified);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOTPSent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnScorePosted);
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class ULeaderboardController : public UObject
{
	GENERATED_BODY()
public:
	//Get Singleton
	UFUNCTION(BlueprintPure, Category= "LeaderboardController")
	static ULeaderboardController* GetLeaderboardController();

	//Setters
	UFUNCTION(BlueprintCallable, Category= "LeaderboardController")
	void SetApplicationID(const FString& InApplicationID) { ApplicationID = InApplicationID; }

	UFUNCTION(BlueprintCallable, Server, Reliable, Category= "LeaderboardController")
	void ServerSetSDKSecret(const FString& InSDKSecret);

	//Authorization
	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void GenerateOTP(const FString& Email);

	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void VerifyOTP(const FString& Email, const FString& OTP);

	UFUNCTION(BlueprintCallable, Client, Reliable, Category= "Authorization")
	void RefreshAccessToken();
	
	//Leaderboard
	UFUNCTION(BlueprintCallable, Category= "LeaderboardController")
	void GetTopScores();
	
	UFUNCTION(BlueprintCallable, Client, Reliable, Category= "LeaderboardController")
	void ClientPostScore(const float Score, const FString& Topic = "", const FString& InSDKSecret = "");

	//Make sure App ID is set
	bool ValidAppID() const;

	bool ValidResponse(const FHttpResponsePtr& Response);

	bool ValidAuthorization() const;

	//Delegates (Events) that can be assigned from blueprint and C++
	UPROPERTY(BlueprintAssignable, Category= "LeaderboardController")
	FOnTopScoresReceived OnTopScoresReceived;

	UPROPERTY(BlueprintAssignable, Category= "LeaderboardController")
	FOnOTPVerified OnOtpVerified;
	
	UPROPERTY(BlueprintAssignable, Category= "LeaderboardController")
	FOnOTPSent OnOtpSent;

	UPROPERTY(BlueprintAssignable, Category= "LeaderboardController")
	FOnScorePosted OnScorePosted;

	static FString GenerateHmac(const FString& Message, const FString& Key);
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Transient, Category= "Authorization")
	FString ApplicationID = "";

	UPROPERTY(BlueprintReadOnly, Transient, Category= "Authorization")
	FString SDKSecret;

	UPROPERTY(BlueprintReadWrite, Category= "LeaderboardController")
	int NumTopScoresToGet = 50;

	virtual void FinishDestroy() override;

	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void GetUser(const FString& BearerToken);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Debug")
	bool bShowDebug = true;
private:
	static ULeaderboardController* Instance;

	//Response Callbacks
	void TopScoresResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void ClientPostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void GenerateOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void VerifyOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void GetUserResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void RefreshAccessTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	FString AccessToken;
	FString RefreshToken;
	std::mutex Mutex;
	
};
