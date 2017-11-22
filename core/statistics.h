#pragma once

#include "proxy_settings.h"

namespace core
{
    class coll_helper;
    class async_executer;

    namespace tools
    {
        class binary_stream;
    }

    const static std::string flurry_url = "https://data.flurry.com/aah.do";

#ifdef DEBUG
    const static auto send_interval = std::chrono::minutes(1);
#else
    const static auto send_interval = std::chrono::hours(1);
#endif // DEBUG

    const static auto save_to_file_interval = std::chrono::seconds(10);
    const static auto delay_send_on_start = std::chrono::seconds(10);

    namespace stats
    {
        enum class stats_event_names;

        class statistics : public std::enable_shared_from_this<statistics>
        {
        private:

            class stats_event
            {
            public:
                std::string to_string(time_t _start_time) const;
                stats_event(stats_event_names _name, std::chrono::system_clock::time_point _event_time, int32_t _event_id, const event_props_type& props);
                stats_event_names get_name() const;
                event_props_type get_props() const;
                static void reset_session_event_id();
                std::chrono::system_clock::time_point get_time() const;
                int32_t get_id() const;
            private:
                stats_event_names name_;
                int32_t event_id_; // natural serial number, starting from 1
                event_props_type props_;
                static long long session_event_id_;
                std::chrono::system_clock::time_point event_time_;
            };

            struct stop_objects
            {
                std::atomic_bool is_stop_;
                stop_objects()
                {
                    is_stop_ = false;
                }
            };

            typedef std::vector<stats_event>::const_iterator events_ci;

            static std::shared_ptr<stop_objects> stop_objects_;
            std::map<std::string, tools::binary_stream> values_;
            std::wstring file_name_;

            bool changed_;
            uint32_t save_timer_;
            uint32_t send_timer_;
            uint32_t start_send_timer_;
            std::unique_ptr<async_executer> stats_thread_;
            std::vector<stats_event> events_;
            std::string events_to_json(events_ci begin, events_ci end, time_t _start_time) const;
            std::chrono::system_clock::time_point last_sent_time_;
            std::vector<std::string> get_post_data() const;
            static bool send(const proxy_settings& _user_proxy, const std::string& post_data, const std::wstring& _file_name);

            void serialize(tools::binary_stream& _bs) const;
            bool unserialize(tools::binary_stream& _bs);
            void save_if_needed();
            void send_async();
            bool load();
            void start_save();
            void start_send();
            void insert_event(stats_event_names _event_name, const event_props_type& _props,
                std::chrono::system_clock::time_point _event_time, int32_t _event_id);
            void clear();
            void delayed_start_send();
        public:
            statistics(const std::wstring& _file_name);
            virtual ~statistics();

            void init();
            void insert_event(stats_event_names _event_name, const event_props_type& _props);
            void insert_event(stats_event_names _event_name);
        };
    }
}