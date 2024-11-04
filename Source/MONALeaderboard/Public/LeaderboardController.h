// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "LeaderboardController.generated.h"

/**
 * 
 */
UCLASS()
class MONALEADERBOARD_API ULeaderboardController : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category= "LeaderboardController")
	static ULeaderboardController* GetLeaderboardController();

	UFUNCTION(BlueprintCallable, Category= "LeaderboardController")
	void SetApplicationID(const FString& InApplicationID) { ApplicationID = InApplicationID; }

	void SetMonaAPISecret(const FString& InMonaAPISecret) {MonaApiSecret = InMonaAPISecret; }

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

	
protected:
	
	UPROPERTY(BlueprintReadWrite)
	FString ApplicationID;

	UPROPERTY(BlueprintReadWrite)
	FString MonaApiSecret;
private:
	static ULeaderboardController* Instance;	

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	
};
