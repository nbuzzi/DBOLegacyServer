#include "stdafx.h"
#include "battle.h"
#include "calcs.h"
#include "Npc.h"
#include "CPlayer.h"
#include "FormulaTable.h"

// =======================================================
// Helpers
// =======================================================

inline float CalcStateOffence(bool subReq, CCharacterAtt* a)
{
    const float phys = subReq ? (float)a->GetSubWeaponPhysicalOffence() : (float)a->GetPhysicalOffence();
    const float ener = subReq ? (float)a->GetSubWeaponEnergyOffence() : (float)a->GetEnergyOffence();
    return (phys + ener) / 2.5f;
}

// Usa el atributo de batalla correcto para props.
// Si NO tienes getter para el atributo de la sub-weapon, neutralizamos al usar sub.
inline BYTE BattleAttrForSkill(CCharacterObject* caster, bool subRequired)
{
    CCharacterAtt* a = caster->GetCharAtt();

    // Si existe algo tipo a->GetSubWeaponBattleAttributeOffence(), úsalo aquí:
    // return subRequired ? a->GetSubWeaponBattleAttributeOffence() : a->GetBattleAttributeOffence();

    return subRequired ? (BYTE)BATTLE_ATTRIBUTE_NONE : a->GetBattleAttributeOffence();
}

// =======================================================
// Probabilidades
// =======================================================

bool BattleIsCrit(CCharacterAtt* pAttackerAtt, CCharacterAtt* pTargetAtt, bool bIsPhysical)
{
    float fVar, fVar2;
    const float fCritRate = bIsPhysical ? (float)pAttackerAtt->GetPhysicalCriticalRate()
        : (float)pAttackerAtt->GetEnergyCriticalRate();

    if (bIsPhysical)
    {
        fVar = CFormulaTable::m_afRate[9100][1] + (pAttackerAtt->GetEng() / CFormulaTable::m_afRate[9100][2]);
        fVar2 = (float)pAttackerAtt->GetEng();
    }
    else
    {
        fVar = CFormulaTable::m_afRate[9100][1] + (pAttackerAtt->GetCon() / CFormulaTable::m_afRate[9100][2]);
        fVar2 = (float)pAttackerAtt->GetCon();
    }

    float fRate = fCritRate - (fCritRate * (pTargetAtt->GetCriticalBlockSuccessRate() / 100.f)) + fVar2 / fVar;

    if (fRate > 90.0f) fRate = 90.0f;
    return Dbo_CheckProbabilityF(fRate);
}

bool BattleIsDodge(bool bTargetPC, WORD hitrate, WORD dodge, BYTE byAttackerLv, BYTE byTargetLv)
{
    float fRate = 100.0f - (CFormulaTable::m_afRate[3700][1]
        * (float)hitrate / (float)MAX(hitrate + dodge, 1)
        * ((float)(byAttackerLv + 1) / (float)(byAttackerLv + byTargetLv)) * 100.0f);

    if (fRate > 90.f) fRate = 90.0f;
    return Dbo_CheckProbabilityF(fRate);
}

bool BattleIsResist(WORD wSuccessRate, WORD wResistRate, BYTE byAttackerLv, BYTE byTargetLv)
{
    float fRate = 100.0f - (CFormulaTable::m_afRate[3900][1]
        * (float)wSuccessRate / (float)MAX(wSuccessRate + wResistRate, 1)
        * ((float)(byAttackerLv + 1) / (float)(byAttackerLv + byTargetLv)) * 100.0f);

    if (fRate > 90.f) fRate = 90.f;
    return Dbo_CheckProbabilityF(fRate);
}

bool BattleIsBlock(WORD wDefenceRate, BYTE byAttackerLv, BYTE byTargetLv)
{
    float fRate = ((float)wDefenceRate * 2.f + (byTargetLv - byAttackerLv)) / 200.f;
    if (fRate > 20.f) fRate = 20.f;
    return Dbo_CheckProbabilityF(fRate);
}

// =======================================================
// Damage helpers
// =======================================================

inline float ApplyPropsBonus(float attackerPower, float FinalProp)
{
    if (FinalProp == 0.f) return attackerPower;
    return attackerPower + attackerPower * (FinalProp / 100.f);
}

