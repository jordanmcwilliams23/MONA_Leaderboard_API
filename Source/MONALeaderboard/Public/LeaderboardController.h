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

//Delegates for broadcasting top scores, specific info, etc.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTopScoresReceived, const FScores&, TopScores);
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class MONALEADERBOARD_API ULeaderboardController : public UObject
{
	GENERATED_BODY()
public:
	//Get Singleton
	UFUNCTION(BlueprintPure, Category= "LeaderboardController")
	static ULeaderboardController* GetLeaderboardController();

	//Setters
	UFUNCTION(BlueprintCallable, Category= "LeaderboardController")
	void SetApplicationID(const FString& InApplicationID) { ApplicationID = InApplicationID; }

	UFUNCTION(BlueprintCallable, Category= "LeaderboardController")
	void SetMonaAPISecret(const FString& InMonaAPISecret) { MonaApiSecret = InMonaAPISecret; }

	//Authorization
	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void GenerateOTP(const FString& Email);

	UFUNCTION(BlueprintCallable, Category= "Authorization")
	void VerifyOTP(const FString& Email, const FString& OTP);


	//Leaderboard
	UFUNCTION(BlueprintCallable)
	void GetTopScores();

	UFUNCTION(BlueprintCallable)
	void PostScore(const FString& Username, const double Score);

	bool ValidAppID() const;

	bool ValidResponse(const FHttpResponsePtr& Response) const;

	UPROPERTY(BlueprintAssignable)
	FOnTopScoresReceived OnTopScoresReceived;
protected:
	
	UPROPERTY(BlueprintReadOnly)
	FString ApplicationID = "";

	UPROPERTY(BlueprintReadOnly)
	FString MonaApiSecret;

	UPROPERTY(BlueprintReadWrite)
	int NumTopScoresToGet = 50;

	virtual void FinishDestroy() override;

	UFUNCTION(BlueprintCallable)
	void GetUser(const FString& BearerToken);
private:
	static ULeaderboardController* Instance;

	//Response Callbacks
	void TopScoresResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void PostScoreResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void GenerateOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void VerifyOTPResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void GetUserResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	FString AccessToken = "";

	FString RefreshToken = "";
	
};
