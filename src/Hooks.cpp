#include "Hooks.h"

namespace MagickaShield
{
	const SKSE::LoadInterface* g_loadInterface = nullptr;

	static RE::Setting* get_gmst(const char* a_setting)
	{
		return RE::GameSettingCollection::GetSingleton()->GetSetting(a_setting);
	}

	static RE::ActorValue magickaShieldAV = RE::ActorValue::kNone;

	RE::ActorValue LookupActorValueByName(const char* av_name) {
		using func_t = decltype(&LookupActorValueByName);
		REL::Relocation<func_t> func{RELOCATION_ID(26570, 27203)};
		return func(av_name);
	}

	static float GetDifficultyMultiplier(RE::Actor* a_aggressor, RE::Actor* a_target)
	{
		float mult = 1.0f;
		auto diff = RE::PlayerCharacter::GetSingleton()->difficulty;
		if ((a_aggressor) && (a_aggressor->IsPlayerRef() || a_aggressor->IsPlayerTeammate())) {
			switch (diff) {
			case 0: mult = get_gmst("fDiffMultHPByPCVE")->GetFloat(); break;
			case 1: mult = get_gmst("fDiffMultHPByPCE")->GetFloat(); break;
			case 2: mult = get_gmst("fDiffMultHPByPCN")->GetFloat(); break;
			case 3: mult = get_gmst("fDiffMultHPByPCH")->GetFloat(); break;
			case 4: mult = get_gmst("fDiffMultHPByPCVH")->GetFloat(); break;
			case 5: mult = get_gmst("fDiffMultHPByPCL")->GetFloat(); break;
			}
		}
		else if ((a_target) && (a_target->IsPlayerRef() || a_target->IsPlayerTeammate())) {
			switch (diff) {
			case 0: mult = get_gmst("fDiffMultHPToPCVE")->GetFloat(); break;
			case 1: mult = get_gmst("fDiffMultHPToPCE")->GetFloat(); break;
			case 2: mult = get_gmst("fDiffMultHPToPCN")->GetFloat(); break;
			case 3: mult = get_gmst("fDiffMultHPToPCH")->GetFloat(); break;
			case 4: mult = get_gmst("fDiffMultHPToPCVH")->GetFloat(); break;
			case 5: mult = get_gmst("fDiffMultHPToPCL")->GetFloat(); break;
			}
		}
		return mult;
	}

	static void AbsorbWithMagicka(RE::Actor* a_target, float& a_damage, float a_magicka, float a_difficultyMult = 1.0f)
	{
		float shieldPercent = a_target->GetActorValue(magickaShieldAV);
		if (shieldPercent > 0.0f) {
			float shieldFraction = (std::clamp)(shieldPercent/100.0f, 0.0f, 1.0f);
			float magickaDamage = a_damage * shieldFraction * a_difficultyMult;
			if (magickaDamage > 0.0f) {
				float absorbFraction = (std::min)(a_magicka/magickaDamage, 1.0f);
				a_target->DamageActorValue(RE::ActorValue::kMagicka, magickaDamage * absorbFraction);
				a_damage *= 1.0f - shieldFraction * absorbFraction;
			}
		}
	}

	struct DoDamage
	{
		static bool thunk(RE::Actor* a_this, float a_healthDamage, RE::Actor* a_source, bool a_dontAdjustDifficulty)
		{
			if (a_this && a_healthDamage > 0.0f) {
				float magicka = a_this->GetActorValue(RE::ActorValue::kMagicka);
				if (magicka > 0.0f) {
					float difficultyMult = a_dontAdjustDifficulty ? 1.0f : GetDifficultyMultiplier(a_source, a_this);
					AbsorbWithMagicka(a_this, a_healthDamage, magicka, difficultyMult);
				}
			}
			return func(a_this, a_healthDamage, a_source, a_dontAdjustDifficulty);
		}
		FUNCTYPE_DETOUR func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> hook{ RELOCATION_ID(36345, 37335) }; 
		stl::write_detour<DoDamage>(hook.address());
		logger::info("Hooked Damage Health"sv);
	}

	void CheckForAVG()
	{
		if (g_loadInterface->GetPluginInfo("ActorValueExtension")) {
			magickaShieldAV = LookupActorValueByName("MagickaShield");
			logger::info("Actor Value Generator detected; using custom MagickaShield AV."sv);
		}
		else {
			magickaShieldAV = RE::ActorValue::kShieldPerks;
			logger::info("Actor Value Generator not detected; using ShieldPerks AV."sv);
		}
	}
}