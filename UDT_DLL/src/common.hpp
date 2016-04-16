#pragma once


#include "uberdemotools.h"
#include "macros.hpp"

#include <string.h>
#include <stdio.h>


// safe strncpy that ensures a trailing zero
extern void Q_strncpyz(char* dest, const char* src, s32 destsize);


// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

// the game guarantees that no string from the network will ever exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			256		// max length of a filesystem pathname

#define	MAX_NAME_LENGTH		32		// max length of a client name

//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility

//=============================================

// TTimo
// centralized and cleaned, that's the max string you can send to a Com_Printf / Com_DPrintf (above gets truncated)
#define	MAXPRINTMSG	4096

// TTimo
// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
#if defined(_WIN32) || defined(_MSC_VER) // myT
#define Q_vsnprintf _vsnprintf
#else
// TODO: do we need Mac define?
#define Q_vsnprintf vsnprintf
#endif

#define Com_Memset memset
#define Com_Memcpy memcpy

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

//
// per-level limits
//
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS		10		// don't need to send any more
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

// Entity numbers are communicated with GENTITY_BITS bits, so any reserved
// values that are going to be communicated over the net need to
// also be in this range.
#define	ENTITYNUM_NONE       (MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD	     (MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL (MAX_GENTITIES-2)


#define	MAX_MODELS			256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS			256		// so they cannot be blindly increased

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

#define	MAX_CONFIGSTRINGS	1024

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define	CS_SERVERINFO			0		// an info string with all the serverinfo cvars
#define	CS_SYSTEMINFO			1		// an info string for server system to client system configuration (timescale, etc)
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11
#define CS_TEAMVOTE_TIME_68		12		// q3 only
#define CS_TEAMVOTE_STRING_68	14		// q3 only
#define	CS_TEAMVOTE_YES_68		16		// q3 only
#define	CS_TEAMVOTE_NO_68		18		// q3 only
#define	CS_GAME_VERSION_68		20
#define	CS_LEVEL_START_TIME_68	21		// so the timer only shows the current level
#define	CS_INTERMISSION_68		22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS_68		23		// string indicating flag status in CTF
#define CS_SHADERSTATE_68		24
#define CS_BOTINFO_68			25		// q3 only
#define	CS_ITEMS_68				27		// string of 0's and 1's that tell which items are present
#define	CS_MODELS_68			32
#define CS_WARMUP_END           13
#define CS_PAUSE_START_73p		669		// if this is non-zero, the game is paused
#define CS_PAUSE_COUNTDOWN_73p	670		// 0 = pause, !0 = timeout
#define CS_CA_ROUND_INFO        661
#define CS_CA_ROUND_START       662
#define	CS_PLAYERS_68           544
#define CS_RED_CLAN_PLAYERS     663
#define CS_BLUE_CLAN_PLAYERS    664
#define CS_FLAG_STATUS_73       658
#define CS_FIRST_PLACE          659
#define CS_SECOND_PLACE         660
#define CS_AD_WAIT              681
#define	CS_SOUNDS_68			(CS_MODELS_68 + MAX_MODELS)
#define CS_LOCATIONS_68			(CS_PLAYERS_68 + ID_MAX_CLIENTS)
#define CS_PARTICLES_68			(CS_LOCATIONS_68 + MAX_LOCATIONS)
#define CS_PAST_LAST_INDEX_68	(CS_PARTICLES_68 + MAX_LOCATIONS)

