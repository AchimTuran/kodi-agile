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


#include "ADSPUtil.h"

const int CADSPUtil::m_StreamTypeNameTable[] =
{
  15004,  //!< "Basic"
  249,    //!< "Music"
  157,    //!< "Video"
  15016,  //!< "Games"
  15005,  //!< "Application"
  15006,  //!< "Phone"
  15007,  //!< "Message"
  14061   //!< "Auto"
};

enum AEChannel CADSPUtil::GetKODIChannel(AE_DSP_CHANNEL channel)
{
  switch (channel)
  {
    case AE_DSP_CH_FL:   return AE_CH_FL;
    case AE_DSP_CH_FR:   return AE_CH_FR;
    case AE_DSP_CH_FC:   return AE_CH_FC;
    case AE_DSP_CH_LFE:  return AE_CH_LFE;
    case AE_DSP_CH_BL:   return AE_CH_BL;
    case AE_DSP_CH_BR:   return AE_CH_BR;
    case AE_DSP_CH_FLOC: return AE_CH_FLOC;
    case AE_DSP_CH_FROC: return AE_CH_FROC;
    case AE_DSP_CH_BC:   return AE_CH_BC;
    case AE_DSP_CH_SL:   return AE_CH_SL;
    case AE_DSP_CH_SR:   return AE_CH_SR;
    case AE_DSP_CH_TC:   return AE_CH_TC;
    case AE_DSP_CH_TFL:  return AE_CH_TFL;
    case AE_DSP_CH_TFC:  return AE_CH_TFC;
    case AE_DSP_CH_TFR:  return AE_CH_TFR;
    case AE_DSP_CH_TBL:  return AE_CH_TBL;
    case AE_DSP_CH_TBC:  return AE_CH_TBC;
    case AE_DSP_CH_TBR:  return AE_CH_TBR;
    
    default:             return AE_CH_NULL;
  }
}

AE_DSP_CHANNEL CADSPUtil::GetDSPChannel(enum AEChannel channel)
{
  switch (channel)
  {
    case AE_CH_FL:   return AE_DSP_CH_FL;
    case AE_CH_FR:   return AE_DSP_CH_FR;
    case AE_CH_FC:   return AE_DSP_CH_FC;
    case AE_CH_LFE:  return AE_DSP_CH_LFE;
    case AE_CH_BL:   return AE_DSP_CH_BL;
    case AE_CH_BR:   return AE_DSP_CH_BR;
    case AE_CH_FLOC: return AE_DSP_CH_FLOC;
    case AE_CH_FROC: return AE_DSP_CH_FROC;
    case AE_CH_BC:   return AE_DSP_CH_BC;
    case AE_CH_SL:   return AE_DSP_CH_SL;
    case AE_CH_SR:   return AE_DSP_CH_SR;
    case AE_CH_TC:   return AE_DSP_CH_TC;
    case AE_CH_TFL:  return AE_DSP_CH_TFL;
    case AE_CH_TFC:  return AE_DSP_CH_TFC;
    case AE_CH_TFR:  return AE_DSP_CH_TFR;
    case AE_CH_TBL:  return AE_DSP_CH_TBL;
    case AE_CH_TBC:  return AE_DSP_CH_TBC;
    case AE_CH_TBR:  return AE_DSP_CH_TBR;
    
    default:         return AE_DSP_CH_INVALID;
  }
}

int CADSPUtil::GetStreamTypeName(unsigned int streamType)
{
  if (streamType > AE_DSP_ASTREAM_AUTO)
    return -1;
  return m_StreamTypeNameTable[streamType];
}
