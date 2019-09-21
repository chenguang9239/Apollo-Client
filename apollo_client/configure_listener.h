#pragma once
#include <string>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <functional>

class Listener
{
public:
    Listener()
            : _hasUpdated(false),
              clusterName("default"),
              namespaceName("application") {}

    Listener(const std::string namespaceConfName)
            : _hasUpdated(false),
              clusterName("default"),
              namespaceName("application"),
              namespaceConfName(namespaceConfName) {}

    Listener(const std::string namespaceConfName, const std::vector<std::string> keyConfNames)
            : _hasUpdated(false),
              clusterName("default"),
              namespaceName("application"),
              namespaceConfName(namespaceConfName),
              keyConfNames(keyConfNames){}

    virtual ~Listener()
    {

    }

    virtual void Update(std::string property) = 0;
    virtual void Update(std::vector<std::string> configurationList) = 0;
    virtual bool HasUpdated();
    virtual void SetHasUpdated(bool hasUpdated);
    virtual std::string GetProperty();
    virtual std::string ParsePropertyStr(std::string item, std::string default_ = "");
    virtual int64_t ParsePropertyInt(std::string item, int default_ = 0);
    virtual uint32_t ParsePropertyUint(std::string item, uint32_t default_ = 0);
    virtual std::string GetReleaseKey();

    std::string GetAppID(){return appID;}
    void SetAppID(const std::string& id){appID = id;}

    std::string GetClusterName(){return clusterName;}
    void SetClusterName(const std::string& name){clusterName = name;}

    std::string GetNamespaceName(){return namespaceName;}
    void SetNamespaceName(const std::string& name){namespaceName = name;}
    std::string GetNamespaceConfName(){return namespaceConfName;};

    std::string GetKeyNamesAsString() {
        std::string res;
        if (keyNames.empty()) {
            res = "all keys";
        }else{
            res = SetToString(keyNames);
        }
        return res;
    }
    std::set<std::string> GetKeyNames(){return keyNames;}
    void AddToKeyNames(const std::string &keyName){keyNames.insert(keyName);}
    std::vector<std::string> GetKeyConfNames(){return keyConfNames;}

    std::string SetToString(const std::set<std::string> &keyNames){
        std::string res;
        for (auto &key : keyNames) {
            res += key;
            res += ",";
        }
        if (!res.empty()) res.pop_back();
        return res;
    }


protected:
    bool        _hasUpdated;
    std::string _property;
    std::string appID; // apollo页面上的AppId
    std::string clusterName; // 集群名称，如prod、default
    std::string namespaceName; // 名字空间名称，如application、filed
    std::string namespaceConfName; // 名字空间名称 在配置文件中的名称
    std::set<std::string> keyNames; // 键的名称，如mongo-qps、tcp-service，如果为空表示获取对应namespace下所有键-值内容
    std::vector<std::string> keyConfNames; // 键的名称 在配置文件中的名称
};

typedef void FIELD_UPDATE_FUNC(std::vector<std::string>&);

class ConfigureListener :public Listener
{
    std::function<FIELD_UPDATE_FUNC> updateFunc;
public:
    virtual void Update(std::string property);
    virtual void Update(std::vector<std::string> configurationList);
    ConfigureListener();
    ConfigureListener(const std::string namespaceConfName);
    ConfigureListener(const std::string namespaceConfName,
                      std::function<FIELD_UPDATE_FUNC> updateFunc);
    ConfigureListener(const std::string namespaceConfName,
                      const std::vector<std::string> keyConfNames,
                      std::function<FIELD_UPDATE_FUNC> updateFunc);
    void setUpdateFunc(std::function<FIELD_UPDATE_FUNC> func);
};