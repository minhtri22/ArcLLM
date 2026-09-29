#pragma once

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace token_xray {

struct RuntimeLifecycleMeta {
    std::string run_id;
    std::string request_id;
    std::string model_sha256;
    std::string hardware_profile_id;
    std::string runtime_name;
    std::string runtime_version;
    std::string backend;
};

struct RuntimeLifecycleEventSpec {
    std::string event_type;
    std::string runtime_name;
    std::string representation_id;
    std::string resource_id;
    std::string execution_domain_id;
    std::vector<uint32_t> related_dispatch_ids;
    int64_t start_ns = -1;
    int64_t end_ns = -1;
    int64_t duration_ns = -1;
    std::string timestamp_source;
    std::string provenance_class = "UNKNOWN";
    std::string provenance_source_id;
    std::string provenance_note;
};

class RuntimeLifecycleTrace {
public:
    explicit RuntimeLifecycleTrace(RuntimeLifecycleMeta meta)
        : meta_(std::move(meta)) {
        if (meta_.run_id.empty()) {
            throw std::runtime_error("Token X-Ray lifecycle: empty run_id");
        }
        if (meta_.request_id.empty()) {
            throw std::runtime_error("Token X-Ray lifecycle: empty request_id");
        }
        if (meta_.runtime_name.empty()) {
            throw std::runtime_error("Token X-Ray lifecycle: empty runtime_name");
        }
    }

    uint32_t add_event(RuntimeLifecycleEventSpec spec) {
        if (!valid_event_type(spec.event_type)) {
            throw std::runtime_error(
                "Token X-Ray lifecycle: invalid event_type " + spec.event_type);
        }
        if (spec.runtime_name.empty() ||
            spec.representation_id.empty() ||
            spec.resource_id.empty()) {
            throw std::runtime_error(
                "Token X-Ray lifecycle: event identity fields must be non-empty");
        }
        if (spec.event_type == "representation_materialize" &&
            spec.execution_domain_id.empty()) {
            throw std::runtime_error(
                "Token X-Ray lifecycle: materialize requires execution_domain_id");
        }
        if (spec.start_ns >= 0 && spec.end_ns >= 0 &&
            spec.duration_ns >= 0 &&
            spec.end_ns - spec.start_ns != spec.duration_ns) {
            throw std::runtime_error(
                "Token X-Ray lifecycle: inconsistent timestamp interval");
        }

        Record r{};
        r.event_id = static_cast<uint32_t>(events_.size());
        r.spec = std::move(spec);
        events_.push_back(std::move(r));
        return events_.back().event_id;
    }

    void write_json(const std::string& path) const {
        std::ofstream o(path, std::ios::binary);
        if (!o) {
            throw std::runtime_error(
                "Token X-Ray lifecycle: cannot open output");
        }

        o << "{\n";
        o << "  \"schema_version\": \"0.1\",\n";
        o << "  \"artifact_type\": \"RUNTIME_LIFECYCLE_TRACE\",\n";
        o << "  \"run_id\": \"" << esc(meta_.run_id) << "\",\n";
        o << "  \"request_id\": \"" << esc(meta_.request_id) << "\",\n";
        o << "  \"source_model\": {\"sha256\": "
          << nullable_string(meta_.model_sha256) << "},\n";
        o << "  \"runtime\": {\"name\": \"" << esc(meta_.runtime_name)
          << "\", \"version\": " << nullable_string(meta_.runtime_version)
          << ", \"backend\": \"" << esc(meta_.backend) << "\"},\n";
        o << "  \"hardware_profile_id\": "
          << nullable_string(meta_.hardware_profile_id) << ",\n";
        o << "  \"events\": [\n";

        for (size_t i = 0; i < events_.size(); ++i) {
            const auto& e = events_[i];
            const auto& s = e.spec;
            o << "    {\n";
            o << "      \"event_id\": " << e.event_id << ",\n";
            o << "      \"event_type\": \"" << esc(s.event_type) << "\",\n";
            o << "      \"request_id\": \"" << esc(meta_.request_id) << "\",\n";
            o << "      \"runtime_name\": \"" << esc(s.runtime_name) << "\",\n";
            o << "      \"representation_id\": \"" << esc(s.representation_id) << "\",\n";
            o << "      \"resource_id\": \"" << esc(s.resource_id) << "\",\n";
            o << "      \"execution_domain_id\": "
              << nullable_string(s.execution_domain_id) << ",\n";
            o << "      \"related_dispatch_ids\": [";
            for (size_t j = 0; j < s.related_dispatch_ids.size(); ++j) {
                if (j) o << ", ";
                o << s.related_dispatch_ids[j];
            }
            o << "],\n";
            o << "      \"timestamp\": {\"source\": "
              << nullable_string(s.timestamp_source)
              << ", \"start_ns\": " << nullable_int(s.start_ns)
              << ", \"end_ns\": " << nullable_int(s.end_ns)
              << ", \"duration_ns\": " << nullable_int(s.duration_ns)
              << "},\n";
            o << "      \"provenance\": {\"class\": \""
              << esc(s.provenance_class)
              << "\", \"source_id\": "
              << nullable_string(s.provenance_source_id)
              << ", \"note\": \"" << esc(s.provenance_note)
              << "\"}\n";
            o << "    }" << (i + 1 == events_.size() ? "\n" : ",\n");
        }

        o << "  ]\n";
        o << "}\n";
    }

private:
    struct Record {
        uint32_t event_id = 0;
        RuntimeLifecycleEventSpec spec;
    };

    static bool valid_event_type(const std::string& s) {
        return s == "representation_acquire" ||
               s == "representation_materialize" ||
               s == "representation_validate" ||
               s == "representation_resident" ||
               s == "representation_reuse" ||
               s == "representation_evict" ||
               s == "representation_release";
    }

    static std::string esc(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            if (c == '\\') o << "\\\\";
            else if (c == '"') o << "\\\"";
            else if (c == '\n') o << "\\n";
            else if (c == '\r') o << "\\r";
            else if (c == '\t') o << "\\t";
            else o << c;
        }
        return o.str();
    }

    static std::string nullable_string(const std::string& s) {
        return s.empty() ? "null" : "\"" + esc(s) + "\"";
    }

    static std::string nullable_int(int64_t v) {
        return v < 0 ? "null" : std::to_string(v);
    }

    RuntimeLifecycleMeta meta_;
    std::vector<Record> events_;
};

} // namespace token_xray
