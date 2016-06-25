#include "demo.hpp"
#include "file_stream.hpp"
#include "utils.hpp"
#include "sprites.hpp"

#include <stdlib.h>
#include <math.h>


// These should probably be in a configuration file.
#define  MAX_FIXABLE_PLAYER_BLINK_TIME_MS  1000
#define  RAIL_BEAM_DURATION_MS             500

// True constants.
#define  LG_BEAM_LENGTH  768


/*
Static items can only spawn at a fixed location and can't be dropped by a player.
The same items can be static in some cases and dynamic in others:
- example #1: quad damage is static in CPMA 2v2/TDM but dynamic in CPMA CTF/FFA
- example #2: ammo is static in QL 1v1 and dynamic in CPMA 1v1
*/

struct StaticItemSpawnTime
{
	udtItem::Id Id;
	u32 SpawnTime;
};

struct ItemClass
{
	enum Id
	{
		Static,
		Dynamic,
		Unknown,
		Count
	};
};

static u32 GetItemSpawnTimeMs(u32 itemId)
{
	if(itemId >= (u32)udtItem::AmmoFirst &&
	   itemId <= (u32)udtItem::AmmoLast)
	{
		return 30 * 1000;
	}

	if(itemId >= (u32)udtItem::WeaponFirst &&
	   itemId <= (u32)udtItem::WeaponLast)
	{
		// @TODO:
		return 5 * 1000;
	}

	if(itemId == (u32)udtItem::ItemHealthMega)
	{
		// @TODO: 35 vs 20
		return 20 * 1000;
	}

	switch((udtItem::Id)itemId)
	{
		case udtItem::ItemArmorBody:
		case udtItem::ItemArmorCombat:
		case udtItem::ItemArmorJacket:
		case udtItem::ItemArmorShard:
		case udtItem::ItemHealth:
		case udtItem::ItemHealthLarge:
		case udtItem::ItemHealthSmall:
			return 25 * 1000;

		default:
			return 0;
	}
}

static u32 GetItemIdFromDynamicItemId(u32 dynItemId)
{
	switch((DynamicItemType::Id)dynItemId)
	{
		case DynamicItemType::AmmoBullets: return udtItem::AmmoBullets;
		case DynamicItemType::AmmoCells: return udtItem::AmmoCells;
		case DynamicItemType::AmmoGrenades: return udtItem::AmmoGrenades;
		case DynamicItemType::AmmoLightning: return udtItem::AmmoLightning;
		case DynamicItemType::AmmoRockets: return udtItem::AmmoRockets;
		case DynamicItemType::AmmoShells: return udtItem::AmmoShells;
		case DynamicItemType::AmmoSlugs: return udtItem::AmmoSlugs;
		case DynamicItemType::ItemEnviro: return udtItem::ItemEnviro;
		case DynamicItemType::ItemFlight: return udtItem::ItemFlight;
		case DynamicItemType::ItemHaste: return udtItem::ItemHaste;
		case DynamicItemType::ItemInvis: return udtItem::ItemInvis;
		case DynamicItemType::ItemQuad: return udtItem::ItemQuad;
		case DynamicItemType::ItemRegen: return udtItem::ItemRegen;
		case DynamicItemType::FlagBlue: return udtItem::FlagBlue;
		case DynamicItemType::FlagRed: return udtItem::FlagRed;
		case DynamicItemType::WeaponBFG: return udtItem::WeaponBFG;
		case DynamicItemType::WeaponGauntlet: return udtItem::WeaponGauntlet;
		case DynamicItemType::WeaponGrenadeLauncher: return udtItem::WeaponGrenadeLauncher;
		case DynamicItemType::WeaponLightningGun: return udtItem::WeaponLightningGun;
		case DynamicItemType::WeaponMachinegun: return udtItem::WeaponMachinegun;
		case DynamicItemType::WeaponPlasmaGun: return udtItem::WeaponPlasmaGun;
		case DynamicItemType::WeaponRailgun: return udtItem::WeaponRailgun;
		case DynamicItemType::WeaponRocketLauncher: return udtItem::WeaponRocketLauncher;
		case DynamicItemType::WeaponShotgun: return udtItem::WeaponShotgun;
		default: return udtItem::Count;
	}
}

static s32 GetDynamicItemSpawnTimeMs(u32 dynItemId)
{
	switch((DynamicItemType::Id)dynItemId)
	{
		case DynamicItemType::ProjectileRocket:
		case DynamicItemType::ProjectileGrenade:
			return 1000;

		case DynamicItemType::ProjectilePlasma:
			return 100;

		default:
			return GetItemSpawnTimeMs(GetItemIdFromDynamicItemId(dynItemId));
	}
}

ItemClass::Id GetItemClassFromId(u32 itemId)
{
	switch((udtItem::Id)itemId)
	{
		case udtItem::HoldableInvulnerability:
		case udtItem::HoldableKamikaze:
		case udtItem::HoldableMedkit:
		case udtItem::HoldablePortal:
		case udtItem::HoldableTeleporter:
		case udtItem::ItemArmorBody:
		case udtItem::ItemArmorCombat:
		case udtItem::ItemArmorJacket:
		case udtItem::ItemArmorShard:
		case udtItem::ItemAmmoRegen:
		case udtItem::ItemDoubler:
		case udtItem::ItemGuard:
		case udtItem::ItemScout:
		case udtItem::ItemHealth:
		case udtItem::ItemHealthLarge:
		case udtItem::ItemHealthMega:
		case udtItem::ItemHealthSmall:
		case udtItem::ItemKeyGold:
		case udtItem::ItemKeyMaster:
		case udtItem::ItemKeySilver:
			return ItemClass::Static;
	}

	return ItemClass::Unknown;
}

struct DynamicItemPair
{
	DynamicItemType::Id DynamicItemId;
	udtItem::Id ItemId;
};

#define REAL_DYNAMIC_ITEM_LIST(N) \
	N(AmmoBullets) \
	N(AmmoCells) \
	N(AmmoGrenades) \
	N(AmmoLightning) \
	N(AmmoRockets) \
	N(AmmoShells) \
	N(AmmoSlugs) \
	N(ItemEnviro) \
	N(ItemFlight) \
	N(ItemHaste) \
	N(ItemInvis) \
	N(ItemQuad) \
	N(ItemRegen) \
	N(FlagBlue) \
	N(FlagRed) \
	N(WeaponGauntlet) \
	N(WeaponGrenadeLauncher) \
	N(WeaponLightningGun) \
	N(WeaponMachinegun) \
	N(WeaponPlasmaGun) \
	N(WeaponRailgun) \
	N(WeaponRocketLauncher) \
	N(WeaponShotgun)

#define ITEM(Enum) Enum,
struct RealDynamicItemType
{
	enum Id
	{
		REAL_DYNAMIC_ITEM_LIST(ITEM)
		Count
	};
};
#undef ITEM

#define ITEM(Enum) { DynamicItemType::Enum, udtItem::Enum },
static const DynamicItemPair DynamicItemPairs[RealDynamicItemType::Count + 1] =
{
	REAL_DYNAMIC_ITEM_LIST(ITEM)
	{ DynamicItemType::AmmoBullets, udtItem::AmmoBullets }
};
#undef ITEM

