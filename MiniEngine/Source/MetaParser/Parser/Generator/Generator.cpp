#include "Common/Precompiled.h"

#include "Generator.h"
#include "LanguageTypes/Class.h"

namespace Generator
{
    void GeneratorInterface::PrepareStatus(std::string path)
    {
        if(!fs::exists(path))
        {
            fs::create_directories(path);
        }
    }

    void GeneratorInterface::GenClassRenderData(std::shared_ptr<Class> class_temp, Mustache::data& class_def)
    {
        class_def.set("class_name", class_temp->GetClassName());
        class_def.set("class_base_class_size", std::to_string(class_temp->m_base_classes.size()));
        class_def.set("class_need_register", true);

        if(class_temp->m_base_classes.size() > 0)
        {
            Mustache::data class_base_class_defines(Mustache::data::type::list);
            class_def.set("class_has_base", true);
            for(int index = 0; index < class_temp->m_base_classes.size(); index ++)
            {
                Mustache::data class_base_class_def;
                class_base_class_def.set("class_base_class_name", class_temp->m_base_classes[index]->name);
                class_base_class_def.set("class_base_class_index", std::to_string(index));
                class_base_class_defines.push_back(class_base_class_def);
            }
            class_def.set("class_base_class_defines", class_base_class_defines);
        }
    }

    void GeneratorInterface::GenClassFieldRenderData(std::shared_ptr<Class> class_temp, Mustache::data& field_defs)
    {
        static const std::string vector_prefix = "std::vector<";

        for (auto& field : class_temp->m_fields)
        {
            if (!field->ShouldCompile())
                continue;
            Mustache::data filed_define;

            filed_define.set("class_field_name", field->m_name);
            filed_define.set("class_field_type", field->m_type);
            filed_define.set("class_field_display_name", field->m_display_name);
            bool is_vector = field->m_type.find(vector_prefix) == 0;
            filed_define.set("class_field_is_vector", is_vector);
            field_defs.push_back(filed_define);
        }
    }
}