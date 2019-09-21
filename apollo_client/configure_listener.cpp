#include "configure_listener.h"
#include <iostream>
#include "log.h"

std::string Listener::GetProperty()
{
    return _property;
}

std::string Listener::GetReleaseKey(){
    std::string res;
    if (_property.empty()) return res;

    Poco::JSON::Parser parser;
    Poco::Dynamic::Var json = parser.parse(_property);
    Poco::JSON::Object::Ptr pObj = json.extract<Poco::JSON::Object::Ptr>();

    if(pObj->has("releaseKey"))
        res = pObj->getValue<std::string>("releaseKey");

    return res;
}

bool Listener::HasUpdated()
{
    return _hasUpdated;
}

void Listener::SetHasUpdated(bool hasUpdated)
{
    _hasUpdated = hasUpdated;
}

std::string Listener::ParsePropertyStr(std::string item, std::string default_ /* = "" */)
{
    std::string re;
    if (_property.empty())
    {
        return default_;
    }
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var json = parser.parse(_property);
    Poco::JSON::Object::Ptr pObj = json.extract<Poco::JSON::Object::Ptr>();

    Poco::Dynamic::Var subJson = pObj->get("configurations");
    if (subJson.isEmpty())
    {
        return default_;
    }
    Poco::JSON::Object::Ptr subObj = subJson.extract<Poco::JSON::Object::Ptr>();
    Poco::Dynamic::Var tempVar = subObj->get(item);
    if (tempVar.isEmpty())
    {
        return default_;
    }
    re = tempVar.toString();
    return re;
}

int64_t Listener::ParsePropertyInt(std::string item, int default_ /* = 0 */)
{
    std::string str = ParsePropertyStr(item);
    if (str == "")
    {
        return default_;
    }
    return Poco::NumberParser::parse64(str);
}

uint32_t Listener::ParsePropertyUint(std::string item, uint32_t default_ /* = 0 */)
{
    std::string str= ParsePropertyStr(item);
    if (str == "")
    {
        return default_;
    }
    return Poco::NumberParser::parseUnsigned(str);
}


ConfigureListener::ConfigureListener()
        : Listener(), updateFunc(nullptr) {}

ConfigureListener::ConfigureListener(const std::string namespaceConfName)
        : Listener(namespaceConfName), updateFunc(nullptr) {}

ConfigureListener::ConfigureListener(const std::string namespaceConfName,
                                     std::function<FIELD_UPDATE_FUNC> updateFunc)
        : Listener(namespaceConfName), updateFunc(updateFunc) {}

ConfigureListener::ConfigureListener(const std::string namespaceConfName,
                                     const std::vector<std::string> keyConfNames,
                                     std::function<FIELD_UPDATE_FUNC> updateFunc)
        : Listener(namespaceConfName, keyConfNames), updateFunc(updateFunc) {}

void ConfigureListener::Update(std::string property)
{
    _property = std::move(property);
    // do some thing
}

void ConfigureListener::Update(std::vector<std::string> configurationList) {
    if (updateFunc != nullptr) {
        updateFunc(configurationList);
    }
}

void ConfigureListener::setUpdateFunc(std::function<FIELD_UPDATE_FUNC> func) {
    updateFunc = func;
}


