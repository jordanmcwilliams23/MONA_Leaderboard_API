// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "LeaderboardController.generated.h"

USTRUCT(BlueprintType)
struct FUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Username;

	UPROPERTY(BlueprintReadOnly)
	FString Name;
};

USTRUCT(BlueprintType)
struct FUserInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int ID;
	
	UPROPERTY(BlueprintReadOnly)
	FUser User;

	UPROPERTY(BlueprintReadOnly)
	int Score;
	
	UPROPERTY(BlueprintReadOnly)
	FString Topic;
	
	UPROPERTY(BlueprintReadOnly)
	FString Created_At;

	UPROPERTY(BlueprintReadOnly)
	int Rank;
};

USTRUCT(BlueprintType)
struct FScores
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FUserInfo> Items;

	UPROPERTY(BlueprintReadOnly)
	int Count;
};

//Delegates for broadcasting top scores, OTP Verified, etc.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTopScoresReceived, const FScores&, TopScores);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOTPVerified);
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
	void ServerSetMonaAPISecret(const FString& InMonaAPISecret);

	//Authorization
	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void GenerateOTP(const FString& Email);

	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void VerifyOTP(const FString& Email, const FString& OTP);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void RefreshAccessToken();


	//Leaderboard
	UFUNCTION(BlueprintCallable)
	void GetTopScores();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerPostScore(const FString& Username, const double Score);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void ClientPostScore(const FString& Username, const double Score);

	//Make sure App ID is set
	bool ValidAppID() const;

	bool ValidResponse(const FHttpResponsePtr& Response, const bool bRefreshAccessToken = false);

	bool ValidAuthorization() const;

	UPROPERTY(BlueprintAssignable)
	FOnTopScoresReceived OnTopScoresReceived;

	UPROPERTY(BlueprintAssignable)
	FOnOTPVerified OnOtpVerified;
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Transient)
	FString ApplicationID = "";

	UPROPERTY(BlueprintReadOnly, Transient)
	FString MonaApiSecret;

	UPROPERTY(BlueprintReadWrite)
	int NumTopScoresToGet = 50;

	virtual void FinishDestroy() override;

	UFUNCTION(BlueprintCallable)
	void GetUser(const FString& BearerToken);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bShowDebug = true;
private:
	static ULeaderboardController* Instance;

	//Response Callbacks
	void TopScoresResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void ServerPostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void ClientPostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void GenerateOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void VerifyOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void GetUserResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void RefreshAccessTokenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	FString AccessToken = "";

	FString RefreshToken = "";
	
};