#define CS_GAME_VERSION_73p               12
#define CS_LEVEL_START_TIME_73p           13
#define CS_INTERMISSION_73p               14
#define CS_ITEMS_73p                      15
#define CS_MODELS_73p                     17
#define CS_SOUNDS_73p                     274
#define CS_PLAYERS_73p                    529
#define CS_LOCATIONS_73p                  593
#define CS_PARTICLES_73p                  657
#define CS_FLAGSTATUS_73p                 658
#define CS_SHADERSTATE_73p                665
#define CS_TIMEOUT_BEGIN_TIME_73p         669
#define CS_TIMEOUT_END_TIME_73p           670
#define CS_RED_TEAM_TIMEOUTS_LEFT_73p     671
#define CS_BLUE_TEAM_TIMEOUTS_LEFT_73p    672
#define CS_MAP_CREATOR_73p                679
#define CS_ORIGINAL_MAP_CREATOR_73p       680
#define CS_PMOVE_SETTINGS_73p             682
#define CS_ARMOR_TIERED_73p               683
#define CS_WEAPON_SETTINGS_73p            684
#define CS_CUSTOM_PLAYER_MODELS_73p       685
#define CS_FIRST_PLACE_CLIENT_NUM_73p     686
#define CS_SECOND_PLACE_CLIENT_NUM_73p    687
#define CS_FIRST_PLACE_SCORE_73p          688
#define CS_SECOND_PLACE_SCORE_73p         689
#define CS_MOST_DAMAGE_DEALT_73p          691
#define CS_MOST_ACCURATE_73p              692
#define CS_RED_TEAM_CLAN_NAME_73p         693
#define CS_BLUE_TEAM_CLAN_NAME_73p        694
#define CS_RED_TEAM_CLAN_TAG_73p          695
#define CS_BLUE_TEAM_CLAN_TAG_73p         696
#define CS_BEST_ITEM_CONTROL_73p          697
#define CS_MVP_OFFENSE_73p                699
#define CS_MVP_DEFENSE_73p                700
#define CS_MVP_73p                        701
#define CS_DOMINATION_RED_POINTS_73p      702
#define CS_DOMINATION_BLUE_POINTS_73p     703
#define CS_ROUND_WINNERS_73p              705
#define CS_CUSTOM_SERVER_SETTINGS_73p     706
#define CS_MAP_VOTE_INFO_73p              707
#define CS_MAP_VOTE_COUNT_73p             708
#define CS_DISABLE_MAP_VOTE_73p           709
#define CS_READY_UP_TIME_73p              710
#define CS_NUMBER_OF_RACE_CHECKPOINTS_73p 713

#define CS_LEVEL_START_TIME_3   13
#define CS_INTERMISSION_3       14
#define CS_FLAGSTATUS_3         15
#define CS_LOCATIONS_3          672

enum idConfigString91
{
	CS_NEXTMAP_91 = 666,
	CS_PRACTICE_91,
	CS_FREECAM_91,

	CS_PAUSE_START_TIME_91,    // if this is non-zero, the game is paused
	CS_PAUSE_END_TIME_91,      // 0 = pause, !0 = timeout

	CS_TIMEOUTS_RED_91,        // TOs REMAINING
	CS_TIMEOUTS_BLUE_91,

	CS_MODEL_OVERRIDE_91,

	CS_PLAYER_CYLINDERS_91,
	CS_DEBUGFLAGS_91,
	CS_ENABLEBREATH_91,
	CS_DMGTHROUGHDEPTH_91,
	CS_AUTHOR_91,              // from the map worldspawn's author field
	CS_AUTHOR2_91,
	CS_ADVERT_DELAY_91,
	CS_PMOVEINFO_91,
	CS_ARMORINFO_91,
	CS_WEAPONINFO_91,
	CS_PLAYERINFO_91,
	CS_SCORE1STPLAYER_91,		// Score of the duel player on the left
	CS_SCORE2NDPLAYER_91,		// Score of the duel player on the right
	CS_CLIENTNUM1STPLAYER_91,	// ClientNum of the duel player on the left
	CS_CLIENTNUM2NDPLAYER_91,	// ClientNum of the duel player on the right
	CS_NAME1STPLAYER_91,		// Name of the duel player on the left
	CS_NAME2NDPLAYER_91,		// Name of the duel player on the right
	CS_ATMOSEFFECT_91, // unused, was per-map rain/snow effects
	CS_MOST_DAMAGEDEALT_PLYR_91,
	CS_MOST_ACCURATE_PLYR_91,
	CS_REDTEAMBASE_91,
	CS_BLUETEAMBASE_91,
	CS_BEST_ITEMCONTROL_PLYR_91,
	CS_MOST_VALUABLE_OFFENSIVE_PLYR_91,
	CS_MOST_VALUABLE_DEFENSIVE_PLYR_91,
	CS_MOST_VALUABLE_PLYR_91,
	CS_GENERIC_COUNT_RED_91,
	CS_GENERIC_COUNT_BLUE_91,
	CS_AD_SCORES_91,
	CS_ROUND_WINNER_91,
	CS_CUSTOM_SETTINGS_91,
	CS_ROTATIONMAPS_91,
	CS_ROTATIONVOTES_91,
	CS_DISABLE_VOTE_UI_91,
	CS_ALLREADY_TIME_91,
	CS_INFECTED_SURVIVOR_MINSPEED_91,
	CS_RACE_POINTS_91,
	CS_DISABLE_LOADOUT_91,
	CS_MATCH_GUID_91,
	CS_STARTING_WEAPONS_91,
	CS_STEAM_ID_91,
	CS_STEAM_WORKSHOP_IDS_91,
	CS_MAX_91
};

