// For conditions of distribution and use, see copyright notice in license.txt

#ifndef __incl_EC_PrimFreeData_h__
#define __incl_EC_PrimFreeData_h__

#include "ComponentInterface.h"
#include "Foundation.h"


class EC_ObjFreeData : public Foundation::ComponentInterface
{
    DECLARE_EC(EC_ObjFreeData);
   
public:
    virtual ~EC_ObjFreeData();

    static std::vector<std::string> getNetworkMessages()
    {
        std::vector<std::string> myinterest;
        myinterest.push_back("GeneralMessage_EntityMetaData");
        return myinterest;
    } 

private:
    EC_ObjFreeData();

    std::string mFreeData;
};

#endif