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

#include <math.h>
#include <string>
#include <vector>

#include "AddonDll.h"

#include "AddonStatusHandler.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "utils/XMLUtils.h"

namespace ADDON
{

CAddonDll::CAddonDll(AddonProps props)
  : CAddon(std::move(props)),
    m_bIsChild(false)
{
  m_initialized = false;
  m_pDll        = NULL;
  m_needsavedsettings = false;
  m_parentLib.clear();
  memset(&m_interface, 0, sizeof(m_interface));
}

CAddonDll::CAddonDll(const CAddonDll &rhs)
  : CAddon(rhs),
    m_bIsChild(true)
{
  m_initialized       = rhs.m_initialized;
  m_pDll              = rhs.m_pDll;
  m_needsavedsettings = rhs.m_needsavedsettings;
  m_parentLib = rhs.m_parentLib;
  memset(&m_interface, 0, sizeof(m_interface));
}

CAddonDll::~CAddonDll()
{
  if (m_initialized)
    Destroy();
}

bool CAddonDll::LoadDll()
{
  if (m_pDll)
    return true;

  std::string strFileName;
  std::string strAltFileName;
  if (!m_bIsChild)
  {
    strFileName = LibPath();
  }
  else
  {
    std::string libPath = LibPath();
    if (!XFILE::CFile::Exists(libPath))
    {
      std::string temp = CSpecialProtocol::TranslatePath("special://xbmc/");
      std::string tempbin = CSpecialProtocol::TranslatePath("special://xbmcbin/");
      libPath.erase(0, temp.size());
      libPath = tempbin + libPath;
      if (!XFILE::CFile::Exists(libPath))
      {
        CLog::Log(LOGERROR, "ADDON: Could not locate %s", m_props.libname.c_str());
        return false;
      }
    }

    std::stringstream childcount;
    childcount << GetChildCount();
    std::string extension = URIUtils::GetExtension(libPath);
    strFileName = "special://temp/" + ID() + "-" + childcount.str() + extension;

    XFILE::CFile::Copy(libPath, strFileName);

    m_parentLib = libPath;
    CLog::Log(LOGNOTICE, "ADDON: Loaded virtual child addon %s", strFileName.c_str());
  }

  /* Check if lib being loaded exists, else check in XBMC binary location */
#if defined(TARGET_ANDROID)
  // Android libs MUST live in this path, else multi-arch will break.
  // The usual soname requirements apply. no subdirs, and filename is ^lib.*\.so$
  if (!XFILE::CFile::Exists(strFileName))
  {
    std::string tempbin = getenv("XBMC_ANDROID_LIBS");
    strFileName = tempbin + "/" + m_props.libname;
  }
#endif
  if (!XFILE::CFile::Exists(strFileName))
  {
    std::string altbin = CSpecialProtocol::TranslatePath("special://xbmcaltbinaddons/");
    if (!altbin.empty())
    {
      strAltFileName = altbin + m_props.libname;
      if (!XFILE::CFile::Exists(strAltFileName))
      {
        std::string temp = CSpecialProtocol::TranslatePath("special://xbmc/addons/");
        strAltFileName = strFileName;
        strAltFileName.erase(0, temp.size());
        strAltFileName = altbin + strAltFileName;
      }
      CLog::Log(LOGDEBUG, "ADDON: Trying to load %s", strAltFileName.c_str());
    }

    if (XFILE::CFile::Exists(strAltFileName))
      strFileName = strAltFileName;
    else
    {
      std::string temp = CSpecialProtocol::TranslatePath("special://xbmc/");
      std::string tempbin = CSpecialProtocol::TranslatePath("special://xbmcbin/");
      strFileName.erase(0, temp.size());
      strFileName = tempbin + strFileName;
      if (!XFILE::CFile::Exists(strFileName))
      {
        CLog::Log(LOGERROR, "ADDON: Could not locate %s", m_props.libname.c_str());
        return false;
      }
    }
  }

  /* Load the Dll */
  m_pDll = new DllAddon;
  m_pDll->SetFile(strFileName);
  m_pDll->EnableDelayedUnload(false);
  if (!m_pDll->Load())
  {
    delete m_pDll;
    m_pDll = NULL;

    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (pDialog)
    {
      std::string heading = StringUtils::Format("%s: %s", TranslateType(Type(), true).c_str(), Name().c_str());
      pDialog->SetHeading(CVariant{heading});
      pDialog->SetLine(1, CVariant{24070});
      pDialog->SetLine(2, CVariant{24071});
      pDialog->SetLine(2, CVariant{"Can't load shared library"});
      pDialog->Open();
    }

    return false;
  }

  return true;
}

ADDON_STATUS CAddonDll::Create()
{
  /* ensure that a previous instance is destroyed */
  Destroy();

  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
  CLog::Log(LOGDEBUG, "ADDON: Dll Initializing - %s", Name().c_str());
  m_initialized = false;

  if (!LoadDll())
    return ADDON_STATUS_PERMANENT_FAILURE;

  /* Allocate the helper function class to allow crosstalk over
     helper libraries */
  memset(&m_interface, 0, sizeof(m_interface));
  m_interface.toKodi.kodiInstance = this;
  m_interface.toKodi.Log = addon_log_msg;

  /* Call Create to make connections, initializing data or whatever is
     needed to become the AddOn running */
  try
  {
    status = m_pDll->Create(&m_interface);
    if (status == ADDON_STATUS_OK)
    {
      m_initialized = true;
    }
    else if ((status == ADDON_STATUS_NEED_SETTINGS) || (status == ADDON_STATUS_NEED_SAVEDSETTINGS))
    {
      m_needsavedsettings = (status == ADDON_STATUS_NEED_SAVEDSETTINGS);
      if ((status = TransferSettings()) == ADDON_STATUS_OK)
        m_initialized = true;
      else
        new CAddonStatusHandler(ID(), status, "", false);
    }
    else
    { // Addon failed initialization
      CLog::Log(LOGERROR, "ADDON: Dll %s - Client returned bad status (%i) from Create and is not usable", Name().c_str(), status);
      
      CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDialog)
      {
        std::string heading = StringUtils::Format("%s: %s", TranslateType(Type(), true).c_str(), Name().c_str());
        pDialog->SetHeading(CVariant{heading});
        pDialog->SetLine(1, CVariant{24070});
        pDialog->SetLine(2, CVariant{24071});
        pDialog->Open();
      }
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Create");
  }

  return status;
}

void CAddonDll::Stop()
{
  /* Inform dll to stop all activities */
  try
  {
    if (m_needsavedsettings)  // If the addon supports it we save some settings to settings.xml before stop
    {
      char   str_id[64] = "";
      char   str_value[1024];
      CAddon::LoadUserSettings();
      for (unsigned int i=0; (strcmp(str_id,"###End") != 0); i++)
      {
        strcpy(str_id, "###GetSavedSettings");
        sprintf (str_value, "%i", i);
        ADDON_STATUS status = ADDON_STATUS_UNKNOWN;
        if (m_interface.toAddon.SetSetting)
          status = m_interface.toAddon.SetSetting((const char*)&str_id, (void*)&str_value);
        if (status == ADDON_STATUS_UNKNOWN)
          break;

        if (strcmp(str_id,"###End") != 0) UpdateSetting(str_id, str_value);
      }
      CAddon::SaveSettings();
    }
    if (m_interface.toAddon.Stop)
    {
      m_interface.toAddon.Stop();
      CLog::Log(LOGINFO, "ADDON: Dll Stopped - %s", Name().c_str());
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_interface.toAddon.Stop");
  }
}

void CAddonDll::Destroy()
{
  /* Unload library file */
  try
  {      
    if (m_pDll)
    {
      if (m_interface.toAddon.Destroy)
        m_interface.toAddon.Destroy();

      m_pDll->Unload();
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_pDll->Unload");
  }

  if (m_pDll)
  {
    if (m_bIsChild)
      XFILE::CFile::Delete(m_pDll->GetFile());
    delete m_pDll;
    m_pDll = NULL;
    CLog::Log(LOGINFO, "ADDON: Dll Destroyed - %s", Name().c_str());
  }
  m_initialized = false;
}

bool CAddonDll::DllLoaded(void) const
{
  return m_pDll != NULL;
}

ADDON_STATUS CAddonDll::GetStatus()
{
  try
  {
    if (m_interface.toAddon.GetStatus)
      return m_interface.toAddon.GetStatus();
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_interface.toAddon.GetStatus()");
  }
  return ADDON_STATUS_UNKNOWN;
}

bool CAddonDll::LoadSettings()
{
  if (m_settingsLoaded)
    return true;

  if (!LoadDll())
    return false;

  ADDON_StructSetting** sSet;
  std::vector<kodi::addon::CAddonSetting> vSet;
  unsigned entries = 0;
  try
  {
    if (m_interface.toAddon.GetSettings)
    {
      entries = m_interface.toAddon.GetSettings(&sSet);
      kodi::addon::StructToVec(entries, &sSet, &vSet);
      m_interface.toAddon.FreeSettings(entries, &sSet);
    }
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_interface.toAddon.GetSettings()");
    return false;
  }

  if (vSet.size())
  {
    // regenerate XML doc
    m_addonXmlDoc.Clear();
    TiXmlElement node("settings");
    m_addonXmlDoc.InsertEndChild(node);

    for (unsigned i=0; i < entries; i++)
    {
       kodi::addon::CAddonSetting& setting = vSet[i];
       m_addonXmlDoc.RootElement()->InsertEndChild(MakeSetting(setting));
    }
    CAddon::SettingsFromXML(m_addonXmlDoc, true);
  }
  else
    return CAddon::LoadSettings();

  m_settingsLoaded = true;
  CAddon::LoadUserSettings();
  return true;
}

TiXmlElement CAddonDll::MakeSetting(kodi::addon::CAddonSetting& setting) const
{
  TiXmlElement node("setting");

  switch (setting.Type)
  {
    case kodi::addon::CAddonSetting::CHECK:
    {
      node.SetAttribute("id", setting.Id);
      node.SetAttribute("type", "bool");
      node.SetAttribute("label", setting.Label);
      break;
    }
    case kodi::addon::CAddonSetting::SPIN:
    {
      node.SetAttribute("id", setting.Id);
      node.SetAttribute("type", "enum");
      node.SetAttribute("label", setting.Label);
      std::string values;
      for (unsigned int i = 0; i < setting.Entries.size(); i++)
      {
        values.append(setting.Entries[i]);
        values.append("|");
      }
      node.SetAttribute("values", values.c_str());
      break;
    }
  default:
    break;
  }

  return node;
}

void CAddonDll::SaveSettings()
{
  // must save first, as TransferSettings() reloads saved settings!
  CAddon::SaveSettings();
  if (m_initialized)
    TransferSettings();
}

std::string CAddonDll::GetSetting(const std::string& key)
{
  return CAddon::GetSetting(key);
}

ADDON_STATUS CAddonDll::TransferSettings()
{
  bool restart = false;
  ADDON_STATUS reportStatus = ADDON_STATUS_OK;

  CLog::Log(LOGDEBUG, "Calling TransferSettings for: %s", Name().c_str());

  LoadSettings();

  const TiXmlElement *category = m_addonXmlDoc.RootElement() ? m_addonXmlDoc.RootElement()->FirstChildElement("category") : NULL;
  if (!category)
    category = m_addonXmlDoc.RootElement(); // no categories

  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      ADDON_STATUS status = ADDON_STATUS_OK;
      const char *id = setting->Attribute("id");
      const std::string type = XMLUtils::GetAttribute(setting, "type");
      const char *option = setting->Attribute("option");

      if (id && !type.empty())
      {
        if (type == "sep" || type == "lsep")
        {
          /* Don't propagate separators */
        }
        else if (type == "text"       || type == "ipaddress" ||
                 type == "video"      || type == "audio"     ||
                 type == "image"      || type == "folder"    ||
                 type == "executable" || type == "file"      ||
                 type == "action"     || type == "date"      ||
                 type == "time"       || type == "select"    ||
                 type == "addon"      || type == "labelenum" ||
                 type == "fileenum" )
        {
          status = m_interface.toAddon.SetSetting(id, (const char*) GetSetting(id).c_str());
        }
        else if (type == "enum"      || type =="integer" ||
                 type == "labelenum" || type == "rangeofnum")
        {
          int tmp = atoi(GetSetting(id).c_str());
          status = m_interface.toAddon.SetSetting(id, (int*) &tmp);
        }
        else if (type == "bool")
        {
          bool tmp = (GetSetting(id) == "true") ? true : false;
          status = m_interface.toAddon.SetSetting(id, (bool*) &tmp);
        }
        else if (type == "rangeofnum" || type == "slider" ||
                 type == "number")
        {
          float tmpf = (float)atof(GetSetting(id).c_str());
          int   tmpi;

          if (option && strcmpi(option,"int") == 0)
          {
            tmpi = (int)floor(tmpf);
            status = m_interface.toAddon.SetSetting(id, (int*) &tmpi);
          }
          else
          {
            status = m_interface.toAddon.SetSetting(id, (float*) &tmpf);
          }
        }
        else
        {
          /* Log unknowns as an error, but go ahead and transfer the string */
          CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type.c_str(), Name().c_str());
          status = m_interface.toAddon.SetSetting(id, (const char*) GetSetting(id).c_str());
        }

        if (status == ADDON_STATUS_NEED_RESTART)
          restart = true;
        else if (status != ADDON_STATUS_OK)
          reportStatus = status;
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }

  if (restart || reportStatus != ADDON_STATUS_OK)
  {
    new CAddonStatusHandler(ID(), restart ? ADDON_STATUS_NEED_RESTART : reportStatus, "", true);
  }

  return ADDON_STATUS_OK;
}

ADDON_STATUS CAddonDll::CreateInstance(int instanceType, const char* instanceID, void* instance, void** addonInstance)
{
  try
  {
    if (m_interface.toAddon.CreateInstance)
      return m_interface.toAddon.CreateInstance(instanceType, instanceID, instance, addonInstance);
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_interface.toAddon.CreateInstance()");
  }
  return ADDON_STATUS_UNKNOWN;
}

void CAddonDll::DestroyInstance(int instanceType, void* instance)
{
  try
  {
    if (m_interface.toAddon.DestroyInstance)
      return m_interface.toAddon.DestroyInstance(instanceType, instance);
  }
  catch (std::exception &e)
  {
    HandleException(e, "m_interface.toAddon.DestroyInstance()");
  }
}

void CAddonDll::HandleException(std::exception &e, const char* context)
{
  m_initialized = false;
  m_pDll->Unload();
  CLog::Log(LOGERROR, "ADDON: Dll %s, throws an exception '%s' during %s. Contact developer '%s' with bug reports", Name().c_str(), e.what(), context, Author().c_str());
}

void CAddonDll::addon_log_msg(void* kodiInstance, const int addonLogLevel, const char* strMessage)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiInstance);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "addon_log_msg(...) called with empty kodi instance pointer");
    return;
  }

  int logLevel = LOGNONE;
  switch (addonLogLevel)
  {
    case LOG_FATAL:
      logLevel = LOGFATAL;
      break;
    case LOG_SEVERE:
      logLevel = LOGSEVERE;
      break;
    case LOG_ERROR:
      logLevel = LOGERROR;
      break;
    case LOG_WARNING:
      logLevel = LOGWARNING;
      break;
    case LOG_NOTICE:
      logLevel = LOGNOTICE;
      break;
    case LOG_INFO:
      logLevel = LOGINFO;
      break;
    case LOG_DEBUG:
      logLevel = LOGDEBUG;
      break;
    default:
      break;
  }

  CLog::Log(logLevel, "AddOnLog: %s: %s", addon->Name().c_str(), strMessage);
}

}; /* namespace ADDON */

