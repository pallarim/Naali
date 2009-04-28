// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "Primitive.h"
#include "RexObject.h"
#include "RexLogicModule.h"
#include "EC_OpenSimPrim.h"
#include "EC_Viewable.h"
#include "../OgreRenderingModule/EC_OgrePlaceable.h"
#include "../OgreRenderingModule/Renderer.h"
#include "ConversionUtils.h"
#include "QuatUtils.h"
#include "SceneEvents.h"

namespace RexLogic
{

    Primitive::Primitive(RexLogicModule *rexlogicmodule) : RexObject(rexlogicmodule)
    {

    }

    Primitive::~Primitive()
    {
    }

    Foundation::EntityPtr Primitive::GetOrCreatePrimEntity(Core::entity_id_t entityid, const RexUUID &fullid)
    {
        // Make sure scene exists
        Foundation::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return Foundation::EntityPtr();    
          
        Foundation::EntityPtr entity = rexlogicmodule_->GetPrimEntity(entityid);
        if (!entity)
        {
            // Create a new entity.
            Foundation::EntityPtr entity = CreateNewPrimEntity(entityid);
            rexlogicmodule_->RegisterFullId(fullid,entityid); 
            EC_OpenSimPrim &prim = *checked_static_cast<EC_OpenSimPrim*>(entity->GetComponent("EC_OpenSimPrim").get());
            prim.LocalId = entityid; ///\note In current design it holds that localid == entityid, but I'm not sure if this will always be so?
            prim.FullId = fullid;
            return entity;
        }

        // Send the 'Entity Updated' event.
        /*
        Core::event_category_id_t cat_id = framework_->GetEventManager()->QueryEventCategory("Scene");
        Foundation::ComponentInterfacePtr component = entity->GetComponent("EC_OpenSimPrim");
        EC_OpenSimPrim *prim = checked_static_cast<RexLogic::EC_OpenSimPrim *>(component.get());
        Scene::SceneEventData::Events entity_event_data(entityid);
        entity_event_data.sceneName = scene->Name();
        framework_->GetEventManager()->SendEvent(cat_id, Scene::Events::EVENT_ENTITY_UPDATED, &entity_event_data);
        */
        return entity;
    }  

    Foundation::EntityPtr Primitive::CreateNewPrimEntity(Core::entity_id_t entityid)
    {
        Foundation::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return Foundation::EntityPtr();
        
        Core::StringVector defaultcomponents;
        defaultcomponents.push_back(EC_OpenSimPrim::NameStatic());
        defaultcomponents.push_back(EC_Viewable::NameStatic());
        defaultcomponents.push_back(OgreRenderer::EC_OgrePlaceable::NameStatic());
        
        Foundation::EntityPtr entity = scene->CreateEntity(entityid,defaultcomponents); 

        DebugCreateOgreBoundingBox(entity->GetComponent(OgreRenderer::EC_OgrePlaceable::NameStatic()),"AmbientRed");
        return entity;
    }    

