#include "configure_client.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSessionFactory.h>
#include <Poco/Net/HTTPSessionInstantiator.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>

#include "log.h"

ConfigureInterface *ConfigureClient::_sConfigureInstance = NULL;

ConfigureClient::ConfigureClient() {
    try {
        _pCof = new Poco::Util::IniFileConfiguration("app.ini");
        _apolloServerIp = _pCof->getString("apollo_server_ip");
        _apolloServerPort = _pCof->getUInt("apollo_server_port");
        _requestRate = _pCof->getUInt("request_rate");

        LOG_SPCL << "apollo_server_ip: " << _apolloServerIp
                 << ", apollo_server_port: " << _apolloServerPort
                 << ", request_rate: " << _requestRate
                 << "s, init appolo client ok";
    }
    catch (Poco::Exception &e) {
        LOG_ERROR << "parse error: " << e.what();
        exit(e.code());
    }
    //_timer.schedule(this, 0, _requestRate*1000);
}

ConfigureClient::ConfigureClient(const std::string cfgPath) {
    try {
        LOG_SPCL << "read appolo configuration from: " << cfgPath;
        _pCof = new Poco::Util::IniFileConfiguration(cfgPath);
        _apolloServerIp = _pCof->getString("apollo_server_ip");
        _apolloServerPort = _pCof->getUInt("apollo_server_port");
        _requestRate = _pCof->getUInt("request_rate");

        LOG_SPCL << "apollo_server_ip: " << _apolloServerIp
                 << ", apollo_server_port: " << _apolloServerPort
                 << ", request_rate: " << _requestRate
                 << "s, init appolo client ok";
    }
    catch (Poco::Exception &e) {
        LOG_ERROR << "parse error: " << e.what();
        exit(e.code());
    }
    //_timer.schedule(this, 0, _requestRate*1000);
}

ConfigureClient::ConfigureClient(std::istream &istr) {
    try {
        LOG_SPCL << "read appolo configration file information...";
        _pCof = new Poco::Util::IniFileConfiguration(istr);
        _apolloServerIp = _pCof->getString("apollo_server_ip");
        _apolloServerPort = _pCof->getUInt("apollo_server_port");
        _requestRate = _pCof->getUInt("request_rate");

        LOG_SPCL << "apollo_server_ip: " << _apolloServerIp
                 << ", apollo_server_port: " << _apolloServerPort
                 << ", request_rate: " << _requestRate
                 << "s, init appolo client ok";
    }
    catch (Poco::Exception &e) {
        LOG_ERROR << "parse error: " << e.what();
        exit(e.code());
    }

    //_timer.schedule(this, 2, _requestRate*1000);
}

ConfigureClient::~ConfigureClient() {
    _timer.cancel(true);
}

ConfigureInterface *ConfigureClient::GetConfigureInstance() {
    if (!_sConfigureInstance) {
        _sConfigureInstance = new ConfigureClient();
    }
    return _sConfigureInstance;
}

ConfigureInterface *ConfigureClient::GetConfigureInstance(const std::string cfgPath) {
    if (!_sConfigureInstance) {
        _sConfigureInstance = new ConfigureClient(cfgPath);
    }
    return _sConfigureInstance;
}

ConfigureInterface *ConfigureClient::GetConfigureInstance(std::istream &istr) {
    if (!_sConfigureInstance) {
        _sConfigureInstance = new ConfigureClient(istr);
    }
    return _sConfigureInstance;
}

// todo need to modify
void ConfigureClient::RegisterListener(Listener *listener) {
    if (listener) {
        _listenerList.push_back(listener);
        std::string appType = listener->GetAppID();
        if (_appIDList.find(appType) == _appIDList.end()) {
            std::string appID;
            try {
                appID = _pCof->getString(appType);
            }
            catch (Poco::Exception &e) {
                exit(e.code());
            }
            _appIDList.insert(std::pair<std::string, std::string>(appType, appID));
            while (!listener->HasUpdated());//?????????
        }
    }
}

void ConfigureClient::RegisterListener(std::shared_ptr<Listener> listenerPtr) {
    if (listenerPtr.get() != nullptr) {
        try {
            listenerPtr->SetAppID(_pCof->getString("conf_app_id"));
            listenerPtr->SetClusterName(_pCof->getString("cluster"));
            listenerPtr->SetNamespaceName(_pCof->getString(listenerPtr->GetNamespaceConfName()));
            for(const std::string& keyConfName: listenerPtr->GetKeyConfNames()){
                listenerPtr->AddToKeyNames(_pCof->getString(keyConfName));
            }
        } catch (Poco::Exception &e) {
            LOG_ERROR << "parse error: " << e.what();
            exit(e.code());
        }

        ListenTask tmpTask(listenerPtr->GetAppID(),
                           listenerPtr->GetClusterName(),
                           listenerPtr->GetNamespaceName(),
                           listenerPtr->GetKeyNamesAsString(),
                           listenerPtr);

        if (_listenTaskList.count(tmpTask.taskKey) == 0) {
            _listenTaskList[tmpTask.taskKey] = tmpTask;
            LOG_SPCL << "register listener ok, task key: " << tmpTask.taskKey;
        }
    }
}

void ConfigureClient::RemoveListener(Listener *listener) {
    std::vector<Listener *>::iterator it = _listenerList.begin();
    for (; it != _listenerList.end(); ++it) {
        if (*it == listener) {
            it = _listenerList.erase(it);
            return;
        }
    }
}

void ConfigureClient::RemoveListener(std::string listenTaskKey) {
    //todo
}