// Doesn't seem to exist in dm_73. Don't know about dm_90.
#define CS_SCORES1PLAYER_91     659 // 1st place player's name
#define CS_SCORES2PLAYER_91     660 // 2nd place player's name

// CPMA
#define CS_CPMA_GAME_INFO       672
#define CS_CPMA_ROUND_INFO      710

// OSP
#define CS_OSP_GAMEPLAY         806

/*
==============================================================

NET

==============================================================
*/

#define	PACKET_BACKUP	32	// number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		// max number of usercmd_t in a packet

#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit

// server to client
// the svc_strings[] array in cl_parse.c should mirror this
enum svc_ops_e
{
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,			// [short] [string] only in gamestate messages
	svc_baseline,				// only in gamestate messages
	svc_serverCommand,			// [string] to be executed by client game module
	svc_download,				// [short] size [size bytes]
	svc_snapshot,
	svc_EOF,
	// svc_extension follows a svc_EOF, followed by another svc_* ...
	// this keeps legacy clients compatible.
	svc_extension,
	svc_voip,     // not wrapped in USE_VOIP, so this value is reserved.
};


// snapshots are a view of the server at a given time
struct idClientSnapshotBase
{
	u8   areamask[MAX_MAP_AREA_BYTES]; // portalarea visibility bits
	s32  snapFlags;                    // rate delayed and dropped commands
	s32  serverTime;                   // server time the message is valid for (in msec)
	s32  messageNum;                   // copied from netchan->incoming_sequence
	s32  deltaNum;                     // messageNum the delta is from
	//s32  cmdNum;                     // the next cmdNum the server is expecting
	s32  numEntities;                  // all of the entities that need to be presented
	s32  parseEntitiesNum;             // at the time of this snapshot
	s32  serverCommandNum;             // execute all commands up to this before making the snapshot current
	bool valid;                        // cleared if delta parsing was invalid
};

struct idClientSnapshot3 : idClientSnapshotBase
{
	idPlayerState3 ps; // complete information about the current player at this time
};

struct idClientSnapshot48 : idClientSnapshotBase
{
	idPlayerState48 ps; // complete information about the current player at this time
};

struct idClientSnapshot66 : idClientSnapshotBase
{
	idPlayerState66 ps; // complete information about the current player at this time
};

struct idClientSnapshot67 : idClientSnapshotBase
{
	idPlayerState67 ps; // complete information about the current player at this time
};

struct idClientSnapshot68 : idClientSnapshotBase
{
	idPlayerState68 ps; // complete information about the current player at this time
};

struct idClientSnapshot73 : idClientSnapshotBase
{
	idPlayerState73 ps; // complete information about the current player at this time
};

struct idClientSnapshot90 : idClientSnapshotBase
{
	idPlayerState90 ps; // complete information about the current player at this time
};

struct idClientSnapshot91 : idClientSnapshotBase
{
	idPlayerState91 ps; // complete information about the current player at this time
};