    bool Primitive::HandleOSNE_ObjectUpdate(OpenSimProtocol::NetworkEventInboundData* data)
    {
        NetInMessage *msg = data->message;
 
        msg->ResetReading();
        uint64_t regionhandle = msg->ReadU64();
        msg->SkipToNextVariable(); // TimeDilation U16 ///\todo Unhandled inbound variable 'TimeDilation'.

        uint32_t localid = msg->ReadU32(); 
        msg->SkipToNextVariable();		// State U8 ///\todo Unhandled inbound variable 'State'.
        RexUUID fullid = msg->ReadUUID();
        msg->SkipToNextVariable();		// CRC U32 ///\todo Unhandled inbound variable 'CRC'.
        uint8_t pcode = msg->ReadU8();

        Foundation::EntityPtr entity = GetOrCreatePrimEntity(localid,fullid);
        EC_OpenSimPrim &prim = *checked_static_cast<EC_OpenSimPrim*>(entity->GetComponent("EC_OpenSimPrim").get());
        OgreRenderer::EC_OgrePlaceable &ogrePos = *checked_static_cast<OgreRenderer::EC_OgrePlaceable*>(entity->GetComponent("EC_OgrePlaceable").get());

        ///\todo Are we setting the param or looking up by this param? I think the latter, but this is now doing the former. 
        ///      Will cause problems with multigrid support.
        prim.RegionHandle = regionhandle;

        prim.Material = msg->ReadU8();
        prim.ClickAction = msg->ReadU8();
        ogrePos.SetScale(Core::OpenSimToOgreCoordinateAxes(msg->ReadVector3()));
        
        size_t bytes_read = 0;
        const uint8_t *objectdatabytes = msg->ReadBuffer(&bytes_read);
        if (bytes_read == 60)
        {
            // The data contents:
            // ofs  0 - pos xyz - 3 x float (3x4 bytes)
            // ofs 12 - vel xyz - 3 x float (3x4 bytes)
            // ofs 24 - acc xyz - 3 x float (3x4 bytes)
            // ofs 36 - orientation, quat with last (w) component omitted - 3 x float (3x4 bytes)
            // ofs 48 - angular velocity - 3 x float (3x4 bytes)
            // total 60 bytes
            Core::Vector3df pos = *reinterpret_cast<const Core::Vector3df*>(&objectdatabytes[0]);
            ogrePos.SetPosition(Core::OpenSimToOgreCoordinateAxes(pos));
            Core::Quaternion quat = Core::UnpackQuaternionFromFloat3((float*)&objectdatabytes[36]); 
            ogrePos.SetOrientation(Core::OpenSimToOgreQuaternion(quat));

            /// \todo Velocity field unhandled.
            /// \todo Acceleration field unhandled.
            /// \todo Angular velocity field unhandled.
        }
        else
            RexLogicModule::LogError("Error reading ObjectData for prim:" + Core::ToString(prim.LocalId) + ". Bytes read:" + Core::ToString(bytes_read));
        
        prim.ParentId = msg->ReadU32();
        prim.UpdateFlags = msg->ReadU32();
        
        // Skip path related variables
        msg->SkipToFirstVariableByName("Text");
        prim.HoveringText = msg->ReadString(); 
        msg->SkipToNextVariable();      // TextColor
        prim.MediaUrl = msg->ReadString();

        return false;
    }
    
    void Primitive::HandleTerseObjectUpdateForPrim_60bytes(const uint8_t* bytes)
    {
        // The data contents:
        // ofs  0 - localid - packed to 4 bytes
        // ofs  4 - state
        // ofs  5 - 0
        // ofs  6 - position xyz - 3 x float (3x4 bytes)
        // ofs 18 - velocity xyz - packed to 6 bytes        
        // ofs 24 - acceleration xyz - packed to 6 bytes           
        // ofs 30 - rotation - packed to 8 bytes 
        // ofs 38 - rotational vel - packed to 6 bytes
        
        //! \todo handle endians
        int i = 0;
        uint32_t localid = *reinterpret_cast<uint32_t*>((uint32_t*)&bytes[i]);                
        i += 6;

        Core::Vector3df position = GetProcessedVector(&bytes[i]);
        i += sizeof(Core::Vector3df);
        
        Core::Vector3df velocity = GetProcessedScaledVectorFromUint16(&bytes[i],128);
        i += 6;
        
        Core::Vector3df accel = GetProcessedVectorFromUint16(&bytes[i]); 
        i += 6;

        Core::Quaternion rotation = GetProcessedQuaternion(&bytes[i]);
        i += 8;

        Core::Vector3df rotvel = GetProcessedVectorFromUint16(&bytes[i]);
        
        // set values
        Foundation::EntityPtr entity = rexlogicmodule_->GetPrimEntity(localid);
        if(entity)
        {
            OgreRenderer::EC_OgrePlaceable &ogrePos = *checked_static_cast<OgreRenderer::EC_OgrePlaceable*>(entity->GetComponent("EC_OgrePlaceable").get());
            ogrePos.SetPosition(position);
            ogrePos.SetOrientation(rotation);                    
        }
    }
    
    bool Primitive::HandleRexGM_RexMediaUrl(OpenSimProtocol::NetworkEventInboundData* data)
    {
        /// \todo tucofixme
        return false;
    }    