static void ComputeTrajectoryPosition(f32* pos, const idTrajectoryBase& tr, s32 serverTimeMs)
{
	switch(tr.trType)
	{
		case ID_TR_LINEAR:
			Float3::Mad(pos, tr.trBase, tr.trDelta, (serverTimeMs - tr.trTime) / 1000.0f);
			break;

		case ID_TR_SINE:
		{
			const f32 deltaTime = (serverTimeMs - tr.trTime) / (f32)tr.trDuration;
			const f32 phase = sinf(deltaTime * UDT_PI * 2.0f);
			Float3::Mad(pos, tr.trBase, tr.trDelta, phase);
		}
		break;

		case ID_TR_LINEAR_STOP:
		{
			if(serverTimeMs > tr.trTime + tr.trDuration)
			{
				serverTimeMs = tr.trTime + tr.trDuration;
			}

			f32 deltaTime = (serverTimeMs - tr.trTime) * 0.001f;
			if(deltaTime < 0.0f)
			{
				deltaTime = 0.0f;
			}

			Float3::Mad(pos, tr.trBase, tr.trDelta, deltaTime);
		}
		break;

		case ID_TR_GRAVITY:
		{
			const f32 gravity = 800.0f;
			const f32 deltaTime = (serverTimeMs - tr.trTime) * 0.001f;
			Float3::Mad(pos, tr.trBase, tr.trDelta, deltaTime);
			pos[2] -= 0.5f * gravity * deltaTime * deltaTime;
		}
		break;

		case ID_TR_STATIONARY:
		case ID_TR_INTERPOLATE:
		default:
			Float3::Copy(pos, tr.trBase);
			break;
	}
}

static f32 ComputeProjectileAngle(const idEntityStateBase& es)
{
	if(es.pos.trDelta[0] == 0.0f && es.pos.trDelta[1] == 0.0f)
	{
		return 0.0f;
	}

	return atan2f(es.pos.trDelta[0], es.pos.trDelta[1]);
}

static f32 ComputePlayerAngle(const idEntityStateBase& es, s32 serverTimeMs)
{
	f32 angles[3];
	ComputeTrajectoryPosition(angles, es.apos, serverTimeMs);

	return (angles[1] / 180.0f) * UDT_PI;
}

idProtocolNumbers::idProtocolNumbers()
{
	memset(this, 0, sizeof(idProtocolNumbers));
}

void idProtocolNumbers::GetNumbers(u32 protocol, u32 mod)
{
	udtGetIdMagicNumber(&EntityTypePlayer, udtMagicNumberType::EntityType, udtEntityType::Player, protocol, mod);
	udtGetIdMagicNumber(&EntityTypeItem, udtMagicNumberType::EntityType, udtEntityType::Item, protocol, mod);
	udtGetIdMagicNumber(&EntityTypeMissile, udtMagicNumberType::EntityType, udtEntityType::Missile, protocol, mod);
	udtGetIdMagicNumber(&EntityTypeGeneral, udtMagicNumberType::EntityType, udtEntityType::General, protocol, mod);
	udtGetIdMagicNumber(&EntityTypeEvent, udtMagicNumberType::EntityType, udtEntityType::Event, protocol, mod);
	udtGetIdMagicNumber(&WeaponRocket, udtMagicNumberType::Weapon, udtWeapon::RocketLauncher, protocol, mod);
	udtGetIdMagicNumber(&WeaponGrenade, udtMagicNumberType::Weapon, udtWeapon::GrenadeLauncher, protocol, mod);
	udtGetIdMagicNumber(&WeaponPlasma, udtMagicNumberType::Weapon, udtWeapon::PlasmaGun, protocol, mod);
	udtGetIdMagicNumber(&WeaponShaft, udtMagicNumberType::Weapon, udtWeapon::LightningGun, protocol, mod);
	udtGetIdMagicNumber(&CSIndexFirstPlayer, udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, protocol, mod);
	udtGetIdMagicNumber(&EntityFlagDead, udtMagicNumberType::EntityFlag, udtEntityFlag::Dead, protocol, mod);
	udtGetIdMagicNumber(&EntityFlagFiring, udtMagicNumberType::EntityFlag, udtEntityFlag::Firing, protocol, mod);
	udtGetIdMagicNumber(&EntityFlagNoDraw, udtMagicNumberType::EntityFlag, udtEntityFlag::NoDraw, protocol, mod);
	udtGetIdMagicNumber(&EntityFlagPlayerEvent, udtMagicNumberType::EntityFlag, udtEntityFlag::PlayerEvent, protocol, mod);
	udtGetIdMagicNumber(&EntityFlagTelePortBit, udtMagicNumberType::EntityFlag, udtEntityFlag::TeleportBit, protocol, mod);
	udtGetIdMagicNumber(&EntityEventBulletHitFlesh, udtMagicNumberType::EntityEvent, udtEntityEvent::BulletHitFlesh, protocol, mod);
	udtGetIdMagicNumber(&EntityEventBulletHitWall, udtMagicNumberType::EntityEvent, udtEntityEvent::BulletHitWall, protocol, mod);
	udtGetIdMagicNumber(&EntityEventMissileHit, udtMagicNumberType::EntityEvent, udtEntityEvent::MissileHit, protocol, mod);
	udtGetIdMagicNumber(&EntityEventMissileMiss, udtMagicNumberType::EntityEvent, udtEntityEvent::MissileMiss, protocol, mod);
	udtGetIdMagicNumber(&EntityEventMissileMissMetal, udtMagicNumberType::EntityEvent, udtEntityEvent::MissileMissMetal, protocol, mod);
	udtGetIdMagicNumber(&EntityEventRailTrail, udtMagicNumberType::EntityEvent, udtEntityEvent::RailTrail, protocol, mod);
	udtGetIdMagicNumber(&PlayerStatsHealth, udtMagicNumberType::LifeStatsIndex, udtLifeStatsIndex::Health, protocol, mod);
	udtGetIdMagicNumber(&PlayerStatsArmor, udtMagicNumberType::LifeStatsIndex, udtLifeStatsIndex::Armor, protocol, mod);

	for(u32 i = 0; i < (u32)UDT_COUNT_OF(DynamicItemIds); ++i)
	{
		DynamicItemIds[i] = -666;
	}
	
	for(u32 i = 0; i < (u32)RealDynamicItemType::Count; ++i)
	{
		udtGetIdMagicNumber(&DynamicItemIds[DynamicItemPairs[i].DynamicItemId], udtMagicNumberType::Item, DynamicItemPairs[i].ItemId, protocol, mod);
	}
}

Demo::Demo()
{
	_snapshots[0].Init(UDT_MEMORY_PAGE_SIZE, "Demo::SnapshotOffsetArray0");
	_snapshots[1].Init(UDT_MEMORY_PAGE_SIZE, "Demo::SnapshotOffsetArray1");
	_snapshotAllocators[0].DisableFourByteAlignment();
	_snapshotAllocators[1].DisableFourByteAlignment();
	_snapshotAllocators[0].Init(UDT_KB(64), "Demo::Persist0");
	_snapshotAllocators[1].Init(UDT_KB(64), "Demo::Persist1");
	_stringAllocator.Init(UDT_KB(64), "Demo::Strings");
	_staticItems.Init(UDT_MEMORY_PAGE_SIZE, "Demo::StaticItemsArray");
	_tempPlayers.Init(UDT_MEMORY_PAGE_SIZE, "Demo::TempPlayersArray");
	_tempDynamicItems.Init(UDT_MEMORY_PAGE_SIZE, "Demo::TempDynamicItemsArray");
	_tempBeams.Init(UDT_MEMORY_PAGE_SIZE, "Demo::TempBeamsArray");
	_beams.Init(UDT_MEMORY_PAGE_SIZE, "Demo::BeamsArray");
	_tempShaftImpacts.Init(UDT_MEMORY_PAGE_SIZE, "Demo::TempShaftImpactsArray");
	_explosions.Init(UDT_MEMORY_PAGE_SIZE, "Demo::ExplosionsArray");
	_bulletImpacts.Init(UDT_MEMORY_PAGE_SIZE, "Demo::BulletImpactsArray");
	_scores.Init(UDT_MEMORY_PAGE_SIZE, "Demo::ScoresArray");
}

