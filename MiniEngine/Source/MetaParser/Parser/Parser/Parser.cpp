#include "Common/Precompiled.h"
#include "LanguageTypes/Class.h"

#include "Generator/ReflectionGenerator.h"
#include "Generator/SerializerGenerator.h"
#include "Parser.h"


#define RECURSE_NAMESPACES(kind, cursor, method, namespaces) \
    { \
        if(kind == CXCursor_Namespace) \
        { \
            auto display_name = cursor.GetDisplayName(); \
            if(!display_name.empty()) \
            { \
                namespaces.emplace_back(display_name); \
                method(cursor, namespaces); \
                namespaces.pop_back(); \
            } \
        } \
    }

#define TRY_ADD_LANGUAGE_TYPE(handle, container) \
    { \
        if(handle->ShouldCompile()) \
        { \
            auto file = handle->GetSourceFile(); \
            m_schema_modules[file].container.emplace_back(handle); \
            m_type_table[handle->m_display_name] = file; \
        } \
    }

void MetaParser::Prepare(void) {}

std::string MetaParser::GetIncludeFile(std::string name)
{
    auto iter = m_type_table.find(name);
    return iter == m_type_table.end() ? std::string() : iter->second;
}

MetaParser::MetaParser( const std::string project_input_file,
                        const std::string include_file_path,
                        const std::string include_path,
                        const std::string include_sys,
                        const std::string module_name,
                        bool              is_show_errors) :
                        m_project_input_file(project_input_file),
                        m_source_include_file_name(include_file_path), m_index(nullptr), m_translation_unit(nullptr),
                        m_sys_include(include_sys), m_module_name(module_name), m_is_show_errors(is_show_errors)
{
    m_work_paths = Utils::Split(include_path, ";");

    m_generators.emplace_back(new Generator::SerializerGenerator(
        m_work_paths[0], std::bind(&MetaParser::GetIncludeFile, this, std::placeholders::_1)));

    m_generators.emplace_back(new Generator::ReflectionGenerator(
        m_work_paths[0], std::bind(&MetaParser::GetIncludeFile, this, std::placeholders::_1)));
}



MetaParser::~MetaParser(void)
{
    for(auto item : m_generators)
    {
        delete item;
    }
    m_generators.clear();

    if(m_translation_unit)
    {
        clang_disposeTranslationUnit(m_translation_unit);
    }

    if(m_index)
    {
        clang_disposeIndex(m_index);
    }
}

void MetaParser::Finish(void)
{
    for(auto generator_iter : m_generators)
    {
        generator_iter->Finish();
    }
}

bool MetaParser::ParseProject()
{
    bool result = true;
    std::cout << "Parsing project file: " << m_project_input_file << std::endl;

    std::fstream include_txt_file(m_project_input_file, std::ios::in);

    if(include_txt_file.fail())
    {
        std::cout << "Could not load file: " << m_project_input_file << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << include_txt_file.rdbuf();

    std::string context = buffer.str();
    // erase the \n char at the end
    context.pop_back();
    
    std::fstream include_file;
    include_file.open(m_source_include_file_name, std::ios::out);
    if(!include_file.is_open())
    {
        std::cout << "Could not open the Source Include files: " << m_source_include_file_name << std::endl;
    }

    std::cout << "Generating the Source Include file: " << m_source_include_file_name << std::endl;

    std::string output_file_name = Utils::GetFileName(m_source_include_file_name);

    if(output_file_name.empty())
    {
        output_file_name = "META_INPUT_HEADER_H";
    }
    else
    {
        Utils::Replace(output_file_name, ".", "_");
        Utils::Replace(output_file_name, " ", "_");
        Utils::ToUpper(output_file_name);
    }

    include_file << "#ifndef __" << output_file_name << "__" << std::endl;
    include_file << "#define __" << output_file_name << "__" << std::endl;

    auto include_files_arr = Utils::Split(context, ",");
    for(auto context : include_files_arr)
    {
        auto include_files = Utils::Split(context, ";");

        for(auto include_item : include_files)
        {
            std::string temp_string(include_item);
            Utils::Replace(temp_string, '\\', '/');
            include_file << "#include \"" << temp_string << "\"" << std::endl;
        }
        
    }

    include_file << "#endif" << std::endl; 
    include_file.close();

    return result;
}

int MetaParser::Parse(void)
{
    bool parse_include_ = ParseProject();
    if(!parse_include_)
    {
        std::cerr << "Parsing object file error!" << std::endl;
        return -1;
    }

    std::cout << "Parsing the whole project..." << std::endl;
    int is_show_errors = m_is_show_errors ? 1 : 0;
    m_index = clang_createIndex(true, is_show_errors);
    std::string pre_include = "-I";
    std::string sys_include_temp;
    if(!(m_sys_include == "*"))
    {
        sys_include_temp = pre_include + m_sys_include;
        arguments.emplace_back(sys_include_temp.c_str());
    }

    auto paths = m_work_paths;
    for(int index = 0; index < paths.size(); ++index)
    {
        paths[index] = pre_include + paths[index];
        arguments.emplace_back(paths[index].c_str());
    }

    fs::path input_path(m_source_include_file_name);
    if(!fs::exists(input_path))
    {
        std::cerr << input_path << " is not exist" << std::endl;
        return -2;
    }

    m_translation_unit = clang_createTranslationUnitFromSourceFile(m_index, m_source_include_file_name.c_str(), static_cast<int>(arguments.size()), arguments.data(), 0, nullptr);
    auto cursor = clang_getTranslationUnitCursor(m_translation_unit);

    std::vector<std::string> temp_namespace;
    
    BuildClassAST(cursor, temp_namespace);

    temp_namespace.clear();

    return 0;
}

void MetaParser::GenerateFiles(void)
{
    std::cerr << "Start generate runtime schemas(" << m_schema_modules.size() << ")..." << std::endl;
    for(auto& schema : m_schema_modules)
    {
        for(auto& generator_iter : m_generators)
        {
            generator_iter->Generate(schema.first, schema.second);
        }
    }

    Finish();
}

void MetaParser::BuildClassAST(const Cursor& cursor, std::vector<std::string> current_namespace)
{
    for(auto& child : cursor.GetChildren())
    {
        auto kind = child.GetKind();

        // actual definition and a class or struct
        if(child.IsDefinition() && (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl))
        {
            auto class_ptr = std::make_shared<Class>(child, current_namespace);

            TRY_ADD_LANGUAGE_TYPE(class_ptr, classes);
        }
        else
        {
            RECURSE_NAMESPACES(kind, child, BuildClassAST, current_namespace);
        }
    }
}