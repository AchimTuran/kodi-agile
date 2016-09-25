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

#include "Visualisation.h"

#include "Application.h"
#include "GUIInfoManager.h"
#include "addons/interfaces/ExceptionHandling.h"
#include "cores/AudioEngine/AEFactory.h"
#include "guiinfo/GUIInfoLabels.h"
#include "guilib/GraphicContext.h"
#include "guilib/WindowIDs.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "windowing/WindowingFactory.h"

using namespace MUSIC_INFO;
using namespace ADDON;

CAudioBuffer::CAudioBuffer(int iSize)
{
  m_iLen = iSize;
  m_pBuffer = new float[iSize];
}

CAudioBuffer::~CAudioBuffer()
{
  delete [] m_pBuffer;
}

const float* CAudioBuffer::Get() const
{
  return m_pBuffer;
}

void CAudioBuffer::Set(const float* psBuffer, int iSize)
{
  if (iSize < 0)
    return;

  memcpy(m_pBuffer, psBuffer, iSize * sizeof(float));
  for (int i = iSize; i < m_iLen; ++i)
    m_pBuffer[i] = 0;
}

CVisualisation::CVisualisation(AddonProps props)
  : CAddonDll(std::move(props)),
    m_addonInstance(nullptr)
{
  memset(&m_struct, 0, sizeof(m_struct));
}
    
bool CVisualisation::Create(int x, int y, int w, int h, void *device)
{
  if (CAddonDll::Create() != ADDON_STATUS_OK)
    return false;

#ifdef HAS_DX
  m_struct.props.device = g_Windowing.Get3D11Context();
#else
  m_struct.props.device = nullptr;
#endif
  m_struct.props.x = x;
  m_struct.props.y = y;
  m_struct.props.width = w;
  m_struct.props.height = h;
  m_struct.props.pixelRatio = g_graphicsContext.GetResInfo().fPixelRatio;
  m_struct.props.name = strdup(Name().c_str());
  m_struct.props.presets = strdup(CSpecialProtocol::TranslatePath(Path()).c_str());
  m_struct.props.profile = strdup(CSpecialProtocol::TranslatePath(Profile()).c_str());
  m_struct.props.submodule = nullptr;

  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.TransferPreset = TransferPreset;
  m_struct.toKodi.TransferSubmodule = TransferSubmodule;

  ADDON_STATUS status = CAddonDll::CreateInstance(ADDON_INSTANCE_VISUALIZATION, ID().c_str(), &m_struct, &m_addonInstance);
  if (status != ADDON_STATUS_OK)
    return false;
  
  // Start the visualisation
  std::string strFile = URIUtils::GetFileName(g_application.CurrentFile());
  CLog::Log(LOGDEBUG, "Visualisation::Start()\n");
  try
  {
    m_struct.toAddon.Start(m_addonInstance, m_iChannels, m_iSamplesPerSec, m_iBitsPerSample, strFile.c_str());
  }
  catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); return false; }

  m_hasPresets = GetPresets();

  if (GetSubModules())
    m_struct.props.submodule = strdup(CSpecialProtocol::TranslatePath(m_submodules.front()).c_str());
  else
    m_struct.props.submodule = nullptr;

  CreateBuffers();

  CAEFactory::RegisterAudioCallback(this);

  return true;
}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const std::string &strSongName)
{
  // notify visz. that new song has been started
  // pass it the nr of audio channels, sample rate, bits/sample and offcourse the songname
  if (Initialized())
  {
    try
    {
      if (m_struct.toAddon.Start)
        m_struct.toAddon.Start(m_addonInstance, iChannels, iSamplesPerSec, iBitsPerSample, strSongName.c_str());
    }
    catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); }
  }
}

void CVisualisation::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  if (Initialized())
  {
    try
    {
      if (m_struct.toAddon.AudioData)
        m_struct.toAddon.AudioData(m_addonInstance, pAudioData, iAudioDataLength, pFreqData, iFreqDataLength);
    }
    catch (std::exception ex)
    {
      ADDON::LogException(this, ex, __FUNCTION__); // Handle exception
      memset(&m_struct, 0, sizeof(m_struct)); // reset function table to prevent further exception call
    }
  }
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  if (Initialized())
  {
    try
    {
      if (m_struct.toAddon.Render)
        m_struct.toAddon.Render(m_addonInstance);
    }
    catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); }
  }
}

void CVisualisation::Stop()
{
  CAEFactory::UnregisterAudioCallback(this);
  if (Initialized())
  {
    CAddonDll::Stop();
  }
}

