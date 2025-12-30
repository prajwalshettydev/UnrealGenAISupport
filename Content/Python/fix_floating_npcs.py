# Fix Floating NPCs - Run this in Unreal Python Console
# Execute: exec(open("C:/ProjectTitan/Plugins/GenerativeAISupport/Content/Python/fix_floating_npcs.py").read())

import unreal

def fix_all_npcs():
    """Fix all floating NPCs in the current level - OPTIMIZED VERSION"""

    print("=" * 50)
    print("FIX FLOATING NPCs (Optimized)")
    print("=" * 50)

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        print("ERROR: No editor world!")
        return

    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()

    fixed_count = 0
    skipped_count = 0

    # Only process these actor types (NPCs/Characters)
    npc_keywords = ["npc", "character", "creature", "monster", "enemy", "villager",
                    "merchant", "guard", "citizen", "animal", "pet", "jelly", "slime",
                    "bp_", "pawn", "humanoid"]

    # Classes to definitely skip
    skip_classes = [
        "Light", "Camera", "Volume", "Trigger", "NavMesh",
        "PlayerStart", "SkyLight", "DirectionalLight", "PointLight",
        "SpotLight", "ExponentialHeightFog", "AtmosphericFog",
        "SkyAtmosphere", "VolumetricCloud", "PostProcessVolume",
        "LevelSequenceActor", "Landscape", "StaticMeshActor",
        "Brush", "Note", "TextRender", "DecalActor", "Spawner",
        "GameMode", "WorldSettings", "CameraShakeSource",
        "Decal", "Foliage", "InstancedFoliage", "Audio"
    ]

    candidates = []

    # First pass: Filter candidates
    for actor in all_actors:
        try:
            actor_name = actor.get_actor_label().lower()
            actor_class = actor.get_class().get_name()

            # Skip excluded classes
            should_skip = False
            for skip_class in skip_classes:
                if skip_class.lower() in actor_class.lower():
                    should_skip = True
                    break

            if should_skip:
                continue

            # Check if this looks like an NPC
            is_npc = False
            for keyword in npc_keywords:
                if keyword in actor_name or keyword in actor_class.lower():
                    is_npc = True
                    break

            # Also check for SkeletalMesh (animated characters)
            if not is_npc:
                skel_components = actor.get_components_by_class(unreal.SkeletalMeshComponent)
                if skel_components and len(skel_components) > 0:
                    is_npc = True

            if is_npc:
                candidates.append((actor, actor_name, actor_class))
        except:
            pass

    print(f"Found {len(candidates)} NPC candidates")

    # Second pass: Fix NPCs
    for actor, actor_name, actor_class in candidates:
        try:
            old_loc = actor.get_actor_location()
            old_z = old_loc.z
            current_rot = actor.get_actor_rotation()

            # Step 1: Move actor HIGH UP first (500 units above)
            high_loc = unreal.Vector(old_loc.x, old_loc.y, old_loc.z + 500.0)
            actor.set_actor_location(high_loc, False, False)

            # Step 2: Drop to ground using NPCChatLibrary
            success = unreal.NPCChatLibrary.adjust_actor_to_ground(actor, True, False)

            # Step 3: Fix rotation (upright)
            if success:
                upright_rot = unreal.Rotator(0.0, current_rot.yaw, 0.0)
                actor.set_actor_rotation(upright_rot, False)

                # Disable physics
                for mesh in actor.get_components_by_class(unreal.SkeletalMeshComponent):
                    if mesh:
                        mesh.set_simulate_physics(False)

                new_z = actor.get_actor_location().z
                print(f"FIXED: {actor_name} - Z: {old_z:.1f} -> {new_z:.1f}")
                fixed_count += 1
            else:
                # Restore original position if failed
                actor.set_actor_location(old_loc, False, False)
                skipped_count += 1

        except Exception as e:
            print(f"FAILED: {actor_name} - {str(e)}")
            skipped_count += 1

    print("=" * 50)
    print(f"DONE! Fixed: {fixed_count}, Skipped: {skipped_count}")
    print("=" * 50)

# Auto-run when executed
fix_all_npcs()
