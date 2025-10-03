#include "stdafx.h"
#include "VirtualTransformationManager.h"
#include "CPlayer.h"
#include "NtlIniFile.h"
#include "NtlPacketGU.h"
#include "ObjectManager.h"
#include "StateManager.h"
#include "CharacterAttPC.h"
#include "NtlLog.h"
#include "GameServer.h"
#include "FeatureFlags.h"

// Helper: read float from CNtlIniFile with default when key is missing
static float ReadFloatDefault(CNtlIniFile& file, const char* section, const char* key, float defVal)
{
    float v = defVal;
    // CNtlIniFile::Read returns false when key missing; in that case we keep default
    file.Read(section, key, v);
    return v;
}

CVirtualTransformationManager::CVirtualTransformationManager()
{
    Init();
}

CVirtualTransformationManager::~CVirtualTransformationManager()
{
}

void CVirtualTransformationManager::Init()
{
    m_transforms.clear();
    m_activeTransforms.clear();
    m_cfgPath = ".\\config\\VirtualTransforms.cfg";
    LoadConfigFromIniPath(m_cfgPath.c_str());
}

bool CVirtualTransformationManager::LoadConfigFromIniPath(const char* iniPath)
{
    if (!iniPath || *iniPath == '\0')
        iniPath = m_cfgPath.c_str();

    m_cfgPath = iniPath;

    // Debug: Get current working directory
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    printf("[VTRANSFORM] Current working directory: %s\n", currentDir);
    printf("[VTRANSFORM] Attempting to load: %s\n", iniPath);

    // Check if file exists
    DWORD fileAttr = GetFileAttributesA(iniPath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES)
    {
        printf("[VTRANSFORM] File does not exist or cannot be accessed!\n");
        printf("[VTRANSFORM] Full path check: %s\n", iniPath);
        return false;
    }

    CNtlIniFile file;
    int createResult = file.Create(iniPath);
    if (createResult != NTL_SUCCESS)
    {
        printf("[VTRANSFORM] Failed to load config from %s (CNtlIniFile::Create returned %d)\n", iniPath, createResult);
        return false;
    }

    printf("[VTRANSFORM] CNtlIniFile::Create succeeded!\n");

    m_transforms.clear();

    // Load all transforms (ID 100-999)
    for (DWORD id = 100; id < 1000; id++)
    {
        char sectionName[64];
        sprintf_s(sectionName, "Transform_%d", id);

        CNtlString name = file.Read(sectionName, "Name");
        if (name.c_str()[0] != '\0')
        {
            LoadTransformFromIni(file, id);
        }
    }

    ERR_LOG(LOG_GENERAL, "[VTRANSFORM] Loaded %u virtual transformations from %s",
        (unsigned)m_transforms.size(), iniPath);

    return true;
}

