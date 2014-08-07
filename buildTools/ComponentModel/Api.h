//--------------------------------------------------------------------------------------------------
/**
 * Api class is used to represent IPC API protocols.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef API_H_INCLUDE_GUARD
#define API_H_INCLUDE_GUARD

namespace legato
{

class Api_t
{
    public:

        Api_t() {}
        Api_t(const std::string& filePath);
        Api_t(const Api_t& original) = delete;
        Api_t(Api_t&& original);

    public:

        Api_t& operator= (Api_t& original) = delete;
        Api_t& operator= (const Api_t& original) = delete;
        Api_t& operator= (Api_t&& original);

    private:

        std::string m_Name;
        std::string m_FilePath;
        std::string m_Hash;
        std::list<const Api_t*> m_Dependencies;

    public:

        const std::string& Name() const { return m_Name; }
        const std::string& FilePath() const { return m_FilePath; }

        void Hash(const char* hash) { m_Hash = hash; }
        void Hash(std::string&& hash) { m_Hash = std::move(hash); }
        const std::string& Hash() const { return m_Hash; }

        void AddDependency(Api_t* apiPtr) { m_Dependencies.push_back(apiPtr); }
        const std::list<const Api_t*>& Dependencies() const { return m_Dependencies; }

        static Api_t* GetApiPtr(const std::string& filePath);
};


}

#endif // API_H_INCLUDE_GUARD
