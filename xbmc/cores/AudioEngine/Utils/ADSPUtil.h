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


#include "AEChannelData.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_adsp_types.h"


class CADSPUtil
{
public:
  /*!
   * @brief Translate audio dsp channel flag to KODI channel flag
   */
  static enum AEChannel GetKODIChannel(AE_DSP_CHANNEL channel);
  
  /*!
   * @brief Translate KODI channel flag to audio dsp channel flag
   */
  static AE_DSP_CHANNEL GetDSPChannel(enum AEChannel channel);
  
  /*!
   * @brief Get name label id to given stream type id
   */
  static int GetStreamTypeName(unsigned int streamType);

private:
  /*!
   * Contains string name id's related to the AE_DSP_ASTREAM_ values
   */
  static const int m_StreamTypeNameTable[];
};
