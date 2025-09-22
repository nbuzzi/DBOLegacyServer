// SafeObjectResolve.h - lightweight helpers for guarded object handle resolution.
#ifndef __INC_DBOG_SAFE_OBJECT_RESOLVE_H__
#define __INC_DBOG_SAFE_OBJECT_RESOLVE_H__

#include "NtlServer.h"
#include "ObjectManager.h"
#include "GameObject.h"
#include "GameServer.h"
// Forward declare CCharacter to avoid direct dependency on missing Character.h
class CCharacter;

inline bool AiVerbose()
{
    CGameServer* app = (CGameServer*)g_pApp;
    return (app && app->m_config.m_bAIVerbose);
}

inline CGameObject* SafeResolveObject(HOBJECT handle, const char* ctx)
{
    if (handle == INVALID_HOBJECT)
    {
        if (AiVerbose())
            ERR_LOG(LOG_SYSTEM, "OBJ_GUARD: invalid handle ctx=%s handle=%u", ctx, handle);
        return nullptr;
    }
    CGameObject* p = g_pObjectManager->GetObject(handle);
    if (!p && AiVerbose())
        ERR_LOG(LOG_SYSTEM, "OBJ_GUARD: resolve failed ctx=%s handle=%u", ctx, handle);
    return p;
}

inline CCharacter* SafeResolveChar(HOBJECT handle, const char* ctx)
{
    CGameObject* p = SafeResolveObject(handle, ctx);
    if (!p) return nullptr;
    if (!p->IsPC() && !p->IsNPC() && !p->IsMonster() && !p->IsSummonPet() && !p->IsItemPet())
    {
        if (AiVerbose())
            ERR_LOG(LOG_SYSTEM, "OBJ_GUARD: non-character ctx=%s handle=%u type=%u", ctx, handle, p->GetObjType());
        return nullptr;
    }
    return reinterpret_cast<CCharacter*>(p); // safe after runtime type checks above
}

#define SAFE_ID(pObj) ((pObj) ? (pObj)->GetID() : (HOBJECT)INVALID_HOBJECT)

#endif // __INC_DBOG_SAFE_OBJECT_RESOLVE_H__
