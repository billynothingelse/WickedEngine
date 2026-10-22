#include "stdafx.h"
#include "armature_window.h"

using namespace Protean;
void ArmatureWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	Protean::Gui::Window::Create(ICON_ARMATURE " Armature", Protean::Gui::Window::WindowControls_COLLAPSE | Protean::Gui::Window::WindowControls_CLOSE | Protean::Gui::Window::WindowControls_FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 380));

	closeButton.SetTooltip("Delete ArmatureComponent");
	OnClose([=](Protean::Gui::EventArgs args) {

		Protean::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().armatures.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 4;
	float hei = 20;
	float step = hei + 2;
	float wid = 220;

	infoLabel.Create("");
	infoLabel.SetText("This window will stay open even if you select other entities until it is collapsed, so you can select other bone entities.");
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	resetPoseButton.Create("Reset Pose");
	resetPoseButton.SetTooltip("Reset Pose will be performed on the Armature, based on the bone inverse bind matrices.");
	resetPoseButton.SetSize(XMFLOAT2(wid, hei));
	resetPoseButton.OnClick([=](Protean::Gui::EventArgs args) {
		Protean::Scene& scene = editor->GetCurrentScene();
		scene.ResetPose(entity);
	});
	AddWidget(&resetPoseButton);

	createHumanoidButton.Create("Try to create humanoid rig");
	createHumanoidButton.SetTooltip("Tries to create a humanoid component based on bone naming convention.\nIt supports the VRM and Mixamo naming convention.");
	createHumanoidButton.SetSize(XMFLOAT2(wid, hei));
	createHumanoidButton.OnClick([=](Protean::Gui::EventArgs args) {
		Protean::Scene& scene = editor->GetCurrentScene();
		if (scene.humanoids.Contains(entity))
		{
			Protean::Helper::messageBox("Humanoid Component already exists!");
			return;
		}

		const ArmatureComponent* armature = scene.armatures.GetComponent(entity);
		if (armature == nullptr)
			return;

		static const Protean::unordered_map<HumanoidComponent::HumanoidBone, Protean::vector<std::string>> mapping = {
			{HumanoidComponent::HumanoidBone_Hips, {"Hips", "pelvis"}},
			{HumanoidComponent::HumanoidBone_Spine, {"Spine", "spine_01"}},
			{HumanoidComponent::HumanoidBone_Chest, {"Chest", "Spine1", "spine_02"}},
			{HumanoidComponent::HumanoidBone_UpperChest, {"UpperChest", "Spine2", "spine_03"}},
			{HumanoidComponent::HumanoidBone_Neck, {"Neck"}},

			{HumanoidComponent::HumanoidBone_Head, {"Head"}},
			{HumanoidComponent::HumanoidBone_LeftEye, {"LeftEye"}},
			{HumanoidComponent::HumanoidBone_RightEye, {"RightEye"}},
			{HumanoidComponent::HumanoidBone_Jaw, {"Jaw"}},

			{HumanoidComponent::HumanoidBone_LeftUpperLeg, {"LeftUpperLeg", "LeftUpLeg", "thigh_l"}},
			{HumanoidComponent::HumanoidBone_LeftLowerLeg, {"LeftLowerLeg", "LeftLeg", "calf_l"}},
			{HumanoidComponent::HumanoidBone_LeftFoot, {"LeftFoot", "foot_l"}},
			{HumanoidComponent::HumanoidBone_LeftToes, {"LeftToe", "ball_l"}},
			{HumanoidComponent::HumanoidBone_RightUpperLeg, {"RightUpperLeg", "RightUpLeg", "thigh_r"}},
			{HumanoidComponent::HumanoidBone_RightLowerLeg, {"RightLowerLeg", "RightLeg", "calf_r"}},
			{HumanoidComponent::HumanoidBone_RightFoot, {"RightFoot", "foot_r"}},
			{HumanoidComponent::HumanoidBone_RightToes, {"RightToe", "ball_r"}},

			{HumanoidComponent::HumanoidBone_LeftShoulder, {"LeftShoulder", "clavicle_l"}},
			{HumanoidComponent::HumanoidBone_LeftUpperArm, {"LeftUpperArm", "LeftArm", "upperarm_l"}},
			{HumanoidComponent::HumanoidBone_LeftLowerArm, {"LeftLowerArm", "LeftForeArm", "lowerarm_l"}},
			{HumanoidComponent::HumanoidBone_LeftHand, {"LeftHand", "hand_l"}},
			{HumanoidComponent::HumanoidBone_RightShoulder, {"RightShoulder", "clavicle_r"}},
			{HumanoidComponent::HumanoidBone_RightUpperArm, {"RightUpperArm", "RightArm", "upperarm_r"}},
			{HumanoidComponent::HumanoidBone_RightLowerArm, {"RightLowerArm", "RightForeArm", "lowerarm_r"}},
			{HumanoidComponent::HumanoidBone_RightHand, {"RightHand", "hand_r"}},

			{HumanoidComponent::HumanoidBone_LeftThumbMetacarpal, {"LeftThumbMetacarpal", "LeftHandThumb1", "thumb_01_l"}},
			{HumanoidComponent::HumanoidBone_LeftThumbProximal, {"LeftThumbProximal", "LeftHandThumb2", "thumb_02_l"}},
			{HumanoidComponent::HumanoidBone_LeftThumbDistal, {"LeftThumbDistal", "LeftHandThumb3", "thumb_03_l"}},
			{HumanoidComponent::HumanoidBone_LeftIndexProximal, {"LeftIndexProximal", "LeftHandIndex1", "index_01_l"}},
			{HumanoidComponent::HumanoidBone_LeftIndexIntermediate, {"LeftIndexIntermediate", "LeftHandIndex2", "index_02_l"}},
			{HumanoidComponent::HumanoidBone_LeftIndexDistal, {"LeftIndexDistal", "LeftHandIndex3", "index_03_l"}},
			{HumanoidComponent::HumanoidBone_LeftMiddleProximal, {"LeftMiddleProximal", "LeftHandMiddle1", "middle_01_l"}},
			{HumanoidComponent::HumanoidBone_LeftMiddleIntermediate, {"LeftMiddleIntermediate", "LeftHandMiddle2", "middle_02_l"}},
			{HumanoidComponent::HumanoidBone_LeftMiddleDistal, {"LeftMiddleDistal", "LeftHandMiddle3", "middle_03_l"}},
			{HumanoidComponent::HumanoidBone_LeftRingProximal, {"LeftRingProximal", "LeftHandRing1", "ring_01_l"}},
			{HumanoidComponent::HumanoidBone_LeftRingIntermediate, {"LeftRingIntermediate", "LeftHandRing2", "ring_02_l"}},
			{HumanoidComponent::HumanoidBone_LeftRingDistal, {"LeftRingDistal", "LeftHandRing3", "ring_03_l"}},
			{HumanoidComponent::HumanoidBone_LeftLittleProximal, {"LeftLittleProximal", "LeftHandPinky1", "pinky_01_l"}},
			{HumanoidComponent::HumanoidBone_LeftLittleIntermediate, {"LeftLittleIntermediate", "LeftHandPinky2", "pinky_02_l"}},
			{HumanoidComponent::HumanoidBone_LeftLittleDistal, {"LeftLittleDistal", "LeftHandPinky3", "pinky_03_l"}},
			{HumanoidComponent::HumanoidBone_RightThumbMetacarpal, {"RightThumbMetacarpal", "RightHandThumb1", "thumb_01_r"}},
			{HumanoidComponent::HumanoidBone_RightThumbProximal, {"RightThumbProximal", "RightHandThumb2", "thumb_02_r"}},
			{HumanoidComponent::HumanoidBone_RightThumbDistal, {"RightThumbDistal", "RightHandThumb3", "thumb_03_r"}},
			{HumanoidComponent::HumanoidBone_RightIndexIntermediate, {"RightIndexIntermediate", "RightHandIndex2", "index_01_r"}},
			{HumanoidComponent::HumanoidBone_RightIndexDistal, {"RightIndexDistal", "RightHandIndex3", "index_02_r"}},
			{HumanoidComponent::HumanoidBone_RightIndexProximal, {"RightIndexProximal", "RightHandIndex1", "index_03_r"}},
			{HumanoidComponent::HumanoidBone_RightMiddleProximal, {"RightMiddleProximal", "RightHandMiddle1", "middle_01_r"}},
			{HumanoidComponent::HumanoidBone_RightMiddleIntermediate, {"RightMiddleIntermediate", "RightHandMiddle2", "middle_02_r"}},
			{HumanoidComponent::HumanoidBone_RightMiddleDistal, {"RightMiddleDistal", "RightHandMiddle3", "middle_03_r"}},
			{HumanoidComponent::HumanoidBone_RightRingProximal, {"RightRingProximal", "RightHandRing1", "ring_01_r"}},
			{HumanoidComponent::HumanoidBone_RightRingIntermediate, {"RightRingIntermediate", "RightHandRing2", "ring_02_r"}},
			{HumanoidComponent::HumanoidBone_RightRingDistal, {"RightRingDistal", "RightHandRing3", "ring_03_r"}},
			{HumanoidComponent::HumanoidBone_RightLittleProximal, {"RightLittleProximal", "RightHandPinky1", "pinky_01_r"}},
			{HumanoidComponent::HumanoidBone_RightLittleIntermediate, {"RightLittleIntermediate", "RightHandPinky2", "pinky_02_r"}},
			{HumanoidComponent::HumanoidBone_RightLittleDistal, {"RightLittleDistal", "RightHandPinky3", "pinky_03_r"}},
		};

		HumanoidComponent humanoid;
		bool found_anything = false;

		for (size_t i = 0; i < armature->boneCollection.size(); ++i)
		{
			Entity bone = armature->boneCollection[i];
			NameComponent* name = scene.names.GetComponent(bone);
			if (name == nullptr)
				continue;

			size_t iType = 0;
			for (auto& humanoidBone : humanoid.bones)
			{
				if (humanoidBone == INVALID_ENTITY)
				{
					HumanoidComponent::HumanoidBone type = (HumanoidComponent::HumanoidBone)iType;
					auto it = mapping.find(type);
					if (it != mapping.end())
					{
						for (auto& candidate : it->second)
						{
							if (Protean::Helper::toUpper(name->name).find(Protean::Helper::toUpper(candidate)) != std::string::npos)
							{
								humanoidBone = bone;
								found_anything = true;
								break;
							}
						}
					}
				}
				iType++;
			}
		}

		if (found_anything)
		{
			scene.humanoids.Create(entity) = humanoid;
			editor->componentsWnd.humanoidWnd.SetEntity(INVALID_ENTITY);
			editor->componentsWnd.humanoidWnd.SetEntity(entity);
		}
		else
		{
			Protean::Helper::messageBox("No matching humanoid bones found!");
		}
	});
	AddWidget(&createHumanoidButton);

	boneList.Create("Bones: ");
	boneList.SetSize(XMFLOAT2(wid, 200));
	boneList.SetPos(XMFLOAT2(4, y += step));
	boneList.OnSelect([=](Protean::Gui::EventArgs args) {

		if (args.iValue < 0)
			return;

		Protean::Archive& archive = editor->AdvanceHistory(true);
		archive << EditorComponent::HISTORYOP_SELECTION;
		// record PREVIOUS selection state...
		editor->RecordSelection(archive);

		editor->translator.selected.clear();

		for (int i = 0; i < boneList.GetItemCount(); ++i)
		{
			const Protean::Gui::TreeList::Item& item = boneList.GetItem(i);
			if (item.selected)
			{
				Protean::PickResult pick;
				pick.entity = (Entity)item.userdata;
				editor->AddSelected(pick);
			}
		}

		// record NEW selection state...
		editor->RecordSelection(archive);

		editor->componentsWnd.RefreshEntityTree();

		});
	AddWidget(&boneList);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ArmatureWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	Scene& scene = editor->GetCurrentScene();

	const ArmatureComponent* armature = scene.armatures.GetComponent(entity);

	if (armature != nullptr || (IsCollapsed() && !scene.Entity_IsDescendant(entity, this->entity)))
	{
		this->entity = entity;
		RefreshBoneList();
	}
}
void ArmatureWindow::RefreshBoneList()
{
	Scene& scene = editor->GetCurrentScene();

	const ArmatureComponent* armature = scene.armatures.GetComponent(entity);

	if (armature != nullptr)
	{
		boneList.ClearItems();
		for (Entity bone : armature->boneCollection)
		{
			Protean::Gui::TreeList::Item item;
			item.userdata = bone;
			item.name += ICON_BONE " ";

			const NameComponent* name = scene.names.GetComponent(bone);
			if (name == nullptr)
			{
				item.name += "[no_name] " + std::to_string(bone);
			}
			else if (name->name.empty())
			{
				item.name += "[name_empty] " + std::to_string(bone);
			}
			else
			{
				item.name += name->name;
			}
			boneList.AddItem(item);
		}
	}
}

void ArmatureWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();
	layout.margin_left = 110;

	layout.add_fullwidth(infoLabel);
	layout.add_fullwidth(resetPoseButton);
	layout.add_fullwidth(createHumanoidButton);

	layout.jump();

	layout.add_fullwidth(boneList);

}
