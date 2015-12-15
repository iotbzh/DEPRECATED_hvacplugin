/*
 * hvacplugin.cpp - AMB plugin to conttrol HVAC demo hardware for CES 2016
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

#include <vehicleproperty.h>
#include <listplusplus.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>

#include <logger.h>

#include "hvacplugin.h"

static const char* DEFAULT_CAN_IF_NAME = "vcan0";

//----------------------------------------------------------------------------
// K2LHvacPlugin
//----------------------------------------------------------------------------

extern "C" void create(AbstractRoutingEngine* routingengine, std::map<std::string, std::string> config)
{
   new K2LHvacPlugin(routingengine, config);
}

K2LHvacPlugin::K2LHvacPlugin(AbstractRoutingEngine* re, const map<string, string>& config)
   : AbstractSource(re, config)
{
   printf("K2LHvacPlugin: created - 2015-12-08 10:04\n");

   addPropertySupport(VehicleProperty::TargetTemperature, Zone::FrontLeft | Zone::FrontRight);

   Zone::ZoneList tempartureZones;
   tempartureZones.push_back(Zone::FrontLeft);
   tempartureZones.push_back(Zone::FrontRight);

   temperatureValues[Zone::FrontLeft] = 21;
   temperatureValues[Zone::FrontRight] = 21;

   PropertyInfo tempartureZonesInfo(0, tempartureZones);
   propertyInfoMap[VehicleProperty::TargetTemperature] = tempartureZonesInfo;

   addPropertySupport(VehicleProperty::FanSpeed, Zone::None);

   socketId = socket(PF_CAN, SOCK_RAW, CAN_RAW);
   printf("Socket: %i\n", socketId);
   if (socketId < 0)
   {
       printf("ERROR: socket could not be created\n");
   }
   else
   {
       struct ifreq ifr;

       strcpy(ifr.ifr_name, "vcan0" );
       if(ioctl(this->socketId, SIOCGIFINDEX, &ifr) < 0)
       {
           close(socketId);
	   socketId = -1;
           printf("ERROR: ioctl failed\n");
       }
       else
       {
	       txAddress.can_family = AF_CAN;
	       txAddress.can_ifindex = ifr.ifr_ifindex;

	       if (bind(this->socketId, (struct sockaddr *)&txAddress, sizeof(txAddress)) < 0)
	       {
		   close(socketId);
		   socketId = -1;
		   printf("ERROR: bind failed\n");
	       }

	       txCanFrame.can_id = 0x30;
	       txCanFrame.can_dlc = 8;
       }
   }
}

K2LHvacPlugin::~K2LHvacPlugin()
{
   if(socketId >= 0)
   {
       close(socketId);
       printf("Socket closed\n");
   }
}

void K2LHvacPlugin::getPropertyAsync(AsyncPropertyReply *reply)
{
   printf("K2LHvacPlugin::getPropertyAsync\n");

   if(reply->property == VehicleProperty::FanSpeed)
   {
       printf("VehicleProperty::FanSpeed=OK\n");
       VehicleProperty::FanSpeedType temp(fanSpeedValue);
       reply->value = &temp;
       reply->success = true;
       reply->completed(reply);
   }
   else if(reply->property == VehicleProperty::TargetTemperature)
   {
       printf("VehicleProperty::TargetTemperature=");
       if(temperatureValues.find(reply->zoneFilter) == temperatureValues.end())
       {
           printf("AsyncPropertyReply::ZoneNotSupported\n");
           reply->success = false;
           reply->error = AsyncPropertyReply::ZoneNotSupported;
           reply->completed(reply);
       }
       else
       {
           printf("OK\n");
           VehicleProperty::TargetTemperatureType temp(temperatureValues[reply->zoneFilter]);
           reply->success = true;
           reply->value = &temp;
           reply->completed(reply);
       }
   }
}

AsyncPropertyReply *K2LHvacPlugin::setProperty(AsyncSetPropertyRequest request )
{
   printf("K2LHvacPlugin::setProperty\n");
   AsyncPropertyReply *reply = new AsyncPropertyReply(request);
   reply->success = false;
   reply->error = AsyncPropertyReply::NoError;

   if(reply->property == VehicleProperty::FanSpeed)
   {
       printf("VehicleProperty::FanSpeed=OK\n");
       fanSpeedValue = (uint8_t)reply->value->value<uint16_t>();
   }
   else if(reply->property == VehicleProperty::TargetTemperature)
   {
       printf("VehicleProperty::TargetTemperature=");
       if(temperatureValues.find(reply->zoneFilter) == temperatureValues.end())
       {
           printf("AsyncPropertyReply::ZoneNotSupported\n");
           reply->error = AsyncPropertyReply::ZoneNotSupported;
       }
       else
       {
           printf("OK\n");
           temperatureValues[reply->zoneFilter] = (uint8_t)reply->value->value<int>();
       }
   }
   else
   {
       reply->error = AsyncPropertyReply::InvalidOperation;
   }

   if(reply->error == AsyncPropertyReply::NoError)
   {
       reply->success = true;

       routingEngine->updateProperty(reply->value,uuid());

       if (socketId >= 0)
       {
           printf("Send CAN message\n");

           txCanFrame.data[0] = GetTemperature(temperatureValues[Zone::FrontLeft]);
           txCanFrame.data[1] = GetTemperature(temperatureValues[Zone::FrontRight]);
           txCanFrame.data[2] = GetTemperature((temperatureValues[Zone::FrontLeft]
                   + temperatureValues[Zone::FrontRight]) / 2);
           txCanFrame.data[3] = 0xF0;
           txCanFrame.data[4] = fanSpeedValue;
           txCanFrame.data[5] = 0x01;
           txCanFrame.data[6] = 0x00;
           txCanFrame.data[7] = 0x00;

           sendto(socketId, &txCanFrame, sizeof(struct can_frame), 0,
                   (struct sockaddr*)&txAddress, sizeof(txAddress));
       }
   }
   else
   {
       reply->success = false;
   }

   reply->completed(reply);
   return reply;
}

uint8_t K2LHvacPlugin::GetTemperature(uint8_t value)
{
   uint8_t result = ((0xF0 - 0x10) / 15) * value - 16;
   if (result < 0x10)
       result = 0x10;
   if (result > 0xF0)
       result = 0xF0;

   return result;
}

void K2LHvacPlugin::subscribeToPropertyChanges(VehicleProperty::Property property)
{
   mRequests.push_back(property);
}

PropertyList K2LHvacPlugin::supported()
{
   return mSupported;
}

void K2LHvacPlugin::unsubscribeToPropertyChanges(VehicleProperty::Property property)
{
   if(contains(mRequests,property))
       removeOne(&mRequests, property);
}

int K2LHvacPlugin::supportedOperations()
{
   return Get | Set | GetRanged;
}

void K2LHvacPlugin::addPropertySupport(VehicleProperty::Property property, Zone::Type zone)
{
   mSupported.push_back(property);

   Zone::ZoneList zones;
   zones.push_back(zone);

   PropertyInfo info(0, zones);
   propertyInfoMap[property] = info;
}