void CVisualisation::GetInfo(VIS_INFO *info)
{
  if (Initialized())
  {
    try
    {
      if (m_struct.toAddon.GetInfo)
        m_struct.toAddon.GetInfo(m_addonInstance, info);
    }
    catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); }
  }
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param)
{
  if (!Initialized())
    return false;

  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  try
  {
    if (action != VIS_ACTION_NONE && m_struct.toAddon.OnAction)
    {
      // if this is a VIS_ACTION_UPDATE_TRACK action, copy relevant
      // tags from CMusicInfoTag to VisTag
      if ( action == VIS_ACTION_UPDATE_TRACK && param )
      {
        const CMusicInfoTag* tag = (const CMusicInfoTag*)param;
        std::string artist(tag->GetArtistString());
        std::string albumArtist(tag->GetAlbumArtistString());
        std::string genre(StringUtils::Join(tag->GetGenre(), g_advancedSettings.m_musicItemSeparator));
        
        kodi::addon::visualization::VisTrack track;
        track.title       = tag->GetTitle().c_str();
        track.artist      = artist.c_str();
        track.album       = tag->GetAlbum().c_str();
        track.albumArtist = albumArtist.c_str();
        track.genre       = genre.c_str();
        track.comment     = tag->GetComment().c_str();
        track.lyrics      = tag->GetLyrics().c_str();
        track.trackNumber = tag->GetTrackNumber();
        track.discNumber  = tag->GetDiscNumber();
        track.duration    = tag->GetDuration();
        track.year        = tag->GetYear();
        track.rating      = tag->GetUserrating();

        return m_struct.toAddon.OnAction(m_addonInstance, action, &track);
      }
      return m_struct.toAddon.OnAction(m_addonInstance, action, param);
    }
  }
  catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); }

  return false;
}

void CVisualisation::OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
  CLog::Log(LOGDEBUG, "OnInitialize() started");

  m_iChannels = iChannels;
  m_iSamplesPerSec = iSamplesPerSec;
  m_iBitsPerSample = iBitsPerSample;
  UpdateTrack();

  CLog::Log(LOGDEBUG, "OnInitialize() done");
}

void CVisualisation::OnAudioData(const float* pAudioData, int iAudioDataLength)
{
  // FIXME: iAudioDataLength should never be less than 0
  if (iAudioDataLength < 0)
    return;

  // Save our audio data in the buffers
  std::unique_ptr<CAudioBuffer> pBuffer ( new CAudioBuffer(iAudioDataLength) );
  pBuffer->Set(pAudioData, iAudioDataLength);
  m_vecBuffers.push_back( pBuffer.release() );

  if ((int)m_vecBuffers.size() < m_iNumBuffers)
    return;

  std::unique_ptr<CAudioBuffer> ptrAudioBuffer ( m_vecBuffers.front() );
  m_vecBuffers.pop_front();
  // Fourier transform the data if the vis wants it...
  if (m_bWantsFreq)
  {
    const float *psAudioData = ptrAudioBuffer->Get();

    if (!m_transform)
      m_transform.reset(new RFFT(AUDIO_BUFFER_SIZE/2, false)); // half due to stereo

    m_transform->calc(psAudioData, m_fFreq);

    // Transfer data to our visualisation
    AudioData(psAudioData, iAudioDataLength, m_fFreq, AUDIO_BUFFER_SIZE/2); // half due to complex-conjugate
  }
  else
  { // Transfer data to our visualisation
    AudioData(ptrAudioBuffer->Get(), iAudioDataLength, NULL, 0);
  }
}

void CVisualisation::CreateBuffers()
{
  ClearBuffers();

  VIS_INFO info;

  try
  {
    // Get the number of buffers from the current vis
    if (m_struct.toAddon.GetInfo)
      m_struct.toAddon.GetInfo(m_addonInstance, &info);
  }
  catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); return; }

  m_iNumBuffers = info.iSyncDelay + 1;
  m_bWantsFreq = (info.bWantsFreq != 0);
  if (m_iNumBuffers > MAX_AUDIO_BUFFERS)
    m_iNumBuffers = MAX_AUDIO_BUFFERS;
  if (m_iNumBuffers < 1)
    m_iNumBuffers = 1;
}

void CVisualisation::ClearBuffers()
{
  m_bWantsFreq = false;
  m_iNumBuffers = 0;

  while (!m_vecBuffers.empty())
  {
    CAudioBuffer* pAudioBuffer = m_vecBuffers.front();
    delete pAudioBuffer;
    m_vecBuffers.pop_front();
  }
  for (int j = 0; j < AUDIO_BUFFER_SIZE; j++)
  {
    m_fFreq[j] = 0.0f;
  }
}