void ConfigureClient::NotifyListeners(std::string propertyType, std::string property) {
    std::vector<Listener *>::iterator it = _listenerList.begin();
    for (; it != _listenerList.end(); ++it) {
        if ((*it)->GetAppID() == propertyType) {
            (*it)->Update(property);
            (*it)->SetHasUpdated(true);
        }
    }
}

void ConfigureClient::run() {
    try {
        for (auto &entry : _listenTaskList) {
            std::vector<std::string> configurationList;
            if (RequestConfiguration(entry.second, configurationList))  //有最新数据
            {
                entry.second.listenerPtr->Update(configurationList);
                entry.second.listenerPtr->SetHasUpdated(true);// not use
            }
        }
    } catch (const std::exception &e) {
        LOG_ERROR << "exception: " << e.what();
    }
}

std::string ConfigureClient::LoadAppID(std::string appType) {
    if (_appIDList.find(appType) == _appIDList.end()) {
        return "";
    }
    return _appIDList[appType];
}

std::string ConfigureClient::getReleaseKey(const std::string property){
    std::string res;
    if (property.empty()) return res;

    Poco::JSON::Parser parser;
    Poco::Dynamic::Var json = parser.parse(property);
    Poco::JSON::Object::Ptr pObj = json.extract<Poco::JSON::Object::Ptr>();

    if(pObj->has("releaseKey"))
        res = pObj->getValue<std::string>("releaseKey");

    return res;
}

// vector
// element:   type1 = int defaultValue
// vector
// element:   tcp-service = serviceName1,serviceName2
std::vector<std::string> ConfigureClient::convertPropertiesToList(ListenTask &m, const std::string property) {
    std::vector<std::string> res;
    if (property.empty()) return res;

    Poco::JSON::Parser parser;
    Poco::Dynamic::Var json = parser.parse(property);
    Poco::JSON::Object::Ptr pObj = json.extract<Poco::JSON::Object::Ptr>();

    Poco::Dynamic::Var subJson = pObj->get("configurations");
    if (subJson.isEmpty()) return res;
    Poco::JSON::Object::Ptr subObj = subJson.extract<Poco::JSON::Object::Ptr>();

    std::set<std::string> returnedKeysSet;

    const std::set<std::string> &keyNames = m.listenerPtr->GetKeyNames();

    for (auto it = subObj->begin(); it != subObj->end(); ++it) {
        if (keyNames.empty() || keyNames.count(it->first) > 0) {
            returnedKeysSet.insert(it->first);
            res.emplace_back(it->first);
            res.back().append(" = ");
            res.back().append(it->second.toString());
        }
    }

    if (!keyNames.empty()) {
        if (returnedKeysSet.size() > keyNames.size()) {
            LOG_ERROR << "got too many configurations, keys are: " << m.listenerPtr->SetToString(returnedKeysSet);
        } else if (returnedKeysSet.size() < keyNames.size()) {
            std::set<std::string> notExistSet;
            for (auto &key : keyNames) {
                if (returnedKeysSet.count(key) == 0) {
                    notExistSet.insert(key);
                }
            }
            LOG_ERROR << "keys not exist: " << m.listenerPtr->SetToString(notExistSet);
        }
    }

    return res;
}

bool ConfigureClient::RequestConfiguration(ListenTask &m, std::vector<std::string> &configurationList) {
    try {
        std::string url = "/configs/";
        url += m.appID;
        url.push_back('/');
        url += m.clusterName;
        url.push_back('/');
        url += m.namespaceName;
        url += "?releaseKey=";
        // 将上一次返回对象中的releaseKey传入即可，用来给服务端比较版本，如果版本比下来没有变化，则服务端直接返回304以节省流量和运算
        url += m.releaseKey;

        Poco::Net::HTTPClientSession session(_apolloServerIp, _apolloServerPort);
        Poco::Net::HTTPRequest httpRequest(Poco::Net::HTTPRequest::HTTP_GET, url, Poco::Net::HTTPMessage::HTTP_1_1);
        Poco::Net::HTTPResponse httpResponse;

        session.sendRequest(httpRequest);
        std::istream &rs = session.receiveResponse(httpResponse);

        if (httpResponse.getStatus() == 304) {
            LOG_DEBUG << "apollo configuration not updated, url path: " << url;
            return false;
        }
        if (httpResponse.getStatus() != 200) {
            LOG_SPCL << "apollo server response code: " << httpResponse.getStatus()
                     << ", url path: " << url << ", task key: " << m.taskKey
                     << ", current ListenTask releaseKey: " << m.releaseKey;
            return false;
        }

        std::string configurationText;
        Poco::StreamCopier::copyToString(rs, configurationText);
        configurationList = convertPropertiesToList(m, configurationText);

        m.releaseKey = getReleaseKey(configurationText);

        if (_lastResponse.find(m.taskKey) == _lastResponse.end() ||
            _lastResponse[m.taskKey] != configurationList) {
            LOG_SPCL << "returned new configuration, item size: " << configurationList.size()
                     << ", url path: " << url << ", task key: " << m.taskKey
                     << ", current ListenTask releaseKey: " << m.releaseKey;
            _lastResponse[m.taskKey] = configurationList;
            return true;
        } else {
            LOG_ERROR << "other key changed in the same namespace, url path: " << url
                      << ", task key: " << m.taskKey << ", current ListenTask releaseKey: " << m.releaseKey;
            return false;
        }
    } catch (const std::exception &e) {
        LOG_ERROR << "exception: " << e.what();
    }
    return false;
}

void ConfigureClient::startListen() {
    _timer.schedule(this, 0, _requestRate * 1000);
    LOG_SPCL << "start listen, task number: " << _listenTaskList.size();
    size_t i = 1;
    for (auto &entry : _listenTaskList) {
        LOG_SPCL << "task " << i << " key: " << entry.first;
        ++i;
    }
}





