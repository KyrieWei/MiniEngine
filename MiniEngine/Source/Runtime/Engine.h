#pragma once
#include <filesystem>


namespace ME
{
	struct EngineInitParams
	{
		std::filesystem::path m_root_folder;
		std::filesystem::path m_config_file_path;
	};

	class MiniEngine
	{

		friend class MiniEngineEditor;

	public:
		void StartEngine(const EngineInitParams& param);
		void ShutdownEngine();

		void Initialize();
		void Clear();

		void Run();

		bool TickOneFrame(float delta_time);

		int GetFPS() const { return m_fps; }

	protected:

		bool RenderTick();

		void CalculateFPS(float delta_time);

		float CalculateDeltaTime();

	protected:

		EngineInitParams m_init_params;

		bool m_is_quit{ false };

		std::chrono::steady_clock::time_point m_last_tick_time_point{ std::chrono::steady_clock::now() };

		float m_average_duration{ 0.f };
		int m_frame_count{ 0 };
		int m_fps{ 0 };
	};

}  // namespace MiniEngine

