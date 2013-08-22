/**
 ******************************************************************************
 * @file       gcscontrol.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup [Group]
 * @{
 * @addtogroup GCSControl
 * @{
 * @brief [Brief]
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "gcscontrol.h"
#include <QDebug>
#include <QtPlugin>

#define CHANNEL_MAX     2000
#define CHANNEL_NEUTRAL 1500
#define CHANNEL_MIN     1000
bool GCSControl::firstInstance = true;

GCSControl::GCSControl():hasControl(false)
{
    Q_ASSERT(firstInstance);//There should only be one instance of this class
    firstInstance = false;
}

void GCSControl::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager * objMngr = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objMngr);
    manControlSettingsUAVO = ManualControlSettings::GetInstance(objMngr);
    Q_ASSERT(manControlSettingsUAVO);
    m_gcsReceiver = GCSReceiver::GetInstance(objMngr);
    Q_ASSERT(m_gcsReceiver);
}


bool GCSControl::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);
    addObject(this);
    return true;
}

void GCSControl::shutdown()
{
}

bool GCSControl::beginGCSControl()
{
    if(hasControl)
        return false;
    dataBackup = manControlSettingsUAVO->getData();
    metaBackup = manControlSettingsUAVO->getMetadata();
    ManualControlSettings::Metadata meta = manControlSettingsUAVO->getDefaultMetadata();
    UAVObject::SetGcsAccess(meta,UAVObject::ACCESS_READWRITE);
    for(quint8 x = 0; x < ManualControlSettings::CHANNELGROUPS_NUMELEM; ++x)
    {
        manControlSettingsUAVO->setChannelGroups(x,ManualControlSettings::CHANNELGROUPS_GCS);
    }
    for(quint8 x = 0; x < ManualControlSettings::CHANNELNUMBER_NUMELEM; ++x)
    {
        manControlSettingsUAVO->setChannelNumber(x,x);
        manControlSettingsUAVO->setChannelMax(x,CHANNEL_MAX);
        manControlSettingsUAVO->setChannelNeutral(x,CHANNEL_NEUTRAL);
        manControlSettingsUAVO->setChannelMin(x,CHANNEL_MIN);
    }
    manControlSettingsUAVO->setDeadband(0);
    manControlSettingsUAVO->setFlightModeNumber(1);
    manControlSettingsUAVO->setFlightModePosition(0,ManualControlSettings::FLIGHTMODEPOSITION_STABILIZED1);
    manControlSettingsUAVO->updated();
    connect(manControlSettingsUAVO,SIGNAL(objectUpdated(UAVObject*)),this,SLOT(objectsUpdated(UAVObject*)));
    hasControl = true;
    return true;
}

bool GCSControl::endGCSControl()
{
    if(!hasControl)
        return false;
    disconnect(manControlSettingsUAVO,SIGNAL(objectUpdated(UAVObject*)),this,SLOT(objectsUpdated(UAVObject*)));
    manControlSettingsUAVO->setData(dataBackup);
    manControlSettingsUAVO->setMetadata(metaBackup);
    manControlSettingsUAVO->updated();
    hasControl = false;
    return true;
}

bool GCSControl::setFlightMode(ManualControlSettings::FlightModePositionOptions flightMode)
{
    if(!hasControl)
        return false;
    manControlSettingsUAVO->setFlightModePosition(0,flightMode);
    manControlSettingsUAVO->updated();
    m_gcsReceiver->setChannel(ManualControlSettings::CHANNELGROUPS_FLIGHTMODE,CHANNEL_MIN);
    m_gcsReceiver->updated();
    return true;
}

bool GCSControl::setThrottle(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_THROTTLE,value);
}

bool GCSControl::setRoll(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_ROLL,value);
}

bool GCSControl::setPitch(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_PITCH,value);
}

bool GCSControl::setYaw(float value)
{
    return setChannel(ManualControlSettings::CHANNELGROUPS_YAW,value);
}

bool GCSControl::setChannel(quint8 channel, float value)
{
    if(value > 1 || value < -1 || channel > GCSReceiver::CHANNEL_NUMELEM || !hasControl)
        return false;
    quint16 pwmValue;
    if(value >= 0)
        pwmValue = (value * (float)(CHANNEL_MAX - CHANNEL_NEUTRAL)) + (float)CHANNEL_NEUTRAL;
    else
        pwmValue = (value * (float)(CHANNEL_NEUTRAL - CHANNEL_MIN)) + (float)CHANNEL_NEUTRAL;
    m_gcsReceiver->setChannel(channel,pwmValue);
    m_gcsReceiver->updated();
    return true;
}

void GCSControl::objectsUpdated(UAVObject *obj)
{
    qDebug()<<__PRETTY_FUNCTION__<<"Object"<<obj->getName()<<"changed outside this class";
}

Q_EXPORT_PLUGIN(GCSControl)

