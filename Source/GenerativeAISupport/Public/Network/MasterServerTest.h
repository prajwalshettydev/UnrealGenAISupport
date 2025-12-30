// Master Server Test Actor - Zum Testen der Server-Verbindung
// Platziere diesen Actor im Level und drücke L zum Login-Test

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MasterServerClient.h"
#include "MasterServerTest.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GENERATIVEAISUPPORT_API AMasterServerTest : public AActor
{
	GENERATED_BODY()

public:
	AMasterServerTest();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Test-Login Credentials */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	FString TestUsername = TEXT("testuser");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	FString TestPassword = TEXT("testpassword123");

	/** Server URL */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	FString ServerURL = TEXT("http://192.168.178.73:9877");

	/** Automatisch beim Start verbinden? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	bool bAutoConnect = false;

	/** Test Login */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestLogin();

	/** Test Registration */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestRegister(const FString& Username, const FString& Email, const FString& Password);

	/** Test Get Characters */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestGetCharacters();

	/** Test Create Character */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestCreateCharacter(const FString& Name, const FString& ClassName);

	/** Test Get Server List */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestGetServerList();

	/** Logout */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestLogout();

protected:
	/** Cached reference to MasterServerClient */
	UPROPERTY()
	UMasterServerClient* ServerClient;

	void OnLoginComplete(const FAuthResponse& Response);
	void OnRegisterComplete(const FAuthResponse& Response);
	void OnCharacterListComplete(const FCharacterListResponse& Response);
	void OnServerListComplete(const FServerListResponse& Response);
	void OnAPIComplete(const FAPIResponse& Response);

	void PrintToScreen(const FString& Message, FColor Color = FColor::Green);
};
