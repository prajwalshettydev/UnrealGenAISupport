// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenSequencerUtils.generated.h"

/**
 * Editor utilities for Level Sequences and Cinematics
 * Allows creating and manipulating sequences via Python/MCP
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenSequencerUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// SEQUENCE CREATION & MANAGEMENT
	// ============================================

	/**
	 * Create a new Level Sequence asset.
	 * @param SequencePath - Full path for the new sequence (e.g., /Game/Cinematics/MySequence)
	 * @param FrameRate - Frames per second (default 30)
	 * @return JSON result with sequence info
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString CreateSequence(const FString& SequencePath, float FrameRate = 30.0f);

	/**
	 * Open a Level Sequence in the Sequencer editor.
	 * @param SequencePath - Path to the sequence asset
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString OpenSequence(const FString& SequencePath);

	/**
	 * List all Level Sequences in a directory.
	 * @param DirectoryPath - Directory to search (default /Game)
	 * @return JSON array of sequences
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString ListSequences(const FString& DirectoryPath = TEXT("/Game"));

	/**
	 * Get sequence info (duration, frame rate, tracks).
	 * @param SequencePath - Path to the sequence
	 * @return JSON with sequence details
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString GetSequenceInfo(const FString& SequencePath);

	/**
	 * Delete a sequence asset.
	 * @param SequencePath - Path to the sequence
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString DeleteSequence(const FString& SequencePath);

	// ============================================
	// TRACK MANAGEMENT
	// ============================================

	/**
	 * Add an actor binding to a sequence.
	 * @param SequencePath - Path to the sequence
	 * @param ActorName - Actor label in the level
	 * @return JSON result with binding GUID
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddActorToSequence(const FString& SequencePath, const FString& ActorName);

	/**
	 * Add a camera cut track to a sequence.
	 * @param SequencePath - Path to the sequence
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddCameraCutTrack(const FString& SequencePath);

	/**
	 * Add a transform track to an actor binding.
	 * @param SequencePath - Path to the sequence
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddTransformTrack(const FString& SequencePath, const FString& ActorName);

	/**
	 * Add a skeletal animation track to an actor.
	 * @param SequencePath - Path to the sequence
	 * @param ActorName - Skeletal mesh actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddAnimationTrack(const FString& SequencePath, const FString& ActorName);

	/**
	 * Add an audio track to the sequence.
	 * @param SequencePath - Path to the sequence
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddAudioTrack(const FString& SequencePath);

	/**
	 * Add an event track to the sequence.
	 * @param SequencePath - Path to the sequence
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddEventTrack(const FString& SequencePath);

	/**
	 * List all tracks in a sequence.
	 * @param SequencePath - Path to the sequence
	 * @return JSON array of tracks
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString ListTracks(const FString& SequencePath);

	// ============================================
	// KEYFRAME OPERATIONS
	// ============================================

	/**
	 * Add a transform keyframe for an actor.
	 * @param SequencePath - Path to the sequence
	 * @param ActorName - Actor label
	 * @param FrameNumber - Frame to set key at
	 * @param Location - World location
	 * @param Rotation - World rotation
	 * @param Scale - Actor scale
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddTransformKey(const FString& SequencePath, const FString& ActorName,
								   int32 FrameNumber, const FVector& Location,
								   const FRotator& Rotation, const FVector& Scale);

	/**
	 * Add an animation section to an actor.
	 * @param SequencePath - Path to the sequence
	 * @param ActorName - Skeletal actor label
	 * @param AnimationPath - Path to animation asset
	 * @param StartFrame - Start frame
	 * @param EndFrame - End frame (-1 for animation length)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddAnimationSection(const FString& SequencePath, const FString& ActorName,
									   const FString& AnimationPath, int32 StartFrame, int32 EndFrame = -1);

	/**
	 * Add an audio section to the audio track.
	 * @param SequencePath - Path to the sequence
	 * @param SoundPath - Path to sound asset
	 * @param StartFrame - Start frame
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddAudioSection(const FString& SequencePath, const FString& SoundPath, int32 StartFrame);

	// ============================================
	// CAMERA OPERATIONS
	// ============================================

	/**
	 * Spawn a cine camera and add to sequence.
	 * @param SequencePath - Path to the sequence
	 * @param CameraName - Label for the camera
	 * @param Location - Camera location
	 * @param Rotation - Camera rotation
	 * @return JSON result with camera info
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString SpawnSequenceCamera(const FString& SequencePath, const FString& CameraName,
									   const FVector& Location, const FRotator& Rotation);

	/**
	 * Add a camera cut at a specific frame.
	 * @param SequencePath - Path to the sequence
	 * @param CameraName - Camera actor label
	 * @param FrameNumber - Frame for the cut
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddCameraCut(const FString& SequencePath, const FString& CameraName, int32 FrameNumber);

	/**
	 * Add a camera shake section.
	 * @param SequencePath - Path to the sequence
	 * @param CameraName - Camera actor label
	 * @param ShakeClass - Camera shake class path
	 * @param StartFrame - Start frame
	 * @param EndFrame - End frame
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddCameraShake(const FString& SequencePath, const FString& CameraName,
								  const FString& ShakeClass, int32 StartFrame, int32 EndFrame);

	// ============================================
	// PLAYBACK CONTROL
	// ============================================

	/**
	 * Play the sequence in editor.
	 * @param SequencePath - Path to the sequence
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString PlaySequence(const FString& SequencePath);

	/**
	 * Stop sequence playback.
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString StopSequence();

	/**
	 * Set the playback position.
	 * @param SequencePath - Path to the sequence
	 * @param FrameNumber - Frame to seek to
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString SetPlaybackPosition(const FString& SequencePath, int32 FrameNumber);

	/**
	 * Set sequence play range.
	 * @param SequencePath - Path to the sequence
	 * @param StartFrame - Start of playback range
	 * @param EndFrame - End of playback range
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString SetPlaybackRange(const FString& SequencePath, int32 StartFrame, int32 EndFrame);

	// ============================================
	// RENDERING & EXPORT
	// ============================================

	/**
	 * Render sequence to video (Movie Render Queue).
	 * @param SequencePath - Path to the sequence
	 * @param OutputPath - Output file path
	 * @param Resolution - Resolution preset (720p, 1080p, 4K)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString RenderSequence(const FString& SequencePath, const FString& OutputPath,
								  const FString& Resolution = TEXT("1080p"));

	/**
	 * Export sequence to FBX.
	 * @param SequencePath - Path to the sequence
	 * @param OutputPath - Output FBX file path
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString ExportToFBX(const FString& SequencePath, const FString& OutputPath);

	// ============================================
	// SUBSEQUENCES
	// ============================================

	/**
	 * Add a subsequence track.
	 * @param ParentSequencePath - Path to parent sequence
	 * @param SubSequencePath - Path to subsequence
	 * @param StartFrame - Start frame in parent
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString AddSubSequence(const FString& ParentSequencePath, const FString& SubSequencePath, int32 StartFrame);

	/**
	 * List subsequences in a sequence.
	 * @param SequencePath - Path to the sequence
	 * @return JSON array of subsequences
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Sequencer Utils")
	static FString ListSubSequences(const FString& SequencePath);

private:
	/** Helper to load a sequence asset */
	static class ULevelSequence* LoadSequenceAsset(const FString& SequencePath);

	/** Helper to find actor by name */
	static AActor* FindActorByName(const FString& ActorName);

	/** Get or create actor binding in sequence */
	static struct FMovieSceneObjectBindingID GetOrCreateBinding(class ULevelSequence* Sequence, AActor* Actor);
};