inline float CalcDefenseWithPen(float def, float penRate)
{
    return def - (penRate * def / 100.f);
}

inline void CalcMinMax(float fDmg0, float level, float& outMin, float& outMax)
{
    outMin = fDmg0 * (CFormulaTable::m_afRate[3500][1] + (level * CFormulaTable::m_afRate[3500][2]));
    outMax = fDmg0 * (CFormulaTable::m_afRate[3500][3] - (level * CFormulaTable::m_afRate[3500][4]));
}

// =======================================================
// Skill Damage
// =======================================================

void CalcSkillDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr,
    float fBaseSkillDmg, float& resultvalue, BYTE& rAttackResult, int& rfReflectDmg,
    sDBO_LP_EP_RECOVERED* pLpEpRecover, bool bIncreaseDmg/*=false*/, bool /*bAttackFromBehindBonus*/ /*=false*/)
{
    CCharacterAtt* A = pCaster->GetCharAtt();
    CCharacterAtt* D = victim->GetCharAtt();

    float fAttackerPower = 0.f;
    float fTargetDefensePower = 0.f;
    float fCritDefRate = 0.f;

    const bool subReq = pCaster->IsPC() && (skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON);
    const bool isValue = skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_VALUE;
    const bool isPercent = skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_PERCENT;
    const BYTE skillType = skilltbl->bySkill_Type;

    // --- Power & Defense ---
    if (isValue)
    {
        if (skillType == NTL_SKILL_TYPE_PHYSICAL)
        {
            fAttackerPower = fBaseSkillDmg;
            fTargetDefensePower = (float)D->GetPhysicalDefence();
            fCritDefRate = D->GetPhysicalCriticalDefenceRate();
        }
        else if (skillType == NTL_SKILL_TYPE_ENERGY)
        {
            fAttackerPower = fBaseSkillDmg;
            fTargetDefensePower = (float)D->GetEnergyDefence();
            fCritDefRate = D->GetEnergyCriticalDefenceRate();
        }
        else // STATE
        {
            fAttackerPower = fBaseSkillDmg + CalcStateOffence(subReq, A);
            fTargetDefensePower = ((float)D->GetPhysicalDefence() + (float)D->GetEnergyDefence()) / 1.5f;
            fCritDefRate = (D->GetPhysicalCriticalDefenceRate() + D->GetEnergyCriticalDefenceRate()) / 2.f;
        }
    }
    else if (isPercent)
    {
        if (skillType == NTL_SKILL_TYPE_PHYSICAL)
        {
            const float off = subReq ? (float)A->GetSubWeaponPhysicalOffence()
                : (float)A->GetPhysicalOffence();
            fAttackerPower = (off * fBaseSkillDmg) / 100.f;
            fTargetDefensePower = (float)D->GetPhysicalDefence();
            fCritDefRate = D->GetPhysicalCriticalDefenceRate();
        }
        else if (skillType == NTL_SKILL_TYPE_ENERGY)
        {
            const float off = subReq ? (float)A->GetSubWeaponEnergyOffence()
                : (float)A->GetEnergyOffence();
            fAttackerPower = (off * fBaseSkillDmg) / 100.f;
            fTargetDefensePower = (float)D->GetEnergyDefence();
            fCritDefRate = D->GetEnergyCriticalDefenceRate();
        }
        else // STATE
        {
            const float stateOff = CalcStateOffence(subReq, A);
            fAttackerPower = (stateOff * fBaseSkillDmg) / 100.f;
            fTargetDefensePower = ((float)D->GetPhysicalDefence() + (float)D->GetEnergyDefence()) / 1.5f;
            fCritDefRate = (D->GetPhysicalCriticalDefenceRate() + D->GetEnergyCriticalDefenceRate()) / 2.f;
        }
    }

    // --- Props ---
    const BYTE offAttr = BattleAttrForSkill(pCaster, subReq);
    float FinalProp = GetAttributeBonusRate(
        pCaster->IsPC(),
        subReq ? true : false,
        offAttr,
        D->GetBattleAttributeDefence(),
        0,
        A->GetAvatarAttribute(),
        D->GetAvatarAttribute());

    fAttackerPower = ApplyPropsBonus(fAttackerPower, FinalProp);

    // --- Base damage window ---
    const float denom = (float)pCaster->GetLevel() * CFormulaTable::m_afRate[3100][1];
    float fDmg0 = fAttackerPower * (1.05f - (fTargetDefensePower / (fTargetDefensePower + denom)));
    if (fDmg0 < 0.f) fDmg0 = 0.f;

    float min_damage, max_damage;
    CalcMinMax(fDmg0, (float)pCaster->GetLevel(), min_damage, max_damage);
    const float fFinalDamage = RandomRangeF(min_damage, max_damage);

    resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

    // --- Critical bonus (una sola vez) ---
    if (rAttackResult == BATTLE_ATTACK_RESULT_CRITICAL_HIT)
    {
        float fCritDmgRate = 0.f;

        if (skillType == NTL_SKILL_TYPE_PHYSICAL)      fCritDmgRate = A->GetPhysicalCriticalDamageRate();
        else if (skillType == NTL_SKILL_TYPE_ENERGY)   fCritDmgRate = A->GetEnergyCriticalDamageRate();
        else                                           fCritDmgRate = (A->GetPhysicalCriticalDamageRate() + A->GetEnergyCriticalDamageRate()) / 2.f;

        float fCritBonus = (resultvalue * fCritDmgRate) / 100.f;
        if (bIncreaseDmg) fCritBonus *= (DBO_BATTLE_OFFENCE_BONUS_RATE_BY_CRITICAL / 100.f);
        fCritBonus -= fCritBonus * fCritDefRate / 100.f;
        resultvalue += fCritBonus;
    }

    // --- Reflect ---
    rfReflectDmg += (int)GetSkillReflectDamage(resultvalue, skillType, D->GetPhysicalReflection(), D->GetEnergyReflection());

    // --- LP/EP on hit ---
    if (pLpEpRecover)
    {
        pLpEpRecover->targetLpRecoveredWhenHit = (int)(D->GetLpRecoveryWhenHit() + (resultvalue * D->GetLpRecoveryWhenHitInPercent() / 100.0f));
        pLpEpRecover->bIsLpRecoveredWhenHit = pLpEpRecover->targetLpRecoveredWhenHit > 0;

        pLpEpRecover->dwTargetEpRecoveredWhenHit = (DWORD)(D->GetEpRecoveryWhenHit() + (resultvalue * D->GetEpRecoveryWhenHitInPercent() / 100.0f));
        pLpEpRecover->bIsEpRecoveredWhenHit = pLpEpRecover->dwTargetEpRecoveredWhenHit > 0;
    }
}

