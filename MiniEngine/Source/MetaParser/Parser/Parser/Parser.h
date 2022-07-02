#pragma once

#include "Common/Precompiled.h"

#include "Common/SchemaModule.h"
#include "Cursor/Cursor.h"

#include "Generator/Generator.h"

class Class;

class MetaParser
{
public:
    static void Prepare(void);

    MetaParser( const std::string project_input_file,
                const std::string include_file_path,
                const std::string include_path,
                const std::string include_sys,
                const std::string module_name,
                bool              is_show_errors);

    ~MetaParser(void);

    void Finish(void);
    int Parse(void);
    void GenerateFiles(void);

private:
    std::string m_project_input_file;

    std::vector<std::string> m_work_paths;
    std::string              m_module_name;
    std::string              m_sys_include;
    std::string              m_source_include_file_name;

    CXIndex m_index;
    CXTranslationUnit m_translation_unit;

    std::unordered_map<std::string, std::string> m_type_table;
    std::unordered_map<std::string, SchemaModule> m_schema_modules;

    std::vector<const char*> arguments;

    std::vector<Generator::GeneratorInterface*> m_generators;

    bool m_is_show_errors;

private:
    bool ParseProject(void);
    void BuildClassAST(const Cursor& cursor, std::vector<std::string> current_namespace);
    std::string GetIncludeFile(std::string name);

};