Demo::~Demo()
{
	udtCuDestroyContext(_context);
	free(_messageData);
	free(_snapshot);
}

bool Demo::Init(ProgressCallback progressCallback, void* userData)
{
	assert(progressCallback != nullptr);
	assert(userData != nullptr);

	udtCuContext* const context = udtCuCreateContext();
	if(context == NULL)
	{
		return false;
	}
	_context = context;
	
	u8* const inMsgData = (u8*)malloc(ID_MAX_MSG_LENGTH);
	if(inMsgData == NULL)
	{
		return false;
	}
	_messageData = inMsgData;

	Snapshot* const snapshot = (Snapshot*)malloc(sizeof(Snapshot));
	if(snapshot == nullptr)
	{
		return false;
	}
	_snapshot = snapshot;

	_progressCallback = progressCallback;
	_userData = userData;

	return true;
}

void Demo::Load(const char* filePath)
{
	const f32 LoadStepCount = 6.0f;
	f32 loadStep = 1.0f;
	(*_progressCallback)(0.0f, _userData);

	_readIndex = 0;
	_writeIndex = 0;
	for(u32 i = 0; i < 2; ++i)
	{
		_snapshots[i].Clear();
		_snapshotAllocators[i].Clear();
	}
	_stringAllocator.Clear();

	const u32 previousProtocol = _protocol;
	const u32 protocol = udtGetProtocolByFilePath(filePath);
	_protocol = protocol;

	for(u32 i = 0; i < 3; ++i)
	{
		_min[i] = 99999.0f;
		_max[i] = -99999.0f;
	}

	const u32 previousMod = _mod;
	if(!AnalyzeDemo(filePath) && protocol <= udtProtocol::Dm68)
	{
		ParseDemo(filePath, &Demo::ProcessMessage_Mod);
	}
	
	const u32 mod = _mod;
	if(protocol != previousProtocol || mod != previousMod)
	{
		_protocolNumbers.GetNumbers(protocol, mod);
	}

	(*_progressCallback)(loadStep / LoadStepCount, _userData);
	ParseDemo(filePath, &Demo::ProcessMessage_StaticItems);
	loadStep += 1.0f;

	(*_progressCallback)(loadStep / LoadStepCount, _userData);
	ParseDemo(filePath, &Demo::ProcessMessage_FinalPass);
	loadStep += 1.0f;

	if(_snapshots[_readIndex].GetSize() == 0)
	{
		return;
	}

	(*_progressCallback)(loadStep / LoadStepCount, _userData);
	FixStaticItems();
	loadStep += 1.0f;

	(*_progressCallback)(loadStep / LoadStepCount, _userData);
	FixDynamicItemsAndPlayers();
	loadStep += 1.0f;

	(*_progressCallback)(loadStep / LoadStepCount, _userData);
	FixLGEndPoints();

	const auto& snapshots = _snapshots[_readIndex];
	const u32 lastIndex = snapshots.GetSize() - 1;
	_firstSnapshotTimeMs = snapshots[0].ServerTimeMs;
	_lastSnapshotTimeMs = snapshots[lastIndex].ServerTimeMs;

	(*_progressCallback)(1.0f, _userData);
}

const char* Demo::GetStringSafe(u32 offset, const char* replacement) const
{
	const char* const s = GetString(offset);
	if(s == nullptr)
	{
		return replacement;
	}

	return s;
}

u32 Demo::GetSnapshotIndexFromServerTime(s32 serverTimeMs) const
{
	if(serverTimeMs < _firstSnapshotTimeMs)
	{
		return 0;
	}

	const u32 lastIndex = _snapshots[_readIndex].GetSize() - 1;
	if(serverTimeMs >= _lastSnapshotTimeMs)
	{
		return lastIndex;
	}

	u32 min = 0;
	u32 max = lastIndex - 1;
	const auto& snapshots = _snapshots[_readIndex];
	for(;;)
	{
		const u32 i = (min + max) / 2;
		if(serverTimeMs < snapshots[i].ServerTimeMs)
		{
			max = i - 1;
			continue;
		}

		if(serverTimeMs >= snapshots[i + 1].ServerTimeMs)
		{
			min = i + 1;
			continue;
		}

		return i;
	}
}

s32 Demo::GetSnapshotServerTimeMs(u32 index) const
{
	const auto& snapshots = _snapshots[_readIndex];
	if(index >= snapshots.GetSize())
	{
		return 0;
	}

	uptr offset = snapshots[index].Offset;
	s32 serverTimeMs;
	Read(offset, serverTimeMs);

	return serverTimeMs;
}

bool Demo::GetSnapshotData(Snapshot& snapshot, u32 index) const
{
	const auto& snapshots = _snapshots[_readIndex];
	if(index >= snapshots.GetSize())
	{
		return false;
	}

	uptr offset = snapshots[index].Offset;

	Read(offset, snapshot.ServerTimeMs);

	u8 staticItemBits[MaxItemMaskByteCount];
	const u32 staticItemCount = _staticItems.GetSize();
	const u32 staticItemByteCount = (staticItemCount + 7) / 8;
	Read(offset, staticItemBits, staticItemByteCount);
	snapshot.StaticItemCount = 0;
	for(u32 i = 0; i < staticItemCount; ++i)
	{
		if(!IsBitSet(staticItemBits, i))
		{
			continue;
		}

		snapshot.StaticItems[snapshot.StaticItemCount] = _staticItems[i];
		++snapshot.StaticItemCount;
	}
	assert(snapshot.StaticItemCount <= MAX_STATIC_ITEMS);

	Read(offset, snapshot.PlayerCount);
	assert(snapshot.PlayerCount <= 64);
	Read(offset, snapshot.Players, snapshot.PlayerCount * (u32)sizeof(Player));

	Read(offset, snapshot.DynamicItemCount);
	assert(snapshot.DynamicItemCount <= MAX_DYN_ITEMS);
	Read(offset, snapshot.DynamicItems, snapshot.DynamicItemCount * (u32)sizeof(DynamicItem));

	Read(offset, snapshot.RailBeamCount);
	assert(snapshot.RailBeamCount <= MAX_RAIL_BEAMS);
	Read(offset, snapshot.RailBeams, snapshot.RailBeamCount * (u32)sizeof(RailBeam));

	Read(offset, snapshot.Core);

	// @TODO: binary search
	const Score* score = nullptr;
	for(u32 i = 0; i < _scores.GetSize(); ++i)
	{
		if(snapshot.ServerTimeMs >= _scores[i].ServerTimeMs)
		{
			score = &_scores[i];
		}
	}

	if(score == nullptr)
	{
		snapshot.Score.IsScoreTeamBased = 0;
		snapshot.Score.Score1Id = 0;
		snapshot.Score.Score2Id = 0;
		snapshot.Score.Score1 = 0;
		snapshot.Score.Score2 = 0;
		snapshot.Score.Score1Name = u32(-1);
		snapshot.Score.Score2Name = u32(-1);
	}
	else
	{
		snapshot.Score = score->Base;
	}

	return true;
}