// =======================================================
// Special Skill Damage
// =======================================================

void CalcSpecialSkillDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE /*byEffectNr*/,
    float fBaseSkillDmg, float& resultvalue, BYTE& rAttackResult, int& rfReflectDmg, sDBO_LP_EP_RECOVERED& rLpEpRecover)
{
    CCharacterAtt* A = pCaster->GetCharAtt();
    CCharacterAtt* D = victim->GetCharAtt();

    float fAttackerPower = 0.f;
    float fTargetDefensePower = 0.f;
    float fCritDefRate = 0.f;

    const bool subReq = pCaster->IsPC() && (skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON);
    const BYTE skillType = skilltbl->bySkill_Type;

    if (skillType == NTL_SKILL_TYPE_PHYSICAL)
    {
        const float off = subReq ? (float)A->GetSubWeaponPhysicalOffence() : (float)A->GetPhysicalOffence();
        fAttackerPower = (off * (fBaseSkillDmg / 2.f)) / 100.f;
        fTargetDefensePower = (float)D->GetPhysicalDefence();
        fCritDefRate = D->GetPhysicalCriticalDefenceRate();
    }
    else if (skillType == NTL_SKILL_TYPE_ENERGY)
    {
        const float off = subReq ? (float)A->GetSubWeaponEnergyOffence() : (float)A->GetEnergyOffence();
        fAttackerPower = (off * (fBaseSkillDmg / 2.f)) / 100.f;
        fTargetDefensePower = (float)D->GetEnergyDefence();
        fCritDefRate = D->GetEnergyCriticalDefenceRate();
    }
    else // STATE
    {
        const float stateOff = subReq
            ? ((float)A->GetSubWeaponPhysicalOffence() + (float)A->GetSubWeaponEnergyOffence()) / 2.f
            : ((float)A->GetPhysicalOffence() + (float)A->GetEnergyOffence()) / 2.f;

        fAttackerPower = (stateOff * fBaseSkillDmg) / 100.f;
        fTargetDefensePower = ((float)D->GetPhysicalDefence() + (float)D->GetEnergyDefence()) / 2.f;
        fCritDefRate = (D->GetPhysicalCriticalDefenceRate() + D->GetEnergyCriticalDefenceRate()) / 2.f;
    }

    float fDmg0 = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + (float)pCaster->GetLevel() * 15.f)));
    if (fDmg0 < 0.f) fDmg0 = 0.f;

    float min_damage, max_damage;
    CalcMinMax(fDmg0, (float)pCaster->GetLevel(), min_damage, max_damage);

    const float fFinalDamage = RandomRangeF(min_damage, max_damage);
    resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

    if (rAttackResult == BATTLE_ATTACK_RESULT_CRITICAL_HIT)
    {
        float fCritDmgRate = 0.f;
        if (skillType == NTL_SKILL_TYPE_PHYSICAL)      fCritDmgRate = A->GetPhysicalCriticalDamageRate();
        else if (skillType == NTL_SKILL_TYPE_ENERGY)   fCritDmgRate = A->GetEnergyCriticalDamageRate();
        else                                           fCritDmgRate = (A->GetPhysicalCriticalDamageRate() + A->GetEnergyCriticalDamageRate()) / 2.f;

        float fCritBonus = (resultvalue * fCritDmgRate) / 100.f;
        fCritBonus -= fCritBonus * fCritDefRate / 100.f;
        resultvalue += fCritBonus;
    }

    rfReflectDmg += (int)GetSkillReflectDamage(resultvalue, skillType, D->GetPhysicalReflection(), D->GetEnergyReflection());

    // LP/EP on hit
    rLpEpRecover.targetLpRecoveredWhenHit += (int)(D->GetLpRecoveryWhenHit() + (resultvalue * D->GetLpRecoveryWhenHitInPercent() / 100.0f));
    rLpEpRecover.bIsLpRecoveredWhenHit = rLpEpRecover.targetLpRecoveredWhenHit > 0;
    rLpEpRecover.dwTargetEpRecoveredWhenHit += (DWORD)(D->GetEpRecoveryWhenHit() + (resultvalue * D->GetEpRecoveryWhenHitInPercent() / 100.0f));
    rLpEpRecover.bIsEpRecoveredWhenHit = rLpEpRecover.dwTargetEpRecoveredWhenHit > 0;
}

