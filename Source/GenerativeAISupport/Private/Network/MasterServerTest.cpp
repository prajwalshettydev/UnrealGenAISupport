// Master Server Test Actor Implementation

#include "Network/MasterServerTest.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AMasterServerTest::AMasterServerTest()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMasterServerTest::BeginPlay()
{
	Super::BeginPlay();

	// Get MasterServerClient
	ServerClient = UMasterServerClient::Get(this);

	if (ServerClient)
	{
		ServerClient->SetServerURL(ServerURL);
		PrintToScreen(FString::Printf(TEXT("Master Server Client bereit: %s"), *ServerURL), FColor::Cyan);

		if (bAutoConnect)
		{
			TestLogin();
		}
		else
		{
			PrintToScreen(TEXT("Druecke L fuer Login-Test"), FColor::Yellow);
		}
	}
	else
	{
		PrintToScreen(TEXT("FEHLER: MasterServerClient nicht gefunden!"), FColor::Red);
	}
}

void AMasterServerTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// L = Login Test
	if (GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(EKeys::L))
	{
		TestLogin();
	}
	// R = Register Test
	if (GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(EKeys::R))
	{
		TestRegister(TEXT("neuerspieler"), TEXT("neu@test.de"), TEXT("passwort123"));
	}
	// C = Characters Test
	if (GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(EKeys::C))
	{
		TestGetCharacters();
	}
	// S = Server List Test
	if (GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(EKeys::S))
	{
		TestGetServerList();
	}
	// O = Logout
	if (GetWorld()->GetFirstPlayerController()->WasInputKeyJustPressed(EKeys::O))
	{
		TestLogout();
	}
}

void AMasterServerTest::TestLogin()
{
	if (!ServerClient)
	{
		PrintToScreen(TEXT("Kein ServerClient!"), FColor::Red);
		return;
	}

	PrintToScreen(FString::Printf(TEXT("Login als: %s ..."), *TestUsername), FColor::Yellow);

	FOnAuthComplete Callback;
	Callback.BindDynamic(this, &AMasterServerTest::OnLoginComplete);
	ServerClient->Login(TestUsername, TestPassword, Callback);
}

void AMasterServerTest::TestRegister(const FString& Username, const FString& Email, const FString& Password)
{
	if (!ServerClient)
	{
		PrintToScreen(TEXT("Kein ServerClient!"), FColor::Red);
		return;
	}

	PrintToScreen(FString::Printf(TEXT("Registriere: %s ..."), *Username), FColor::Yellow);

	FOnAuthComplete Callback;
	Callback.BindDynamic(this, &AMasterServerTest::OnRegisterComplete);
	ServerClient->Register(Username, Email, Password, Callback);
}

void AMasterServerTest::TestGetCharacters()
{
	if (!ServerClient)
	{
		PrintToScreen(TEXT("Kein ServerClient!"), FColor::Red);
		return;
	}

	if (!ServerClient->IsAuthenticated())
	{
		PrintToScreen(TEXT("Nicht eingeloggt! Druecke L zuerst."), FColor::Red);
		return;
	}

	PrintToScreen(TEXT("Lade Charaktere..."), FColor::Yellow);

	FOnCharacterListComplete Callback;
	Callback.BindDynamic(this, &AMasterServerTest::OnCharacterListComplete);
	ServerClient->GetCharacters(Callback);
}

void AMasterServerTest::TestCreateCharacter(const FString& Name, const FString& ClassName)
{
	if (!ServerClient || !ServerClient->IsAuthenticated())
	{
		PrintToScreen(TEXT("Nicht eingeloggt!"), FColor::Red);
		return;
	}

	PrintToScreen(FString::Printf(TEXT("Erstelle Charakter: %s (%s)"), *Name, *ClassName), FColor::Yellow);

	FOnAPIComplete Callback;
	Callback.BindDynamic(this, &AMasterServerTest::OnAPIComplete);
	ServerClient->CreateCharacter(Name, ClassName, Callback);
}