bool CVirtualTransformationManager::LoadTransformFromIni(CNtlIniFile& file, DWORD transformId)
{
    char sectionName[64];
    sprintf_s(sectionName, "Transform_%d", transformId);

    VirtualTransform transform;
    transform.virtualId = transformId;
    transform.name = file.Read(sectionName, "Name");

    // Parse base aspect state
    CNtlString baseAspect = file.Read(sectionName, "BaseAspect");
    transform.baseAspectState = ParseAspectStateFromString(baseAspect.c_str());

    // Load stat multipliers
    transform.hpMultiplier = ReadFloatDefault(file, sectionName, "HPMultiplier", 1.0f);
    transform.physAtkMultiplier = ReadFloatDefault(file, sectionName, "PhysAtkMultiplier", 1.0f);
    transform.engAtkMultiplier = ReadFloatDefault(file, sectionName, "EngAtkMultiplier", 1.0f);
    transform.physDefMultiplier = ReadFloatDefault(file, sectionName, "PhysDefMultiplier", 1.0f);
    transform.engDefMultiplier = ReadFloatDefault(file, sectionName, "EngDefMultiplier", 1.0f);
    transform.atkSpdMultiplier = ReadFloatDefault(file, sectionName, "AtkSpdMultiplier", 1.0f);
    transform.runSpdMultiplier = ReadFloatDefault(file, sectionName, "RunSpdMultiplier", 1.0f);
    transform.physCritMultiplier = ReadFloatDefault(file, sectionName, "PhysCritMultiplier", 1.0f);
    transform.engCritMultiplier = ReadFloatDefault(file, sectionName, "EngCritMultiplier", 1.0f);

    // Load model override settings
    int enableModelSwap = 0;
    file.Read(sectionName, "EnableModelSwap", enableModelSwap);
    transform.hasModelOverride = (enableModelSwap != 0);

    if (transform.hasModelOverride)
    {
        CNtlString raceStr = file.Read(sectionName, "TargetRace");
        CNtlString genderStr = file.Read(sectionName, "TargetGender");

        transform.modelOverride.byRace = ParseRaceFromString(raceStr.c_str());
        transform.modelOverride.byGender = ParseGenderFromString(genderStr.c_str());

        int face = 0, hair = 0, hairColor = 0, skinColor = 0;
        file.Read(sectionName, "TargetFace", face);
        file.Read(sectionName, "TargetHair", hair);
        file.Read(sectionName, "TargetHairColor", hairColor);
        file.Read(sectionName, "TargetSkinColor", skinColor);

        transform.modelOverride.byFace = (BYTE)face;
        transform.modelOverride.byHair = (BYTE)hair;
        transform.modelOverride.byHairColor = (BYTE)hairColor;
        transform.modelOverride.bySkinColor = (BYTE)skinColor;
    }

    m_transforms[transformId] = transform;

    ERR_LOG(LOG_GENERAL, "[VTRANSFORM] Loaded: %s (ID %d, Base: %d, ModelSwap: %s)",
        transform.name.c_str(), transformId, transform.baseAspectState,
        transform.hasModelOverride ? "YES" : "NO");

    return true;
}

BYTE CVirtualTransformationManager::ParseAspectStateFromString(const char* str) const
{
    if (!str) return ASPECTSTATE_KAIOKEN;

    if (_stricmp(str, "ASPECTSTATE_SUPER_SAIYAN") == 0 || _stricmp(str, "SSJ") == 0)
        return ASPECTSTATE_SUPER_SAIYAN;
    if (_stricmp(str, "ASPECTSTATE_PURE_MAJIN") == 0 || _stricmp(str, "PURE_MAJIN") == 0)
        return ASPECTSTATE_PURE_MAJIN;
    if (_stricmp(str, "ASPECTSTATE_GREAT_NAMEK") == 0 || _stricmp(str, "GREAT_NAMEK") == 0)
        return ASPECTSTATE_GREAT_NAMEK;
    if (_stricmp(str, "ASPECTSTATE_KAIOKEN") == 0 || _stricmp(str, "KAIOKEN") == 0)
        return ASPECTSTATE_KAIOKEN;
    if (_stricmp(str, "ASPECTSTATE_VEHICLE") == 0 || _stricmp(str, "VEHICLE") == 0)
        return ASPECTSTATE_VEHICLE;

    return ASPECTSTATE_KAIOKEN; // Default fallback
}

BYTE CVirtualTransformationManager::ParseRaceFromString(const char* str) const
{
    if (!str) return RACE_HUMAN;

    if (_stricmp(str, "RACE_HUMAN") == 0 || _stricmp(str, "HUMAN") == 0)
        return RACE_HUMAN;
    if (_stricmp(str, "RACE_NAMEK") == 0 || _stricmp(str, "NAMEK") == 0)
        return RACE_NAMEK;
    if (_stricmp(str, "RACE_MAJIN") == 0 || _stricmp(str, "MAJIN") == 0)
        return RACE_MAJIN;

    return RACE_HUMAN;
}

BYTE CVirtualTransformationManager::ParseGenderFromString(const char* str) const
{
    if (!str) return GENDER_MALE;

    if (_stricmp(str, "GENDER_MALE") == 0 || _stricmp(str, "MALE") == 0)
        return GENDER_MALE;
    if (_stricmp(str, "GENDER_FEMALE") == 0 || _stricmp(str, "FEMALE") == 0)
        return GENDER_FEMALE;

    return GENDER_MALE;
}

