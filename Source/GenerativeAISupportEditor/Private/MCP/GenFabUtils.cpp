// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenFabUtils.h"

#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Guid.h"
#include "Modules/ModuleManager.h"
#include "Regex.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "IWebBrowserWindow.h"

#if GENAI_WITH_FAB
#include "FabModule.h"
#include "Utilities/FabLocalAssets.h"
#endif

namespace
{
#if GENAI_WITH_FAB
	enum class EGenFabOperationType : uint8
	{
		Search,
		Import
	};

	struct FGenFabSearchResult
	{
		FString ListingId;
		FString Title;
		FString ListingUrl;
		FString RawText;
		bool bIsLikelyFree = false;
		bool bIsProbablyContentAsset = false;
		bool bCanAddToProject = false;
		bool bRequiresAddToLibrary = false;
		FString VerificationStatus = TEXT("unverified");
		FString RejectionReason;
	};

	static FString ToString(const EWebBrowserDocumentState State)
	{
		switch (State)
		{
		case EWebBrowserDocumentState::Completed:
			return TEXT("Completed");
		case EWebBrowserDocumentState::Error:
			return TEXT("Error");
		case EWebBrowserDocumentState::Loading:
			return TEXT("Loading");
		case EWebBrowserDocumentState::NoDocument:
		default:
			return TEXT("NoDocument");
		}
	}

	static FString ToString(const EWebBrowserConsoleLogSeverity Severity)
	{
		switch (Severity)
		{
		case EWebBrowserConsoleLogSeverity::Default:
			return TEXT("Default");
		case EWebBrowserConsoleLogSeverity::Verbose:
			return TEXT("Verbose");
		case EWebBrowserConsoleLogSeverity::Debug:
			return TEXT("Debug");
		case EWebBrowserConsoleLogSeverity::Info:
			return TEXT("Info");
		case EWebBrowserConsoleLogSeverity::Warning:
			return TEXT("Warning");
		case EWebBrowserConsoleLogSeverity::Error:
			return TEXT("Error");
		case EWebBrowserConsoleLogSeverity::Fatal:
			return TEXT("Fatal");
		default:
			return TEXT("Unknown");
		}
	}

	struct FGenFabOperation
	{
		FString OperationId;
		EGenFabOperationType Type = EGenFabOperationType::Search;
		FString Query;
		TArray<FString> SearchUrls;
		int32 SearchUrlIndex = 0;
		FString ListingId;
		FString ListingUrl;
		int32 MaxResults = 10;
		double StartedAtSeconds = 0.0;
		double TimeoutSeconds = 30.0;
		bool bScriptInjected = false;
		bool bImportTriggered = false;
		FString Status = TEXT("pending");
		FString PayloadJson;
		FString ErrorMessage;
		FString CurrentBrowserUrl;
		FString CurrentDocumentState = TEXT("NoDocument");
		FString LastConsoleMessage;
		FString LastConsoleSeverity;
		TArray<FGenFabSearchResult> SearchCandidates;
		TArray<FGenFabSearchResult> VerifiedSearchResults;
		int32 CurrentSearchCandidateIndex = INDEX_NONE;
	};

	class FGenFabAutomationManager
	{
	public:
		static FGenFabAutomationManager& Get()
		{
			static FGenFabAutomationManager Instance;
			return Instance;
		}

		FString StartSearch(const FString& Query, const int32 MaxResults, const float TimeoutSeconds)
		{
			if (Query.TrimStartAndEnd().IsEmpty())
			{
				return MakeErrorResponse(TEXT("Query cannot be empty."));
			}

			FString ErrorMessage;
			if (!EnsureReady(ErrorMessage))
			{
				return MakeErrorResponse(ErrorMessage);
			}

			if (IsBusy())
			{
				return MakeErrorResponse(FString::Printf(TEXT("Fab automation is busy with operation %s."), *ActiveOperation->OperationId));
			}

			const TSharedPtr<FGenFabOperation> Operation = MakeShared<FGenFabOperation>();
			Operation->OperationId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
			Operation->Type = EGenFabOperationType::Search;
			Operation->Query = Query;
			Operation->SearchUrls = BuildSearchUrls(Query);
			Operation->MaxResults = FMath::Clamp(MaxResults, 1, 25);
			Operation->StartedAtSeconds = FPlatformTime::Seconds();
			Operation->TimeoutSeconds = FMath::Max(static_cast<double>(TimeoutSeconds), 10.0);

			Operations.Add(Operation->OperationId, Operation);
			ActiveOperation = Operation;

			BrowserWindow->LoadURL(Operation->SearchUrls[0]);

			return MakePendingResponse(*Operation);
		}

		FString StartImport(const FString& ListingIdOrUrl, const float TimeoutSeconds)
		{
			FString ErrorMessage;
			if (!EnsureReady(ErrorMessage))
			{
				return MakeErrorResponse(ErrorMessage);
			}

			if (IsBusy())
			{
				return MakeErrorResponse(FString::Printf(TEXT("Fab automation is busy with operation %s."), *ActiveOperation->OperationId));
			}

			FString ListingId = ExtractListingId(ListingIdOrUrl);
			if (ListingId.IsEmpty())
			{
				return MakeErrorResponse(TEXT("Expected a Fab listing ID or listing URL."));
			}

			if (const FString* ExistingPath = UFabLocalAssets::FindPath(ListingId))
			{
				const TSharedPtr<FJsonObject> Completed = MakeShared<FJsonObject>();
				Completed->SetBoolField(TEXT("success"), true);
				Completed->SetStringField(TEXT("status"), TEXT("completed"));
				Completed->SetStringField(TEXT("operation_id"), TEXT(""));
				Completed->SetStringField(TEXT("listing_id"), ListingId);
				Completed->SetStringField(TEXT("import_path"), *ExistingPath);
				return SerializeObject(Completed);
			}

			const TSharedPtr<FGenFabOperation> Operation = MakeShared<FGenFabOperation>();
			Operation->OperationId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
			Operation->Type = EGenFabOperationType::Import;
			Operation->ListingId = ListingId;
			Operation->ListingUrl = NormalizeListingUrl(ListingIdOrUrl, ListingId);
			Operation->StartedAtSeconds = FPlatformTime::Seconds();
			Operation->TimeoutSeconds = FMath::Max(static_cast<double>(TimeoutSeconds), 20.0);

			Operations.Add(Operation->OperationId, Operation);
			ActiveOperation = Operation;

			BrowserWindow->LoadURL(Operation->ListingUrl);

			return MakePendingResponse(*Operation);
		}