void Demo::WriteSnapshot(const Snapshot& snapshot)
{
	SnapshotDesc snapDesc;
	snapDesc.ServerTimeMs = snapshot.ServerTimeMs;
	snapDesc.Offset = (u32)_snapshotAllocators[_writeIndex].GetCurrentByteCount();
	_snapshots[_writeIndex].Add(snapDesc);

	Write(snapshot.ServerTimeMs);

	u8 staticItemBits[MaxItemMaskByteCount];
	memset(staticItemBits, 0, sizeof(staticItemBits));
	const u32 staticItemCount = _staticItems.GetSize();
	const u32 staticItemByteCount = (staticItemCount + 7) / 8;
	for(u32 i = 0; i < snapshot.StaticItemCount; ++i)
	{
		const auto& snapItem = snapshot.StaticItems[i];
		for(u32 j = 0; j < staticItemCount; ++j)
		{
			const auto& regItem = _staticItems[j];
			if(snapItem.Id == regItem.Id &&
			   snapItem.Position[0] == regItem.Position[0] &&
			   snapItem.Position[1] == regItem.Position[1] &&
			   snapItem.Position[2] == regItem.Position[2])
			{
				SetBit(staticItemBits, j);
				break;
			}
		}
	}
	assert(staticItemCount <= MAX_STATIC_ITEMS);
	Write(staticItemBits, staticItemByteCount);

	assert(snapshot.PlayerCount <= 64);
	Write(snapshot.PlayerCount);
	Write(snapshot.Players, snapshot.PlayerCount * (u32)sizeof(Player));

	assert(snapshot.DynamicItemCount <= MAX_DYN_ITEMS);
	Write(snapshot.DynamicItemCount);
	Write(snapshot.DynamicItems, snapshot.DynamicItemCount * (u32)sizeof(DynamicItem));

	assert(snapshot.RailBeamCount <= MAX_RAIL_BEAMS);
	Write(snapshot.RailBeamCount);
	Write(snapshot.RailBeams, snapshot.RailBeamCount * (u32)sizeof(RailBeam));

	Write(snapshot.Core);
}

void Demo::Read(uptr& offset, void* data, u32 byteCount) const
{
	memcpy(data, _snapshotAllocators[_readIndex].GetAddressAt(offset), (size_t)byteCount);
	offset += (uptr)byteCount;
}

void Demo::Write(const void* data, u32 byteCount)
{
	u8* const dest = _snapshotAllocators[_writeIndex].AllocateAndGetAddress(byteCount);
	memcpy(dest, data, (size_t)byteCount);
}

void Demo::ParseDemo(const char* filePath, MessageHandler messageHandler)
{
	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Read))
	{
		return;
	}

	udtCuContext* const context = _context;
	s32 errorCode = udtCuStartParsing(context, _protocol);
	if(errorCode != udtErrorCode::None)
	{
		return;
	}

	udtCuMessageInput input;
	udtCuMessageOutput output;
	u32 continueParsing = 0;
	u32 gsIndex = 0;
	for(;;)
	{
		if(file.Read(&input.MessageSequence, 4, 1) != 1)
		{
			break;
		}

		if(file.Read(&input.BufferByteCount, 4, 1) != 1)
		{
			break;
		}

		if(input.MessageSequence == -1 &&
		   input.BufferByteCount == u32(-1))
		{
			break;
		}

		if(input.BufferByteCount > ID_MAX_MSG_LENGTH)
		{
			break;
		}

		if(file.Read(_messageData, input.BufferByteCount, 1) != 1)
		{
			break;
		}
		input.Buffer = _messageData;

		errorCode = udtCuParseMessage(context, &output, &continueParsing, &input);
		if(errorCode != udtErrorCode::None)
		{
			break;
		}

		if(continueParsing == 0)
		{
			break;
		}

		if(output.IsGameState)
		{
			++gsIndex;
			if(gsIndex >= 2)
			{
				break;
			}
		}
		else if(output.GameStateOrSnapshot.Snapshot != nullptr)
		{
			const s32 serverTimeMs = output.GameStateOrSnapshot.Snapshot->ServerTimeMs;
			if(serverTimeMs < _firstMatchStartTimeMs)
			{
				continue;
			}
			else if(serverTimeMs > _firstMatchEndTimeMs)
			{
				break;
			}
		}

		if(!(this->*messageHandler)(output))
		{
			break;
		}
	}
}

bool Demo::ProcessMessage_Mod(const udtCuMessageOutput&)
{
	udtCuConfigString cs;
	if(udtCuGetConfigString(_context, &cs, 0) != udtErrorCode::None)
	{
		return false;
	}

	char gameName[64];
	char temp[64];
	if(udtParseConfigStringValueAsString(gameName, sizeof(gameName), temp, sizeof(temp), "gamename", cs.ConfigString) != udtErrorCode::None)
	{
		return false;
	}

	const udtString gameNameString = udtString::NewConstRef(gameName);
	if(udtString::ContainsNoCase(gameNameString, "osp"))
	{
		_mod = udtMod::OSP;
	}
	else if(udtString::ContainsNoCase(gameNameString, "cpm"))
	{
		_mod = udtMod::CPMA;
	}
	
	return false;
}

bool Demo::ProcessMessage_StaticItems(const udtCuMessageOutput& message)
{
	if(message.IsGameState)
	{
		_staticItems.Clear();
		return true;
	}

	if(message.GameStateOrSnapshot.Snapshot == nullptr)
	{
		return true;
	}

	const udtCuSnapshotMessage& snapshot = *message.GameStateOrSnapshot.Snapshot;
	for(u32 i = 0; i < snapshot.EntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.Entities[i];
		if(es.eType != _protocolNumbers.EntityTypeItem)
		{
			continue;
		}

		s32 udtItemId;
		udtGetUDTMagicNumber(&udtItemId, udtMagicNumberType::Item, es.modelindex, _protocol, _mod);
		if(GetItemClassFromId(udtItemId) == ItemClass::Static)
		{
			RegisterStaticItem(es, udtItemId);
		}
	}

	return true;
}