typedef idClientSnapshot91 idLargestClientSnapshot;

inline idPlayerStateBase* GetPlayerState(idClientSnapshotBase* snap, udtProtocol::Id protocol)
{
	switch(protocol)
	{
		case udtProtocol::Dm3: return &((idClientSnapshot3*)snap)->ps;
		case udtProtocol::Dm48: return &((idClientSnapshot48*)snap)->ps;
		case udtProtocol::Dm66: return &((idClientSnapshot66*)snap)->ps;
		case udtProtocol::Dm67: return &((idClientSnapshot67*)snap)->ps;
		case udtProtocol::Dm68: return &((idClientSnapshot68*)snap)->ps;
		case udtProtocol::Dm73: return &((idClientSnapshot73*)snap)->ps;
		case udtProtocol::Dm90: return &((idClientSnapshot90*)snap)->ps;
		case udtProtocol::Dm91: return &((idClientSnapshot91*)snap)->ps;
		default: return NULL;
	}
}

// allow a lot of command backups for very fast systems
// multiple commands may be combined s32o a single packet, so this
// needs to be larger than PACKET_BACKUP
#define	CMD_BACKUP		64
#define	CMD_MASK		(CMD_BACKUP - 1)

// Two bits at the top of the idEntityStateBase::event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.
// And off the value with (~EV_EVENT_BITS) to retrieve the actual event number.
#define	ID_ES_EVENT_BIT_1    0x00000100
#define	ID_ES_EVENT_BIT_2    0x00000200
#define	ID_ES_EVENT_BITS     (ID_ES_EVENT_BIT_1 | ID_ES_EVENT_BIT_2)

#define	EVENT_VALID_MSEC	300

typedef enum
{
	EV_OBITUARY_3 = 58

} entity_event_3_t;

typedef enum
{
	EV_OBITUARY_48 = 60

} entity_event_t;

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT_68,
	EV_FALL_MEDIUM_68,
	EV_FALL_FAR_68,

	EV_JUMP_PAD_68,			// boing sound at origin, jump sound on player

	EV_JUMP_68,
	EV_WATER_TOUCH_68,	// foot touches
	EV_WATER_LEAVE_68,	// foot leaves
	EV_WATER_UNDER_68,	// head touches
	EV_WATER_CLEAR_68,	// head leaves

	EV_ITEM_PICKUP_68,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP_68,	// powerup / team sounds are broadcast to everyone

	EV_NOAMMO_68,
	EV_CHANGE_WEAPON_68,
	EV_FIRE_WEAPON_68,

	EV_USE_ITEM0_68,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,

	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY_68,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,

	EV_GIB_PLAYER,			// gib a previously living player
	EV_SCOREPLUM,			// score plum

	//#ifdef MISSIONPACK
	EV_PROXIMITY_MINE_STICK,
	EV_PROXIMITY_MINE_TRIGGER,
	EV_KAMIKAZE,			// kamikaze explodes
	EV_OBELISKEXPLODE,		// obelisk explodes
	EV_OBELISKPAIN,			// obelisk is in pain
	EV_INVUL_IMPACT,		// invulnerability sphere impact
	EV_JUICED,				// invulnerability juiced effect
	EV_LIGHTNINGBOLT,		// lightning bolt bounced of invulnerability sphere
	//#endif

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL
} entity_event_68_t;

