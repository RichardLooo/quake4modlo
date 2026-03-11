
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class rvWeaponNailgun : public rvWeapon {
public:

	CLASS_PROTOTYPE(rvWeaponNailgun);

	rvWeaponNailgun(void);

	virtual void		Spawn(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);
	void				PreSave(void);
	void				PostSave(void);

private:

	stateResult_t		State_Raise(const stateParms_t& parms);
	stateResult_t		State_Lower(const stateParms_t& parms);
	stateResult_t		State_Idle(const stateParms_t& parms);
	stateResult_t		State_Fire(const stateParms_t& parms);
	stateResult_t		State_Reload(const stateParms_t& parms);

	CLASS_STATES_PROTOTYPE(rvWeaponNailgun);
};

CLASS_DECLARATION(rvWeapon, rvWeaponNailgun)
END_CLASS

/*
================
rvWeaponNailgun::rvWeaponNailgun
================
*/
rvWeaponNailgun::rvWeaponNailgun(void) {
}

/*
================
rvWeaponNailgun::Spawn
================
*/
void rvWeaponNailgun::Spawn(void) {
	SetState("Raise", 0);
}

/*
================
rvWeaponNailgun::Save
================
*/
void rvWeaponNailgun::Save(idSaveGame* savefile) const {
}

/*
================
rvWeaponNailgun::Restore
================
*/
void rvWeaponNailgun::Restore(idRestoreGame* savefile) {
}

/*
================
rvWeaponNailgun::PreSave
================
*/
void rvWeaponNailgun::PreSave(void) {
	SetState("Idle", 4);
	StopSound(SND_CHANNEL_WEAPON, 0);
	StopSound(SND_CHANNEL_BODY, 0);
	StopSound(SND_CHANNEL_ITEM, 0);
	StopSound(SND_CHANNEL_ANY, false);
}

/*
================
rvWeaponNailgun::PostSave
================
*/
void rvWeaponNailgun::PostSave(void) {
}

/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvWeaponNailgun)
STATE("Raise", rvWeaponNailgun::State_Raise)
STATE("Lower", rvWeaponNailgun::State_Lower)
STATE("Idle", rvWeaponNailgun::State_Idle)
STATE("Fire", rvWeaponNailgun::State_Fire)
STATE("Reload", rvWeaponNailgun::State_Reload)
END_CLASS_STATES

/*
================
rvWeaponNailgun::State_Raise
================
*/
stateResult_t rvWeaponNailgun::State_Raise(const stateParms_t& parms) {
	enum {
		RAISE_INIT,
		RAISE_WAIT,
	};
	switch (parms.stage) {
	case RAISE_INIT:
		SetStatus(WP_RISING);
		PlayAnim(ANIMCHANNEL_ALL, "raise", parms.blendFrames);
		return SRESULT_STAGE(RAISE_WAIT);

	case RAISE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_Lower
================
*/
stateResult_t rvWeaponNailgun::State_Lower(const stateParms_t& parms) {
	enum {
		LOWER_INIT,
		LOWER_WAIT,
		LOWER_WAITRAISE,
	};
	switch (parms.stage) {
	case LOWER_INIT:
		SetStatus(WP_LOWERING);
		PlayAnim(ANIMCHANNEL_ALL, "putaway", parms.blendFrames);
		return SRESULT_STAGE(LOWER_WAIT);

	case LOWER_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 0)) {
			SetStatus(WP_HOLSTERED);
			return SRESULT_STAGE(LOWER_WAITRAISE);
		}
		return SRESULT_WAIT;

	case LOWER_WAITRAISE:
		if (wsfl.raiseWeapon) {
			SetState("Raise", 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_Idle
================
*/
stateResult_t rvWeaponNailgun::State_Idle(const stateParms_t& parms) {
	enum {
		IDLE_INIT,
		IDLE_WAIT,
	};
	switch (parms.stage) {
	case IDLE_INIT:
		SetStatus(WP_READY);
		PlayCycle(ANIMCHANNEL_ALL, "idle", parms.blendFrames);
		return SRESULT_STAGE(IDLE_WAIT);

	case IDLE_WAIT:
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		// Auto reload when clip is empty
		if (!AmmoInClip() && AmmoAvailable() && AutoReload()) {
			SetState("Reload", 4);
			return SRESULT_DONE;
		}
		if (wsfl.reload && AmmoAvailable() > AmmoInClip() && AmmoInClip() < ClipSize()) {
			SetState("Reload", 4);
			return SRESULT_DONE;
		}
		if (wsfl.attack && gameLocal.time >= nextAttackTime && AmmoInClip() > 0) {
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier(PMOD_FIRERATE));
			SetState("Fire", 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_Fire
================
*/
stateResult_t rvWeaponNailgun::State_Fire(const stateParms_t& parms) {
	enum {
		FIRE_INIT,
		FIRE_WAIT,
	};
	switch (parms.stage) {
	case FIRE_INIT:
	{
		idPlayer* player = gameLocal.GetLocalPlayer();

		if (player && player->GuiActive()) {
			SetState("Lower", 0);
			return SRESULT_DONE;
		}
		if (player && !player->CanFire()) {
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		StartSound("snd_fire", SND_CHANNEL_WEAPON, 0, false, NULL);
		Attack(false, 1, spread, 0, 1.0f);
		PlayEffect("fx_normalflash", barrelJointView, false);
		PlayAnim(ANIMCHANNEL_ALL, "fire", parms.blendFrames);

		return SRESULT_STAGE(FIRE_WAIT);
	}
	case FIRE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			// Auto reload if clip empty after shot
			if (!AmmoInClip() && AmmoAvailable() && AutoReload()) {
				SetState("Reload", 4);
			}
			else {
				SetState("Idle", 4);
			}
			return SRESULT_DONE;
		}
		if (wsfl.attack && gameLocal.time >= nextAttackTime && AmmoInClip() > 0) {
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier(PMOD_FIRERATE));
			SetState("Fire", 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponNailgun::State_Reload
================
*/
stateResult_t rvWeaponNailgun::State_Reload(const stateParms_t& parms) {
	enum {
		RELOAD_INIT,
		RELOAD_WAIT,
	};
	switch (parms.stage) {
	case RELOAD_INIT:
		SetStatus(WP_RELOAD);

		if (wsfl.netReload) {
			wsfl.netReload = false;
		}
		else {
			NetReload();
		}

		StartSound("snd_reload", SND_CHANNEL_ITEM, 0, false, NULL);
		PlayAnim(ANIMCHANNEL_ALL, "reload", parms.blendFrames);
		return SRESULT_STAGE(RELOAD_WAIT);

	case RELOAD_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			AddToClip(ClipSize());
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}