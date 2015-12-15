/*
 * hvacplugin.h - AMB plugin to conttrol HVAC demo hardware for CES 2016
 *
 * Copyright (C) 2013-2015 K2L GmbH & Co. KG
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This file is licensed under GPLv2.
 */

#ifndef _HvacPlugin_H_
#define _HvacPlugin_H_

/*!
 *  \defgroup hvacplugin hvacplugin shared library
 *  \brief HVAC plug-in from K2L.
 *
 *  To load this plugin at AMB starup, insert following rows into AMB configuration file:
 *  \code
 *      {
 *          "name" : "HvacPlugin",
 *          "path":"/usr/lib/automotive-message-broker/hvacplugin.so"
 *      }
 *  \endcode
 *
 * \note HvacPlugin has to be the last source plug-in listed in AMB configuration file. Otherwise it can accidentally unregister or try to simulate some AMB properties supported from other sources.
 *
 *  @{
 */

#include <abstractsource.h>
#include <string>

using namespace std;

class K2LHvacPlugin: public AbstractSource
{
public:
    K2LHvacPlugin(): AbstractSource(nullptr, map<string, string>()) {}
    K2LHvacPlugin(AbstractRoutingEngine* re, const map<string, string>& config);
    virtual ~K2LHvacPlugin();

public:
    const string uuid() { return "4B029567-A667-4F6B-8F23-44D6E49DBF1E"; }

    void getPropertyAsync(AsyncPropertyReply *reply);
    void getRangePropertyAsync(AsyncRangePropertyReply *reply) {}
    AsyncPropertyReply * setProperty(AsyncSetPropertyRequest request);
    void subscribeToPropertyChanges(VehicleProperty::Property property);
    void unsubscribeToPropertyChanges(VehicleProperty::Property property);
    PropertyList supported();
    int supportedOperations();
    void supportedChanged(const PropertyList &) {}

    PropertyInfo getPropertyInfo(const VehicleProperty::Property & property)
    {
            if(propertyInfoMap.find(property) != propertyInfoMap.end())
                    return propertyInfoMap[property];

            return PropertyInfo::invalid();
    }
private:  
    void addPropertySupport(VehicleProperty::Property property, Zone::Type zone);
    uint8_t GetTemperature(uint8_t value);
    void printFrame(const can_frame& frame) const;
    
    std::map<VehicleProperty::Property, PropertyInfo> propertyInfoMap;
    std::map<Zone::Type, uint8_t> temperatureValues;
    uint8_t fanSpeedValue;
    
    PropertyList mRequests;
    PropertyList mSupported;
   
    VehicleProperty::TargetTemperatureType targetTemperature;
    VehicleProperty::FanSpeedType fanSpeed;
    
    int socketId = -1;
    struct can_frame txCanFrame;
    struct sockaddr_can txAddress;    
};
#endif // _CANSIMPLUGINIMPL_H_

/** @} */
