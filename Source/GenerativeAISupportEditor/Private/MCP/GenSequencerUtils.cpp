// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenSequencerUtils.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Tracks/MovieSceneAudioTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Tracks/MovieSceneEventTrack.h"
#include "Tracks/MovieSceneSubTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Sections/MovieSceneSkeletalAnimationSection.h"
#include "Sections/MovieSceneAudioSection.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Sections/MovieSceneSubSection.h"
#include "CineCameraActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
// LevelSequenceFactoryNew.h is private in UE5.5 - using alternative creation method
#include "UObject/SavePackage.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "ISequencer.h"
#include "ISequencerModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EditorAssetLibrary.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"

ULevelSequence* UGenSequencerUtils::LoadSequenceAsset(const FString& SequencePath)
{
	return LoadObject<ULevelSequence>(nullptr, *SequencePath);
}

AActor* UGenSequencerUtils::FindActorByName(const FString& ActorName)
{
	if (!GEditor) return nullptr;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if ((*It)->GetActorLabel() == ActorName)
		{
			return *It;
		}
	}

	return nullptr;
}

FString UGenSequencerUtils::CreateSequence(const FString& SequencePath, float FrameRate)
{
	// Check if exists
	if (LoadSequenceAsset(SequencePath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence already exists at '%s'\"}"), *SequencePath);
	}

	// Create the sequence without using the private LevelSequenceFactoryNew
	FString PackagePath = FPackageName::GetLongPackagePath(SequencePath);
	FString AssetName = FPackageName::GetShortName(SequencePath);
	FString FullPackageName = PackagePath / AssetName;

	// Create package
	UPackage* Package = CreatePackage(*FullPackageName);
	if (!Package)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to create package\"}");
	}

	// Create the sequence object directly
	ULevelSequence* Sequence = NewObject<ULevelSequence>(Package, *AssetName, RF_Public | RF_Standalone);

	if (!Sequence)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to create sequence\"}");
	}

	// Initialize the MovieScene
	Sequence->Initialize();

	// Set frame rate
	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (MovieScene)
	{
		FFrameRate DisplayRate(FMath::RoundToInt(FrameRate), 1);
		MovieScene->SetDisplayRate(DisplayRate);
	}

	// Mark package as dirty and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Sequence);

	// Save the asset
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Sequence, *FPackageName::LongPackageNameToFilename(FullPackageName, FPackageName::GetAssetPackageExtension()), SaveArgs);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Sequence created at '%s'\", \"frame_rate\": %f}"),
		*SequencePath, FrameRate);
}

FString UGenSequencerUtils::OpenSequence(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Sequence);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Opened sequence '%s'\"}"), *SequencePath);
}

