#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IAddonInterface.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_addon.h"

#include <stdint.h>

namespace KODI
{
namespace MESSAGING
{
  class ThreadMessage;
}
}

namespace ADDON
{

  class CAddon;

  class CAddonInterfaces
  {
  public:
    CAddonInterfaces(CAddon* addon);
    ~CAddonInterfaces();

    AddonCB* GetCallbacks()        { return m_callbacks; }
    CAddon *GetAddon()             { return m_addon; }
    const CAddon *GetAddon() const { return m_addon; }
    /*\_________________________________________________________________________
    \*/
    static void*        AudioEngineLib_RegisterMe      (void* addonData);
    static void         AudioEngineLib_UnRegisterMe    (void* addonData, void* cbTable);
    void*               AudioEngineLib_GetHelper()    { return m_helperAudioEngine; }
    /*\__________________________________________________________________________________________
    \*/
    static void*        GUILib_RegisterMe              (void* addonData);
    static void         GUILib_UnRegisterMe            (void* addonData, void* cbTable);
    void*               GUILib_GetHelper()            { return m_helperGUI; }
    /*\_________________________________________________________________________
    \*/
    static void*        CodecLib_RegisterMe            (void* addonData);
    static void         CodecLib_UnRegisterMe          (void* addonData, void* cbTable);
    void*               GetHelperCODEC()              { return m_helperCODEC; }
    /*
     * API level independent functions for Kodi
     */
    static void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg);

  private:
    AddonCB*  m_callbacks;
    CAddon*   m_addon;

    void*     m_helperAudioEngine;
    void*     m_helperGUI;
    void*     m_helperCODEC;
  };

} /* namespace ADDON */
