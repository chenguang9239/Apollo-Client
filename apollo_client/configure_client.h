#pragma once

#include "configure_listener.h"
#include <vector>
#include <map>
#include <Poco/Util/TimerTask.h>
#include <Poco/Util/Timer.h>
#include <Poco/Util/IniFileConfiguration.h>
#include <Poco/AutoPtr.h>
#include <memory>


class ConfigureInterface {
public:
    virtual void RegisterListener(Listener *listener) = 0;

    virtual void RemoveListener(Listener *listener) = 0;

    virtual void NotifyListeners(std::string propertyType, std::string property) = 0;

    virtual void RegisterListener(std::shared_ptr<Listener> listenerPtr) = 0;

    virtual void RemoveListener(std::string listenTaskKey) = 0;

    virtual void startListen() = 0;

protected:
    std::vector<Listener *> _listenerList; // not use
};

struct ListenTask {
    std::string appID;
    std::string clusterName;
    std::string namespaceName;
    std::string keyNames;
    std::string taskKey;
    std::string releaseKey;
    std::shared_ptr<Listener> listenerPtr;

    ListenTask() = default;

    ListenTask(const std::string &appID,
               const std::string &clusterName,
               const std::string &namespaceName,
               const std::string &keyNames,
               std::shared_ptr<Listener> listenerPtr)
            : appID(appID),
              clusterName(clusterName),
              namespaceName(namespaceName),
              keyNames(keyNames),
              releaseKey("init"),
              listenerPtr(listenerPtr) {
        taskKey += appID;
        taskKey += "|";
        taskKey += clusterName;
        taskKey += "|";
        taskKey += namespaceName;
        taskKey += "|";
        taskKey += keyNames;
    }
};

class ConfigureClient : public ConfigureInterface, public Poco::Util::TimerTask {
public:
    ~ConfigureClient();

    virtual void RegisterListener(Listener *listener);

    virtual void RemoveListener(Listener *listener);

    virtual void NotifyListeners(std::string propertyType, std::string property);

    virtual void RegisterListener(std::shared_ptr<Listener> listenerPtr);

    virtual void RemoveListener(std::string listenTaskKey);

    static ConfigureInterface *GetConfigureInstance();    //获得单例
    static ConfigureInterface *GetConfigureInstance(const std::string cfgPath);    //获得单例
    static ConfigureInterface *GetConfigureInstance(std::istream &istr);    //获得单例

    virtual void run() override;

    virtual void startListen();

    std::string LoadAppID(std::string appType);    //根据appType获得appID

protected:
    bool RequestConfiguration(ListenTask &m, std::vector<std::string> &configurationList);

private:
    ConfigureClient();

    ConfigureClient(const std::string cfgPath);

    ConfigureClient(std::istream &istr);

    std::vector<std::string> convertPropertiesToList(ListenTask &m, const std::string property);

    std::string getReleaseKey(const std::string property);

    Poco::Util::Timer _timer;
    static ConfigureInterface *_sConfigureInstance;
    std::map<std::string, std::string>     _appIDList;// not use
    std::map<std::string, ListenTask> _listenTaskList;
    std::map<std::string, std::vector<std::string>> _lastResponse;
    Poco::AutoPtr<Poco::Util::IniFileConfiguration> _pCof;
    std::string _apolloServerIp;
    uint16_t _apolloServerPort;
    uint32_t _requestRate;             //请求更新频率（s）
};