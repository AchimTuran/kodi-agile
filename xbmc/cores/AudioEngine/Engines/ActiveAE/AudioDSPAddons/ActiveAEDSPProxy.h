#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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


#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"


namespace ActiveAE
{
class CActiveAEDSPAddon;
class CActiveAEBufferPoolADSP;
class CActiveAEDSPMode;
class CActiveAEDSPProcess;

class CActiveAEDSPProxy : public CActiveAEDSP
{
  friend class CActiveAEDSPAddon;
  friend class CActiveAEBufferPoolADSP;
  friend class CActiveAEDSPMode;
  friend class CActiveAEDSPProcess;

protected:
  inline int CreateAudioDSPProcessor(int streamId, CActiveAEDSPProcessPtr &process, const AEAudioFormat &inputFormat,
                                     const AEAudioFormat &outputFormat, bool upmix, bool bypassDSP, AEQuality quality,
                                     AVMatrixEncoding matrix_encoding, AVAudioServiceType audio_service_type, int profile)
  {
    return CActiveAEDSP::CreateDSPs(streamId, process, inputFormat,
                                    outputFormat, upmix, bypassDSP, quality,
                                    matrix_encoding, audio_service_type, profile);
  }

  inline void DestroyAudioDSPProcessor(int streamId)
  {
    CActiveAEDSP::DestroyDSPs(streamId);
  }

  inline const AE_DSP_MODELIST GetAvailableModes(AE_DSP_MODE_TYPE modeType)
  {
    return CActiveAEDSP::GetAvailableModes(modeType);
  }

  inline void UpdateAddons()
  {
    CActiveAEDSP::UpdateAddons();
  }

  inline CActiveAEDSPDatabase* GetADSPDatabase(void)
  {
    return CActiveAEDSP::GetADSPDatabase();
  }
};
}
