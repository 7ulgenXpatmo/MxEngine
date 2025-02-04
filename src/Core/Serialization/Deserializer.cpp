// Copyright(c) 2019 - 2020, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Serialization.h"
#include "Utilities/STL/MxMap.h"
#include "Utilities/Logging/Logger.h"
#include "Core/Runtime/ResourceReflection.h"
#include "Core/Runtime/HandleMappings.h"

namespace MxEngine
{
    rttr::variant VisitDeserialize(const JsonFile& json, const rttr::variant& property, HandleMappings& mappings);

    template<typename T>
    rttr::variant DeserializeGeneric(const JsonFile& json,  const rttr::type& type)
    {
        return json.empty() ? rttr::variant{ } : rttr::variant{ json.get<T>() };
    }

    rttr::variant DeserializeSequentialContainer(const JsonFile& json, const rttr::variant& property, HandleMappings& mappings)
    {
        auto view = property.create_sequential_view();
        if (view.is_dynamic())
        {
            view.clear();
            view.set_size(json.size());
        }
        size_t size = Min(view.get_size(), json.size());

        for (size_t index = 0; index < size; index++)
        {
            auto element = view.get_value(index);
            auto result = VisitDeserialize(json[index], element.extract_wrapped_value(), mappings);
            view.set_value(index, result);
        }

        return property;
    }

    rttr::variant DeserializeEnumeration(const JsonFile& json, const rttr::variant& property, HandleMappings& mappings)
    {
        return json.empty() ? rttr::variant{ } : property.get_type().get_enumeration().name_to_value(json.get<MxString>().c_str());
    }

    rttr::variant Deserialize(const JsonFile& json, const rttr::variant& property, HandleMappings& mappings)
    {
        if (IsHandle(property))
        {
            size_t handleId = json;
            return GetHandleById(property.get_type(), handleId, mappings);
        }
        else
        {
            Deserialize(json, rttr::instance{ property }, mappings);
            return property;
        }
    }

    rttr::variant VisitDeserialize(const JsonFile& json, const rttr::variant& property, HandleMappings& mappings)
    {
        #define VISITOR_DESERIALIZE_ENTRY(TYPE) { rttr::type::get<TYPE>(), DeserializeGeneric<TYPE> }
        using DeserializeCallback = rttr::variant(*)(const JsonFile&, const rttr::type&);
        static MxMap<rttr::type, DeserializeCallback> visitor = {
            VISITOR_DESERIALIZE_ENTRY(bool),
            VISITOR_DESERIALIZE_ENTRY(MxString),
            VISITOR_DESERIALIZE_ENTRY(float),
            VISITOR_DESERIALIZE_ENTRY(double),
            VISITOR_DESERIALIZE_ENTRY(int),
            VISITOR_DESERIALIZE_ENTRY(unsigned int),
            VISITOR_DESERIALIZE_ENTRY(size_t),
            VISITOR_DESERIALIZE_ENTRY(Quaternion),
            VISITOR_DESERIALIZE_ENTRY(Vector2),
            VISITOR_DESERIALIZE_ENTRY(Vector3),
            VISITOR_DESERIALIZE_ENTRY(Vector4),
        };

        auto type = property.get_type();
        if (visitor.find(type) != visitor.end())
        {
            return visitor[type](json, type);
        }
        else if (type.is_sequential_container())
        {
            return DeserializeSequentialContainer(json, property, mappings);
        }
        else if (type.is_enumeration())
        {
            return DeserializeEnumeration(json, property, mappings);
        }
        else
        {
            return Deserialize(json, property, mappings);
        }
    }

    void Deserialize(const JsonFile& json, rttr::instance object, HandleMappings& mappings)
    {
        auto type = object.get_type();

        for (auto property : type.get_properties())
        {
            ReflectionMeta propertyMeta(property);
            if ((propertyMeta.Flags & MetaInfo::SERIALIZABLE) == 0) continue;

            const char* propertyName = property.get_name().cbegin();

            if (propertyMeta.Serialization.CustomDeserialize != nullptr)
            {
                propertyMeta.Serialization.CustomDeserialize(json[propertyName], object, mappings);
            }
            else
            {
                if (json.contains(propertyName))
                {
                    auto result = VisitDeserialize(json[propertyName], property.get_value(object), mappings);
                    if (result.is_valid())
                    {
                        property.set_value(object, result);
                    }
                }
            }
        }

        if (type.get_properties().empty())
        {
            MXLOG_WARNING("MxEngine::Serializer", "no properties for type: " + MxString(object.get_type().get_name().cbegin()));
        }
    }

    void DeserializeComponentImpl(const JsonFile& json, rttr::instance object, rttr::variant handle, HandleMappings& mappings)
    {
        size_t jsonHandleId = json["id"];
        mappings.TypeToProjection[handle.get_type()][jsonHandleId] = handle;
        Deserialize(json, object, mappings);
    }
}