		FString GetOperationStatus(const FString& OperationId)
		{
			const TSharedPtr<FGenFabOperation>* FoundOperation = Operations.Find(OperationId);
			if (FoundOperation == nullptr || !FoundOperation->IsValid())
			{
				return MakeErrorResponse(FString::Printf(TEXT("Unknown Fab operation id: %s"), *OperationId));
			}

			const TSharedPtr<FGenFabOperation>& Operation = *FoundOperation;
			UpdateImportCompletion(Operation);
			UpdateTimeout(Operation);
			return BuildOperationResponse(*Operation);
		}

	private:
		TMap<FString, TSharedPtr<FGenFabOperation>> Operations;
		TSharedPtr<FGenFabOperation> ActiveOperation;
		TSharedPtr<IWebBrowserWindow> BrowserWindow;
		TObjectPtr<UObject> FabApiObject = nullptr;

	private:
		bool EnsureReady(FString& OutError)
		{
			if (!FModuleManager::Get().ModuleExists(TEXT("Fab")))
			{
				OutError = TEXT("The Unreal Fab plugin is not available in this engine build.");
				return false;
			}

			IFabModule::Get();

			if (FabApiObject == nullptr)
			{
				UClass* FabApiClass = FindObject<UClass>(nullptr, TEXT("/Script/Fab.FabBrowserApi"));
				if (FabApiClass == nullptr)
				{
					FabApiClass = LoadClass<UObject>(nullptr, TEXT("/Script/Fab.FabBrowserApi"));
				}

				if (FabApiClass == nullptr)
				{
					OutError = TEXT("Unable to load the FabBrowserApi class.");
					return false;
				}

				FabApiObject = NewObject<UObject>(GetTransientPackage(), FabApiClass);
				if (FabApiObject == nullptr)
				{
					OutError = TEXT("Unable to construct the FabBrowserApi bridge object.");
					return false;
				}

				FabApiObject->AddToRoot();
			}

			if (BrowserWindow.IsValid())
			{
				return true;
			}

			IWebBrowserModule& WebBrowserModule = IWebBrowserModule::Get();
			if (!WebBrowserModule.IsWebModuleAvailable())
			{
				OutError = TEXT("The Unreal WebBrowser module is not available.");
				return false;
			}

			FCreateBrowserWindowSettings WindowSettings;
			WindowSettings.InitialURL = TEXT("about:blank");
			WindowSettings.BrowserFrameRate = 60;
			WindowSettings.bShowErrorMessage = true;

			BrowserWindow = WebBrowserModule.GetSingleton()->CreateBrowserWindow(WindowSettings);
			if (!BrowserWindow.IsValid())
			{
				OutError = TEXT("Failed to create a hidden browser window for Fab automation.");
				return false;
			}

			BrowserWindow->SetViewportSize(FIntPoint(1280, 960));
			BrowserWindow->BindUObject(TEXT("fab"), FabApiObject, true);
			BrowserWindow->OnDocumentStateChanged().AddRaw(this, &FGenFabAutomationManager::HandleDocumentStateChanged);
			BrowserWindow->OnUrlChanged().AddRaw(this, &FGenFabAutomationManager::HandleUrlChanged);
			BrowserWindow->OnConsoleMessage().BindRaw(this, &FGenFabAutomationManager::HandleConsoleMessage);

			return true;
		}

		bool IsBusy() const
		{
			return ActiveOperation.IsValid()
				&& ActiveOperation->Status != TEXT("completed")
				&& ActiveOperation->Status != TEXT("error");
		}

		void HandleDocumentStateChanged(const EWebBrowserDocumentState NewState)
		{
			if (!ActiveOperation.IsValid())
			{
				return;
			}

			ActiveOperation->CurrentDocumentState = ToString(NewState);

			if (NewState == EWebBrowserDocumentState::Error)
			{
				MarkError(ActiveOperation, TEXT("The hidden Fab browser failed to load the requested page."));
				return;
			}

			if (NewState != EWebBrowserDocumentState::Completed || ActiveOperation->bScriptInjected)
			{
				return;
			}

			ActiveOperation->bScriptInjected = true;
			if (ActiveOperation->Type == EGenFabOperationType::Search)
			{
				if (ActiveOperation->SearchCandidates.IsValidIndex(ActiveOperation->CurrentSearchCandidateIndex))
				{
					BrowserWindow->ExecuteJavascript(BuildSearchVerificationScript(*ActiveOperation, ActiveOperation->SearchCandidates[ActiveOperation->CurrentSearchCandidateIndex]));
				}
				else
				{
					BrowserWindow->ExecuteJavascript(BuildSearchScript(*ActiveOperation));
				}
			}
			else
			{
				BrowserWindow->ExecuteJavascript(BuildImportScript(*ActiveOperation));
			}
		}

		void HandleUrlChanged(FString NewUrl)
		{
			if (!ActiveOperation.IsValid())
			{
				return;
			}

			ActiveOperation->CurrentBrowserUrl = NewUrl;
			ActiveOperation->bScriptInjected = false;
		}

		void HandleConsoleMessage(const FString& Message, const FString&, const int32, const EWebBrowserConsoleLogSeverity Severity)
		{
			if (ActiveOperation.IsValid())
			{
				ActiveOperation->LastConsoleMessage = Message.Left(800);
				ActiveOperation->LastConsoleSeverity = ToString(Severity);
			}

			static const FString Prefix = TEXT("GENFAB::");
			if (!Message.StartsWith(Prefix))
			{
				return;
			}

			const FString Payload = Message.Mid(Prefix.Len());
			TSharedPtr<FJsonObject> RootObject;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
			if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
			{
				return;
			}

			const FString OperationId = RootObject->GetStringField(TEXT("operationId"));
			const TSharedPtr<FGenFabOperation>* FoundOperation = Operations.Find(OperationId);
			if (FoundOperation == nullptr || !FoundOperation->IsValid())
			{
				return;
			}

			const TSharedPtr<FGenFabOperation>& Operation = *FoundOperation;
			const FString Type = RootObject->GetStringField(TEXT("type"));

			if (Type == TEXT("search_candidates"))
			{
				HandleSearchCandidates(Operation, RootObject);
				return;
			}

			if (Type == TEXT("search_candidate_verified"))
			{
				HandleSearchCandidateVerified(Operation, RootObject);
				return;
			}

			if (Type == TEXT("search_candidate_rejected"))
			{
				HandleSearchCandidateRejected(Operation, RootObject);
				return;
			}

			if (Type == TEXT("search_results"))
			{
				Operation->Status = TEXT("completed");
				Operation->PayloadJson = Payload;
				if (ActiveOperation == Operation)
				{
					ActiveOperation.Reset();
				}
				return;
			}

			if (Type == TEXT("import_clicked"))
			{
				Operation->Status = TEXT("importing");
				Operation->bImportTriggered = true;
				return;
			}

			if (Type == TEXT("error"))
			{
				if (TryAdvanceSearch(Operation, RootObject))
				{
					return;
				}

				MarkError(Operation, RootObject->GetStringField(TEXT("error")));
			}
		}