FString UGenSequencerUtils::ListSequences(const FString& DirectoryPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*DirectoryPath), AssetList, true);

	TArray<TSharedPtr<FJsonValue>> SequencesArray;

	for (const FAssetData& Asset : AssetList)
	{
		if (Asset.AssetClassPath == ULevelSequence::StaticClass()->GetClassPathName())
		{
			TSharedPtr<FJsonObject> SeqJson = MakeShareable(new FJsonObject);
			SeqJson->SetStringField("name", Asset.AssetName.ToString());
			SeqJson->SetStringField("path", Asset.GetObjectPathString());

			SequencesArray.Add(MakeShareable(new FJsonValueObject(SeqJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", SequencesArray.Num());
	ResultJson->SetArrayField("sequences", SequencesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenSequencerUtils::GetSequenceInfo(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("path", SequencePath);

	// Frame rate
	FFrameRate DisplayRate = MovieScene->GetDisplayRate();
	ResultJson->SetNumberField("frame_rate", DisplayRate.AsDecimal());

	// Duration
	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	int32 StartFrame = PlaybackRange.GetLowerBoundValue().Value;
	int32 EndFrame = PlaybackRange.GetUpperBoundValue().Value;
	ResultJson->SetNumberField("start_frame", StartFrame);
	ResultJson->SetNumberField("end_frame", EndFrame);
	ResultJson->SetNumberField("duration_frames", EndFrame - StartFrame);

	// Duration in seconds
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	double DurationSeconds = (double)(EndFrame - StartFrame) / TickResolution.AsDecimal();
	ResultJson->SetNumberField("duration_seconds", DurationSeconds);

	// Track counts
	ResultJson->SetNumberField("track_count", MovieScene->GetTracks().Num());
	ResultJson->SetNumberField("binding_count", MovieScene->GetBindings().Num());

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenSequencerUtils::DeleteSequence(const FString& SequencePath)
{
	if (!UEditorAssetLibrary::DoesAssetExist(SequencePath))
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	bool bDeleted = UEditorAssetLibrary::DeleteAsset(SequencePath);

	if (bDeleted)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Sequence deleted: '%s'\"}"), *SequencePath);
	}

	return TEXT("{\"success\": false, \"error\": \"Failed to delete sequence\"}");
}

FString UGenSequencerUtils::AddActorToSequence(const FString& SequencePath, const FString& ActorName)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	// Check if already bound
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetName() == ActorName)
		{
			return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' already in sequence\"}"), *ActorName);
		}
	}

	// Create spawnable or possessable
	FGuid BindingGuid = MovieScene->AddPossessable(ActorName, Actor->GetClass());
	Sequence->BindPossessableObject(BindingGuid, *Actor, Actor->GetWorld());

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Actor '%s' added to sequence\", \"binding_guid\": \"%s\"}"),
		*ActorName, *BindingGuid.ToString());
}

FString UGenSequencerUtils::AddCameraCutTrack(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	// Check if camera cut track already exists
	UMovieSceneCameraCutTrack* CameraCutTrack = MovieScene->FindTrack<UMovieSceneCameraCutTrack>();
	if (CameraCutTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"Camera cut track already exists\"}");
	}

	CameraCutTrack = MovieScene->AddTrack<UMovieSceneCameraCutTrack>();
	if (!CameraCutTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to create camera cut track\"}");
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return TEXT("{\"success\": true, \"message\": \"Camera cut track added\"}");
}

FString UGenSequencerUtils::AddTransformTrack(const FString& SequencePath, const FString& ActorName)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	// Find the binding
	FGuid BindingGuid;
	bool bFound = false;
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetName() == ActorName)
		{
			BindingGuid = Binding.GetObjectGuid();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in sequence. Add it first.\"}"), *ActorName);
	}

	// Add transform track
	UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(BindingGuid);
	if (!TransformTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add transform track\"}");
	}

	// Create a section
	UMovieScene3DTransformSection* Section = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
	if (Section)
	{
		Section->SetRange(MovieScene->GetPlaybackRange());
		TransformTrack->AddSection(*Section);
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Transform track added for '%s'\"}"), *ActorName);
}

FString UGenSequencerUtils::AddAnimationTrack(const FString& SequencePath, const FString& ActorName)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	// Find the binding
	FGuid BindingGuid;
	bool bFound = false;
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetName() == ActorName)
		{
			BindingGuid = Binding.GetObjectGuid();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in sequence\"}"), *ActorName);
	}

	UMovieSceneSkeletalAnimationTrack* AnimTrack = MovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(BindingGuid);
	if (!AnimTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add animation track\"}");
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Animation track added for '%s'\"}"), *ActorName);
}

FString UGenSequencerUtils::AddAudioTrack(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	UMovieSceneAudioTrack* AudioTrack = MovieScene->AddTrack<UMovieSceneAudioTrack>();
	if (!AudioTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add audio track\"}");
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return TEXT("{\"success\": true, \"message\": \"Audio track added\"}");
}

FString UGenSequencerUtils::AddEventTrack(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	UMovieSceneEventTrack* EventTrack = MovieScene->AddTrack<UMovieSceneEventTrack>();
	if (!EventTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add event track\"}");
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return TEXT("{\"success\": true, \"message\": \"Event track added\"}");
}