    bool Primitive::HandleRexGM_RexPrimData(OpenSimProtocol::NetworkEventInboundData* data)
    {
        size_t bytes_read;    
        data->message->ResetReading();
        data->message->SkipToFirstVariableByName("Parameter");
        RexUUID primuuid(data->message->ReadString());
        
        Foundation::EntityPtr entity = rexlogicmodule_->GetPrimEntity(primuuid);
        if(entity)
        {
            // Calculate full data size
            size_t fulldatasize = 0;        
            NetVariableType nextvartype = data->message->CheckNextVariableType();
            while(nextvartype != NetVarNone)
            {
                data->message->ReadBuffer(&bytes_read);
                fulldatasize += bytes_read;
                nextvartype = data->message->CheckNextVariableType();
            }
           
            size_t bytes_read;
            const uint8_t *readbytedata;
            data->message->ResetReading();
            data->message->SkipToFirstVariableByName("Parameter");
            data->message->SkipToNextVariable();            // Prim UUID
            
            uint8_t *fulldata = new uint8_t[fulldatasize];
            int pos = 0;
            nextvartype = data->message->CheckNextVariableType();
            while(nextvartype != NetVarNone)
            {
                readbytedata = data->message->ReadBuffer(&bytes_read);
                memcpy(fulldata+pos,readbytedata,bytes_read);
                pos += bytes_read;
                nextvartype = data->message->CheckNextVariableType();
            }

            Foundation::ComponentInterfacePtr oscomponent = entity->GetComponent("EC_OpenSimPrim");
            checked_static_cast<EC_OpenSimPrim*>(oscomponent.get())->HandleRexPrimData(fulldata);
            /// \todo tucofixme, checked_static_cast<EC_OpenSimPrim*>(oscomponent.get())->PrintDebug();

            Foundation::ComponentInterfacePtr viewcomponent = entity->GetComponent("EC_Viewable");
            checked_static_cast<EC_Viewable*>(viewcomponent.get())->HandleRexPrimData(fulldata);
            /// \todo tucofixme, checked_static_cast<EC_Viewable*>(viewcomponent.get())->PrintDebug();
            
            delete fulldata;
        }
        return false;
    }
    
    bool Primitive::HandleOSNE_KillObject(uint32_t objectid)
    {
        Foundation::ScenePtr scene = rexlogicmodule_->GetCurrentActiveScene();
        if (!scene)
            return false;

        RexTypes::RexUUID fullid;
        fullid.SetNull();
        Foundation::EntityPtr entity = rexlogicmodule_->GetPrimEntity(objectid);
        if(!entity)
            return false;

        Foundation::ComponentPtr component = entity->GetComponent("EC_OpenSimPrim");
        if(component)
        {
            EC_OpenSimPrim &prim = *checked_static_cast<EC_OpenSimPrim*>(component.get());
            fullid = prim.FullId;
        }
        
        scene->DestroyEntity(objectid);
        rexlogicmodule_->UnregisterFullId(fullid);        
        return false;
    }    

    bool Primitive::HandleOSNE_ObjectProperties(OpenSimProtocol::NetworkEventInboundData* data)
    {
        NetInMessage *msg = data->message;
        msg->ResetReading();
        
        RexUUID full_id = msg->ReadUUID();
        msg->SkipToFirstVariableByName("Name");
        std::string name = msg->ReadString();
        std::string desc = msg->ReadString();
        ///\todo Handle rest of the vars.
        
        Foundation::EntityPtr entity = rexlogicmodule_->GetPrimEntity(full_id);
        if(entity)
        {
            EC_OpenSimPrim &prim = *checked_static_cast<EC_OpenSimPrim*>(entity->GetComponent("EC_OpenSimPrim").get());
            prim.ObjectName = name;
            prim.Description = desc;
            
            // Send the 'Entity Selected' event.
            Core::event_category_id_t event_category_id = rexlogicmodule_->GetFramework()->GetEventManager()->QueryEventCategory("Scene");
            Scene::Events::SceneEventData event_data(prim.LocalId);
            rexlogicmodule_->GetFramework()->GetEventManager()->SendEvent(event_category_id, Scene::Events::EVENT_ENTITY_SELECTED, &event_data);
        }
        else
            RexLogicModule::LogInfo("Received 'ObjectProperties' packet for unknown entity (" + full_id.ToString() + ").");
        
        return false;        
    }
        
}