bool Demo::ProcessMessage_FinalPass(const udtCuMessageOutput& message)
{
	if(message.IsGameState)
	{
		udtCuConfigString cs;
		udtCuGetConfigString(_context, &cs, 0);
		char mapName[256];
		char tempBuffer[256];
		udtParseConfigStringValueAsString(mapName, sizeof(mapName), tempBuffer, sizeof(tempBuffer), "mapname", cs.ConfigString);
		_mapName = udtString::NewClone(_stringAllocator, mapName);

		for(s32 p = 0; p < 64; ++p)
		{
			_players[p].Name = u32(-1);
			_players[p].Team = udtTeam::Spectators;
			ProcessPlayerConfigString(_protocolNumbers.CSIndexFirstPlayer + p, p);
		}
	}

	if(message.IsGameState ||
	   message.GameStateOrSnapshot.Snapshot == nullptr)
	{
		return true;
	}

	_tempPlayers.Clear();
	_tempDynamicItems.Clear();
	_tempBeams.Clear();
	_tempShaftImpacts.Clear();

	const udtCuSnapshotMessage& snapshot = *message.GameStateOrSnapshot.Snapshot;
	auto& snapshots = _snapshots[_writeIndex];

	for(u32 i = 0; i < snapshot.ChangedEntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.ChangedEntities[i];
		for(u32 j = 0; j < 3; ++j)
		{
			_min[j] = udt_min(_min[j], es.pos.trBase[j]);
			_max[j] = udt_max(_max[j], es.pos.trBase[j]);
		}
	}

	const s32 idCSIndexFirstPlayer = _protocolNumbers.CSIndexFirstPlayer;
	for(u32 i = 0; i < message.CommandCount; ++i)
	{
		const udtCuCommandMessage& cmd = message.Commands[i];
		if(cmd.IsConfigString &&
		   cmd.ConfigStringIndex >= idCSIndexFirstPlayer &&
		   cmd.ConfigStringIndex < idCSIndexFirstPlayer + 64)
		{
			ProcessPlayerConfigString(cmd.ConfigStringIndex, cmd.ConfigStringIndex - idCSIndexFirstPlayer);
		}
	}

	Snapshot& newSnap = *_snapshot;
	newSnap.ServerTimeMs = snapshot.ServerTimeMs;

	//
	// Static items.
	//

	newSnap.StaticItemCount = 0;
	for(u32 i = 0; i < snapshot.EntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.Entities[i];
		if(es.eType != _protocolNumbers.EntityTypeItem)
		{
			continue;
		}

		for(u32 j = 0, count = _staticItems.GetSize(); j < count; ++j)
		{
			s32 udtItemId;
			udtGetUDTMagicNumber(&udtItemId, udtMagicNumberType::Item, es.modelindex, _protocol, _mod);
			if(IsSame(es, _staticItems[j], udtItemId))
			{
				newSnap.StaticItems[newSnap.StaticItemCount++] = _staticItems[j];
				break;
			}
		}
	}

	//
	// Players.
	//

	for(u32 i = 0; i < snapshot.EntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.Entities[i];
		if(es.eType == _protocolNumbers.EntityTypeEvent &&
		   IsBitSet(&snapshot.EntityFlags[i], udtEntityStateFlag::NewEvent))
		{
			if(es.event == _protocolNumbers.EntityEventBulletHitFlesh ||
			   es.event == _protocolNumbers.EntityEventBulletHitWall ||
			   es.event == _protocolNumbers.EntityEventMissileHit ||
			   es.event == _protocolNumbers.EntityEventMissileMiss ||
			   es.event == _protocolNumbers.EntityEventMissileMissMetal)
			{
				if(es.weapon == _protocolNumbers.WeaponShaft)
				{
					Impact impact;
					Float3::Copy(impact.Position, es.pos.trBase);
					_tempShaftImpacts.Add(impact);
				}
			}
		}
	}
	for(u32 i = 0; i < snapshot.EntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.Entities[i];
		if(es.eType == _protocolNumbers.EntityTypePlayer)
		{
			ProcessPlayer(es, snapshot.ServerTimeMs, false);
		}
	}
	idLargestEntityState es;
	udtPlayerStateToEntityState(&es, snapshot.PlayerState, 0, snapshot.ServerTimeMs, _protocol);
	if(es.eType == _protocolNumbers.EntityTypePlayer)
	{
		// @NOTE: The resulting entity can be of type "Invisible".
		ProcessPlayer(es, snapshot.ServerTimeMs, true);
	}
	const u32 playerCount = _tempPlayers.GetSize();
	assert(playerCount <= 64);
	newSnap.PlayerCount = playerCount;
	memcpy(newSnap.Players, _tempPlayers.GetStartAddress(), playerCount * sizeof(Player));

	//
	// Dynamic items.
	//

	DynamicItem dynItem;
	for(u32 i = 0; i < snapshot.EntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.Entities[i];
		if(es.eType == _protocolNumbers.EntityTypeItem)
		{
			for(u32 j = 0; j < (u32)RealDynamicItemType::Count; ++j)
			{
				if(es.modelindex == _protocolNumbers.DynamicItemIds[j])
				{
					dynItem.Id = (u8)j;
					dynItem.IdEntityNumber = (u16)es.number;
					dynItem.Angle = 0.0f;
					Float3::Copy(dynItem.Position, es.pos.trBase);
					_tempDynamicItems.Add(dynItem);
					break;
				}
			}
		}
		else if(es.eType == _protocolNumbers.EntityTypeMissile)
		{
			if(es.weapon == _protocolNumbers.WeaponRocket)
			{
				dynItem.Id = DynamicItemType::ProjectileRocket;
				dynItem.IdEntityNumber = (u16)es.number;
				dynItem.Angle = ComputeProjectileAngle(es);
				ComputeTrajectoryPosition(dynItem.Position, es.pos, snapshot.ServerTimeMs);
				_tempDynamicItems.Add(dynItem);
			}
			else if(es.weapon == _protocolNumbers.WeaponGrenade)
			{
				dynItem.Id = DynamicItemType::ProjectileGrenade;
				dynItem.IdEntityNumber = (u16)es.number;
				dynItem.Angle = ComputeProjectileAngle(es);
				ComputeTrajectoryPosition(dynItem.Position, es.pos, snapshot.ServerTimeMs);
				_tempDynamicItems.Add(dynItem);
			}
			else if(es.weapon == _protocolNumbers.WeaponPlasma)
			{
				dynItem.Id = DynamicItemType::ProjectilePlasma;
				dynItem.IdEntityNumber = (u16)es.number;
				dynItem.Angle = ComputeProjectileAngle(es);
				ComputeTrajectoryPosition(dynItem.Position, es.pos, snapshot.ServerTimeMs);
				_tempDynamicItems.Add(dynItem);
			}
		}
		else if(es.eType == _protocolNumbers.EntityTypeGeneral)
		{
			if(es.weapon == _protocolNumbers.WeaponRocket || es.weapon == _protocolNumbers.WeaponGrenade)
			{
				Impact explosion;
				Float3::Copy(explosion.Position, es.pos.trBase);
				explosion.SnapshotIndex = snapshots.GetSize();
				_explosions.Add(explosion);
			}
			else if(es.weapon == _protocolNumbers.WeaponPlasma)
			{
				dynItem.Id = DynamicItemType::ImpactPlasma;
				dynItem.IdEntityNumber = (u16)es.number;
				dynItem.Angle = 0.0f;
				Float3::Copy(dynItem.Position, es.pos.trBase);
				_tempDynamicItems.Add(dynItem);
			}
		}
		else if(es.eType == _protocolNumbers.EntityTypeEvent &&
				IsBitSet(&snapshot.EntityFlags[i], udtEntityStateFlag::NewEvent))
		{
			if(es.event == _protocolNumbers.EntityEventBulletHitFlesh ||
			   es.event == _protocolNumbers.EntityEventBulletHitWall)
			{
				Impact impact;
				Float3::Copy(impact.Position, es.pos.trBase);
				impact.SnapshotIndex = snapshots.GetSize();
				_bulletImpacts.Add(impact);
			}
			else if(es.event == _protocolNumbers.EntityEventMissileHit ||
					es.event == _protocolNumbers.EntityEventMissileMiss ||
					es.event == _protocolNumbers.EntityEventMissileMissMetal)
			{
				dynItem.Id = (es.weapon == _protocolNumbers.WeaponPlasma) ?
					(u8)DynamicItemType::ImpactPlasma :
					(u8)DynamicItemType::ImpactGeneric;
				dynItem.IdEntityNumber = (u16)es.number;
				dynItem.Angle = 0.0f;
				Float3::Copy(dynItem.Position, es.pos.trBase);
				_tempDynamicItems.Add(dynItem);
			}
		}
	}
	for(s32 i = (s32)_bulletImpacts.GetSize() - 1; i >= 0; --i)
	{
		const u32 offset = (snapshots.GetSize() - _bulletImpacts[i].SnapshotIndex) / 4;
		if(offset >= (u32)Sprite::BulletImpactFrames)
		{
			_bulletImpacts.RemoveUnordered(i);
		}
		else
		{
			dynItem.Id = DynamicItemType::ImpactBullet;
			dynItem.IdEntityNumber = u16(-1);
			dynItem.Angle = 0.0f;
			Float3::Copy(dynItem.Position, _bulletImpacts[i].Position);
			dynItem.SpriteOffset = (u8)offset;
			_tempDynamicItems.Add(dynItem);
		}
	}
	for(s32 i = (s32)_explosions.GetSize() - 1; i >= 0; --i)
	{
		const u32 offset = snapshots.GetSize() - _explosions[i].SnapshotIndex;
		if(offset >= (u32)Sprite::ExplosionFrames)
		{
			_explosions.RemoveUnordered(i);
		}
		else
		{
			dynItem.Id = DynamicItemType::Explosion;
			dynItem.IdEntityNumber = u16(-1);
			dynItem.Angle = 0.0f;
			Float3::Copy(dynItem.Position, _explosions[i].Position);
			dynItem.SpriteOffset = (u8)offset;
			_tempDynamicItems.Add(dynItem);
		}
	}
	const u32 dynItemCount = _tempDynamicItems.GetSize();
	newSnap.DynamicItemCount = dynItemCount;
	memcpy(newSnap.DynamicItems, _tempDynamicItems.GetStartAddress(), dynItemCount * (u32)sizeof(DynamicItem));

	//
	// Beams.
	//

	RailBeamEx railBeam;
	for(u32 i = 0; i < snapshot.ChangedEntityCount; ++i)
	{
		const idEntityStateBase& es = *snapshot.ChangedEntities[i];
		if(es.eType == _protocolNumbers.EntityTypeEvent &&
		   es.event == _protocolNumbers.EntityEventRailTrail &&
		   es.clientNum >= 0 &&
		   es.clientNum < 64)
		{
			railBeam.ServerTimeMs = snapshot.ServerTimeMs;
			railBeam.Base.Alpha = 1.0f;
			railBeam.Base.Team = (u8)_players[es.clientNum].Team;
			Float3::Copy(railBeam.Base.StartPosition, es.origin2);
			Float3::Copy(railBeam.Base.EndPosition, es.pos.trBase);
			_beams.Add(railBeam);
		}
	}
	for(s32 i = (s32)_beams.GetSize() - 1; i >= 0; --i)
	{
		if(snapshot.ServerTimeMs - _beams[i].ServerTimeMs > RAIL_BEAM_DURATION_MS)
		{
			_beams.RemoveUnordered(i);
		}
	}
	for(u32 i = 0; i < _beams.GetSize(); ++i)
	{
		const f32 t = 1.0f - (f32)(snapshot.ServerTimeMs - _beams[i].ServerTimeMs) / (f32)RAIL_BEAM_DURATION_MS;
		_beams[i].Base.Alpha = udt_clamp(t*t*t, 0.0f, 1.0f);
		_tempBeams.Add(_beams[i].Base);
	}
	const u32 railBeamCount = _tempBeams.GetSize();
	newSnap.RailBeamCount = railBeamCount;
	memcpy(newSnap.RailBeams, _tempBeams.GetStartAddress(), railBeamCount * (u32)sizeof(RailBeam));

	//
	// Core.
	//

	const s32 followedPlayerIndex = snapshot.PlayerState->clientNum;
	const s32 weapon = snapshot.PlayerState->weapon;
	newSnap.Core.FollowedHealth = (s16)snapshot.PlayerState->stats[_protocolNumbers.PlayerStatsHealth];
	newSnap.Core.FollowedArmor = (s16)snapshot.PlayerState->stats[_protocolNumbers.PlayerStatsArmor];
	newSnap.Core.FollowedAmmo = (weapon >= 0 && weapon < ID_MAX_PS_WEAPONS) ? (s16)snapshot.PlayerState->ammo[snapshot.PlayerState->weapon] : s16(0);
	newSnap.Core.FollowedName = u32(-1);
	if(followedPlayerIndex >= 0 && followedPlayerIndex < 64)
	{
		newSnap.Core.FollowedName = _players[followedPlayerIndex].Name;
	}

	WriteSnapshot(newSnap);

	return true;
}