void AMasterServerTest::TestGetServerList()
{
	if (!ServerClient)
	{
		PrintToScreen(TEXT("Kein ServerClient!"), FColor::Red);
		return;
	}

	PrintToScreen(TEXT("Lade Serverliste..."), FColor::Yellow);

	FOnServerListComplete Callback;
	Callback.BindDynamic(this, &AMasterServerTest::OnServerListComplete);
	ServerClient->GetServerList(Callback);
}

void AMasterServerTest::TestLogout()
{
	if (!ServerClient)
	{
		PrintToScreen(TEXT("Kein ServerClient!"), FColor::Red);
		return;
	}

	PrintToScreen(TEXT("Logout..."), FColor::Yellow);

	FOnAPIComplete Callback;
	Callback.BindDynamic(this, &AMasterServerTest::OnAPIComplete);
	ServerClient->Logout(Callback);
}

void AMasterServerTest::OnLoginComplete(const FAuthResponse& Response)
{
	if (Response.bSuccess)
	{
		PrintToScreen(FString::Printf(TEXT("LOGIN ERFOLGREICH! Willkommen %s (ID: %d)"),
			*Response.Username, Response.AccountID), FColor::Green);
		PrintToScreen(TEXT("Druecke C fuer Charaktere, S fuer Server"), FColor::Cyan);
	}
	else
	{
		PrintToScreen(FString::Printf(TEXT("LOGIN FEHLGESCHLAGEN: %s"), *Response.Error), FColor::Red);
	}
}

void AMasterServerTest::OnRegisterComplete(const FAuthResponse& Response)
{
	if (Response.bSuccess)
	{
		PrintToScreen(FString::Printf(TEXT("REGISTRIERUNG ERFOLGREICH! Account ID: %d"),
			Response.AccountID), FColor::Green);
	}
	else
	{
		PrintToScreen(FString::Printf(TEXT("REGISTRIERUNG FEHLGESCHLAGEN: %s"), *Response.Error), FColor::Red);
	}
}

void AMasterServerTest::OnCharacterListComplete(const FCharacterListResponse& Response)
{
	if (Response.bSuccess)
	{
		PrintToScreen(FString::Printf(TEXT("CHARAKTERE GELADEN: %d Stueck"), Response.Characters.Num()), FColor::Green);

		for (const FCharacterData& Char : Response.Characters)
		{
			PrintToScreen(FString::Printf(TEXT("  - %s (Level %d %s)"),
				*Char.Name, Char.Level, *Char.ClassName), FColor::White);
		}

		if (Response.Characters.Num() == 0)
		{
			PrintToScreen(TEXT("Keine Charaktere vorhanden."), FColor::Yellow);
		}
	}
	else
	{
		PrintToScreen(FString::Printf(TEXT("FEHLER: %s"), *Response.Error), FColor::Red);
	}
}

void AMasterServerTest::OnServerListComplete(const FServerListResponse& Response)
{
	if (Response.bSuccess)
	{
		PrintToScreen(FString::Printf(TEXT("SERVER GELADEN: %d Stueck"), Response.Servers.Num()), FColor::Green);

		for (const FGameServerInfo& Server : Response.Servers)
		{
			PrintToScreen(FString::Printf(TEXT("  - %s [%s] (%d/%d Spieler)"),
				*Server.Name, *Server.Status, Server.CurrentPlayers, Server.MaxPlayers), FColor::White);
		}

		if (Response.Servers.Num() == 0)
		{
			PrintToScreen(TEXT("Keine Game Server registriert."), FColor::Yellow);
		}
	}
	else
	{
		PrintToScreen(FString::Printf(TEXT("FEHLER: %s"), *Response.Error), FColor::Red);
	}
}

void AMasterServerTest::OnAPIComplete(const FAPIResponse& Response)
{
	if (Response.bSuccess)
	{
		PrintToScreen(TEXT("Aktion erfolgreich!"), FColor::Green);
	}
	else
	{
		PrintToScreen(FString::Printf(TEXT("FEHLER: %s"), *Response.Error), FColor::Red);
	}
}

void AMasterServerTest::PrintToScreen(const FString& Message, FColor Color)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, Color, Message);
	}
	UE_LOG(LogTemp, Log, TEXT("[MasterServerTest] %s"), *Message);
}