		static FString GetJsonStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName)
		{
			FString Value;
			Object->TryGetStringField(FieldName, Value);
			return Value;
		}

		static bool GetJsonBoolField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, const bool DefaultValue = false)
		{
			bool Value = DefaultValue;
			Object->TryGetBoolField(FieldName, Value);
			return Value;
		}

		static bool ParseSearchResult(const TSharedPtr<FJsonObject>& Object, FGenFabSearchResult& OutResult)
		{
			if (!Object.IsValid())
			{
				return false;
			}

			OutResult.ListingId = GetJsonStringField(Object, TEXT("listing_id"));
			OutResult.Title = GetJsonStringField(Object, TEXT("title"));
			OutResult.ListingUrl = GetJsonStringField(Object, TEXT("listing_url"));
			OutResult.RawText = GetJsonStringField(Object, TEXT("raw_text"));
			OutResult.bIsLikelyFree = GetJsonBoolField(Object, TEXT("is_likely_free"));
			OutResult.bIsProbablyContentAsset = GetJsonBoolField(Object, TEXT("is_probably_content_asset"));
			OutResult.bCanAddToProject = GetJsonBoolField(Object, TEXT("can_add_to_project"));
			OutResult.bRequiresAddToLibrary = GetJsonBoolField(Object, TEXT("requires_add_to_library"));
			OutResult.VerificationStatus = GetJsonStringField(Object, TEXT("verification_status"));
			if (OutResult.VerificationStatus.IsEmpty())
			{
				OutResult.VerificationStatus = TEXT("unverified");
			}
			OutResult.RejectionReason = GetJsonStringField(Object, TEXT("rejection_reason"));
			return !OutResult.ListingId.IsEmpty() && !OutResult.ListingUrl.IsEmpty();
		}

		static TSharedPtr<FJsonObject> MakeSearchResultObject(const FGenFabSearchResult& Result)
		{
			const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetStringField(TEXT("listing_id"), Result.ListingId);
			Root->SetStringField(TEXT("title"), Result.Title);
			Root->SetStringField(TEXT("listing_url"), Result.ListingUrl);
			Root->SetStringField(TEXT("raw_text"), Result.RawText);
			Root->SetBoolField(TEXT("is_likely_free"), Result.bIsLikelyFree);
			Root->SetBoolField(TEXT("is_probably_content_asset"), Result.bIsProbablyContentAsset);
			Root->SetBoolField(TEXT("can_add_to_project"), Result.bCanAddToProject);
			Root->SetBoolField(TEXT("requires_add_to_library"), Result.bRequiresAddToLibrary);
			Root->SetStringField(TEXT("verification_status"), Result.VerificationStatus);
			if (!Result.RejectionReason.IsEmpty())
			{
				Root->SetStringField(TEXT("rejection_reason"), Result.RejectionReason);
			}
			return Root;
		}

		void HandleSearchCandidates(const TSharedPtr<FGenFabOperation>& Operation, const TSharedPtr<FJsonObject>& RootObject)
		{
			if (!Operation.IsValid() || Operation->Type != EGenFabOperationType::Search)
			{
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* Results = nullptr;
			if (!RootObject->TryGetArrayField(TEXT("results"), Results) || Results == nullptr || Results->IsEmpty())
			{
				MarkError(Operation, TEXT("Fab search returned no candidate listings to verify."));
				return;
			}

			Operation->SearchCandidates.Reset();
			Operation->VerifiedSearchResults.Reset();
			Operation->CurrentSearchCandidateIndex = INDEX_NONE;

			for (const TSharedPtr<FJsonValue>& ResultValue : *Results)
			{
				const TSharedPtr<FJsonObject> ResultObject = ResultValue.IsValid() ? ResultValue->AsObject() : nullptr;
				FGenFabSearchResult ParsedResult;
				if (ParseSearchResult(ResultObject, ParsedResult))
				{
					Operation->SearchCandidates.Add(ParsedResult);
				}
			}

			if (Operation->SearchCandidates.IsEmpty())
			{
				MarkError(Operation, TEXT("Fab search did not produce any valid candidate listings."));
				return;
			}

			Operation->Status = TEXT("verifying");
			AdvanceSearchCandidateVerification(Operation);
		}

		void HandleSearchCandidateVerified(const TSharedPtr<FGenFabOperation>& Operation, const TSharedPtr<FJsonObject>& RootObject)
		{
			if (!Operation.IsValid() || Operation->Type != EGenFabOperationType::Search)
			{
				return;
			}

			if (!Operation->SearchCandidates.IsValidIndex(Operation->CurrentSearchCandidateIndex))
			{
				MarkError(Operation, TEXT("Fab search verification lost track of the active candidate."));
				return;
			}

			FGenFabSearchResult& Candidate = Operation->SearchCandidates[Operation->CurrentSearchCandidateIndex];
			const FString VerifiedListingUrl = GetJsonStringField(RootObject, TEXT("listing_url"));
			if (!VerifiedListingUrl.IsEmpty())
			{
				Candidate.ListingUrl = VerifiedListingUrl;
			}
			Candidate.bCanAddToProject = GetJsonBoolField(RootObject, TEXT("can_add_to_project"), true);
			Candidate.bRequiresAddToLibrary = GetJsonBoolField(RootObject, TEXT("requires_add_to_library"));
			Candidate.VerificationStatus = GetJsonStringField(RootObject, TEXT("verification_status"));
			if (Candidate.VerificationStatus.IsEmpty())
			{
				Candidate.VerificationStatus = TEXT("verified");
			}
			Candidate.RejectionReason.Reset();

			if (Candidate.bCanAddToProject)
			{
				Operation->VerifiedSearchResults.Add(Candidate);
			}

			AdvanceSearchCandidateVerification(Operation);
		}

		void HandleSearchCandidateRejected(const TSharedPtr<FGenFabOperation>& Operation, const TSharedPtr<FJsonObject>& RootObject)
		{
			if (!Operation.IsValid() || Operation->Type != EGenFabOperationType::Search)
			{
				return;
			}

			if (Operation->SearchCandidates.IsValidIndex(Operation->CurrentSearchCandidateIndex))
			{
				FGenFabSearchResult& Candidate = Operation->SearchCandidates[Operation->CurrentSearchCandidateIndex];
				Candidate.VerificationStatus = TEXT("rejected");
				Candidate.RejectionReason = GetJsonStringField(RootObject, TEXT("rejection_reason"));
				if (Candidate.RejectionReason.IsEmpty())
				{
					Candidate.RejectionReason = GetJsonStringField(RootObject, TEXT("error"));
				}
			}

			const FString RejectionReason = GetJsonStringField(RootObject, TEXT("rejection_reason"));
			const FString ErrorMessage = GetJsonStringField(RootObject, TEXT("error"));
			if (RejectionReason == TEXT("auth_required") || RejectionReason == TEXT("browser_verification_challenge"))
			{
				MarkError(Operation, ErrorMessage.IsEmpty() ? TEXT("Fab requires manual user action before search verification can continue.") : ErrorMessage);
				return;
			}

			AdvanceSearchCandidateVerification(Operation);
		}

		void AdvanceSearchCandidateVerification(const TSharedPtr<FGenFabOperation>& Operation)
		{
			if (!Operation.IsValid())
			{
				return;
			}

			if (Operation->VerifiedSearchResults.Num() >= Operation->MaxResults)
			{
				CompleteSearch(Operation);
				return;
			}

			while (Operation->SearchCandidates.IsValidIndex(Operation->CurrentSearchCandidateIndex + 1))
			{
				Operation->CurrentSearchCandidateIndex += 1;

				const FGenFabSearchResult& Candidate = Operation->SearchCandidates[Operation->CurrentSearchCandidateIndex];
				if (Candidate.ListingUrl.IsEmpty())
				{
					continue;
				}

				Operation->Status = TEXT("verifying");
				Operation->bScriptInjected = false;
				BrowserWindow->LoadURL(Candidate.ListingUrl);
				return;
			}

			if (!Operation->VerifiedSearchResults.IsEmpty())
			{
				CompleteSearch(Operation);
				return;
			}

			MarkError(Operation, TEXT("No free Fab content assets with Add to Project were found for this query."));
		}

		void CompleteSearch(const TSharedPtr<FGenFabOperation>& Operation)
		{
			if (!Operation.IsValid())
			{
				return;
			}

			const TSharedPtr<FJsonObject> PayloadObject = MakeShared<FJsonObject>();
			TArray<TSharedPtr<FJsonValue>> ResultValues;
			ResultValues.Reserve(Operation->VerifiedSearchResults.Num());

			for (const FGenFabSearchResult& Result : Operation->VerifiedSearchResults)
			{
				ResultValues.Add(MakeShared<FJsonValueObject>(MakeSearchResultObject(Result)));
			}

			PayloadObject->SetArrayField(TEXT("results"), ResultValues);
			PayloadObject->SetNumberField(TEXT("candidate_count"), Operation->SearchCandidates.Num());
			PayloadObject->SetNumberField(TEXT("verified_count"), Operation->VerifiedSearchResults.Num());

			Operation->PayloadJson = SerializeObject(PayloadObject);
			Operation->Status = TEXT("completed");

			if (ActiveOperation == Operation)
			{
				ActiveOperation.Reset();
			}
		}

		void UpdateImportCompletion(const TSharedPtr<FGenFabOperation>& Operation)
		{
			if (!Operation.IsValid() || Operation->Type != EGenFabOperationType::Import || Operation->Status != TEXT("importing"))
			{
				return;
			}

			if (const FString* ImportedPath = UFabLocalAssets::FindPath(Operation->ListingId))
			{
				const TSharedPtr<FJsonObject> CompletedPayload = MakeShared<FJsonObject>();
				CompletedPayload->SetStringField(TEXT("listing_id"), Operation->ListingId);
				CompletedPayload->SetStringField(TEXT("listing_url"), Operation->ListingUrl);
				CompletedPayload->SetStringField(TEXT("import_path"), *ImportedPath);
				Operation->PayloadJson = SerializeObject(CompletedPayload);
				Operation->Status = TEXT("completed");

				if (ActiveOperation == Operation)
				{
					ActiveOperation.Reset();
				}
			}
		}

		void UpdateTimeout(const TSharedPtr<FGenFabOperation>& Operation)
		{
			if (!Operation.IsValid() || Operation->Status == TEXT("completed") || Operation->Status == TEXT("error"))
			{
				return;
			}

			const double Elapsed = FPlatformTime::Seconds() - Operation->StartedAtSeconds;
			if (Elapsed <= Operation->TimeoutSeconds)
			{
				return;
			}

			if (Operation->Type == EGenFabOperationType::Import && !Operation->bImportTriggered)
			{
				MarkError(Operation, TEXT("Timed out waiting for the Fab page to expose an Add to Project action."));
			}
			else if (Operation->Type == EGenFabOperationType::Import)
			{
				MarkError(Operation, TEXT("The asset import did not finish before the timeout expired."));
			}
			else if (Operation->Status == TEXT("verifying"))
			{
				MarkError(Operation, TEXT("Timed out while verifying which Fab search results support Add to Project."));
			}
			else
			{
				MarkError(Operation, TEXT("Timed out waiting for free Fab search results."));
			}
		}

		void MarkError(const TSharedPtr<FGenFabOperation>& Operation, const FString& ErrorMessage)
		{
			if (!Operation.IsValid())
			{
				return;
			}

			Operation->Status = TEXT("error");
			Operation->ErrorMessage = ErrorMessage;

			if (ActiveOperation == Operation)
			{
				ActiveOperation.Reset();
			}
		}

		TArray<FString> BuildSearchUrls(const FString& Query) const
		{
			const FString PluginBaseUrl = GetPluginFabBaseUrl();
			const FString SiteBaseUrl = GetCanonicalFabBaseUrl();

			TArray<FString> SearchUrls;
			SearchUrls.Reserve(4);
			SearchUrls.Add(PluginBaseUrl);
			SearchUrls.Add(FString::Printf(TEXT("%s/search"), *PluginBaseUrl));
			SearchUrls.Add(FString::Printf(TEXT("%s/search/channels/unreal-engine?is_free=1"), *SiteBaseUrl));
			SearchUrls.Add(FString::Printf(TEXT("%s/search?is_free=1"), *SiteBaseUrl));
			return SearchUrls;
		}

		FString NormalizeListingUrl(const FString& ListingIdOrUrl, const FString& ListingId) const
		{
			if (ListingIdOrUrl.StartsWith(TEXT("http://")) || ListingIdOrUrl.StartsWith(TEXT("https://")))
			{
				return ListingIdOrUrl;
			}

			return FString::Printf(TEXT("%s/listings/%s"), *GetPluginFabBaseUrl(), *ListingId);
		}

		FString GetPluginFabBaseUrl() const
		{
			const FString BaseUrl = CallFabStringFunction(TEXT("GetUrl"));
			return BaseUrl.IsEmpty() ? TEXT("https://www.fab.com/plugins/ue5") : BaseUrl;
		}

		FString GetCanonicalFabBaseUrl() const
		{
			FString BaseUrl = GetPluginFabBaseUrl();
			BaseUrl.RemoveFromEnd(TEXT("/plugins/ue5"));
			return BaseUrl.IsEmpty() ? TEXT("https://www.fab.com") : BaseUrl;
		}

		FString CallFabStringFunction(const TCHAR* FunctionName) const
		{
			if (FabApiObject == nullptr)
			{
				return FString();
			}

			UFunction* Function = FabApiObject->FindFunction(FName(FunctionName));
			if (Function == nullptr)
			{
				return FString();
			}

			struct FFabStringReturnParams
			{
				FString ReturnValue;
			};

			FFabStringReturnParams Params;
			FabApiObject->ProcessEvent(Function, &Params);
			return Params.ReturnValue;
		}

		static FString ExtractListingId(const FString& ListingIdOrUrl)
		{
			if (ListingIdOrUrl.IsEmpty())
			{
				return FString();
			}

			if (!ListingIdOrUrl.Contains(TEXT("/")))
			{
				return ListingIdOrUrl;
			}

			static const FRegexPattern ListingPattern(TEXT("/listings/([^/?#]+)"));
			FRegexMatcher Matcher(ListingPattern, ListingIdOrUrl);
			return Matcher.FindNext() ? Matcher.GetCaptureGroup(1) : FString();
		}

		static FString EscapeForJavaScript(const FString& Input)
		{
			FString Escaped = Input;
			Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
			Escaped.ReplaceInline(TEXT("'"), TEXT("\\'"));
			Escaped.ReplaceInline(TEXT("\r"), TEXT("\\r"));
			Escaped.ReplaceInline(TEXT("\n"), TEXT("\\n"));
			return Escaped;
		}

		static FString BuildSearchScript(const FGenFabOperation& Operation)
		{
			return FString::Printf(
				TEXT(R"JS(
(function() {
  const opId = '%s';
  const maxResults = %d;
  const emit = (payload) => console.log('GENFAB::' + JSON.stringify(Object.assign({ operationId: opId }, payload)));
  const normalize = (value) => (value || '').replace(/\s+/g, ' ').trim();
  const queryText = normalize('%s');
  let searchSubmitted = false;
  const truncate = (value, max = 600) => {
    const text = normalize(value);
    return text.length > max ? text.slice(0, max) + '...' : text;
  };
  try {
    const buildDebugSnapshot = () => ({
      page_url: window.location.href,
      title: document.title || '',
      query_text: queryText,
      search_submitted: searchSubmitted,
      body_text: truncate(document.body && document.body.innerText),
      listing_anchor_count: Array.from(document.querySelectorAll('a[href*="/listings/"]')).length,
      input_descriptors: Array.from(document.querySelectorAll('input, textarea'))
        .map((element) => truncate([
          element.tagName,
          element.type,
          element.name,
          element.id,
          element.placeholder,
          element.getAttribute('aria-label'),
          element.getAttribute('data-testid')
        ].filter(Boolean).join(' | '), 160))
        .filter(Boolean)
        .slice(0, 12),
      button_labels: Array.from(document.querySelectorAll('button, a, [role="button"]'))
        .map((element) => truncate(element.innerText || element.textContent, 120))
        .filter(Boolean)
        .slice(0, 20)
    });
    emit({
      type: 'search_script_started',
      debug: buildDebugSnapshot()
    });
    const isLikelyFreeText = (value) => {
      const text = normalize(value).toLowerCase();
      return /\bfree\b/i.test(text) || /\bfrom\s+\$?\s*0(?:[.,]00)?\b/i.test(text) || /(?:^|[\s(])\$?\s*0(?:[.,]00)?(?:[\s),]|$)/i.test(text);
    };
    const hasKeyword = (value, keywords) => {
      const text = normalize(value).toLowerCase();
      return keywords.some((keyword) => text.includes(keyword));
    };
    const nonContentKeywords = [
      'plugin',
      'code plugin',
      'install plugin',
      'template',
      'project template',
      'sample project',
      'complete project',
      'game template',
      'starter project',
      'launcher'
    ];
    const contentKeywords = [
      'material',
      'materials',
      'mesh',
      'meshes',
      'prop',
      'props',
      'environment',
      'foliage',
      'tree',
      'trees',
      'texture',
      'textures',
      'character',
      'animation',
      'vfx',
      'niagara',
      'audio',
      'sound',
      'music',
      'scene',
      'pack'
    ];
    const extract = () => {
      const anchors = Array.from(document.querySelectorAll('a[href*="/listings/"]'));
      const seen = new Set();
      const results = [];
      for (const anchor of anchors) {
        const href = anchor.href || anchor.getAttribute('href');
        if (!href || seen.has(href)) {
          continue;
        }
        const container = anchor.closest('article, li, section, div');
        const rawText = normalize((container && container.innerText) || anchor.innerText || '');
        if (!rawText) {
          continue;
        }
        const title = normalize(anchor.textContent) || rawText.split(' Free ')[0] || href;
        const listingMatch = href.match(/\/listings\/([^/?#]+)/);
        const classificationText = normalize(title + ' ' + rawText);
        results.push({
          listing_id: listingMatch ? listingMatch[1] : '',
          title,
          listing_url: href,
          raw_text: rawText,
          is_likely_free: isLikelyFreeText(rawText),
          is_probably_content_asset: hasKeyword(classificationText, contentKeywords),
          is_probably_non_content_asset: hasKeyword(classificationText, nonContentKeywords)
        });
        seen.add(href);
        if (results.length >= Math.max(maxResults * 3, 12)) {
          break;
        }
      }
      const scoredResults = results
        .map((result) => ({
          ...result,
          score:
            (result.is_likely_free ? 100 : 0) +
            (result.is_probably_content_asset ? 20 : 0) -
            (result.is_probably_non_content_asset ? 80 : 0)
        }))
        .sort((left, right) => right.score - left.score);
      const importableCandidates = scoredResults.filter((result) => result.is_likely_free && !result.is_probably_non_content_asset);
      const fallbackCandidates = scoredResults.filter((result) => result.is_likely_free || result.is_probably_content_asset);
      return (importableCandidates.length > 0 ? importableCandidates : fallbackCandidates)
        .slice(0, Math.max(maxResults * 3, 12))
        .map(({ score, ...result }) => result);
    };
    const findSearchInput = () => {
      const candidates = Array.from(document.querySelectorAll('input, textarea'));
      return candidates.find((element) => {
        const attrs = [
          element.type,
          element.name,
          element.id,
          element.placeholder,
          element.getAttribute('aria-label'),
          element.getAttribute('data-testid')
        ].map((value) => normalize(value).toLowerCase()).join(' ');
        return attrs.includes('search') || attrs.includes('search fab') || attrs.includes('search...') || element.type === 'search';
      });
    };
    const submitSearch = (input) => {
      if (!input || !queryText) {
        return false;
      }
      const descriptor = window.HTMLInputElement ? Object.getOwnPropertyDescriptor(window.HTMLInputElement.prototype, 'value') : null;
      const nativeSetter = descriptor ? descriptor.set : null;
      if (nativeSetter) {
        nativeSetter.call(input, queryText);
      } else {
        input.value = queryText;
      }
      input.dispatchEvent(new Event('input', { bubbles: true }));
      input.dispatchEvent(new Event('change', { bubbles: true }));
      input.focus();
      input.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', code: 'Enter', bubbles: true }));
      input.dispatchEvent(new KeyboardEvent('keypress', { key: 'Enter', code: 'Enter', bubbles: true }));
      input.dispatchEvent(new KeyboardEvent('keyup', { key: 'Enter', code: 'Enter', bubbles: true }));
      const submitButton = Array.from(document.querySelectorAll('button, [role="button"]')).find((element) => {
        const text = normalize(element.innerText || element.textContent).toLowerCase();
        return text === 'search' || text.includes('apply');
      });
      if (submitButton) {
        submitButton.click();
      }
      return true;
    };
    let probeEmitted = false;
    let attempts = 0;
    const timer = setInterval(() => {
      attempts += 1;
      const bodyText = normalize(document.body && document.body.innerText);
      if (bodyText.includes('verify you are human') || bodyText.includes('just a moment')) {
        if (attempts >= 40) {
          clearInterval(timer);
          emit({ type: 'error', error: 'Fab search page is blocked by a browser verification challenge. Open Fab in Unreal once, complete the verification, and retry.' });
        }
        return;
      }
      if (!probeEmitted) {
        probeEmitted = true;
        emit({
          type: 'search_probe',
          debug: Object.assign(buildDebugSnapshot(), {
            found_search_input: !!findSearchInput()
          })
        });
      }
      if (!searchSubmitted && queryText) {
        const searchInput = findSearchInput();
        if (submitSearch(searchInput)) {
          searchSubmitted = true;
          emit({
            type: 'search_submitted',
            debug: buildDebugSnapshot()
          });
          return;
        }
      }
      const results = extract();
      if (results.length > 0) {
        clearInterval(timer);
        emit({ type: 'search_candidates', results });
        return;
      }
      if (attempts >= 40) {
        clearInterval(timer);
        emit({
          type: 'error',
          error: 'No free Fab results were found on the current search page.',
          debug: buildDebugSnapshot()
        });
      }
    }, 500);
  } catch (error) {
    emit({
      type: 'error',
      error: 'Search script exception: ' + (error && error.message ? error.message : String(error)),
      debug: {
        page_url: window.location.href,
        title: document.title || '',
        stack: truncate(error && error.stack ? error.stack : '')
      }
    });
  }
})();
)JS"),
				*EscapeForJavaScript(Operation.OperationId),
				Operation.MaxResults,
				*EscapeForJavaScript(Operation.Query));
		}

		bool TryAdvanceSearch(const TSharedPtr<FGenFabOperation>& Operation, const TSharedPtr<FJsonObject>& RootObject)
		{
			if (!Operation.IsValid() || Operation->Type != EGenFabOperationType::Search)
			{
				return false;
			}

			const FString ErrorMessage = RootObject->GetStringField(TEXT("error"));
			if (!ErrorMessage.Contains(TEXT("No free Fab results were found")))
			{
				return false;
			}

			if (!Operation->SearchUrls.IsValidIndex(Operation->SearchUrlIndex + 1))
			{
				return false;
			}

			Operation->SearchUrlIndex += 1;
			Operation->Status = TEXT("pending");
			Operation->bScriptInjected = false;
			Operation->PayloadJson.Reset();
			Operation->ErrorMessage.Reset();
			Operation->StartedAtSeconds = FPlatformTime::Seconds();
			BrowserWindow->LoadURL(Operation->SearchUrls[Operation->SearchUrlIndex]);
			return true;
		}

		static FString BuildSearchVerificationScript(const FGenFabOperation& Operation, const FGenFabSearchResult& Candidate)
		{
			return FString::Printf(
				TEXT(R"JS(
(function() {
  const opId = '%s';
  const listingId = '%s';
  const emit = (payload) => console.log('GENFAB::' + JSON.stringify(Object.assign({ operationId: opId, listing_id: listingId }, payload)));
  const normalize = (value) => (value || '').replace(/\s+/g, ' ').trim().toLowerCase();
  const findAction = () => {
    const elements = Array.from(document.querySelectorAll('button, a, [role="button"]'));
    return {
      addToLibrary: elements.find((element) => normalize(element.innerText || element.textContent).includes('add to my library')),
      addToProject: elements.find((element) => normalize(element.innerText || element.textContent).includes('add to project')),
      installPlugin: elements.find((element) => normalize(element.innerText || element.textContent).includes('install plugin')),
      createProject: elements.find((element) => normalize(element.innerText || element.textContent).includes('create project')),
      signIn: elements.find((element) => normalize(element.innerText || element.textContent) === 'sign in')
    };
  };
  let attempts = 0;
  const timer = setInterval(() => {
    attempts += 1;
    const bodyText = normalize(document.body && document.body.innerText);
    if (bodyText.includes('verify you are human') || bodyText.includes('just a moment')) {
      clearInterval(timer);
      emit({
        type: 'search_candidate_rejected',
        rejection_reason: 'browser_verification_challenge',
        error: 'Fab result verification is blocked by a browser verification challenge. Complete the challenge in Unreal and retry.',
        listing_url: window.location.href
      });
      return;
    }
    const actions = findAction();
    if (actions.signIn) {
      clearInterval(timer);
      emit({
        type: 'search_candidate_rejected',
        rejection_reason: 'auth_required',
        error: 'Fab requires an authenticated Epic account before the asset can be added to this project.',
        listing_url: window.location.href
      });
      return;
    }
    if (actions.installPlugin || actions.createProject || bodyText.includes('view in launcher')) {
      clearInterval(timer);
      emit({
        type: 'search_candidate_rejected',
        rejection_reason: 'unsupported_asset_type',
        error: 'This Fab item is not a standard content asset with Add to Project support.',
        listing_url: window.location.href
      });
      return;
    }
    if (actions.addToProject || actions.addToLibrary) {
      clearInterval(timer);
      emit({
        type: 'search_candidate_verified',
        listing_url: window.location.href,
        can_add_to_project: true,
        requires_add_to_library: !actions.addToProject && !!actions.addToLibrary,
        verification_status: actions.addToProject ? 'verified_add_to_project' : 'verified_add_to_library_then_project'
      });
      return;
    }
    if (attempts >= 60) {
      clearInterval(timer);
      emit({
        type: 'search_candidate_rejected',
        rejection_reason: 'missing_add_to_project_action',
        error: 'Timed out waiting for Add to Project actions on this Fab listing.',
        listing_url: window.location.href
      });
    }
  }, 500);
})();
)JS"),
				*EscapeForJavaScript(Operation.OperationId),
				*EscapeForJavaScript(Candidate.ListingId));
		}

		static FString BuildImportScript(const FGenFabOperation& Operation)
		{
			return FString::Printf(
				TEXT(R"JS(
(function() {
  const opId = '%s';
  const emit = (payload) => console.log('GENFAB::' + JSON.stringify(Object.assign({ operationId: opId }, payload)));
  const normalize = (value) => (value || '').replace(/\s+/g, ' ').trim().toLowerCase();
  const findAction = () => {
    const elements = Array.from(document.querySelectorAll('button, a, [role="button"]'));
    return {
      addToLibrary: elements.find((element) => normalize(element.innerText || element.textContent).includes('add to my library')),
      addToProject: elements.find((element) => normalize(element.innerText || element.textContent).includes('add to project')),
      installPlugin: elements.find((element) => normalize(element.innerText || element.textContent).includes('install plugin')),
      createProject: elements.find((element) => normalize(element.innerText || element.textContent).includes('create project')),
      signIn: elements.find((element) => normalize(element.innerText || element.textContent) === 'sign in')
    };
  };
  let attempts = 0;
  let clickedLibrary = false;
  const timer = setInterval(() => {
    attempts += 1;
    const bodyText = normalize(document.body && document.body.innerText);
    if (bodyText.includes('verify you are human') || bodyText.includes('just a moment')) {
      if (attempts >= 80) {
        clearInterval(timer);
        emit({ type: 'error', error: 'Fab listing page is blocked by a browser verification challenge. Open Fab in Unreal once, complete the verification, and retry.' });
      }
      return;
    }
    const actions = findAction();
    if (actions.signIn) {
      clearInterval(timer);
      emit({ type: 'error', error: 'Fab requires an authenticated Epic account. Sign in through the Fab window in Unreal and retry.' });
      return;
    }
    if (actions.installPlugin || actions.createProject || bodyText.includes('view in launcher')) {
      clearInterval(timer);
      emit({ type: 'error', error: 'This Fab item is not importable as a standard content asset via Add to Project.' });
      return;
    }
    if (actions.addToProject) {
      actions.addToProject.click();
      clearInterval(timer);
      emit({ type: 'import_clicked' });
      return;
    }
    if (!clickedLibrary && actions.addToLibrary) {
      clickedLibrary = true;
      actions.addToLibrary.click();
      return;
    }
    if (attempts >= 80) {
      clearInterval(timer);
      emit({ type: 'error', error: 'Timed out waiting for Add to Project to become available on the Fab listing page.' });
    }
  }, 750);
})();
)JS"),
				*EscapeForJavaScript(Operation.OperationId));
		}

		FString BuildOperationResponse(const FGenFabOperation& Operation) const
		{
			if (Operation.Status == TEXT("error"))
			{
				const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
				Root->SetBoolField(TEXT("success"), false);
				Root->SetStringField(TEXT("status"), TEXT("error"));
				Root->SetStringField(TEXT("operation_id"), Operation.OperationId);
				Root->SetStringField(TEXT("error"), Operation.ErrorMessage);
				AppendDebugFields(Root, Operation);
				return SerializeObject(Root);
			}

			if (Operation.Status == TEXT("completed"))
			{
				TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
				Root->SetBoolField(TEXT("success"), true);
				Root->SetStringField(TEXT("status"), TEXT("completed"));
				Root->SetStringField(TEXT("operation_id"), Operation.OperationId);

				if (Operation.Type == EGenFabOperationType::Search)
				{
					Root->SetStringField(TEXT("query"), Operation.Query);
				}

				if (!Operation.ListingId.IsEmpty())
				{
					Root->SetStringField(TEXT("listing_id"), Operation.ListingId);
				}

				if (!Operation.ListingUrl.IsEmpty())
				{
					Root->SetStringField(TEXT("listing_url"), Operation.ListingUrl);
				}

				if (!Operation.PayloadJson.IsEmpty())
				{
					TSharedPtr<FJsonObject> PayloadObject;
					const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Operation.PayloadJson);
					if (FJsonSerializer::Deserialize(Reader, PayloadObject) && PayloadObject.IsValid())
					{
						if (Operation.Type == EGenFabOperationType::Search)
						{
							const TArray<TSharedPtr<FJsonValue>>* Results;
							if (PayloadObject->TryGetArrayField(TEXT("results"), Results))
							{
								Root->SetArrayField(TEXT("results"), *Results);
							}

							double CandidateCount = 0.0;
							if (PayloadObject->TryGetNumberField(TEXT("candidate_count"), CandidateCount))
							{
								Root->SetNumberField(TEXT("candidate_count"), CandidateCount);
							}

							double VerifiedCount = 0.0;
							if (PayloadObject->TryGetNumberField(TEXT("verified_count"), VerifiedCount))
							{
								Root->SetNumberField(TEXT("verified_count"), VerifiedCount);
							}
						}
						else
						{
							const FString ImportPath = PayloadObject->GetStringField(TEXT("import_path"));
							Root->SetStringField(TEXT("import_path"), ImportPath);
						}
					}
				}

				return SerializeObject(Root);
			}

			const TSharedPtr<FJsonObject> Pending = MakeShared<FJsonObject>();
			Pending->SetBoolField(TEXT("success"), true);
			Pending->SetStringField(TEXT("status"), Operation.Status);
			Pending->SetStringField(TEXT("operation_id"), Operation.OperationId);

			if (Operation.Type == EGenFabOperationType::Import)
			{
				Pending->SetStringField(TEXT("listing_id"), Operation.ListingId);
				Pending->SetStringField(TEXT("listing_url"), Operation.ListingUrl);
			}
			else
			{
				Pending->SetStringField(TEXT("query"), Operation.Query);
			}

			AppendDebugFields(Pending, Operation);
			return SerializeObject(Pending);
		}

		static void AppendDebugFields(const TSharedPtr<FJsonObject>& Root, const FGenFabOperation& Operation)
		{
			const TSharedPtr<FJsonObject> Debug = MakeShared<FJsonObject>();
			Debug->SetStringField(TEXT("browser_url"), Operation.CurrentBrowserUrl);
			Debug->SetStringField(TEXT("document_state"), Operation.CurrentDocumentState);
			Debug->SetBoolField(TEXT("script_injected"), Operation.bScriptInjected);
			Debug->SetBoolField(TEXT("import_triggered"), Operation.bImportTriggered);
			Debug->SetStringField(TEXT("last_console_message"), Operation.LastConsoleMessage);
			Debug->SetStringField(TEXT("last_console_severity"), Operation.LastConsoleSeverity);
			Debug->SetNumberField(TEXT("search_url_index"), Operation.SearchUrlIndex);
			Debug->SetNumberField(TEXT("search_url_count"), Operation.SearchUrls.Num());
			Debug->SetNumberField(TEXT("search_candidate_count"), Operation.SearchCandidates.Num());
			Debug->SetNumberField(TEXT("search_verified_count"), Operation.VerifiedSearchResults.Num());
			Debug->SetNumberField(TEXT("search_candidate_index"), Operation.CurrentSearchCandidateIndex);
			if (Operation.SearchUrls.IsValidIndex(Operation.SearchUrlIndex))
			{
				Debug->SetStringField(TEXT("active_search_url"), Operation.SearchUrls[Operation.SearchUrlIndex]);
			}
			if (Operation.SearchCandidates.IsValidIndex(Operation.CurrentSearchCandidateIndex))
			{
				Debug->SetStringField(TEXT("active_candidate_listing_id"), Operation.SearchCandidates[Operation.CurrentSearchCandidateIndex].ListingId);
				Debug->SetStringField(TEXT("active_candidate_title"), Operation.SearchCandidates[Operation.CurrentSearchCandidateIndex].Title);
			}
			Root->SetObjectField(TEXT("debug"), Debug);
		}

		static FString MakeErrorResponse(const FString& ErrorMessage)
		{
			const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetBoolField(TEXT("success"), false);
			Root->SetStringField(TEXT("status"), TEXT("error"));
			Root->SetStringField(TEXT("error"), ErrorMessage);
			return SerializeObject(Root);
		}

		static FString MakePendingResponse(const FGenFabOperation& Operation)
		{
			const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetBoolField(TEXT("success"), true);
			Root->SetStringField(TEXT("status"), TEXT("pending"));
			Root->SetStringField(TEXT("operation_id"), Operation.OperationId);
			if (Operation.Type == EGenFabOperationType::Search)
			{
				Root->SetStringField(TEXT("query"), Operation.Query);
			}
			else
			{
				Root->SetStringField(TEXT("listing_id"), Operation.ListingId);
				Root->SetStringField(TEXT("listing_url"), Operation.ListingUrl);
			}
			return SerializeObject(Root);
		}

		static FString SerializeObject(const TSharedPtr<FJsonObject>& Object)
		{
			FString Output;
			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
			FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
			return Output;
		}
	};
#endif
}

FString UGenFabUtils::StartSearchFreeFabAssets(const FString& Query, const int32 MaxResults, const float TimeoutSeconds)
{
#if GENAI_WITH_FAB
	return FGenFabAutomationManager::Get().StartSearch(Query, MaxResults, TimeoutSeconds);
#else
	const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("success"), false);
	Root->SetStringField(TEXT("status"), TEXT("error"));
	Root->SetStringField(TEXT("error"), TEXT("This engine build does not expose the Fab plugin private headers needed for Fab automation."));
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Output;
#endif
}

FString UGenFabUtils::StartAddFreeFabAssetToProject(const FString& ListingIdOrUrl, const float TimeoutSeconds)
{
#if GENAI_WITH_FAB
	return FGenFabAutomationManager::Get().StartImport(ListingIdOrUrl, TimeoutSeconds);
#else
	const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("success"), false);
	Root->SetStringField(TEXT("status"), TEXT("error"));
	Root->SetStringField(TEXT("error"), TEXT("This engine build does not expose the Fab plugin private headers needed for Fab automation."));
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Output;
#endif
}

FString UGenFabUtils::GetFabOperationStatus(const FString& OperationId)
{
#if GENAI_WITH_FAB
	return FGenFabAutomationManager::Get().GetOperationStatus(OperationId);
#else
	const TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("success"), false);
	Root->SetStringField(TEXT("status"), TEXT("error"));
	Root->SetStringField(TEXT("error"), TEXT("This engine build does not expose the Fab plugin private headers needed for Fab automation."));
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Output;
#endif
}