bool Demo::ProcessPlayer(const idEntityStateBase& player, s32 serverTimeMs, bool followed)
{
	if(player.clientNum < 0 ||
	   player.clientNum >= 64 ||
	   IsBitSet(&player.eFlags, _protocolNumbers.EntityFlagNoDraw) ||
	   _players[player.clientNum].Team == udtTeam::Spectators ||
	   (IsBitSet(&player.eFlags, _protocolNumbers.EntityFlagDead) && player.pos.trType == ID_TR_GRAVITY))
	{
		return false;
	}
	
	s32 udtWeaponId;
	udtGetUDTMagicNumber(&udtWeaponId, udtMagicNumberType::Weapon, player.weapon, _protocol, _mod);

	Player p;
	ComputeTrajectoryPosition(p.Position, player.pos, serverTimeMs);
	p.Angle = ComputePlayerAngle(player, serverTimeMs);
	p.WeaponId = (u8)udtWeaponId;
	p.IdClientNumber = (u8)player.clientNum;
	p.Team = (u8)_players[player.clientNum].Team;
	p.Name = _players[player.clientNum].Name;
	p.Flags = 0;
	if(followed)
	{
		SetBit(&p.Flags, PlayerFlags::Followed);
	}
	if(IsBitSet(&player.eFlags, _protocolNumbers.EntityFlagDead))
	{
		SetBit(&p.Flags, PlayerFlags::Dead);
	}
	if(IsBitSet(&player.eFlags, _protocolNumbers.EntityFlagFiring))
	{
		SetBit(&p.Flags, PlayerFlags::Firing);
	}
	if(IsBitSet(&player.eFlags, _protocolNumbers.EntityFlagTelePortBit))
	{
		SetBit(&p.Flags, PlayerFlags::TelePortBit);
	}
	if(udtWeaponId == (s32)udtWeapon::LightningGun &&
	   IsBitSet(&player.eFlags, _protocolNumbers.EntityFlagFiring))
	{
		ComputeLGEndPoint(p, player.pos.trBase, player.apos.trBase);
	}
	_tempPlayers.Add(p);

	return true;
}

