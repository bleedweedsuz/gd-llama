#ifndef GD_LLAMA_HPP
#define GD_LLAMA_HPP

#include "llama.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    class GDLLama : public Node {
        GDCLASS(GDLLama, Node);

    protected:
        static void _bind_methods();

    public:
        GDLLama();
        ~GDLLama();

        // Public API exposed to GDScript
        String system_info();
        void load_model(String path);
        String ask(String prompt);
        void set_context_size(int size);
        int get_context_size();
        void reset_memory();
        String kv_cache_info();
        void reset_context();
    private:
        int context_size = 512;
        struct llama_model *model = nullptr;
        struct llama_context *context = nullptr;
        void cleanup();
    };

} // namespace godot

#endif // GD_LLAMA_HPP