FString UGenSequencerUtils::ListTracks(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> TracksArray;

	// Master tracks
	for (UMovieSceneTrack* Track : MovieScene->GetTracks())
	{
		TSharedPtr<FJsonObject> TrackJson = MakeShareable(new FJsonObject);
		TrackJson->SetStringField("name", Track->GetTrackName().ToString());
		TrackJson->SetStringField("type", Track->GetClass()->GetName());
		TrackJson->SetStringField("binding", TEXT("Master"));
		TrackJson->SetNumberField("section_count", Track->GetAllSections().Num());

		TracksArray.Add(MakeShareable(new FJsonValueObject(TrackJson)));
	}

	// Binding tracks
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		for (UMovieSceneTrack* Track : Binding.GetTracks())
		{
			TSharedPtr<FJsonObject> TrackJson = MakeShareable(new FJsonObject);
			TrackJson->SetStringField("name", Track->GetTrackName().ToString());
			TrackJson->SetStringField("type", Track->GetClass()->GetName());
			TrackJson->SetStringField("binding", Binding.GetName());
			TrackJson->SetNumberField("section_count", Track->GetAllSections().Num());

			TracksArray.Add(MakeShareable(new FJsonValueObject(TrackJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", TracksArray.Num());
	ResultJson->SetArrayField("tracks", TracksArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenSequencerUtils::AddTransformKey(const FString& SequencePath, const FString& ActorName,
											int32 FrameNumber, const FVector& Location,
											const FRotator& Rotation, const FVector& Scale)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	// Find binding
	FGuid BindingGuid;
	bool bFound = false;
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetName() == ActorName)
		{
			BindingGuid = Binding.GetObjectGuid();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not in sequence\"}"), *ActorName);
	}

	// Find transform track
	UMovieScene3DTransformTrack* TransformTrack = MovieScene->FindTrack<UMovieScene3DTransformTrack>(BindingGuid);
	if (!TransformTrack)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No transform track for '%s'. Add one first.\"}"), *ActorName);
	}

	// Get or create section
	UMovieScene3DTransformSection* Section = nullptr;
	if (TransformTrack->GetAllSections().Num() > 0)
	{
		Section = Cast<UMovieScene3DTransformSection>(TransformTrack->GetAllSections()[0]);
	}

	if (!Section)
	{
		return TEXT("{\"success\": false, \"error\": \"No transform section found\"}");
	}

	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameNumber Frame = FFrameNumber(FrameNumber);

	// Access channels and add keys
	TArrayView<FMovieSceneDoubleChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();

	if (Channels.Num() >= 9)
	{
		// Location X, Y, Z (indices 0, 1, 2)
		Channels[0]->AddCubicKey(Frame, Location.X);
		Channels[1]->AddCubicKey(Frame, Location.Y);
		Channels[2]->AddCubicKey(Frame, Location.Z);

		// Rotation X, Y, Z (indices 3, 4, 5)
		Channels[3]->AddCubicKey(Frame, Rotation.Roll);
		Channels[4]->AddCubicKey(Frame, Rotation.Pitch);
		Channels[5]->AddCubicKey(Frame, Rotation.Yaw);

		// Scale X, Y, Z (indices 6, 7, 8)
		Channels[6]->AddCubicKey(Frame, Scale.X);
		Channels[7]->AddCubicKey(Frame, Scale.Y);
		Channels[8]->AddCubicKey(Frame, Scale.Z);
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Transform key added at frame %d for '%s'\"}"),
		FrameNumber, *ActorName);
}