void Demo::ProcessPlayerConfigString(u32 csIndex, u32 playerIndex)
{
	udtCuConfigString cs;
	if(udtCuGetConfigString(_context, &cs, csIndex) != udtErrorCode::None ||
	   cs.ConfigString == nullptr ||
	   cs.ConfigStringLength == 0)
	{
		return;
	}

	s32 idTeam;
	s32 udtTeam;
	char tempBuffer[256];
	if(udtParseConfigStringValueAsInteger(&idTeam, tempBuffer, sizeof(tempBuffer), "t", cs.ConfigString) == udtErrorCode::None &&
	   udtGetUDTMagicNumber(&udtTeam, udtMagicNumberType::Team, idTeam, _protocol, _mod) == udtErrorCode::None)
	{
		_players[playerIndex].Team = udtTeam;
	}

	char nameBuffer[64];
	if(udtParseConfigStringValueAsString(nameBuffer, sizeof(nameBuffer), tempBuffer, sizeof(tempBuffer), "n", cs.ConfigString) == udtErrorCode::None)
	{
		_players[playerIndex].Name = udtString::NewCleanClone(_stringAllocator, (udtProtocol::Id)_protocol, nameBuffer).GetOffset();
	}
}

void Demo::RegisterStaticItem(const idEntityStateBase& item, s32 udtItemId)
{
	for(u32 i = 0, count = _staticItems.GetSize(); i < count; ++i)
	{
		if(IsSame(item, _staticItems[i], udtItemId))
		{
			return;
		}
	}

	StaticItem newItem;
	newItem.Id = udtItemId;
	newItem.Position[0] = item.pos.trBase[0];
	newItem.Position[1] = item.pos.trBase[1];
	newItem.Position[2] = item.pos.trBase[2];
	_staticItems.Add(newItem);
}

bool Demo::IsSame(const idEntityStateBase& es, const StaticItem& item, s32 udtItemId)
{
	return 
		item.Id == udtItemId &&
		item.Position[0] == es.pos.trBase[0] &&
		item.Position[1] == es.pos.trBase[1] &&
		item.Position[2] == es.pos.trBase[2];
}

void Demo::FixStaticItems()
{
	const auto& snapshots = _snapshots[_readIndex];
	auto& snapDataAllocator = _snapshotAllocators[_readIndex];
	const u32 itemCount = _staticItems.GetSize();
	const u32 snapshotCount = snapshots.GetSize();
	for(u32 i = 0; i < itemCount; ++i)
	{
		const s32 itemId = _staticItems[i].Id;
		const u32 spawnTimeMs = GetItemSpawnTimeMs(itemId);
		if(spawnTimeMs == 0)
		{
			continue;
		}

		u32 lastSnapUp = 0;
		s32 lastTimeUp = UDT_S32_MIN;
		for(u32 s = 0; s < snapshotCount; ++s)
		{
			const u8* const snapData = snapDataAllocator.GetAddressAt(snapshots[s].Offset);
			if(IsBitSet(snapData + 4, i))
			{
				lastSnapUp = s;
				lastTimeUp = *(s32*)snapData;
				continue;
			}

			if(lastTimeUp == UDT_S32_MIN)
			{
				continue;
			}

			bool fix = false;
			u32 onePastLastSnapToFix = 0;
			for(u32 s2 = s + 1; s2 < snapshotCount; ++s2)
			{
				const u8* const snapData2 = snapDataAllocator.GetAddressAt(snapshots[s2].Offset);
				const s32 time = *(s32*)snapData2;
				if(time - lastTimeUp >= (s32)spawnTimeMs)
				{
					break;
				}

				if(IsBitSet(snapData2 + 4, i))
				{
					fix = true;
					onePastLastSnapToFix = s2;
					break;
				}
			}

			if(!fix)
			{
				continue;
			}

			for(u32 s2 = lastSnapUp + 1; s2 < onePastLastSnapToFix; ++s2)
			{
				SetBit(snapDataAllocator.GetAddressAt(snapshots[s2].Offset + 4), i);
			}
		}
	}
}

void Demo::FixDynamicItemsAndPlayers()
{
	_readIndex = 0;
	_writeIndex = 1;

	Snapshot snaps[2];
	Snapshot snap2;
	u32 snapIdx = 1;
	GetSnapshotData(snaps[0], 0);
	WriteSnapshot(snaps[0]);

	const u32 snapshotCount = _snapshots[_readIndex].GetSize();
	for(u32 s = 1; s < snapshotCount - 1; ++s)
	{
		GetSnapshotData(snaps[snapIdx], s);
		auto& currSnap = snaps[snapIdx];
		const auto& prevSnap = snaps[snapIdx ^ 1];
		snapIdx ^= 1;
		assert(currSnap.DynamicItemCount <= MAX_STATIC_ITEMS);
		assert(currSnap.StaticItemCount <= MAX_STATIC_ITEMS);
		assert(currSnap.RailBeamCount <= MAX_RAIL_BEAMS);
		assert(currSnap.PlayerCount <= 64);

		for(u32 i = 0; i < prevSnap.DynamicItemCount; ++i)
		{
			const DynamicItem& item = prevSnap.DynamicItems[i];
			const u32 spawnTimeMs = GetDynamicItemSpawnTimeMs(item.Id);
			if(spawnTimeMs == 0)
			{
				continue;
			}
			
			bool fixed = false;
			for(u32 s2 = s; s2 < snapshotCount && !fixed; ++s2)
			{
				GetSnapshotData(snap2, s2);
				if(snap2.ServerTimeMs - prevSnap.ServerTimeMs >= (s32)spawnTimeMs)
				{
					break;
				}

				for(u32 i2 = 0; i2 < snap2.DynamicItemCount; ++i2)
				{
					const DynamicItem& item2 = snap2.DynamicItems[i2];
					if(item2.Id == item.Id &&
					   item2.IdEntityNumber == item.IdEntityNumber)
					{
						if(s2 > s)
						{
							DynamicItem newItem = item2;
							const f32 t = (f32)(currSnap.ServerTimeMs - prevSnap.ServerTimeMs) / (f32)(snap2.ServerTimeMs - prevSnap.ServerTimeMs);
							Float3::Lerp(newItem.Position, item.Position, item2.Position, t);
							currSnap.DynamicItems[currSnap.DynamicItemCount++] = newItem;

						}
						fixed = true;
						break;
					}
				}
			}
		}

		FixPlayers(prevSnap, currSnap, snap2, s, snapshotCount, true);
		FixPlayers(prevSnap, currSnap, snap2, s, snapshotCount, false);

		WriteSnapshot(currSnap);
	}

	_readIndex = 1;
	_writeIndex = 0;
}

void Demo::FixPlayers(const Snapshot& prevSnap, Snapshot& currSnap, Snapshot& snap2, u32 s, u32 snapshotCount, bool alive)
{
	for(u32 p = 0; p < prevSnap.PlayerCount; ++p)
	{
		const Player& player = prevSnap.Players[p];
		if(IsBitSet(&player.Flags, PlayerFlags::Dead) != alive)
		{
			continue;
		}

		const bool tpBit = IsBitSet(&player.Flags, PlayerFlags::TelePortBit);

		bool fixed = false;
		for(u32 s2 = s; s2 < snapshotCount && !fixed; ++s2)
		{
			GetSnapshotData(snap2, s2);
			if(snap2.ServerTimeMs - prevSnap.ServerTimeMs >= MAX_FIXABLE_PLAYER_BLINK_TIME_MS)
			{
				break;
			}

			for(u32 p2 = 0; p2 < snap2.PlayerCount; ++p2)
			{
				const Player& player2 = snap2.Players[p2];
				if(player2.IdClientNumber == player.IdClientNumber)
				{
					if(s2 > s && 
					   IsBitSet(&player2.Flags, PlayerFlags::Dead) == alive &&
					   IsBitSet(&player2.Flags, PlayerFlags::TelePortBit) == tpBit)
					{
						Player newPlayer = player2;
						const f32 t = (f32)(currSnap.ServerTimeMs - prevSnap.ServerTimeMs) / (f32)(snap2.ServerTimeMs - prevSnap.ServerTimeMs);
						Float3::Lerp(newPlayer.Position, player.Position, player2.Position, t);
						ClearBit(&newPlayer.Flags, PlayerFlags::Firing);
						currSnap.Players[currSnap.PlayerCount++] = newPlayer;

					}
					fixed = true;
					break;
				}
			}
		}
	}
}