// =======================================================
// DoT Damage
// =======================================================

void CalcSkillDotDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr,
    WORD wDefence, float fBaseSkillDmg, float fBonusDmg, float& resultvalue, BYTE rAttackResult, BYTE byEffectCode)
{
    CCharacterAtt* A = pCaster->GetCharAtt();
    CCharacterAtt* D = victim->GetCharAtt();

    float fAttackerPower = 0.f;
    float fTargetDefensePower = (float)wDefence;
    float fCritDefRate = 0.f;

    const bool subReq = pCaster->IsPC() && (skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON);
    const BYTE skillType = skilltbl->bySkill_Type;

    if (skilltbl->bySkill_Effect_Type[byEffectNr] == SYSTEM_EFFECT_APPLY_TYPE_VALUE)
    {
        fAttackerPower = fBaseSkillDmg;
        // 25.f tal y como tenías
        const float denom = (float)pCaster->GetLevel() * 25.f;
        float fDmg0 = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + denom)));
        if (fDmg0 < 0.f) fDmg0 = 0.f;
        fTargetDefensePower = 0.f; // ya aplicado arriba
        fAttackerPower = fDmg0;    // seguimos el pipeline común abajo
    }
    else // PERCENT
    {
        if (skillType == NTL_SKILL_TYPE_PHYSICAL)
        {
            const float off = subReq ? (float)A->GetSubWeaponPhysicalOffence()
                : (float)A->GetPhysicalOffence();

            fAttackerPower = (off * fBaseSkillDmg) / 100.f;
            fTargetDefensePower += (float)D->GetPhysicalDefence();
            fCritDefRate = D->GetPhysicalCriticalDefenceRate();
        }
        else if (skillType == NTL_SKILL_TYPE_ENERGY)
        {
            const float off = subReq ? (float)A->GetSubWeaponEnergyOffence()
                : (float)A->GetEnergyOffence();

            fAttackerPower = (off * fBaseSkillDmg) / 100.f;
            fTargetDefensePower += (float)D->GetEnergyDefence();
            fCritDefRate = D->GetEnergyCriticalDefenceRate();
        }
        else // STATE
        {
            const float stateOff = subReq
                ? ((float)A->GetSubWeaponPhysicalOffence() + (float)A->GetSubWeaponEnergyOffence()) / 2.f
                : ((float)A->GetPhysicalOffence() + (float)A->GetEnergyOffence()) / 2.f;

            fAttackerPower = (stateOff * fBaseSkillDmg) / 100.f;
            fTargetDefensePower += ((float)D->GetPhysicalDefence() + (float)D->GetEnergyDefence()) / 2.f;
            fCritDefRate = (D->GetPhysicalCriticalDefenceRate() + D->GetEnergyCriticalDefenceRate()) / 2.f;
        }

        // 35.f tal y como tenías
        const float denom = (float)pCaster->GetLevel() * 35.f;
        float fDmg0 = fAttackerPower * (1.f - (fTargetDefensePower / (fTargetDefensePower + denom)));
        if (fDmg0 < 0.f) fDmg0 = 0.f;
        fAttackerPower = fDmg0;
    }

    // Ajuste de bonus por rango concreto (corregido &&)
    if (skilltbl->tblidx >= 910471 && skilltbl->tblidx <= 910476)
        fBonusDmg /= 1.8f;

    float fFinalDamage = fAttackerPower + fBonusDmg;

    if (victim->IsPC())
    {
        if (byEffectCode == ACTIVE_BLEED || byEffectCode == ACTIVE_BURN || wDefence < 1)
            fFinalDamage -= (float)wDefence;
        else
            fFinalDamage -= (float)wDefence / 2.0f;
    }

    if (fFinalDamage < 0.f) fFinalDamage = 0.f;
    resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;

    // Crit de DoT (tu lógica)
    if (rAttackResult == BATTLE_ATTACK_RESULT_CRITICAL_HIT)
    {
        float fCritDmgRate = 0.0f;
        if (skillType == NTL_SKILL_TYPE_PHYSICAL)      fCritDmgRate = A->GetPhysicalCriticalDamageRate() / 2.f;
        else if (skillType == NTL_SKILL_TYPE_ENERGY)   fCritDmgRate = A->GetEnergyCriticalDamageRate() / 2.f;
        else                                           fCritDmgRate = (A->GetPhysicalCriticalDamageRate() + A->GetEnergyCriticalDamageRate()) / 4.f;

        float fCritDmgBonus = (resultvalue * fCritDmgRate) / 100.f;
        fCritDmgBonus -= fCritDmgBonus * fCritDefRate / 100.f;
        resultvalue += fCritDmgBonus;
    }
}

