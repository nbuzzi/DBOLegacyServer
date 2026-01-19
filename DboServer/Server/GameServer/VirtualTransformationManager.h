#ifndef __VIRTUAL_TRANSFORMATION_MANAGER_H__
#define __VIRTUAL_TRANSFORMATION_MANAGER_H__

#include "NtlSingleton.h"
#include "NtlString.h"
#include "NtlSharedType.h"
#include "NtlCharacter.h"
#include "NtlCharacterState.h"
#include <unordered_map>
#include <vector>
#include <string>

class CPlayer;
class CNtlIniFile;

class CVirtualTransformationManager : public CNtlSingleton<CVirtualTransformationManager>
{
public:
    struct ModelOverride
    {
        BYTE byRace;      // Target race (RACE_HUMAN, RACE_NAMEK, RACE_MAJIN)
        BYTE byGender;    // Target gender (GENDER_MALE, GENDER_FEMALE)
        BYTE byFace;      // Face ID
        BYTE byHair;      // Hair ID
        BYTE byHairColor; // Hair color
        BYTE bySkinColor; // Skin color

        ModelOverride()
            : byRace(0), byGender(0), byFace(0), byHair(0), byHairColor(0), bySkinColor(0) {}
    };

    struct VirtualTransform
    {
        DWORD virtualId;              // 100+ for custom transformations
        BYTE baseAspectState;         // Maps to real enum (0-6)
        CNtlString name;              // Display name

        // Stat multipliers
        float hpMultiplier;
        float physAtkMultiplier;
        float engAtkMultiplier;
        float physDefMultiplier;
        float engDefMultiplier;
        float atkSpdMultiplier;
        float runSpdMultiplier;
        float physCritMultiplier;
        float engCritMultiplier;

        std::vector<DWORD> buffIds;   // Additional buffs to apply

        bool hasModelOverride;        // Enable model swap?
        ModelOverride modelOverride;  // Target model data

        VirtualTransform()
            : virtualId(0), baseAspectState(ASPECTSTATE_KAIOKEN), hasModelOverride(false),
              hpMultiplier(1.0f), physAtkMultiplier(1.0f), engAtkMultiplier(1.0f),
              physDefMultiplier(1.0f), engDefMultiplier(1.0f), atkSpdMultiplier(1.0f),
              runSpdMultiplier(1.0f), physCritMultiplier(1.0f), engCritMultiplier(1.0f) {}
    };

public:
    CVirtualTransformationManager();
    virtual ~CVirtualTransformationManager();

    bool LoadConfigFromIniPath(const char* iniPath = ".\\config\\VirtualTransforms.cfg");

    // Activate/deactivate virtual transformations
    bool ActivateVirtualTransform(CPlayer* player, DWORD virtualId);
    bool DeactivateVirtualTransform(CPlayer* player);

    // Query active transforms
    bool IsVirtualTransformActive(CPlayer* player) const;
    DWORD GetActiveVirtualTransformId(unsigned int charId) const;
    const VirtualTransform* GetVirtualTransform(DWORD virtualId) const;

    // Network packet sending
    void SendModelOverridePacket(CPlayer* player, const ModelOverride& model, DWORD virtualId);
    void SendClearModelOverridePacket(CPlayer* player);

private:
    void Init();
    bool LoadTransformFromIni(CNtlIniFile& file, DWORD transformId);
    void ApplyStatModifiers(CPlayer* player, const VirtualTransform& transform);
    void RemoveStatModifiers(CPlayer* player, const VirtualTransform& transform);

    BYTE ParseAspectStateFromString(const char* str) const;
    BYTE ParseRaceFromString(const char* str) const;
    BYTE ParseGenderFromString(const char* str) const;

private:
    std::unordered_map<DWORD, VirtualTransform> m_transforms;
    std::unordered_map<unsigned int, DWORD> m_activeTransforms; // CharID -> VirtualTransformID
    CNtlString m_cfgPath;
};

#define GetVirtualTransformManager() CVirtualTransformationManager::GetInstance()
#define g_pVirtualTransformManager GetVirtualTransformManager()

#endif // __VIRTUAL_TRANSFORMATION_MANAGER_H__
