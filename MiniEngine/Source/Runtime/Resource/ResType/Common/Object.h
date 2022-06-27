#pragma once
#include "Runtime/Core/Meta/Reflection/Reflection.h"

#include <string>
#include <vector>

namespace ME
{
	class Component;

	REFLECTION_TYPE(ComponentDefinitionRes)
		CLASS(ComponentDefinitionRes, Fields)
	{
		REFLECTION_BODY(ComponentDefinitionRes);

	public:
		std::string m_type_name;
		std::string m_component;
	};

	REFLECTION_TYPE(ObjectDefinitionRes)
		CLASS(ObjectDefinitionRes, Fields)
	{
		REFLECTION_BODY(ObjectDefinitionRes);

	public:

	};

	REFLECTION_TYPE(ObjectInstanceRes)
		CLASS(ObjectInstanceRes, Fields)
	{
		REFLECTION_BODY(ObjectInstanceRes);

	public:
		std::string m_name;
		std::string m_definition;

	};
}