bool CVirtualTransformationManager::ActivateVirtualTransform(CPlayer* player, DWORD virtualId)
{
    // Check feature flag
    if (!g_pFeatureFlags->IsVirtualTransformationsEnabled())
    {
        ERR_LOG(LOG_GENERAL, "[VTRANSFORM] Virtual transformations are disabled by feature flag");
        return false;
    }

    if (!player || !player->IsInitialized())
        return false;

    auto it = m_transforms.find(virtualId);
    if (it == m_transforms.end())
    {
        ERR_LOG(LOG_GENERAL, "[VTRANSFORM] Transform ID %d not found", virtualId);
        return false;
    }

    const VirtualTransform& transform = it->second;
    unsigned int charId = player->GetCharID();

    // Check if already transformed
    if (IsVirtualTransformActive(player))
    {
        DeactivateVirtualTransform(player);
    }

    // Send model override packet to protector (if enabled)
    if (transform.hasModelOverride)
    {
        SendModelOverridePacket(player, transform.modelOverride, virtualId);
    }

    // Apply stat modifiers
    ApplyStatModifiers(player, transform);

    // Activate base aspect state transformation
    CStateManager* pStateManager = player->GetStateManager();
    if (pStateManager)
    {
        // Use existing API: Change aspect to requested state
        pStateManager->ChangeAspectState(transform.baseAspectState, NULL, true);
    }

    // Store active transform
    m_activeTransforms[charId] = virtualId;

    ERR_LOG(LOG_GENERAL, "[VTRANSFORM] Activated %s for %s (CharID: %d)",
        transform.name.c_str(), player->GetCharName(), charId);

    return true;
}

bool CVirtualTransformationManager::DeactivateVirtualTransform(CPlayer* player)
{
    if (!player || !player->IsInitialized())
        return false;

    unsigned int charId = player->GetCharID();
    auto it = m_activeTransforms.find(charId);
    if (it == m_activeTransforms.end())
        return false;

    DWORD virtualId = it->second;
    auto transformIt = m_transforms.find(virtualId);
    if (transformIt == m_transforms.end())
        return false;

    const VirtualTransform& transform = transformIt->second;

    // Clear model override
    if (transform.hasModelOverride)
    {
        SendClearModelOverridePacket(player);
    }

    // Remove stat modifiers
    RemoveStatModifiers(player, transform);

    // End aspect state
    CStateManager* pStateManager = player->GetStateManager();
    if (pStateManager)
    {
        // Use existing API: reset aspect to INVALID
        pStateManager->ChangeAspectState(ASPECTSTATE_INVALID, NULL, true);
    }

    // Remove from active list
    m_activeTransforms.erase(it);

    ERR_LOG(LOG_GENERAL, "[VTRANSFORM] Deactivated %s for %s",
        transform.name.c_str(), player->GetCharName());

    return true;
}