bool CVisualisation::UpdateTrack()
{
  bool handled = false;
  if (Initialized())
  {
    // get the current album art filename
    m_AlbumThumb = CSpecialProtocol::TranslatePath(g_infoManager.GetImage(MUSICPLAYER_COVER, WINDOW_INVALID));

    // get the current track tag
    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();

    if (m_AlbumThumb == "DefaultAlbumCover.png")
      m_AlbumThumb = "";
    else
      CLog::Log(LOGDEBUG,"Updating visualisation albumart: %s", m_AlbumThumb.c_str());

    // inform the visualisation of the current album art
    if (OnAction( VIS_ACTION_UPDATE_ALBUMART, (void*)( m_AlbumThumb.c_str() ) ) )
      handled = true;

    // inform the visualisation of the current track's tag information
    if ( tag && OnAction( VIS_ACTION_UPDATE_TRACK, (void*)tag ) )
      handled = true;
  }
  return handled;
}

bool CVisualisation::GetPresetList(std::vector<std::string> &vecpresets)
{
  vecpresets = m_presets;
  return !m_presets.empty();
}

bool CVisualisation::GetPresets()
{
  m_presets.clear();
  try
  {
    if (m_struct.toAddon.GetPresets)
      m_struct.toAddon.GetPresets(m_addonInstance);
    // Note: m_presets becomes filled up with callback function TransferPreset
  }
  catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); return false; }

  return (!m_presets.empty());
}

void CVisualisation::TransferPreset(void* kodiInstance, const char* preset)
{
  CVisualisation *addon = static_cast<CVisualisation*>(kodiInstance);
  if (!addon)
    throw std::logic_error("Visualization - TransferPreset - invalid handler data");

  addon->m_presets.push_back(preset);
}

bool CVisualisation::GetSubModuleList(std::vector<std::string> &vecmodules)
{
  vecmodules = m_submodules;
  return !m_submodules.empty();
}

bool CVisualisation::GetSubModules()
{
  m_submodules.clear();
  try
  {
    if (m_struct.toAddon.GetSubModules)
      m_struct.toAddon.GetSubModules(m_addonInstance);
    // Note: m_submodules becomes filled up with callback function TransferSubmodule
  }
  catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); return false; }

  return (!m_submodules.empty());
}

void CVisualisation::TransferSubmodule(void* kodiInstance, const char* submodule)
{
  CVisualisation *addon = static_cast<CVisualisation*>(kodiInstance);
  if (!addon)
    throw std::logic_error("Visualization - TransferSubmodule - invalid handler data");

  addon->m_submodules.push_back(submodule);
}

std::string CVisualisation::GetFriendlyName(const std::string& strVisz,
                                            const std::string& strSubModule)
{
  // should be of the format "moduleName (visName)"
  return strSubModule + " (" + strVisz + ")";
}

bool CVisualisation::IsLocked()
{
  if (!m_presets.empty())
  {
    try
    {
      if (m_struct.toAddon.IsLocked)
        return m_struct.toAddon.IsLocked(m_addonInstance);
    }
    catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); }
  }
  return false;
}

void CVisualisation::Destroy()
{
  // Free what was allocated in method CVisualisation::Create
  CAddonDll::DestroyInstance(ADDON_INSTANCE_VISUALIZATION, m_addonInstance);
  m_addonInstance = nullptr;

  if (m_struct.props.name)
    free((void *) m_struct.props.name);
  if (m_struct.props.presets)
    free((void *) m_struct.props.presets);
  if (m_struct.props.profile)
    free((void *) m_struct.props.profile);

  memset(&m_struct, 0, sizeof(m_struct));

  CAddonDll::Destroy();
}

unsigned CVisualisation::GetPreset()
{
  try
  {
    if (m_struct.toAddon.GetPreset)
      return m_struct.toAddon.GetPreset(m_addonInstance);
  }
  catch (std::exception& ex) { ExceptionHandle(ex, __FUNCTION__); }

  return 0;
}

std::string CVisualisation::GetPresetName()
{
  if (!m_presets.empty())
    return m_presets[GetPreset()];
  else
    return "";
}

bool CVisualisation::IsInUse() const
{
  return CSettings::GetInstance().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION) == ID();
}

void CVisualisation::ExceptionHandle(std::exception& ex, const char* function)
{
  ADDON::LogException(this, ex, function); // Handle exception
  memset(&m_struct.toAddon, 0, sizeof(m_struct.toAddon)); // reset function table to prevent further exception call  
}