FString UGenSequencerUtils::AddAnimationSection(const FString& SequencePath, const FString& ActorName,
												const FString& AnimationPath, int32 StartFrame, int32 EndFrame)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UAnimSequence* AnimSequence = LoadObject<UAnimSequence>(nullptr, *AnimationPath);
	if (!AnimSequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Animation not found at '%s'\"}"), *AnimationPath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();

	// Find binding
	FGuid BindingGuid;
	bool bFound = false;
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetName() == ActorName)
		{
			BindingGuid = Binding.GetObjectGuid();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not in sequence\"}"), *ActorName);
	}

	UMovieSceneSkeletalAnimationTrack* AnimTrack = MovieScene->FindTrack<UMovieSceneSkeletalAnimationTrack>(BindingGuid);
	if (!AnimTrack)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"No animation track for '%s'\"}"), *ActorName);
	}

	// Calculate end frame if not specified
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	if (EndFrame < 0)
	{
		double AnimLength = AnimSequence->GetPlayLength();
		EndFrame = StartFrame + FMath::RoundToInt(AnimLength * TickResolution.AsDecimal());
	}

	UMovieSceneSkeletalAnimationSection* Section = Cast<UMovieSceneSkeletalAnimationSection>(
		AnimTrack->AddNewAnimationOnRow(FFrameNumber(StartFrame), AnimSequence, -1));

	if (Section)
	{
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(StartFrame), FFrameNumber(EndFrame)));
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Animation section added for '%s' (frames %d-%d)\"}"),
		*ActorName, StartFrame, EndFrame);
}

FString UGenSequencerUtils::AddAudioSection(const FString& SequencePath, const FString& SoundPath, int32 StartFrame)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *SoundPath);
	if (!Sound)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sound not found at '%s'\"}"), *SoundPath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();

	// Find or create audio track
	UMovieSceneAudioTrack* AudioTrack = nullptr;
	for (UMovieSceneTrack* Track : MovieScene->GetTracks())
	{
		AudioTrack = Cast<UMovieSceneAudioTrack>(Track);
		if (AudioTrack) break;
	}

	if (!AudioTrack)
	{
		AudioTrack = MovieScene->AddTrack<UMovieSceneAudioTrack>();
	}

	UMovieSceneAudioSection* Section = AudioTrack->AddNewSoundOnRow(Sound, FFrameNumber(StartFrame), -1);

	if (!Section)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add audio section\"}");
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Audio section added at frame %d\"}"), StartFrame);
}

FString UGenSequencerUtils::SpawnSequenceCamera(const FString& SequencePath, const FString& CameraName,
												const FVector& Location, const FRotator& Rotation)
{
	if (!GEditor)
	{
		return TEXT("{\"success\": false, \"error\": \"GEditor not available\"}");
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	// Spawn cine camera
	ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(Location, Rotation);
	if (!Camera)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to spawn camera\"}");
	}

	Camera->SetActorLabel(*CameraName);

	// Add to sequence
	FString AddResult = AddActorToSequence(SequencePath, CameraName);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Camera '%s' spawned and added to sequence\", \"add_result\": %s}"),
		*CameraName, *AddResult);
}

FString UGenSequencerUtils::AddCameraCut(const FString& SequencePath, const FString& CameraName, int32 FrameNumber)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	UMovieSceneCameraCutTrack* CameraCutTrack = MovieScene->FindTrack<UMovieSceneCameraCutTrack>();

	if (!CameraCutTrack)
	{
		return TEXT("{\"success\": false, \"error\": \"No camera cut track. Add one first.\"}");
	}

	// Find camera binding
	FGuid CameraGuid;
	bool bFound = false;
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetName() == CameraName)
		{
			CameraGuid = Binding.GetObjectGuid();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Camera '%s' not in sequence\"}"), *CameraName);
	}

	// Add camera cut section
	TRange<FFrameNumber> Range = MovieScene->GetPlaybackRange();
	UMovieSceneCameraCutSection* Section = Cast<UMovieSceneCameraCutSection>(CameraCutTrack->CreateNewSection());

	if (Section)
	{
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(FrameNumber), Range.GetUpperBoundValue()));
		FMovieSceneObjectBindingID BindingID(CameraGuid, MovieSceneSequenceID::Root);
		Section->SetCameraBindingID(BindingID);
		CameraCutTrack->AddSection(*Section);
	}

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Camera cut to '%s' at frame %d\"}"),
		*CameraName, FrameNumber);
}

FString UGenSequencerUtils::AddCameraShake(const FString& SequencePath, const FString& CameraName,
										   const FString& ShakeClass, int32 StartFrame, int32 EndFrame)
{
	// Camera shake requires additional setup - placeholder for now
	return TEXT("{\"success\": false, \"error\": \"Camera shake not yet implemented\"}");
}