void Demo::FixLGEndPoints()
{
	const u32 staticItemCount = _staticItems.GetSize();
	const u32 staticItemByteCount = (staticItemCount + 7) / 8;
	const auto& snapshots = _snapshots[_readIndex];
	const u32 snapshotCount = snapshots.GetSize();
	for(u32 s = 1; s < snapshotCount - 1; ++s)
	{
		uptr offset = snapshots[s].Offset + staticItemByteCount + 4;
		u32 playerCount;
		Read(offset, playerCount);
		Player* const players = (Player*)_snapshotAllocators[_readIndex].GetAddressAt(offset);
		for(u32 p = 0; p < playerCount; ++p)
		{
			Player& player = players[p];
			const Player* prevPlayer = nullptr;
			const Player* nextPlayer = nullptr;
			if(IsBitSet(&player.Flags, PlayerFlags::ShortLGBeam) ||
			   !FindPlayer(prevPlayer, s - 1, player.IdClientNumber) ||
			   !FindPlayer(nextPlayer, s + 1, player.IdClientNumber))
			{
				continue;
			}

			// We keep the current view direction but use the beam length of the next snapshot.
			f32 normDir[3];
			const f32 length = Float3::Dist(nextPlayer->Position, nextPlayer->LGEndPoint);
			Float3::Direction(normDir, player.Position, player.LGEndPoint);
			Float3::Mad(player.LGEndPoint, player.Position, normDir, length);
			SetBit(&player.Flags, PlayerFlags::ShortLGBeam);
		}
	}
}

bool Demo::FindPlayer(const Player*& playerOut, u32 snapshotIndex, u8 idClientNumber)
{
	const u32 staticItemByteCount = (_staticItems.GetSize() + 7) / 8;
	uptr offset = _snapshots[_readIndex][snapshotIndex].Offset + staticItemByteCount + 4;
	u32 playerCount;
	Read(offset, playerCount);
	const Player* const players = (const Player*)_snapshotAllocators[_readIndex].GetAddressAt(offset);
	for(u32 p = 0; p < playerCount; ++p)
	{
		const Player& player = players[p];
		if(idClientNumber == player.IdClientNumber &&
		   IsBitSet(&player.Flags, PlayerFlags::ShortLGBeam))
		{
			playerOut = &player;
			return true;
		}
	}

	return false;
}

void Demo::ComputeLGEndPoint(Player& player, const f32* start, const f32* angles)
{
	f32 viewVector[3];
	Float3::EulerAnglesToAxisVector(viewVector, angles);

	s32 bestImpact = -1;
	f32 bestDotProduct = 0.85f; // The best angle has to match pretty closely.
	for(u32 i = 0; i < _tempShaftImpacts.GetSize(); ++i)
	{
		f32 beamVector[3];
		Float3::Direction(beamVector, start, _tempShaftImpacts[i].Position);
		const f32 dot = Float3::Dot(viewVector, beamVector);
		if(dot > bestDotProduct)
		{
			bestDotProduct = dot;
			bestImpact = (s32)i;
		}
	}

	if(bestImpact >= 0)
	{
		Float3::Copy(player.LGEndPoint, _tempShaftImpacts[bestImpact].Position);
		SetBit(&player.Flags, PlayerFlags::ShortLGBeam);
	}
	else
	{
		Float3::Mad(player.LGEndPoint, start, viewVector, (f32)LG_BEAM_LENGTH);
	}
}

bool Demo::AnalyzeDemo(const char* filePath)
{
	_firstMatchStartTimeMs = UDT_S32_MIN;
	_firstMatchEndTimeMs = UDT_S32_MAX;
	_mod = udtMod::None;
	_gameType = udtGameType::Count;

	const u32 plugInIds[2] = { udtParserPlugIn::Stats, udtParserPlugIn::Scores };
	udtParseArg arg;
	memset(&arg, 0, sizeof(arg));
	arg.PlugIns = plugInIds;
	arg.PlugInCount = 2;

	s32 errorCode = (s32)udtErrorCode::Unprocessed;
	udtMultiParseArg extraArg;
	memset(&extraArg, 0, sizeof(extraArg));
	extraArg.MaxThreadCount = 1;
	extraArg.OutputErrorCodes = &errorCode;
	extraArg.FilePaths = &filePath;
	extraArg.FileCount = 1;

	udtParserContextGroup* contextGroup = nullptr;
	udtParserContext* context = nullptr;
	if(udtParseDemoFiles(&contextGroup, &arg, &extraArg) != udtErrorCode::None ||
	   errorCode != udtErrorCode::None ||
	   udtGetContextFromGroup(contextGroup, 0, &context) != udtErrorCode::None)
	{
		udtDestroyContextGroup(contextGroup);
		return false;
	}

	udtParseDataScoreBuffers scoreBuffers;
	if(udtGetContextPlugInBuffers(context, udtParserPlugIn::Scores, &scoreBuffers) == udtErrorCode::None &&
	   scoreBuffers.ScoreCount > 0)
	{
		const u32 offset = (u32)_stringAllocator.GetCurrentByteCount();
		u8* const newCopy = _stringAllocator.AllocateAndGetAddress(scoreBuffers.StringBufferSize);
		memcpy(newCopy, scoreBuffers.StringBuffer, (size_t)scoreBuffers.StringBufferSize);

		for(u32 i = 0; i < scoreBuffers.ScoreCount; ++i)
		{
			const udtParseDataScore& s = scoreBuffers.Scores[i];
			if(s.GameStateIndex >= 1)
			{
				break;
			}

			Score score;
			score.ServerTimeMs = s.ServerTimeMs;
			score.Base.IsScoreTeamBased = (s.Flags & udtParseDataScoreMask::TeamBased) != 0 ? 1 : 0;
			score.Base.Score1Id = (u8)s.Id1;
			score.Base.Score2Id = (u8)s.Id2;
			score.Base.Score1 = (s16)s.Score1;
			score.Base.Score2 = (s16)s.Score2;
			score.Base.Score1Name = s.Name1 + offset;
			score.Base.Score2Name = s.Name2 + offset;
			_scores.Add(score);
		}
	}

	bool success = false;
	udtParseDataStatsBuffers statsBuffers;
	if(udtGetContextPlugInBuffers(context, udtParserPlugIn::Stats, &statsBuffers) == udtErrorCode::None &&
	   statsBuffers.MatchCount > 0)
	{
		const udtParseDataStats& stats = statsBuffers.MatchStats[0];
		if(stats.GameStateIndex == 0)
		{
			success = true;
			_firstMatchStartTimeMs = stats.StartTimeMs + 50;
			_firstMatchEndTimeMs = stats.EndTimeMs - 50;
			_mod = stats.Mod;
			_gameType = stats.GameType;
			// @TODO: custom red/blue team names?
		}
	}

	udtDestroyContextGroup(contextGroup);

	return success;
}