// =======================================================
// Life Steal
// =======================================================

void CalcLifeStealDamage(CCharacterObject* pCaster, CCharacterObject* victim, sSKILL_TBLDAT* skilltbl, BYTE /*byEffectNr*/,
    float fBaseSkillDmg, float& resultvalue)
{
    CCharacterAtt* A = pCaster->GetCharAtt();
    CCharacterAtt* D = victim->GetCharAtt();

    const bool subReq = pCaster->IsPC() && (skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON);

    const bool isEnergy = (skilltbl->bySkill_Type == NTL_SKILL_TYPE_ENERGY);
    const float off = isEnergy
        ? (subReq ? (float)A->GetSubWeaponEnergyOffence() : (float)A->GetEnergyOffence())
        : (subReq ? (float)A->GetSubWeaponPhysicalOffence() : (float)A->GetPhysicalOffence());

    const float def = isEnergy ? (float)D->GetEnergyDefence() : (float)D->GetPhysicalDefence();

    const float fAttackerPower = fBaseSkillDmg + off;
    const float denom = (float)pCaster->GetLevel() * 35.f;

    float fFinalDamage = fAttackerPower * (1.f - (def / (def + denom)));
    resultvalue = (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;
}

// =======================================================
// Melee
// =======================================================

float CalcMeleeDamage(CCharacter* pkAttacker, CCharacter* pkVictim)
{
    CCharacterAtt* A = pkAttacker->GetCharAtt();
    CCharacterAtt* D = pkVictim->GetCharAtt();

    const bool isEnergy = (pkAttacker->GetAttackType() == BATTLE_ATTACK_TYPE_ENERGY);

    float fAttackerPower = isEnergy ? (float)A->GetEnergyOffence() : (float)A->GetPhysicalOffence();
    float fTargetDefensePower = isEnergy ? (float)D->GetEnergyDefence() : (float)D->GetPhysicalDefence();

    // Props (ofensivo del atacante!)
    float FinalProp = GetAttributeBonusRate(
        pkAttacker->IsPC(),
        false,
        A->GetBattleAttributeOffence(),
        D->GetBattleAttributeDefence(),
        0,
        A->GetAvatarAttribute(),
        D->GetAvatarAttribute());

    fAttackerPower = ApplyPropsBonus(fAttackerPower, FinalProp);

    float fDmg0 = fAttackerPower * (1.05f - (fTargetDefensePower / (fTargetDefensePower + (float)pkAttacker->GetLevel() * CFormulaTable::m_afRate[3100][1])));
    if (fDmg0 < 0.f) fDmg0 = 0.f;

    float min_damage, max_damage;
    CalcMinMax(fDmg0, (float)pkAttacker->GetLevel(), min_damage, max_damage);

    const float fFinalDamage = RandomRangeF(min_damage, max_damage);
    return (fFinalDamage <= 1.f) ? 1.f : fFinalDamage;
}

// =======================================================
// Heals
// =======================================================

void CalcDirectHeal(CCharacterObject* pCaster, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr, float& resultvalue)
{
    CCharacterAtt* a = pCaster->GetCharAtt();
    const bool subRequired = pCaster->IsPC() && (skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON);

    const float en = (float)a->GetEnergyOffence();
    const float sub = (float)a->GetSubWeaponEnergyOffence();

    const float off = subRequired ? sub : (en + sub);

    resultvalue = (float)skilltbl->aSkill_Effect_Value[byEffectNr];
    resultvalue += off;                                                 // aporte directo
    resultvalue += off * a->GetDirectHealPowerBonusInPercent() / 100.f; // % bonus
    resultvalue += a->GetDirectHealPowerBonus();                        // bonus fijo
}

void CalcHealOverTime(CCharacterObject* pCaster, sSKILL_TBLDAT* skilltbl, BYTE byEffectNr, float& resultvalue)
{
    CCharacterAtt* a = pCaster->GetCharAtt();
    const bool subRequired = pCaster->IsPC() && (skilltbl->byRequire_Epuip_Slot_Type == EQUIP_SLOT_TYPE_SUB_WEAPON);

    const float en = (float)a->GetEnergyOffence();
    const float sub = (float)a->GetSubWeaponEnergyOffence();

    const float off = subRequired ? sub : (en + sub);

    resultvalue = (float)skilltbl->aSkill_Effect_Value[byEffectNr];
    resultvalue += off * a->GetHotPowerBonusInPercent() / 100.f; // % bonus
    resultvalue += a->GetHotPowerBonus();                        // bonus fijo
}

// =======================================================
// Aggro
// =======================================================

void IncreaseTargetEnemyAggro(CCharacter* pCaster, CCharacter* pTarget, DWORD dwDefaultAggro)
{
    int nAgro = (int)(dwDefaultAggro + pCaster->GetCharAtt()->GetSkillAggroBonus());
    nAgro += (int)((float)nAgro * pCaster->GetCharAtt()->GetSkillAggroBonusInPercent() / 100.f);

    CTargetListManager::AGGROPOINT_MAP::iterator it = pTarget->GetTargetListManager()->AggroBegin();
    CTargetListManager::AGGROPOINT_MAP::iterator itEnd = pTarget->GetTargetListManager()->AggroEnd();

    int nLoopCount = 0;

    while (it != itEnd)
    {
        ++nLoopCount;
        if (nLoopCount > 5000)
        {
            ERR_LOG(LOG_GENERAL, "INFINITE LOOP FOUND");
        }

        CCharacter* pAttacker = g_pObjectManager->GetChar(it->first);
        if (pAttacker && pAttacker->IsInitialized())
        {
            if (pAttacker->IsNPC() || pAttacker->IsMonster())
            {
                pAttacker->ChangeAggro(pCaster->GetID(), DBO_AGGRO_CHANGE_TYPE_INCREASE, (DWORD)nAgro);
            }
        }

        ++it;
    }
}

// =======================================================
// Reflects
// =======================================================

float GetReflectDamage(float fDmg, BYTE byAttackType, float fPhysicalReflect, float fEnergyReflect)
{
    if (byAttackType == BATTLE_ATTACK_TYPE_PHYSICAL) return fDmg * fPhysicalReflect / 100.0f;
    if (byAttackType == BATTLE_ATTACK_TYPE_ENERGY)   return fDmg * fEnergyReflect / 100.0f;
    return 0.0f;
}

float GetSkillReflectDamage(float fDmg, BYTE bySkillType, float fPhysicalReflect, float fEnergyReflect)
{
    if (bySkillType == NTL_SKILL_TYPE_PHYSICAL) return fDmg * fPhysicalReflect / 100.0f;
    if (bySkillType == NTL_SKILL_TYPE_ENERGY)   return fDmg * fEnergyReflect / 100.0f;
    return 0.0f;
}

// =======================================================
// Attributes
// =======================================================

float GetAttributeBonusRate(bool /*bIsPc*/, bool /*bSubWeapon*/, BYTE byOffence, BYTE byDefence, BYTE /*bySubOffence*/,
    sAVATAR_ATTRIBUTE& sOffenceAttribute, sAVATAR_ATTRIBUTE& sDefenceAttribute)
{
    float fAttributeBonusRate = NtlGetBattleAttributeBonusRate(byOffence, byDefence);

    switch (byOffence)
    {
    case BATTLE_ATTRIBUTE_HONEST:   fAttributeBonusRate += sOffenceAttribute.fHonestOffense - sDefenceAttribute.fHonestDefense;   break;
    case BATTLE_ATTRIBUTE_STRANGE:  fAttributeBonusRate += sOffenceAttribute.fStrangeOffense - sDefenceAttribute.fStrangeDefense;  break;
    case BATTLE_ATTRIBUTE_WILD:     fAttributeBonusRate += sOffenceAttribute.fWildOffense - sDefenceAttribute.fWildDefense;     break;
    case BATTLE_ATTRIBUTE_ELEGANCE: fAttributeBonusRate += sOffenceAttribute.fEleganceOffense - sDefenceAttribute.fEleganceDefense; break;
    case BATTLE_ATTRIBUTE_FUNNY:    fAttributeBonusRate += sOffenceAttribute.fFunnyOffense - sDefenceAttribute.fFunnyDefense;    break;

    default:
    {
        switch (byDefence)
        {
        case BATTLE_ATTRIBUTE_HONEST:   fAttributeBonusRate -= sDefenceAttribute.fHonestDefense;   break;
        case BATTLE_ATTRIBUTE_STRANGE:  fAttributeBonusRate -= sDefenceAttribute.fStrangeDefense;  break;
        case BATTLE_ATTRIBUTE_WILD:     fAttributeBonusRate -= sDefenceAttribute.fWildDefense;     break;
        case BATTLE_ATTRIBUTE_ELEGANCE: fAttributeBonusRate -= sDefenceAttribute.fEleganceDefense; break;
        case BATTLE_ATTRIBUTE_FUNNY:    fAttributeBonusRate -= sDefenceAttribute.fFunnyDefense;    break;
        default: break;
        }
    }
    break;
    }

    return fAttributeBonusRate;
}