FString UGenSequencerUtils::PlaySequence(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	// Open in editor and play
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Sequence);

	// Note: Actually controlling playback requires the Sequencer interface
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Sequence '%s' opened. Use Sequencer controls to play.\"}"), *SequencePath);
}

FString UGenSequencerUtils::StopSequence()
{
	return TEXT("{\"success\": true, \"message\": \"Stop playback via Sequencer UI\"}");
}

FString UGenSequencerUtils::SetPlaybackPosition(const FString& SequencePath, int32 FrameNumber)
{
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Set position to frame %d via Sequencer UI\"}"), FrameNumber);
}

FString UGenSequencerUtils::SetPlaybackRange(const FString& SequencePath, int32 StartFrame, int32 EndFrame)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return TEXT("{\"success\": false, \"error\": \"MovieScene not found\"}");
	}

	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(StartFrame), FFrameNumber(EndFrame)));

	UEditorAssetLibrary::SaveAsset(SequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Playback range set to %d-%d\"}"), StartFrame, EndFrame);
}

FString UGenSequencerUtils::RenderSequence(const FString& SequencePath, const FString& OutputPath, const FString& Resolution)
{
	// Movie Render Queue integration would go here
	return TEXT("{\"success\": false, \"error\": \"Rendering requires Movie Render Queue setup\"}");
}

FString UGenSequencerUtils::ExportToFBX(const FString& SequencePath, const FString& OutputPath)
{
	return TEXT("{\"success\": false, \"error\": \"FBX export not yet implemented\"}");
}

FString UGenSequencerUtils::AddSubSequence(const FString& ParentSequencePath, const FString& SubSequencePath, int32 StartFrame)
{
	ULevelSequence* ParentSequence = LoadSequenceAsset(ParentSequencePath);
	if (!ParentSequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Parent sequence not found at '%s'\"}"), *ParentSequencePath);
	}

	ULevelSequence* SubSequence = LoadSequenceAsset(SubSequencePath);
	if (!SubSequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sub sequence not found at '%s'\"}"), *SubSequencePath);
	}

	UMovieScene* MovieScene = ParentSequence->GetMovieScene();

	UMovieSceneSubTrack* SubTrack = MovieScene->FindTrack<UMovieSceneSubTrack>();
	if (!SubTrack)
	{
		SubTrack = MovieScene->AddTrack<UMovieSceneSubTrack>();
	}

	UMovieSceneSubSection* Section = SubTrack->AddSequence(SubSequence, FFrameNumber(StartFrame), 0);

	if (!Section)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to add subsequence\"}");
	}

	UEditorAssetLibrary::SaveAsset(ParentSequencePath, false);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Subsequence added at frame %d\"}"), StartFrame);
}

FString UGenSequencerUtils::ListSubSequences(const FString& SequencePath)
{
	ULevelSequence* Sequence = LoadSequenceAsset(SequencePath);
	if (!Sequence)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Sequence not found at '%s'\"}"), *SequencePath);
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();

	TArray<TSharedPtr<FJsonValue>> SubsArray;

	for (UMovieSceneTrack* Track : MovieScene->GetTracks())
	{
		UMovieSceneSubTrack* SubTrack = Cast<UMovieSceneSubTrack>(Track);
		if (!SubTrack) continue;

		for (UMovieSceneSection* Section : SubTrack->GetAllSections())
		{
			UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
			if (!SubSection) continue;

			UMovieSceneSequence* SubSeq = SubSection->GetSequence();
			if (!SubSeq) continue;

			TSharedPtr<FJsonObject> SubJson = MakeShareable(new FJsonObject);
			SubJson->SetStringField("name", SubSeq->GetName());
			SubJson->SetNumberField("start_frame", SubSection->GetRange().GetLowerBoundValue().Value);

			SubsArray.Add(MakeShareable(new FJsonValueObject(SubJson)));
		}
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", SubsArray.Num());
	ResultJson->SetArrayField("subsequences", SubsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}
