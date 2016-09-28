#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon.h"
#include "DllAddon.h"
#include "AddonManager.h"
#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_addon_types.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/Directory.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Util.h"

class DllAddon;

namespace ADDON
{

  class CAddonDll : public CAddon
  {
  public:
    CAddonDll(AddonProps props);

    //FIXME: does shallow pointer copy. no copy assignment op
    CAddonDll(const CAddonDll &rhs);
    virtual ~CAddonDll();
    virtual ADDON_STATUS GetStatus();

    // addon settings
    virtual void SaveSettings();
    virtual std::string GetSetting(const std::string& key);

    ADDON_STATUS Create();
    virtual void Stop();
    virtual bool CheckAPIVersion(void) { return true; }
    void Destroy();

    bool DllLoaded(void) const;

    bool SupportMultipleInstances(int instanceType, const char* instanceID);
    ADDON_STATUS CreateInstance(int instanceType, const char* instanceID, void* instance, void** addonInstance);
    void DestroyInstance(int instanceType, void* instance);

  protected:
    void HandleException(std::exception &e, const char* context);
    bool Initialized() { return m_initialized; }
    virtual bool LoadSettings();
    static uint32_t GetChildCount() { static uint32_t childCounter = 0; return childCounter++; }
    CAddonInterfaces* m_pHelpers;
    sFuncTable_Addon m_interface;
    bool m_bIsChild;
    std::string m_parentLib;

  private:
    DllAddon* m_pDll;
    bool m_initialized;
    bool LoadDll();
    bool m_needsavedsettings;

    virtual ADDON_STATUS TransferSettings();
    TiXmlElement MakeSetting(kodi::addon::CAddonSetting& setting) const;

    static void AddOnStatusCallback(void *userData, const ADDON_STATUS status, const char* msg);
    static bool AddOnGetSetting(void *userData, const char *settingName, void *settingValue);
    static void AddOnOpenSettings(const char *url, bool bReload);
    static void AddOnOpenOwnSettings(void *userData, bool bReload);
    static const char* AddOnGetAddonDirectory(void *userData);
    static const char* AddOnGetUserDirectory(void *userData);
  };

}; /* namespace ADDON */