typedef enum {
	EV_FOOTSTEP_73p = 1,
	EV_FOOTSTEP_METAL_73p = 2,
	EV_FOOTSPLASH_73p = 3,
	EV_FOOTWADE_73p = 4, // guess
	EV_SWIM_73p = 5,     // guess
	EV_FALL_SHORT_73p = 6,
	EV_FALL_MEDIUM_73p = 7,
	EV_FALL_FAR_73p = 8,
	EV_JUMP_PAD_73p = 9,
	EV_JUMP_73p = 10,
	EV_WATER_TOUCH_73p = 11,
	EV_WATER_LEAVE_73p = 12,
	EV_WATER_UNDER_73p = 13,
	EV_WATER_CLEAR_73p = 14,
	EV_ITEM_PICKUP_73p = 15,
	EV_GLOBAL_ITEM_PICKUP_73p = 16,
	EV_NOAMMO_73p = 17,
	EV_CHANGE_WEAPON_73p = 18,
	EV_DROP_WEAPON_73p = 19,
	EV_FIRE_WEAPON_73p = 20,
	EV_USE_ITEM0_73p = 21,

	EV_GLOBAL_TEAM_SOUND_73p = 44,
	EV_BULLET_HIT_FLESH_73p = 45,

	EV_DEATH1_73p = 54,
	EV_DEATH2_73p = 55,
	EV_DEATH3_73p = 56,
	EV_DROWN_73p = 57,
	EV_OBITUARY_73p = 58,

	EV_POWERUP_QUAD_73p,  // guess
	EV_POWERUP_BATTLESUIT_73p = 60,
	EV_POWERUP_REGEN_73p = 61,  // 62 in older demo zero4 vs cl0ck
	EV_POWERUP_ARMOR_REGEN_73p = 62,  // ctf silentnight (2010-12-26)  armor regen?
	EV_GIB_PLAYER_73p = 63,
	EV_SCOREPLUM_73p = 64,

	EV_PROXIMITY_MINE_STICK_73p = 65,
	EV_PROXIMITY_MINE_TRIGGER_73p = 66,

	EV_KAMIKAZE_73p = 67,			// kamikaze explodes
	EV_OBELISKEXPLODE_73p = 68,
	EV_OBELISKPAIN_73p = 69,
	EV_INVUL_IMPACT_73p = 70,		// invulnerability sphere impact

	EV_DEBUG_LINE_73p = 72,  // guess
	EV_STOPLOOPINGSOUND_73p = 73, // guess
	EV_TAUNT_73p = 74,
	EV_TAUNT_YES_73p,  // guess
	EV_TAUNT_NO_73p,  // guess
	EV_TAUNT_FOLLOWME_73p,  // guess
	EV_TAUNT_GETFLAG_73p,  // guess
	EV_TAUNT_GUARDBASE_73p,  // guess
	EV_TAUNT_PATROL_73p,  // guess

	EV_FOOTSTEP_SNOW_73p = 81,
	EV_FOOTSTEP_WOOD_73p = 82,
	EV_ITEM_PICKUP_SPEC_73p = 83,
	EV_OVERTIME_73p = 84,
	EV_GAMEOVER_73p = 85,

	EV_THAW_PLAYER_73p = 87,
	EV_THAW_TICK_73p = 88,
	EV_HEADSHOT_73p = 89,
	EV_POI_73p = 90,

	EV_RACE_START_73p = 93,
	EV_RACE_CHECKPOINT_73p = 94,
	EV_RACE_END_73p = 95,

	EV_DAMAGEPLUM_73p = 96,
	EV_AWARD_73p = 97,
} entity_event_73p_t;

// Means of Death
struct idMeansOfDeath68
{
	enum Id
	{
		Unknown,
		Shotgun,
		Gauntlet,
		MachineGun,
		Grenade,
		GrenadeSplash,
		Rocket,
		RocketSplash,
		Plasma,
		PlasmaSplash,
		RailGun,
		Lightning,
		BFG,
		BFGSplash,
		Water,
		Slime,
		Lava,
		Crush,
		TeleFrag,
		Fall,
		Suicide,
		TargetLaser,
		HurtTrigger,
		Grapple,
		Count
	};
};

struct idMeansOfDeath73p
{
	enum Id
	{
		Unknown,
		Shotgun,
		Gauntlet,
		MachineGun,
		Grenade,
		GrenadeSplash,
		Rocket,
		RocketSplash,
		Plasma,
		PlasmaSplash,
		RailGun,
		Lightning,
		BFG,
		BFGSplash,
		Water,
		Slime,
		Lava,
		Crush,
		TeleFrag,
		Fall,
		Suicide,
		TargetLaser,
		HurtTrigger,
		// Mission pack start
		NailGun,
		ChainGun,
		ProximityMine,
		Kamikaze,
		Juiced,
		// Mission pack end
		Grapple,
		// QL start
		TeamSwitch,
		Thaw,
		UnknownQlMod1,
		HeavyMachineGun,
		// QL end
		Count
	};
};

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;