void CVirtualTransformationManager::ApplyStatModifiers(CPlayer* player, const VirtualTransform& transform)
{
    CCharacterAttPC* pAtt = reinterpret_cast<CCharacterAttPC*>(player->GetCharAtt());
    if (!pAtt)
        return;

    // Apply HP multiplier
    if (transform.hpMultiplier != 1.0f)
    {
        DWORD maxLp = pAtt->GetMaxLP();
        DWORD bonus = (DWORD)((float)maxLp * (transform.hpMultiplier - 1.0f));
        if (bonus > 0)
            pAtt->CalculateMaxLP((float)bonus, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
    }

    // Apply attack multipliers
    if (transform.physAtkMultiplier != 1.0f)
    {
        WORD cur = pAtt->GetPhysicalOffence();
        WORD bonus = (WORD)((float)cur * (transform.physAtkMultiplier - 1.0f));
        if (bonus > 0)
            pAtt->CalculatePhysicalOffence((float)bonus, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
    }

    if (transform.engAtkMultiplier != 1.0f)
    {
        WORD cur = pAtt->GetEnergyOffence();
        WORD bonus = (WORD)((float)cur * (transform.engAtkMultiplier - 1.0f));
        if (bonus > 0)
            pAtt->CalculateEnergyOffence((float)bonus, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
    }

    // Apply defense multipliers
    if (transform.physDefMultiplier != 1.0f)
    {
        WORD cur = pAtt->GetPhysicalDefence();
        WORD bonus = (WORD)((float)cur * (transform.physDefMultiplier - 1.0f));
        if (bonus > 0)
            pAtt->CalculatePhysicalDefence((float)bonus, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
    }

    if (transform.engDefMultiplier != 1.0f)
    {
        WORD cur = pAtt->GetEnergyDefence();
        WORD bonus = (WORD)((float)cur * (transform.engDefMultiplier - 1.0f));
        if (bonus > 0)
            pAtt->CalculateEnergyDefence((float)bonus, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
    }

    // Apply speed multiplier
    if (transform.runSpdMultiplier != 1.0f)
    {
        float cur = pAtt->GetRunSpeed();
        float bonus = cur * (transform.runSpdMultiplier - 1.0f);
        if (bonus > 0.0f)
            pAtt->CalculateRunSpeed(bonus, SYSTEM_EFFECT_APPLY_TYPE_VALUE, true);
    }

    // Send attribute update reflecting changes
    pAtt->CalculateAtt();
}

void CVirtualTransformationManager::RemoveStatModifiers(CPlayer* player, const VirtualTransform& transform)
{
    // Recalculate all attributes from base (will remove transformation bonuses)
    CCharacterAttPC* pAtt = reinterpret_cast<CCharacterAttPC*>(player->GetCharAtt());
    if (pAtt)
    {
        // Recalculate all attributes from base (remove any ad-hoc modifiers) and broadcast
        pAtt->CalculateAll();
    }
}

void CVirtualTransformationManager::SendModelOverridePacket(CPlayer* player, const ModelOverride& model, DWORD virtualId)
{
    CNtlPacket packet(sizeof(sGU_VIRTUAL_TRANSFORM_DATA));
    sGU_VIRTUAL_TRANSFORM_DATA* res = (sGU_VIRTUAL_TRANSFORM_DATA*)packet.GetPacketData();
    res->wOpCode = GU_VIRTUAL_TRANSFORM_DATA;
    res->handle = player->GetID();
    res->virtualTransformId = virtualId;
    res->targetRace = model.byRace;
    res->targetGender = model.byGender;
    res->targetFace = model.byFace;
    res->targetHair = model.byHair;
    res->targetHairColor = model.byHairColor;
    res->targetSkinColor = model.bySkinColor;
    packet.SetPacketLen(sizeof(sGU_VIRTUAL_TRANSFORM_DATA));

    // Broadcast to player and nearby players
    player->Broadcast(&packet);
}

void CVirtualTransformationManager::SendClearModelOverridePacket(CPlayer* player)
{
    CNtlPacket packet(sizeof(sGU_VIRTUAL_TRANSFORM_DATA));
    sGU_VIRTUAL_TRANSFORM_DATA* res = (sGU_VIRTUAL_TRANSFORM_DATA*)packet.GetPacketData();
    res->wOpCode = GU_VIRTUAL_TRANSFORM_DATA;
    res->handle = player->GetID();
    res->virtualTransformId = 0; // 0 = clear override
    res->targetRace = 0;
    res->targetGender = 0;
    res->targetFace = 0;
    res->targetHair = 0;
    res->targetHairColor = 0;
    res->targetSkinColor = 0;
    packet.SetPacketLen(sizeof(sGU_VIRTUAL_TRANSFORM_DATA));

    player->Broadcast(&packet);
}

bool CVirtualTransformationManager::IsVirtualTransformActive(CPlayer* player) const
{
    if (!player)
        return false;
    return m_activeTransforms.find(player->GetCharID()) != m_activeTransforms.end();
}

DWORD CVirtualTransformationManager::GetActiveVirtualTransformId(unsigned int charId) const
{
    auto it = m_activeTransforms.find(charId);
    return (it != m_activeTransforms.end()) ? it->second : 0;
}

const CVirtualTransformationManager::VirtualTransform* CVirtualTransformationManager::GetVirtualTransform(DWORD virtualId) const
{
    auto it = m_transforms.find(virtualId);
    return (it != m_transforms.end()) ? &it->second : nullptr;
}