typedef enum
{
	ET_EVENTS_3 = 12

} entityType_3_t;

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,

	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,

	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,

	PW_SCOUT,
	PW_GUARD,
	PW_DOUBLER,
	PW_AMMOREGEN,
	PW_INVULNERABILITY,

	PW_NUM_POWERUPS,

	PW_FIRST = PW_QUAD,
	PW_LAST = PW_INVULNERABILITY

} powerup_t;

typedef enum
{
	PW_REDFLAG_91 = 2,
	PW_BLUEFLAG_91 = 3,
	PW_NEUTRALFLAG_91 = 4,
	PW_QUAD_91 = 5,
	PW_BATTLESUIT_91 = 6,
	PW_HASTE_91 = 7,
	PW_INVIS_91 = 8,
	PW_REGEN_91 = 9,
	PW_FLIGHT_91 = 10,
	PW_INVULNERABILITY_91 = 11,

	// These are not powerups.
	// They occupy slots here for entity state marshaling.
	NOTPW_SCOUT_91 = 12,
	NOTPW_GUARD_91 = 13,
	NOTPW_DOUBLER_91 = 14,
	NOTPW_ARMORREGEN_91 = 15,

	PW_FREEZE_91 = 16,
} powerup_91_t;


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH_68,
	STAT_HOLDABLE_ITEM_68,
	STAT_WEAPONS_68,					// 16 bit fields
	STAT_ARMOR_68,				
	STAT_DEAD_YAW_68,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY_68,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH_68					// health / armor limit, changeable by handicap
} statIndex_68_t;

typedef enum {
	STAT_HEALTH_73p,
	STAT_HOLDABLE_ITEM_73p,
	STAT_PERSISTANT_POWERUP_73p,
	STAT_WEAPONS_73p,					// 16 bit fields
	STAT_ARMOR_73p,				
	STAT_BATTLE_SUIT_KILL_COUNT_73p,
	STAT_CLIENTS_READY_73p,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH_73p,				// health / armor limit, changeable by handicap
	STAT_POWERUP_REMAINING_73p = 11,
	STAT_QUAD_KILL_COUNT_73p = 13,
	STAT_ARMOR_TIER_73p = 14
} statIndex_73p_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE_68,					// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS_68,					// total pos32s damage inflicted so damage beeps can sound on change
	PERS_RANK_68,					// player rank or team rank
	PERS_TEAM_68,					// player team
	PERS_SPAWN_COUNT_68,			// incremented every respawn
	PERS_PLAYEREVENTS_68,			// 16 bits that can be flipped for events
	PERS_ATTACKER_68,				// clientnum of last damage inflicter
	PERS_ATTACKEE_ARMOR_68,			// health/armor of last person we attacked
	PERS_KILLED_68,					// count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT_68,		// two railgun hits in a row
	PERS_EXCELLENT_COUNT_68,		// two successive kills in a short amount of time
	PERS_DEFEND_COUNT_68,			// defend awards
	PERS_ASSIST_COUNT_68,			// assist awards
	PERS_GAUNTLET_FRAG_COUNT_68,	// kills with the gauntlet
	PERS_CAPTURES_68				// captures
} persEnum_68_t;

typedef enum
{
	PERS_SCORE_73p = 0,
	PERS_HITS_73p = 1,
	PERS_RANK_73p = 2,
	PERS_TEAM_73p = 3,
	PERS_SPAWN_COUNT_73p = 4,
	PERS_PLAYEREVENTS_73p = 5,
	PERS_ATTACKER_73p = 6,
	PERS_KILLED_73p = 7,
	PERS_IMPRESSIVE_COUNT_73p = 8,
	PERS_EXCELLENT_COUNT_73p = 9,
	PERS_DEFEND_COUNT_73p = 10,
	PERS_ASSIST_COUNT_73p = 11,
	PERS_GAUNTLET_FRAG_COUNT_73p = 12,
	PERS_CAPTURES_73p = 13,
	PERS_ATTACKEE_ARMOR_73p = 14
} persEnum_73_t;

// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define EF_TICKING_73		0x00000002		// used to make players play the prox mine ticking sound
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_KAMIKAZE			0x00000200
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team votetypedef enum 

struct idWeapon68
{
	enum Id
	{
		None,
		Gauntlet,
		MachineGun,
		Shotgun,
		GrenadeLauncher,
		RocketLauncher,
		LightningGun,
		Railgun,
		PlasmaGun,
		BFG,
		GrapplingHook,
		AfterLast
	};
};

struct idWeapon73p
{
	enum Id
	{
		None,
		Gauntlet,
		MachineGun,
		Shotgun,
		GrenadeLauncher,
		RocketLauncher,
		LightningGun,
		Railgun,
		PlasmaGun,
		BFG,
		GrapplingHook,
		// Mission Pack start
		NailGun,
		ProximityMineLauncher,
		ChainGun,
		// Mission Pack end
		HeavyMachineGun, // @TODO: Correct value?
		AfterLast
	};
};

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40

struct idItem68_baseq3
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		Count
	};
};

struct idItem68_CPMA
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		ItemArmorJacket,
		ItemBackpack,
		TeamCTFNeutralflag,
		Count
	};
};

struct idItem73
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemArmorJacket,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		HoldableKamikaze,
		HoldablePortal,
		HoldableInvulnerability,
		AmmoNails,
		AmmoMines,
		AmmoBelt,
		ItemScout,
		ItemGuard,
		ItemDoubler,
		ItemAmmoregen,
		TeamCTFNeutralflag,
		ItemRedcube,
		ItemBluecube,
		WeaponNailgun,
		WeaponProxLauncher,
		WeaponChaingun,
		Count
	};
};

struct idItem90
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemArmorJacket,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		HoldableKamikaze,
		HoldablePortal,
		HoldableInvulnerability,
		AmmoNails,
		AmmoMines,
		AmmoBelt,
		ItemScout,
		ItemGuard,
		ItemDoubler,
		ItemAmmoregen,
		TeamCTFNeutralflag,
		ItemRedcube,
		ItemBluecube,
		WeaponNailgun,
		WeaponProxLauncher,
		WeaponChaingun,
		ItemSpawnarmor,
		WeaponHMG,
		AmmoHMG,
		AmmoPack,
		Count
	};
};

struct idGameType3
{
	enum Id
	{
		FFA,
		Duel,
		SP,
		TDM,
		CTF,
		Count
	};
};

struct idGameType48p
{
	enum Id
	{
		FFA,
		Duel,
		SP,
		TDM,
		CTF,
		OneFlagCTF,
		Obelisk,
		Harvester,
		Count
	};
};

struct idGameType68_CPMA
{
	enum Id
	{
		HM = -1,
		FFA = 0,
		Duel = 1,
		SP = 2,
		TDM = 3,
		CTF = 4,
		CA = 5,
		FT = 6,
		CTFS = 7,
		NTF = 8,
		TwoVsTwo = 9,
		Count
	};
};

struct idGameType73p
{
	enum Id
	{
		FFA,
		Duel,
		Race,
		TDM,
		CA,
		CTF,
		OneFlagCTF,
		Obelisk,
		Harvester,
		FT,
		Domination,
		CTFS,
		RedRover,
		Count
	};
};

struct udtGame
{
	enum Id
	{
		Q3,
		QL,
		CPMA,
		OSP
	};
};

struct idFlagStatus
{
	enum Id
	{
		InBase,   // In its spot in base.
		Captured, // Being carried by an enemy player.
		Missing   // Not being carried by anyone but not in its spot either.
	};
};
